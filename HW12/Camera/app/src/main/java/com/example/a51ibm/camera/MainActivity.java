package com.example.a51ibm.camera;

// libraries
import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.view.WindowManager;
import android.widget.TextView;
import java.io.IOException;
import static android.graphics.Color.blue;
import static android.graphics.Color.green;
import static android.graphics.Color.red;
import static java.lang.Math.acos;
import static java.lang.Math.sqrt;

import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;




public class MainActivity extends Activity implements TextureView.SurfaceTextureListener {
    private Camera mCamera;
    private TextureView mTextureView;
    private SurfaceView mSurfaceView;
    private SurfaceHolder mSurfaceHolder;
    private Bitmap bmp = Bitmap.createBitmap(640,480,Bitmap.Config.ARGB_8888);
    private Canvas canvas = new Canvas(bmp);
    private Paint paint1 = new Paint();
    private TextView mTextView;

    SeekBar myControl;
    int x;

    private void setMyControlListener() {
        myControl.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

            int progressChanged = 0;

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                progressChanged = progress;
                x = progress * 3 + 450;
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
    }

    static long prevtime = 0; // for FPS calculation

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON); // keeps the screen from turning off

        mSurfaceView = (SurfaceView) findViewById(R.id.surfaceview);
        mSurfaceHolder = mSurfaceView.getHolder();

        mTextureView = (TextureView) findViewById(R.id.textureview);
        mTextureView.setSurfaceTextureListener(this);

        mTextView = (TextView) findViewById(R.id.cameraStatus);

        myControl = (SeekBar) findViewById(R.id.seek1);

        paint1.setColor(0xffff0000); // red
        paint1.setTextSize(24);

        setMyControlListener();
    }

    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        mCamera = Camera.open();
        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setPreviewSize(640, 480);

        //parameters.setColorEffect(Camera.Parameters.EFFECT_MONO); // black and white
        parameters.setColorEffect(Camera.Parameters.EFFECT_NONE);
        parameters.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_DAYLIGHT);

        parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_INFINITY); // no autofocusing

//        parameters.setFlashMode(Camera.Parameters.FLASH_MODE_TORCH);                                                       //!!!!!!!!!!!!!!!!

        mCamera.setParameters(parameters);
        mCamera.setDisplayOrientation(90); // rotate to portrait mode

        try {
            mCamera.setPreviewTexture(surface);
            mCamera.startPreview();
        } catch (IOException ioe) {
            // Something bad happened
        }
    }

    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
        // Ignored, Camera does all the work for us
    }

    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        mCamera.stopPreview();
        mCamera.release();
        return true;
    }

    // the important function
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {
        // Invoked every time there's a new Camera preview frame
        mTextureView.getBitmap(bmp);

        final Canvas c = mSurfaceHolder.lockCanvas();
        if (c != null) {

            int[] pixels = new int[bmp.getWidth()];
            int[] pixels2 = new int[bmp.getWidth()];
            int[] pixels3 = new int[bmp.getWidth()];



            //int startY = 15; // which row in the bitmap to analyse to read
            int startY = 360; // which row in the bitmap to analyse to read
            int startY2 = 370;
            int startY3 = 320;

            // only look at one row in the image
            bmp.getPixels(pixels, 0, bmp.getWidth(), 0, startY, bmp.getWidth(), 1); // (array name, offset inside array, stride (size of row), start x, start y, num pixels to read per row, num rows to read)
            bmp.getPixels(pixels2, 0, bmp.getWidth(), 0, startY2, bmp.getWidth(), 1); // (array name, offset inside array, stride (size of row), start x, start y, num pixels to read per row, num rows to read)
            bmp.getPixels(pixels3, 0, bmp.getWidth(), 0, startY3, bmp.getWidth(), 1); // (array name, offset inside array, stride (size of row), start x, start y, num pixels to read per row, num rows to read)


            // pixels[] is the RGBA data (in black an white).
            // instead of doing center of mass on it, decide if each pixel is dark enough to consider black or white
            // then do a center of mass on the thresholded array
            int[] thresholdedPixels = new int[bmp.getWidth()];
            int[] thresholdedPixels2 = new int[bmp.getWidth()];
            int[] thresholdedPixels3 = new int[bmp.getWidth()];

            int wbTotal = 0; // total mass
            int wbCOM = 0; // total (mass time position)
            for (int i = 0; i < bmp.getWidth(); i++) {
                // sum the red, green and blue, subtract from 255 to get the darkness of the pixel.
                // if it is greater than some value (600 here), consider it black
                // play with the 600 value if you are having issues reliably seeing the line

                //if (255*3-(red(pixels[i])+green(pixels[i])+blue(pixels[i])) > x) {
                if (red(pixels[i])>green(pixels[i]) && red(pixels[i])>blue(pixels[i])){
                    if (red(pixels[i])>80) {

                        thresholdedPixels[i] = 255*3;
                    }
                    else {
                        thresholdedPixels[i] = 0;
                    }
                    wbTotal = wbTotal + thresholdedPixels[i];
                    wbCOM = wbCOM + thresholdedPixels[i]*i;
                }

            }
            int COM;
            //watch out for divide by 0
            if (wbTotal<=0) {
                COM = bmp.getWidth()/2;
            }
            else {
                COM = wbCOM/wbTotal;
            }

            int wbTotal2 = 0; // total mass
            int wbCOM2 = 0; // total (mass time position)
            for (int j = 0; j < bmp.getWidth(); j++) {
                // sum the red, green and blue, subtract from 255 to get the darkness of the pixel.
                // if it is greater than some value (600 here), consider it black
                // play with the 600 value if you are having issues reliably seeing the line

                //if (255*3-(red(pixels[i])+green(pixels[i])+blue(pixels[i])) > x) {
                if (red(pixels2[j])>100) {

                    thresholdedPixels2[j] = 255*3;
                }
                else {
                    thresholdedPixels2[j] = 0;
                }
                wbTotal2 = wbTotal2 + thresholdedPixels2[j];
                wbCOM2 = wbCOM2 + thresholdedPixels2[j]*j;
            }
            int COM2;
            //watch out for divide by 0
            if (wbTotal2<=0) {
                COM2 = bmp.getWidth()/2;
            }
            else {
                COM2 = wbCOM2/wbTotal2;
            }
            int wbTotal3 = 0; // total mass
            int wbCOM3 = 0; // total (mass time position)
            for (int i = 0; i < bmp.getWidth(); i++) {
                // sum the red, green and blue, subtract from 255 to get the darkness of the pixel.
                // if it is greater than some value (600 here), consider it black
                // play with the 600 value if you are having issues reliably seeing the line

                //if (255*3-(red(pixels[i])+green(pixels[i])+blue(pixels[i])) > x) {
                if (red(pixels3[i])>100) {

                    thresholdedPixels3[i] = 255*3;
                }
                else {
                    thresholdedPixels3[i] = 0;
                }
                wbTotal3 = wbTotal3 + thresholdedPixels3[i];
                wbCOM3 = wbCOM3 + thresholdedPixels3[i]*i;
            }
            int COM3;
            //watch out for divide by 0
            if (wbTotal3<=0) {
                COM3 = bmp.getWidth()/2;
            }
            else {
                COM3 = wbCOM3/wbTotal3;
            }


            int x2;
            x2 = COM - COM2;
            int y2 = startY - startY2;
            int x3 = COM - COM3;
            int y3 = startY - startY3;

            int angle;
            angle = (int)acos((x2*x3+y2*y3)/(sqrt(x2^2+y2^2)+sqrt(x3^2+y3^2)));

            // draw a circle where you think the COM is
            canvas.drawCircle(COM, startY, 5, paint1);
            canvas.drawCircle(COM2, startY2, 5, paint1);
            canvas.drawCircle(COM3, startY3, 5, paint1);



            // also write the value as text
            canvas.drawText("COM = " + COM, 10, 200, paint1);
            canvas.drawText("Angle = " + angle, 10, 280, paint1);
            canvas.drawText("x2 = " + angle, 10, 300, paint1);
            canvas.drawText("y2 = " + angle, 10, 320, paint1);
            canvas.drawText("x3 = " + angle, 10, 340, paint1);
            canvas.drawText("y3 = " + angle, 10, 360, paint1);
            canvas.drawText("COM2 = " + COM2, 10, 380, paint1);



            canvas.drawText("R = " + red(pixels[COM]), 10, 220, paint1);
            canvas.drawText("G = " + green(pixels[COM]), 10, 240, paint1);
            canvas.drawText("B = " + blue(pixels[COM]), 10, 260, paint1);

            c.drawBitmap(bmp, 0, 0, null);
            mSurfaceHolder.unlockCanvasAndPost(c);

            // calculate the FPS to see how fast the code is running
            long nowtime = System.currentTimeMillis();
            long diff = nowtime - prevtime;
            mTextView.setText("FPS " + 1000/diff);
            prevtime = nowtime;
        }
    }
}

