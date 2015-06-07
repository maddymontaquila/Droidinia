package com.example;

import java.io.IOException;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ImageFormat;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.hardware.Camera;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class CameraPreview {
    static {
        System.loadLibrary("JNIProcessor");
    }

	private static final String TAG = "CameraPreview";

    private Camera 	mCamera;
    private byte[] 	mVideoSource;
    private Bitmap 	mBackBuffer;
    private int[]	mImgDims;
    private Paint 	mPaint;
    private Rect	mSrcRect;
    private Rect	mTrgtRect;
    private int     mChoice;

    native private void runfilter(Bitmap out, byte[] in, int width, int height, int choice);

    public void setProcessedPreview(int choice) {
    	mChoice = choice;
        if (choice==0){
            //
        }
        else if (choice==1){
            //
        }
    }

    public void onPreviewFrame(byte[] data, Camera camera) {
    	try {
            runfilter(mBackBuffer,data,mImgDims[0],mImgDims[1],mChoice);
    	} catch(Exception e) {
    		Log.i(TAG, e.getMessage());
    	}
        
    }

}
