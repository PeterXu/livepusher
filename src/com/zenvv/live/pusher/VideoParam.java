package com.zenvv.live.pusher;

public class VideoParam {
	private int cameraId;
	private int width = 640;
	private int height = 360;
	private int fps = 15;
	private int bitrate = 512*1024; // 512 kb/s
	private int gopNum = 15; 
	
	private int frameRate = 90000;  // HZ
	private String codec = "libx264";  

	public VideoParam(int cameraId, int width, int height, int fps, int bitrate, int gopNum) {
		this.cameraId = cameraId;
		this.width = width;
		this.height = height;
		this.fps = fps;
		this.bitrate = bitrate;
		this.gopNum = gopNum;
	}
	
	public int getCameraId() {
		return cameraId;
	}

	public void setCameraId(int cameraId) {
		this.cameraId = cameraId;
	}

	public int getWidth() {
		return width;
	}

	public void setWidth(int width) {
		this.width = width;
	}

	public int getHeight() {
		return height;
	}

	public void setHeight(int height) {
		this.height = height;
	}

	public int getFps() {
		return fps;
	}

	public void setFps(int fps) {
		this.fps = fps;
	}
	
	public int getBitrate() {
		return bitrate;
	}

	public void setBitrate(int bitrate) {
		this.bitrate = bitrate;
	}

	public int getGopNum() {
		return gopNum;
	}

	public void setGopNum(int gopNum) {
		this.gopNum = gopNum;
	}
	
	public int getframeRate() {
		return frameRate;
	}

	public void setFrameRate(int frameRate) {
		this.frameRate = frameRate;
	}
	
	public String getCodec() {
		return codec;
	}

	public void setCodec(String codec) {
		this.codec = codec;
	}
}
