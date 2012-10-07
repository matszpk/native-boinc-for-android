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
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;
import java.util.ArrayList;

import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.bridge.ClientBridge.ReplyHandler;
import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.ClientReceiver;
import sk.boinc.nativeboinc.clientconnection.HostInfo;
import sk.boinc.nativeboinc.clientconnection.MessageInfo;
import sk.boinc.nativeboinc.clientconnection.ModeInfo;
import sk.boinc.nativeboinc.clientconnection.NoticeInfo;
import sk.boinc.nativeboinc.clientconnection.PollOp;
import sk.boinc.nativeboinc.clientconnection.ProjectInfo;
import sk.boinc.nativeboinc.clientconnection.TaskDescriptor;
import sk.boinc.nativeboinc.clientconnection.TaskInfo;
import sk.boinc.nativeboinc.clientconnection.TransferDescriptor;
import sk.boinc.nativeboinc.clientconnection.TransferInfo;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Debugging;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.debug.NetStats;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.StringUtil;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.util.Log;
import edu.berkeley.boinc.lite.AccountIn;
import edu.berkeley.boinc.lite.AccountMgrInfo;
import edu.berkeley.boinc.lite.AccountMgrRPCReply;
import edu.berkeley.boinc.lite.AccountOut;
import edu.berkeley.boinc.lite.App;
import edu.berkeley.boinc.lite.CcState;
import edu.berkeley.boinc.lite.CcStatus;
import edu.berkeley.boinc.lite.GlobalPreferences;
import edu.berkeley.boinc.lite.Message;
import edu.berkeley.boinc.lite.Notice;
import edu.berkeley.boinc.lite.Notices;
import edu.berkeley.boinc.lite.Project;
import edu.berkeley.boinc.lite.ProjectAttachReply;
import edu.berkeley.boinc.lite.ProjectConfig;
import edu.berkeley.boinc.lite.ProjectListEntry;
import edu.berkeley.boinc.lite.Result;
import edu.berkeley.boinc.lite.RpcClient;
import edu.berkeley.boinc.lite.Transfer;
import edu.berkeley.boinc.lite.Workunit;


public class ClientBridgeWorkerHandler extends Handler {
	private static final String TAG = "ClientBridgeWorkerHandler";

	private static final int MESSAGE_INITIAL_LIMIT = 50;
	
	private static final int TIMEOUT = 60000;	// in milliseconds

	private ClientBridge.ReplyHandler mReplyHandler = null; // write in UI thread only
	private Context mContext = null;
	private Boolean mDisconnecting = false; // read by worker thread, write by both threads

	private RpcClient mRpcClient = null; // read/write only by worker thread 
	private NetStats mNetStats = null;
	private Formatter mFormatter = null;

	private boolean mPreviousStateOfIsWorking = false;
	private boolean mHandlerIsWorking = false;
	
	private Object mUpdateCancelSync = new Object();
	private int mUpdateCancelMask = 0;

	private VersionInfo mClientVersion = null;
	private Map<String, ProjectInfo> mProjects = new HashMap<String, ProjectInfo>();
	private Map<String, App> mApps = new HashMap<String, App>();
	private Map<String, Workunit> mWorkunits = new HashMap<String, Workunit>();
	private Map<String, TaskInfo> mTasks = new HashMap<String, TaskInfo>();
	private Set<String> mActiveTasks = new HashSet<String>();
	private ArrayList<TransferInfo> mTransfers = new ArrayList<TransferInfo>();
	private SortedMap<Integer, MessageInfo> mMessages = new TreeMap<Integer, MessageInfo>();
	private SortedMap<Integer, NoticeInfo> mNotices = new TreeMap<Integer, NoticeInfo>();
	private boolean mInitialStateRetrieved = false;

	private boolean mHaveAti = false;
	private boolean mHaveCuda = false;
	
	public ClientBridgeWorkerHandler(ClientBridge.ReplyHandler replyHandler,
			final Context context, final NetStats netStats) {
		mReplyHandler = replyHandler;
		mContext = context;
		mNetStats = netStats;
		mFormatter = new Formatter(mContext);
	}

	public void cleanup() {
		mFormatter.cleanup();
		mFormatter = null;
		cancelPollOperations(PollOp.POLL_ALL_MASK);
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
	
	public synchronized boolean isWorking() {
		return mHandlerIsWorking || mAccountMgrRPCCall != null || mLookupAccountCall != null ||
				mCreateAccountCall != null || mProjectAttachCall != null ||
				mGetProjectConfigCall != null;
	}

	private void closeConnection() {
		if (mRpcClient != null) {
			cancelPollOperations(PollOp.POLL_ALL_MASK);
			
			mRpcClient.close();
			mRpcClient = null;
			if (Logging.DEBUG) Log.d(TAG, "Connection closed");
		}
	}

	private class ConnectionAliveChecker extends Thread {
		private boolean mIsConnectionAlive = false;
		
		@Override
		public void run() {
			mIsConnectionAlive = mRpcClient.connectionAlive();
		}
		
		public boolean isConnectionAlive() {
			return mIsConnectionAlive;
		}
	}
	
	protected void rpcFailed() {
		// check whether connection alive
		ConnectionAliveChecker aliveChecker = new ConnectionAliveChecker();
		aliveChecker.start();
		if (Logging.DEBUG) Log.d(TAG, "Checking connection alive");
		try {
			aliveChecker.join(3000);
		} catch (InterruptedException e) { }
		
		if (Logging.DEBUG) Log.d(TAG, "Checking connection alive interrupt");
		aliveChecker.interrupt();
		
		try {
			aliveChecker.join();
		} catch (InterruptedException e) { }
		
		if (!aliveChecker.isConnectionAlive()) {
			// if not
			notifyDisconnected(false);
			closeConnection();
		}
	}
	
	private synchronized void changeIsHandlerWorking(boolean isWorking) {
		mHandlerIsWorking = isWorking;
		notifyChangeOfIsWorking();
	}


	public void connect(ClientId client, boolean retrieveInitialData) {
		if (mDisconnecting) return;  // already in disconnect phase
		
		changeIsHandlerWorking(true);
		
		if (Logging.DEBUG) Log.d(TAG, "Opening connection to " + ((client != null) ? client.getNickname() : "(null)"));
		notifyProgress(BoincOp.Connect, ClientReceiver.PROGRESS_CONNECTING);
		mRpcClient = new RpcClient(mNetStats);
		if (!mRpcClient.open(client.getAddress(), client.getPort())) {
			// Connect failed
			if (Logging.WARNING) Log.w(TAG, "Failed connect to " + client.getAddress() + ":" + client.getPort());
			mRpcClient = null;
			notifyDisconnected(false);
			changeIsHandlerWorking(false);
			return;
		}
		if (Debugging.INSERT_DELAYS) { try { Thread.sleep(1000); } catch (InterruptedException e) {} }
		String password = client.getPassword();
		if (!password.equals("")) {
			// Password supplied, we need to authorize
			if (mDisconnecting) return;  // already in disconnect phase
			notifyProgress(BoincOp.Connect, ClientReceiver.PROGRESS_AUTHORIZATION_PENDING);
			if (!mRpcClient.authorize(password)) {
				// Authorization failed
				if (Logging.WARNING) Log.w(TAG, "Authorization failed for " + client.getAddress() + ":" + client.getPort());
				notifyDisconnected(false);
				closeConnection();
				changeIsHandlerWorking(false);
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
			notifyProgress(BoincOp.Connect, ClientReceiver.PROGRESS_INITIAL_DATA);
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
				notifyDisconnected(false);
				closeConnection();
				changeIsHandlerWorking(false);
				return;
			}
			if (mDisconnecting) {
				changeIsHandlerWorking(false);
				return;  // already in disconnect phase
			}
			mClientVersion = VersionInfoCreator.create(ccState.version_info);
		}
		notifyConnected(mClientVersion);
		changeIsHandlerWorking(false);
	}

	public void disconnect() {
		// Disconnect request
		// This is not run in the worker thread, but in UI thread
		// Therefore we will not directly operate mRpcClient here, 
		// as it could be just in use by worker thread
		if (Logging.DEBUG) Log.d(TAG, "disconnect()");
		changeIsHandlerWorking(true);
		notifyDisconnected(true);
		// Now, trigger socket closing (to be done by worker thread)
		this.post(new Runnable() {
			@Override
			public void run() {
				closeConnection();
			}
		});
	}

	public void cancelPendingUpdates(final int refreshType) {
		// This is run in UI thread - immediately add callback to list,
		// so worker thread will not run update for this callback afterwards 
		synchronized (mUpdateCancelSync) {
			mUpdateCancelMask |= 1<<refreshType;
		}
		// Put removal of callback at the end of queue. So only currently pending
		// updates (these already in queue) will be canceled. Any later updates
		// for the same calback will be again processed normally
		this.post(new Runnable() {
			@Override
			public void run() {
				synchronized (mUpdateCancelSync) {
					// remove
					mUpdateCancelMask &= ~(1<<refreshType);
				}
			}
		});
	}

	public void updateClientMode(boolean runInternally) {
		if (mDisconnecting) return;  // already in disconnect phase
		synchronized (mUpdateCancelSync) {
			if ((mUpdateCancelMask & (1<<AutoRefresh.CLIENT_MODE)) != 0) {
				// This update was canceled meanwhile
				if (Logging.DEBUG) Log.d(TAG, "Canceled updateClientMode()");
				notifyCancelUpdate(BoincOp.UpdateClientMode);
				return;
			}
		}
		changeIsHandlerWorking(true);
		
		if (runInternally)
			notifyOperationBegin(BoincOp.UpdateClientMode);
		notifyProgress(BoincOp.UpdateClientMode, ClientReceiver.PROGRESS_XFER_STARTED);
		CcStatus ccStatus = mRpcClient.getCcStatus();
		if (ccStatus == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in updateClientMode()");
			notifyError(BoincOp.UpdateClientMode, 0, mContext.getString(R.string.updateClientModeError));
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		final ModeInfo clientMode = ModeInfoCreator.create(ccStatus);
		// Finally, send reply back to the calling thread (that is UI thread)
		updatedClientMode(clientMode);
		notifyProgress(BoincOp.UpdateClientMode, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}

	public void updateHostInfo() {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.UpdateHostInfo, ClientReceiver.PROGRESS_XFER_STARTED);
		edu.berkeley.boinc.lite.HostInfo boincHostInfo = mRpcClient.getHostInfo();
		if (boincHostInfo == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in updateHostInfo()");
			notifyError(BoincOp.UpdateHostInfo, 0, mContext.getString(R.string.boincOperationError));
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		final HostInfo hostInfo = HostInfoCreator.create(boincHostInfo, mFormatter);
		// Finally, send reply back to the calling thread (that is UI thread)
		updatedHostInfo(hostInfo);
		notifyProgress(BoincOp.UpdateHostInfo, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}

	public void updateProjects(boolean runInternally) {
		if (mDisconnecting) return;  // already in disconnect phase
		synchronized (mUpdateCancelSync) {
			if ((mUpdateCancelMask & (1<<AutoRefresh.PROJECTS)) != 0) {
				// This update was canceled meanwhile
				if (Logging.DEBUG) Log.d(TAG, "Canceled updateProjects()");
				notifyCancelUpdate(BoincOp.UpdateProjects);
				return;
			}
		}
		changeIsHandlerWorking(true);
		if (runInternally)
			notifyOperationBegin(BoincOp.UpdateProjects);
		
		notifyProgress(BoincOp.UpdateProjects, ClientReceiver.PROGRESS_XFER_STARTED);
		ArrayList<Project> projects = mRpcClient.getProjectStatus();
		if (projects == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in updateProjects()");
			notifyError(BoincOp.UpdateProjects, 0, mContext.getString(R.string.boincOperationError));
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		/* disk usage */
		if (!mRpcClient.getDiskUsage(projects)) {
			/* if failed */
			if (Logging.INFO) Log.i(TAG, "RPC failed in getDiskUsage()");
			notifyError(BoincOp.UpdateProjects, 0, mContext.getString(R.string.boincOperationError));
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		
		dataSetProjects(projects);
		updatedProjects(getProjects());
		notifyProgress(BoincOp.UpdateProjects, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}

	public void updateTasks(boolean runInternally) {
		if (mDisconnecting) return;  // already in disconnect phase
		synchronized (mUpdateCancelSync) {
			if ((mUpdateCancelMask & (1<<AutoRefresh.TASKS)) != 0) {
				// This update was canceled meanwhile
				if (Logging.DEBUG) Log.d(TAG, "Canceled updateTasks()");
				notifyCancelUpdate(BoincOp.UpdateTasks);
				return;
			}
		}
		
		if (Logging.DEBUG) Log.d(TAG, "run updateTasks()");
		changeIsHandlerWorking(true);
		if (runInternally)
			notifyOperationBegin(BoincOp.UpdateTasks);
		
		notifyProgress(BoincOp.UpdateTasks, ClientReceiver.PROGRESS_XFER_STARTED);
		boolean updateFinished = false;
		ArrayList<Result> results;
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
				notifyError(BoincOp.UpdateTasks, 0, mContext.getString(R.string.boincOperationError));
				rpcFailed();
				changeIsHandlerWorking(false);
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
		
		updatedTasks(getTasks());
		notifyProgress(BoincOp.UpdateTasks, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}

	public void updateTransfers(boolean runInternally) {
		if (mDisconnecting) return;  // already in disconnect phase
		synchronized (mUpdateCancelSync) {
			if ((mUpdateCancelMask & (1<<AutoRefresh.TRANSFERS)) != 0) {
				// This update was canceled meanwhile
				if (Logging.DEBUG) Log.d(TAG, "Canceled updateTransfers()");
				notifyCancelUpdate(BoincOp.UpdateTransfers);
				return;
			}
		}
		changeIsHandlerWorking(true);
		if (runInternally)
			notifyOperationBegin(BoincOp.UpdateTransfers);
		
		notifyProgress(BoincOp.UpdateTransfers, ClientReceiver.PROGRESS_XFER_STARTED);
		ArrayList<Transfer> transfers = mRpcClient.getFileTransfers();
		if (transfers == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in updateTransfers()");
			notifyError(BoincOp.UpdateTransfers, 0, mContext.getString(R.string.boincOperationError));
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		dataSetTransfers(transfers);
		updatedTransfers(getTransfers());
		notifyProgress(BoincOp.UpdateTransfers, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}

	public void updateMessages(boolean runInternally) {
		if (mDisconnecting) return;  // already in disconnect phase
		synchronized (mUpdateCancelSync) {
			if ((mUpdateCancelMask & (1<<AutoRefresh.MESSAGES)) != 0) {
				// This update was canceled meanwhile
				if (Logging.DEBUG) Log.d(TAG, "Canceled updateMessages()");
				notifyCancelUpdate(BoincOp.UpdateMessages);
				return;
			}
		}
		changeIsHandlerWorking(true);
		if (runInternally)
			notifyOperationBegin(BoincOp.UpdateMessages);
		
		notifyProgress(BoincOp.UpdateMessages, ClientReceiver.PROGRESS_XFER_STARTED);
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
		if (mDisconnecting) {
			changeIsHandlerWorking(false);
			return;  // already in disconnect phase
		}
		ArrayList<Message> messages = mRpcClient.getMessages(reqSeqno);
		if (messages == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in updateMessages()");
			notifyError(BoincOp.UpdateMessages, 0, mContext.getString(R.string.boincOperationError));
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		dataUpdateMessages(messages);
		updatedMessages(getMessages());
		notifyProgress(BoincOp.UpdateMessages, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}
	
	public void updateNotices(boolean runInternally) {
		// TODO: do it
		if (mDisconnecting) return;  // already in disconnect phase
		synchronized (mUpdateCancelSync) {
			if ((mUpdateCancelMask & (1<<AutoRefresh.NOTICES)) != 0) {
				// This update was canceled meanwhile
				if (Logging.DEBUG) Log.d(TAG, "Canceled updateNotices()");
				notifyCancelUpdate(BoincOp.UpdateNotices);
				return;
			}
		}
		changeIsHandlerWorking(true);
		if (runInternally)
			notifyOperationBegin(BoincOp.UpdateNotices);
		
		notifyProgress(BoincOp.UpdateNotices, ClientReceiver.PROGRESS_XFER_STARTED);
		int reqSeqno = (mNotices.isEmpty()) ? 0 : mNotices.lastKey();
		if (mDisconnecting) {
			changeIsHandlerWorking(false);
			return;  // already in disconnect phase
		}
		Notices notices = mRpcClient.getNotices(reqSeqno);
		if (notices == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in updateNotices()");
			notifyError(BoincOp.UpdateNotices, 0, mContext.getString(R.string.boincOperationError));
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		if (!notices.complete) {
			// do update
			dataUpdateNotices(notices.notices);
		}
		// always update notices
		updatedNotices(getNotices());
		notifyProgress(BoincOp.UpdateNotices, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}
	
	public void getAllProjectsList(boolean excludeAttachedProjects) {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.GetAllProjectList, ClientReceiver.PROGRESS_XFER_STARTED);
		
		ArrayList<ProjectListEntry> projects = mRpcClient.getAllProjectsList();
		if (projects == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in getAllProjectsList()");
			notifyError(BoincOp.GetAllProjectList, 0, mContext.getString(R.string.boincOperationError));
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		if (excludeAttachedProjects) { //
			updateProjects(true);
			
			ArrayList<ProjectListEntry> filteredProjects = new ArrayList<ProjectListEntry>();
			for (ProjectListEntry projectEntry: projects)
				if (!mProjects.containsKey(projectEntry.url)) // if not attached
					filteredProjects.add(projectEntry);
			
			// return filtered projects
			currentAllProjectsList(filteredProjects);
		} else
			currentAllProjectsList(projects);
		notifyProgress(BoincOp.GetAllProjectList, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}

	public void getBAMInfo() {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.GetBAMInfo, ClientReceiver.PROGRESS_XFER_STARTED);
		AccountMgrInfo accountMgrInfo = mRpcClient.getAccountMgrInfo();
		if (accountMgrInfo == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in getBAMInfo()");
			notifyError(BoincOp.GetBAMInfo, 0, mContext.getString(R.string.boincOperationError));
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		currentBAMInfo(accountMgrInfo);
		notifyProgress(BoincOp.GetBAMInfo, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}
	
	private static class AccountMgrRPCCall {
		public int operation;
		public String name;
		public String url;
		public String password;
		public boolean useConfigFile;
		public String infoMsg;
		public long startTime;
		
		public AccountMgrRPCCall(int operation, String name, String url,
				String password, boolean useConfigFile, String infoMsg, long startTime) {
			this.operation = operation;
			this.name = name;
			this.url = url;
			this.password = password;
			this.useConfigFile = useConfigFile;
			this.infoMsg = infoMsg;
			this.startTime = startTime;
		}
	}
	
	private AccountMgrRPCCall mAccountMgrRPCCall = null;
	
	/* return true if clear not null call */
	private synchronized boolean clearAccountMgrCall() {
		if (mAccountMgrRPCCall != null) {
			mAccountMgrRPCCall = null;
			notifyChangeOfIsWorking();
			return true;
		}
		return false;
	}
	
	private synchronized boolean isAccountMgrRPCCallCancelled() {
		return mAccountMgrRPCCall == null;
	}
	
	/**
	 * poller - polls in every second during performing operation
	 */
	private Runnable mAccountMgrRPCPoller = new Runnable() {
		@Override
		public void run() {
			AccountMgrRPCReply reply;
			AccountMgrRPCCall call = null;
			synchronized(ClientBridgeWorkerHandler.this) {
				if (mAccountMgrRPCCall == null) return;
				call = mAccountMgrRPCCall;
			}
			if (mDisconnecting) return;  // already in disconnect phase
			
			if (SystemClock.elapsedRealtime()-call.startTime >= TIMEOUT) { // time out
				if (Logging.INFO) Log.i(TAG, "RPC failed in " + call.infoMsg);
				if (clearAccountMgrCall())
					notifyError(BoincOp.SyncWithBAM, 0, mContext.getString(R.string.boincPollError));
				rpcFailed();
				return;
			}
			
			reply = mRpcClient.accountMgrRPCPoll();
			if (reply != null) {
				if (reply.error_num == RpcClient.ERR_IN_PROGRESS) {
					// try poll in next second
					if (Logging.DEBUG) Log.d(TAG, "polling mAccountMgr()");
					if (!isAccountMgrRPCCallCancelled())
						postDelayed(mAccountMgrRPCPoller, 1000);
				} else if (reply.error_num == RpcClient.ERR_RETRY) { // retry operation
					if (isAccountMgrRPCCallCancelled())
						return; // if cancelled
					
					if (!mRpcClient.accountMgrRPC(call.url, call.name, call.password,
							call.useConfigFile)) {
						if (Logging.INFO) Log.i(TAG, "RPC failed in " + call.infoMsg);
						
						if (clearAccountMgrCall())
							notifyError(BoincOp.SyncWithBAM, 0, mContext.getString(R.string.boincPollError));
						rpcFailed();
					} else {
						// try poll in next second
						if (Logging.DEBUG) Log.d(TAG, "polling mAccountMgr() (retry)");
						if (!isAccountMgrRPCCallCancelled())
							postDelayed(mAccountMgrRPCPoller, 1000);
					}
				} else {	// if other error
					if (reply.error_num != RpcClient.SUCCESS) {
						if (Logging.INFO) Log.i(TAG, "RPC failed in " + call.infoMsg);
						if (clearAccountMgrCall())
							notifyPollError(BoincOp.SyncWithBAM, reply.error_num, call.operation,
								StringUtil.joinString("\n", reply.messages), null);
					} else {
						// if success
						if (clearAccountMgrCall())
							notifyAfterAccountMgrRPC();
					}
				}
			} else {	// rpc failed
				if (Logging.INFO) Log.i(TAG, "RPC failed in " + call.infoMsg);
				if (clearAccountMgrCall())
					notifyError(BoincOp.SyncWithBAM, 0, mContext.getString(R.string.boincPollError));
				rpcFailed();
			}
		}
	};
	
	private void accountMgrRPC(int operation, String name, String url, String password,
			boolean useConfigFile, String infoMsg) {
		if (mDisconnecting) return;  // already in disconnect phase
		
		// do nothing if already run
		synchronized(this) {
			if (mAccountMgrRPCCall != null) return;
		}
		
		changeIsHandlerWorking(true);
		
		notifyProgress(BoincOp.SyncWithBAM, ClientReceiver.PROGRESS_XFER_STARTED);
		if (!mRpcClient.accountMgrRPC(url, name, password, useConfigFile)) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in " + infoMsg);
			notifyError(BoincOp.SyncWithBAM, 0, mRpcClient.getLastErrorMessage());
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		
		long startTime = SystemClock.elapsedRealtime();
		synchronized(this) {
			mAccountMgrRPCCall = new AccountMgrRPCCall(operation, name, url, password,
					useConfigFile, infoMsg, startTime);
		}
		postDelayed(mAccountMgrRPCPoller, 1000);
		notifyProgress(BoincOp.SyncWithBAM, ClientReceiver.PROGRESS_XFER_POLL);
		changeIsHandlerWorking(false);
	}
	
	public void attachToBAM(String name, String url, String password) {
		accountMgrRPC(PollOp.POLL_ATTACH_TO_BAM, name, url, password, false, "RPC failed in attachToBAM()");
	}
	
	public void synchronizeWithBAM() {
		accountMgrRPC(PollOp.POLL_SYNC_WITH_BAM, "", "", "", true,
				"RPC failed in synchorizeWithBAM()");
	}
	
	public void stopUsingBAM() {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.StopUsingBAM, ClientReceiver.PROGRESS_XFER_STARTED);
		boolean success = mRpcClient.accountMgrRPC("", "", "", false);
		
		if (!success) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in stopUsingBAM()");
			notifyError(BoincOp.StopUsingBAM, 0, mRpcClient.getLastErrorMessage());
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		
		notifyOperationFinish(BoincOp.StopUsingBAM);
		notifyProgress(BoincOp.StopUsingBAM, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}
	
	private static class LookupAccountCall {
		public AccountIn accountIn;
		public long startTime; 
		
		public LookupAccountCall(AccountIn accountIn, long startTime) {
			this.accountIn = accountIn;
			this.startTime = startTime;
		}
	}
	
	private LookupAccountCall mLookupAccountCall = null;
	
	/* return true if clears not cancelled task */
	private synchronized boolean clearLookupAccountCall() {
		if (mLookupAccountCall != null) {
			mLookupAccountCall = null;
			notifyChangeOfIsWorking();
			return true;
		}
		return false;
	}
	
	private synchronized boolean isLookupAccountCallCancelled() {
		return mLookupAccountCall == null;
	}
	
	/**
	 * poller - polls in every second during performing operation
	 */
	private Runnable mLookupAccountPoller = new Runnable() {
		@Override
		public void run() {
			LookupAccountCall call = null;
			synchronized(ClientBridgeWorkerHandler.this) {
				if (mLookupAccountCall == null) return;
				call = mLookupAccountCall;
			}
			if (mDisconnecting) return;  // already in disconnect phase
			
			final String projectUrl = call.accountIn.url;
			
			if (SystemClock.elapsedRealtime()-call.startTime >= TIMEOUT) { // time out
				if (Logging.INFO) Log.i(TAG, "RPC failed in lookupAccount() timeout");
				if (clearLookupAccountCall())
					notifyError(BoincOp.AddProject, 0, mContext.getString(R.string.boincPollError));
				rpcFailed();
				return;
			}
			
			AccountOut accountOut = mRpcClient.lookupAccountPoll();
			if (accountOut != null) {
				if (accountOut.error_num == RpcClient.ERR_IN_PROGRESS) {
					// try poll in next second
					if (Logging.DEBUG) Log.d(TAG, "polling lookupAccount()");
					
					if (!isLookupAccountCallCancelled())
						postDelayed(mLookupAccountPoller, 1000);
				} else if (accountOut.error_num == RpcClient.ERR_RETRY) { // retry operation
					if (isLookupAccountCallCancelled())
						return;	// if cancelled
					
					if (!mRpcClient.lookupAccount(call.accountIn)) {
						if (Logging.INFO) Log.i(TAG, "RPC failed in lookupAccount() retry");
						if (clearLookupAccountCall())
							notifyError(BoincOp.AddProject, 0, mContext.getString(R.string.boincPollError)); 
						rpcFailed();
					} else {
						// try poll in next second
						if (Logging.DEBUG) Log.d(TAG, "polling lookupAccount() (retry)");
						if (!isLookupAccountCallCancelled())
							postDelayed(mLookupAccountPoller, 1000);
					}
				} else {	// if other error
					if (accountOut.error_num != RpcClient.SUCCESS) {
						if (Logging.INFO) Log.i(TAG,
								"RPC failed in lookupAccount(): "+accountOut.error_num);
						if (clearLookupAccountCall())
							notifyPollError(BoincOp.AddProject, accountOut.error_num, PollOp.POLL_LOOKUP_ACCOUNT,
								accountOut.error_msg, projectUrl);
					} else {
						// if success
						if (clearLookupAccountCall())
							currentAuthCode(projectUrl, accountOut.authenticator);
					}
				}
			} else {	// rpc failed
				if (Logging.INFO) Log.i(TAG, "RPC failed in lookupAccount() (null)");
				if (clearLookupAccountCall())
					notifyError(BoincOp.AddProject, 0, mContext.getString(R.string.boincPollError));
				rpcFailed();
			}
		}
	};
	
	public void lookupAccount(AccountIn accountIn) {
		if (mDisconnecting) return;  // already in disconnect phase
		
		// do nothing if already run
		synchronized(this) {
			if (mLookupAccountCall != null) return;
		}
		
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.AddProject, ClientReceiver.PROGRESS_XFER_STARTED);
		if (!mRpcClient.lookupAccount(accountIn)) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in lookupAccount()");
			notifyError(BoincOp.AddProject, 0, mRpcClient.getLastErrorMessage());
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		notifyProgress(BoincOp.AddProject, ClientReceiver.PROGRESS_XFER_POLL);
		
		long startTime = SystemClock.elapsedRealtime();
		synchronized(this) {
			mLookupAccountCall = new LookupAccountCall(accountIn, startTime);
		}
		
		postDelayed(mLookupAccountPoller, 1000);
		changeIsHandlerWorking(false);
	}
	
	private static class CreateAccountCall {
		public AccountIn accountIn;
		public long startTime;
		
		public CreateAccountCall(AccountIn accountIn, long startTime) {
			this.accountIn = accountIn;
			this.startTime = startTime;
		}
	}
	
	private CreateAccountCall mCreateAccountCall = null;
	
	/* return true if clears not cancelled task */
	private synchronized boolean clearCreateAccountCall() {
		if (mCreateAccountCall != null) {
			mCreateAccountCall = null;
			notifyChangeOfIsWorking();
			return true;
		} 
		return false;
	}
	
	private synchronized boolean isCreateAccountCallCancelled() {
		return mCreateAccountCall == null;
	}
	
	/**
	 * poller - polls in every second during performing operation
	 */
	private Runnable mCreateAccountPoller = new Runnable() {
		@Override
		public void run() {
			CreateAccountCall call = null;
			synchronized(ClientBridgeWorkerHandler.this) {
				if (mCreateAccountCall == null) return;
				call = mCreateAccountCall;
			}
			if (mDisconnecting) return;  // already in disconnect phase
			
			final String projectUrl = call.accountIn.url;
			
			if (SystemClock.elapsedRealtime()-call.startTime >= TIMEOUT) { // time out
				if (Logging.INFO) Log.i(TAG, "RPC failed in createAccount()");
				if (clearCreateAccountCall())
					notifyError(BoincOp.AddProject, 0, mContext.getString(R.string.boincPollError));
				rpcFailed();
				return;
			}
			
			AccountOut accountOut = mRpcClient.createAccountPoll();
			if (accountOut != null) {
				if (accountOut.error_num == RpcClient.ERR_IN_PROGRESS) {
					// try poll in next second
					if (Logging.DEBUG) Log.d(TAG, "polling createAccount()");
					if (!isCreateAccountCallCancelled())
						postDelayed(mCreateAccountPoller, 1000);
				} else if (accountOut.error_num == RpcClient.ERR_RETRY) { // retry operation
					if (isCreateAccountCallCancelled())
						return; // if cancelled
					
					if (mRpcClient.createAccount(call.accountIn) == false) {
						if (Logging.INFO) Log.i(TAG, "RPC failed in createAccount()");
						
						if (clearCreateAccountCall())
							notifyError(BoincOp.AddProject, 0, mContext.getString(R.string.boincPollError));
						rpcFailed();
					} else {
						// try poll in next second
						if (Logging.DEBUG) Log.d(TAG, "polling createAccount() (retry)");
						if (!isCreateAccountCallCancelled())
							postDelayed(mCreateAccountPoller, 1000);
					}
				} else {	// if other error
					if (accountOut.error_num != RpcClient.SUCCESS) {
						if (Logging.INFO) Log.i(TAG, "RPC failed in createAccount()");
						if (clearCreateAccountCall())
							notifyPollError(BoincOp.AddProject, accountOut.error_num,
								PollOp.POLL_CREATE_ACCOUNT, accountOut.error_msg, projectUrl);
						
					} else {
						// if success
						if (clearCreateAccountCall())
							currentAuthCode(projectUrl, accountOut.authenticator);
					}
				}
			} else {	// rpc failed
				if (Logging.INFO) Log.i(TAG, "RPC failed in createAccount()");
				if (clearCreateAccountCall())
					notifyError(BoincOp.AddProject, 0, mContext.getString(R.string.boincPollError));
				rpcFailed();
			}
		}
	};
	
	public void createAccount(AccountIn accountIn) {
		if (mDisconnecting) return;  // already in disconnect phase
		
		// do nothing if already run
		synchronized(this) {
			if (mCreateAccountCall != null) return;
		}
		
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.AddProject, ClientReceiver.PROGRESS_XFER_STARTED);
		if (!mRpcClient.createAccount(accountIn)) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in createAccount()");
			notifyError(BoincOp.AddProject, 0, mRpcClient.getLastErrorMessage());
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		notifyProgress(BoincOp.AddProject, ClientReceiver.PROGRESS_XFER_POLL);
		
		long startTime = SystemClock.elapsedRealtime();
		synchronized(this) {
			mCreateAccountCall = new CreateAccountCall(accountIn, startTime);
		}
		
		postDelayed(mCreateAccountPoller, 1000);
		changeIsHandlerWorking(false);
	}
	
	private static class ProjectAttachCall {
		public String url;
		public String authCode;
		public String projectName;
		public long startTime;
		
		public ProjectAttachCall(String url, String authCode, String projectName, long startTime) {
			this.url = url;
			this.authCode = authCode;
			this.projectName = projectName;
			this.startTime = startTime;
		}
	}
	
	private ProjectAttachCall mProjectAttachCall = null;

	/* return true if clears not cancelled task */
	private synchronized boolean clearProjectAttachCall() {
		if (mProjectAttachCall != null) {
			mProjectAttachCall = null;
			notifyChangeOfIsWorking();
			return true;
		}
		return false;
	}
	
	private synchronized boolean isProjectAttachCallCancelled() {
		return mProjectAttachCall == null;
	}
	
	/**
	 * poller - polls in every second during performing operation
	 */
	private Runnable mProjectAttachPoller = new Runnable() {
		@Override
		public void run() {
			ProjectAttachCall call = null;
			synchronized(ClientBridgeWorkerHandler.this) {
				if (mProjectAttachCall == null) return;
				call = mProjectAttachCall;
			}
			if (mDisconnecting) return;  // already in disconnect phase
			
			final String projectUrl = call.url;
			
			if (SystemClock.elapsedRealtime()-call.startTime >= TIMEOUT) { // time out
				if (Logging.INFO) Log.i(TAG, "RPC failed in projectAttach()");
				if (clearProjectAttachCall())
					notifyError(BoincOp.AddProject, 0, mContext.getString(R.string.boincPollError));
				rpcFailed();
				return;
			}
			
			ProjectAttachReply reply = mRpcClient.projectAttachPoll();
			if (reply != null) {
				if (reply.error_num == RpcClient.ERR_IN_PROGRESS) {
					// try poll in next second
					if (Logging.DEBUG) Log.d(TAG, "polling projectAttach()");
					if (!isProjectAttachCallCancelled())
						postDelayed(mProjectAttachPoller, 1000);
				} else if (reply.error_num == RpcClient.ERR_RETRY) { // retry operation
					if (isProjectAttachCallCancelled())
						return; // if cancelled
					
					if (mRpcClient.projectAttach(call.url, call.authCode, call.projectName) == false) {
						if (Logging.INFO) Log.i(TAG, "RPC failed in projectAttach()");
						if (clearProjectAttachCall())
							notifyError(BoincOp.AddProject, 0, mContext.getString(R.string.boincPollError));
						rpcFailed();
					} else {
						// try poll in next second
						if (Logging.DEBUG) Log.d(TAG, "polling projectAttach() (retry)");
						if (!isProjectAttachCallCancelled())
							postDelayed(mProjectAttachPoller, 1000);
					}
				} else {	// if other error
					if (reply.error_num != RpcClient.SUCCESS) {
						if (Logging.INFO) Log.i(TAG, "RPC failed in projectAttach()");
						if (clearProjectAttachCall())
							notifyPollError(BoincOp.AddProject, reply.error_num, PollOp.POLL_PROJECT_ATTACH,
								null, projectUrl);
						
					} else {
						// if success
						if (clearProjectAttachCall())
							notifyAfterProjectAttach(projectUrl);
					}
				}
			} else {	// rpc failed
				if (Logging.INFO) Log.i(TAG, "RPC failed in projectAttach()");
				if (clearProjectAttachCall())
					notifyError(BoincOp.AddProject, 0, mContext.getString(R.string.boincPollError));
				rpcFailed();
			}
		}
	};
	
	public void projectAttach(String url, String authCode, String projectName) {
		if (mDisconnecting) return;  // already in disconnect phase
		
		// do nothing if already run
		synchronized(this) {
			if (mProjectAttachCall != null) return;
		}
		
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.AddProject, ClientReceiver.PROGRESS_XFER_STARTED);
		if (!mRpcClient.projectAttach(url, authCode, projectName)) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in projectAttach()");
			notifyError(BoincOp.AddProject, 0, mRpcClient.getLastErrorMessage());
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		notifyProgress(BoincOp.AddProject, ClientReceiver.PROGRESS_XFER_POLL);
		
		long startTime = SystemClock.elapsedRealtime();
		synchronized(this) {
			mProjectAttachCall = new ProjectAttachCall(url, authCode, projectName, startTime);
		}
		postDelayed(mProjectAttachPoller, 1000);
		changeIsHandlerWorking(false);
	}
	
	private static class GetProjectConfigCall {
		public String url;
		public long startTime;
		
		public GetProjectConfigCall(String url, long startTime) {
			this.url = url;
			this.startTime = startTime;
		}
	}
	
	private GetProjectConfigCall mGetProjectConfigCall = null;
	
	/* return true if clears not cancelled task */
	private synchronized boolean clearGetProjectConfigCall() {
		if (mGetProjectConfigCall != null) {
			mGetProjectConfigCall = null;
			notifyChangeOfIsWorking();
			return true;
		}
		return false;
	}
	
	private synchronized boolean isGetProjectConfigCallCancelled() {
		return mGetProjectConfigCall == null;
	}
	
	/**
	 * poller - polls in every second during performing operation
	 */
	private Runnable mGetProjectConfigPoller = new Runnable() {
		@Override
		public void run() {
			GetProjectConfigCall call = null;
			synchronized(ClientBridgeWorkerHandler.this) {
				if (mGetProjectConfigCall == null) return;
				call = mGetProjectConfigCall;
			}
			if (mDisconnecting) return;  // already in disconnect phase
			
			final String projectUrl = call.url;
			
			if (SystemClock.elapsedRealtime()-call.startTime >= TIMEOUT) { // time out
				if (Logging.INFO) Log.i(TAG, "RPC failed in getProjectConfig()");
				
				if (clearGetProjectConfigCall())
					notifyError(BoincOp.GetProjectConfig, 0, mContext.getString(R.string.boincPollError));
				rpcFailed();
				return;
			}
			
			ProjectConfig projectConfig = mRpcClient.getProjectConfigPoll();
			if (projectConfig != null) {
				if (projectConfig.error_num == RpcClient.ERR_IN_PROGRESS) {
					// try poll in next second
					if (Logging.DEBUG) Log.d(TAG, "polling getProjectConfig()");
					if (!isGetProjectConfigCallCancelled())
						postDelayed(mGetProjectConfigPoller, 1000);
				} else if (projectConfig.error_num == RpcClient.ERR_RETRY) { // retry operation
					if (isGetProjectConfigCallCancelled())
						return; // cancelled
					
					if (mRpcClient.getProjectConfig(projectUrl) == false) {
						if (Logging.INFO) Log.i(TAG, "RPC failed in getProjectConfig()");
						if (clearGetProjectConfigCall())
							notifyError(BoincOp.GetProjectConfig, 0, mContext.getString(R.string.boincPollError));
						rpcFailed();
					} else {
						// try poll in next second
						if (Logging.DEBUG) Log.d(TAG, "polling getProjectConfig() (retry)");
						if (!isGetProjectConfigCallCancelled())
							postDelayed(mGetProjectConfigPoller, 1000);
					}
				} else {	// if other error
					if (projectConfig.error_num != RpcClient.SUCCESS) {
						if (Logging.INFO) Log.i(TAG, "RPC failed in getProjectConfig()");
						if (clearGetProjectConfigCall())
							notifyPollError(BoincOp.GetProjectConfig, projectConfig.error_num,
								PollOp.POLL_PROJECT_CONFIG, projectConfig.error_msg, projectUrl);
					} else {
						// if success
						if (clearGetProjectConfigCall())
							currentProjectConfig(projectUrl, projectConfig);
					}
				}
			} else {	// rpc failed
				if (Logging.INFO) Log.i(TAG, "RPC failed in getProjectConfig()");
				if (clearGetProjectConfigCall())
					notifyError(BoincOp.GetProjectConfig, 0, mContext.getString(R.string.boincPollError));
				rpcFailed();
			}
		}
	};
	
	public void getProjectConfig(String url) {
		if (mDisconnecting) return;  // already in disconnect phase
		
		// do nothing if already run
		synchronized(this) {
			if (mGetProjectConfigCall != null) return;
		}
		
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.GetProjectConfig, ClientReceiver.PROGRESS_XFER_STARTED);
		if (!mRpcClient.getProjectConfig(url)) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in projectConfig()");
			notifyError(BoincOp.GetProjectConfig, 0, mRpcClient.getLastErrorMessage());
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		
		notifyProgress(BoincOp.GetProjectConfig, ClientReceiver.PROGRESS_XFER_POLL);
		
		long startTime = SystemClock.elapsedRealtime();
		synchronized(this) {
			mGetProjectConfigCall = new GetProjectConfigCall(url, startTime);
		}
		postDelayed(mGetProjectConfigPoller, 1000);
		changeIsHandlerWorking(false);
	}
	
	/**
	 * cancel all poll operations
	 */
	public synchronized void cancelPollOperations(int opFlags) {
		if (Logging.DEBUG) Log.d(TAG, String.format("Cancel poll ops:0x%02x", opFlags));
		
		int destOpFlags = 0;
		
		if ((opFlags & PollOp.POLL_BAM_OPERATION_MASK) != 0 && mAccountMgrRPCCall != null) {
			mAccountMgrRPCCall = null;
			removeCallbacks(mAccountMgrRPCPoller);
			destOpFlags |= PollOp.POLL_BAM_OPERATION_MASK;
		}
		if ((opFlags & PollOp.POLL_CREATE_ACCOUNT_MASK) != 0 && mCreateAccountCall != null) {
			mCreateAccountCall = null;
			removeCallbacks(mLookupAccountPoller);
			destOpFlags |= PollOp.POLL_CREATE_ACCOUNT_MASK;
		}
		if ((opFlags & PollOp.POLL_LOOKUP_ACCOUNT_MASK) != 0 && mLookupAccountCall != null) {
			mLookupAccountCall = null;
			removeCallbacks(mCreateAccountPoller);
			destOpFlags |= PollOp.POLL_LOOKUP_ACCOUNT_MASK;
		}
		if ((opFlags & PollOp.POLL_PROJECT_ATTACH_MASK) != 0 && mProjectAttachCall != null) {
			mProjectAttachCall = null;
			removeCallbacks(mProjectAttachPoller);
			destOpFlags |= PollOp.POLL_PROJECT_ATTACH_MASK;
		}
		if ((opFlags & PollOp.POLL_PROJECT_CONFIG_MASK) != 0 && mGetProjectConfigCall != null) {
			mGetProjectConfigCall = null;
			removeCallbacks(mGetProjectConfigPoller);
			destOpFlags |= PollOp.POLL_PROJECT_CONFIG_MASK;
		}
		
		notifyChangeOfIsWorking();
		
		if (destOpFlags != 0)
			notifyCancelPollOperations(destOpFlags);
	}
	
	public void getGlobalPrefsWorking() {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		if (Logging.DEBUG) Log.d(TAG, "Begin Get globalprefs");
		notifyProgress(BoincOp.GlobalPrefsWorking, ClientReceiver.PROGRESS_XFER_STARTED);
		
		GlobalPreferences globalPrefs = mRpcClient.getGlobalPrefsWorkingStruct();
		if (globalPrefs == null) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in getGlobalPrefsWorking()");
			notifyError(BoincOp.GlobalPrefsWorking, 0, mContext.getString(R.string.boincOperationError));
			rpcFailed();
			if (Logging.DEBUG) Log.d(TAG, "Finish Get globalprefs with error");
			changeIsHandlerWorking(false);
			return;
		}
		currentGlobalPreferences(globalPrefs);
		if (Logging.DEBUG) Log.d(TAG, "Finish Get globalprefs");
		notifyProgress(BoincOp.GlobalPrefsWorking, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}
	
	public void setGlobalPrefsOverride(String globalPrefs) {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.GlobalPrefsOverride, ClientReceiver.PROGRESS_XFER_STARTED);
		boolean success = mRpcClient.setGlobalPrefsOverride(globalPrefs);
		
		if (!success) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in setGlobalPrefsOverride()");
			notifyError(BoincOp.GlobalPrefsOverride, 0, mContext.getString(R.string.boincOperationError));
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		} 
		
		success = mRpcClient.readGlobalPrefsOverride();
		if (!success) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in setGlobalPrefsOverride()");
			notifyError(BoincOp.GlobalPrefsOverride, 0, mRpcClient.getLastErrorMessage());
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		notifyGlobalPrefsChanged();
		notifyProgress(BoincOp.GlobalPrefsOverride, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}
	
	public void setGlobalPrefsOverrideStruct(GlobalPreferences globalPrefs) {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.GlobalPrefsOverride, ClientReceiver.PROGRESS_XFER_STARTED);
		boolean success = mRpcClient.setGlobalPrefsOverrideStruct(globalPrefs);
		notifyProgress(BoincOp.GlobalPrefsOverride, ClientReceiver.PROGRESS_XFER_FINISHED);
		
		if (!success) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in setGlobalPrefsOverride()");
			notifyError(BoincOp.GlobalPrefsOverride, 0, mContext.getString(R.string.boincOperationError));
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		}
		
		success = mRpcClient.readGlobalPrefsOverride();
		if (!success) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in setGlobalPrefsOverride()");
			notifyError(BoincOp.GlobalPrefsOverride, 0, mRpcClient.getLastErrorMessage());
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		} 
		notifyGlobalPrefsChanged();
		notifyProgress(BoincOp.GlobalPrefsOverride, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
	}
	
	public void runBenchmarks() {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.RunBenchmarks, ClientReceiver.PROGRESS_XFER_STARTED);
		boolean success = mRpcClient.runBenchmarks();
		if (!success) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in runBenchmarks()");
			notifyProgress(BoincOp.RunBenchmarks, ClientReceiver.PROGRESS_XFER_FINISHED);
			notifyError(BoincOp.RunBenchmarks, 0, mRpcClient.getLastErrorMessage());
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		} else { // if ok
			notifyOperationFinish(BoincOp.RunBenchmarks);
			notifyProgress(BoincOp.RunBenchmarks, ClientReceiver.PROGRESS_XFER_FINISHED);
		}
		changeIsHandlerWorking(false);
	}

	public void setRunMode(int mode) {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.SetRunMode, ClientReceiver.PROGRESS_XFER_STARTED);
		mRpcClient.setRunMode(mode, 0);
		notifyOperationFinish(BoincOp.SetRunMode);
		notifyProgress(BoincOp.SetRunMode, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
		// Regardless of success we run update of client mode
		// If there is problem with socket, it will be handled there
		updateClientMode(true);
	}

	public void setNetworkMode(int mode) {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.SetNetworkMode, ClientReceiver.PROGRESS_XFER_STARTED);
		mRpcClient.setNetworkMode(mode, 0);
		notifyOperationFinish(BoincOp.SetNetworkMode);
		notifyProgress(BoincOp.SetNetworkMode, ClientReceiver.PROGRESS_XFER_FINISHED);
		changeIsHandlerWorking(false);
		// Regardless of success we run update of client mode
		// If there is problem with socket, it will be handled there
		updateClientMode(true);
	}

	public void shutdownCore() {
		if (mDisconnecting) return;  // already in disconnect phase
		notifyProgress(BoincOp.ShutdownCore, ClientReceiver.PROGRESS_XFER_STARTED);
		mRpcClient.quit();
		// We have to check, whether we are really disconnected
		// We will try for 5 seconds only
		changeIsHandlerWorking(true);
		boolean connectionAlive = true;
		try {
			// First, give the other side a little time to close socket
			Thread.sleep(100);
			for (int i = 0; i < 5; ++i) {
				connectionAlive = mRpcClient.connectionAlive();
				if (!connectionAlive) {
					// The socket is already closed on the other side
					if (Logging.DEBUG) Log.d(TAG,
							"shutdownCore(), socket closed after " + i + " retries since trigger");
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
		notifyProgress(BoincOp.ShutdownCore, ClientReceiver.PROGRESS_XFER_FINISHED);
		if (!connectionAlive) {
			// Socket was closed on remote side, so connection was lost as expected
			// We notify about lost connection
			notifyDisconnected(true);
			closeConnection();
			changeIsHandlerWorking(false);
			return;
		}
		changeIsHandlerWorking(false);
		// Otherwise, there is still connection present, we did not shutdown
		// remote client, we keep connection, so user is aware and can possibly
		// re-try
	}

	public void doNetworkCommunication() {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.DoNetworkComm, ClientReceiver.PROGRESS_XFER_STARTED);
		boolean success = mRpcClient.networkAvailable();
		
		if (!success) {
			if (Logging.INFO) Log.i(TAG, "RPC failed in doNetworkCommunication()");
			notifyProgress(BoincOp.DoNetworkComm, ClientReceiver.PROGRESS_XFER_FINISHED);
			notifyError(BoincOp.DoNetworkComm, 0, mRpcClient.getLastErrorMessage());
			rpcFailed();
			changeIsHandlerWorking(false);
			return;
		} else {
			notifyOperationFinish(BoincOp.DoNetworkComm);
			notifyProgress(BoincOp.DoNetworkComm, ClientReceiver.PROGRESS_XFER_FINISHED);
		}
		changeIsHandlerWorking(false);
	}

	public void projectOperation(int operation, String projectUrl) {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.ProjectOperation, ClientReceiver.PROGRESS_XFER_STARTED);
		mRpcClient.projectOp(operation, projectUrl);
		notifyOperationFinish(BoincOp.ProjectOperation);
		notifyProgress(BoincOp.ProjectOperation, ClientReceiver.PROGRESS_XFER_FINISHED);
		// Regardless of success we run update of projects
		// If there is problem with socket, it will be handled there
		updateProjects(true);
		changeIsHandlerWorking(false);
	}
	
	public void projectsOperation(int operation, String[] projectUrls) {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.ProjectOperation, ClientReceiver.PROGRESS_XFER_STARTED);
		for (String projectUrl: projectUrls)
			mRpcClient.projectOp(operation, projectUrl);
		notifyOperationFinish(BoincOp.ProjectOperation);
		notifyProgress(BoincOp.ProjectOperation, ClientReceiver.PROGRESS_XFER_FINISHED);
		// Regardless of success we run update of projects
		// If there is problem with socket, it will be handled there
		updateProjects(true);
		changeIsHandlerWorking(false);
	}

	public void taskOperation(int operation, String projectUrl, String taskName) {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.TaskOperation, ClientReceiver.PROGRESS_XFER_STARTED);
		mRpcClient.resultOp(operation, projectUrl, taskName);
		notifyOperationFinish(BoincOp.TaskOperation);
		notifyProgress(BoincOp.TaskOperation, ClientReceiver.PROGRESS_XFER_FINISHED);
		// Regardless of success we run update of tasks
		// If there is problem with socket, it will be handled there
		updateTasks(true);
		changeIsHandlerWorking(false);
	}
	
	public void tasksOperation(int operation, TaskDescriptor[] tasks) {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.TaskOperation, ClientReceiver.PROGRESS_XFER_STARTED);
		for (TaskDescriptor task: tasks)
			mRpcClient.resultOp(operation, task.projectUrl, task.taskName);
		notifyOperationFinish(BoincOp.TaskOperation);
		notifyProgress(BoincOp.TaskOperation, ClientReceiver.PROGRESS_XFER_FINISHED);
		// Regardless of success we run update of tasks
		// If there is problem with socket, it will be handled there
		updateTasks(true);
		changeIsHandlerWorking(false);
	}

	public void transferOperation(int operation, String projectUrl, String fileName) {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.TransferOperation, ClientReceiver.PROGRESS_XFER_STARTED);
		mRpcClient.transferOp(operation, projectUrl, fileName);
		notifyOperationFinish(BoincOp.TransferOperation);
		notifyProgress(BoincOp.TransferOperation, ClientReceiver.PROGRESS_XFER_FINISHED);
		// Regardless of success we run update of transfers
		// If there is problem with socket, it will be handled there
		updateTransfers(true);
		changeIsHandlerWorking(false);
	}
	
	public void transfersOperation(int operation, TransferDescriptor[] transfers) {
		if (mDisconnecting) return;  // already in disconnect phase
		changeIsHandlerWorking(true);
		notifyProgress(BoincOp.TransferOperation, ClientReceiver.PROGRESS_XFER_STARTED);
		for (TransferDescriptor transfer: transfers)
			mRpcClient.transferOp(operation, transfer.projectUrl, transfer.fileName);
		notifyOperationFinish(BoincOp.TransferOperation);
		notifyProgress(BoincOp.TransferOperation, ClientReceiver.PROGRESS_XFER_FINISHED);
		// Regardless of success we run update of transfers
		// If there is problem with socket, it will be handled there
		updateTransfers(true);
		changeIsHandlerWorking(false);
	}

	private synchronized void notifyOperationBegin(final BoincOp boincOp) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.notifyOperationBegin(boincOp);
			}
		});
	}
	
	private synchronized void notifyProgress(final BoincOp boincOp, final int progress) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.notifyProgress(boincOp, progress);
			}
		});
	}
	
	private synchronized void notifyOperationFinish(final BoincOp boincOp) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.notifyOperationFinish(boincOp);
			}
		});
	}
	
	private synchronized void notifyError(final BoincOp boincOp, final int error_num,
			final String message) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.notifyError(boincOp, error_num, message);
			}
		});
	}
	
	public synchronized void notifyPollError(final BoincOp boincOp,
			final int error_num, final int operation, final String errorMessage, final String param) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.notifyPollError(boincOp, error_num, operation, errorMessage, param);
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

	private synchronized void notifyDisconnected(final boolean disconnectedByManager) {
		if (mDisconnecting) return; // already notified (by other thread)
		// Set flag, so no further notifications/replies will be posted to UI-thread
		mDisconnecting = true;
		// post last notification - about disconnect
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				// will send notification to observers
				mReplyHandler.notifyDisconnected(disconnectedByManager);
				mReplyHandler.disconnecting(); // will initiate clearing of bridge
				// The mDisconnecting set to true above will prevent further posts
				// and all post() calls are guarded by synchronized statement
			}
		});
	}

	private synchronized void updatedClientMode(final ModeInfo clientMode) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedClientMode(clientMode);
			}
		});
	}

	private synchronized void updatedHostInfo(final HostInfo hostInfo) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedHostInfo(hostInfo);
			}
		});
	}
	
	private synchronized void currentAllProjectsList(final ArrayList<ProjectListEntry> projects) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.currentAllProjectsList(projects);
			}
		});
	}
	
	private synchronized void currentBAMInfo(final AccountMgrInfo bamInfo) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.currentBAMInfo(bamInfo);
			}
		});
	}
	
	private synchronized void currentAuthCode(final String projectUrl, final String authCode) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.currentAuthCode(projectUrl, authCode);
			}
		});
	}
	
	private synchronized void currentProjectConfig(final String projectUrl,
			final ProjectConfig projectConfig) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.currentProjectConfig(projectUrl, projectConfig);
			}
		});
	}
	
	private synchronized void currentGlobalPreferences(final GlobalPreferences globalPrefs) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.currentGlobalPreferences(globalPrefs);
			}
		});
	}
	
	private synchronized void notifyGlobalPrefsChanged() {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.onGlobalPreferencesChanged();
			}
		});
	}
	
	private synchronized void notifyAfterProjectAttach(final String projectUrl) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.afterProjectAttach(projectUrl);
			}
		});
	}
	
	private synchronized void notifyAfterAccountMgrRPC() {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.afterAccountMgrRPC();
			}
		});
	}
	
	private synchronized void notifyCancelUpdate(final BoincOp boincOp) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.cancelledUpdate(boincOp);
			}
		});
	}

	private synchronized void updatedProjects(final ArrayList<ProjectInfo> projects) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedProjects(projects);
			}
		});
	}

	private synchronized void updatedTasks(final ArrayList<TaskInfo> tasks) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedTasks(tasks);
			}
		});
	}

	private synchronized void updatedTransfers(final ArrayList<TransferInfo> transfers) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedTransfers(transfers);
			}
		});
	}

	private synchronized void updatedMessages(final ArrayList<MessageInfo> messages) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedMessages(messages);
			}
		});
	}
	
	private synchronized void updatedNotices(final ArrayList<NoticeInfo> notices) {
		if (mDisconnecting) return;
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				mReplyHandler.updatedNotices(notices);
			}
		});
	}
	
	private synchronized void notifyChangeOfIsWorking() {
		final boolean currentIsWorking = isWorking();
		if (mPreviousStateOfIsWorking != currentIsWorking) {
			mPreviousStateOfIsWorking = currentIsWorking;
			final ReplyHandler replyHandler = mReplyHandler;
			replyHandler.post(new Runnable() {
				@Override
				public void run() {
					if (replyHandler != null)
						replyHandler.onChangeIsWorking(currentIsWorking);
				}
			});
		}
	}
	
	private synchronized void notifyCancelPollOperations(final int opFlags) {
		mReplyHandler.post(new Runnable() {
			@Override
			public void run() {
				if (mReplyHandler != null)
					mReplyHandler.cancelPollOperations(opFlags);
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
		mHaveCuda = ccState.have_cuda;
		mHaveAti = ccState.have_ati;
		
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
		mHaveCuda = ccState.have_cuda;
		mHaveAti = ccState.have_ati;
		
		dataSetProjects(ccState.projects);
		updatedProjects(getProjects());
		dataSetApps(ccState.apps);
		if (mDisconnecting) return;  // already in disconnect phase
		dataSetTasks(ccState.workunits, ccState.results);
		updatedTasks(getTasks());
		// Retrieve also transfers. Most of time empty anyway, so it runs fast
		updateTransfers(true);
		if (mDisconnecting) return;  // already in disconnect phase
		// Messages are useful in most of cases, so we start to retrieve them automatically as well
		updateMessages(true);
		// notices also should be retrieved
		if (mDisconnecting) return;  // already in disconnect phase
		updateNotices(true);
		mInitialStateRetrieved = true;
	}

	private void dataSetProjects(ArrayList<Project> projects) {
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
			ProjectInfo project = ProjectInfoCreator.create(prj, totalResources, mHaveAti, mHaveCuda,
					mFormatter);
			mProjects.put(prj.master_url, project);
		}
		if (Logging.DEBUG) Log.d(TAG, "dataSetProjects(): End update");
	}

	private void dataSetApps(ArrayList<App> apps) {
		mApps.clear();
		Iterator<App> ai = apps.iterator();
		while (ai.hasNext()) {
			App app = ai.next();
			mApps.put(app.name, app);
		}
	}

	private void dataSetTasks(ArrayList<Workunit> workunits, ArrayList<Result> results) {
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
				if (Logging.WARNING) Log.w(TAG, "No project info for WU=" + result.name +
						" (project_url: " + result.project_url + "), skipping WU");
				continue;
			}
			Workunit workunit = mWorkunits.get(result.wu_name);
			if (workunit == null) {
				if (Logging.WARNING) Log.w(TAG, "No workunit info for WU=" + result.name +
						" (wu_name: " + result.wu_name + "), skipping WU");
				continue;
			}
			App app = mApps.get(workunit.app_name);
			if (app == null) {
				if (Logging.WARNING) Log.w(TAG, "No application info for WU=" + result.name +
						" (app_name: " + workunit.app_name + "), skipping WU");
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

	private void dataSetTransfers(ArrayList<Transfer> transfers) {
		if (Logging.DEBUG) Log.d(TAG, "dataSetTransfers(): Begin update");
		mTransfers.clear();
		Iterator<Transfer> ti = transfers.iterator();
		while (ti.hasNext()) {
			Transfer transfer = ti.next();
			ProjectInfo proj = mProjects.get(transfer.project_url);
			if (proj == null) {
				if (Logging.WARNING) Log.w(TAG, "No project for WU=" +
						transfer.name + " (project_url: " + transfer.project_url + "), setting dummy");
				proj = new ProjectInfo();
				proj.project = "???";
			}
			TransferInfo transferInfo = TransferInfoCreator.create(transfer, proj.project, mFormatter);
			mTransfers.add(transferInfo);
		}
		if (Logging.DEBUG) Log.d(TAG, "dataSetTransfers(): End update");
	}

	private boolean dataUpdateTasks(ArrayList<Result> results) {
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
				if (Logging.DEBUG) Log.d(TAG,
						"Task not found while trying dataUpdateTasks() - needs full updateCcState() update");
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
			if (Logging.DEBUG) Log.d(TAG, "dataUpdateTasks(): " + oldTaskNames.size() +
					" obsolete tasks detected");
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

	private void dataUpdateMessages(ArrayList<Message> messages) {
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
	
	private void dataUpdateNotices(ArrayList<Notice> notices) {
		if (notices == null) return;
		if (Logging.DEBUG) Log.d(TAG, "dataUpdateNotices(): Begin update");
		Iterator<Notice> ni = notices.iterator();
		while (ni.hasNext()) {
			edu.berkeley.boinc.lite.Notice msg = ni.next();
			NoticeInfo message = NoticeInfoCreator.create(msg, mFormatter);
			mNotices.put(msg.seqno, message);
		}
		if (Logging.DEBUG) Log.d(TAG, "dataUpdateNotices(): End update");
	}

	private final ArrayList<ProjectInfo> getProjects() {
		return new ArrayList<ProjectInfo>(mProjects.values());
	}

	private final ArrayList<TaskInfo> getTasks() {
		return new ArrayList<TaskInfo>(mTasks.values());
	}

	private final ArrayList<TransferInfo> getTransfers() {
		return new ArrayList<TransferInfo>(mTransfers);
	}

	private final ArrayList<MessageInfo> getMessages() {
		return new ArrayList<MessageInfo>(mMessages.values());
	}
	
	private final ArrayList<NoticeInfo> getNotices() {
		return new ArrayList<NoticeInfo>(mNotices.values());
	}
}
