package com.zenvv.live.demo;

import android.app.Activity;
import android.hardware.Camera.CameraInfo;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.zenvv.live.LivePusher;
import com.zenvv.live.LiveStateListener;
import com.zenvv.livepusher.R;

public class MainActivity extends Activity implements OnClickListener,
		Callback, LiveStateListener {
	private final static String TAG = "MainActivity";
	
	private Button mStartBtn;
	private SurfaceView mSurfaceView;
	private SurfaceHolder mSurfaceHolder;
	private boolean isStart;
	private LivePusher livePusher;

	private String mUrl;
	private String mTitle;
	
	private final Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {
			case -100:
				Toast.makeText(MainActivity.this, R.string.video_preview_failure, Toast.LENGTH_SHORT).show();
				livePusher.stopPusher();
				break;
			case -101:
				Toast.makeText(MainActivity.this, R.string.audio_record_failure, Toast.LENGTH_SHORT).show();
				livePusher.stopPusher();
				break;
			case -102:
				Toast.makeText(MainActivity.this, R.string.audio_config_failure, Toast.LENGTH_SHORT).show();
				livePusher.stopPusher();
				break;
			case -103:
				Toast.makeText(MainActivity.this, R.string.video_config_failure, Toast.LENGTH_SHORT).show();
				livePusher.stopPusher();
				break;
			case -104:
				Toast.makeText(MainActivity.this, R.string.net_streaming_failure, Toast.LENGTH_SHORT).show();
				livePusher.stopPusher();
				break;
			}
			mStartBtn.setText(R.string.stop);
			isStart = false;
		};
	};

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.activity_main);
		
		mStartBtn = (Button) findViewById(R.id.button_start);
		mStartBtn.setOnClickListener(this);
		findViewById(R.id.button_switch).setOnClickListener(
				new OnClickListener() {
					@Override
					public void onClick(View v) {
						livePusher.switchCamera();
					}
				});
		
		mSurfaceView = (SurfaceView) this.findViewById(R.id.surface);
		mSurfaceHolder = mSurfaceView.getHolder();
		mSurfaceHolder.addCallback(this);

		EditText text1 = (EditText)findViewById(R.id.live_title);
		text1.setText(R.string.live_testing);
		
		livePusher = new LivePusher(this, 640, 480, 768*1024, 20,
				CameraInfo.CAMERA_FACING_FRONT);
		livePusher.setLiveStateListener(this);
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
		checkUserInput();

		if (mUrl == null || mUrl.isEmpty()) {
			return;
		}

		if (isStart) {
			mStartBtn.setText(R.string.start);
			isStart = false;
			livePusher.stopPusher();
		} else {
			mStartBtn.setText(R.string.stop);
			isStart = true;
			livePusher.startPusher(mUrl);

		}
	}

	public void checkUserInput() {
		TextView tv = (TextView)findViewById(R.id.live_url);
		tv.setText("");

		EditText text1 = (EditText)findViewById(R.id.live_title);
		mTitle = text1.getText().toString();
		if (mTitle == null || mTitle.isEmpty()) {
			Toast.makeText(MainActivity.this, R.string.invalid_title, Toast.LENGTH_SHORT).show();
			return;
		}

		mUrl = "";
		EditText text2 = (EditText)findViewById(R.id.live_channel);
		if (text2.getText().length() < 4) {
			Toast.makeText(MainActivity.this, R.string.invalid_channel, Toast.LENGTH_SHORT).show();
			return;
		}

		mUrl = "rtmp://media.sportsdata.cn/app/" + text2.getText().toString();
		tv.setText(mUrl);
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		Log.d(TAG, "surfaceCreated");
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		Log.d(TAG, "surfaceChanged, width=" + width + ", height=" + height);
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		Log.d(TAG, "surfaceDestroyed");
	}

	/**
	 * may run in child-thread
	 */
	@Override
	public void onErrorPusher(int code) {
		Log.d(TAG, "onErrorPusher code:" + code);
		mHandler.sendEmptyMessage(code);
	}

	/**
	 * may run in child-thread
	 */
	@Override
	public void onStartPusher() {
		Log.d(TAG, "start push streaming");
	}

	/**
	 * may run in child-thread
	 */
	@Override
	public void onStopPusher() {
		Log.d(TAG, "stop push streaming");
	}

}
