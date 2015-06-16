package com.example;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.FrameLayout;
import android.widget.TextView;

public class LiveFeatureActivity extends Activity {

	static {
        System.loadLibrary("JNIProcessor");
    }

    private final String TAG="RodiniaBenchmarks";
    private int[]     mChoice;
    private CheckBox leukocyte, heartwall, cfdsolver, ludecomp, hotspot, backpropogation;
    private CheckBox needleman, kmeans, bfs, srad, streamcluster, particlefilter;
    private CheckBox pathfinder, gaussian, nearestneighbors, lavamd, myocyte, btree, gpudwt, hybridsort;
    private Button runbutton;
    private Button selectallbutton;
    private TextView tv;

	native private boolean compileKernels();
	native private void runbenchmarks(int[] choice);

    private void copyFile(final String f) {
		InputStream in;
		try {
			//creates the output data
			in = getAssets().open(f);
			final File of = new File(getDir("execdir",MODE_PRIVATE), f);

			final OutputStream out = new FileOutputStream(of);

			final byte b[] = new byte[65535];
			int sz = 0;
			while ((sz = in.read(b)) > 0) {
				out.write(b, 0, sz);
			}
			in.close();
			out.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_live_feature);
        copyFile("kernels.cl");

        if( compileKernels() == false )
            Log.i(TAG,"Kernel compilation failed");

        addListenerOnRunButton();
	}
	
	public void addListenerOnRunButton() {
	     runbutton = (Button) findViewById(R.id.runbutton);
	     
	     runbutton.setOnClickListener(new OnClickListener(){

			public void onClick(View v) {
				startTests();
			}
	     });
	}
	
	
    public void startTests() {
    	tv = (TextView) findViewById(R.id.tv);
    	
    	//create checkboxes for each option
    	leukocyte = (CheckBox) findViewById(R.id.leukocyte);
    	heartwall = (CheckBox) findViewById(R.id.heartwall);
    	cfdsolver = (CheckBox) findViewById(R.id.cfdsolver);
    	ludecomp = (CheckBox) findViewById(R.id.ludecomp);
    	hotspot = (CheckBox) findViewById(R.id.hotspot);
    	backpropogation = (CheckBox) findViewById(R.id.backpropogation);
    	needleman = (CheckBox) findViewById(R.id.needleman);
    	kmeans = (CheckBox) findViewById(R.id.kmeans);
    	bfs = (CheckBox) findViewById(R.id.bfs);
    	srad = (CheckBox) findViewById(R.id.srad);
    	streamcluster = (CheckBox) findViewById(R.id.streamcluster);
    	particlefilter = (CheckBox) findViewById(R.id.particlefilter);
    	pathfinder = (CheckBox) findViewById(R.id.pathfinder);
    	gaussian = (CheckBox) findViewById(R.id.gaussian);
    	nearestneighbors = (CheckBox) findViewById(R.id.nearestneighbors);
    	lavamd = (CheckBox) findViewById(R.id.lavamd);
    	myocyte = (CheckBox) findViewById(R.id.myocyte);
    	btree = (CheckBox) findViewById(R.id.btree);
    	gpudwt = (CheckBox) findViewById(R.id.gpudwt);
    	hybridsort = (CheckBox) findViewById(R.id.hybridsort);
    	
    	int[] choice = new int[19];
    	
    	if (leukocyte.isChecked())
    		choice[0]=1;
    	else
    		choice[0]=0;
    	
    	
    	mChoice=choice;
    	try {
            //runbenchmarks(mChoice);
            tv.setText("Running Tests...");
    	} catch(Exception e) {
    		Log.i(TAG, e.getMessage());
    	}
        
    }
	
	@Override
	protected void onPause() {
		super.onPause();
	}

	@Override
	protected void onResume() {
		super.onResume();
	}
}
