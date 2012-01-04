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

import edu.berkeley.boinc.lite.RpcClient;
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
	private RpcClient mRpcClient;
	
	public NativeBoincWorkerThread(NativeBoincService.ListenerHandler listenerHandler, final Context context,
			final ConditionVariable lock, final RpcClient rpcClient) {
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
		mHandler = null;
		if (Logging.DEBUG) Log.d(TAG, "run() - Finished" + Thread.currentThread().toString());
	}
	
	public void getGlobalProgress(final NativeBoincReplyListener callback) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.getGlobalProgress(callback);
			}
		});
	}
	
	public void configureClient(final NativeBoincReplyListener callback) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.configureClient(callback);
			}
		});
	}
	
	public void getResults(final NativeBoincResultsListener callback) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.getResults(callback);
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
