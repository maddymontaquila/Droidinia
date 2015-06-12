#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS

#include "processor.h"

using namespace std; //from timer.cc

//notes, things left to look at here: examine the run file(figure out what to do about data).
//also the makefile in bfs.
//NOT KNOWN: issues involved with bringing over a bunch. Need to see if include guards covered major
//issues and if things have been brought over correctly
//Q: the Node struct is currently both defined in kernels.cl and in processor.cpp. Problem?
//Also need to figure out use of bfs make.config file in determining determination between nvidia and amd



static cl::Context      gContext;
static cl::CommandQueue gQueue;
static cl::Kernel       gNV21Kernel;
static cl::Kernel       gLaplacianK;
static cl::Kernel		gbfs; //also, bfs has two separate kernels for different numbers of arguments.
							  //I have yet to look into whatever each does.


char *file_contents(const char *filename, int *length)
{
    FILE *f = fopen(filename, "r");
    void *buffer;

    if (!f) {
        LOGE("Unable to open %s for reading\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    *length = ftell(f);
    fseek(f, 0, SEEK_SET);

    buffer = malloc(*length+1);
    *length = fread(buffer, 1, *length, f);
    fclose(f);
    ((char*)buffer)[*length] = '\0';

    return (char*)buffer;
}

bool throwJavaException(JNIEnv *env,std::string method_name,std::string exception_msg, int errorCode=0)
{
    char buf[8];
    sprintf(buf,"%d",errorCode);
    std::string code(buf);

    std::string msg = "@" + method_name + ": " + exception_msg + " ";
    if(errorCode!=0) msg += code;

    jclass generalExp = env->FindClass("java/lang/Exception");
    if (generalExp != 0) {
        env->ThrowNew(generalExp, msg.c_str());
        return true;
    }
    return false;
}

void cb(cl_program p,void* data)
{
    clRetainProgram(p);
    cl_device_id devid[1];
    clGetProgramInfo(p,CL_PROGRAM_DEVICES,sizeof(cl_device_id),(void*)devid,NULL);
    char bug[65536];
    clGetProgramBuildInfo(p,devid[0],CL_PROGRAM_BUILD_LOG,65536*sizeof(char),bug,NULL);
    clReleaseProgram(p);
    LOGE("Build log \n %s\n",bug);
}

JNIEXPORT jboolean JNICALL Java_com_example_LiveFeatureActivity_compileKernels(JNIEnv *env, jclass clazz)
{
    // Find OCL devices and compile kernels
    cl_int err = CL_SUCCESS;
    try {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        if (platforms.size() == 0) {
            return false;
        }
        cl_context_properties properties[] =
        { CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};
        gContext = cl::Context(CL_DEVICE_TYPE_GPU, properties);
        std::vector<cl::Device> devices = gContext.getInfo<CL_CONTEXT_DEVICES>();
        gQueue = cl::CommandQueue(gContext, devices[0], 0, &err);
        int src_length = 0;
		const char* src  = file_contents("/data/data/com.example/app_execdir/kernels.cl",&src_length);
        cl::Program::Sources sources(1,std::make_pair(src, src_length) );
        cl::Program program(gContext, sources);
        program.build(devices,NULL,cb);
        while(program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) != CL_BUILD_SUCCESS);
        gNV21Kernel = cl::Kernel(program, "nv21torgba", &err);
        gLaplacianK = cl::Kernel(program, "laplacian", &err);
        return true;
    }
    catch (cl::Error e) {
        if( !throwJavaException(env,"decode",e.what(),e.err()) )
            LOGI("@decode: %s \n",e.what());
        return false;
    }
}

void helper(uint32_t* out, int osize, uint8_t* in, int isize, int w, int h, int choice[])
{
	int set_NDRange_size=16;

    try {
        cl::Buffer bufferIn = cl::Buffer(gContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                isize*sizeof(cl_uchar), in, NULL);
        cl::Buffer bufferOut = cl::Buffer(gContext, CL_MEM_READ_WRITE, osize*sizeof(cl_uchar4));
        cl::Buffer bufferOut2= cl::Buffer(gContext, CL_MEM_READ_WRITE, osize*sizeof(cl_uchar4));

        ////do we need to declare the buffer sizes????????////


        if (choice[0]==1) {
        	/***ZACH*** Here is where you're going to do BFS
        	 * I don't know what sequential number bfs is yet. I'll figure it out eventually.....
        	 */
            gLaplacianK.setArg(2,w);
            gLaplacianK.setArg(3,h);
            gLaplacianK.setArg(1,bufferOut);
            gLaplacianK.setArg(0,bufferOut2);
            gQueue.enqueueNDRangeKernel(gLaplacianK,
                    cl::NullRange,
                    cl::NDRange( (int)ceil((float)w/16.0f)*16,(int)ceil((float)h/16.0f)*16),
                    cl::NDRange(set_NDRange_size,set_NDRange_size),
                    NULL,
                    NULL);
        }
        gQueue.enqueueReadBuffer(bufferOut2, CL_TRUE, 0, osize*sizeof(cl_uchar4), out);



    }
    catch (cl::Error e) {
        LOGI("@oclDecoder: %s %d \n",e.what(),e.err());
    }
}

JNIEXPORT void JNICALL Java_com_example_LiveFeatureActivity_runbenchmarks(
        JNIEnv *env,
        jclass clazz,
        jobject output,
        jbyteArray inData,
        jint width,
        jint height,
        jint choice[])
{
    int outsz = width*height;
    int insz = outsz + outsz/2;

    uint32_t* outContent;

    jbyte* inPtr = (jbyte*)env->GetPrimitiveArrayCritical(inData, 0);
    if (inPtr == NULL) {
        throwJavaException(env,"gaussianBlur","NV21 byte stream getPointer returned NULL");
        return;
    }
    // call helper for processing frame
    helper(outContent,outsz,(uint8_t*)inPtr,insz,width,height,choice);
    // This is absolutely necessary before calling any other JNI function
    env->ReleasePrimitiveArrayCritical(inData,inPtr,0);
    //output
}


/* ------------------------------------------------------
 * FROM timer.cc
 ------------------------------------------------------*/

double timer::CPU_speed_in_MHz = timer::get_CPU_speed_in_MHz();

double timer::get_CPU_speed_in_MHz()
{
#if defined __linux__
    ifstream infile("/proc/cpuinfo");
    char     buffer[256], *colon;

    while (infile.good()) {
	infile.getline(buffer, 256);

	if (strncmp("cpu MHz", buffer, 7) == 0 && (colon = strchr(buffer, ':')) != 0)
	    return atof(colon + 2);
    }
#endif

    return 0.0;
}

void timer::print_time(ostream &str, const char *which, double time) const
{
    static const char *units[] = { " ns", " us", " ms", "  s", " ks", 0 };
    const char	      **unit   = units;

    time = 1000.0 * time / CPU_speed_in_MHz;

    while (time >= 999.5 && unit[1] != 0) {
	time /= 1000.0;
	++ unit;
    }

    str << which << " = " << setprecision(3) << setw(4) << time << *unit;
}

ostream &timer::print(ostream &str)
{
    str << left << setw(25) << (name != 0 ? name : "timer") << ": " << right;

    if (CPU_speed_in_MHz == 0)
	str << "could not determine CPU speed\n";
    else if (count > 0) {
	double total = static_cast<double>(total_time);

	print_time(str, "avg", total / static_cast<double>(count));
	print_time(str, ", total", total);
	str << ", count = " << setw(9) << count << '\n';
    }
    else
	str << "not used\n";

    return str;
}

ostream &operator << (ostream &str, class timer &timer)
{
    return timer.print(str);
}

double timer::getTimeInSeconds()
{
    double total = static_cast<double>(total_time);
    double res = (total / 1000000.0) / CPU_speed_in_MHz;
    return res;
}



/*--------------------------------------
 * FROM bfs.cpp:
 */
//Structure to hold a node information, also found in kernels.cl
struct Node
{
	int starting;
	int no_of_edges;
};

//----------------------------------------------------------
//--breadth first search on GPUs
//----------------------------------------------------------
void run_bfs_gpu(int no_of_nodes, Node *h_graph_nodes, int edge_list_size, \
		int *h_graph_edges, char *h_graph_mask, char *h_updating_graph_mask, \
		char *h_graph_visited, int *h_cost)
					throw(std::string){

	//int number_elements = height*width;
	char h_over;
	cl_mem d_graph_nodes, d_graph_edges, d_graph_mask, d_updating_graph_mask, \
			d_graph_visited, d_cost, d_over;
	try{
		//--1 transfer data from host to device
		_clInit();
		d_graph_nodes = _clMalloc(no_of_nodes*sizeof(Node), h_graph_nodes);
		d_graph_edges = _clMalloc(edge_list_size*sizeof(int), h_graph_edges);
		d_graph_mask = _clMallocRW(no_of_nodes*sizeof(char), h_graph_mask);
		d_updating_graph_mask = _clMallocRW(no_of_nodes*sizeof(char), h_updating_graph_mask);
		d_graph_visited = _clMallocRW(no_of_nodes*sizeof(char), h_graph_visited);


		d_cost = _clMallocRW(no_of_nodes*sizeof(int), h_cost);
		d_over = _clMallocRW(sizeof(char), &h_over);

		_clMemcpyH2D(d_graph_nodes, no_of_nodes*sizeof(Node), h_graph_nodes);
		_clMemcpyH2D(d_graph_edges, edge_list_size*sizeof(int), h_graph_edges);
		_clMemcpyH2D(d_graph_mask, no_of_nodes*sizeof(char), h_graph_mask);
		_clMemcpyH2D(d_updating_graph_mask, no_of_nodes*sizeof(char), h_updating_graph_mask);
		_clMemcpyH2D(d_graph_visited, no_of_nodes*sizeof(char), h_graph_visited);
		_clMemcpyH2D(d_cost, no_of_nodes*sizeof(int), h_cost);

		//--2 invoke kernel
#ifdef	PROFILING
		timer kernel_timer;
		double kernel_time = 0.0;
		kernel_timer.reset();
		kernel_timer.start();
#endif
		do{
			h_over = false;
			_clMemcpyH2D(d_over, sizeof(char), &h_over);
			//--kernel 0
			int kernel_id = 0;
			int kernel_idx = 0;
			_clSetArgs(kernel_id, kernel_idx++, d_graph_nodes);
			_clSetArgs(kernel_id, kernel_idx++, d_graph_edges);
			_clSetArgs(kernel_id, kernel_idx++, d_graph_mask);
			_clSetArgs(kernel_id, kernel_idx++, d_updating_graph_mask);
			_clSetArgs(kernel_id, kernel_idx++, d_graph_visited);
			_clSetArgs(kernel_id, kernel_idx++, d_cost);
			_clSetArgs(kernel_id, kernel_idx++, &no_of_nodes, sizeof(int));

			//int work_items = no_of_nodes;
			_clInvokeKernel(kernel_id, no_of_nodes, work_group_size);

			//--kernel 1
			kernel_id = 1;
			kernel_idx = 0;
			_clSetArgs(kernel_id, kernel_idx++, d_graph_mask);
			_clSetArgs(kernel_id, kernel_idx++, d_updating_graph_mask);
			_clSetArgs(kernel_id, kernel_idx++, d_graph_visited);
			_clSetArgs(kernel_id, kernel_idx++, d_over);
			_clSetArgs(kernel_id, kernel_idx++, &no_of_nodes, sizeof(int));

			//work_items = no_of_nodes;
			_clInvokeKernel(kernel_id, no_of_nodes, work_group_size);

			_clMemcpyD2H(d_over,sizeof(char), &h_over);
			}while(h_over);

		_clFinish();
#ifdef	PROFILING
		kernel_timer.stop();
		kernel_time = kernel_timer.getTimeInSeconds();
#endif
		//--3 transfer data from device to host
		_clMemcpyD2H(d_cost,no_of_nodes*sizeof(int), h_cost);
		//--statistics
#ifdef	PROFILING
		std::cout<<"kernel time(s):"<<kernel_time<<std::endl;
#endif
		//--4 release cl resources.
		_clFree(d_graph_nodes);
		_clFree(d_graph_edges);
		_clFree(d_graph_mask);
		_clFree(d_updating_graph_mask);
		_clFree(d_graph_visited);
		_clFree(d_cost);
		_clFree(d_over);
		_clRelease();
	}
	catch(std::string msg){
		_clFree(d_graph_nodes);
		_clFree(d_graph_edges);
		_clFree(d_graph_mask);
		_clFree(d_updating_graph_mask);
		_clFree(d_graph_visited);
		_clFree(d_cost);
		_clFree(d_over);
		_clRelease();
		std::string e_str = "in run_transpose_gpu -> ";
		e_str += msg;
		throw(e_str);
	}
	return ;
}
void Usage(int argc, char**argv){

fprintf(stderr,"Usage: %s <input_file>\n", argv[0]);

}
//----------------------------------------------------------
//--cambine:	main function
//--author:		created by Jianbin Fang
//--date:		25/01/2011
//----------------------------------------------------------
int mainRunBFS(char * argv[])
{

	int no_of_nodes;
	int edge_list_size;
	FILE *fp;
	int argc = 4; //was originally a main argument of function
	Node* h_graph_nodes;
	char *h_graph_mask, *h_updating_graph_mask, *h_graph_visited;
	try{
		char *input_f;
		if(argc!=2){
		  Usage(argc, argv);
		  exit(0);
		}

		input_f = argv[1];
		printf("Reading File\n");
		//Read in Graph from a file
		fp = fopen(input_f,"r");
		if(!fp){
		  printf("Error Reading graph file\n");
		  return -1;
		}

		int source = 0;

		fscanf(fp,"%d",&no_of_nodes);

		int num_of_blocks = 1;
		int num_of_threads_per_block = no_of_nodes;

		//Make execution Parameters according to the number of nodes
		//Distribute threads across multiple Blocks if necessary
		if(no_of_nodes>MAX_THREADS_PER_BLOCK){
			num_of_blocks = (int)ceil(no_of_nodes/(double)MAX_THREADS_PER_BLOCK);
			num_of_threads_per_block = MAX_THREADS_PER_BLOCK;
		}
		work_group_size = num_of_threads_per_block;
		// allocate host memory
		h_graph_nodes = (Node*) malloc(sizeof(Node)*no_of_nodes);
		h_graph_mask = (char*) malloc(sizeof(char)*no_of_nodes);
		h_updating_graph_mask = (char*) malloc(sizeof(char)*no_of_nodes);
		h_graph_visited = (char*) malloc(sizeof(char)*no_of_nodes);

		int start, edgeno;
		// initalize the memory
		for(int i = 0; i < no_of_nodes; i++){
			fscanf(fp,"%d %d",&start,&edgeno);
			h_graph_nodes[i].starting = start;
			h_graph_nodes[i].no_of_edges = edgeno;
			h_graph_mask[i]=false;
			h_updating_graph_mask[i]=false;
			h_graph_visited[i]=false;
		}
		//read the source node from the file
		fscanf(fp,"%d",&source);
		source=0;
		//set the source node as true in the mask
		h_graph_mask[source]=true;
		h_graph_visited[source]=true;
    	fscanf(fp,"%d",&edge_list_size);
   		int id,cost;
		int* h_graph_edges = (int*) malloc(sizeof(int)*edge_list_size);
		for(int i=0; i < edge_list_size ; i++){
			fscanf(fp,"%d",&id);
			fscanf(fp,"%d",&cost);
			h_graph_edges[i] = id;
		}

		if(fp)
			fclose(fp);
		// allocate mem for the result on host side
		int	*h_cost = (int*) malloc(sizeof(int)*no_of_nodes);
		int *h_cost_ref = (int*)malloc(sizeof(int)*no_of_nodes);
		for(int i=0;i<no_of_nodes;i++){
			h_cost[i]=-1;
			h_cost_ref[i] = -1;
		}
		h_cost[source]=0;
		h_cost_ref[source]=0;
		//---------------------------------------------------------
		//--gpu entry
		run_bfs_gpu(no_of_nodes,h_graph_nodes,edge_list_size,h_graph_edges,
				h_graph_mask, h_updating_graph_mask, h_graph_visited, h_cost);
		//---------------------------------------------------------

		//release host memory
		free(h_graph_nodes);
		free(h_graph_mask);
		free(h_updating_graph_mask);
		free(h_graph_visited);

	}
	catch(std::string msg){
		std::cout<<"--combine: exception in main ->"<<msg<<std::endl;
		//release host memory
		free(h_graph_nodes);
		free(h_graph_mask);
		free(h_updating_graph_mask);
		free(h_graph_visited);
	}

    return 0;
}





