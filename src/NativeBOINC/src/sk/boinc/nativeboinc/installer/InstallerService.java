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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.ArrayList;

import sk.boinc.nativeboinc.AddProjectActivity;
import sk.boinc.nativeboinc.ProgressActivity;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.UpdateActivity;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.util.NotificationId;
import sk.boinc.nativeboinc.util.ProgressItem;
import sk.boinc.nativeboinc.util.UpdateItem;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
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
import android.widget.RemoteViews;

/**
 * @author mat
 *
 */
public class InstallerService extends Service {
	private static final String TAG = "InstallerService";
	
	public static final String BOINC_CLIENT_ITEM_NAME = "BOINC client";
	
	private NotificationManager mNotificationManager = null;
	
	private class DistribNotification {
		public int notificationId;
		public Notification notification;
		public RemoteViews contentView;
		
		public DistribNotification(int notificationId, Notification notification,
				RemoteViews contentView) {
			this.notificationId = notificationId;
			this.notification = notification;
			this.contentView = contentView;
		}
	}
	
	private DistribNotification mClientInstallNotification = null;
	
	private Map<String, DistribNotification> mProjectInstallNotifications =
			new HashMap<String, DistribNotification>();
	
	private Map<String, ProgressItem> mDistribInstallProgresses =
			new HashMap<String, ProgressItem>();
	
	private int mCurrentProjectInstallNotificationId = 0;
	
	private InstallerThread mInstallerThread = null;
	private InstallerHandler mInstallerHandler = null;
	
	private Context mAppContext = null;
	
	private boolean mAtUpdating = false;
	
	private List<AbstractInstallerListener> mListeners = new ArrayList<AbstractInstallerListener>();
	
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
		unbindService(mRunnerServiceConnection);
		mRunner = null;
	}
	
	@Override
	public void onCreate() {
		mListenerHandler = new ListenerHandler();
		mNotificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
		mAppContext = getApplicationContext();
		startInstaller();
		doBindRunnerService();
	}
	
	@Override
	public void onDestroy() {
		doUnbindRunnerService();
		stopInstaller();
	}
	
	@Override
	public IBinder onBind(Intent intent) {
		startService(new Intent(this, InstallerService.class));
		return mBinder;
	}
	
	/**
	 * listener handler - 
	 * @author mat
	 *
	 */
	public class ListenerHandler extends Handler {
		
		public void onOperation(String distribName, String projectUrl, String opDescription) {
			updateDistribInstallProgress(distribName, projectUrl, opDescription, -1,
					ProgressItem.STATE_IN_PROGRESS);
			
			if (distribName.equals(BOINC_CLIENT_ITEM_NAME))
				notifyInstallClientOperation(opDescription);
			// ignore non distrib notifications
			else if (distribName.length()!=0)
				notifyInstallProjectAppsOperation(distribName, opDescription);
			
			for (AbstractInstallerListener listener: mListeners)
				if (listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperation(distribName, opDescription);
		}
		
		public void onOperationProgress(String distribName, String projectUrl,
				String opDescription,int progress) {
			
			updateDistribInstallProgress(distribName, projectUrl, opDescription, progress,
					ProgressItem.STATE_IN_PROGRESS);
			
			if (distribName.equals(BOINC_CLIENT_ITEM_NAME))
				notifyInstallClientProgress(opDescription, progress);
			// ignore non distrib notifications
			else if (distribName.length()!=0)
				notifyInstallProjectAppsProgress(distribName, opDescription, progress);
			
			for (AbstractInstallerListener listener: mListeners)
				if (listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperationProgress(
							distribName, opDescription, progress);
		}

		public void onOperationError(String distribName, String projectUrl, String errorMessage) {
			updateDistribInstallProgress(distribName, projectUrl, errorMessage, -1,
					ProgressItem.STATE_ERROR_OCCURRED);
			
			if (distribName.equals(BOINC_CLIENT_ITEM_NAME))
				notifyInstallClientOperation(errorMessage);
			// ignore non distrib notifications
			else if (distribName.length()!=0)
				notifyInstallProjectAppsOperation(distribName, errorMessage);
			
			for (AbstractInstallerListener listener: mListeners)
				if (listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperationError(
							distribName, errorMessage);
		}
		
		public void onOperationCancel(String distribName, String projectUrl) {
			updateDistribInstallProgress(distribName, projectUrl, null, -1, 
					ProgressItem.STATE_CANCELLED);

			if (distribName.equals(BOINC_CLIENT_ITEM_NAME))
				notifyInstallClientOperation(getString(R.string.operationCancelled));
			// ignore non distrib notifications
			else if (distribName.length()!=0)
				notifyInstallProjectAppsOperation(distribName,
						getString(R.string.operationCancelled));
			
			for (AbstractInstallerListener listener: mListeners)
				if (listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperationCancel(distribName);
		}
		
		public void onOperationFinish(String distribName, String projectUrl) {
			updateDistribInstallProgress(distribName, projectUrl, null, -1, 
					ProgressItem.STATE_FINISHED);
			
			if (distribName.equals(BOINC_CLIENT_ITEM_NAME))
				notifyInstallClientFinish();
			else
				notifyInstallProjectAppsFinish(distribName);
			
			for (AbstractInstallerListener listener: mListeners)
				if (listener instanceof InstallerProgressListener)
					((InstallerProgressListener)listener).onOperationFinish(distribName);
		}
		
		public void currentProjectDistribList(ArrayList<ProjectDistrib> projectDistribs) {
			for (AbstractInstallerListener listener: mListeners)
				if (listener instanceof InstallerUpdateListener)
					((InstallerUpdateListener)listener).currentProjectDistribList(projectDistribs);
		}
		
		public void currentClientDistrib(ClientDistrib clientDistrib) {
			for (AbstractInstallerListener listener: mListeners)
				if (listener instanceof InstallerUpdateListener)
					((InstallerUpdateListener)listener).currentClientDistrib(clientDistrib);
		}
	}
	
	private ListenerHandler mListenerHandler = null;
	
	public void addInstallerListener(AbstractInstallerListener listener) {
		mListeners.add(listener);
	}
	
	public void removeInstallerListener(AbstractInstallerListener listener) {
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
	
	/*
	 * adds, update distrib install progresses 
	 */
	private synchronized void updateDistribInstallProgress(String distribName,
			String projectUrl, String opDesc, int progress, int state) {
		ProgressItem toUpdate = mDistribInstallProgresses.get(distribName);
		if (toUpdate!=null) {
			toUpdate.opDesc = opDesc;
			toUpdate.progress = progress;
			toUpdate.state = state;
		} else {	// adds new progress item
			if (distribName != null && distribName.length() != 0)
				mDistribInstallProgresses.put(distribName, 
						new ProgressItem(distribName, projectUrl, opDesc, -1,
								ProgressItem.STATE_IN_PROGRESS));
		}
	}
	
	public synchronized void clearDistribInstallProgresses() {
		mDistribInstallProgresses.clear();
	}
	
	/**
	 * Notificaitions 
	 */
	private void notifyInstallClientBegin() {
		Intent intent = null;
		intent = new Intent(this, ProgressActivity.class);
		
		PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, 0);
		RemoteViews contentView = new RemoteViews(getPackageName(), R.layout.install_notification);
		
		String notifyText = getString(R.string.installClientNotifyBegin);
		
		Notification notification = new Notification(R.drawable.nativeboinc, notifyText,
				System.currentTimeMillis());
		mClientInstallNotification = new DistribNotification(NotificationId.INSTALL_BOINC_CLIENT,
				notification, contentView);
		notification.setLatestEventInfo(mAppContext, notifyText, notifyText, pendingIntent);
		
		mNotificationManager.notify(NotificationId.INSTALL_BOINC_CLIENT, notification);
	}
	
	private void notifyInstallClientOperation(String description) {
		String notifyText = BOINC_CLIENT_ITEM_NAME + ": " + description;
		
		mClientInstallNotification.notification.setLatestEventInfo(this, notifyText, notifyText,
				mClientInstallNotification.notification.contentIntent);
		mNotificationManager.notify(NotificationId.INSTALL_BOINC_CLIENT,
				mClientInstallNotification.notification);
	}
	
	private void notifyInstallClientProgress(String description, int progress) {
		String notifyText = BOINC_CLIENT_ITEM_NAME + ": " + description;
		
		mClientInstallNotification.notification.contentView = mClientInstallNotification.contentView;
		mClientInstallNotification.contentView.setProgressBar(R.id.operationProgress,
				10000, progress, false);
		mClientInstallNotification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		mNotificationManager.notify(NotificationId.INSTALL_BOINC_CLIENT,
				mClientInstallNotification.notification);
	}
	
	private void notifyInstallClientFinish() {
		mNotificationManager.cancel(NotificationId.INSTALL_BOINC_CLIENT);
		mClientInstallNotification = null;
	}
	
	private void notifyInstallProjectBegin(String projectName) {
		Intent intent = null;
		if (!mAtUpdating)
			intent = new Intent(this, AddProjectActivity.class);
		else
			intent = new Intent(this, UpdateActivity.class);
		
		PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, 0);
		RemoteViews contentView = new RemoteViews(getPackageName(), R.layout.install_notification);
		
		String notifyText = getString(R.string.installProjectNotifyBegin) + " " + projectName;
		
		Notification notification = new Notification(R.drawable.nativeboinc, notifyText,
				System.currentTimeMillis());
		
		int notificationId = mCurrentProjectInstallNotificationId++;
		
		mProjectInstallNotifications.put(projectName, new DistribNotification(notificationId,
				notification, contentView));
		notification.setLatestEventInfo(mAppContext, notifyText, notifyText, pendingIntent);
		
		mNotificationManager.notify(notificationId, notification);
	}
	
	private void notifyInstallProjectAppsOperation(String projectName, String description) {
		DistribNotification notification = mProjectInstallNotifications.get(projectName);
		
		if (notification != null) {
			String notifyText = projectName + ": " + description;
			
			mClientInstallNotification.notification.setLatestEventInfo(this, notifyText,
					notifyText, notification.notification.contentIntent);
			mNotificationManager.notify(notification.notificationId, notification.notification);
		}
	}
	
	private void notifyInstallProjectAppsProgress(String projectName,
			String description, int progress) {
		DistribNotification notification = mProjectInstallNotifications.get(projectName);
		if (notification != null) {
			String notifyText = projectName + ": " + description;
			
			notification.notification.contentView = notification.contentView;
			notification.contentView.setProgressBar(R.id.operationProgress,
					10000, progress, false);
			notification.contentView.setTextViewText(R.id.operationDesc, notifyText);
			mNotificationManager.notify(notification.notificationId, notification.notification);
		}
	}
	
	private void notifyInstallProjectAppsFinish(String projectName) {
		DistribNotification notification = mProjectInstallNotifications.remove(projectName);
		if (notification != null)
			mNotificationManager.cancel(notification.notificationId);
	}
	
	/**
	 * Install client automatically (with signature checking)
	 */
	public void installClientAutomatically() {
		mAtUpdating = false;
		notifyInstallClientBegin();
		mInstallerThread.installClientAutomatically();
	}
	
	/* update client distrib info */
	public void updateClientDistrib() {
		mInstallerThread.updateClientDistrib();
	}
	
	/**
	 * Installs BOINC application automatically (with signature checking)
	 * @param projectUrl
	 * @param appName
	 */
	public void installProjectApplicationsAutomatically(String projectName, String projectUrl) {
		mAtUpdating = false;
		mCurrentProjectInstallNotificationId = NotificationId.INSTALL_PROJECT_APPS_BASE;
		notifyInstallProjectBegin(projectName);
		mInstallerThread.installBoincApplicationAutomatically(projectUrl);
	}
	
	/**
	 * Reinstalls update item 
	 */
	public void reinstallUpdateItems(ArrayList<UpdateItem> updateItems) {
		mAtUpdating = true;
		mCurrentProjectInstallNotificationId = NotificationId.INSTALL_PROJECT_APPS_BASE;
		
		for (UpdateItem item: updateItems)
			if (item.name.equals(BOINC_CLIENT_ITEM_NAME))
				notifyInstallClientBegin();
			else
				notifyInstallProjectBegin(item.name);
			
		mInstallerThread.reinstallUpdateItems(updateItems);
	}
	
	public void updateProjectDistribList() {
		mInstallerThread.updateProjectDistribList();
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
	
	/**
	 * Check whether client is installed
	 * @return
	 */
	public boolean isClientInstalled() {
		return mInstallerHandler.isClientInstalled();
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
	public UpdateItem[] getBinariesToUpdateOrInstall(String[] attachedProjectUrls) {
		return mInstallerHandler.getBinariesToUpdateOrInstall(attachedProjectUrls);
	}
	
	public boolean isWorking() {
		return mInstallerHandler.isWorking();
	}
	
	/**
	 * get currently installed distrib (progresses)
	 * @return distrib progresses (not sorted)
	 */
	public synchronized ProgressItem[] getCurrentlyInstalledBinaries() {
		if (mDistribInstallProgresses.isEmpty())
			return null;
		
		ProgressItem[] installProgresses = new ProgressItem[mDistribInstallProgresses.size()];
		
		int index = 0;
		for (ProgressItem progress: mDistribInstallProgresses.values()) {
			installProgresses[index] = new ProgressItem(progress.name, progress.projectUrl,
					progress.opDesc, progress.progress, progress.state);
			index++;
		}
		
		return installProgresses;
	}
	
	/* get current status */
	public synchronized ProgressItem getCurrentStatusForDistribInstall(String distribName) {
		ProgressItem progress = mDistribInstallProgresses.get(distribName);
		if (progress == null)
			return null;
		return new ProgressItem(progress.name, progress.projectUrl, progress.opDesc,
				progress.progress, progress.state);
	}
}
