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

import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;
import java.util.Vector;

import sk.boinc.androboinc.clientconnection.ClientReplyReceiver;
import sk.boinc.androboinc.clientconnection.HostInfo;
import sk.boinc.androboinc.clientconnection.MessageInfo;
import sk.boinc.androboinc.clientconnection.ModeInfo;
import sk.boinc.androboinc.clientconnection.ProjectInfo;
import sk.boinc.androboinc.clientconnection.TaskInfo;
import sk.boinc.androboinc.clientconnection.TransferInfo;
import sk.boinc.androboinc.clientconnection.VersionInfo;
import sk.boinc.androboinc.debug.Debugging;
import sk.boinc.androboinc.debug.Logging;
import sk.boinc.androboinc.debug.NetStats;
import sk.boinc.androboinc.util.ClientId;
import sk.boinc.androboinc.util.PreferenceName;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.Log;
import edu.berkeley.boinc.lite.App;
import edu.berkeley.boinc.lite.CcState;
import edu.berkeley.boinc.lite.CcStatus;
import edu.berkeley.boinc.lite.Message;
import edu.berkeley.boinc.lite.Project;
import edu.berkeley.boinc.lite.Result;
import edu.berkeley.boinc.lite.RpcClient;
import edu.berkeley.boinc.lite.Transfer;
import edu.berkeley.boinc.lite.Workunit;


public class ClientBridgeWorkerHandler extends Handler {
	private static final String TAG = "ClientBridgeWorkerHandler";

	private static final int MESSAGE_INITIAL_LIMIT = 50;

	private ClientBridge.ReplyHandler mReplyHandler = null; // write in UI thread only
	private Context mContext = null;
	private Boolean mDisconnecting = false; // read by worker thread, write by both threads

	private RpcClient mRpcClient = null; // read/write only by worker thread 
	private NetStats mNetStats = null;
	private Formatter mFormatter = null;

	private Set<ClientReplyReceiver> mUpdateCancel = new HashSet<ClientReplyReceiver>();

	private VersionInfo mClientVersion = null;
	private Map<String, ProjectInfo> mProjects = new HashMap<String, ProjectInfo>();
	private Map<String, App> mApps = new HashMap<String, App>();
	private Map<String, Workunit> mWorkunits = new HashMap<String, Workunit>();
	private Map<String, TaskInfo> mTasks = new HashMap<String, TaskInfo>();
	private Set<String> mActiveTasks = new HashSet<String>();
	private Vector<TransferInfo> mTransfers = new Vector<TransferInfo>();
	private SortedMap<Integer, MessageInfo> mMessages = new TreeMap<Integer, MessageInfo>();
	private boolean mInitialStateRetrieved = false;


	public ClientBridgeWorkerHandler(ClientBridge.ReplyHandler replyHandler, final Context context, final NetStats netStats) {
		mReplyHandler = replyHandler;
		mContext = context;
		mNetStats = netStats;
		mFormatter = new Formatter(mContext);
	}

	public void cleanup() {
		mFormatter.cleanup();
		mFormatter = null;
		if (mRpcClient != null) {
			if (Logging.WARNING) Log.w(TAG, "RpcClient still opened in cleanup(), closing it now");
			closeConnection();
		}
		final ClientBridge.ReplyHandler moribund = mReplyHandler;
		mReplyHandler = null;
		moribund.post(new Runnable() {
			@Override
			public void run() {
				moribund.disconnected();
			}			
		});
	}

	private void closeConnection() {
		if (mRpcClient != null) {
			mRpcClient.close();
			mRpcClient = null;
			if (Logging.DEBUG) Log.d(TAG, "Connection closed");
		}
	}

	private void rpcFailed() {
		notifyDisconnected();
		closeConnection();
	}


	public void connect(ClientId client, boolean retrieveInitialData) {
		if (mDisconnecting) return;  // already in disconnect phase
		if (Logging.DEBUG) Log.d(TAG, "Opening connection to " + client.getNickname());
		notifyProgress(ClientReplyReceiver.PROGRESS_CONNECTING);
		mRpcClient = new RpcClient(mNetStats);
		if (!mRpcClient.open(client.getAddress(), client.getPort())) {
			// Connect failed
			if (Logging.WARNING) Log.w(TAG, "Failed connect to " + client.getAddress() + ":" + client.getPort());
			mRpcClient = null;
			notifyDisconnected();
			return;
		}
		if (Debugging.INSERT_DELAYS) { try { Thread.sleep(1000); } catch (InterruptedException e) {} }
		String password = client.getPassword();
		if (!password.equals("")) {
			// Password supplied, we need to authorize
			if (mDisconnecting) return;  // already in disconnect phase
			notifyProgress(ClientReplyReceiver.PROGRESS_AUTHORIZATION_PENDING);
			if (!mRpcClient.authorize(password)) {
				// Authorization failed
				if (Logging.WARNING) Log.w(TAG, "Authorization failed for " + client.getAddress() + ":" + client.getPort());
				notifyDisconnected();
				closeConnection();
				return;
			}
			if (Debugging.INSERT_DELAYS) { try { Thread.sleep(1000); } catch (InterruptedException e) {} }
		}
		if (Logging.DEBUG) Log.d(TAG, "Connected to " + client.getNickname());
		edu.berkeley.boinc.lite.VersionInfo versionInfo = mRpcClient.exchangeVersions();
		if (versionInfo != null) {
			// Newer client, supports operation <exchange_versions>
			mClientVersion = VersionInfoCreator.create(versionInfo);
		}
		if (retrieveInitialData) {
			// Before we reply, we also retrieve the complete state
			// It can be time consuming, but it is very useful in typical usage;
			// At the time of connected notification the first data will be
			// already available (the screen is not empty)
			// It is not so useful in ManageClientActivity, where data could be possibly
			// not needed - they have to be retrieved later when returning from
			// ManageClientActivity to home BoincManagerActivity (if still connected)
			notifyProgress(ClientReplyReceiver.PROGRESS_INITIAL_DATA);
			initialStateRetrieval();
		}
		else if (mClientVersion == null) {
			// For older versions of client (those that do not support <exchange_versions>)
			// we will retrieve full state, because we must get the version of connected client
			// But we will not do full initial state update, only version info setting so
			// some time can be saved this way (no parsing of all projects, applications, workunits,
			// tasks, no retrieval of transfers/messages...)
			CcState ccState = mRpcClient.getState();
			if (ccState == null) {
				if (Logging.INFO) Log.i(TAG, "RPC failed in connect()");
				notifyDisconnected();
				closeConnection();
				return;
			}
			if (mDisconnecting) return;  // already in disconnect phase
			mClientVersion = VersionInfoCreator.create(ccState.version_info);
		}
		notifyConnected(mClientVersion);
	}

	public void disconnect() {
		// Disconnect request
		// This is not run in the worker thread, but in UI thread
		// Therefore we will not directly operate mRpcClient here, 
		// as it could be just in use by worker thread
		if (Logging.DEBUG) Log.d(TAG, "disconnect()");
		notifyDisconnected();
		// Now, trigger socket closing (to be done by worker thread)
		this.post(new Runnable() {
			@Override
			public void run() {
				closeConnection();
			}
		});
	}

	public void cancelPendingUpdates(final ClientReplyReceiver callback) {
		// This is run in UI thread - immediately add callback to list,
		// so worker thread will not run update for this callback afterwards 
		synchronized (mUpdateCancel) {
			mUpdateCancel.add(callback);
		}
		// Put removal of callback at the end of queue. So only currently pending
		// updates (these already in queue) will be canceled. Any later updates
		// for the same calback will be again processed normally
		this.post(new Runnable() {
			@Override
			public void run() {
				synchronized (mUpdateCancel) {
					mUpdateCancel.remove(callback);
				}
			}
		});
	}

	public void updateClientMode(final ClientReplyReceiver callback) {
		if (mDisconnecting) return;  // already in disconnect phase
		synchronized (mUpdateCancel) {
			if (mUpdateCancel.contains(callback)) {
				// This update was canceled meanwhile
				if (Logging.DEBUG) Log.d(TAG, "Canceled updateClientMode(" + callback.toString() + ")");
				return;
			}
		}
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		CcStatus ccStatus = mRpcClient.getCcStatus();
		if (ccStatus == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in updateClientMode()");
			rpcFailed();
			return;
		}
		final ModeInfo clientMode = ModeInfoCreator.create(ccStatus);
		// Finally, send reply back to the calling thread (that is UI thread)
		updatedClientMode(callback, clientMode);
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
	}

	public void updateHostInfo(final ClientReplyReceiver callback) {
		if (mDisconnecting) return;  // already in disconnect phase
		synchronized (mUpdateCancel) {
			if (mUpdateCancel.contains(callback)) {
				// This update was canceled meanwhile
				if (Logging.DEBUG) Log.d(TAG, "Canceled updateHostInfo(" + callback.toString() + ")");
				return;
			}
		}
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		edu.berkeley.boinc.lite.HostInfo boincHostInfo = mRpcClient.getHostInfo();
		if (boincHostInfo == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in updateHostInfo()");
			rpcFailed();
			return;
		}
		final HostInfo hostInfo = HostInfoCreator.create(boincHostInfo, mFormatter);
		// Finally, send reply back to the calling thread (that is UI thread)
		updatedHostInfo(callback, hostInfo);
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
	}

	public void updateProjects(final ClientReplyReceiver callback) {
		if (mDisconnecting) return;  // already in disconnect phase
		synchronized (mUpdateCancel) {
			if (mUpdateCancel.contains(callback)) {
				// This update was canceled meanwhile
				if (Logging.DEBUG) Log.d(TAG, "Canceled updateProjects(" + callback.toString() + ")");
				return;
			}
		}
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		Vector<Project> projects = mRpcClient.getProjectStatus();
		if (projects == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in updateProjects()");
			rpcFailed();
			return;
		}
		dataSetProjects(projects);
		updatedProjects(callback, getProjects());
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
	}

	public void updateTasks(final ClientReplyReceiver callback) {
		if (mDisconnecting) return;  // already in disconnect phase
		synchronized (mUpdateCancel) {
			if (mUpdateCancel.contains(callback)) {
				// This update was canceled meanwhile
				if (Logging.DEBUG) Log.d(TAG, "Canceled updateTasks(" + callback.toString() + ")");
				return;
			}
		}
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		boolean updateFinished = false;
		Vector<Result> results;
		if (!mInitialStateRetrieved) {
			// Initial state retrieval was not done yet
			initialStateRetrieval();
			updateFinished = true;
		}
		else {
			// First try to get only results
			results = mRpcClient.getResults();
			if (results == null) {
				if (Logging.INFO) Log.i(TAG, "RPC failed in updateTasks()");
				rpcFailed();
				return;
			}
			updateFinished = dataUpdateTasks(results);
		}
		if (!updateFinished) {
			// Update still not finished :-(
			// This is normal in case new work-unit arrived, because we have
			// just fetched new result, but we do not have stored corresponding WU
			// (so we cannot find application of the new task, as it is part of 
			// <workunit> data, not part of <result> data.
			// We will retrieve complete state now
			updateState();
		}
		updatedTasks(callback, getTasks());
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
	}

	public void updateTransfers(final ClientReplyReceiver callback) {
		if (mDisconnecting) return;  // already in disconnect phase
		synchronized (mUpdateCancel) {
			if (mUpdateCancel.contains(callback)) {
				// This update was canceled meanwhile
				if (Logging.DEBUG) Log.d(TAG, "Canceled updateTransfers(" + callback.toString() + ")");
				return;
			}
		}
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		Vector<Transfer> transfers = mRpcClient.getFileTransfers();
		if (transfers == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in updateTransfers()");
			rpcFailed();
			return;
		}
		dataSetTransfers(transfers);
		updatedTransfers(callback, getTransfers());
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
	}

	public void updateMessages(final ClientReplyReceiver callback) {
		if (mDisconnecting) return;  // already in disconnect phase
		synchronized (mUpdateCancel) {
			if (mUpdateCancel.contains(callback)) {
				// This update was canceled meanwhile
				if (Logging.DEBUG) Log.d(TAG, "Canceled updateMessages(" + callback.toString() + ")");
				return;
			}
		}
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		int reqSeqno = (mMessages.isEmpty()) ? 0 : mMessages.lastKey();
		if (reqSeqno == 0) {
			// No messages stored yet
			SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(mContext);
			boolean recentMessagesOnly = globalPrefs.getBoolean(PreferenceName.LIMIT_MESSAGES, true);
			if (recentMessagesOnly) {
				// Preference: Initially retrieve only 50 (MESSAGE_INITIAL_LIMIT) recent messages
				int lastSeqno = mRpcClient.getMessageCount();
				if (lastSeqno > 0) {
					// Retrieval of message count is supported operation - get only last 50 messages
					reqSeqno = lastSeqno - MESSAGE_INITIAL_LIMIT;
					if (reqSeqno < 1) reqSeqno = 0; // get all if less than 50 messages are available
				}
			}
		}
		if (mDisconnecting) return;  // already in disconnect phase
		Vector<Message> messages = mRpcClient.getMessages(reqSeqno);
		if (messages == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in updateMessages()");
			rpcFailed();
			return;
		}
		dataUpdateMessages(messages);
		updatedMessages(callback, getMessages());
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
	}

	public void runBenchmarks() {
		if (mDisconnecting) return;  // already in disconnect phase
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		boolean success = mRpcClient.runBenchmarks();
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
		if (!success) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in runBenchmarks()");
			rpcFailed();
			return;
		}
	}

	public void setRunMode(final ClientReplyReceiver callback, int mode) {
		if (mDisconnecting) return;  // already in disconnect phase
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		mRpcClient.setRunMode(mode, 0);
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
		// Regardless of success we run update of client mode
		// If there is problem with socket, it will be handled there
		updateClientMode(callback);
	}

	public void setNetworkMode(final ClientReplyReceiver callback, int mode) {
		if (mDisconnecting) return;  // already in disconnect phase
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		mRpcClient.setNetworkMode(mode, 0);
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
		// Regardless of success we run update of client mode
		// If there is problem with socket, it will be handled there
		updateClientMode(callback);
	}

	public void shutdownCore() {
		if (mDisconnecting) return;  // already in disconnect phase
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		mRpcClient.quit();
		// We have to check, whether we are really disconnected
		// We will try for 5 seconds only
		boolean connectionAlive = true;
		try {
			// First, give the other side a little time to close socket
			Thread.sleep(100);
			for (int i = 0; i < 5; ++i) {
				connectionAlive = mRpcClient.connectionAlive();
				if (!connectionAlive) {
					// The socket is already closed on the other side
					if (Logging.DEBUG) Log.d(TAG, "shutdownCore(), socket closed after " + i + " retries since trigger");
					break;
				}
				Thread.sleep(1000);
			}
		}
		catch (InterruptedException e) {
			// Interrupted while sleep, we better close socket now
			if (Logging.INFO) Log.i(TAG, "interrupted sleep in shutdownCore()");
			connectionAlive = false;
		}
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
		if (!connectionAlive) {
			// Socket was closed on remote side, so connection was lost as expected
			// We notify about lost connection
			notifyDisconnected();
			closeConnection();
			return;
		}
		// Otherwise, there is still connection present, we did not shutdown
		// remote client, we keep connection, so user is aware and can possibly
		// re-try
	}

	public void doNetworkCommunication() {
		if (mDisconnecting) return;  // already in disconnect phase
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		boolean success = mRpcClient.networkAvailable();
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
		if (!success) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in doNetworkCommunication()");
			rpcFailed();
			return;
		}
	}

	public void projectOperation(final ClientReplyReceiver callback, int operation, String projectUrl) {
		if (mDisconnecting) return;  // already in disconnect phase
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		mRpcClient.projectOp(operation, projectUrl);
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
		// Regardless of success we run update of projects
		// If there is problem with socket, it will be handled there
		updateProjects(callback);
	}

	public void taskOperation(final ClientReplyReceiver callback, int operation, String projectUrl, String taskName) {
		if (mDisconnecting) return;  // already in disconnect phase
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		mRpcClient.resultOp(operation, projectUrl, taskName);
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
		// Regardless of success we run update of tasks
		// If there is problem with socket, it will be handled there
		updateTasks(callback);
	}

	public void transferOperation(final ClientReplyReceiver callback, int operation, String projectUrl, String fileName) {
		if (mDisconnecting) return;  // already in disconnect phase
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_STARTED);
		mRpcClient.transferOp(operation, projectUrl, fileName);
		notifyProgress(ClientReplyReceiver.PROGRESS_XFER_FINISHED);
		// Regardless of success we run update of transfers
		// If there is problem with socket, it will be handled there
		updateTransfers(callback);
	}

	private synchronized void notifyProgress(final int progress) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.notifyProgress(progress);
			}
		});
	}

	private synchronized void notifyConnected(final VersionInfo clientVersion) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.notifyConnected(clientVersion);
			}
		});
	}

	private synchronized void notifyDisconnected() {
		if (mDisconnecting) return; // already notified (by other thread)
		// Set flag, so no further notifications/replies will be posted to UI-thread
		mDisconnecting = true;
		// post last notification - about disconnect
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.notifyDisconnected(); // will send notification to observers
				mReplyHandler.disconnecting(); // will initiate clearing of bridge
				// The mDisconnecting set to true above will prevent further posts
				// and all post() calls are guarded by synchronized statement
			}
		});
	}

	private synchronized void updatedClientMode(final ClientReplyReceiver callback, final ModeInfo clientMode) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedClientMode(callback, clientMode);
			}
		});
	}

	private synchronized void updatedHostInfo(final ClientReplyReceiver callback, final HostInfo hostInfo) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedHostInfo(callback, hostInfo);
			}
		});
	}

	private synchronized void updatedProjects(final ClientReplyReceiver callback, final Vector<ProjectInfo> projects) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedProjects(callback, projects);
			}
		});
	}

	private synchronized void updatedTasks(final ClientReplyReceiver callback, final Vector<TaskInfo> tasks) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedTasks(callback, tasks);
			}
		});
	}

	private synchronized void updatedTransfers(final ClientReplyReceiver callback, final Vector<TransferInfo> transfers) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedTransfers(callback, transfers);
			}
		});
	}

	private synchronized void updatedMessages(final ClientReplyReceiver callback, final Vector<MessageInfo> messages) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedMessages(callback, messages);
			}
		});
	}

	private void updateState() {
		if (mDisconnecting) return;  // Started disconnect phase, don't bother with further data retrieval
		CcState ccState = mRpcClient.getState();
		if (ccState == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in updateCcState()");
			rpcFailed();
			return;
		}
		if (mDisconnecting) return;  // already in disconnect phase
		dataSetProjects(ccState.projects);
		dataSetApps(ccState.apps);
		if (mDisconnecting) return;  // already in disconnect phase
		dataSetTasks(ccState.workunits, ccState.results);
	}

	private void initialStateRetrieval() {
		if (mDisconnecting) return;  // Started disconnect phase, don't bother with further data retrieval
		CcState ccState = mRpcClient.getState();
		if (ccState == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in initialStateRetrieval()");
			rpcFailed();
			return;
		}
		if (mDisconnecting) return;  // already in disconnect phase
		if (mClientVersion == null) {
			// Older versions of client do not support separate <exchange_versions>,
			// but they report version in state
			mClientVersion = VersionInfoCreator.create(ccState.version_info);
		}
		dataSetProjects(ccState.projects);
		updatedProjects(null, getProjects());
		dataSetApps(ccState.apps);
		if (mDisconnecting) return;  // already in disconnect phase
		dataSetTasks(ccState.workunits, ccState.results);
		updatedTasks(null, getTasks());
		// Retrieve also transfers. Most of time empty anyway, so it runs fast
		updateTransfers(null);
		if (mDisconnecting) return;  // already in disconnect phase
		// Messages are useful in most of cases, so we start to retrieve them automatically as well
		updateMessages(null);
		mInitialStateRetrieved = true;
	}

	private void dataSetProjects(Vector<Project> projects) {
		if (Logging.DEBUG) Log.d(TAG, "dataSetProjects(): Begin update");
		mProjects.clear();
		Iterator<Project> pi;
		// First calculate sum of all resource shares, to get base
		float totalResources = 0;
		pi = projects.iterator();
		while (pi.hasNext()) {
			totalResources += pi.next().resource_share;
		}
		// Now set all projects, using the sum of shares
		pi = projects.iterator();
		while (pi.hasNext()) {
			Project prj = pi.next();
			ProjectInfo project = ProjectInfoCreator.create(prj, totalResources, mFormatter);
			mProjects.put(prj.master_url, project);
		}
		if (Logging.DEBUG) Log.d(TAG, "dataSetProjects(): End update");
	}

	private void dataSetApps(Vector<App> apps) {
		mApps.clear();
		Iterator<App> ai = apps.iterator();
		while (ai.hasNext()) {
			App app = ai.next();
			mApps.put(app.name, app);
		}
	}

	private void dataSetTasks(Vector<Workunit> workunits, Vector<Result> results) {
		if (Logging.DEBUG) Log.d(TAG, "dataSetTasks(): Begin update");
		mTasks.clear();
		mActiveTasks.clear();
		// First, parse workunits, to create auxiliary map of workunits
		mWorkunits.clear();
		Iterator<Workunit> wi = workunits.iterator();
		while (wi.hasNext()) {
			Workunit wu = wi.next();
			mWorkunits.put(wu.name, wu);
		}
		// Then, parse results to set the tasks data
		Iterator<Result> ri = results.iterator();
		while (ri.hasNext()) {
			Result result = ri.next();
			ProjectInfo pi = mProjects.get(result.project_url);
			if (pi == null) {
				if (Logging.WARNING) Log.w(TAG, "No project info for WU=" + result.name + " (project_url: " + result.project_url + "), skipping WU");
				continue;
			}
			Workunit workunit = mWorkunits.get(result.wu_name);
			if (workunit == null) {
				if (Logging.WARNING) Log.w(TAG, "No workunit info for WU=" + result.name + " (wu_name: " + result.wu_name + "), skipping WU");
				continue;
			}
			App app = mApps.get(workunit.app_name);
			if (app == null) {
				if (Logging.WARNING) Log.w(TAG, "No application info for WU=" + result.name + " (app_name: " + workunit.app_name + "), skipping WU");
				continue;
			}
			TaskInfo task = TaskInfoCreator.create(result, workunit, pi, app, mFormatter);
			mTasks.put(task.taskName, task);
			if (result.active_task) {
				// This is also active task
				mActiveTasks.add(result.name);
			}
		}
		if (Logging.DEBUG) Log.d(TAG, "dataSetTasks(): End update");
	}

	private void dataSetTransfers(Vector<Transfer> transfers) {
		if (Logging.DEBUG) Log.d(TAG, "dataSetTransfers(): Begin update");
		mTransfers.clear();
		Iterator<Transfer> ti = transfers.iterator();
		while (ti.hasNext()) {
			Transfer transfer = ti.next();
			ProjectInfo proj = mProjects.get(transfer.project_url);
			if (proj == null) {
				if (Logging.WARNING) Log.w(TAG, "No project for WU=" + transfer.name + " (project_url: " + transfer.project_url + "), setting dummy");
				proj = new ProjectInfo();
				proj.project = "???";
			}
			TransferInfo transferInfo = TransferInfoCreator.create(transfer, proj.project, mFormatter);
			mTransfers.add(transferInfo);
		}
		if (Logging.DEBUG) Log.d(TAG, "dataSetTransfers(): End update");
	}

	private boolean dataUpdateTasks(Vector<Result> results) {
		if (Logging.DEBUG) Log.d(TAG, "dataUpdateTasks(): Begin update");
		// Auxiliary set, to know which tasks were updated and which not
		Set<String> oldTaskNames = new HashSet<String>(mTasks.keySet());
		mActiveTasks.clear(); // We will build new record of active tasks
		// Parse results to set the tasks data
		Iterator<Result> ri = results.iterator();
		while (ri.hasNext()) {
			Result result = ri.next();
			TaskInfo task = (TaskInfo)mTasks.get(result.name);
			if (task == null) {
				// Maybe new workunit wad downloaded meanwhile, so we have
				// its result part, but not workunit part
				if (Logging.DEBUG) Log.d(TAG, "Task not found while trying dataUpdateTasks() - needs full updateCcState() update");
				return false;
			}
			TaskInfoCreator.update(task, result, mFormatter);
			if (result.active_task) {
				// This is also active task
				mActiveTasks.add(result.name);
			}
			// We updated this task - remove it from auxiliary set
			oldTaskNames.remove(result.name);
		}
		// We updated all entries in mTasks, which were in results
		// But, there could still be some obsolete tasks in mTasks
		// e.g. those uploaded and reported successfully
		// We should remove them now
		if (oldTaskNames.size() > 0) {
			if (Logging.DEBUG) Log.d(TAG, "dataUpdateTasks(): " + oldTaskNames.size() + " obsolete tasks detected");
			Iterator<String> it = oldTaskNames.iterator();
			while (it.hasNext()) {
				String obsoleteName = it.next();
				mTasks.remove(obsoleteName);
				if (Logging.DEBUG) Log.d(TAG, "dataUpdateTasks(): removed " + obsoleteName);
			}
		}
		if (Logging.DEBUG) Log.d(TAG, "dataUpdateTasks(): End update");
		return true;
	}

	private void dataUpdateMessages(Vector<Message> messages) {
		if (messages == null) return;
		if (Logging.DEBUG) Log.d(TAG, "dataUpdateMessages(): Begin update");
		Iterator<Message> mi = messages.iterator();
		while (mi.hasNext()) {
			edu.berkeley.boinc.lite.Message msg = mi.next();
			MessageInfo message = MessageInfoCreator.create(msg, mFormatter);
			mMessages.put(msg.seqno, message);
		}
		if (Logging.DEBUG) Log.d(TAG, "dataUpdateMessages(): End update");
	}

	private final Vector<ProjectInfo> getProjects() {
		return new Vector<ProjectInfo>(mProjects.values());
	}

	private final Vector<TaskInfo> getTasks() {
		return new Vector<TaskInfo>(mTasks.values());
	}

	private final Vector<TransferInfo> getTransfers() {
		return new Vector<TransferInfo>(mTransfers);
	}

	private final Vector<MessageInfo> getMessages() {
		return new Vector<MessageInfo>(mMessages.values());
	}
}
