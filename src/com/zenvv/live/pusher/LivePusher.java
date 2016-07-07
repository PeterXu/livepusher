package com.zenvv.live.pusher;

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
		mNative = new PusherNative();
		
		videoParam = new VideoParam(cameraId, width, height, bitrate, fps, 10);
		audioParam = new AudioParam();
	}

	public void prepare(SurfaceHolder surfaceHolder) {
		surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		
		// create videoPusher
		videoPusher = new VideoPusher(mActivity, surfaceHolder, videoParam, mNative);
		videoPusher.setLiveStateListener(mListener);
		
		// create audioPusher
		audioPusher = new AudioPusher(audioParam, mNative);
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

	public void setLiveStateListener(LiveStateListener listener) {
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
