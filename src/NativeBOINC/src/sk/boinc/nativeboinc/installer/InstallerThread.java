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

import java.util.ArrayList;

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.UpdateItem;
import android.os.ConditionVariable;
import android.os.Looper;
import android.util.Log;

/**
 * @author mat
 *
 */
public class InstallerThread extends Thread {
	private final static String TAG = "InstallerThread";
	
	private ConditionVariable mLock;
	private InstallerService mInstallerService;
	private InstallerService.ListenerHandler mListenerHandler;
	
	private InstallerHandler mHandler;
	
	public InstallerThread(ConditionVariable lock, final InstallerService installerService,
			final InstallerService.ListenerHandler listenerHandler) {
		mListenerHandler = listenerHandler;
		mLock = lock;
		mInstallerService = installerService;
		setDaemon(true);
	}
	
	public InstallerHandler getInstallerHandler() {
		return mHandler;
	}
	
	@Override
	public void run() {
		if (Logging.DEBUG) Log.d(TAG, "run() - Started " + Thread.currentThread().toString());
		Looper.prepare();
		
		mHandler = new InstallerHandler(mInstallerService, mListenerHandler);
		
		if (mLock != null) {
			mLock.open();
			mLock = null;
		}
		
		// cleanup variable dependencies
		mListenerHandler = null;
		mInstallerService = null;
		
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
				if (Logging.DEBUG) Log.d(TAG, "Stopping InstallerThread");
				Looper.myLooper().quit();
			}
		});
	}
	
	public void updateClientDistrib(final int channelId) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateClientDistrib(channelId);
			}
		});
	}
	
	public void updateProjectDistribList(final int channelId) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateProjectDistribList(channelId);
			}
		});
	}
	
	public void installClientAutomatically() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.installClientAutomatically(true);
			}
		});
	}
	
	public void updateClientFromSDCard() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				//mHandler.installClientAutomatically(true);
			}
		});
	}
	
	public void installProjectApplicationAutomatically(final String projectUrl) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.installProjectApplicationsAutomatically(projectUrl);
			}
		});
	}
	
	public void updateProjectApplicationFromSDCard(final String projectUrl) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				//mHandler.installProjectApplicationsAutomatically(projectUrl);
			}
		});
	}
	
	public void reinstallUpdateItems(final ArrayList<UpdateItem> updateItems) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.reinstallUpdateItems(updateItems);
			}
		});
	}
	
	public void updateDistribsFromSDCard(final String dirPath, final String[] distribNames) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateDistribsFromSDCard(dirPath, distribNames);
			}
		});
	}
	
	public void getBinariesToUpdateOrInstall(final int channelId) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.getBinariesToUpdateOrInstall(channelId);
			}
		});
	}
	
	public void getBinariesToUpdateFromSDCard(final int channelId, final String path) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.getBinariesToUpdateFromSDCard(channelId, path);
			}
		});
	}
	
	public void dumpBoincFiles(final String directory) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.dumpBoincFiles(directory);
			}
		});
	}
	
	public void reinstallBoinc() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.reinstallBoinc();
			}
		});
	}
}
