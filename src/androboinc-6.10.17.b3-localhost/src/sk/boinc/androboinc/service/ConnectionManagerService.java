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

package sk.boinc.androboinc.service;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import sk.boinc.androboinc.bridge.ClientBridge;
import sk.boinc.androboinc.bridge.ClientBridgeCallback;
import sk.boinc.androboinc.clientconnection.ClientReplyReceiver;
import sk.boinc.androboinc.clientconnection.ClientRequestHandler;
import sk.boinc.androboinc.clientconnection.NoConnectivityException;
import sk.boinc.androboinc.debug.Logging;
import sk.boinc.androboinc.util.ClientId;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;


public class ConnectionManagerService extends Service implements ClientRequestHandler, ClientBridgeCallback, ConnectivityListener {
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
	private Set<ClientReplyReceiver> mObservers = new HashSet<ClientReplyReceiver>();
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
			if (Logging.DEBUG) Log.d(TAG, "onConnectivityLost() while connected to host " + mClientBridge.getClientId().getNickname());
			// TODO Handle connectivity loss
		}
	}

	@Override
	public void onConnectivityRestored(int connectivityType) {
		if (mClientBridge != null) {
			// We are interested in this event only when connected
			if (Logging.DEBUG) Log.d(TAG, "onConnectivityRestored() while connected to host " + mClientBridge.getClientId().getNickname() + ", connectivity type: " + connectivityType);
			// TODO Handle connectivity restoration
		}
	}

	@Override
	public void onConnectivityChangedType(int connectivityType) {
		if (mClientBridge != null) {
			// We are interested in this event only when connected
			if (Logging.DEBUG) Log.d(TAG, "onConnectivityChangedType() while connected to host " + mClientBridge.getClientId().getNickname() + ", new connectivity type: " + connectivityType);
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
	public void registerStatusObserver(ClientReplyReceiver observer) {
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
	public void unregisterStatusObserver(ClientReplyReceiver observer) {
		mObservers.remove(observer);
		if (mClientBridge != null) {
			mClientBridge.unregisterStatusObserver(observer);
		}
		if (Logging.DEBUG) Log.d(TAG, "Detached observer: " + observer.toString());
	}

	@Override
	public void connect(ClientId host, boolean retrieveInitialData) throws NoConnectivityException {
		if (Logging.DEBUG) Log.d(TAG, "connect() to host " + host.getNickname());
		if (mClientBridge != null) {
			// Connected to some client - disconnect it first
			disconnect();
		}
		//if (mConnectivityStatus.isConnected()) {
		// Create new bridge first
		mClientBridge = new ClientBridge(this, mNetStats);
		// Propagate all current observers to bridge, so they will be informed about status
		Iterator<ClientReplyReceiver> it = mObservers.iterator();
		while (it.hasNext()) {
			ClientReplyReceiver observer = it.next();
			mClientBridge.registerStatusObserver(observer);
		}
		// Finally, initiate connection to remote client
		mClientBridge.connect(host, retrieveInitialData);
	}

	@Override
	public void disconnect() {
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
	public void updateClientMode(ClientReplyReceiver callback) {
		if (mClientBridge != null) {
			mClientBridge.updateClientMode(callback);
		}
	}

	@Override
	public void updateHostInfo(ClientReplyReceiver callback) {
		if (mClientBridge != null) {
			mClientBridge.updateHostInfo(callback);
		}
	}

	@Override
	public void updateProjects(ClientReplyReceiver callback) {
		if (mClientBridge != null) {
			mClientBridge.updateProjects(callback);
		}
	}

	@Override
	public void updateTasks(ClientReplyReceiver callback) {
		if (mClientBridge != null) {
			mClientBridge.updateTasks(callback);
		}
	}

	@Override
	public void updateTransfers(ClientReplyReceiver callback) {
		if (mClientBridge != null) {
			mClientBridge.updateTransfers(callback);
		}
	}

	@Override
	public void updateMessages(ClientReplyReceiver callback) {
		if (mClientBridge != null) {
			mClientBridge.updateMessages(callback);
		}
	}

	@Override
	public void cancelScheduledUpdates(ClientReplyReceiver callback) {
		if (mClientBridge != null) {
			mClientBridge.cancelScheduledUpdates(callback);
		}
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
	public void projectOperation(ClientReplyReceiver callback, int operation, String projectUrl) {
		if (mClientBridge != null) {
			mClientBridge.projectOperation(callback, operation, projectUrl);
		}
	}

	@Override
	public void taskOperation(ClientReplyReceiver callback, int operation, String projectUrl, String taskName) {
		if (mClientBridge != null) {
			mClientBridge.taskOperation(callback, operation, projectUrl, taskName);
		}
	}

	@Override
	public void transferOperation(ClientReplyReceiver callback, int operation, String projectUrl, String fileName) {
		if (mClientBridge != null) {
			mClientBridge.transferOperation(callback, operation, projectUrl, fileName);
		}
	}
}
