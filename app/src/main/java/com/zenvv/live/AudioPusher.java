package com.zenvv.live;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

import com.zenvv.live.jni.PusherNative;

public class AudioPusher extends Pusher {
	private final static String TAG = "AudioPusher";
	
	private AudioParam mParam;
	private int minBufferSize;
	private AudioRecord audioRecord;

	public AudioPusher(AudioParam param, PusherNative pusherNative) {
		super(pusherNative);
		
		mParam = param;
		int channel = mParam.getChannel() == 1 ? AudioFormat.CHANNEL_IN_MONO
				: AudioFormat.CHANNEL_IN_STEREO;
		int pcmBits = AudioFormat.ENCODING_PCM_16BIT;
		
		minBufferSize = AudioRecord.getMinBufferSize(mParam.getSampleRate(), channel, pcmBits);
		audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC,
				mParam.getSampleRate(), channel, pcmBits, minBufferSize);
		mNative.setAudioOptions(mParam.getSampleRate(), mParam.getChannel(), mParam.getBitrate());
		Log.d(TAG, "audio input:" + mNative.getInputSamples());
	}

	@Override
	public void startPusher() {
		if (null == audioRecord) {
			return;
		}
		
		mPusherRuning = true;
		if (audioRecord.getRecordingState() == AudioRecord.RECORDSTATE_STOPPED) {
			try {
				audioRecord.startRecording();
				new Thread(new AudioRecordTask()).start();
			} catch (Throwable th) {
				th.printStackTrace();
				if (null != mListener) {
					mListener.onErrorPusher(-101);
				}
			}
		}
	}

	@Override
	public void stopPusher() {
		if (null == audioRecord) {
			return;
		}
		
		mPusherRuning = false;
		if (audioRecord.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING) {
			audioRecord.stop();
		}
	}

	@Override
	public void release() {
		if (null == audioRecord) {
			return;
		}
		
		mPusherRuning = false;
		if (audioRecord.getRecordingState() == AudioRecord.RECORDSTATE_STOPPED) {
			audioRecord.release();
		}
	}

	class AudioRecordTask implements Runnable {
		@Override
		public void run() {
			byte[] buffer = new byte[2048];
			while (mPusherRuning
					&& audioRecord.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING) {
				int len = audioRecord.read(buffer, 0, buffer.length);
				if (0 < len) {
					mNative.fireAudio(buffer, len);
				}
			}
			buffer = null;
			audioRecord = null;
		}
	}
}
