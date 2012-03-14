/* 
 * NativeBOINC - Native BOINC Client with Manager
 * Copyright (C) 2011, Mateusz Szpakowski
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

package sk.boinc.nativeboinc;

import java.text.SimpleDateFormat;
import java.util.Date;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.ExitCode;
import sk.boinc.nativeboinc.nativeclient.NativeBoincReplyListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincStateListener;
import sk.boinc.nativeboinc.util.PreferenceName;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class ScreenLockActivity extends Activity implements NativeBoincReplyListener,
		NativeBoincStateListener {
	private static final String TAG = "ScreenLockActivity";
	
	private int mUpdatePeriod;
	
	private RelativeLayout mLockProgress;
	private ProgressBar mProgressRunning;
	private TextView mProgressText;
	
	private TextView mLockText;
	private TextView mBarText;
	
	private SimpleDateFormat mDateFormat;
	
	private int mBatteryLevel = -1;
	
	private boolean mIsRefreshingOn = true;
	private boolean mIfProgressUpdated = false;
	private boolean mIsFirstUpdate = true;
	
	private String mErrorMessage = null;
	
	private BroadcastReceiver mBatteryStateReceiver = new BroadcastReceiver() {
		
		@Override
		public void onReceive(Context context, Intent intent) {
			if (intent.getAction().equals(Intent.ACTION_BATTERY_CHANGED)) {
				mBatteryLevel = intent.getIntExtra("level", 0);
			}
		}
	};
	
	private Runnable mRefresher = new Runnable() {
		@Override
		public void run() {
			if (!mIsRefreshingOn)
				return;
			
			if (Logging.DEBUG) Log.d(TAG, "Run refreshing action");
			
			if (mRunner != null) {
				mIfProgressUpdated = false;
				if (mIsFirstUpdate) {
					mLockProgress.post(new Runnable() {
						@Override
						public void run() {
							/* if not updated through getProgress */
							if (!mIfProgressUpdated) {
								mLockText.setText(getString(R.string.lockNotRan));
								mLockProgress.setVisibility(View.GONE);
							}
						}
					});
					mIsFirstUpdate = false;
				}
				mRunner.getGlobalProgress(ScreenLockActivity.this);
			}
			
			// bar text
			if (mBatteryLevel != -1)
				mBarText.setText(mBatteryLevel + mDateFormat.format(new Date()));
			else
				mBarText.setText("---" + mDateFormat.format(new Date()));
			
			// run again
			mLockProgress.postDelayed(mRefresher, mUpdatePeriod);
		}
	};
	
	private NativeBoincService mRunner = null;
	
	private ServiceConnection mRunnerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceConnected()");
			mRunner.addNativeBoincListener(ScreenLockActivity.this);
		}
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			mRunner = null;
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceDisconnected()");
			mRunner.removeNativeBoincListener(ScreenLockActivity.this);
		}
	};
	
	private void doBindRunnerService() {
		bindService(new Intent(ScreenLockActivity.this, NativeBoincService.class),
				mRunnerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindRunnerService() {
		unbindService(mRunnerServiceConnection);
		mRunner = null;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.screen_lock);
		
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		mUpdatePeriod = Integer.parseInt(globalPrefs.getString(
				PreferenceName.SCREEN_LOCK_UPDATE, "10"))*1000;
		
		doBindRunnerService();
		
		IntentFilter mBatteryFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
		registerReceiver(mBatteryStateReceiver, mBatteryFilter);
		
		mDateFormat = new SimpleDateFormat("%, HH:mm");
		
		mLockText = (TextView)findViewById(R.id.lockText);
		mLockProgress = (RelativeLayout)findViewById(R.id.lockProgress);
		mProgressRunning = (ProgressBar)findViewById(R.id.lockProgressRunning);
		mProgressText = (TextView)findViewById(R.id.lockProgressText);
		mBarText = (TextView)findViewById(R.id.screenLockBarText);
		
		// run refresher
		mLockProgress.postDelayed(mRefresher, 100); 
	}
	
	@Override
	protected void onPause() {
		mIsRefreshingOn = false;
		super.onPause();
		finish();
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		if (mRunner == null)
			doBindRunnerService();
	}
	
	@Override
	protected void onDestroy() {
		mIsRefreshingOn = false;
		super.onDestroy();
		
		unregisterReceiver(mBatteryStateReceiver);
		
		if (mRunner != null)
			mRunner = null;
		
		doUnbindRunnerService();
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent keyEvent) {
		// do nothing
		return false;
	}

	@Override
	public void onNativeBoincClientError(String message) {
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void onProgressChange(final double progress) {
		mLockProgress.post(new Runnable() {
			@Override
			public void run() {
				if (progress >= 0.0) {
					mLockText.setText(getString(R.string.lockWeAreComputing));
					mLockProgress.setVisibility(View.VISIBLE);
					mProgressText.setText(String.format("%.3f%%", progress));
					mProgressRunning.setProgress((int)(progress*10.0));
				} else {
					if (mRunner.isRun())
						mLockText.setText(getString(R.string.lockTasksSuspended));
					else {
						if (mErrorMessage == null)
							mLockText.setText(getString(R.string.lockNotRan));
						else
							mLockText.setText(mErrorMessage);
					}
					
					mLockProgress.setVisibility(View.GONE);
				}
				mIfProgressUpdated = true;
			}
		});
	}

	@Override
	public void onNativeBoincServiceError(String message) {
		// trigger progress change
		mErrorMessage = message;
		onProgressChange(-1.0);
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
	}

	@Override
	public void onClientStart() {
		mErrorMessage = null;
		// trigger progress change
		onProgressChange(0.0);
	}

	@Override
	public void onClientStop(int exitCode, boolean stoppedByManager) {
		mErrorMessage = ExitCode.getExitCodeMessage(this, exitCode, stoppedByManager);
		// trigger progress change
		onProgressChange(-1.0);
	}
}
