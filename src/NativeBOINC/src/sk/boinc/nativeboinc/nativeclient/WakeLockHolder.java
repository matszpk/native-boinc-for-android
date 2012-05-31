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
	OnSharedPreferenceChangeListener {

	private static final String TAG = "WakeLockHolder";
	
	private NativeBoincService mNativeBoincService;
	private WakeLock mPartialWakeLock;
	private WakeLock mDimWakeLock;
		
	private boolean mPowerSaving = false;
	
	private boolean mRegisteredAsMonitorListener = false;
	
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
	
	public void destroy() {
		// if quit
		if (Logging.DEBUG) Log.i(TAG, "destroy: Unregistering listeners");
		mListenerHandler.removeCallbacks(mAutoRelease);
		mGlobalPrefs.unregisterOnSharedPreferenceChangeListener(this);
		mNativeBoincService.removeMonitorListener(this);
		mNativeBoincService.removeNativeBoincListener(this);
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

	private Runnable mAutoRelease = new Runnable() {
		@Override
		public void run() {
			if (Logging.INFO) Log.i(TAG, "Wake lock releasing from auto release");
			synchronized(WakeLockHolder.this) {
				if (mDimWakeLock != null && mDimWakeLock.isHeld())
					mDimWakeLock.release();
				if (mPartialWakeLock != null && mPartialWakeLock.isHeld())
					mPartialWakeLock.release();
			}
		}
	};
	
	private static final int AUTO_RELEASE_DELAY = 15000;
	
	@Override
	public void onMonitorEvent(ClientEvent event) {
		switch (event.type) {
		case ClientEvent.EVENT_RUN_TASKS:
		case ClientEvent.EVENT_RUN_BENCHMARK:
			if (Logging.INFO) Log.i(TAG, "Wake lock acquiring");
			synchronized(this) {
				mListenerHandler.removeCallbacks(mAutoRelease);
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
			break;
		case ClientEvent.EVENT_SUSPEND_ALL_TASKS:
			mListenerHandler.postDelayed(mAutoRelease, AUTO_RELEASE_DELAY);
			break;
		case ClientEvent.EVENT_FINISH_BENCHMARK:
			mListenerHandler.postDelayed(mAutoRelease, 1200);
			break;
		default: // do nothinh		
		}
	}
	
	

	@Override
	public void onClientStart() {
		// register listener
		if (!mRegisteredAsMonitorListener) {
			if (Logging.DEBUG) Log.i(TAG, "Registering monitor listener");
			mNativeBoincService.addMonitorListener(this);
			mRegisteredAsMonitorListener = true;
		}
	}
	
	@Override
	public void onClientStop(int exitCode, boolean stoppedByManager) {
		// do nothing
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
		// TODO Auto-generated method stub
		
	}
}
