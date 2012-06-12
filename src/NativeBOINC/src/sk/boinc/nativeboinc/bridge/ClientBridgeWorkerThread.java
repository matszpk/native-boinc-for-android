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


import edu.berkeley.boinc.lite.AccountIn;
import edu.berkeley.boinc.lite.GlobalPreferences;
import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.TaskDescriptor;
import sk.boinc.nativeboinc.clientconnection.TransferDescriptor;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.debug.NetStats;
import sk.boinc.nativeboinc.util.ClientId;
import android.content.Context;
import android.os.ConditionVariable;
import android.os.Looper;
import android.util.Log;


public class ClientBridgeWorkerThread extends Thread {
	private static final String TAG = "ClientBridgeWorkerThread";

	private ClientBridgeWorkerHandler mHandler;
	private ConditionVariable mLock;
	private ClientBridge.ReplyHandler mReplyHandler;
	private Context mContext;
	private NetStats mNetStats;

	public ClientBridgeWorkerThread(
			ConditionVariable lock, 
			final ClientBridge.ReplyHandler replyHandler, 
			final Context context, 
			final NetStats netStats) {
		mLock = lock;
		mReplyHandler = replyHandler;
		mContext = context;
		mNetStats = netStats;
		setDaemon(true);
	}
	
	protected void initializeWorkerHandler() {
		mHandler = new ClientBridgeWorkerHandler(mReplyHandler, mContext, mNetStats);
	}

	@Override
	public void run() {
		if (Logging.DEBUG) Log.d(TAG, "run() - Started " + Thread.currentThread().toString());

		// Prepare Looper in this thread, to receive messages
		// "forever"
		Looper.prepare();

		// Create Handler - we must create it within run() method,
		// so it will be associated with this thread
		mHandler = new ClientBridgeWorkerHandler(mReplyHandler, mContext, mNetStats);
		
		// We have handler, we are ready to receive messages :-)
		if (mLock != null) {
			mLock.open();
			mLock = null;
		}

		// We passed the references to handler, we don't need them here anymore
		mReplyHandler = null;
		mContext = null;
		mNetStats = null;

		// Now, start looping
		Looper.loop();

		// We finished Looper and thread is going to stop 
		if (mLock != null) {
			mLock.open();
			mLock = null;
		}

		mHandler.cleanup();
		mHandler = null;
		if (Logging.DEBUG) Log.d(TAG, "run() - Finished " + Thread.currentThread().toString());
	}

	public void stopThread(ConditionVariable lock) {
		mLock = lock;
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				if (Logging.DEBUG) Log.d(TAG, "Quit message received, stopping " + Thread.currentThread().toString());
				Looper.myLooper().quit();
			}
		});
	}

	public void connect(final ClientId remoteClient, final boolean retrieveInitialData) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.connect(remoteClient, retrieveInitialData);
			}
		});
	}

	public void disconnect() {
		// Run immediately NOW (from UI thread), 
		// because worker thread can be busy with some time-consuming task
		mHandler.disconnect();
		// Triggers for worker thread will be internally arranged by mHandler
	}
	
	public boolean isWorking() {
		if (mHandler != null)
			return mHandler.isWorking();
		return false;
	}

	public void cancelPendingUpdates(int refreshType) {
		// Run immediately NOW (from UI thread)
		if (mHandler != null)
			mHandler.cancelPendingUpdates(refreshType);
		else
			if (Logging.WARNING) Log.w(TAG, "warning: NPE");
	}

	public void updateClientMode() {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateClientMode(false);
			}
		});
	}

	public void updateHostInfo() {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateHostInfo();
			}
		});
	}

	public void updateProjects() {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateProjects(false);
			}
		});
	}

	public void updateTasks() {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateTasks(false);
			}
		});
	}

	public void updateTransfers() {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateTransfers(false);
			}
		});
	}

	public void updateMessages() {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateMessages(false);
			}
		});
	}
	
	public void updateNotices() {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateNotices();
			}
		});
	}
	
	public void getBAMInfo() {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.getBAMInfo();
			}
		});
	}
	
	public void attachToBAM(final String name, final String url, final String password) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.attachToBAM(name, url, password);
			}
		});
	}
	
	public void synchronizeWithBAM() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.synchronizeWithBAM();
			}
		});
	}
	
	public void stopUsingBAM() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.stopUsingBAM();
			}
		});
	}
	
	public void getAllProjectsList(final boolean excludeAttachedProjects) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.getAllProjectsList(excludeAttachedProjects);
			}
		});
	}
	
	public void lookupAccount(final AccountIn accountIn) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.lookupAccount(accountIn);
			}
		});
	}
	
	public void createAccount(final AccountIn accountIn) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.createAccount(accountIn);
			}
		});
	}
	
	public void projectAttach(final String url, final String authCode,
			final String projectName) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.projectAttach(url, authCode, projectName);
			}
		});
	}
	
	public void getProjectConfig(final String url) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.getProjectConfig(url);
			}
		});
	}
	
	public void cancelPollOperations(final int opFlags) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.cancelPollOperations(opFlags);
			}
		});
	}
	
	public void getGlobalPrefsWorking() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.getGlobalPrefsWorking();
			}
		});
	}
	
	public void setGlobalPrefsOverride(final String globalPrefs) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.setGlobalPrefsOverride(globalPrefs);
			}
		});
	}
	
	public void setGlobalPrefsOverrideStruct(final GlobalPreferences globalPrefs,
			final boolean nativeBoinc) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.setGlobalPrefsOverrideStruct(globalPrefs);
			}
		});
	}
	
	public void runBenchmarks() {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.runBenchmarks();
			}
		});
	}

	public void setRunMode(final int mode) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.setRunMode(mode);
			}
		});
	}

	public void setNetworkMode(final int mode) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.setNetworkMode(mode);
			}
		});
	}

	public void shutdownCore() {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.shutdownCore();
			}
		});
	}

	public void doNetworkCommunication() {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.doNetworkCommunication();
			}
		});
	}

	public void projectOperation(final int operation, final String projectUrl) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.projectOperation(operation, projectUrl);
			}
		});
	}
	
	public void projectsOperation(final int operation, final String[] projectUrls) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.projectsOperation(operation, projectUrls);
			}
		});
	}

	public void taskOperation(final int operation, final String projectUrl, final String taskName) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.taskOperation(operation, projectUrl, taskName);
			}
		});
	}
	
	public void tasksOperation(final int operation, final TaskDescriptor[] tasks) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.tasksOperation(operation, tasks);
			}
		});
	}

	public void transferOperation(final int operation, final String projectUrl, final String fileName) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.transferOperation(operation, projectUrl, fileName);
			}
		});
	}
	
	public void transfersOperation(final int operation, final TransferDescriptor[] transfers) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.transfersOperation(operation, transfers);
			}
		});
	}
}
