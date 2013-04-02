/* 
 * NativeBOINC - Native BOINC Client with Manager
 * Copyright (C) 2011, Mateusz Szpakowski
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
package sk.boinc.nativeboinc.bugcatch;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.NotificationController;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.installer.AbstractInstallerListener;
import sk.boinc.nativeboinc.util.PendingController;
import sk.boinc.nativeboinc.util.PendingErrorHandler;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;

/**
 * @author mat
 *
 */
public class BugCatcherService extends Service {

	private static final String TAG = "BugCatcherService"; 
	
	public class LocalBinder extends Binder {
		public BugCatcherService getService() {
			return BugCatcherService.this;
		}
	}
	
	private final IBinder mBinder = new LocalBinder();
	
	private BoincManagerApplication mApp = null;
	private NotificationController mNotificationController = null;
	
	private int mBindCounter = 0;
	private boolean mUnlockStopWhenNotWorking = false;
	private Object mStoppingLocker = new Object();
	
	private Runnable mBugCatchStopper = null;
	private Handler mBugCatchHandler;
	
	private PendingController<BugCatchOp> mPendingController = new PendingController<BugCatchOp>("BugCatcher");
	
	private static final int DELAYED_DESTROY_INTERVAL = 4000;
	
	@Override
	public IBinder onBind(Intent intent) {
		mBindCounter++;
		if (Logging.DEBUG) Log.d(TAG, "onBind(), binCounter="+mBindCounter);
		// Just make sure the service is running:
		startService(new Intent(this, BugCatcherService.class));
		return mBinder;
	}
	
	@Override
	public void onRebind(Intent intent) {
		synchronized(mStoppingLocker) {
			mBindCounter++;
			mUnlockStopWhenNotWorking = false;
			if (Logging.DEBUG) Log.d(TAG, "Rebind. bind counter: " + mBindCounter);
		}
		if (mBugCatchHandler != null && mBugCatchStopper != null) {
			if (Logging.DEBUG) Log.d(TAG, "Rebind");
			mBugCatchHandler.removeCallbacks(mBugCatchStopper);
			mBugCatchStopper = null;
		}
	}
	
	@Override
	public boolean onUnbind(Intent intent) {
		int bindCounter; 
		synchronized(mStoppingLocker) {
			mBindCounter--;
			bindCounter = mBindCounter;
		}
		if (Logging.DEBUG) Log.d(TAG, "Unbind. bind counter: " + bindCounter);
		if (!serviceIsWorking() && bindCounter == 0) {
			mBugCatchStopper = new Runnable() {
				@Override
				public void run() {
					synchronized(mStoppingLocker) {
						if (serviceIsWorking() || mBindCounter != 0) {
							mUnlockStopWhenNotWorking = (mBindCounter == 0);
							mBugCatchStopper = null;
							return;
						}
					}
					if (Logging.DEBUG) Log.d(TAG, "Stop InstallerService");
					mBugCatchStopper = null;
					stopSelf();
				}
			};
			mBugCatchHandler.postDelayed(mBugCatchStopper, DELAYED_DESTROY_INTERVAL);
		} else {
			synchronized(mStoppingLocker) {
				mUnlockStopWhenNotWorking = (mBindCounter == 0);
			}
		}
		return true;
	}
	
	public synchronized boolean doStopWhenNotWorking() {
		return (mBindCounter == 0 && mUnlockStopWhenNotWorking);
	}
	

	public static class ListenerHandler extends Handler {
		
		public boolean notifyBugReportBegin(String message) {
			return false;
		}
		
		public boolean notifyBugReportProgress(String message, String bugReportId, int count, int total) {
			return false;
		}
		
		public boolean notifyBugReportFinish(String message) {
			return false;
		}
		
		public boolean notifyBugReportError(String message, String bugReportId) {
			return false;
		}
	}
	
	private ListenerHandler mListenerHandler = null;
	private BugCatcherThread mBugCatcherThread = null;
	private BugCatcherHandler mBugCatcherHandler = null;
	
	private ArrayList<BugCatcherListener> mListeners = new ArrayList<BugCatcherListener>();
	
	@Override
	public void onCreate() {
		if (Logging.DEBUG) Log.d(TAG, "onCreate");
		mApp = (BoincManagerApplication)getApplication();
		mNotificationController = mApp.getNotificationController();
		mListenerHandler = new ListenerHandler();
		mPendingController = new PendingController<BugCatchOp>("BugCatcher");
		
		startBugCatcher();
	}
	
	@Override
	public void onDestroy() {
		if (Logging.DEBUG) Log.d(TAG, "onDestroy");
		stopBugCatcher();
		mNotificationController = null;
		mApp = null;
	}
	
	private void startBugCatcher() {
		if (mBugCatcherThread != null)
			return;	// do nothing
		
		if (Logging.DEBUG) Log.d(TAG, "Starting up BugCatcher");
		ConditionVariable lock = new ConditionVariable(false);
		mBugCatcherThread = new BugCatcherThread(lock, this, mListenerHandler);
		mBugCatcherThread.start();
		boolean runningOk = lock.block(2000); // Locking until new thread fully runs
		if (!runningOk) {
			if (Logging.ERROR) Log.e(TAG, "BugCatcherThread did not start in 1 second");
			throw new RuntimeException("BugCatcher thread cannot start");
		}
		if (Logging.DEBUG) Log.d(TAG, "BugCatcherThread started successfully");
		mBugCatcherHandler = mBugCatcherThread.getInstallerHandler();
	}
	
	public void stopBugCatcher() {
		BugCatcherHandler handler = mBugCatcherHandler;
		mBugCatcherHandler = null;
		handler.cancelAll();
		mBugCatcherThread.stopThread();
		mBugCatcherThread = null;
		synchronized(this) {
			mListeners.clear();
		}
		try {
			Thread.sleep(200);
		} catch(InterruptedException ex) { }
		handler.destroy();
	}
	
	public synchronized void addBugCatcherListener(BugCatcherListener listener) {
		if (mListeners != null)
			mListeners.add(listener);
	}
	
	public synchronized void removeBugCatcherListener(BugCatcherListener listener) {
		if (mListeners != null)
			mListeners.remove(listener);
	}
	
	
	public boolean serviceIsWorking() {
		if (mBugCatcherHandler == null)
			return false;
		return mBugCatcherHandler.isWorking();
	}

	public static void clearBugReports(Context context) {
		File bugCatchDir = new File(context.getFilesDir()+"/bugcatch");
		for (File file: bugCatchDir.listFiles())
			file.delete();
	}
	
	public void saveToSDCard() {
		if (mBugCatcherHandler == null) return;
		
		mNotificationController.notifyBugCatcherBegin(mApp.getString(R.string.bugCopyingBegin));
		mBugCatcherThread.saveToSDCard();
	}
	
	public static List<BugReportInfo> loadBugReports(Context context) {
		return null;
	}
	
	public void sendBugsToAuthor() {
		if (mBugCatcherHandler == null) return;
		
		mNotificationController.notifyBugCatcherBegin(mApp.getString(R.string.bugSendingBegin));
		mBugCatcherThread.sendBugsToAuthor();
	}
	
	/**
	 * get pending install error
	 * @return pending install error
	 */
	public boolean handlePendingErrors(final BugCatchOp bugCatchOp, final BugCatcherListener listener) {
		return mPendingController.handlePendingErrors(bugCatchOp, new PendingErrorHandler<BugCatchOp>() {
			
			@Override
			public boolean handleError(BugCatchOp bugCatchOp, Object error) {
				if (error == null)
					return false;
				return listener.onBugReportError(null);
			}
		});
	}
	
	public void cancelOperation() {
		
	}
}
