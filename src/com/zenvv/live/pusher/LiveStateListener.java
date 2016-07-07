package com.zenvv.live.pusher;

public interface LiveStateListener {

	// for video
	public void onErrorPusher(int code);

	// start push
	public void onStartPusher(); 

	// stop push
	public void onStopPusher();
}
