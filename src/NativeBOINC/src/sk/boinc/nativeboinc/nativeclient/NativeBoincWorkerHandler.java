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

import java.util.Iterator;
import java.util.ArrayList;

import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import edu.berkeley.boinc.lite.Result;
import edu.berkeley.boinc.lite.RpcClient;
import edu.berkeley.boinc.nativeboinc.ExtendedRpcClient;
import edu.berkeley.boinc.nativeboinc.UpdateProjectAppsReply;
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
	private ExtendedRpcClient mRpcClient;
	
	public NativeBoincWorkerHandler(final Context context, final NativeBoincService.ListenerHandler listenerHandler,
			ExtendedRpcClient rpcClient) {
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
		
		ArrayList<Result> results = mRpcClient.getActiveResults();
		
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
	 * get results
	 * 
	 */
	public void getResults(NativeBoincResultsListener callback) {
		if (Logging.DEBUG) Log.d(TAG, "Get results from native client");
		
		if (mRpcClient == null) {
			notifyNativeBoincServiceError(mContext.getString(R.string.nativeClientResultsError));
			return;
		}
		ArrayList<Result> results = mRpcClient.getResults();
		if (results == null) {
			notifyNativeBoincServiceError(mContext.getString(R.string.nativeClientResultsError));
			return;
		}
		notifyResults(callback, results);
	}
	
	private boolean mUpdatingPollerIsRun = false;
	private ArrayList<String> mUpdatingProjects = new ArrayList<String>();
	
	private Runnable mUpdatingPoller = new Runnable() {
		@Override
		public void run() {
			int count = 0;
			UpdateProjectAppsReply reply = null;
			Iterator<String> listIter = mUpdatingProjects.iterator();
			
			if (mRpcClient == null) {
				mUpdatingProjects.clear();
				return;
			}
			while (listIter.hasNext()) {
				String projectUrl = listIter.next();
				reply = mRpcClient.updateProjectAppsPoll(projectUrl);
				if (reply.error_num == RpcClient.ERR_IN_PROGRESS) {
					count++;
				} else {
					// remove finished
					listIter.remove();
					
					if (reply.error_num == 0)
						notifyUpdatedProject(projectUrl);
					else	// if error
						notifyNativeBoincServiceError(mContext.getString(R.string.nativeClientResultsError)+
								" "+projectUrl);
				}
			}
			if (count != 0) // do next update
				postDelayed(mUpdatingPoller, 1000);
		}
	};
	
	public void updateProjectApps(String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "Update projects apps");
		
		if (mRpcClient == null) {
			notifyNativeBoincServiceError(mContext.getString(R.string.nativeClientResultsError));
			return;
		}
		
		if (mUpdatingProjects.contains(projectUrl)) // do nothing already updating
			return;
		
		if (mRpcClient.updateProjectApps(projectUrl)) {
			notifyNativeBoincServiceError(mContext.getString(R.string.nativeClientResultsError)+
					" "+projectUrl);
		} else
			mUpdatingProjects.add(projectUrl);
		
		if (!mUpdatingPollerIsRun) {
			mUpdatingPollerIsRun = true;
			postAtTime(mUpdatingPoller, 1000);
		}
	}
	
	/**
	 * shutdowns client
	 */
	public void shutdownClient() {
		if (mRpcClient != null) {
			removeCallbacks(mUpdatingPoller);
			mRpcClient.quit();
			mRpcClient.close();
			mRpcClient = null;
		}
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
	
	private synchronized void notifyNativeBoincServiceError(final String message) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.nativeBoincServiceError(message);
			}
		});
	}
	
	private synchronized void notifyResults(final NativeBoincResultsListener callback,
			final ArrayList<Result> results) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.getResults(callback, results);
			}
		});
	}
	
	private synchronized void notifyUpdatedProject(final String projectUrl) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.updatedProjectApps(projectUrl);
			}
		});
	}
}
