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

import java.util.Vector;

import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import edu.berkeley.boinc.lite.Result;
import edu.berkeley.boinc.lite.RpcClient;
import android.content.Context;
import android.os.Handler;
import android.util.Log;

/**
 * @author mat
 *
 */
public class NativeBoincWorkerHandler extends Handler {
	private static final String TAG = "NativeBoincWorkerHandler";
	
	private Context mContext;
	private NativeBoincService.ListenerHandler mListenerHandler;
	private RpcClient mRpcClient;
	
	public NativeBoincWorkerHandler(final Context context, final NativeBoincService.ListenerHandler listenerHandler,
			RpcClient rpcClient) {
		mContext = context;
		mListenerHandler = listenerHandler;
		mRpcClient = rpcClient;
	}
	
	/**
	 * getGlobalProgress
	 */
	public void getGlobalProgress(NativeBoincReplyListener callback) {
		if (Logging.DEBUG) Log.d(TAG, "Get global progress");
		if (mRpcClient == null || !mRpcClient.isConnected()) {
			notifyGlobalProgress(callback, -1.0);
			return;
		}
		
		Vector<Result> results = mRpcClient.getResults();
		
		if (results == null) {
			notifyGlobalProgress(callback, -1.0);
			return;
		}
		
		int taskCount = 0;
		double globalProgress = 0.0;
		
		for (Result result: results)
			/* only if running */
			if (result.state == 2 && !result.suspended_via_gui && !result.project_suspended_via_gui &&
					result.active_task_state == 1) {
				globalProgress += result.fraction_done*100.0;
				taskCount++;
			}
		
		if (taskCount == 0) {	// no tasks
			notifyGlobalProgress(callback, -1.0);
			return;
		}
		notifyGlobalProgress(callback, globalProgress / taskCount);
	}
	
	/**
	 * configureClient
	 */
	public void configureClient(NativeBoincReplyListener callback) {
		if (Logging.DEBUG) Log.d(TAG, "Configure native client");
		if (mRpcClient == null) {
			notifyNativeBoincServiceError(callback, mContext.getString(R.string.nativeClientConfigError));
			return;
		}
		if (!mRpcClient.setGlobalPrefsOverride(NativeBoincService.INITIAL_BOINC_CONFIG)) {
			notifyNativeBoincServiceError(callback, mContext.getString(R.string.nativeClientConfigError));
			return;
		}
		
		if (!mRpcClient.readGlobalPrefsOverride()) {
			notifyNativeBoincServiceError(callback, mContext.getString(R.string.nativeClientConfigError));
			return;
		}
		
		notifyClientConfigured(callback);
	}
	
	/**
	 * get results
	 * 
	 */
	public void getResults(NativeBoincResultsListener callback) {
		if (Logging.DEBUG) Log.d(TAG, "Get results from native client");
		
		if (mRpcClient == null) {
			notifyNativeBoincServiceError(callback, mContext.getString(R.string.nativeClientResultsError));
			return;
		}
		Vector<Result> results = mRpcClient.getResults();
		if (results == null) {
			notifyNativeBoincServiceError(callback, mContext.getString(R.string.nativeClientResultsError));
			return;
		}
		notifyResults(callback, results);
	}
	
	/**
	 * shutdowns client
	 */
	public void shutdownClient() {
		if (mRpcClient != null) {
			mRpcClient.quit();
			mRpcClient.close();
			mRpcClient = null;
		}
	}
	
	private synchronized void notifyClientConfigured(final NativeBoincReplyListener callback) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onClientConfigured(callback);
			}
		});
	}
	
	private synchronized void notifyGlobalProgress(final NativeBoincReplyListener callback,
			final double progress) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onProgressChange(callback, progress);
			}
		});
	}
	
	private synchronized void notifyNativeBoincServiceError(final AbstractNativeBoincListener callback,
			final String message) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.nativeBoincServiceError(callback, message);
			}
		});
	}
	
	private synchronized void notifyResults(final NativeBoincResultsListener callback,
			final Vector<Result> results) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.getResults(callback, results);
			}
		});
	}
}
