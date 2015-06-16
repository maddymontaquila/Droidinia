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

public class BenchmarkHandler {
    static {
        System.loadLibrary("JNIProcessor");
    }

	private static final String TAG = "BenchmarkHandler";

    private Bitmap 	mBackBuffer;
    private int[]	mImgDims;
    private Rect	mSrcRect;
    private Rect	mTrgtRect;
    private int[]     mChoice;

    native private void runbenchmarks(Bitmap out, byte[] in, int width, int height, int[] choice);




}
