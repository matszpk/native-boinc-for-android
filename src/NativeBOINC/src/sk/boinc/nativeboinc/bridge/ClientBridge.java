/* 
 * AndroBOINC - BOINC Manager for Android
 * Copyright (C) 2010, Pavol Michalec
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

package sk.boinc.nativeboinc.bridge;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.ArrayList;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import edu.berkeley.boinc.lite.AccountIn;
import edu.berkeley.boinc.lite.AccountMgrInfo;
import edu.berkeley.boinc.lite.GlobalPreferences;
import edu.berkeley.boinc.lite.ProjectConfig;
import edu.berkeley.boinc.lite.ProjectListEntry;

import sk.boinc.nativeboinc.clientconnection.ClientAllProjectsListReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientAccountMgrReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientError;
import sk.boinc.nativeboinc.clientconnection.ClientPollErrorReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientPreferencesReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientProjectReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientReplyReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientRequestHandler;
import sk.boinc.nativeboinc.clientconnection.HostInfo;
import sk.boinc.nativeboinc.clientconnection.MessageInfo;
import sk.boinc.nativeboinc.clientconnection.ModeInfo;
import sk.boinc.nativeboinc.clientconnection.PollError;
import sk.boinc.nativeboinc.clientconnection.PollOp;
import sk.boinc.nativeboinc.clientconnection.ProjectInfo;
import sk.boinc.nativeboinc.clientconnection.TaskInfo;
import sk.boinc.nativeboinc.clientconnection.TransferInfo;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.debug.NetStats;
import sk.boinc.nativeboinc.util.ClientId;
import android.content.Context;
import android.os.ConditionVariable;
import android.os.Handler;
import android.util.Log;


/**
 * The <code>ClientBridge</code> runs in UI thread. 
 * Right on its own creation it creates another thread, the worker thread.
 * When request is received at this class, it is posted to worker thread.
 */
public class ClientBridge implements ClientRequestHandler {
	private static final String TAG = "ClientBridge";

	private ConcurrentMap<String, ProjectConfig> mPendingProjectConfigs =
			new ConcurrentHashMap<String, ProjectConfig>();
	
	// for joining two requests in one (create/lookup and attach)
	private ConcurrentMap<String, String> mJoinedAddProjectsMap = new ConcurrentHashMap<String, String>();
	
	private ConcurrentMap<String, PollError> mPollErrorsMap = new ConcurrentHashMap<String, PollError>();
	
	private ClientError mPendingClientError = null;
	
	private boolean mIsWorking = false;
	
	public class ReplyHandler extends Handler {
		private static final String TAG = "ClientBridge.ReplyHandler";

		public void disconnecting() {
			// The worker thread started disconnecting
			// This means, that all actions needed to close connection are either
			// processed already, or waiting in queue to be processed
			// We can start clearing of worker thread (will be also done as post,
			// so it will be executed afterwards)
			if (Logging.DEBUG) Log.d(TAG, "disconnecting(), stopping ClientBridgeWorkerThread");
			mIsWorking = false;
			mWorker.stopThread(null);
			// Clean up periodic updater as well, because no more periodic updates will be needed
			mAutoRefresh.cleanup();
			mAutoRefresh = null;
		}

		public void disconnected() {
			if (Logging.DEBUG) Log.d(TAG, "disconnected()");
			// The worker thread was cleared completely 
			mIsWorking = false;
			mWorker = null;
			if (mCallback != null) {
				// We are ready to be deleted
				// The callback can now delete reference to us, so this
				// object can be garbage collected then
				mCallback.bridgeDisconnected(ClientBridge.this);
				mCallback = null;
			}
		}

		public void notifyProgress(int progress) {
			if (progress == ClientReceiver.PROGRESS_XFER_FINISHED)
				mIsWorking = false;
			Iterator<ClientReceiver> it = mObservers.iterator();
			while (it.hasNext()) {
				ClientReceiver observer = it.next();
				observer.clientConnectionProgress(progress);
			}
		}
		
		public void notifyError(int errorNum, String errorMessage) {
			mIsWorking = false;
			Iterator<ClientReceiver> it = mObservers.iterator();
			
			boolean called = false;
			
			while (it.hasNext()) {
				ClientReceiver observer = it.next();
				observer.clientError(errorNum, errorMessage);
				called = true;
			}
			
			synchronized(ClientBridge.this) {
				if (!called)
					mPendingClientError = new ClientError(errorNum, errorMessage);
				else	// if error already handled 
					mPendingClientError = null;
			}
		}
		
		public void notifyPollError(int errorNum, int operation, String errorMessage, String param) {
			mIsWorking = false;
			
			if (operation == PollOp.POLL_CREATE_ACCOUNT ||
					operation == PollOp.POLL_LOOKUP_ACCOUNT ||
					operation == PollOp.POLL_PROJECT_ATTACH) {
				// addProject operation
				mJoinedAddProjectsMap.remove(param);
			}
			
			Iterator<ClientReceiver> it = mObservers.iterator();
			
			boolean called = false;
			
			while (it.hasNext()) {
				ClientReceiver observer = it.next();
				if (observer instanceof ClientPollErrorReceiver) {
					((ClientPollErrorReceiver)observer).onPollError(errorNum, operation,
							errorMessage, param);
					called = true;
				}
			}
			synchronized(ClientBridge.this) {
				if (!called) {
					PollError pollError = new PollError(errorNum, operation, errorMessage, param);
					if (param != null)
						mPollErrorsMap.put(param, pollError);
					else
						mPollErrorsMap.put("", pollError);
				} else { // if already handled (remove old errors)
					if (param != null)
						mPollErrorsMap.remove(param);
					else
						mPollErrorsMap.remove("");
				}
			}
		}
		
		public void notifyConnected(VersionInfo clientVersion) {
			mIsWorking = false;
			mConnected = true;
			mRemoteClientVersion = clientVersion;
			Iterator<ClientReceiver> it = mObservers.iterator();
			while (it.hasNext()) {
				ClientReceiver observer = it.next();
				observer.clientConnected(mRemoteClientVersion);
			}
		}

		public void notifyDisconnected() {
			mIsWorking = false;
			mConnected = false;
			mRemoteClientVersion = null;
			Iterator<ClientReceiver> it = mObservers.iterator();
			while (it.hasNext()) {
				ClientReceiver observer = it.next();
				observer.clientDisconnected();
				if (Logging.DEBUG) Log.d(TAG, "Detached observer: " + observer.toString()); // see below clearing of all observers
			}
			mObservers.clear();
		}


		public void updatedClientMode(final ClientReceiver callback, final ModeInfo modeInfo) {
			mIsWorking = false;
			if (callback == null) {
				// No specific callback - broadcast to all observers
				// This is used for early notification after connect
				Iterator<ClientReceiver> it = mObservers.iterator();
				while (it.hasNext()) {
					ClientReceiver observer = it.next();
					if (observer instanceof ClientReplyReceiver)
						((ClientReplyReceiver)observer).updatedClientMode(modeInfo);
				}
				return;
			}
			// Check whether callback is still present in observers
			if (mObservers.contains(callback) && callback instanceof ClientReplyReceiver) {
				// Observer is still present, so we can call it back with data
				boolean periodicAllowed = ((ClientReplyReceiver)callback).updatedClientMode(modeInfo);
				if (periodicAllowed) {
					mAutoRefresh.scheduleAutomaticRefresh(
							(ClientReplyReceiver)callback, AutoRefresh.CLIENT_MODE);
				}
			}
		}

		public void updatedHostInfo(final ClientReplyReceiver callback, final HostInfo hostInfo) {
			mIsWorking = false;
			// First, check whether callback is still present in observers
			if (mObservers.contains(callback)) {
				// Yes, observer is still present, so we can call it back with data
				callback.updatedHostInfo(hostInfo);
			}
		}
		
		public void currentBAMInfo(final ClientAccountMgrReceiver callback, final AccountMgrInfo bamInfo) {
			mIsWorking = false;
			// First, check whether callback is still present in observers
			if (mObservers.contains(callback)) {
				// Yes, observer is still present, so we can call it back with data
				callback.currentBAMInfo(bamInfo);
			}
		}
		
		public void currentAllProjectsList(final ClientAllProjectsListReceiver callback,
				final ArrayList<ProjectListEntry> projects) {
			mIsWorking = false;
			// First, check whether callback is still present in observers
			if (mObservers.contains(callback)) {
				// Yes, observer is still present, so we can call it back with data
				callback.currentAllProjectsList(projects);
			}
		}
		
		public void currentAuthCode(final String projectUrl, final String authCode) {
			boolean joinedOps = mJoinedAddProjectsMap.containsKey(projectUrl);
			mIsWorking = joinedOps;	// if joined then still working mode
			// First, check whether callback is still present in observers
			Iterator<ClientReceiver> it = mObservers.iterator();
			while (it.hasNext()) {
				ClientReceiver observer = it.next();
				if (observer instanceof ClientProjectReceiver)
					((ClientProjectReceiver)observer).currentAuthCode(projectUrl, authCode);
			}
			
			if (mJoinedAddProjectsMap.containsKey(projectUrl)) {
				projectAttach(projectUrl, authCode, "");
			}
		}
		
		public void currentProjectConfig(final ProjectConfig projectConfig) {
			mIsWorking = false;
			// First, check whether callback is still present in observers
			mPendingProjectConfigs.put(projectConfig.master_url, projectConfig);
			Iterator<ClientReceiver> it = mObservers.iterator();
			while (it.hasNext()) {
				ClientReceiver observer = it.next();
				if (observer instanceof ClientProjectReceiver)
					((ClientProjectReceiver)observer).currentProjectConfig(projectConfig);
			}
		}
		
		public void currentGlobalPreferences(final ClientPreferencesReceiver callback,
				final GlobalPreferences globalPrefs) {
			mIsWorking = false;
			// First, check whether callback is still present in observers
			if (mObservers.contains(callback)) {
				// Yes, observer is still present, so we can call it back with data
				callback.currentGlobalPreferences(globalPrefs);
			}
		}
		
		public void afterAccountMgrRPC() {
			mIsWorking = false;
			// First, check whether callback is still present in observers
			Iterator<ClientReceiver> it = mObservers.iterator();
			while (it.hasNext()) {
				ClientReceiver observer = it.next();
				if (observer instanceof ClientAccountMgrReceiver)
					((ClientAccountMgrReceiver)observer).onAfterAccountMgrRPC();
			}
		}
		
		public void afterProjectAttach(final String projectUrl) {
			mIsWorking = false;
			// First, check whether callback is still present in observers
			if (Logging.DEBUG) Log.d(TAG, "Remove after project attach:"+projectUrl);
			mJoinedAddProjectsMap.remove(projectUrl);
			Iterator<ClientReceiver> it = mObservers.iterator();
			while (it.hasNext()) {
				ClientReceiver observer = it.next();
				if (observer instanceof ClientProjectReceiver)
					((ClientProjectReceiver)observer).onAfterProjectAttach(projectUrl);
			}
		}
		
		public void onGlobalPreferencesChanged(final ClientPreferencesReceiver callback) {
			mIsWorking = false;
			// First, check whether callback is still present in observers
			if (mObservers.contains(callback)) {
				// Yes, observer is still present, so we can call it back with data
				callback.onGlobalPreferencesChanged();
			}
		}

		public void updatedProjects(final ClientReplyReceiver callback, final ArrayList <ProjectInfo> projects) {
			mIsWorking = false;
			if (callback == null) {
				// No specific callback - broadcast to all observers
				// This is used for early notification after connect
				Iterator<ClientReceiver> it = mObservers.iterator();
				while (it.hasNext()) {
					ClientReceiver observer = it.next();
					if (observer instanceof ClientReplyReceiver)
						((ClientReplyReceiver)observer).updatedProjects(projects);
				}
				return;
			}
			// Check whether callback is still present in observers
			if (mObservers.contains(callback)) {
				// Yes, observer is still present, so we can call it back with data
				boolean periodicAllowed = callback.updatedProjects(projects);
				if (periodicAllowed) {
					mAutoRefresh.scheduleAutomaticRefresh(callback, AutoRefresh.PROJECTS);
				}
			}
		}

		public void updatedTasks(final ClientReplyReceiver callback, final ArrayList <TaskInfo> tasks) {
			mIsWorking = false;
			if (callback == null) {
				// No specific callback - broadcast to all observers
				// This is used for early notification after connect
				Iterator<ClientReceiver> it = mObservers.iterator();
				while (it.hasNext()) {
					ClientReceiver observer = it.next();
					if (observer instanceof ClientReplyReceiver)
						((ClientReplyReceiver)observer).updatedTasks(tasks);
				}
				return;
			}
			// Check whether callback is still present in observers
			if (mObservers.contains(callback)) {
				// Yes, observer is still present, so we can call it back with data
				boolean periodicAllowed = callback.updatedTasks(tasks);
				if (periodicAllowed) {
					mAutoRefresh.scheduleAutomaticRefresh(callback, AutoRefresh.TASKS);
				}
			}
		}

		public void updatedTransfers(final ClientReplyReceiver callback, final ArrayList <TransferInfo> transfers) {
			mIsWorking = false;
			if (callback == null) {
				// No specific callback - broadcast to all observers
				// This is used for early notification after connect
				Iterator<ClientReceiver> it = mObservers.iterator();
				while (it.hasNext()) {
					ClientReceiver observer = it.next();
					if (observer instanceof ClientReplyReceiver)
						((ClientReplyReceiver)observer).updatedTransfers(transfers);
				}
				return;
			}
			// Check whether callback is still present in observers
			if (mObservers.contains(callback)) {
				// Yes, observer is still present, so we can call it back with data
				boolean periodicAllowed = callback.updatedTransfers(transfers);
				if (periodicAllowed) {
					mAutoRefresh.scheduleAutomaticRefresh(callback, AutoRefresh.TRANSFERS);
				}
			}
		}

		public void updatedMessages(final ClientReplyReceiver callback, final ArrayList <MessageInfo> messages) {
			mIsWorking = false;
			if (callback == null) {
				// No specific callback - broadcast to all observers
				// This is used for early notification after connect
				Iterator<ClientReceiver> it = mObservers.iterator();
				while (it.hasNext()) {
					ClientReceiver observer = it.next();
					if (observer instanceof ClientReplyReceiver)
						((ClientReplyReceiver)observer).updatedMessages(messages);
				}
				return;
			}
			// Check whether callback is still present in observers
			if (mObservers.contains(callback)) {
				// Yes, observer is still present, so we can call it back with data
				boolean periodicAllowed = callback.updatedMessages(messages);
				if (periodicAllowed) {
					mAutoRefresh.scheduleAutomaticRefresh(callback, AutoRefresh.MESSAGES);
				}
			}
		}
	}

	protected final ReplyHandler mReplyHandler = new ReplyHandler();

	private Set<ClientReceiver> mObservers = new HashSet<ClientReceiver>();
	protected boolean mConnected = false;

	protected ClientBridgeCallback mCallback = null;
	protected ClientBridgeWorkerThread mWorker = null;

	protected ClientId mRemoteClient = null;
	private VersionInfo mRemoteClientVersion = null;

	protected AutoRefresh mAutoRefresh = null;

	public ClientBridge() {
	}
	
	/**
	 * Constructs a new <code>ClientBridge</code> and starts worker thread
	 * 
	 * @throws RuntimeException if worker thread cannot start in a timely fashion
	 */
	public ClientBridge(ClientBridgeCallback callback, NetStats netStats) throws RuntimeException {
		mCallback = callback;
		if (Logging.DEBUG) Log.d(TAG, "Starting ClientBridgeWorkerThread");
		ConditionVariable lock = new ConditionVariable(false);
		Context context = (Context)callback;
		mAutoRefresh = new AutoRefresh(context, this);
		mWorker = new ClientBridgeWorkerThread(lock, mReplyHandler, context, netStats);
		mWorker.start();
		boolean runningOk = lock.block(2000); // Locking until new thread fully runs
		if (!runningOk) {
			// Too long time waiting for worker thread to be on-line - cancel it
			if (Logging.ERROR) Log.e(TAG, "ClientBridgeWorkerThread did not start in 1 second");
			throw new RuntimeException("Worker thread cannot start");
		}
		if (Logging.DEBUG) Log.d(TAG, "ClientClientBridgeWorkerThread started successfully");
	}

	@Override
	public void registerStatusObserver(ClientReceiver observer) {
		// Another observer wants to be notified - add him into collection of observers
		mObservers.add(observer);
		if (Logging.DEBUG) Log.d(TAG, "Attached new observer: " + observer.toString());
		if (mConnected) {
			// New observer is attached while we are already connected
			// Notify new observer that we are connected, so it can fetch data
			observer.clientConnected(mRemoteClientVersion);
		}
	}

	@Override
	public void unregisterStatusObserver(ClientReceiver observer) {
		// Observer does not want to receive notifications anymore - remove him
		mObservers.remove(observer);
		if (mConnected) {
			// The observer could have automatic refresh pending
			// Remove it now
			if (observer instanceof ClientReplyReceiver)
				mAutoRefresh.unscheduleAutomaticRefresh((ClientReplyReceiver)observer);
		}
		if (Logging.DEBUG) Log.d(TAG, "Detached observer: " + observer.toString());
	}

	public boolean isWorking() {
		return mIsWorking;
	}
	
	@Override
	public void connect(final ClientId remoteClient, final boolean retrieveInitialData) {
		if (mRemoteClient != null) {
			// already connected
			if (Logging.ERROR) Log.e(TAG, "Request to connect to: " + remoteClient.getNickname() + " while already connected to: " + mRemoteClient.getNickname());
			return;
		}
		mRemoteClient = remoteClient;
		mIsWorking = true;
		mWorker.connect(remoteClient, retrieveInitialData);
	}
	
	@Override
	public void disconnect() {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.disconnect();
		mRemoteClient = null; // This will prevent further triggers towards worker thread
	}

	@Override
	public final ClientId getClientId() {
		return mRemoteClient;
	}

	@Override
	public void updateClientMode(final ClientReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.updateClientMode(callback);
	}

	@Override
	public void updateHostInfo(final ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.updateHostInfo(callback);
	}

	@Override
	public void updateProjects(final ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.updateProjects(callback);
	}

	@Override
	public void updateTasks(final ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.updateTasks(callback);
	}

	@Override
	public void updateTransfers(final ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.updateTransfers(callback);
	}

	@Override
	public void updateMessages(final ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.updateMessages(callback);
	}

	@Override
	public void cancelScheduledUpdates(ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		// Cancel pending updates in worker thread
		mWorker.cancelPendingUpdates(callback);
		// Remove scheduled auto-refresh (if any)
		if (mAutoRefresh!=null && callback != null)
			mAutoRefresh.unscheduleAutomaticRefresh(callback);
	}

	@Override
	public void getBAMInfo(ClientAccountMgrReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.getBAMInfo(callback);
	}
	
	@Override
	public void attachToBAM(ClientAccountMgrReceiver callback, String name, String url, String password) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.attachToBAM(callback, name, url, password);
	}
	
	@Override
	public void synchronizeWithBAM(ClientAccountMgrReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.synchronizeWithBAM(callback);
	}
	
	@Override
	public void stopUsingBAM(ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.stopUsingBAM(callback);
	}
	
	@Override
	public void getAllProjectsList(ClientAllProjectsListReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.getAllProjectsList(callback);
	}
	
	@Override
	public void lookupAccount(AccountIn accountIn) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.lookupAccount(accountIn);
	}
	
	@Override
	public void createAccount(AccountIn accountIn) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.createAccount(accountIn);
	}
	
	@Override
	public void projectAttach(String url, String authCode, String projectName) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.projectAttach(url, authCode, projectName);
	}
	
	@Override
	public void addProject(AccountIn accountIn, boolean create) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		if (mJoinedAddProjectsMap.putIfAbsent(accountIn.url, "") != null)
			return;
		
		if (create)
			mWorker.createAccount(accountIn);
		else
			mWorker.lookupAccount(accountIn);
	}
	
	public boolean isProjectBeingAdded(String projectUrl) {
		if (mRemoteClient == null)
			return false;
		return mJoinedAddProjectsMap.containsKey(projectUrl);
	}
	
	@Override
	public void getProjectConfig(String url) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.getProjectConfig(url);
	}
	
	public ProjectConfig getPendingProjectConfig(String projectUrl) {
		return mPendingProjectConfigs.remove(projectUrl);
	}
	
	public ClientError getPendingClientError() {
		if (mRemoteClient == null)
			return null;
		
		synchronized(this) {
			ClientError clientError = mPendingClientError;
			mPendingClientError = null;
			return clientError;
		}
	}
	
	public PollError getPendingPollError(String projectUrl) {
		if (mRemoteClient == null)
			return null;
		if (projectUrl == null)
			mPollErrorsMap.remove("");
		return mPollErrorsMap.remove(projectUrl);
	}
	
	@Override
	public void getGlobalPrefsWorking(ClientPreferencesReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.getGlobalPrefsWorking(callback);
	}
	
	@Override
	public void setGlobalPrefsOverride(ClientPreferencesReceiver callback, String globalPrefs) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.setGlobalPrefsOverride(callback, globalPrefs);
	}
	
	@Override
	public void setGlobalPrefsOverrideStruct(ClientPreferencesReceiver callback, GlobalPreferences globalPrefs) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.setGlobalPrefsOverrideStruct(callback, globalPrefs);
	}
	
	@Override
	public void runBenchmarks() {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.runBenchmarks();
	}

	@Override
	public void setRunMode(final ClientReplyReceiver callback, final int mode) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.setRunMode(callback, mode);
	}

	@Override
	public void setNetworkMode(final ClientReplyReceiver callback, final int mode) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.setNetworkMode(callback, mode);
	}

	@Override
	public void shutdownCore() {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.shutdownCore();
	}

	@Override
	public void doNetworkCommunication() {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.doNetworkCommunication();
	}

	@Override
	public void projectOperation(final ClientReplyReceiver callback, final int operation, final String projectUrl) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.projectOperation(callback, operation, projectUrl);
	}

	@Override
	public void taskOperation(final ClientReplyReceiver callback, final int operation, final String projectUrl, final String taskName) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.taskOperation(callback, operation, projectUrl, taskName);
	}

	@Override
	public void transferOperation(final ClientReplyReceiver callback, final int operation, final String projectUrl, final String fileName) {
		if (mRemoteClient == null) return; // not connected
		mIsWorking = true;
		mWorker.transferOperation(callback, operation, projectUrl, fileName);
	}
}
