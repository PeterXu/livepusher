package com.zenvv.live.jni;

import android.util.Log;

import com.zenvv.live.LiveStateListener;

public class PusherNative {
	private final static String TAG = "PusherNative";

	private LiveStateListener mListener;

	public PusherNative() {
	}

	public void setLiveStateListener(LiveStateListener listener) {
		mListener = listener;
	}

	public void onPostNativeError(int code) {
		Log.d(TAG, "onPostNativeError, code=" + code);
		if (null != mListener) {
			mListener.onErrorPusher(code);
		}
	}

	public void onPostNativeState(int state) {
		Log.d(TAG, "onPostNativeState, state=" + state);
		if (null != mListener) {
			if (state == 100) {
				mListener.onStartPusher();
			} else if (state == 101) {
				mListener.onStopPusher();
			}
		}
	}

	public native void setVideoOptions(String inOpts, String encOpts);
	public native void setAudioOptions(String inOpts, String encOpts);
    public native void startPusher(String url);
    public native void stopPusher();
    public native void fireVideo(byte[] buffer, int len);
    public native void fireAudio(byte[] buffer, int len);
	public native void release();
}
