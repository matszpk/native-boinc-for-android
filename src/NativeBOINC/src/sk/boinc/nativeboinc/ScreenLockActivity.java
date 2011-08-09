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

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
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
public class ScreenLockActivity extends Activity {
	private static final String TAG = "ScreenLockActivity";
	
	private static final int UPDATE_PERIOD = 60000;
	
	private RelativeLayout mLockProgress;
	private ProgressBar mProgressRunning;
	private TextView mProgressText;
	
	private TextView mLockText;
	
	private boolean mIsRefreshingOn = true;
	
	private Runnable mRefresher = new Runnable() {
		
		@Override
		public void run() {
			if (!mIsRefreshingOn)
				return;
			
			if (Logging.DEBUG) Log.d(TAG, "Run refreshing action");
			
			double progress = -1.0;
			if (mRunner != null)
				progress = mRunner.getGlobalProgress();
			
			if (progress >= 0.0) {
				mLockText.setText(getString(R.string.lockWeAreComputing));
				mLockProgress.setVisibility(View.VISIBLE);
				mProgressText.setText(String.format("%.3f%%", progress));
				mProgressRunning.setProgress((int)(progress*10.0));
			} else {
				mLockText.setText(getString(R.string.lockStopped));
				mLockProgress.setVisibility(View.GONE);
			}
			
			// run again
			mLockProgress.postDelayed(mRefresher, UPDATE_PERIOD);
		}
	};
	
	private NativeBoincService mRunner = null;
	
	private ServiceConnection mRunnerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceConnected()");
		}
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			mRunner = null;
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceDisconnected()");
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
		
		doBindRunnerService();
		
		mLockText = (TextView)findViewById(R.id.lockText);
		mLockProgress = (RelativeLayout)findViewById(R.id.lockProgress);
		mProgressRunning = (ProgressBar)findViewById(R.id.lockProgressRunning);
		mProgressText = (TextView)findViewById(R.id.lockProgressText);
		
		// run refresher
		mLockProgress.post(mRefresher); 
	}
	
	@Override
	protected void onPause() {
		mIsRefreshingOn = false;
		super.onPause();
		finish();	// do finish
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
		
		if (mRunner != null)
			mRunner = null;
		
		doUnbindRunnerService();
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent keyEvent) {
		// do nothing
		return false;
	}
}
