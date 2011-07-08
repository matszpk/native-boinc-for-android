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

package sk.boinc.androboinc.bridge;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.Vector;

import sk.boinc.androboinc.clientconnection.ClientReplyReceiver;
import sk.boinc.androboinc.clientconnection.ClientRequestHandler;
import sk.boinc.androboinc.clientconnection.HostInfo;
import sk.boinc.androboinc.clientconnection.MessageInfo;
import sk.boinc.androboinc.clientconnection.ModeInfo;
import sk.boinc.androboinc.clientconnection.ProjectInfo;
import sk.boinc.androboinc.clientconnection.TaskInfo;
import sk.boinc.androboinc.clientconnection.TransferInfo;
import sk.boinc.androboinc.clientconnection.VersionInfo;
import sk.boinc.androboinc.debug.Logging;
import sk.boinc.androboinc.debug.NetStats;
import sk.boinc.androboinc.util.ClientId;
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
			Iterator<ClientReplyReceiver> it = mObservers.iterator();
			while (it.hasNext()) {
				ClientReplyReceiver observer = it.next();
				observer.clientConnectionProgress(progress);
			}
		}

		public void notifyConnected(VersionInfo clientVersion) {
			mConnected = true;
			mRemoteClientVersion = clientVersion;
			Iterator<ClientReplyReceiver> it = mObservers.iterator();
			while (it.hasNext()) {
				ClientReplyReceiver observer = it.next();
				observer.clientConnected(mRemoteClientVersion);
			}
		}

		public void notifyDisconnected() {
			mConnected = false;
			mRemoteClientVersion = null;
			Iterator<ClientReplyReceiver> it = mObservers.iterator();
			while (it.hasNext()) {
				ClientReplyReceiver observer = it.next();
				observer.clientDisconnected();
				if (Logging.DEBUG) Log.d(TAG, "Detached observer: " + observer.toString()); // see below clearing of all observers
			}
			mObservers.clear();
		}


		public void updatedClientMode(final ClientReplyReceiver callback, final ModeInfo modeInfo) {
			if (callback == null) {
				// No specific callback - broadcast to all observers
				// This is used for early notification after connect
				Iterator<ClientReplyReceiver> it = mObservers.iterator();
				while (it.hasNext()) {
					ClientReplyReceiver observer = it.next();
					observer.updatedClientMode(modeInfo);
				}
				return;
			}
			// Check whether callback is still present in observers
			if (mObservers.contains(callback)) {
				// Observer is still present, so we can call it back with data
				boolean periodicAllowed = callback.updatedClientMode(modeInfo);
				if (periodicAllowed) {
					mAutoRefresh.scheduleAutomaticRefresh(callback, AutoRefresh.CLIENT_MODE);
				}
			}
		}

		public void updatedHostInfo(final ClientReplyReceiver callback, final HostInfo hostInfo) {
			// First, check whether callback is still present in observers
			if (mObservers.contains(callback)) {
				// Yes, observer is still present, so we can call it back with data
				callback.updatedHostInfo(hostInfo);
			}
		}

		public void updatedProjects(final ClientReplyReceiver callback, final Vector <ProjectInfo> projects) {
			if (callback == null) {
				// No specific callback - broadcast to all observers
				// This is used for early notification after connect
				Iterator<ClientReplyReceiver> it = mObservers.iterator();
				while (it.hasNext()) {
					ClientReplyReceiver observer = it.next();
					observer.updatedProjects(projects);
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

		public void updatedTasks(final ClientReplyReceiver callback, final Vector <TaskInfo> tasks) {
			if (callback == null) {
				// No specific callback - broadcast to all observers
				// This is used for early notification after connect
				Iterator<ClientReplyReceiver> it = mObservers.iterator();
				while (it.hasNext()) {
					ClientReplyReceiver observer = it.next();
					observer.updatedTasks(tasks);
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

		public void updatedTransfers(final ClientReplyReceiver callback, final Vector <TransferInfo> transfers) {
			if (callback == null) {
				// No specific callback - broadcast to all observers
				// This is used for early notification after connect
				Iterator<ClientReplyReceiver> it = mObservers.iterator();
				while (it.hasNext()) {
					ClientReplyReceiver observer = it.next();
					observer.updatedTransfers(transfers);
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

		public void updatedMessages(final ClientReplyReceiver callback, final Vector <MessageInfo> messages) {
			if (callback == null) {
				// No specific callback - broadcast to all observers
				// This is used for early notification after connect
				Iterator<ClientReplyReceiver> it = mObservers.iterator();
				while (it.hasNext()) {
					ClientReplyReceiver observer = it.next();
					observer.updatedMessages(messages);
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

	private final ReplyHandler mReplyHandler = new ReplyHandler();

	private Set<ClientReplyReceiver> mObservers = new HashSet<ClientReplyReceiver>();
	private boolean mConnected = false;

	private ClientBridgeCallback mCallback = null;
	private ClientBridgeWorkerThread mWorker = null;

	private ClientId mRemoteClient = null;
	private VersionInfo mRemoteClientVersion = null;

	private AutoRefresh mAutoRefresh = null;

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
	public void registerStatusObserver(ClientReplyReceiver observer) {
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
	public void unregisterStatusObserver(ClientReplyReceiver observer) {
		// Observer does not want to receive notifications anymore - remove him
		mObservers.remove(observer);
		if (mConnected) {
			// The observer could have automatic refresh pending
			// Remove it now
			mAutoRefresh.unscheduleAutomaticRefresh(observer);
		}
		if (Logging.DEBUG) Log.d(TAG, "Detached observer: " + observer.toString());
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
	public void updateClientMode(final ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mWorker.updateClientMode(callback);
	}

	@Override
	public void updateHostInfo(final ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mWorker.updateHostInfo(callback);
	}

	@Override
	public void updateProjects(final ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mWorker.updateProjects(callback);
	}

	@Override
	public void updateTasks(final ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mWorker.updateTasks(callback);
	}

	@Override
	public void updateTransfers(final ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mWorker.updateTransfers(callback);
	}

	@Override
	public void updateMessages(final ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		mWorker.updateMessages(callback);
	}

	@Override
	public void cancelScheduledUpdates(ClientReplyReceiver callback) {
		if (mRemoteClient == null) return; // not connected
		// Cancel pending updates in worker thread
		mWorker.cancelPendingUpdates(callback);
		// Remove scheduled auto-refresh (if any)
		mAutoRefresh.unscheduleAutomaticRefresh(callback);
	}

	@Override
	public void runBenchmarks() {
		if (mRemoteClient == null) return; // not connected
		mWorker.runBenchmarks();
	}

	@Override
	public void setRunMode(final ClientReplyReceiver callback, final int mode) {
		if (mRemoteClient == null) return; // not connected
		mWorker.setRunMode(callback, mode);
	}

	@Override
	public void setNetworkMode(final ClientReplyReceiver callback, final int mode) {
		if (mRemoteClient == null) return; // not connected
		mWorker.setNetworkMode(callback, mode);
	}

	@Override
	public void shutdownCore() {
		if (mRemoteClient == null) return; // not connected
		mWorker.shutdownCore();
	}

	@Override
	public void doNetworkCommunication() {
		if (mRemoteClient == null) return; // not connected
		mWorker.doNetworkCommunication();
	}

	@Override
	public void projectOperation(final ClientReplyReceiver callback, final int operation, final String projectUrl) {
		if (mRemoteClient == null) return; // not connected
		mWorker.projectOperation(callback, operation, projectUrl);
	}

	@Override
	public void taskOperation(final ClientReplyReceiver callback, final int operation, final String projectUrl, final String taskName) {
		if (mRemoteClient == null) return; // not connected
		mWorker.taskOperation(callback, operation, projectUrl, taskName);
	}

	@Override
	public void transferOperation(final ClientReplyReceiver callback, final int operation, final String projectUrl, final String fileName) {
		if (mRemoteClient == null) return; // not connected
		mWorker.transferOperation(callback, operation, projectUrl, fileName);
	}
}
