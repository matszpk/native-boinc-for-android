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

import java.util.HashMap;
import java.util.HashSet;
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
import sk.boinc.nativeboinc.clientconnection.ClientManageReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientRequestHandler;
import sk.boinc.nativeboinc.clientconnection.ClientUpdateMessagesReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientUpdateNoticesReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientUpdateProjectsReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientUpdateTasksReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientUpdateTransfersReceiver;
import sk.boinc.nativeboinc.clientconnection.HostInfo;
import sk.boinc.nativeboinc.clientconnection.MessageInfo;
import sk.boinc.nativeboinc.clientconnection.ModeInfo;
import sk.boinc.nativeboinc.clientconnection.NoticeInfo;
import sk.boinc.nativeboinc.clientconnection.PollError;
import sk.boinc.nativeboinc.clientconnection.PollOp;
import sk.boinc.nativeboinc.clientconnection.ProjectInfo;
import sk.boinc.nativeboinc.clientconnection.TaskDescriptor;
import sk.boinc.nativeboinc.clientconnection.TaskInfo;
import sk.boinc.nativeboinc.clientconnection.TransferDescriptor;
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
	
	private HashMap<String, PollError> mPollErrorsMap = new HashMap<String, PollError>();
	private HashMap<String, PollError> mProjectConfigErrorsMap = new HashMap<String, PollError>();
	private ClientError mPendingClientError = null;
	private Object mPendingClientErrorSync = new Object(); // syncer
	
	private ClientError mPendingAutoRefreshClientError = null;
	private Object mPendingAutoRefreshClientErrorSync = new Object(); // syncer
	
	private ArrayList<ProjectListEntry> mPendingAllProjectsList = null;
	private Object mPendingAllProjectsListSync = new Object(); // syncer
	
	private AccountMgrInfo mPendingAccountMgrInfo = null;
	private Object mPendingAccountMgrInfoSync = new Object(); // syncer
	
	private boolean mBAMBeingSynchronized = false;
	
	private GlobalPreferences mPendingGlobalPrefs = null;
	private Object mPendingGlobalPrefsSync = new Object(); // syncer
	
	private boolean mGlobalPrefsBeingOverriden = false;
	private HostInfo mPendingHostInfo = null;
	private Object mPendingHostInfoSync = new Object();  // syncer
	
	/* data updates */
	private ArrayList<ProjectInfo> mPendingProjects = null;
	private Object mPendingProjectsSync = new Object();
	private ArrayList<TaskInfo> mPendingTasks = null;
	private Object mPendingTasksSync = new Object();
	private ArrayList<TransferInfo> mPendingTransfers = null;
	private Object mPendingTransfersSync = new Object();
	private ArrayList<MessageInfo> mPendingMessages = null;
	private Object mPendingMessagesSync = new Object();
	private ArrayList<NoticeInfo> mPendingNotices = null;
	private Object mPendingNoticesSync = new Object();
	
	public class ReplyHandler extends Handler {
		private static final String TAG = "ClientBridge.ReplyHandler";

		public void disconnecting() {
			// The worker thread started disconnecting
			// This means, that all actions needed to close connection are either
			// processed already, or waiting in queue to be processed
			// We can start clearing of worker thread (will be also done as post,
			// so it will be executed afterwards)
			if (Logging.DEBUG) Log.d(TAG, "disconnecting(), stopping ClientBridgeWorkerThread");
			mWorker.stopThread(null);
			// Clean up periodic updater as well, because no more periodic updates will be needed
			mAutoRefresh.cleanup();
			mAutoRefresh = null;
		}

		public void disconnected() {
			if (Logging.DEBUG) Log.d(TAG, "disconnected()");
			// The worker thread was cleared completely 
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
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers)
				observer.clientConnectionProgress(progress);
		}
		
		public void notifyError(boolean autoRefresh, int errorNum, String errorMessage) {
			boolean called = false;
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer.clientError(errorNum, errorMessage))
					called = true;
			}
			
			if (!autoRefresh) // if not autorefeshed operation
				synchronized(mPendingClientErrorSync) {
					if (Logging.DEBUG) Log.d(TAG, "Is PendingClientError is set: "+(!called));
					if (!called) /* set up pending if not already handled */
						mPendingClientError = new ClientError(errorNum, errorMessage);
					else	// if error already handled 
						mPendingClientError = null;
				}
			else // use different pending error handling channel
				synchronized(mPendingAutoRefreshClientErrorSync) {
					if (Logging.DEBUG) Log.d(TAG, "Is PendingAutoRefreshClientError is set: "+(!called));
					if (!called) /* set up pending if not already handled */
						mPendingAutoRefreshClientError = new ClientError(errorNum, errorMessage);
					else	// if error already handled 
						mPendingAutoRefreshClientError = null;
				}
		}
		
		public void notifyPollError(int errorNum, int operation, String errorMessage, String param) {
			if (operation == PollOp.POLL_CREATE_ACCOUNT ||
					operation == PollOp.POLL_LOOKUP_ACCOUNT ||
					operation == PollOp.POLL_PROJECT_ATTACH) {
				// addProject operation
				mJoinedAddProjectsMap.remove(param);
			}
			
			boolean called = false;
			
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientPollErrorReceiver) {
					if (((ClientPollErrorReceiver)observer).onPollError(errorNum, operation,
							errorMessage, param))
						called = true;
				}
			}
			
			String key = (param != null) ? param : "";
			
			if (operation != PollOp.POLL_PROJECT_CONFIG) {
				// error from other operation
				synchronized(mPollErrorsMap) {
					if (Logging.DEBUG) Log.d(TAG, "Is PendingPollErrorError is set: "+
							(!called)+" for "+key);
					if (!called) { /* set up pending if not already handled */
						PollError pollError = new PollError(errorNum, operation, errorMessage, param);
						mPollErrorsMap.put(key, pollError);
					} else { // if already handled (remove old errors)
						mPollErrorsMap.remove(key);
					}
				}
			} else {
				synchronized(mProjectConfigErrorsMap) {
					if (Logging.DEBUG) Log.d(TAG, "Is PendingProjectConfigErrorError is set: "+
							(!called)+" for "+key);
					if (!called) { /* set up pending if not already handled */
						PollError pollError = new PollError(errorNum, operation, errorMessage, param);
						mProjectConfigErrorsMap.put(key, pollError);
					} else { // if already handled (remove old errors)
						mProjectConfigErrorsMap.remove(key);
					}
				}
			}
		}
		
		public void notifyConnected(VersionInfo clientVersion) {
			mConnected = true;
			mRemoteClientVersion = clientVersion;
			
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers)
				observer.clientConnected(mRemoteClientVersion);
		}

		public void notifyDisconnected() {
			mConnected = false;
			mRemoteClientVersion = null;
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				observer.clientDisconnected();
				if (Logging.DEBUG) Log.d(TAG, "Detached observer: " + observer.toString()); // see below clearing of all observers
			}
			// before clearing observers inform is client connection finish work
			onChangeIsWorking(false);
			mObservers.clear();
		}


		public void updatedClientMode(final ModeInfo modeInfo) {
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientManageReceiver) {
					ClientManageReceiver callback = (ClientManageReceiver)observer;
					
					boolean periodicAllowed = callback.updatedClientMode(modeInfo);
					if (periodicAllowed)
						mAutoRefresh.scheduleAutomaticRefresh(callback, AutoRefresh.CLIENT_MODE, -1);
				}
			}
		}

		public void updatedHostInfo(final HostInfo hostInfo) {
			synchronized(mPendingHostInfoSync) {
				mPendingHostInfo = hostInfo;
			}
			
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientManageReceiver)
					((ClientManageReceiver)observer).updatedHostInfo(hostInfo);
			}
		}
		
		public void currentBAMInfo(final AccountMgrInfo bamInfo) {
			synchronized(mPendingAccountMgrInfoSync) {
				mPendingAccountMgrInfo = bamInfo;
			}
			
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientAccountMgrReceiver)
					((ClientAccountMgrReceiver)observer).currentBAMInfo(bamInfo);
			}
		}
		
		public void currentAllProjectsList(final ArrayList<ProjectListEntry> projects) {
			synchronized(mPendingAllProjectsListSync) {
				mPendingAllProjectsList = projects;
			}
			
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientAllProjectsListReceiver)
					((ClientAllProjectsListReceiver)observer).currentAllProjectsList(projects);
			}
		}
		
		public void currentAuthCode(final String projectUrl, final String authCode) {
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientProjectReceiver)
					((ClientProjectReceiver)observer).currentAuthCode(projectUrl, authCode);
			}
			
			if (mJoinedAddProjectsMap.containsKey(projectUrl)) {
				projectAttach(projectUrl, authCode, "");
			}
		}
		
		public void currentProjectConfig(final ProjectConfig projectConfig) {
			mPendingProjectConfigs.put(projectConfig.master_url, projectConfig);
			
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientProjectReceiver)
					((ClientProjectReceiver)observer).currentProjectConfig(projectConfig);
			}
		}
		
		public void currentGlobalPreferences(final GlobalPreferences globalPrefs) {
			synchronized(mPendingGlobalPrefsSync) {
				mPendingGlobalPrefs = globalPrefs;
			}
			
			// First, check whether callback is still present in observers
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientPreferencesReceiver)
					((ClientPreferencesReceiver)observer).currentGlobalPreferences(globalPrefs);
			}
		}
		
		public void afterAccountMgrRPC() {
			// First, check whether callback is still present in observers
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			mBAMBeingSynchronized = false;
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientAccountMgrReceiver)
					((ClientAccountMgrReceiver)observer).onAfterAccountMgrRPC();
			}
		}
		
		public void afterProjectAttach(final String projectUrl) {
			// First, check whether callback is still present in observers
			if (Logging.DEBUG) Log.d(TAG, "Remove after project attach:"+projectUrl);
			mJoinedAddProjectsMap.remove(projectUrl);
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientProjectReceiver)
					((ClientProjectReceiver)observer).onAfterProjectAttach(projectUrl);
			}
		}
		
		public void onGlobalPreferencesChanged() {
			mGlobalPrefsBeingOverriden = false; // if finished
			
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientPreferencesReceiver)
					((ClientPreferencesReceiver)observer).onGlobalPreferencesChanged();
			}
		}

		public void updatedProjects(final ArrayList <ProjectInfo> projects) {
			synchronized(mPendingProjectsSync) {
				mPendingProjects = projects;
			}
			
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientUpdateProjectsReceiver) {
					ClientUpdateProjectsReceiver callback = (ClientUpdateProjectsReceiver)observer;
					
					boolean periodicAllowed = callback.updatedProjects(projects);
					if (periodicAllowed)
						mAutoRefresh.scheduleAutomaticRefresh(callback, AutoRefresh.PROJECTS, -1);
				}
			}
		}

		public void updatedTasks(final ArrayList <TaskInfo> tasks) {
			synchronized(mPendingTasksSync) {
				mPendingTasks = tasks;
			}
			
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientUpdateTasksReceiver) {
					ClientUpdateTasksReceiver callback = (ClientUpdateTasksReceiver)observer;
					
					boolean periodicAllowed = callback.updatedTasks(tasks);
					if (periodicAllowed)
						mAutoRefresh.scheduleAutomaticRefresh(callback, AutoRefresh.TASKS, -1);
				}
			}
		}

		public void updatedTransfers(final ArrayList <TransferInfo> transfers) {
			synchronized(mPendingTransfersSync) {
				mPendingTransfers = transfers;
			}
			
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientUpdateTransfersReceiver) {
					ClientUpdateTransfersReceiver callback = (ClientUpdateTransfersReceiver)observer;
					
					boolean periodicAllowed = callback.updatedTransfers(transfers);
					if (periodicAllowed)
						mAutoRefresh.scheduleAutomaticRefresh(callback, AutoRefresh.TRANSFERS, -1);
				}
			}
		}

		public void updatedMessages(final ArrayList <MessageInfo> messages) {
			synchronized(mPendingMessagesSync) {
				mPendingMessages = messages;
			}
			
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientUpdateMessagesReceiver) {
					ClientUpdateMessagesReceiver callback = (ClientUpdateMessagesReceiver)observer;
					
					boolean periodicAllowed = callback.updatedMessages(messages);
					if (periodicAllowed)
						mAutoRefresh.scheduleAutomaticRefresh(callback, AutoRefresh.MESSAGES, -1);
				}
			}
		}
		
		public void updatedNotices(final ArrayList <NoticeInfo> notices) {
			synchronized(mPendingNoticesSync) {
				mPendingNotices = notices;
			}
			
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers) {
				if (observer instanceof ClientUpdateNoticesReceiver) {
					ClientUpdateNoticesReceiver callback = (ClientUpdateNoticesReceiver)observer;
					
					boolean periodicAllowed = callback.updatedNotices(notices);
					if (periodicAllowed)
						mAutoRefresh.scheduleAutomaticRefresh(callback, AutoRefresh.NOTICES, -1);
				}
			}
		}
		
		public void onChangeIsWorking(boolean isWorking) {
			ClientReceiver[] observers = mObservers.toArray(new ClientReceiver[0]);
			for (ClientReceiver observer: observers)
				observer.onClientIsWorking(isWorking);
		}
		
		/*
		 * cancel poll operations
		 */
		public void cancelPollOperations() {
			Log.d(TAG, "on cancel poll operations");
			synchronized(mPendingAccountMgrInfoSync) {
				mPendingAccountMgrInfo = null;
			}
			// clear 'add project' operations
			mJoinedAddProjectsMap.clear();
			// clear project config temp results
			mPendingProjectConfigs.clear();
			mBAMBeingSynchronized = false;
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
	
	public int getAutoRefresh() {
		if (mAutoRefresh != null)
			return mAutoRefresh.getAutoRefresh();
		return -1;
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
			if (observer instanceof ClientManageReceiver)
				mAutoRefresh.unscheduleAutomaticRefresh((ClientManageReceiver)observer);
		}
		if (Logging.DEBUG) Log.d(TAG, "Detached observer: " + observer.toString());
	}

	public boolean isWorking() {
		return mWorker.isWorking();
	}
	
	@Override
	public void connect(final ClientId remoteClient, final boolean retrieveInitialData) {
		if (mRemoteClient != null) {
			// already connected
			if (Logging.ERROR) Log.e(TAG, "Request to connect to: " + remoteClient.getNickname() + " while already connected to: " + mRemoteClient.getNickname());
			return;
		}
		mRemoteClient = remoteClient;
		mWorker.connect(remoteClient, retrieveInitialData);
	}
	
	@Override
	public void disconnect() {
		if (mRemoteClient == null) return; // not connected
		mWorker.disconnect();
		mRemoteClient = null; // This will prevent further triggers towards worker thread
	}

	@Override
	public final ClientId getClientId() {
		return mRemoteClient;
	}
	
	@Override
	public boolean isNativeConnected() {
		if (mRemoteClient == null) return false;
		return mRemoteClient.isNativeClient();
	}

	@Override
	public void updateClientMode() {
		if (mRemoteClient == null) return; // not connected
		clearPendingAutoRefreshClientError();
		mWorker.updateClientMode();
	}

	@Override
	public void updateHostInfo() {
		if (mRemoteClient == null) return; // not connected
		synchronized(mPendingHostInfoSync) {
			mPendingHostInfo = null;
		}
		clearPendingClientError();
		mWorker.updateHostInfo();
	}
	
	@Override
	public HostInfo getPendingHostInfo() {
		if (mRemoteClient == null) return null; // not connected
		synchronized(mPendingHostInfoSync) {
			HostInfo pending = mPendingHostInfo;
			mPendingHostInfo = null;
			return pending;
		}
	}

	@Override
	public void updateProjects() {
		if (mRemoteClient == null) return; // not connected
		synchronized(mPendingProjectsSync) {
			mPendingProjects = null;
		}
		clearPendingAutoRefreshClientError();
		mWorker.updateProjects();
	}
	
	@Override
	public ArrayList<ProjectInfo> getPendingProjects() {
		if (mRemoteClient == null) return null; // not connected
		synchronized(mPendingProjectsSync) {
			ArrayList<ProjectInfo> pending = mPendingProjects;
			mPendingProjects = null;
			return pending;
		}
	}

	@Override
	public void updateTasks() {
		if (mRemoteClient == null) return; // not connected
		synchronized(mPendingTasksSync) {
			mPendingTasks = null;
		}
		clearPendingAutoRefreshClientError();
		mWorker.updateTasks();
	}
	
	@Override
	public ArrayList<TaskInfo> getPendingTasks() {
		if (mRemoteClient == null) return null; // not connected
		synchronized(mPendingTasksSync) {
			ArrayList<TaskInfo> pending = mPendingTasks;
			mPendingTasks = null;
			return pending;
		}
	}

	@Override
	public void updateTransfers() {
		if (mRemoteClient == null) return; // not connected
		synchronized(mPendingTransfersSync) {
			mPendingTransfers = null;
		}
		clearPendingAutoRefreshClientError();
		mWorker.updateTransfers();
	}
	
	@Override
	public ArrayList<TransferInfo> getPendingTransfers() {
		if (mRemoteClient == null) return null; // not connected
		synchronized(mPendingTransfersSync) {
			ArrayList<TransferInfo> pending = mPendingTransfers;
			mPendingTransfers = null;
			return pending;
		}
	}

	@Override
	public void updateMessages() {
		if (mRemoteClient == null) return; // not connected
		synchronized(mPendingMessagesSync) {
			mPendingMessages = null;
		}
		clearPendingAutoRefreshClientError();
		mWorker.updateMessages();
	}
	
	@Override
	public ArrayList<MessageInfo> getPendingMessages() {
		if (mRemoteClient == null) return null; // not connected
		synchronized(mPendingMessagesSync) {
			ArrayList<MessageInfo> pending = mPendingMessages;
			mPendingMessages = null;
			return pending;
		}
	}
	
	public void updateNotices() {
		if (mRemoteClient == null) return; // not connected
		synchronized(mPendingMessagesSync) {
			mPendingMessages = null;
		}
		clearPendingAutoRefreshClientError();
		mWorker.updateNotices();
	}
	
	@Override
	public ArrayList<NoticeInfo> getPendingNotices() {
		if (mRemoteClient == null) return null; // not connected
		synchronized(mPendingNoticesSync) {
			ArrayList<NoticeInfo> pending = mPendingNotices;
			mPendingNotices = null;
			return pending;
		}
	}
	
	@Override
	public void addToScheduledUpdates(ClientReceiver callback, int refreshType, int period) {
		if (mRemoteClient == null) return; // not connected
		mAutoRefresh.scheduleAutomaticRefresh(callback, refreshType, period);
	}

	@Override
	public void cancelScheduledUpdates(int refreshType) {
		if (mRemoteClient == null) return; // not connected
		// Cancel pending updates in worker thread
		mWorker.cancelPendingUpdates(refreshType);
		// Remove scheduled auto-refresh (if any)
		if (mAutoRefresh!=null)
			mAutoRefresh.unscheduleAutomaticRefresh(refreshType);
	}

	@Override
	public void getBAMInfo() {
		if (mRemoteClient == null) return; // not connected
		synchronized(mPendingAccountMgrInfoSync) {
			mPendingAccountMgrInfo = null;
		}
		clearPendingClientError();
		mWorker.getBAMInfo();
	}

	@Override
	public AccountMgrInfo getPendingBAMInfo() {
		if (mRemoteClient == null) return null; // not connected
		synchronized(mPendingAccountMgrInfoSync) {
			AccountMgrInfo toReturn = mPendingAccountMgrInfo;
			mPendingAccountMgrInfo = null;
			return toReturn;
		}
	}
	
	@Override
	public void attachToBAM(String name, String url, String password) {
		if (mRemoteClient == null) return; // not connected
		clearPendingPollError("");
		clearPendingClientError();
		mBAMBeingSynchronized = true;
		mWorker.attachToBAM(name, url, password);
	}
	
	@Override
	public void synchronizeWithBAM() {
		if (mRemoteClient == null) return; // not connected
		mBAMBeingSynchronized = true;
		clearPendingPollError("");
		clearPendingClientError();
		mWorker.synchronizeWithBAM();
	}
	
	@Override
	public void stopUsingBAM() {
		if (mRemoteClient == null) return; // not connected
		clearPendingClientError();
		mWorker.stopUsingBAM();
	}
	
	@Override
	public boolean isBAMBeingSynchronized() {
		if (mRemoteClient == null) return false;
		return mBAMBeingSynchronized;
	}
	
	@Override
	public void getAllProjectsList() {
		if (mRemoteClient == null) return; // not connected
		synchronized (mPendingAllProjectsListSync) {
			mPendingAllProjectsList = null;
		}
		clearPendingClientError();
		mWorker.getAllProjectsList();
	}
	
	@Override
	public ArrayList<ProjectListEntry> getPendingAllProjectsList() {
		if (mRemoteClient == null) return null; // not connected
		synchronized(mPendingAllProjectsListSync) {
			ArrayList<ProjectListEntry> pending = mPendingAllProjectsList;
			mPendingAllProjectsList = null;
			return pending;
		}
	}
	
	@Override
	public void lookupAccount(AccountIn accountIn) {
		if (mRemoteClient == null) return; // not connected
		clearPendingPollError(accountIn.url);
		clearPendingClientError();
		mWorker.lookupAccount(accountIn);
	}
	
	@Override
	public void createAccount(AccountIn accountIn) {
		if (mRemoteClient == null) return; // not connected
		clearPendingPollError(accountIn.url);
		clearPendingClientError();
		mWorker.createAccount(accountIn);
	}
	
	@Override
	public void projectAttach(String url, String authCode, String projectName) {
		if (mRemoteClient == null) return; // not connected
		clearPendingPollError(url);
		clearPendingClientError();
		mWorker.projectAttach(url, authCode, projectName);
	}
	
	@Override
	public void addProject(AccountIn accountIn, boolean create) {
		if (mRemoteClient == null) return; // not connected
		if (mJoinedAddProjectsMap.putIfAbsent(accountIn.url, "") != null)
			return;
		
		if (create)
			mWorker.createAccount(accountIn);
		else
			mWorker.lookupAccount(accountIn);
	}
	
	@Override
	public boolean isProjectBeingAdded(String projectUrl) {
		if (mRemoteClient == null)
			return false;
		return mJoinedAddProjectsMap.containsKey(projectUrl);
	}
	
	@Override
	public void getProjectConfig(String url) {
		if (mRemoteClient == null) return; // not connected
		mPendingProjectConfigs.remove(url);
		clearPendingProjectConfigError(url);
		clearPendingClientError();
		mWorker.getProjectConfig(url);
	}
	
	@Override
	public ProjectConfig getPendingProjectConfig(String projectUrl) {
		return mPendingProjectConfigs.remove(projectUrl);
	}
	
	@Override
	public boolean handlePendingClientErrors(ClientReceiver receiver) {
		if (mRemoteClient == null)
			return false;
		
		boolean handled = false;
		synchronized(mPendingClientErrorSync) {
			ClientError clientError = mPendingClientError;
			
			if (clientError == null)
				return false;
			
			if (clientError != null && receiver.clientError(clientError.errorNum, clientError.message)) {
				if (Logging.DEBUG) Log.d(TAG, "PendingClientError handled");
				mPendingClientError = null;
				handled = true;
			}
		}
		
		synchronized(mPendingAutoRefreshClientErrorSync) {
			ClientError clientError = mPendingAutoRefreshClientError;
			
			if (clientError != null && receiver.clientError(clientError.errorNum, clientError.message)) {
				if (Logging.DEBUG) Log.d(TAG, "PendingAutoRefreshClientError handled");
				mPendingAutoRefreshClientError = null;
				handled = true;
			}
		}
		
		return handled;
	}
	
	private void clearPendingClientError() {
		synchronized (mPendingClientErrorSync) {
			mPendingClientError = null;
		}
	}
	
	private void clearPendingAutoRefreshClientError() {
		synchronized (mPendingAutoRefreshClientErrorSync) {
			mPendingAutoRefreshClientError = null;
		}
	}
	
	@Override
	public void cancelPollOperations(int opFlags) {
		if (mRemoteClient == null)
			return;
		mWorker.cancelPollOperations(opFlags);
	}
	
	@Override
	public boolean handlePendingPollErrors(ClientPollErrorReceiver receiver, String projectUrl) {
		if (mRemoteClient == null)
			return false;
		
		boolean handled = false;
		
		synchronized(mPollErrorsMap) {
			String key = (projectUrl != null) ? projectUrl : "";
			PollError pollError = mPollErrorsMap.get(key);
			
			if (pollError != null) {
				if (receiver.onPollError(pollError.errorNum, pollError.operation, pollError.message,
						pollError.param)) {
					if (Logging.DEBUG) Log.d(TAG, "PendingPollError handled for "+key);
					mPollErrorsMap.remove(key);
					handled = true;
				}
			}
		}
		
		synchronized(mProjectConfigErrorsMap) {
			String key = (projectUrl != null) ? projectUrl : "";
			PollError pollError = mProjectConfigErrorsMap.get(key);
			
			if (pollError != null) {
				if (receiver.onPollError(pollError.errorNum, pollError.operation, pollError.message,
						pollError.param)) {
					if (Logging.DEBUG) Log.d(TAG, "PendingProjectConfigError handled for "+key);
					mProjectConfigErrorsMap.remove(key);
					handled = true;
				}
			}
		}
		return handled;
	}
	
	private void clearPendingPollError(String projectUrl) {
		synchronized(mPollErrorsMap) {
			mPollErrorsMap.remove(projectUrl);
		}
	}
	
	private void clearPendingProjectConfigError(String projectUrl) {
		synchronized(mProjectConfigErrorsMap) {
			mProjectConfigErrorsMap.remove(projectUrl);
		}
	}
	
	@Override
	public void getGlobalPrefsWorking() {
		if (mRemoteClient == null) return; // not connected
		synchronized(mPendingGlobalPrefsSync) {
			mPendingGlobalPrefs = null;
		}
		clearPendingClientError();
		mWorker.getGlobalPrefsWorking();
	}
	
	@Override
	public GlobalPreferences getPendingGlobalPrefsWorking() {
		if (mRemoteClient == null) return null; // not connected
		synchronized(mPendingGlobalPrefsSync) {
			GlobalPreferences pending = mPendingGlobalPrefs;
			mPendingGlobalPrefs = null;
			return pending;
		}
	}
	
	@Override
	public void setGlobalPrefsOverride(String globalPrefs) {
		if (mRemoteClient == null) return; // not connected
		mGlobalPrefsBeingOverriden = true;
		clearPendingClientError();
		mWorker.setGlobalPrefsOverride(globalPrefs);
	}
	
	@Override
	public void setGlobalPrefsOverrideStruct(GlobalPreferences globalPrefs) {
		if (mRemoteClient == null) return; // not connected
		mGlobalPrefsBeingOverriden = true;
		clearPendingClientError();
		mWorker.setGlobalPrefsOverrideStruct(globalPrefs, mRemoteClient.isNativeClient());
	}
	
	@Override
	public boolean isGlobalPrefsBeingOverriden() {
		if (mRemoteClient == null) return false;
		return mGlobalPrefsBeingOverriden;
	}
	
	@Override
	public void runBenchmarks() {
		if (mRemoteClient == null) return; // not connected
		clearPendingClientError();
		mWorker.runBenchmarks();
	}

	@Override
	public void setRunMode(final int mode) {
		if (mRemoteClient == null) return; // not connected
		clearPendingClientError();
		mWorker.setRunMode(mode);
	}

	@Override
	public void setNetworkMode(final int mode) {
		if (mRemoteClient == null) return; // not connected
		clearPendingClientError();
		mWorker.setNetworkMode(mode);
	}

	@Override
	public void shutdownCore() {
		if (mRemoteClient == null) return; // not connected
		clearPendingClientError();
		mWorker.shutdownCore();
	}

	@Override
	public void doNetworkCommunication() {
		if (mRemoteClient == null) return; // not connected
		clearPendingClientError();
		mWorker.doNetworkCommunication();
	}

	@Override
	public void projectOperation(final int operation, final String projectUrl) {
		if (mRemoteClient == null) return; // not connected
		clearPendingClientError();
		mWorker.projectOperation(operation, projectUrl);
	}
	
	@Override
	public void projectsOperation(final int operation, final String[] projectUrls) {
		if (mRemoteClient == null) return; // not connected
		clearPendingClientError();
		mWorker.projectsOperation(operation, projectUrls);
	}

	@Override
	public void taskOperation(final int operation, final String projectUrl, final String taskName) {
		if (mRemoteClient == null) return; // not connected
		clearPendingClientError();
		mWorker.taskOperation(operation, projectUrl, taskName);
	}
	
	@Override
	public void tasksOperation(final int operation, final TaskDescriptor[] tasks) {
		if (mRemoteClient == null) return; // not connected
		clearPendingClientError();
		mWorker.tasksOperation(operation, tasks);
	}

	@Override
	public void transferOperation(final int operation, final String projectUrl, final String fileName) {
		if (mRemoteClient == null) return; // not connected
		clearPendingClientError();
		mWorker.transferOperation(operation, projectUrl, fileName);
	}
	
	@Override
	public void transfersOperation(final int operation, final TransferDescriptor[] transfers) {
		if (mRemoteClient == null) return; // not connected
		clearPendingClientError();
		mWorker.transfersOperation(operation, transfers);
	}
}
