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
package sk.boinc.nativeboinc.installer;

import sk.boinc.nativeboinc.debug.Logging;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.WifiLock;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;

/**
 * @author mat
 *
 */
public class ResourcesLocker {
	private static final String TAG = "ResourcesLocker";
	
	private static final String PARTIALLOCK_NAME = "InstallerPartialLock";
	private static final String WIFILOCK_NAME = "InstallerWifiLock";
	
	private WifiLock mWifiLock = null;
	private WakeLock mPartialLock = null;
	
	private int mNestedWakeCounter = 0;
	private int mNestedWifiCounter = 0;
	
	public ResourcesLocker(Context context) {
		/* partial lock */
		PowerManager powerManager = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
		mPartialLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, PARTIALLOCK_NAME);
		/* wifi resources */
		WifiManager wifiManager = (WifiManager)context.getSystemService(Context.WIFI_SERVICE);
		mWifiLock = wifiManager.createWifiLock(WIFILOCK_NAME);
	}
	
	public void destroy() {
		/* release locks */
		if (mPartialLock.isHeld())
			mPartialLock.release();
		if (mWifiLock.isHeld())
			mWifiLock.release();
		
		mPartialLock = null;
		mWifiLock = null;
	}
	
	public synchronized void acquireAllLocks() {
		mNestedWakeCounter++;
		if (Logging.DEBUG) Log.d(TAG, "Acquire partial lock:"+mNestedWakeCounter);
		
		if (mNestedWakeCounter == 1)
			mPartialLock.acquire();
		
		mNestedWifiCounter++;
		if (Logging.DEBUG) Log.d(TAG, "Acquire WiFi lock:"+mNestedWifiCounter);
		
		if (mNestedWifiCounter == 1)
			mWifiLock.acquire();
	}
	
	public synchronized void acquirePartialLock() {
		mNestedWakeCounter++;
		if (Logging.DEBUG) Log.d(TAG, "Acquire partial lock:"+mNestedWakeCounter);
		
		if (mNestedWakeCounter == 1)
			mPartialLock.acquire();
	}
	
	public synchronized void releaseAllLocks() {
		mNestedWakeCounter--;
		if (Logging.DEBUG) Log.d(TAG, "Release partial lock:"+mNestedWakeCounter);
		
		if (mNestedWakeCounter == 0)
			mPartialLock.release();
		
		mNestedWifiCounter--;
		if (Logging.DEBUG) Log.d(TAG, "Release WiFi lock:"+mNestedWifiCounter);
		
		if (mNestedWifiCounter == 0)
			mWifiLock.release();
	}
	
	public synchronized void releasePartialLocks() {
		mNestedWakeCounter--;
		if (Logging.DEBUG) Log.d(TAG, "Release partial lock:"+mNestedWakeCounter);
		
		if (mNestedWakeCounter == 0)
			mPartialLock.release();
	}
}
