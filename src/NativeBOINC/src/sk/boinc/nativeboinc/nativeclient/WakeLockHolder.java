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

package sk.boinc.nativeboinc.nativeclient;

import edu.berkeley.boinc.nativeboinc.ClientEvent;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.PreferenceName;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.PowerManager.WakeLock;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * @author mat
 *
 */
public class WakeLockHolder implements NativeBoincStateListener, MonitorListener,
	OnSharedPreferenceChangeListener, NativeBoincReplyListener {

	private static final int WAKELOCK_CHANNEL_ID = 4;
	
	private static final String TAG = "WakeLockHolder";
	
	private NativeBoincService mNativeBoincService;
	private WakeLock mPartialWakeLock;
	private WakeLock mDimWakeLock;
	
	private boolean mMonitorDoesntWork = false;
	private boolean mFirstClientEvent = false;
	
	private boolean mPowerSaving = false;
	
	private boolean mRegisteredAsMonitorListener = false;
	
	private boolean mAutoRefreshRegistered = false;
	private boolean mAutoReleaseIsRan = false;
	
	private SharedPreferences mGlobalPrefs = null;
	private NativeBoincService.ListenerHandler mListenerHandler = null;
	
	public WakeLockHolder(NativeBoincService service, WakeLock partialWakeLock, WakeLock dimWakeLock) {
		this.mNativeBoincService = service;
		this.mPartialWakeLock = partialWakeLock;
		this.mDimWakeLock = dimWakeLock;
		service.addNativeBoincListener(this);
		mListenerHandler = service.getListenerHandler();
		
		mGlobalPrefs = PreferenceManager.getDefaultSharedPreferences(mNativeBoincService);
		mPowerSaving = mGlobalPrefs.getBoolean(PreferenceName.POWER_SAVING, false);
		
		mGlobalPrefs.registerOnSharedPreferenceChangeListener(this);
		
		if (service.isRun()) {
			if (Logging.DEBUG) Log.i(TAG, "Registering monitor listener");
			service.addMonitorListener(this);
			mRegisteredAsMonitorListener = true;
		}
	}
	
	@Override
	public int getRunnerServiceChannelId() {
		return WAKELOCK_CHANNEL_ID;
	}
	
	public void destroy() {
		// if quit
		if (Logging.DEBUG) Log.i(TAG, "destroy: Unregistering listeners");
		mListenerHandler.removeCallbacks(mAutoRelease);
		mListenerHandler.removeCallbacks(mAutoRefresh);
		mGlobalPrefs.unregisterOnSharedPreferenceChangeListener(this);
		mNativeBoincService.removeMonitorListener(this);
		mNativeBoincService.removeNativeBoincListener(this);
		mGlobalPrefs = null;
		// native boinc service should release wake lock before destroying
	}
	
	private void updatePowerSaving(boolean newPowerSaving) {
		if (Logging.DEBUG) Log.d(TAG, "Change power saving preference:" + newPowerSaving);
		synchronized (this) {
			if (newPowerSaving && !mPowerSaving) { // switch to partial wakelock
				if (mDimWakeLock != null && mDimWakeLock.isHeld()) {
					if (Logging.DEBUG) Log.d(TAG, "Switch to partial wake lock");
					mDimWakeLock.release();
					if (mPartialWakeLock != null)
						mPartialWakeLock.acquire();
				}
			} else if (!newPowerSaving && mPowerSaving) { // switch to dim wakelock
				if (mPartialWakeLock != null && mPartialWakeLock.isHeld()) {
					if (Logging.DEBUG) Log.d(TAG, "Switch to dim wake lock");
					mPartialWakeLock.release();
					if (mDimWakeLock != null)
						mDimWakeLock.acquire();
				}
			}
			mPowerSaving = newPowerSaving;
		}
	}
	
	@Override
	public boolean onNativeBoincClientError(String message) {
		// nothing
		return false;
	}

	private synchronized void acquireWakeLock() {
		if (Logging.INFO) Log.i(TAG, "Wake lock acquiring");
		mListenerHandler.removeCallbacks(mAutoRelease);
		mAutoReleaseIsRan = false;
		if (!mPowerSaving) {
			if (mPartialWakeLock != null && mPartialWakeLock.isHeld())
				mPartialWakeLock.release();
			if (mDimWakeLock != null && !mDimWakeLock.isHeld())
				mDimWakeLock.acquire();
		} else {
			if (mDimWakeLock != null && mDimWakeLock.isHeld())
				mDimWakeLock.release();
			if (mPartialWakeLock != null && !mPartialWakeLock.isHeld())
				mPartialWakeLock.acquire();
		}
	}
	
	private synchronized void releaseWakeLock() {
		if (Logging.INFO) Log.i(TAG, "Wake lock releasing from auto release");
		if (mDimWakeLock != null && mDimWakeLock.isHeld())
			mDimWakeLock.release();
		if (mPartialWakeLock != null && mPartialWakeLock.isHeld())
			mPartialWakeLock.release();
	}
	
	private synchronized boolean checkWakeLock() {
		if (mDimWakeLock != null && mDimWakeLock.isHeld())
			return true;
		if (mPartialWakeLock != null && mPartialWakeLock.isHeld())
			return true;
		return false;
	}
	
	private Runnable mAutoRelease = new Runnable() {
		@Override
		public void run() {
			mAutoReleaseIsRan = false;
			releaseWakeLock();
		}
	};
	
	private static final int AUTO_RELEASE_DELAY = 15000;
	
	@Override
	public void onMonitorEvent(ClientEvent event) {
		boolean processed = true;
		switch (event.type) {
		case ClientEvent.EVENT_RUN_TASKS:
		case ClientEvent.EVENT_RUN_BENCHMARK:
			acquireWakeLock();
			break;
		case ClientEvent.EVENT_SUSPEND_ALL_TASKS:
			mListenerHandler.postDelayed(mAutoRelease, AUTO_RELEASE_DELAY);
			break;
		case ClientEvent.EVENT_FINISH_BENCHMARK:
			mListenerHandler.postDelayed(mAutoRelease, 1200);
			break;
		default: // do nothinh
			processed = false;
		}
		
		mFirstClientEvent = processed; // yes it received 
	}

	private static final int AUTO_REFRESH_DELAY = 10000;
	
	private Runnable mAutoRefresh = new Runnable() {
		
		@Override
		public void run() {
			mNativeBoincService.getGlobalProgress(getRunnerServiceChannelId());
			
			if (mNativeBoincService.isRun()) {
				mListenerHandler.postDelayed(mAutoRefresh, AUTO_REFRESH_DELAY);
			}
		}
	};
	
	@Override
	public void onClientStart() {
		// register listener
		if (!mRegisteredAsMonitorListener) {
			if (Logging.DEBUG) Log.i(TAG, "Registering monitor listener");
			mNativeBoincService.addMonitorListener(this);
			mRegisteredAsMonitorListener = true;
		}
		if (mMonitorDoesntWork) { // if monitor doesnt work
			acquireWakeLock();
			// enable autorefresh
			if (!mAutoRefreshRegistered) {
				mListenerHandler.post(mAutoRefresh);
				mAutoRefreshRegistered = true;
			}
		} else {
			mListenerHandler.postDelayed(new Runnable() {
				@Override
				public void run() {
					if (!mFirstClientEvent) {
						if (Logging.DEBUG) Log.d(TAG, "Do get global progress (fallback)");
						mNativeBoincService.getGlobalProgress(getRunnerServiceChannelId());
					}
				}
			}, 3000); // waiting 3 seconds for first client event
		}
	}
	
	@Override
	public void onClientStop(int exitCode, boolean stoppedByManager) {
		if (mMonitorDoesntWork) { // if monitor doesnt work
			releaseWakeLock();
			mAutoRefreshRegistered = false;
			mListenerHandler.removeCallbacks(mAutoRefresh);
		}
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
			String key) {
		if (key.equals(PreferenceName.POWER_SAVING)) {
			updatePowerSaving(sharedPreferences.getBoolean(PreferenceName.POWER_SAVING, false)); 
		}
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		
	}

	@Override
	public void onMonitorDoesntWork() {
		if (Logging.INFO) Log.i(TAG, "Monitor doesnt work");
		mMonitorDoesntWork = true;
		acquireWakeLock();
		
		acquireWakeLock();
		// enable autorefresh
		if (!mAutoRefreshRegistered) {
			mListenerHandler.post(mAutoRefresh);
			mAutoRefreshRegistered = true;
		}
	}

	@Override
	public boolean onNativeBoincServiceError(WorkerOp workerOp, String message) {
		// disable on Progress change
		mFirstClientEvent = true;
		return false;
	}

	@Override
	public void onProgressChange(double progress) {
		if (mMonitorDoesntWork) {
			if (Logging.DEBUG) Log.d(TAG, "On global progress (no monitor)");
			// first get tasks when first client event not received
			if (progress >= 0.0) {
				acquireWakeLock();
			}else if (checkWakeLock() && !mAutoReleaseIsRan) {// if no task and if held
				mAutoReleaseIsRan = true;
				mListenerHandler.postDelayed(mAutoRelease, AUTO_RELEASE_DELAY);
			}
		} else if (!mFirstClientEvent) {
			if (Logging.DEBUG) Log.d(TAG, "On global progress (fallback)");
			// first get tasks when first client event not received
			if (progress >= 0.0)
				acquireWakeLock();
			else // no tasks to run
				releaseWakeLock();
			// disable  it
			mFirstClientEvent = true;
		}
	}
}
