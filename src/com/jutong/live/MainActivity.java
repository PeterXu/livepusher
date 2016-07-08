package com.jutong.live;

import android.annotation.TargetApi;
import android.app.Activity;
import android.hardware.Camera.CameraInfo;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

import com.zenvv.livepusher.R;

public class MainActivity extends Activity implements OnClickListener,
		Callback, LiveStateChangeListener {

	private Button button01;
	private SurfaceView mSurfaceView;
	private SurfaceHolder mSurfaceHolder;
	private boolean isStart;
	private LivePusher livePusher;
	private Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {

			case -100:
				Toast.makeText(MainActivity.this, R.string.video_preview_failure, 0).show();
				livePusher.stopPusher();
				break;
			case -101:
				Toast.makeText(MainActivity.this, R.string.audio_record_failure, 0).show();
				livePusher.stopPusher();
				break;
			case -102:
				Toast.makeText(MainActivity.this, R.string.audio_config_failure, 0).show();
				livePusher.stopPusher();
				break;
			case -103:
				Toast.makeText(MainActivity.this, R.string.video_config_failure, 0).show();
				livePusher.stopPusher();
				break;
			case -104:
				Toast.makeText(MainActivity.this, R.string.net_streaming_failure, 0).show();
				livePusher.stopPusher();
				break;
			}
			button01.setText(R.string.push_stream);
			isStart = false;
		};
	};

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		button01 = (Button) findViewById(R.id.button_first);
		button01.setOnClickListener(this);
		findViewById(R.id.button_take).setOnClickListener(
				new OnClickListener() {

					@Override
					public void onClick(View v) {
						livePusher.switchCamera();
					}
				});
		mSurfaceView = (SurfaceView) this.findViewById(R.id.surface);
		mSurfaceHolder = mSurfaceView.getHolder();
		mSurfaceHolder.addCallback(this);
		livePusher = new LivePusher(this, 960, 720, 1024000, 15,
				CameraInfo.CAMERA_FACING_FRONT);
		livePusher.setLiveStateChangeListener(this);
		livePusher.prepare(mSurfaceHolder);

	}

	// @Override
	// public void onRequestPermissionsResult(int requestCode,
	// String[] permissions, int[] grantResults) {
	// super.onRequestPermissionsResult(requestCode, permissions, grantResults);
	// }

	@Override
	protected void onDestroy() {
		super.onDestroy();
		livePusher.relase();
	}

	@Override
	public void onClick(View v) {
		if (isStart) {
			button01.setText(R.string.start);
			isStart = false;
			livePusher.stopPusher();
		} else {
			button01.setText(R.string.stop);
			isStart = true;
			livePusher.startPusher("rtmp://10.11.40.4/app/live");

		}
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		System.out.println("MAIN: CREATE");
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width,
			int height) {
		System.out.println("MAIN: CHANGE");
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		System.out.println("MAIN: DESTORY");
	}

	/**
	 * may run in child-thread
	 */
	@Override
	public void onErrorPusher(int code) {
		System.out.println("code:" + code);
		mHandler.sendEmptyMessage(code);
	}

	/**
	 * may run in child-thread
	 */
	@Override
	public void onStartPusher() {
		Log.d("MainActivity", "start push streaming");
	}

	/**
	 * may run in child-thread
	 */
	@Override
	public void onStopPusher() {
		Log.d("MainActivity", "stop push streaming");
	}

}
