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

import sk.boinc.androboinc.clientconnection.ClientReplyReceiver;
import sk.boinc.androboinc.debug.Logging;
import sk.boinc.androboinc.debug.NetStats;
import sk.boinc.androboinc.util.ClientId;
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

	public void cancelPendingUpdates(final ClientReplyReceiver callback) {
		// Run immediately NOW (from UI thread)
		mHandler.cancelPendingUpdates(callback);
	}

	public void updateClientMode(final ClientReplyReceiver callback) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateClientMode(callback);
			}
		});
	}

	public void updateHostInfo(final ClientReplyReceiver callback) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateHostInfo(callback);
			}
		});
	}

	public void updateProjects(final ClientReplyReceiver callback) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateProjects(callback);
			}
		});
	}

	public void updateTasks(final ClientReplyReceiver callback) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateTasks(callback);
			}
		});
	}

	public void updateTransfers(final ClientReplyReceiver callback) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateTransfers(callback);
			}
		});
	}

	public void updateMessages(final ClientReplyReceiver callback) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.updateMessages(callback);
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

	public void setRunMode(final ClientReplyReceiver callback, final int mode) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.setRunMode(callback, mode);
			}
		});
	}

	public void setNetworkMode(final ClientReplyReceiver callback, final int mode) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.setNetworkMode(callback, mode);
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

	public void projectOperation(final ClientReplyReceiver callback, final int operation, final String projectUrl) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.projectOperation(callback, operation, projectUrl);
			}
		});
	}

	public void taskOperation(final ClientReplyReceiver callback, final int operation, final String projectUrl, final String taskName) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.taskOperation(callback, operation, projectUrl, taskName);
			}
		});
	}

	public void transferOperation(final ClientReplyReceiver callback, final int operation, final String projectUrl, final String fileName) {
		// Execute in worker thread
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				mHandler.transferOperation(callback, operation, projectUrl, fileName);
			}
		});
	}
}
