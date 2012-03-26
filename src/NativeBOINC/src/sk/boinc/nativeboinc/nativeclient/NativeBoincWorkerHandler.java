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

import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.ArrayList;
import java.util.Set;

import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.bridge.Formatter;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.TaskItem;
import edu.berkeley.boinc.lite.App;
import edu.berkeley.boinc.lite.AppVersion;
import edu.berkeley.boinc.lite.CcState;
import edu.berkeley.boinc.lite.Project;
import edu.berkeley.boinc.lite.Result;
import edu.berkeley.boinc.lite.RpcClient;
import edu.berkeley.boinc.lite.Workunit;
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
	
	private Formatter mFormatter = null;
	
	private HashMap<String, App> mApps = new HashMap<String, App>();
	private HashMap<String, Workunit> mWorkunits = new HashMap<String, Workunit>();
	private HashMap<String, Project> mProjects = new HashMap<String, Project>();
	private HashMap<String, TaskItem> mTasks = new HashMap<String, TaskItem>();
	
	public NativeBoincWorkerHandler(final Context context, final NativeBoincService.ListenerHandler listenerHandler,
			ExtendedRpcClient rpcClient) {
		mContext = context;
		mListenerHandler = listenerHandler;
		mRpcClient = rpcClient;
		mFormatter = new Formatter(context);
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
	
	private boolean updateState() {
		CcState ccState = mRpcClient.getState();
		
		if (ccState == null)  {
			notifyNativeBoincServiceError(mContext.getString(R.string.nativeClientResultsError));
			return false;
		}
		
		mProjects.clear();
		for (Project project: ccState.projects)
			mProjects.put(project.master_url, project);
		
		mApps.clear();
		for (App app: ccState.apps)
			mApps.put(app.name, app);
		
		mWorkunits.clear();
		for (Workunit workunit: ccState.workunits)
			mWorkunits.put(workunit.name, workunit);
				
		mTasks.clear();
		/* update tasks */
		for (Result result: ccState.results) {
			Project project = mProjects.get(result.project_url);
			Workunit workunit = mWorkunits.get(result.wu_name);
			App app = mApps.get(workunit.app_name);
			if (project == null || workunit == null || app == null) {
				if (Logging.WARNING) Log.d(TAG, "Warning datasets are incomplete! skipping result "+
							result.name);
				continue;
			}
			TaskItem taskItem = new TaskItem(project, workunit, app, result, mFormatter);
			mTasks.put(result.name, taskItem);
		}
		return true;
	}
	
	private boolean updateTasks(ArrayList<Result> results) {
		Set<String> obsoleteTasks = new HashSet<String>(mTasks.keySet());
		for (Result result: results) {
			TaskItem taskItem = mTasks.get(result.name);
			if (taskItem == null) {
				obsoleteTasks.add(result.name);
				return false;
			}
			taskItem.update(result, mFormatter);
			// if updated
			obsoleteTasks.remove(result.name);
		}
		// remove obsolete tasks
		for (String obsolete: obsoleteTasks)
			mTasks.remove(obsolete);
		return true;
	}
	
	/**
	 * get tasks
	 * 
	 */
	public void getTasks(NativeBoincTasksListener callback) {
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
		// try to update results
		if (!updateTasks(results)) {
			// try to update whole ccstate
			if (!updateState())
				return;
		}
		notifyResults(callback, new ArrayList<TaskItem>(mTasks.values()));
	}
	
	/**
	 * get projects
	 * 
	 */
	public void getProjects(NativeBoincProjectsListener callback) {
		if (Logging.DEBUG) Log.d(TAG, "Get projects from native client");
		
		if (mRpcClient == null) {
			notifyNativeBoincServiceError(mContext.getString(R.string.nativeClientProjectsError));
			return;
		}
		ArrayList<Project> projects = mRpcClient.getProjectStatus();
		if (projects == null) {
			notifyNativeBoincServiceError(mContext.getString(R.string.nativeClientProjectsError));
			return;
		}
		notifyProjects(callback, projects);
	}
	
	private boolean mUpdatingPollerIsRun = false;
	private ArrayList<String> mUpdatingProjects = new ArrayList<String>();
	
	private Runnable mUpdatingPoller = new Runnable() {
		@Override
		public void run() {
			int count = 0;
			UpdateProjectAppsReply reply = null;
			
			if (mRpcClient == null) {
				mUpdatingProjects.clear();
				return;
			}
			
			Iterator<String> listIter = mUpdatingProjects.iterator();
			
			while (listIter.hasNext()) {
				String projectUrl = listIter.next();
				reply = mRpcClient.updateProjectAppsPoll(projectUrl);
				if (reply.error_num == RpcClient.ERR_IN_PROGRESS) {
					count++;
				} else {
					// remove finished
					listIter.remove();
					
					if (reply.error_num == 0) {
						if (Logging.INFO) Log.i(TAG, "after update_apps for "+projectUrl);
						notifyUpdatedProject(projectUrl);
					}
					else {	// if error
						if (Logging.WARNING) Log.w(TAG, "error in updating app for "+projectUrl +
								",errornum:" +reply.error_num);
						notifyNativeBoincServiceError(mContext.getString(R.string.nativeClientResultsError)+
								" "+projectUrl);
					}
				}
			}
			if (count != 0) { // do next update
				if (Logging.DEBUG) Log.d(TAG, "polling update_apps");
				postDelayed(mUpdatingPoller, 1000);
			} else
				mUpdatingPollerIsRun = false;
		}
	};
	
	/**
	 * update project apps
	 * @param projectUrl
	 */
	public void updateProjectApps(String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "Update projects apps");
		
		if (mRpcClient == null)
			return;
		
		if (mUpdatingProjects.contains(projectUrl)) // do nothing already updating
			return;
		
		if (!mRpcClient.updateProjectApps(projectUrl)) {
			if (Logging.WARNING) Log.w(TAG, "updatProjectApps error "+projectUrl);
			notifyNativeBoincServiceError(mContext.getString(R.string.nativeClientResultsError)+
					" "+projectUrl);
		} else
			mUpdatingProjects.add(projectUrl);
		
		if (!mUpdatingPollerIsRun) {
			mUpdatingPollerIsRun = true;
			postDelayed(mUpdatingPoller, 1000);
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
	
	private synchronized void notifyResults(final NativeBoincTasksListener callback,
			final ArrayList<TaskItem> tasks) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.getTasks(callback, tasks);
			}
		});
	}
	
	private synchronized void notifyProjects(final NativeBoincProjectsListener callback,
			final ArrayList<Project> projects) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.getProjects(callback, projects);
			}
		});
	}
	
	private synchronized void notifyUpdatedProject(final String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "Notify updated project:"+projectUrl);
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.updatedProjectApps(projectUrl);
			}
		});
	}
}
