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
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.util.PendingController;
import sk.boinc.nativeboinc.util.PendingErrorHandler;
import sk.boinc.nativeboinc.util.ProgressItem;
import sk.boinc.nativeboinc.util.UpdateItem;

import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.res.Resources;
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
	
	public static final int DEFAULT_CHANNEL_ID = 0; // for activity
	public static final int MAX_CHANNEL_ID = 16; // for activity
	
	public static final String BOINC_CLIENT_ITEM_NAME = "BOINC client";
	public static final String BOINC_DUMP_ITEM_NAME = "BOINC Dump Files";
	public static final String BOINC_REINSTALL_ITEM_NAME = "BOINC Reinstall";
	
	public static String resolveItemName(Resources res, String name) {
		if (name.equals(BOINC_CLIENT_ITEM_NAME))
			return res.getString(R.string.boincClient);
		else if (name.equals(BOINC_DUMP_ITEM_NAME))
			return res.getString(R.string.dumpFiles);
		else if (name.equals(BOINC_REINSTALL_ITEM_NAME))
			return res.getString(R.string.boincReinstall);
		return name;
	}
	
	private InstallerThread mInstallerThread = null;
	private InstallerHandler mInstallerHandler = null;
	
	private List<AbstractInstallerListener> mListeners = new ArrayList<AbstractInstallerListener>();
	
	private BoincManagerApplication mApp = null;
	private NotificationController mNotificationController = null;
	
	/**
	 * channels: I created channels for separating communication betweens activities and services,
	 * installer calls: calls without channelId argument uses DEFAULT_CHANNEL_ID
	 * DEFAULT_CHANNEL_ID should be used inside UI handling (activities)
	 * other channels should be used by services (for example NativeBoincService)
	 */
	private PendingController<InstallOp>[] mPendingChannels = null;
	
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
		
		mPendingChannels = new PendingController[MAX_CHANNEL_ID];
		for (int i = 0; i < MAX_CHANNEL_ID; i++)
			mPendingChannels[i] = new PendingController<InstallOp>("Inst:PendingCtrl:"+i);
		
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
		doUnbindRunnerService();
		stopInstaller();
		mNotificationController = null;
		mApp = null;
	}
	
	/**
	 * listener handler - 
	 * @author mat
	 *
	 */
	public class ListenerHandler extends Handler {
		
		/**
		 * called after when installer have new operation to notify
		 */
		/* only used in the default channel */
		public void onOperation(String distribName, String projectUrl, String opDescription) {
			
			mNotificationController.handleOnOperation(distribName, projectUrl, opDescription);
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperation(distribName, opDescription);
		}
		
		/**
		 * called when operation progress happens
		 * progress determined in range between 0 10000
		 */
		/* only used in the default channel */
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

		/**
		 * called when installer encounter problem while performing operation
		 */
		public void onOperationError(int channelId, InstallOp installOp, String distribName,
				String projectUrl, String errorMessage) {
			if (Logging.DEBUG) Log.d(TAG, "onError: "+channelId+","+distribName+","+errorMessage);
			PendingController<InstallOp> channel = mPendingChannels[channelId];
			
			if (installOp.equals(InstallOp.ProgressOperation) && channelId == DEFAULT_CHANNEL_ID) {
				// if installer operation (with progress)
				if (mApp.isInstallerRun())
					mApp.backToPreviousInstallerStage();
				
				mNotificationController.handleOnOperationError(distribName, projectUrl, errorMessage);
			}
			boolean called = false;
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners) {
				if (listener.getInstallerChannelId() != channelId)
					continue;
				if (listener.onOperationError(installOp, distribName, errorMessage)) {
					called = true;
				}
			}
			
			// enqueue only errors from simple operations
			if (!called && !installOp.equals(InstallOp.ProgressOperation))
				channel.finishWithError(installOp, new InstallError(distribName, projectUrl, errorMessage));
			else
				channel.finish(installOp);
		}
		
		/**
		 * called when operation was cancelled
		 */
		public void onOperationCancel(int channelId, InstallOp installOp,
				String distribName, String projectUrl) {
			if (Logging.DEBUG) Log.d(TAG, "onCancel: "+channelId+","+distribName);
			PendingController<InstallOp> channel = mPendingChannels[channelId];
			
			if (installOp.equals(InstallOp.ProgressOperation) && channelId == DEFAULT_CHANNEL_ID) {
				// if installer operation (with progress)
				if (mApp.isInstallerRun())
					mApp.backToPreviousInstallerStage();
				
				mNotificationController.handleOnOperationCancel(distribName, projectUrl);
			}
						
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener.getInstallerChannelId() == channelId &&
						listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperationCancel(installOp, distribName);
			
			channel.finish(installOp);
		}
		
		/**
		 * called when progress operation (installation, dumping, reinstall) was finished
		 */
		public void onOperationFinish(InstallOp installOp,String distribName, String projectUrl) {
			/* to next insraller stage */
			if (Logging.DEBUG) Log.d(TAG, "Finish operation:"+distribName);
			PendingController<InstallOp> channel = mPendingChannels[DEFAULT_CHANNEL_ID];
			
			if (installOp.equals(InstallOp.ProgressOperation)) {
				int installerStage = mApp.getInstallerStage(); 
				if (installerStage == BoincManagerApplication.INSTALLER_CLIENT_INSTALLING_STAGE)
					mApp.setInstallerStage(BoincManagerApplication.INSTALLER_PROJECT_STAGE);
				else if (installerStage == BoincManagerApplication.INSTALLER_PROJECT_INSTALLING_STAGE)
					mApp.setInstallerStage(BoincManagerApplication.INSTALLER_FINISH_STAGE);
			
				mNotificationController.handleOnOperationFinish(distribName, projectUrl);
			}
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperationFinish(installOp, distribName);
			
			channel.finish(installOp);
			
		}
		
		/**
		 * called when Project distributions retrieved from repository
		 */
		public void currentProjectDistribList(int channelId, InstallOp installOp,
				ArrayList<ProjectDistrib> projectDistribs) {
			
			if (Logging.DEBUG) Log.d(TAG, "onProjectDistribs: "+channelId);
			PendingController<InstallOp> channel = mPendingChannels[channelId];
			
			channel.finishWithOutput(installOp, projectDistribs);
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener.getInstallerChannelId() == channelId &&
						listener instanceof InstallerUpdateListener)
					((InstallerUpdateListener)listener).currentProjectDistribList(projectDistribs);
		}
		
		/**
		 * called when Client distribution retrieved from repository
		 */
		public void currentClientDistrib(int channelId, InstallOp installOp, ClientDistrib clientDistrib) {
			if (Logging.DEBUG) Log.d(TAG, "onClientDistrib: "+channelId);
			PendingController<InstallOp> channel = mPendingChannels[channelId];
			
			channel.finishWithOutput(installOp, clientDistrib);
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener.getInstallerChannelId() == channelId &&
						listener instanceof InstallerUpdateListener)
					((InstallerUpdateListener)listener).currentClientDistrib(clientDistrib);
		}
		
		public void notifyBinariesToInstallOrUpdate(int channelId, InstallOp installOp,
				UpdateItem[] updateItems) {
			PendingController<InstallOp> channel = mPendingChannels[channelId];
			
			channel.finishWithOutput(installOp, updateItems);
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			
			for (AbstractInstallerListener listener: listeners)
				if (listener.getInstallerChannelId() == channelId &&
						listener instanceof InstallerUpdateListener)
					((InstallerUpdateListener)listener).binariesToUpdateOrInstall(updateItems);
		}
		
		public void notifyBinariesToUpdateFromSDCard(int channelId, InstallOp installOp,
				String[] projectNames) {
			PendingController<InstallOp> channel = mPendingChannels[channelId];
			
			channel.finishWithOutput(installOp, projectNames);
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			
			for (AbstractInstallerListener listener: listeners)
				if (listener.getInstallerChannelId() == channelId &&
						listener instanceof InstallerUpdateListener)
					((InstallerUpdateListener)listener).binariesToUpdateFromSDCard(projectNames);
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
		InstallerHandler handler = mInstallerHandler;
		mInstallerHandler = null;
		handler.cancelSimpleOperationAlways(mInstallerThread);
		handler.cancelAllProgressOperations();
		mInstallerThread.stopThread();
		mInstallerThread = null;
		mListeners.clear();
		try {
			Thread.sleep(200);
		} catch(InterruptedException ex) { }
		handler.destroy();
	}
	
	/**
	 * resolve project name
	 */
	public String resolveProjectName(String projectUrl) {
		return mInstallerHandler.resolveProjectName(projectUrl);
	}
	
	/**
	 * get pending install error
	 * @return pending install error
	 */
	public boolean handlePendingErrors(final InstallOp installOp, final AbstractInstallerListener listener) {
		PendingController<InstallOp> channel = mPendingChannels[listener.getInstallerChannelId()]; 
		
		return channel.handlePendingErrors(installOp, new PendingErrorHandler<InstallOp>() {
			
			@Override
			public boolean handleError(InstallOp installOp, Object error) {
				if (error == null)
					return false;
				InstallError installError = (InstallError)error;
				return listener.onOperationError(installOp,
						installError.distribName, installError.errorMessage);
			}
		});
	}
	
	public Object getPendingOutput(InstallOp installOp) {
		return getPendingOutput(DEFAULT_CHANNEL_ID, installOp);
	}
	
	public Object getPendingOutput(int channelId, InstallOp installOp) {
		PendingController<InstallOp> channel = mPendingChannels[channelId];
		return channel.takePendingOutput(installOp);
	}
	
	public boolean isOperationBeingExecuted(InstallOp installOp) {
		return mPendingChannels[DEFAULT_CHANNEL_ID].isRan(installOp);
	}
	
	public boolean isOperationBeingExecuted(int channelId, InstallOp installOp) {
		return mPendingChannels[channelId].isRan(installOp);
	}
	
	/*
	 * some simple operations (calls) return boolean value
	 * indicates whether task has been ran (true) or not (false) 
	 */
	
	/* update client distrib info */
	public boolean updateClientDistrib() {
		return updateClientDistrib(DEFAULT_CHANNEL_ID);
	}
	
	public boolean updateClientDistrib(int channelId) {
		if (mInstallerHandler == null)
			return false;
		
		PendingController<InstallOp> channel = mPendingChannels[channelId];
		
		if (channel.begin(InstallOp.UpdateClientDistrib)) {
			mInstallerThread.updateClientDistrib(channelId);
			return true;
		}
		return false;
	}
	
	/**
	 * Install client automatically (with signature checking)
	 */
	public void installClientAutomatically() {
		if (mInstallerHandler == null) return;
		
		mPendingChannels[DEFAULT_CHANNEL_ID].begin(InstallOp.ProgressOperation);
		// always run it
		mNotificationController.notifyInstallClientBegin();
		mInstallerThread.installClientAutomatically();
	}
	
	/**
	 * Installs BOINC application automatically (with signature checking)
	 * @param projectUrl
	 * @param appName
	 */
	public void installProjectApplicationsAutomatically(String projectName, String projectUrl) {
		if (mInstallerHandler == null) return;
		
		mPendingChannels[DEFAULT_CHANNEL_ID].begin(InstallOp.ProgressOperation);
		// always run it
		mNotificationController.notifyInstallProjectBegin(projectName);
		mInstallerThread.installProjectApplicationAutomatically(projectUrl);
	}
	
	/**
	 * Reinstalls update item 
	 */
	public void reinstallUpdateItems(ArrayList<UpdateItem> updateItems) {
		if (mInstallerHandler == null) return;
		
		mPendingChannels[DEFAULT_CHANNEL_ID].begin(InstallOp.ProgressOperation);
		// always run it
		for (UpdateItem item: updateItems)
			if (item.name.equals(BOINC_CLIENT_ITEM_NAME))
				mNotificationController.notifyInstallClientBegin();
			else
				mNotificationController.notifyInstallProjectBegin(item.name);
		
		mInstallerThread.reinstallUpdateItems(updateItems);
	}
	
	public void updateDistribsFromSDCard(String dirPath, String[] distribNames) {
		if (mInstallerHandler == null) return;
		
		mPendingChannels[DEFAULT_CHANNEL_ID].begin(InstallOp.ProgressOperation);
		// always run it
		for (String distribName: distribNames)
			if (distribName.equals(BOINC_CLIENT_ITEM_NAME))
				mNotificationController.notifyInstallClientBegin();
			else
				mNotificationController.notifyInstallProjectBegin(distribName);
		
		mInstallerThread.updateDistribsFromSDCard(dirPath, distribNames);
	}
	
	public boolean updateProjectDistribList() {
		return updateProjectDistribList(DEFAULT_CHANNEL_ID);
	}
	
	public boolean updateProjectDistribList(int channelId) {
		if (mInstallerHandler == null) 
			return false;
		
		PendingController<InstallOp> channel = mPendingChannels[channelId];
		
		if (channel.begin(InstallOp.UpdateProjectDistribs)) { // run when not ran
			mInstallerThread.updateProjectDistribList(channelId);
			return true;
		}
		return false;
	}
	
	/* remove detached projects from installed projects */
	public void synchronizeInstalledProjects() {
		if (mInstallerHandler == null) return;
		
		mInstallerHandler.synchronizeInstalledProjects();
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
		if (mInstallerHandler == null)
			return null;
		
		return mInstallerHandler.getInstalledBinaries();
	}
	
	/**
	 * @param attachProjectUrls attached projects urls list 
	 */
	public boolean getBinariesToUpdateOrInstall() {
		return getBinariesToUpdateOrInstall(DEFAULT_CHANNEL_ID);
	}
	
	public boolean getBinariesToUpdateOrInstall(int channelId) {
		if (mInstallerHandler == null)
			return false;
		
		PendingController<InstallOp> channel = mPendingChannels[channelId];

		if (channel.begin(InstallOp.GetBinariesToInstall)) {
			mInstallerThread.getBinariesToUpdateOrInstall(channelId);
			return true;
		}
		return false;
	}
	
	public boolean getBinariesToUpdateFromSDCard(String path) {
		return getBinariesToUpdateFromSDCard(DEFAULT_CHANNEL_ID, path);
	}
	
	public boolean getBinariesToUpdateFromSDCard(int channelId, String path) {
		if (mInstallerHandler == null)
			return false;
		
		PendingController<InstallOp> channel = mPendingChannels[channelId];
		
		if (channel.begin(InstallOp.GetBinariesFromSDCard(path))) {
			mInstallerThread.getBinariesToUpdateFromSDCard(channelId, path);
			return true;
		}
		return false;
	}
	
	/**
	 * dump boinc files
	 * @return
	 */
	public void dumpBoincFiles(String directory) {
		if (mInstallerHandler == null) return;
		
		mPendingChannels[DEFAULT_CHANNEL_ID].begin(InstallOp.ProgressOperation);
		mInstallerThread.dumpBoincFiles(directory); 
	}
	
	public boolean isBeingDumpedFiles() {
		if (mInstallerHandler == null)
			return false;
		
		return mInstallerHandler.isBeingDumpedFiles();
	}
	
	public void reinstallBoinc() {
		if (mInstallerHandler == null) return;
		
		mPendingChannels[DEFAULT_CHANNEL_ID].begin(InstallOp.ProgressOperation);
		mInstallerThread.reinstallBoinc();
	}
	
	public boolean isBeingReinstalled() {
		if (mInstallerHandler == null)
			return false;
		
		return mInstallerHandler.isBeingReinstalled();
	}
	
	public boolean isWorking() {
		if (mInstallerHandler == null) return false;
		
		return mInstallerHandler.isWorking();
	}


	/**
	 * cancelling routines
	 */
	public void cancelAllProgressOperations() {
		if (mInstallerHandler == null) return;
		
		mInstallerHandler.cancelAllProgressOperations();
	}
	
	public void cancelClientInstallation() {
		if (mInstallerHandler == null) return;
		
		mInstallerHandler.cancelClientInstallation();
	}
	
	public void cancelProjectAppsInstallation(String projectUrl) {
		if (mInstallerHandler == null) return;
		
		mInstallerHandler.cancelProjectAppsInstallation(projectUrl);
	}
	
	public void cancelDumpFiles() {
		if (mInstallerHandler == null) return;
		
		mInstallerHandler.cancelDumpFiles();
	}
	
	public void cancelReinstallation() {
		if (mInstallerHandler == null) return;
		
		mInstallerHandler.cancelReinstallation();
	}
	
	public void cancelSimpleOperation() { // cancel in default channel
		if (mInstallerHandler == null) return;
		
		mInstallerHandler.cancelSimpleOperation(DEFAULT_CHANNEL_ID, mInstallerThread);
	}
	
	public void cancelSimpleOperation(int channelId) {
		if (mInstallerHandler == null) return;
		
		mInstallerHandler.cancelSimpleOperation(channelId, mInstallerThread);
	}
	
	public void cancelSimpleOperationAlways() { // used by destroy
		if (mInstallerHandler == null) return;
		
		mInstallerHandler.cancelSimpleOperationAlways(mInstallerThread);
	}
		
	/**
	 * get progress item
	 * @return distrib progresses (not sorted)
	 */
	public synchronized ProgressItem[] getProgressItems() {
		if (mNotificationController != null)
			return mNotificationController.getProgressItems();
		else
			return null;
	}
	
	/* get current status */
	public synchronized ProgressItem getProgressItem(String distribName) {
		if (mNotificationController != null)
			return mNotificationController.getProgressItem(distribName);
		else
			return null;
	}
}
