package com.example;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.FrameLayout;

public class LiveFeatureActivity extends Activity {

	static {
        System.loadLibrary("JNIProcessor");
    }

    private final String TAG="LiveFeature";

	native private boolean compileKernels();
	native private void runbenchmarks(Bitmap out, byte[] in, int width, int height, int choice);

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

        
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.live_feature, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
	    switch (item.getItemId()) {
	        default:
	            return super.onOptionsItemSelected(item);
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
