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

import edu.berkeley.boinc.nativeboinc.BatteryInfo;
import edu.berkeley.boinc.nativeboinc.ExtendedRpcClient;
import sk.boinc.nativeboinc.debug.Logging;
import android.content.Context;
import android.os.ConditionVariable;
import android.os.Looper;
import android.util.Log;

/**
 * @author mat
 *
 */
public class NativeBoincWorkerThread extends Thread {
	private final static String TAG = "NativeBoincWorkerThread";
	
	private NativeBoincService.ListenerHandler mListenerHandler;
	private ConditionVariable mLock;
	private Context mContext;
	private NativeBoincWorkerHandler mHandler;
	private ExtendedRpcClient mRpcClient;
	
	public NativeBoincWorkerThread(NativeBoincService.ListenerHandler listenerHandler, final Context context,
			final ConditionVariable lock, final ExtendedRpcClient rpcClient) {
		mListenerHandler = listenerHandler;
		mLock = lock;
		mContext = context;
		mRpcClient = rpcClient;
		setDaemon(true);
	}
	
	@Override
	public void run() {
		if (Logging.DEBUG) Log.d(TAG, "run() - Started " + Thread.currentThread().toString());
		Looper.prepare();
		
		mHandler = new NativeBoincWorkerHandler(mContext, mListenerHandler, mRpcClient);
		
		if (mLock != null) {
			mLock.open();
			mLock = null;
		}
		
		// cleanup variable dependencies
		mListenerHandler = null;
		mContext = null;
		
		
		Looper.loop();
		
		if (mLock != null) {
			mLock.open();
			mLock = null;
		}

		mRpcClient = null;
		mHandler.destroy();
		mHandler = null;
		if (Logging.DEBUG) Log.d(TAG, "run() - Finished" + Thread.currentThread().toString());
	}
	
	public void getGlobalProgress(final int channelId) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.getGlobalProgress(channelId);
			}
		});
	}
	
	public void getTasks(final int channelId) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.getTasks(channelId);
			}
		});
	}
	
	public void getProjects(final int channelId) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.getProjects(channelId);
			}
		});
	}
	
	public void shutdownClient() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.shutdownClient();
			}
		});
	}
	
	public void updateProjectApps(final int channelId, final String projectUrl) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateProjectApps(channelId, projectUrl);
			}
		});
	}
	
	public void doNetworkCommunication(final int channelId) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.doNetworkCommunication(channelId);
			}
		});
	}
	
	public void sendBatteryInfo(final int channelId, final BatteryInfo batteryInfo) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.sendBatteryInfo(channelId, batteryInfo);
			}
		});
	}
	
	public void stopThread() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				if (Logging.DEBUG) Log.d(TAG, "Quit message received, stopping " + Thread.currentThread().toString());
				Looper.myLooper().quit();
			}
		});
	}
}
