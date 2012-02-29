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

package sk.boinc.nativeboinc.service;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import edu.berkeley.boinc.lite.AccountIn;
import edu.berkeley.boinc.lite.AccountMgrInfo;
import edu.berkeley.boinc.lite.GlobalPreferences;
import edu.berkeley.boinc.lite.ProjectConfig;
import edu.berkeley.boinc.lite.ProjectListEntry;

import sk.boinc.nativeboinc.bridge.ClientBridge;
import sk.boinc.nativeboinc.bridge.ClientBridgeCallback;
import sk.boinc.nativeboinc.clientconnection.ClientError;
import sk.boinc.nativeboinc.clientconnection.ClientReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientReplyReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientRequestHandler;
import sk.boinc.nativeboinc.clientconnection.HostInfo;
import sk.boinc.nativeboinc.clientconnection.MessageInfo;
import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.clientconnection.PollError;
import sk.boinc.nativeboinc.clientconnection.ProjectInfo;
import sk.boinc.nativeboinc.clientconnection.TaskDescriptor;
import sk.boinc.nativeboinc.clientconnection.TaskInfo;
import sk.boinc.nativeboinc.clientconnection.TransferDescriptor;
import sk.boinc.nativeboinc.clientconnection.TransferInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.ClientId;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;


public class ConnectionManagerService extends Service implements
		ClientRequestHandler, ClientBridgeCallback, ConnectivityListener {
	private static final String TAG = "ConnectionManagerService";

	private static final int TERMINATE_GRACE_PERIOD_CONN = 45;
	private static final int TERMINATE_GRACE_PERIOD_IDLE = 3;

	public class LocalBinder extends Binder {
		public ConnectionManagerService getService() {
			return ConnectionManagerService.this;
		}
	}

	private final IBinder mBinder = new LocalBinder();
	private final Handler mHandler = new Handler();
	private Runnable mTerminateRunnable = null;

	private ConnectivityStatus mConnectivityStatus = null;
	private ClientBridge mClientBridge = null;
	private Set<ClientBridge> mDyingBridges = new HashSet<ClientBridge>();
	private Set<ClientReceiver> mObservers = new HashSet<ClientReceiver>();
	private NetworkStatisticsHandler mNetStats = null;


	@Override
	public IBinder onBind(Intent intent) {
		if (Logging.DEBUG) Log.d(TAG, "onBind()");
		// Just make sure the service is running:
		startService(new Intent(this, ConnectionManagerService.class));
		return mBinder;
	}

	@Override
	public void onRebind(Intent intent) {
		if (mTerminateRunnable != null) {
			// There is Runnable to stop the service
			// We cancel that request now
			mHandler.removeCallbacks(mTerminateRunnable);
			mTerminateRunnable = null;
			if (Logging.DEBUG) Log.d(TAG, "onRebind() - canceled stopping of the service");
		}
		else {
			// This is not expected
			if (Logging.ERROR) Log.e(TAG, "onRebind() - mTerminateRunnable empty");
			// We just make sure the service is running
			// If service is still running, it's kept running anyway
			startService(new Intent(this, ConnectionManagerService.class));
		}
	}

	@Override
	public boolean onUnbind(Intent intent) {
		// The observers should be empty at this time (all are unbound from us)
		if (!mObservers.isEmpty()) {
			if (Logging.WARNING) Log.w(TAG, "onUnbind(), but mObservers is not empty");
			mObservers.clear();
		}
		// Create runnable which will stop service after grace period
		mTerminateRunnable = new Runnable() {
			@Override
			public void run() {
				// We remove reference to self
				mTerminateRunnable = null;
				// Disconnect client if still connected
				if (mClientBridge != null) {
					// Still connected to some client - disconnect it now
					disconnect();
				}
				// Stop service
				stopSelf();
				if (Logging.DEBUG) Log.d(TAG, "Stopped service");
			}
		};
		// Post the runnable to self - delayed by grace period
		long gracePeriod;
		if (mClientBridge != null) {
			gracePeriod = TERMINATE_GRACE_PERIOD_CONN * 1000;
		}
		else {
			gracePeriod = TERMINATE_GRACE_PERIOD_IDLE * 1000;
		}
		mHandler.postDelayed(mTerminateRunnable, gracePeriod);
		if (Logging.DEBUG) Log.d(TAG, "onUnbind() - Started grace period to terminate self");
		return true;
	}

	@Override
	public void onCreate() {
		if (Logging.DEBUG) Log.d(TAG, "onCreate()");
		// Add connectivity monitoring (to be notified when connection is down)
		mConnectivityStatus = new ConnectivityStatus(this, this);
		// Create network statistics handler
		mNetStats = new NetworkStatisticsHandler(this);
	}

	@Override
	public void onDestroy() {
		if (Logging.DEBUG) Log.d(TAG, "onDestroy()");
		// Clean-up connectivity monitoring
		mConnectivityStatus.cleanup();
		mConnectivityStatus = null;
		// Clean-up network statistics handler
		mNetStats.cleanup();
		mNetStats = null;
	}

	@Override
	public void onConnectivityLost() {
		if (mClientBridge != null) {
			// We are interested in this event only when connected
			if (Logging.DEBUG) Log.d(TAG, "onConnectivityLost() while connected to host " +
			mClientBridge.getClientId().getNickname());
			// TODO Handle connectivity loss
		}
	}

	@Override
	public void onConnectivityRestored(int connectivityType) {
		if (mClientBridge != null) {
			// We are interested in this event only when connected
			if (Logging.DEBUG) Log.d(TAG, "onConnectivityRestored() while connected to host " +
			mClientBridge.getClientId().getNickname() + ", connectivity type: " + connectivityType);
			// TODO Handle connectivity restoration
		}
	}

	@Override
	public void onConnectivityChangedType(int connectivityType) {
		if (mClientBridge != null) {
			// We are interested in this event only when connected
			if (Logging.DEBUG) Log.d(TAG, "onConnectivityChangedType() while connected to host " + 
			mClientBridge.getClientId().getNickname() + ", new connectivity type: " + connectivityType);
			// TODO Handle connectivity type change
		}
	}

	@Override
	public void bridgeDisconnected(ClientBridge clientBridge) {
		// The bridge got disconnected (explicit disconnect, connect unsuccessful, abnormal close etc)
		if (mClientBridge == clientBridge) {
			// The currently connected bridge was disconnected.
			// This is unsolicited disconnect
			if (Logging.INFO) Log.i(TAG, "Unsolicited disconnect of ClientBridge");
			mClientBridge = null;
		}
		else {
			if (mDyingBridges.contains(clientBridge)) {
				// This is the bridge disconnected by us
				if (Logging.DEBUG) Log.d(TAG, "ClientBridge finished disconnect");
				mDyingBridges.remove(clientBridge);
			}
			else {
				// This is not expected
				if (Logging.WARNING) Log.w(TAG, "Unknown ClientBridge disconnected");
			}
		}
	}

	/**
	 * Locally stores reference to the service user.
	 * <p>
	 * If bridge is connected, the reference is passed to bridge as well.
	 * <br>
	 * If bridge is not connected, the stored reference will be used later at connection time
	 * and it will be passed to bridge then.
	 * @param observer - user of the service
	 */
	@Override
	public void registerStatusObserver(ClientReceiver observer) {
		mObservers.add(observer);
		if (mClientBridge != null) {
			mClientBridge.registerStatusObserver(observer);
		}
		if (Logging.DEBUG) Log.d(TAG, "Attached new observer: " + observer.toString());
	}

	/**
	 * Deletes locally stored reference to service user.
	 * <p>
	 * If bridge is connected, the reference is also passed to bridge, so it can be deleted there as well.
	 * @param observer - user of the service
	 */
	@Override
	public void unregisterStatusObserver(ClientReceiver observer) {
		mObservers.remove(observer);
		if (mClientBridge != null) {
			mClientBridge.unregisterStatusObserver(observer);
		}
		if (Logging.DEBUG) Log.d(TAG, "Detached observer: " + observer.toString());
	}

	private boolean mDisconnectedByManager = false;
	
	@Override
	public void connect(ClientId host, boolean retrieveInitialData) throws NoConnectivityException {
		if (Logging.DEBUG) Log.d(TAG, "connect() to host " + host.getNickname());
		
		mDisconnectedByManager = false;
		
		if (mClientBridge != null) {
			// Connected to some client - disconnect it first
			disconnect();
		}
		//if (mConnectivityStatus.isConnected()) {
		// Create new bridge first
		mClientBridge = new ClientBridge(this, mNetStats);
		// Propagate all current observers to bridge, so they will be informed about status
		Iterator<ClientReceiver> it = mObservers.iterator();
		while (it.hasNext()) {
			ClientReceiver observer = it.next();
			mClientBridge.registerStatusObserver(observer);
		}
		// Finally, initiate connection to remote client
		mClientBridge.connect(host, retrieveInitialData);
	}
	
	public boolean isDisconnectedByManager() {
		return mDisconnectedByManager;
	}

	@Override
	public void disconnect() {
		mDisconnectedByManager = true;
		if (mClientBridge != null) {
			if (Logging.DEBUG) Log.d(TAG, "disconnect() - started towards " + mClientBridge.getClientId().getNickname());
			mDyingBridges.add(mClientBridge);
			mClientBridge.disconnect();
			mClientBridge = null;
		}
		else {
			if (Logging.DEBUG) Log.d(TAG, "disconnect() - not connected already ");
		}
	}

	@Override
	public ClientId getClientId() {
		if (mClientBridge != null) {
			return mClientBridge.getClientId();
		}
		else {
			return null;
		}
	}
	
	@Override
	public boolean isWorking() {
		if (mClientBridge != null) {
			return mClientBridge.isWorking();
		}
		else return false;
	}
	
	@Override
	public boolean isNativeConnected() {
		if (mClientBridge != null) {
			return mClientBridge.isNativeConnected();
		}
		else return false;
	}

	@Override
	public void updateClientMode() {
		if (mClientBridge != null) {
			mClientBridge.updateClientMode();
		}
	}

	@Override
	public void updateHostInfo() {
		if (mClientBridge != null) {
			mClientBridge.updateHostInfo();
		}
	}
	
	@Override
	public HostInfo getPendingHostInfo() {
		if (mClientBridge != null)
			return mClientBridge.getPendingHostInfo();
		return null;
	}

	@Override
	public void updateProjects() {
		if (mClientBridge != null) {
			mClientBridge.updateProjects();
		}
	}
	
	@Override
	public ArrayList<ProjectInfo> getPendingProjects() {
		if (mClientBridge != null)
			return mClientBridge.getPendingProjects();
		return null;
	}

	@Override
	public void updateTasks() {
		if (mClientBridge != null) {
			mClientBridge.updateTasks();
		}
	}
	
	@Override
	public ArrayList<TaskInfo> getPendingTasks() {
		if (mClientBridge != null)
			return mClientBridge.getPendingTasks();
		return null;
	}

	@Override
	public void updateTransfers() {
		if (mClientBridge != null) {
			mClientBridge.updateTransfers();
		}
	}

	@Override
	public ArrayList<TransferInfo> getPendingTransfers() {
		if (mClientBridge != null)
			return mClientBridge.getPendingTransfers();
		return null;
	}
	
	@Override
	public void updateMessages() {
		if (mClientBridge != null) {
			mClientBridge.updateMessages();
		}
	}
	
	@Override
	public ArrayList<MessageInfo> getPendingMessages() {
		if (mClientBridge != null)
			return mClientBridge.getPendingMessages();
		return null;
	}
	
	@Override
	public void addToScheduledUpdates(ClientReplyReceiver callback, int refreshType) {
		if (mClientBridge != null)
			mClientBridge.addToScheduledUpdates(callback, refreshType);
	}
	
	@Override
	public void cancelScheduledUpdates(int refreshType) {
		if (mClientBridge != null) {
			mClientBridge.cancelScheduledUpdates(refreshType);
		}
	}

	@Override
	public void getBAMInfo() {
		if (mClientBridge != null) {
			mClientBridge.getBAMInfo();
		}
	}
	
	@Override
	public AccountMgrInfo getPendingBAMInfo() {
		if (mClientBridge != null)
			return mClientBridge.getPendingBAMInfo();
		return null;
	}
	
	@Override
	public void attachToBAM(String name, String url, String password) {
		if (mClientBridge != null) {
			mClientBridge.attachToBAM(name, url, password);
		}
	}
	
	@Override
	public void synchronizeWithBAM() {
		if (mClientBridge != null) {
			mClientBridge.synchronizeWithBAM();
		}
	}
	
	@Override
	public void stopUsingBAM() {
		if (mClientBridge != null) {
			mClientBridge.stopUsingBAM();
		}
	}
	
	@Override
	public boolean isBAMBeingSynchronized() {
		if (mClientBridge != null)
			return mClientBridge.isBAMBeingSynchronized();
		return false;
	}
	
	@Override
	public void getAllProjectsList() {
		if (mClientBridge != null) {
			mClientBridge.getAllProjectsList();
		}
	}
	
	@Override
	public ArrayList<ProjectListEntry> getPendingAllProjectsList() {
		if (mClientBridge != null)
			return mClientBridge.getPendingAllProjectsList();
		return null;
	}
	
	@Override
	public void lookupAccount(AccountIn accountIn) {
		if (mClientBridge != null) {
			mClientBridge.lookupAccount(accountIn);
		}
	}
	
	@Override
	public void createAccount(AccountIn accountIn) {
		if (mClientBridge != null) {
			mClientBridge.createAccount(accountIn);
		}
	}
	
	@Override
	public void projectAttach(String url, String authCode, String projectName) {
		if (mClientBridge != null) {
			mClientBridge.projectAttach(url, authCode, projectName);
		}
	}
	
	@Override
	public boolean isProjectBeingAdded(String projectUrl) {
		if (mClientBridge != null)
			return mClientBridge.isProjectBeingAdded(projectUrl);
		return false;
	}
	
	/* join of createAccount/lookupAccount with projectAttach */
	@Override
	public void addProject(AccountIn accountIn, boolean create) {
		if (mClientBridge != null)
			mClientBridge.addProject(accountIn, create);
	}
	
	@Override
	public void getProjectConfig(String url) {
		if (mClientBridge != null) {
			mClientBridge.getProjectConfig(url);
		}
	}
	
	/**
	 * returns pending project config after calling.
	 * @param projectUrl
	 * @return pending projectConfig
	 */
	@Override
	public ProjectConfig getPendingProjectConfig(String projectUrl) {
		if (mClientBridge != null)
			return mClientBridge.getPendingProjectConfig(projectUrl);
		return null;
	}
	
	/*
	 * cancels all poll operations
	 */
	@Override
	public void cancelPollOperations() {
		if (mClientBridge != null)
			mClientBridge.cancelPollOperations();
	}
	
	@Override
	public ClientError getPendingClientError() {
		if (mClientBridge != null) {
			return mClientBridge.getPendingClientError();
		}
		return null;
	}
	
	/**
	 * pending poll errors
	 */
	@Override
	public PollError getPendingPollError(String projectUrl) {
		if (mClientBridge != null) {
			return mClientBridge.getPendingPollError(projectUrl);
		}
		return null;
	}
	
	@Override
	public void getGlobalPrefsWorking() {
		if (mClientBridge != null) {
			mClientBridge.getGlobalPrefsWorking();
		}
	}
	
	@Override
	public GlobalPreferences getPendingGlobalPrefsWorking() {
		if (mClientBridge != null)
			return mClientBridge.getPendingGlobalPrefsWorking();
		return null;
	}
	
	@Override
	public void setGlobalPrefsOverride(String globalPrefs) {
		if (mClientBridge != null) {
			mClientBridge.setGlobalPrefsOverride(globalPrefs);
		}
	}
	
	@Override
	public void setGlobalPrefsOverrideStruct(GlobalPreferences globalPrefs) {
		if (mClientBridge != null) {
			mClientBridge.setGlobalPrefsOverrideStruct(globalPrefs);
		}
	}
	
	@Override
	public boolean isGlobalPrefsBeingOverriden() {
		if (mClientBridge != null)
			return mClientBridge.isGlobalPrefsBeingOverriden();
		return false;
	}
	
	@Override
	public void runBenchmarks() {
		if (mClientBridge != null) {
			mClientBridge.runBenchmarks();
		}
	}

	@Override
	public void setRunMode(ClientReplyReceiver callback, int mode) {
		if (mClientBridge != null) {
			mClientBridge.setRunMode(callback, mode);
		}
	}

	@Override
	public void setNetworkMode(ClientReplyReceiver callback, int mode) {
		if (mClientBridge != null) {
			mClientBridge.setNetworkMode(callback, mode);
		}
	}

	@Override
	public void shutdownCore() {
		if (mClientBridge != null) {
			mClientBridge.shutdownCore();
		}
	}

	@Override
	public void doNetworkCommunication() {
		if (mClientBridge != null) {
			mClientBridge.doNetworkCommunication();
		}
	}

	@Override
	public void projectOperation(int operation, String projectUrl) {
		if (mClientBridge != null) {
			mClientBridge.projectOperation(operation, projectUrl);
		}
	}
	
	@Override
	public void projectsOperation(int operation, String[] projectUrls) {
		if (mClientBridge != null) {
			mClientBridge.projectsOperation(operation, projectUrls);
		}
	}

	@Override
	public void taskOperation(int operation, String projectUrl, String taskName) {
		if (mClientBridge != null) {
			mClientBridge.taskOperation(operation, projectUrl, taskName);
		}
	}
	
	@Override
	public void tasksOperation(int operation, TaskDescriptor[] tasks) {
		if (mClientBridge != null) {
			mClientBridge.tasksOperation(operation, tasks);
		}
	}

	@Override
	public void transferOperation(int operation, String projectUrl, String fileName) {
		if (mClientBridge != null) {
			mClientBridge.transferOperation(operation, projectUrl, fileName);
		}
	}
	
	@Override
	public void transfersOperation(int operation, TransferDescriptor[] transfers) {
		if (mClientBridge != null) {
			mClientBridge.transfersOperation(operation, transfers);
		}
	}
}
