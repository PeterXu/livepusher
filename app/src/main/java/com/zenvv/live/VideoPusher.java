package com.zenvv.live;

import java.util.Iterator;
import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;

import com.zenvv.live.jni.PusherNative;

public class VideoPusher extends Pusher implements Callback, PreviewCallback {
	private final static String TAG = "VideoPusher";
	
	private boolean mPreviewRunning;
	private Camera mCamera;
	private SurfaceHolder mHolder;
	private VideoParam mParam;
	
	private byte[] mBuffer;
	private byte[] mRaw;
	private Activity mActivity;
	
	private int mScreen;
	private final static int SCREEN_PORTRAIT = 0;
	private final static int SCREEN_LANDSCAPE_LEFT = 90;
    private final static int SCREEN_PORTRAIT_REVERSE = 180;
	private final static int SCREEN_LANDSCAPE_RIGHT = 270;

	public VideoPusher(Activity activity, SurfaceHolder surfaceHolder,
			VideoParam param, PusherNative pusherNative) {
		super(pusherNative);
		
		mActivity = activity;
		mParam = param;
		mHolder = surfaceHolder;
		surfaceHolder.addCallback(this);
	}

	@Override
	public void startPusher() {
		startPreview();
		mPusherRuning = true;
	}

	@Override
	public void stopPusher() {
		mPusherRuning = false;
	}

	@Override
	public void release() {
		mPusherRuning = false;
		mActivity = null;
		stopPreview();
	}

	public void switchCamera() {
		if (mParam.getCameraId() == CameraInfo.CAMERA_FACING_BACK) {
			mParam.setCameraId(CameraInfo.CAMERA_FACING_FRONT);
		} else {
			mParam.setCameraId(CameraInfo.CAMERA_FACING_BACK);
		}
		
		stopPreview();
		startPreview();
	}

	private void stopPreview() {
		if (mPreviewRunning && mCamera != null) {
			mCamera.setPreviewCallback(null);
			mCamera.stopPreview();
			mCamera.release();
			mCamera = null;
			mPreviewRunning = false;
		}
	}

	@SuppressWarnings("deprecation")
	private void startPreview() {
		if (mPreviewRunning) {
			return;
		}
		try {
			mCamera = Camera.open(mParam.getCameraId());
			Camera.Parameters parameters = mCamera.getParameters();
			parameters.setPreviewFormat(ImageFormat.NV21);
			setPreviewSize(parameters);
			setPreviewFpsRange(parameters);
			setPreviewOrientation(parameters);
			mCamera.setParameters(parameters);
			
			mBuffer = new byte[mParam.getWidth() * mParam.getHeight() * 3 / 2];
			mRaw = new byte[mParam.getWidth() * mParam.getHeight() * 3 / 2];
			mCamera.addCallbackBuffer(mBuffer);
			mCamera.setPreviewCallbackWithBuffer(this);
			mCamera.setPreviewDisplay(mHolder);
			mCamera.startPreview();
			mPreviewRunning = true;
		} catch (Exception ex) {
			ex.printStackTrace();
			if (null != mListener) {
				mListener.onErrorPusher(-100);
			}
		}
	}

	private void setPreviewSize(Camera.Parameters parameters) {
		List<Integer> supportedPreviewFormats = parameters
				.getSupportedPreviewFormats();
		for (Integer integer : supportedPreviewFormats) {
			System.out.println("setPreviewSize fmt:" + integer);
		}
		
		List<Size> supportedPreviewSizes = parameters.getSupportedPreviewSizes();
		Size size = supportedPreviewSizes.get(0);
		Log.d(TAG, "setPreviewSize size: " + size.width + "x" + size.height);
		
		int m = Math.abs(size.height * size.width - mParam.getHeight() * mParam.getWidth());
		supportedPreviewSizes.remove(0);
		
		Iterator<Size> iterator = supportedPreviewSizes.iterator();
		while (iterator.hasNext()) {
			Size next = iterator.next();
			Log.d(TAG, "setPreviewSize next: " + next.width + "x" + next.height);
			int n = Math.abs(next.height * next.width - mParam.getHeight() * mParam.getWidth());
			if (n < m) {
				m = n;
				size = next;
			}
		}
		
		mParam.setHeight(size.height);
		mParam.setWidth(size.width);
		parameters.setPreviewSize(mParam.getWidth(), mParam.getHeight());
		Log.d(TAG, "width:" + size.width + " height:" + size.height);
	}

	private void setPreviewFpsRange(Camera.Parameters parameters) {
		int range[] = new int[2];
		parameters.getPreviewFpsRange(range);
		Log.d(TAG, "setPreviewFpsRange, fps:" + range[0] + " - " + range[1]);
	}

	private void setPreviewOrientation(Camera.Parameters parameters) {
        int width = mParam.getWidth();
        int height = mParam.getHeight();
		int rotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
		switch (rotation) {
		case Surface.ROTATION_0:
			mScreen = SCREEN_PORTRAIT;
            width = mParam.getHeight();
            height = mParam.getWidth();
			break;
		case Surface.ROTATION_90: 
			mScreen = SCREEN_LANDSCAPE_LEFT;
			break;
		case Surface.ROTATION_180:
			mScreen = 180;
			break;
		case Surface.ROTATION_270:
			mScreen = SCREEN_LANDSCAPE_RIGHT;
			break;
        default:
            mScreen = 0;
            break;
		}

        Log.d(TAG, "setPreviewOrientation, screen=" + mScreen + ", width=" + width + ", height=" + height);
		String inOpts = String.format("-f yuv4mpegpipe");
		String encOpts = "";
		encOpts += "-pix_fmt yuv420p -vcodec libx264 -vprofile baseline -maxrate 640k -minrate 128k -framerate 20 -g 20";
		encOpts += String.format(" -s %dx%d", width, height);
		mNative.setVideoOptions(inOpts, encOpts);
		
		int result;
		CameraInfo info = new CameraInfo();
		Camera.getCameraInfo(mParam.getCameraId(), info);
		if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
			result = (info.orientation + mScreen) % 360;
			result = (360 - result) % 360; // compensate the mirror
		} else { // back-facing
			result = (info.orientation - mScreen + 360) % 360;
		}
		mCamera.setDisplayOrientation(result);

		// 
		// if (mContext.getResources().getConfiguration().orientation ==
		// Configuration.ORIENTATION_PORTRAIT) {
		// mNative.setVideoOptions(mParam.getHeight(), mParam.getWidth(),
		// mParam.getBitrate(), mParam.getFps());
		// parameters.set("orientation", "portrait");
		// mCamera.setDisplayOrientation(90);
		// } else {
		// mNative.setVideoOptions(mParam.getWidth(), mParam.getHeight(),
		// mParam.getBitrate(), mParam.getFps());
		// parameters.set("orientation", "landscape");
		// mCamera.setDisplayOrientation(0);
		// }
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		mHolder = holder;
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width,
			int height) {
		mHolder = holder;
		stopPreview();
		startPreview();
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		stopPreview();
	}

	@Override
	public void onPreviewFrame(byte[] data, Camera camera) {
		if (mPusherRuning) {
			byte[] pData = data;
			switch (mScreen) {
			    case SCREEN_PORTRAIT:
                    portraitData2Raw(data);
                    pData = mRaw;
                    break;
                case SCREEN_LANDSCAPE_LEFT:
                    pData = data;
                    break;
                case SCREEN_LANDSCAPE_RIGHT:
                    landscapeData2Raw(data);
                    pData = mRaw;
                    break;
			}
			mNative.fireVideo(pData, pData.length);
			pData = null;
		}
		camera.addCallbackBuffer(mBuffer);
	}

	private void landscapeData2Raw(byte[] data) {
		int width = mParam.getWidth(), height = mParam.getHeight();
		int y_len = width * height;
		int k = 0;
		// y => raw
		for (int i = y_len - 1; i > -1; i--) {
			mRaw[k] = data[i];
			k++;
		}
		
		// System.arraycopy(data, y_len, raw, y_len, uv_len);
		// v1 u1 v2 u2
		// v3 u3 v4 u4
		//  vu reverse
		// v4 u4 v3 u3
		// v2 u2 v1 u1
		int maxpos = data.length - 1;
		int uv_len = y_len >> 2; // 4:1:1
		for (int i = 0; i < uv_len; i++) {
			int pos = i << 1;
			mRaw[y_len + i * 2] = data[maxpos - pos - 1];
			mRaw[y_len + i * 2 + 1] = data[maxpos - pos];
		}
	}

	private void portraitData2Raw(byte[] data) {
		// if (mContext.getResources().getConfiguration().orientation !=
		// Configuration.ORIENTATION_PORTRAIT) {
		// raw = data;
		// return;
		// }
		
		int width = mParam.getWidth(); 
		int height = mParam.getHeight();
		int y_len = width * height;
		int uvHeight = height >> 1;
		int k = 0;
		
		if (mParam.getCameraId() == CameraInfo.CAMERA_FACING_BACK) {
			for (int j = 0; j < width; j++) {
				for (int i = height - 1; i >= 0; i--) {
					mRaw[k++] = data[width * i + j];
				}
			}
			
			for (int j = 0; j < width; j += 2) {
				for (int i = uvHeight - 1; i >= 0; i--) {
					mRaw[k++] = data[y_len + width * i + j];
					mRaw[k++] = data[y_len + width * i + j + 1];
				}
			}
		} else {
			for (int i = 0; i < width; i++) {
				int nPos = width - 1;
				for (int j = 0; j < height; j++) {
					mRaw[k] = data[nPos - i];
					k++;
					nPos += width;
				}
			}
			
			for (int i = 0; i < width; i += 2) {
				int nPos = y_len + width - 1;
				for (int j = 0; j < uvHeight; j++) {
					mRaw[k] = data[nPos - i - 1];
					mRaw[k + 1] = data[nPos - i];
					k += 2;
					nPos += width;
				}
			}
		}
	}

}
