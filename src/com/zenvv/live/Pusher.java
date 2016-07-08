package com.zenvv.live;

import com.zenvv.live.jni.PusherNative;

public abstract class Pusher {

	protected boolean mPusherRuning;
	protected PusherNative mNative;
	protected LiveStateListener mListener;

	public Pusher(PusherNative pusherNative) {
		mNative = pusherNative;
	}

	public void setLiveStateChangeListener(LiveStateListener listener) {
		mListener = listener;
	}

	public abstract void startPusher();

	public abstract void stopPusher();

	public abstract void release();
}
