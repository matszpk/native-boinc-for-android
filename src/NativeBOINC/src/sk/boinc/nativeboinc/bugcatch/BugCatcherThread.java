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

import sk.boinc.nativeboinc.debug.Logging;
import android.os.ConditionVariable;
import android.os.Looper;
import android.util.Log;

/**
 * @author mat
 *
 */
public class BugCatcherThread extends Thread {

private final static String TAG = "InstallerThread";
	
	private ConditionVariable mLock;
	private BugCatcherService mBugCatcherService;
	private BugCatcherService.ListenerHandler mListenerHandler;
	
	private BugCatcherHandler mHandler;
	
	public BugCatcherThread(ConditionVariable lock, final BugCatcherService bugCatcherService,
			final BugCatcherService.ListenerHandler listenerHandler) {
		mListenerHandler = listenerHandler;
		mLock = lock;
		mBugCatcherService = bugCatcherService;
		setDaemon(true);
	}
	
	public BugCatcherHandler getInstallerHandler() {
		return mHandler;
	}
	
	@Override
	public void run() {
		if (Logging.DEBUG) Log.d(TAG, "run() - Started " + Thread.currentThread().toString());
		Looper.prepare();
		
		mHandler = new BugCatcherHandler(mBugCatcherService, mListenerHandler, this);
		
		if (mLock != null) {
			mLock.open();
			mLock = null;
		}
		
		// cleanup variable dependencies
		mListenerHandler = null;
		mBugCatcherService = null;
		
		Looper.loop();
		
		if (mLock != null) {
			mLock.open();
			mLock = null;
		}

		mHandler = null;
		if (Logging.DEBUG) Log.d(TAG, "run() - Finished" + Thread.currentThread().toString());
	}
	
	public void stopThread() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				if (Logging.DEBUG) Log.d(TAG, "Stopping BugCatcherThread");
				Looper.myLooper().quit();
			}
		});
	}
	
	public void saveToSDCard() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.saveToSDCard();
			}
		});
	}
	
	public void sendBugsToAuthor() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.sendBugsToAuthor();
			}
		});
	}
}
