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

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import edu.berkeley.boinc.lite.AccountIn;
import edu.berkeley.boinc.lite.GlobalPreferences;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.bridge.ClientBridge;
import sk.boinc.nativeboinc.bridge.ClientBridgeCallback;
import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.ClientPollReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientRequestHandler;
import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.clientconnection.TaskDescriptor;
import sk.boinc.nativeboinc.clientconnection.TransferDescriptor;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.PreferenceName;
import android.app.Service;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.preference.PreferenceManager;
import android.util.Log;


public class ConnectionManagerService extends Service implements
		ClientRequestHandler, ClientBridgeCallback, ConnectivityListener,
		OnSharedPreferenceChangeListener {
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

	private BoincManagerApplication mApp = null;
	
	// wake locker
	private PowerManager.WakeLock mWakeLocker = null;
	private boolean mLockScreenOn = false;
	private boolean mIsLockScreenOnAcquired = false;

	private boolean mDisconnectedByManager = false;
	
	private SharedPreferences mGlobalPrefs = null;
	
	private int mBindCounter = 0;
	
	@Override
	public IBinder onBind(Intent intent) {
		mBindCounter++;
		if (Logging.DEBUG) Log.d(TAG, "onBind(), binCounter="+mBindCounter);
		// Just make sure the service is running:
		startService(new Intent(this, ConnectionManagerService.class));
		return mBinder;
	}

	@Override
	public void onRebind(Intent intent) {
		mBindCounter++;
		if (mTerminateRunnable != null) {
			// There is Runnable to stop the service
			// We cancel that request now
			mHandler.removeCallbacks(mTerminateRunnable);
			mTerminateRunnable = null;
			if (Logging.DEBUG) Log.d(TAG, "onRebind() - canceled stopping of the service");
		}
		else {
			// This is not expected
			if (Logging.ERROR) Log.e(TAG, "onRebind() - mTerminateRunnable empty, , binCounter="+mBindCounter);
			// We just make sure the service is running
			// If service is still running, it's kept running anyway
			startService(new Intent(this, ConnectionManagerService.class));
		}
	}

	@Override
	public boolean onUnbind(Intent intent) {
		mBindCounter--;
		// The observers should be empty at this time (all are unbound from us)
		if (!mObservers.isEmpty()) {
			if (Logging.WARNING) Log.w(TAG, "onUnbind(), but mObservers is not empty, bindCounter="+mBindCounter);
			mObservers.clear();
		}
		// Create runnable which will stop service after grace period
		if (mBindCounter == 0) {
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
			if (Logging.DEBUG) Log.d(TAG, "onUnbind() - Started grace period to terminate self, bindCounter="+mBindCounter);
		}
		return true;
	}

	@Override
	public void onCreate() {
		if (Logging.DEBUG) Log.d(TAG, "onCreate()");
		// Add connectivity monitoring (to be notified when connection is down)
		mConnectivityStatus = new ConnectivityStatus(this, this);
		// Create network statistics handler
		mNetStats = new NetworkStatisticsHandler(this);
		mApp = (BoincManagerApplication)getApplication();
		
		// create wake lock
		initPowerManagement();
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
		
		destroyPowerManagement();
	}

	@Override
	public void onConnectivityLost() {
		if (mClientBridge != null) {
			// We are interested in this event only when connected
			/*if (Logging.DEBUG) Log.d(TAG, "onConnectivityLost() while connected to host " +
			mClientBridge.getClientId().getNickname());*/
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
			/*if (Logging.DEBUG) Log.d(TAG, "onConnectivityChangedType() while connected to host " + 
			mClientBridge.getClientId().getNickname() + ", new connectivity type: " + connectivityType);*/
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
			mDisconnectedByManager = false;
			// handle disconnect for power management
			setUpWakeLock();
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

	@Override
	public void connect(ClientId host, boolean retrieveInitialData) throws NoConnectivityException {
		if (Logging.DEBUG) Log.d(TAG, "connect() to host " + ((host != null) ? host.getNickname() : "(none)"));
		
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
		
		// handle power management changes
		setUpWakeLock();
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
			// handle changes for power management
			setUpWakeLock();
		}
		else {
			if (Logging.DEBUG) Log.d(TAG, "disconnect() - not connected already ");
		}
	}
	
	@Override
	public int getAutoRefresh() {
		if (mClientBridge != null)
			return mClientBridge.getAutoRefresh();
		return -1;
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
		if (mClientBridge != null)
			return mApp.isBoincClientRun() && mClientBridge.isNativeConnected();
		else return false;
	}

	@Override
	public boolean updateClientMode() {
		if (mClientBridge != null) {
			return mClientBridge.updateClientMode();
		}
		return false;
	}

	@Override
	public boolean updateHostInfo() {
		if (mClientBridge != null) {
			return mClientBridge.updateHostInfo();
		}
		return false;
	}
	
	@Override
	public boolean updateProjects() {
		if (mClientBridge != null) {
			return mClientBridge.updateProjects();
		}
		return false;
	}
	
	@Override
	public boolean updateTasks() {
		if (mClientBridge != null) {
			return mClientBridge.updateTasks();
		}
		return false;
	}
	
	@Override
	public boolean updateTransfers() {
		if (mClientBridge != null) {
			return mClientBridge.updateTransfers();
		}
		return false;
	}

	@Override
	public boolean updateMessages() {
		if (mClientBridge != null) {
			return mClientBridge.updateMessages();
		}
		return false;
	}
	
	@Override
	public boolean updateNotices() {
		if (mClientBridge != null) {
			return mClientBridge.updateNotices();
		}
		return false;
	}
	
	@Override
	public void addToScheduledUpdates(ClientReceiver callback, int refreshType, int period) {
		if (mClientBridge != null)
			mClientBridge.addToScheduledUpdates(callback, refreshType, period);
	}
	
	@Override
	public void cancelScheduledUpdates(int refreshType) {
		if (mClientBridge != null) {
			mClientBridge.cancelScheduledUpdates(refreshType);
		}
	}

	@Override
	public boolean getBAMInfo() {
		if (mClientBridge != null) {
			return mClientBridge.getBAMInfo();
		}
		return false;
	}
	
	@Override
	public boolean attachToBAM(String name, String url, String password) {
		if (mClientBridge != null) {
			return mClientBridge.attachToBAM(name, url, password);
		}
		return false;
	}
	
	@Override
	public boolean synchronizeWithBAM() {
		if (mClientBridge != null) {
			return mClientBridge.synchronizeWithBAM();
		}
		return false;
	}
	
	@Override
	public boolean stopUsingBAM() {
		if (mClientBridge != null) {
			return mClientBridge.stopUsingBAM();
		}
		return false;
	}
	
	@Override
	public boolean getAllProjectsList(boolean excludeAttachedProjects) {
		if (mClientBridge != null) {
			return mClientBridge.getAllProjectsList(excludeAttachedProjects);
		}
		return false;
	}
	
	@Override
	public boolean lookupAccount(AccountIn accountIn) {
		if (mClientBridge != null) {
			return mClientBridge.lookupAccount(accountIn);
		}
		return false;
	}
	
	@Override
	public boolean createAccount(AccountIn accountIn) {
		if (mClientBridge != null) {
			return mClientBridge.createAccount(accountIn);
		}
		return false;
	}
	
	@Override
	public boolean projectAttach(String url, String authCode, String projectName) {
		if (mClientBridge != null) {
			return mClientBridge.projectAttach(url, authCode, projectName);
		}
		return false;
	}
	
	/* join of createAccount/lookupAccount with projectAttach */
	@Override
	public boolean addProject(AccountIn accountIn, boolean create) {
		if (mClientBridge != null)
			return mClientBridge.addProject(accountIn, create);
		return false;
	}
	
	@Override
	public boolean getProjectConfig(String url) {
		if (mClientBridge != null) {
			return mClientBridge.getProjectConfig(url);
		}
		return false;
	}
	
	/*
	 * cancels all poll operations
	 */
	@Override
	public void cancelPollOperations(int opFlags) {
		if (mClientBridge != null)
			mClientBridge.cancelPollOperations(opFlags);
	}
	
	@Override
	public boolean handlePendingClientErrors(BoincOp boincOp, ClientReceiver receiver) {
		if (mClientBridge != null) {
			return mClientBridge.handlePendingClientErrors(boincOp, receiver);
		}
		return false;
	}
	
	/**
	 * pending poll errors
	 */
	@Override
	public boolean handlePendingPollErrors(BoincOp boincOp, ClientPollReceiver receiver) {
		if (mClientBridge != null) {
			return mClientBridge.handlePendingPollErrors(boincOp, receiver);
		}
		return false;
	}
	
	@Override
	public Object getPendingOutput(BoincOp boincOp) {
		if (mClientBridge != null) {
			return mClientBridge.getPendingOutput(boincOp);
		}
		return null;
	}
	
	@Override
	public boolean isOpBeingExecuted(BoincOp boincOp) {
		if (mClientBridge != null) {
			return mClientBridge.isOpBeingExecuted(boincOp);
		}
		return false;
	}
	
	@Override
	public boolean getGlobalPrefsWorking() {
		if (mClientBridge != null) {
			return mClientBridge.getGlobalPrefsWorking();
		}
		return false;
	}
	
	@Override
	public boolean setGlobalPrefsOverride(String globalPrefs) {
		if (mClientBridge != null) {
			return mClientBridge.setGlobalPrefsOverride(globalPrefs);
		}
		return false;
	}
	
	@Override
	public boolean setGlobalPrefsOverrideStruct(GlobalPreferences globalPrefs) {
		if (mClientBridge != null) {
			return mClientBridge.setGlobalPrefsOverrideStruct(globalPrefs);
		}
		return false;
	}
	
	@Override
	public boolean runBenchmarks() {
		if (mClientBridge != null) {
			return mClientBridge.runBenchmarks();
		}
		return false;
	}

	@Override
	public boolean setRunMode(int mode) {
		if (mClientBridge != null) {
			return mClientBridge.setRunMode(mode);
		}
		return false;
	}

	@Override
	public boolean setNetworkMode(int mode) {
		if (mClientBridge != null) {
			return mClientBridge.setNetworkMode(mode);
		}
		return false;
	}

	@Override
	public void shutdownCore() {
		if (mClientBridge != null) {
			mClientBridge.shutdownCore();
		}
	}

	@Override
	public boolean doNetworkCommunication() {
		if (mClientBridge != null) {
			return mClientBridge.doNetworkCommunication();
		}
		return false;
	}

	@Override
	public boolean projectOperation(int operation, String projectUrl) {
		if (mClientBridge != null) {
			return mClientBridge.projectOperation(operation, projectUrl);
		}
		return false;
	}
	
	@Override
	public boolean projectsOperation(int operation, String[] projectUrls) {
		if (mClientBridge != null) {
			return mClientBridge.projectsOperation(operation, projectUrls);
		}
		return false;
	}

	@Override
	public boolean taskOperation(int operation, String projectUrl, String taskName) {
		if (mClientBridge != null) {
			return mClientBridge.taskOperation(operation, projectUrl, taskName);
		}
		return false;
	}
	
	@Override
	public boolean tasksOperation(int operation, TaskDescriptor[] tasks) {
		if (mClientBridge != null) {
			return mClientBridge.tasksOperation(operation, tasks);
		}
		return false;
	}

	@Override
	public boolean transferOperation(int operation, String projectUrl, String fileName) {
		if (mClientBridge != null) {
			return mClientBridge.transferOperation(operation, projectUrl, fileName);
		}
		return false;
	}
	
	@Override
	public boolean transfersOperation(int operation, TransferDescriptor[] transfers) {
		if (mClientBridge != null) {
			return mClientBridge.transfersOperation(operation, transfers);
		}
		return false;
	}

	/*
	 * power management
	 */
	
	private void initPowerManagement() {
		PowerManager powerManager = (PowerManager)getSystemService(POWER_SERVICE);
		mWakeLocker = powerManager.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, "onScreenAlways");
		
		mGlobalPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		mLockScreenOn = mGlobalPrefs.getBoolean(PreferenceName.LOCK_SCREEN_ON, false);
		
		mGlobalPrefs.registerOnSharedPreferenceChangeListener(this);
	}
	
	private void destroyPowerManagement() {
		mGlobalPrefs.unregisterOnSharedPreferenceChangeListener(this);
		// release wake locker
		if (mWakeLocker != null) {
			if (mWakeLocker.isHeld())
				mWakeLocker.release();
		}
		mWakeLocker = null;
		mGlobalPrefs = null;
	}
	
	public void acquireLockScreenOn() {
		if (Logging.DEBUG) Log.d(TAG, "LockScreenOn acquired");
		mIsLockScreenOnAcquired = true;
		setUpWakeLock();
	}
	
	public void releaseLockScreenOn() {
		if (Logging.DEBUG) Log.d(TAG, "LockScreenOn released");
		mIsLockScreenOnAcquired = false;
		setUpWakeLock();
	}
	
	private void setUpWakeLock() {
		if (mWakeLocker != null) {
			if (Logging.DEBUG) Log.d(TAG, "SetUp WakeLock: "+
						(mClientBridge != null && mLockScreenOn && mIsLockScreenOnAcquired));
			// if connected, lockscreenOn and screen lock and if acquired
			if (mClientBridge != null && mLockScreenOn && mIsLockScreenOnAcquired) {
				if (!mWakeLocker.isHeld())
					mWakeLocker.acquire();
			} else { // to release
				if (mWakeLocker.isHeld())
					mWakeLocker.release();
			}
		}
	}
	
	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
			String key) {
		if (key.equals(PreferenceName.LOCK_SCREEN_ON)) {
			if (Logging.DEBUG) Log.d(TAG, "Change Lock screen on");
			mLockScreenOn = sharedPreferences.getBoolean(PreferenceName.LOCK_SCREEN_ON, false);
			setUpWakeLock();
		}
	}
}
