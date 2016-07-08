package com.zenvv.live;

public class AudioParam {
	private int sampleRate = 44100;		//> HZ
	private int channel = 1;
	private int bitrate = 64 * 1024; 	//> bit/s

	public AudioParam(int sampleRate, int channel, int bitrate) {
		this.sampleRate = sampleRate;
		this.channel = channel;
		this.bitrate = bitrate;
	}

	public AudioParam() {
	}

	public int getSampleRate() {
		return sampleRate;
	}

	public void setSampleRate(int sampleRate) {
		this.sampleRate = sampleRate;
	}

	public int getChannel() {
		return channel;
	}

	public void setChannel(int channel) {
		this.channel = channel;
	}
	
	public int getBitrate() {
		return bitrate;
	}

	public void setBitrate(int bitrate) {
		this.bitrate = bitrate;
	}
}
