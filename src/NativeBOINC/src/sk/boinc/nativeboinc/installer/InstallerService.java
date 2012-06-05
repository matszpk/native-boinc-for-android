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
import java.util.concurrent.atomic.AtomicReference;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.NotificationController;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
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
	public static final String BOINC_DUMP_ITEM_NAME = ":::BOINC Dump Files";
	public static final String BOINC_REINSTALL_ITEM_NAME = ":::BOINC Reinstall";
	public static final String BOINC_CLIENT_DISTRIB_UPDATE_ITEM_NAME = "::!cdist";
	public static final String BOINC_PROJECTS_DISTRIBS_UPDATE_ITEM_NAME = "::!pdist";
	public static final String BOINC_IGNORE_ITEM_PREFIX = "::!"; // should be ignored by progress activity
	
	// determine wheter distrib is normal simple install operation
	public static boolean isSimpleOperation(String distribName) {
		return (distribName == null || distribName.length() == 0 ||
				distribName.startsWith(BOINC_IGNORE_ITEM_PREFIX));
	}
	
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
	private static final class PendingSimpleChannel {
		public AtomicReference<ArrayList<ProjectDistrib>> newProjectDistribs =
				new AtomicReference<ArrayList<ProjectDistrib>>();
		
		public AtomicReference<ClientDistrib> clientDistrib = new AtomicReference<ClientDistrib>();
		
		public AtomicReference<UpdateItem[]> updateItems = new AtomicReference<UpdateItem[]>();
		
		public AtomicReference<String[]> updateProjNames = new AtomicReference<String[]>();
		
		/* pending errors used during working simple installer operations (not progress operations) */
		public InstallError installError = null;
		public Object installErrorSync = new Object();
	};
	
	private PendingSimpleChannel[] mPendingChannels = null;
	
	
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
		
		mPendingChannels = new PendingSimpleChannel[MAX_CHANNEL_ID];
		for (int i = 0; i < MAX_CHANNEL_ID; i++)
			mPendingChannels[i] = new PendingSimpleChannel();
		
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
		mInstallerHandler = null;
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
		public void onOperation(String distribName, String projectUrl,
				String opDescription) {
			if (!isSimpleOperation(distribName))
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
			if (!isSimpleOperation(distribName))
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
		public void onOperationError(int channelId, String distribName, String projectUrl,
				String errorMessage) {
			if (Logging.DEBUG) Log.d(TAG, "onError: "+channelId+","+distribName+","+errorMessage);
			PendingSimpleChannel channel = mPendingChannels[channelId];
			
			if (!isSimpleOperation(distribName) && channelId == DEFAULT_CHANNEL_ID) {
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
				if (listener.onOperationError(distribName, errorMessage))
					called = true;
			}
			
			// enqueue only errors from simple operations
			if (isSimpleOperation(distribName))
				synchronized(channel.installErrorSync) {
					if (Logging.DEBUG) Log.d(TAG, "Is PendingInstallError set: "+ (!called)+
							", channel: "+channelId);
					if (!called) /* set up pending if not already handled */
						channel.installError = new InstallError(distribName, projectUrl, errorMessage);
					else // if already handled
						channel.installError = null;
				}
		}
		
		/**
		 * called when operation was cancelled
		 */
		public void onOperationCancel(int channelId, String distribName, String projectUrl) {
			if (Logging.DEBUG) Log.d(TAG, "onCancel: "+channelId+","+distribName);
			
			if (!isSimpleOperation(distribName) && channelId == DEFAULT_CHANNEL_ID) {
				// if installer operation (with progress)
				if (mApp.isInstallerRun())
					mApp.backToPreviousInstallerStage();
				
				mNotificationController.handleOnOperationCancel(distribName, projectUrl);
			}
						
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener.getInstallerChannelId() == channelId &&
						listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperationCancel(distribName);
		}
		
		/**
		 * called when progress operation (installation, dumping, reinstall) was finished
		 */
		public void onOperationFinish(String distribName, String projectUrl) {
			/* to next insraller stage */
			if (Logging.DEBUG) Log.d(TAG, "Finish operation:"+distribName);
			
			if (!isSimpleOperation(distribName)) {
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
					((InstallerProgressListener)listener).onOperationFinish(distribName);
		}
		
		/**
		 * called when Project distributions retrieved from repository
		 */
		public void currentProjectDistribList(int channelId, ArrayList<ProjectDistrib> projectDistribs) {
			if (Logging.DEBUG) Log.d(TAG, "onProjectDistribs: "+channelId);
			PendingSimpleChannel channel = mPendingChannels[channelId];
			
			channel.newProjectDistribs.set(projectDistribs);
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener.getInstallerChannelId() == channelId &&
						listener instanceof InstallerUpdateListener)
					((InstallerUpdateListener)listener).currentProjectDistribList(projectDistribs);
		}
		
		/**
		 * called when Client distribution retrieved from repository
		 */
		public void currentClientDistrib(int channelId, ClientDistrib clientDistrib) {
			if (Logging.DEBUG) Log.d(TAG, "onClientDistrib: "+channelId);
			PendingSimpleChannel channel = mPendingChannels[channelId];
			
			channel.clientDistrib.set(clientDistrib);
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			for (AbstractInstallerListener listener: listeners)
				if (listener.getInstallerChannelId() == channelId &&
						listener instanceof InstallerUpdateListener)
					((InstallerUpdateListener)listener).currentClientDistrib(clientDistrib);
		}
		
		public void notifyBinariesToInstallOrUpdate(int channelId, UpdateItem[] updateItems) {
			PendingSimpleChannel channel = mPendingChannels[channelId];
			
			channel.updateItems.set(updateItems);
			
			AbstractInstallerListener[] listeners = mListeners.toArray(new AbstractInstallerListener[0]);
			
			for (AbstractInstallerListener listener: listeners)
				if (listener.getInstallerChannelId() == channelId &&
						listener instanceof InstallerUpdateListener)
					((InstallerUpdateListener)listener).binariesToUpdateOrInstall(updateItems);
		}
		
		public void notifyBinariesToUpdateFromSDCard(int channelId, String[] projectNames) {
			PendingSimpleChannel channel = mPendingChannels[channelId];
			
			channel.updateProjNames.set(projectNames);
			
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
		mInstallerHandler.cancelSimpleOperationAlways(mInstallerThread);
		mInstallerHandler.cancelAllProgressOperations();
		mInstallerThread.stopThread();
		mInstallerThread = null;
		mListeners.clear();
		try {
			Thread.sleep(200);
		} catch(InterruptedException ex) { }
		mInstallerHandler.destroy();
		mListenerHandler = null;
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
	public boolean handlePendingError(AbstractInstallerListener listener) {
		PendingSimpleChannel channel = mPendingChannels[listener.getInstallerChannelId()]; 
		
		synchronized (channel.installErrorSync) {
			if (channel.installError == null)
				return false;
			InstallError installError = channel.installError;
			if (listener.onOperationError(installError.distribName, installError.errorMessage)) {
				if (Logging.DEBUG) Log.d(TAG, "PendingInstallError is handled, channel: "+
							listener.getInstallerChannelId());
				channel.installError = null;
				return true;
			}
			return false;
		}
	}
	
	private void clearPendingError(int channelId) {
		PendingSimpleChannel channel = mPendingChannels[channelId];
		
		synchronized(channel.installErrorSync) {
			channel.installError = null;
		}
	}
	
	/* update client distrib info */
	public void updateClientDistrib() {
		updateClientDistrib(DEFAULT_CHANNEL_ID);
	}
	
	public void updateClientDistrib(int channelId) {
		PendingSimpleChannel channel = mPendingChannels[channelId];
		
		
		channel.clientDistrib.set(null);
		
		clearPendingError(channelId);
		mInstallerThread.updateClientDistrib(channelId);
	}
	
	public ClientDistrib getPendingClientDistrib() {
		return getPendingClientDistrib(DEFAULT_CHANNEL_ID);
	}
	
	public ClientDistrib getPendingClientDistrib(int channelId) {
		PendingSimpleChannel channel = mPendingChannels[channelId];
		
		return channel.clientDistrib.getAndSet(null);
	}
	
	/**
	 * Install client automatically (with signature checking)
	 */
	public void installClientAutomatically() {
		mNotificationController.notifyInstallClientBegin();
		mInstallerThread.installClientAutomatically();
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
		updateProjectDistribList(DEFAULT_CHANNEL_ID);
	}
	
	public void updateProjectDistribList(int channelId) {
		PendingSimpleChannel channel = mPendingChannels[channelId];
		
		channel.newProjectDistribs.set(null);
		
		clearPendingError(channelId);
		mInstallerThread.updateProjectDistribList(channelId);
	}
	
	public ArrayList<ProjectDistrib> getPendingNewProjectDistribList() {
		return getPendingNewProjectDistribList(DEFAULT_CHANNEL_ID);
	}
	
	public ArrayList<ProjectDistrib> getPendingNewProjectDistribList(int channelId) {
		PendingSimpleChannel channel = mPendingChannels[channelId];
		
		return channel.newProjectDistribs.getAndSet(null);
	}
	
	/* remove detached projects from installed projects */
	public void synchronizeInstalledProjects() {
		mInstallerHandler.synchronizeInstalledProjects();
	}
	
	public void cancelAllProgressOperations() {
		mInstallerHandler.cancelAllProgressOperations();
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
	
	public void cancelReinstallation() {
		mInstallerHandler.cancelReinstallation();
	}
	
	public void cancelSimpleOperation() { // cancel in default channel
		mInstallerHandler.cancelSimpleOperation(DEFAULT_CHANNEL_ID, mInstallerThread);
	}
	
	public void cancelSimpleOperation(int channelId) {
		mInstallerHandler.cancelSimpleOperation(channelId, mInstallerThread);
	}
	
	public void cancelSimpleOperationAlways() { // used by destroy
		mInstallerHandler.cancelSimpleOperationAlways(mInstallerThread);
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
		getBinariesToUpdateOrInstall(DEFAULT_CHANNEL_ID);
	}
	
	public void getBinariesToUpdateOrInstall(int channelId) {
		PendingSimpleChannel channel = mPendingChannels[channelId];
		
		channel.updateItems.set(null);
		
		clearPendingError(channelId);
		mInstallerThread.getBinariesToUpdateOrInstall(channelId);
	}
	
	public UpdateItem[] getPendingBinariesToUpdateOrInstall() {
		return getPendingBinariesToUpdateOrInstall(DEFAULT_CHANNEL_ID);
	}
	
	public UpdateItem[] getPendingBinariesToUpdateOrInstall(int channelId) {
		PendingSimpleChannel channel = mPendingChannels[channelId];
		
		return channel.updateItems.getAndSet(null);
	}
	
	public void getBinariesToUpdateFromSDCard(String path) {
		getBinariesToUpdateFromSDCard(DEFAULT_CHANNEL_ID, path);
	}
	
	public void getBinariesToUpdateFromSDCard(int channelId, String path) {
		PendingSimpleChannel channel = mPendingChannels[channelId];
		
		channel.updateProjNames.set(null);
		
		clearPendingError(channelId);
		mInstallerThread.getBinariesToUpdateFromSDCard(channelId, path);
	}
	
	public String[] getPendingBinariesToUpdateFromSDCard() {
		return getPendingBinariesToUpdateFromSDCard(DEFAULT_CHANNEL_ID);
	}
	
	public String[] getPendingBinariesToUpdateFromSDCard(int channelId) {
		PendingSimpleChannel channel = mPendingChannels[channelId];
		
		return channel.updateProjNames.getAndSet(null);
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
