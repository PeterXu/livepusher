package com.zenvv.live.pusher;

import com.zenvv.live.jni.PusherNative;

public abstract class MediaPusher {

	protected boolean mPusherRuning;
	protected PusherNative mNative;
	protected LiveStateListener mListener;

	public MediaPusher(PusherNative pusherNative) {
		mNative = pusherNative;
	}

	public void setLiveStateListener(LiveStateListener listener) {
		mListener = listener;
	}

	public abstract void startPusher();

	public abstract void stopPusher();

	public abstract void release();
}
