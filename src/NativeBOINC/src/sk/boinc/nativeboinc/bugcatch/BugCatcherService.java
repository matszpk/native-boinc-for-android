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
package sk.boinc.nativeboinc.bugcatch;

import java.util.List;

import sk.boinc.nativeboinc.debug.Logging;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;

/**
 * @author mat
 *
 */
public class BugCatcherService extends Service {

	private static final String TAG = "BugCatcherService"; 
	
	public class LocalBinder extends Binder {
		public BugCatcherService getService() {
			return BugCatcherService.this;
		}
	}
	
	private final IBinder mBinder = new LocalBinder();
	
	private int mBindCounter = 0;
	private boolean mUnlockStopWhenNotWorking = false;
	private Object mStoppingLocker = new Object();
	
	private Runnable mBugCatchStopper = null;
	private Handler mBugCatchHandler;
	
	private static final int DELAYED_DESTROY_INTERVAL = 4000;
	
	@Override
	public IBinder onBind(Intent intent) {
		mBindCounter++;
		if (Logging.DEBUG) Log.d(TAG, "onBind(), binCounter="+mBindCounter);
		// Just make sure the service is running:
		startService(new Intent(this, BugCatcherService.class));
		return mBinder;
	}
	
	@Override
	public void onRebind(Intent intent) {
		synchronized(mStoppingLocker) {
			mBindCounter++;
			mUnlockStopWhenNotWorking = false;
			if (Logging.DEBUG) Log.d(TAG, "Rebind. bind counter: " + mBindCounter);
		}
		if (mBugCatchHandler != null && mBugCatchStopper != null) {
			if (Logging.DEBUG) Log.d(TAG, "Rebind");
			mBugCatchHandler.removeCallbacks(mBugCatchStopper);
			mBugCatchStopper = null;
		}
	}
	
	@Override
	public boolean onUnbind(Intent intent) {
		int bindCounter; 
		synchronized(mStoppingLocker) {
			mBindCounter--;
			bindCounter = mBindCounter;
		}
		if (Logging.DEBUG) Log.d(TAG, "Unbind. bind counter: " + bindCounter);
		if (!isWorking() && bindCounter == 0) {
			mBugCatchStopper = new Runnable() {
				@Override
				public void run() {
					synchronized(mStoppingLocker) {
						if (isWorking() || mBindCounter != 0) {
							mUnlockStopWhenNotWorking = (mBindCounter == 0);
							mBugCatchStopper = null;
							return;
						}
					}
					if (Logging.DEBUG) Log.d(TAG, "Stop InstallerService");
					mBugCatchStopper = null;
					stopSelf();
				}
			};
			mBugCatchHandler.postDelayed(mBugCatchStopper, DELAYED_DESTROY_INTERVAL);
		} else {
			synchronized(mStoppingLocker) {
				mUnlockStopWhenNotWorking = (mBindCounter == 0);
			}
		}
		return true;
	}
	
	public synchronized boolean doStopWhenNotWorking() {
		return (mBindCounter == 0 && mUnlockStopWhenNotWorking);
	}
	
	@Override
	public void onDestroy() {
		if (Logging.DEBUG) Log.d(TAG, "onDestroy");
		//stopInstaller();
		//mNotificationController = null;
		//mApp = null;
	}
	
	
	public boolean isWorking() {
		return false;
	}

	public static void clearBugReports() {
		
	}
	
	public static void saveToSDCard() {
		
	}
	
	public static List<BugReportInfo> loadBugReports() {
		return null;
	}
	
	public void sendBugsToAuthor() {
		
	}
}
