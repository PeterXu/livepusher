package com.zenvv.live;

import android.app.Activity;
import android.view.SurfaceHolder;

import com.zenvv.live.jni.PusherNative;

public class LivePusher {
	private final static String TAG = "LivePusher";
	
	private VideoParam videoParam;
	private AudioParam audioParam;
	
	private PusherNative mNative;
	private VideoPusher videoPusher;
	private AudioPusher audioPusher;
	private LiveStateListener mListener;
	
	private Activity mActivity;

	static {
		System.loadLibrary("Pusher");
	}

	public LivePusher(Activity activity, int width, int height, int bitrate,
			int fps, int cameraId) {
		mActivity = activity;
		videoParam = new VideoParam(width, height, bitrate, fps, cameraId);
		audioParam = new AudioParam();
		mNative = new PusherNative();
	}

	public void prepare(SurfaceHolder surfaceHolder) {
		surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		
		videoPusher = new VideoPusher(mActivity, surfaceHolder, videoParam, mNative);
		audioPusher = new AudioPusher(audioParam, mNative);
		
		videoPusher.setLiveStateListener(mListener);
		audioPusher.setLiveStateListener(mListener);
	}

	public void startPusher(String url) {
		videoPusher.startPusher();
		audioPusher.startPusher();
		mNative.startPusher(url);
	}

	public void stopPusher() {
		videoPusher.stopPusher();
		audioPusher.stopPusher();
		mNative.stopPusher();
	}

	public void switchCamera() {
		videoPusher.switchCamera();
	}

	public void relase() {
		mActivity = null;
		stopPusher();
		
		videoPusher.setLiveStateListener(null);
		audioPusher.setLiveStateListener(null);
		mNative.setLiveStateListener(null);
		
		videoPusher.release();
		audioPusher.release();
		mNative.release();
	}

	public void setLiveStateChangeListener(LiveStateListener listener) {
		mListener = listener;
		mNative.setLiveStateListener(listener);
		
		if (null != videoPusher) {
			videoPusher.setLiveStateListener(listener);
		}
		
		if (null != audioPusher) {
			audioPusher.setLiveStateListener(listener);
		}
	}
}
