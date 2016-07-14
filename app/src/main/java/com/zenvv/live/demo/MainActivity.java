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
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.melnykov.fab.FloatingActionButton;
import com.zenvv.live.LivePusher;
import com.zenvv.live.LiveStateListener;
import com.zenvv.livepusher.R;

public class MainActivity extends Activity implements
		Callback, LiveStateListener {
	private final static String TAG = "MainActivity";

	private final static String URI = "rtmp://v.sportsdata.cn/app/";
	//private final static String URI = "http://v.sportsdata.cn/app/";

	private Button mStartBtn;
	private SurfaceHolder mHolder;
	private boolean isStart;
	private LivePusher livePusher;

	private String mUrl;
	private String mTitle;

	private boolean mShowCtrl = false;
	private RelativeLayout mFloatCtrl;
	
	private final Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			int msgId = 0;
			switch (msg.what) {
				case -100:	msgId = R.string.e_video_preview; break;
				case -101:	msgId = R.string.e_audio_record; break;
				case -102:	msgId = R.string.e_audio_encoder; break;
				case -103:	msgId = R.string.e_video_encoder; break;
				case -104:
				case -105:
				case -106:
				case -107:
				case -108: msgId = R.string.e_streaming_failure; break;
				default:
					break;
			}

			if (msgId > 0) {
                String error = getString(msgId) + ", code=" + msg.what;
				Toast.makeText(MainActivity.this, error, Toast.LENGTH_SHORT).show();
				checkStatus(true);
			}
		};
	};

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.activity_main);
		
		mStartBtn = (Button) findViewById(R.id.button_start);

		SurfaceView surface = (SurfaceView) this.findViewById(R.id.surface);
		mHolder = surface.getHolder();
		mHolder.addCallback(this);

		EditText text1 = (EditText)findViewById(R.id.live_title);
		text1.setText(R.string.live_testing);

		mFloatCtrl = (RelativeLayout) findViewById(R.id.float_ctrl);
		if (!mShowCtrl) {
			mFloatCtrl.setVisibility(View.INVISIBLE);
		}
		
		livePusher = new LivePusher(this, 640, 480, 768*1024, 20, CameraInfo.CAMERA_FACING_FRONT);
		livePusher.setLiveStateListener(this);
		livePusher.prepare(mHolder);
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

	public void checkStatus(boolean forceStopped) {
		if (forceStopped || isStart) {
			mStartBtn.setText(R.string.start);
			isStart = false;
			livePusher.stopPusher();
		}else {
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
		String channel = text2.getText().toString();
		if (channel == null || channel.length() < 4) {
			Toast.makeText(MainActivity.this, R.string.invalid_channel, Toast.LENGTH_SHORT).show();
			return;
		}

		mUrl = URI + channel;
		tv.setText(mUrl);
	}

	public void onClickStart(View v) {
		checkUserInput();

		if (mUrl != null && !mUrl.isEmpty()) {
            checkStatus(false);
        }
	}

	public void onClickSwitch(View view) {
		livePusher.switchCamera();
	}

	public void onClickFloatingActionButton(View view) {
		if (mShowCtrl) {
			mFloatCtrl.setVisibility(View.INVISIBLE);
			mShowCtrl = false;
		}else {
			mFloatCtrl.setVisibility(View.VISIBLE);
			mShowCtrl = true;
		}
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

	@Override
	public void onStartPusher() {
		Log.d(TAG, "start push streaming");
	}

	@Override
	public void onStopPusher() {
		Log.d(TAG, "stop push streaming");
	}

}
