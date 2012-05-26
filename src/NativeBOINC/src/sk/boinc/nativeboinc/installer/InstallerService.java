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

package sk.boinc.nativeboinc.installer;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.NotificationController;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.util.ProgressItem;
import sk.boinc.nativeboinc.util.UpdateItem;

import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Binder;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;

/**
 * @author mat
 *
 */
public class InstallerService extends Service {
	private static final String TAG = "InstallerService";
	
	public static final String BOINC_CLIENT_ITEM_NAME = "BOINC client";
	public static final String BOINC_DUMP_ITEM_NAME = "BOINC Dump Files";
	public static final String BOINC_REINSTALL_ITEM_NAME = "BOINC Reinstall";
	
	private InstallerThread mInstallerThread = null;
	private InstallerHandler mInstallerHandler = null;
	
	private List<AbstractInstallerListener> mListeners = new ArrayList<AbstractInstallerListener>();
	
	private BoincManagerApplication mApp = null;
	private NotificationController mNotificationController = null;
	
	private ArrayList<ProjectDistrib> mPendingNewProjectDistribs = null;
	private Object mPendingNewProjectDistribsSync = new Object(); // syncer
	private ClientDistrib mPendingClientDistrib = null;
	private Object mPendingClientDistribSync = new Object();
	
	private UpdateItem[] mPendingUpdateItems = null;
	private Object mPendingUpdateItemsSync = new Object();
	
	public class LocalBinder extends Binder {
		public InstallerService getService() {
			return InstallerService.this;
		}
	}
	
	private LocalBinder mBinder = new LocalBinder();
	
	private NativeBoincService mRunner = null;
	
	private ServiceConnection mRunnerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceConnected()");
			mRunner.addNativeBoincListener(mInstallerHandler);
			mRunner.addMonitorListener(mInstallerHandler);
			mInstallerHandler.setRunnerService(mRunner);
		}
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			mRunner.removeNativeBoincListener(mInstallerHandler);
			mRunner.removeMonitorListener(mInstallerHandler);
			mRunner = null;
			mInstallerHandler.setRunnerService(null);
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceDisconnected()");
		}
	};
	
	private void doBindRunnerService() {
		bindService(new Intent(InstallerService.this, NativeBoincService.class),
				mRunnerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindRunnerService() {
		mRunner.removeNativeBoincListener(mInstallerHandler);
		mRunner.removeMonitorListener(mInstallerHandler);
		unbindService(mRunnerServiceConnection);
		mRunner = null;
	}
	
	@Override
	public void onCreate() {
		if (Logging.DEBUG) Log.d(TAG, "onCreate");
		mApp = (BoincManagerApplication)getApplication();
		mNotificationController = mApp.getNotificationController();
		mListenerHandler = new ListenerHandler();
		startInstaller();
		doBindRunnerService();
	}
	
	private Runnable mInstallerStopper = null;
	
	private static final int DELAYED_DESTROY_INTERVAL = 4000;
	
	private int mBindCounter = 0;
	private boolean mUnlockStopWhenNotWorking = false;
	private Object mStoppingLocker = new Object();
	
	@Override
	public IBinder onBind(Intent intent) {
		synchronized(mStoppingLocker) {
			mBindCounter++;
			mUnlockStopWhenNotWorking = false;
			if (Logging.DEBUG) Log.d(TAG, "Bind. bind counter: " + mBindCounter);
		}
		startService(new Intent(this, InstallerService.class));
		return mBinder;
	}
	
	@Override
	public void onRebind(Intent intent) {
		synchronized(mStoppingLocker) {
			mBindCounter++;
			mUnlockStopWhenNotWorking = false;
			if (Logging.DEBUG) Log.d(TAG, "Rebind. bind counter: " + mBindCounter);
		}
		if (mInstallerHandler != null && mInstallerStopper != null) {
			if (Logging.DEBUG) Log.d(TAG, "Rebind");
			mInstallerHandler.removeCallbacks(mInstallerStopper);
			mInstallerStopper = null;
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
		if (!isWorking() && bindCounter == 0) {
			mInstallerStopper = new Runnable() {
				@Override
				public void run() {
					synchronized(mStoppingLocker) {
						if (isWorking() || mBindCounter != 0) {
							mUnlockStopWhenNotWorking = (mBindCounter == 0);
							mInstallerStopper = null;
							return;
						}
					}
					if (Logging.DEBUG) Log.d(TAG, "Stop InstallerService");
					mInstallerStopper = null;
					stopSelf();
				}
			};
			mInstallerHandler.postDelayed(mInstallerStopper, DELAYED_DESTROY_INTERVAL);
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
	
	@Override
	public void onDestroy() {
		if (Logging.DEBUG) Log.d(TAG, "onDestroy");
		mInstallerHandler.destroy();
		doUnbindRunnerService();
		stopInstaller();
		mInstallerHandler = null;
		mNotificationController = null;
		mApp = null;
	}
	
	private InstallError mPendingInstallError = null;
	private Object mPendingInstallErrorSync = new Object();
	
	/**
	 * listener handler - 
	 * @author mat
	 *
	 */
	public class ListenerHandler extends Handler {
		
		public void onOperation(String distribName, String projectUrl, String opDescription) {
			mNotificationController.handleOnOperation(distribName, projectUrl, opDescription);
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperation(distribName, opDescription);
		}
		
		public void onOperationProgress(String distribName, String projectUrl,
				String opDescription,int progress) {
			mNotificationController.handleOnOperationProgress(distribName, projectUrl,
					opDescription, progress);
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperationProgress(
							distribName, opDescription, progress);
		}

		public void onOperationError(String distribName, String projectUrl, String errorMessage) {
			if (mApp.isInstallerRun())
				mApp.backToPreviousInstallerStage();
			
			mNotificationController.handleOnOperationError(distribName, projectUrl, errorMessage);
			
			boolean called = false;
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners) {
				listener.onOperationError(distribName, errorMessage);
				called = true;
			}
			
			synchronized(mPendingInstallErrorSync) {
				if (!called)
					mPendingInstallError = new InstallError(distribName, projectUrl, errorMessage);
				else // if already handled
					mPendingInstallError = null;
			}
		}
		
		public void onOperationCancel(String distribName, String projectUrl) {
			if (mApp.isInstallerRun())
				mApp.backToPreviousInstallerStage();
			
			mNotificationController.handleOnOperationCancel(distribName, projectUrl);
						
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperationCancel(distribName);
		}
		
		public void onOperationFinish(String distribName, String projectUrl) {
			/* to next insraller stage */
			if (Logging.DEBUG) Log.d(TAG, "Finish operation:"+distribName);
			
			int installerStage = mApp.getInstallerStage(); 
			if (installerStage == BoincManagerApplication.INSTALLER_CLIENT_INSTALLING_STAGE)
				mApp.setInstallerStage(BoincManagerApplication.INSTALLER_PROJECT_STAGE);
			else if (installerStage == BoincManagerApplication.INSTALLER_PROJECT_INSTALLING_STAGE)
				mApp.setInstallerStage(BoincManagerApplication.INSTALLER_FINISH_STAGE);
		
			mNotificationController.handleOnOperationFinish(distribName, projectUrl);
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperationFinish(distribName);
		}
		
		public void currentProjectDistribList(ArrayList<ProjectDistrib> projectDistribs) {
			synchronized(mPendingNewProjectDistribsSync) {
				mPendingNewProjectDistribs = projectDistribs;
			}
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener instanceof InstallerUpdateListener)
					((InstallerUpdateListener)listener).currentProjectDistribList(projectDistribs);
		}
		
		public void currentClientDistrib(ClientDistrib clientDistrib) {
			synchronized(mPendingClientDistribSync) {
				mPendingClientDistrib = clientDistrib;
			}
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener instanceof InstallerUpdateListener)
					((InstallerUpdateListener)listener).currentClientDistrib(clientDistrib);
		}
		
		public void notifyBinariesToInstallOrUpdate(UpdateItem[] updateItems) {
			synchronized(mPendingUpdateItemsSync) {
				mPendingUpdateItems = updateItems;
			}
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			
			for (AbstractInstallerListener listener: listeners)
				if (listener instanceof InstallerUpdateListener)
					((InstallerUpdateListener)listener).binariesToUpdateOrInstall(updateItems);
		}
		
		public void onChangeIsWorking(boolean isWorking) {
			for (AbstractInstallerListener listener: mListeners)
				listener.onChangeInstallerIsWorking(isWorking);
		}
	}
	
	private ListenerHandler mListenerHandler = null;
	
	public synchronized void addInstallerListener(AbstractInstallerListener listener) {
		mListeners.add(listener);
	}
	
	public synchronized void removeInstallerListener(AbstractInstallerListener listener) {
		mListeners.remove(listener);
	}
	
	private void startInstaller() {
		if (mInstallerThread != null)
			return;	// do nothing
		
		if (Logging.DEBUG) Log.d(TAG, "Starting up Installer");
		ConditionVariable lock = new ConditionVariable(false);
		mInstallerThread = new InstallerThread(lock, this, mListenerHandler);
		mInstallerThread.start();
		boolean runningOk = lock.block(2000); // Locking until new thread fully runs
		if (!runningOk) {
			if (Logging.ERROR) Log.e(TAG, "InstallerThread did not start in 1 second");
			throw new RuntimeException("Worker thread cannot start");
		}
		if (Logging.DEBUG) Log.d(TAG, "InstallerThread started successfully");
		mInstallerHandler = mInstallerThread.getInstallerHandler();
	}
	
	public void stopInstaller() {
		mInstallerHandler.cancelAll();
		mInstallerThread.stopThread();
		mInstallerThread = null;
		mListeners.clear();
		mListenerHandler = null;
	}
	
	/**
	 * resolve project name
	 */
	public String resolveProjectName(String projectUrl) {
		return mInstallerHandler.resolveProjectName(projectUrl);
	}
	
	/**
	 * Install client automatically (with signature checking)
	 */
	public void installClientAutomatically() {
		mNotificationController.notifyInstallClientBegin();
		mInstallerThread.installClientAutomatically();
	}
	
	/* update client distrib info */
	public void updateClientDistrib() {
		synchronized(mPendingClientDistribSync) {
			mPendingClientDistrib = null;
		}
		mInstallerThread.updateClientDistrib();
	}
	
	public ClientDistrib getPendingClientDistrib() {
		synchronized(mPendingClientDistribSync) {
			ClientDistrib pending = mPendingClientDistrib;
			mPendingClientDistrib = null;
			return pending;
		}
	}
	/**
	 * get pending install error
	 * @return pending install error
	 */
	public InstallError getPendingError() {
		synchronized (mPendingInstallErrorSync) {
			InstallError installError = mPendingInstallError;
			mPendingInstallError = null;
			return installError;
		}
	}
	
	/**
	 * Installs BOINC application automatically (with signature checking)
	 * @param projectUrl
	 * @param appName
	 */
	public void installProjectApplicationsAutomatically(String projectName, String projectUrl) {
		mNotificationController.notifyInstallProjectBegin(projectName);
		mInstallerThread.installProjectApplicationAutomatically(projectUrl);
	}
	
	/**
	 * Reinstalls update item 
	 */
	public void reinstallUpdateItems(ArrayList<UpdateItem> updateItems) {
		for (UpdateItem item: updateItems)
			if (item.name.equals(BOINC_CLIENT_ITEM_NAME))
				mNotificationController.notifyInstallClientBegin();
			else
				mNotificationController.notifyInstallProjectBegin(item.name);
			
		mInstallerThread.reinstallUpdateItems(updateItems);
	}
	
	public void updateDistribsFromSDCard(String dirPath, String[] distribNames) {
		for (String distribName: distribNames)
			if (distribName.equals(BOINC_CLIENT_ITEM_NAME))
				mNotificationController.notifyInstallClientBegin();
			else
				mNotificationController.notifyInstallProjectBegin(distribName);
		
		mInstallerThread.updateDistribsFromSDCard(dirPath, distribNames);
	}
	
	public void updateProjectDistribList() {
		synchronized(mPendingNewProjectDistribsSync) {
			mPendingNewProjectDistribs = null;
		}
		mInstallerThread.updateProjectDistribList();
	}
	
	public ArrayList<ProjectDistrib> getPendingNewProjectDistribList() {
		synchronized(mPendingNewProjectDistribsSync) {
			ArrayList<ProjectDistrib> pending = mPendingNewProjectDistribs;
			mPendingNewProjectDistribs = null;
			return pending;
		}
	}
	
	/* remove detached projects from installed projects */
	public void synchronizeInstalledProjects() {
		mInstallerHandler.synchronizeInstalledProjects();
	}
	
	public void cancelAll() {
		mInstallerHandler.cancelAll();
	}
	
	public void cancelClientInstallation() {
		mInstallerHandler.cancelClientInstallation();
	}
	
	public void cancelProjectAppsInstallation(String projectUrl) {
		mInstallerHandler.cancelProjectAppsInstallation(projectUrl);
	}
	
	public void cancelDumpFiles() {
		mInstallerHandler.cancelDumpFiles();
	}
	
	public void cancelOperation() {
		mInstallerHandler.cancelOperation(mInstallerThread);
	}
	
	/**
	 * Check whether client is installed
	 * @return
	 */

	private static final String[] sRequiredFiles = {
		"/boinc/client_state.xml",
		"/boinc/client_state_prev.xml",
		"/boinc/time_stats_log",
		"/boinc/lockfile",
		"/boinc/daily_xfer_history.xml",
		"/boinc/gui_rpc_auth.cfg"
	};
	
	public static boolean isClientInstalled(Context context) {
		File filesDir = context.getFilesDir();
		if (!context.getFileStreamPath("boinc_client").exists() ||
			!context.getFileStreamPath("boinc").isDirectory())
			return false;
		File boincFile = new File(filesDir.getAbsolutePath()+"/boinc/gui_rpc_auth.cfg");
		if (!boincFile.exists())
			return false;
		for (String path: sRequiredFiles) {
			boincFile = new File(filesDir.getAbsolutePath()+path);
			if (!boincFile.exists())
				return false;	// if installation broken
		}
		return true;
	}
	
	public static String getBoincLogs(Context context) {
		StringBuilder sB = new StringBuilder();
		
		InputStreamReader reader = null;
		try {
			String messagesFilePath = context.getFilesDir()+"/boinc/messages.log";
			reader = new InputStreamReader(new FileInputStream(messagesFilePath), "UTF-8");
			
			char[] buffer = new char[4096];
			
			while (true) {
				int readed = reader.read(buffer);
				if (readed == -1)
					break; // if end
				sB.append(buffer, 0, readed);
			}
		} catch(IOException ex) {
			return null; // cant read
		} finally {
			try {
				if (reader != null)
					reader.close();
			} catch(IOException ex) { }
		}
		return sB.toString();
	}
	
	/**
	 * 
	 * @return installed binaries info
	 */
	public InstalledBinary[] getInstalledBinaries() {
		return mInstallerHandler.getInstalledBinaries();
	}
	
	/**
	 * @param attachProjectUrls attached projects urls list
	 * @return update items
	 */
	public void getBinariesToUpdateOrInstall() {
		mInstallerThread.getBinariesToUpdateOrInstall();
	}
	
	public UpdateItem[] getPendingBinariesToUpdateOrInstall() {
		synchronized(mPendingUpdateItemsSync) {
			UpdateItem[] pending = mPendingUpdateItems;
			mPendingUpdateItems = null;
			return pending;
		}
	}
	
	public String[] getBinariesToUpdateFromSDCard(String path) {
		return mInstallerHandler.getBinariesToUpdateFromSDCard(path);
	}
	
	/**
	 * dump boinc files
	 * @return
	 */
	public void dumpBoincFiles(String directory) {
		mInstallerThread.dumpBoincFiles(directory); 
	}
	
	public void reinstallBoinc() {
		mInstallerThread.reinstallBoinc();
	}
	
	public boolean isWorking() {
		return mInstallerHandler.isWorking();
	}
	
	/**
	 * get progress item
	 * @return distrib progresses (not sorted)
	 */
	public synchronized ProgressItem[] getProgressItems() {
		return mNotificationController.getProgressItems();
	}
	
	/* get current status */
	public synchronized ProgressItem getProgressItem(String distribName) {
		return mNotificationController.getProgressItem(distribName);
	}
}
