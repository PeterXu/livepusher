package com.zenvv.live;

public interface LiveStateListener {

	public void onErrorPusher(int code);

	public void onStartPusher();

	public void onStopPusher();
}
