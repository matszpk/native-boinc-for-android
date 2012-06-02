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

package sk.boinc.nativeboinc;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.util.NotificationId;
import sk.boinc.nativeboinc.util.ProgressItem;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.widget.RemoteViews;

/**
 * @author mat
 *
 */
public class NotificationController {

	//private static final String TAG = "NotificationsController";
	
	private NotificationManager mNotificationManager = null;
	
	private Context mAppContext = null;
	
	private AtomicInteger mCurrentProjectInstallNotificationId =
			new AtomicInteger(NotificationId.INSTALL_PROJECT_APPS_BASE);
	
	public class DistribNotification {
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
	private DistribNotification mDumpFilesNotification = null;
	
	private DistribNotification mReinstallNotification = null;
	
	private Map<String, DistribNotification> mProjectInstallNotifications =
			new HashMap<String, DistribNotification>();
	
	private Map<String, ProgressItem> mDistribInstallProgresses =
			new HashMap<String, ProgressItem>();
	
	private Notification mClientEventNotification = null;
	
	
	public NotificationController(Context appContext) {
		mAppContext = appContext;
		mNotificationManager = (NotificationManager)appContext.getSystemService(
				Context.NOTIFICATION_SERVICE);
	}
	
	/**
	 * 
	 * Installer Service notifications
	 * because installer notifications has wider lifecycle, we use different object, which
	 * should be attached on application object
	 */
	
	/*
	 * adds, update distrib install progresses 
	 */
	public synchronized void updateDistribInstallProgress(String distribName,
			String projectUrl, String opDesc, int progress, int state) {
		ProgressItem toUpdate = mDistribInstallProgresses.get(distribName);
		if (toUpdate!=null) {
			toUpdate.opDesc = opDesc;
			toUpdate.progress = progress;
			toUpdate.state = state;
		} else {	// adds new progress item
			if (distribName != null && distribName.length() != 0)
				mDistribInstallProgresses.put(distribName, 
						new ProgressItem(distribName, projectUrl, opDesc, -1, state));
		}
	}
	
	/**
	 * remove and cancels all notifications for cancelled, aborted and finished tasks
	 */
	public synchronized void removeAllNotInProgress() {
		ArrayList<String> toRemove = new ArrayList<String>();
		
		for (Map.Entry<String,ProgressItem> entry: mDistribInstallProgresses.entrySet()) {
			ProgressItem item = entry.getValue();
			if (item.state != ProgressItem.STATE_IN_PROGRESS)
				toRemove.add(entry.getKey());
		}
		for (String url: toRemove) {
			ProgressItem item = mDistribInstallProgresses.remove(url);
			// cancel notifications
			if (url.equals(InstallerService.BOINC_CLIENT_ITEM_NAME)) {
				mNotificationManager.cancel(NotificationId.INSTALL_BOINC_CLIENT);
				mClientInstallNotification = null;
			} else {
				/* remove notifications */
				DistribNotification notification = mProjectInstallNotifications.remove(item.name);
				if (notification != null)
					mNotificationManager.cancel(notification.notificationId);
			}
		}
	}
	
	/*
	 * create new or reuse existing notification for client
	 */
	private DistribNotification getClientNotification(boolean emptyIntent) {
		Intent intent = null;
		if (!emptyIntent) {
			intent = new Intent(mAppContext, ProgressActivity.class);
			intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		} else
			intent = new Intent();
		
		PendingIntent pendingIntent = PendingIntent.getActivity(mAppContext, 0, intent, 0);
		
		if (mClientInstallNotification != null) {
			mClientInstallNotification.notification.when = System.currentTimeMillis();
			mClientInstallNotification.notification.contentIntent = pendingIntent;
			return mClientInstallNotification;
		}
		
		RemoteViews contentView = new RemoteViews(mAppContext.getPackageName(),
				R.layout.install_notification);
		Notification notification = new Notification(R.drawable.ic_download,
				mAppContext.getString(R.string.installClientNotifyBegin),
				System.currentTimeMillis());
		mClientInstallNotification = new DistribNotification(NotificationId.INSTALL_BOINC_CLIENT,
				notification, contentView);
		
		notification.contentIntent = pendingIntent;
		
		return mClientInstallNotification;
	}
	
	/**
	 * Notificaitions for client
	 */
	public synchronized void notifyInstallClientBegin() {
		String notifyText = mAppContext.getString(R.string.installClientNotifyBegin);
		
		DistribNotification clientNotification = getClientNotification(false);
		
		clientNotification.notification.tickerText = notifyText;
		clientNotification.notification.contentView = clientNotification.contentView;
		clientNotification.contentView.setProgressBar(R.id.operationProgress,
				10000, 0, true);
		clientNotification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		
		clientNotification.notification.flags &= ~Notification.FLAG_ONLY_ALERT_ONCE;
		clientNotification.notification.flags &= ~Notification.FLAG_AUTO_CANCEL;
		
		mNotificationManager.notify(NotificationId.INSTALL_BOINC_CLIENT,
				clientNotification.notification);
	}
	
	public synchronized void notifyInstallClientOperation(String description) {
		String notifyText = InstallerService.BOINC_CLIENT_ITEM_NAME + ": " + description;
		
		DistribNotification clientNotification = getClientNotification(false);
		
		clientNotification.notification.tickerText = notifyText;
		clientNotification.notification.contentView = clientNotification.contentView;
		clientNotification.contentView.setProgressBar(R.id.operationProgress,
				10000, 0, true);
		clientNotification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		
		clientNotification.notification.flags |= Notification.FLAG_ONLY_ALERT_ONCE;
		clientNotification.notification.flags &= ~Notification.FLAG_AUTO_CANCEL;
		
		mNotificationManager.notify(NotificationId.INSTALL_BOINC_CLIENT,
				clientNotification.notification);
	}
	
	public synchronized void notifyInstallClientProgress(String description, int progress) {
		String notifyText = InstallerService.BOINC_CLIENT_ITEM_NAME + ": " + description;
		
		DistribNotification clientNotification = getClientNotification(false);
		
		clientNotification.notification.tickerText = notifyText;
		clientNotification.notification.contentView = clientNotification.contentView;
		clientNotification.contentView.setProgressBar(R.id.operationProgress,
				10000, progress, false);
		clientNotification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		
		clientNotification.notification.flags |= Notification.FLAG_ONLY_ALERT_ONCE;
		clientNotification.notification.flags &= ~Notification.FLAG_AUTO_CANCEL;
		
		mNotificationManager.notify(NotificationId.INSTALL_BOINC_CLIENT,
				clientNotification.notification);
	}
	
	public synchronized void notifyInstallClientFinish(String description) {
		Notification notification = getClientNotification(true).notification;
		
		notification.tickerText = description;
		notification.setLatestEventInfo(mAppContext, description, description,
				notification.contentIntent);
		
		notification.flags |= Notification.FLAG_ONLY_ALERT_ONCE | Notification.FLAG_AUTO_CANCEL;
		
		mNotificationManager.notify(NotificationId.INSTALL_BOINC_CLIENT, notification);
	}
	
	/**
	 * dump boinc files: notifications handling
	 */
	private DistribNotification getDumpFilesNotification(boolean emptyIntent) {
		Intent intent = null;
		if (!emptyIntent) {
			intent = new Intent(mAppContext, ProgressActivity.class);
			intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		} else
			intent = new Intent();
		
		PendingIntent pendingIntent = PendingIntent.getActivity(mAppContext, 0, intent, 0);
		
		if (mDumpFilesNotification != null) {
			mDumpFilesNotification.notification.when = System.currentTimeMillis();
			mDumpFilesNotification.notification.contentIntent = pendingIntent;
			return mDumpFilesNotification;
		}
		
		RemoteViews contentView = new RemoteViews(mAppContext.getPackageName(),
				R.layout.install_notification);
		
		Notification notification = new Notification(R.drawable.ic_progress,
				mAppContext.getString(R.string.dumpBoincBegin),
				System.currentTimeMillis());
		mDumpFilesNotification = new DistribNotification(NotificationId.INSTALL_DUMP_FILES,
				notification, contentView);
		
		notification.contentIntent = pendingIntent;
		
		return mDumpFilesNotification;
	}
	
	public synchronized void notifyDumpFilesOperation(String notifyText) {
		DistribNotification dumpNotification = getDumpFilesNotification(false);
		
		dumpNotification.notification.tickerText = notifyText;
		dumpNotification.notification.contentView = dumpNotification.contentView;
		dumpNotification.contentView.setProgressBar(R.id.operationProgress,
				10000, 0, true);
		dumpNotification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		
		dumpNotification.notification.flags &= ~Notification.FLAG_ONLY_ALERT_ONCE;
		dumpNotification.notification.flags &= ~Notification.FLAG_AUTO_CANCEL;
		
		mNotificationManager.notify(NotificationId.INSTALL_DUMP_FILES,
				dumpNotification.notification);
	}
	
	public synchronized void notifyDumpFilesProgress(String filePath, int progress) {
		String notifyText = mAppContext.getString(R.string.dumpBoincProgress, filePath);
		
		DistribNotification dumpNotification = getDumpFilesNotification(false);
		
		dumpNotification.notification.tickerText = notifyText;
		dumpNotification.notification.contentView = dumpNotification.contentView;
		dumpNotification.contentView.setProgressBar(R.id.operationProgress,
				10000, progress, false);
		dumpNotification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		
		dumpNotification.notification.flags |= Notification.FLAG_ONLY_ALERT_ONCE;
		dumpNotification.notification.flags &= ~Notification.FLAG_AUTO_CANCEL;
		
		mNotificationManager.notify(NotificationId.INSTALL_DUMP_FILES,
				dumpNotification.notification);
	}
	
	public synchronized void notifyDumpFilesFinish(String description) {
		Notification notification = getDumpFilesNotification(true).notification;
		
		notification.tickerText = description;
		notification.setLatestEventInfo(mAppContext, description, description,
				notification.contentIntent);
		
		notification.flags |= Notification.FLAG_ONLY_ALERT_ONCE | Notification.FLAG_AUTO_CANCEL;
		
		mNotificationManager.notify(NotificationId.INSTALL_DUMP_FILES, notification);
	}
	
	/**
	 * routines for reinstall notifications
	 */
	
	private DistribNotification getReinstallNotification(boolean emptyIntent) {
		Intent intent = null;
		if (!emptyIntent) {
			intent = new Intent(mAppContext, ProgressActivity.class);
			intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		} else
			intent = new Intent();
		PendingIntent pendingIntent = PendingIntent.getActivity(mAppContext, 0, intent, 0);
		
		if (mReinstallNotification != null) {
			mReinstallNotification.notification.when = System.currentTimeMillis();
			mReinstallNotification.notification.contentIntent = pendingIntent;
			return mReinstallNotification;
		}
		
		Notification notification = new Notification(R.drawable.ic_progress,
				mAppContext.getString(R.string.reinstallInProgress),
				System.currentTimeMillis());
		notification.contentIntent = pendingIntent;
		RemoteViews contentView = new RemoteViews(mAppContext.getPackageName(),
				R.layout.install_notification);
		
		mReinstallNotification = new DistribNotification(NotificationId.INSTALL_REINSTALL,
				notification, contentView);
		
		return mReinstallNotification;
	}
	
	public void notifyReinstallBoincOperation(String notifyText) {
		DistribNotification reinstallNotification = getReinstallNotification(false);
		
		reinstallNotification.notification.tickerText = notifyText;
		reinstallNotification.notification.contentView = reinstallNotification.contentView;
		reinstallNotification.contentView.setProgressBar(R.id.operationProgress, 10000, 0, true);
		reinstallNotification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		
		reinstallNotification.notification.flags |= Notification.FLAG_ONLY_ALERT_ONCE;
		reinstallNotification.notification.flags &= ~Notification.FLAG_AUTO_CANCEL;
		
		mNotificationManager.notify(NotificationId.INSTALL_REINSTALL,
				reinstallNotification.notification);
	}
	
	public synchronized void notifyReinstallFinish(String description) {
		Notification notification = getReinstallNotification(true).notification;
		
		notification.tickerText = description;
		notification.setLatestEventInfo(mAppContext, description, description,
				notification.contentIntent);
		
		notification.flags |= Notification.FLAG_ONLY_ALERT_ONCE | Notification.FLAG_AUTO_CANCEL;
		
		mNotificationManager.notify(NotificationId.INSTALL_REINSTALL, notification);
	}
	
	/***
	 * create new or reuse existing notification for project distrib
	 */
	private DistribNotification getProjectNotification(String projectName, boolean emptyIntent) {
		DistribNotification distribNotification = mProjectInstallNotifications.get(projectName);
		
		Intent intent = null;
		if (!emptyIntent) {
			intent = new Intent(mAppContext, ProgressActivity.class);
			intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		} else
			intent = new Intent();
		
		PendingIntent pendingIntent = PendingIntent.getActivity(mAppContext, 0, intent, 0);
		
		if (distribNotification != null) {
			Notification notification = distribNotification.notification;
			notification.when = System.currentTimeMillis();
			notification.contentIntent = pendingIntent;
			return distribNotification;
		}
		
		RemoteViews contentView = new RemoteViews(mAppContext.getPackageName(),
				R.layout.install_notification);
		
		String notifyText = mAppContext.getString(
				R.string.installProjectNotifyBegin) + " " + projectName;
		
		int notificationId = mCurrentProjectInstallNotificationId.incrementAndGet();
		
		Notification notification = new Notification(R.drawable.ic_download,
				notifyText, System.currentTimeMillis());
		distribNotification = new DistribNotification(notificationId,
				notification, contentView);
		
		notification.contentIntent = pendingIntent;
		
		mProjectInstallNotifications.put(projectName, distribNotification);
		
		return distribNotification;
	}
	
	public synchronized void notifyInstallProjectBegin(String projectName) {
		String notifyText = mAppContext.getString(
				R.string.installProjectNotifyBegin) + " " + projectName;
		
		DistribNotification notification = getProjectNotification(projectName, false);
		
		notification.notification.tickerText = notifyText;
		
		notification.notification.flags &= ~Notification.FLAG_ONLY_ALERT_ONCE;
		notification.notification.flags &= ~Notification.FLAG_AUTO_CANCEL;
		
		notification.notification.contentView = notification.contentView;
		notification.contentView.setProgressBar(R.id.operationProgress, 10000, 0, true);
		notification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		mNotificationManager.notify(notification.notificationId, notification.notification);
	}
	
	public synchronized void notifyInstallProjectAppsOperation(String projectName,
			String description) {
		DistribNotification notification = getProjectNotification(projectName, false);
		
		String notifyText = projectName + ": " + description;
		
		notification.notification.flags |= Notification.FLAG_ONLY_ALERT_ONCE;
		notification.notification.flags &= ~Notification.FLAG_AUTO_CANCEL;
		
		notification.notification.tickerText = notifyText;
		notification.notification.contentView = notification.contentView;
		notification.contentView.setProgressBar(R.id.operationProgress, 10000, 0, true);
		notification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		mNotificationManager.notify(notification.notificationId, notification.notification);
	}
	
	public synchronized void notifyInstallProjectAppsProgress(String projectName,
			String description, int progress) {
		DistribNotification notification = getProjectNotification(projectName, false);
		
		String notifyText = projectName + ": " + description;
		
		notification.notification.flags |= Notification.FLAG_ONLY_ALERT_ONCE;
		notification.notification.flags &= ~Notification.FLAG_AUTO_CANCEL;
		
		notification.notification.tickerText = notifyText;
		notification.notification.contentView = notification.contentView;
		notification.contentView.setProgressBar(R.id.operationProgress,
				10000, progress, false);
		notification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		mNotificationManager.notify(notification.notificationId, notification.notification);
	}
	
	public synchronized void notifyInstallProjectAppsFinish(String projectName, String description) {
		DistribNotification notification = getProjectNotification(projectName, true);
		
		String notifyText = projectName + ": " + description;
		
		notification.notification.flags |= Notification.FLAG_ONLY_ALERT_ONCE |
				Notification.FLAG_AUTO_CANCEL;
		
		notification.notification.tickerText = notifyText;
		notification.notification.setLatestEventInfo(mAppContext, notifyText, notifyText,
				notification.notification.contentIntent);
		mNotificationManager.notify(notification.notificationId, notification.notification);
	}
	
	/*
	 * get current status methods
	 */
	
	public synchronized ProgressItem[] getProgressItems() {
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
	public synchronized ProgressItem getProgressItem(String distribName) {
		ProgressItem progress = mDistribInstallProgresses.get(distribName);
		if (progress == null)
			return null;
		return new ProgressItem(progress.name, progress.projectUrl, progress.opDesc,
				progress.progress, progress.state);
	}
	
	/**
	 * 
	 * Native Boinc Service notifications
	 * 
	 */
	public void notifyClientEvent(String title, String message, boolean empty) {
		PendingIntent pendingIntent = null;
		if (!empty) {
			Intent intent = new Intent(mAppContext, BoincManagerActivity.class);
			intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			intent.putExtra(BoincManagerActivity.PARAM_CONNECT_NATIVE_CLIENT, true);
			pendingIntent = PendingIntent.getActivity(mAppContext, 0, intent, 
					PendingIntent.FLAG_UPDATE_CURRENT);
		} else { // if empty notification
			pendingIntent = PendingIntent.getActivity(mAppContext, 0, new Intent(), 
					PendingIntent.FLAG_UPDATE_CURRENT);
		}
		
		if (mClientEventNotification == null) {
			mClientEventNotification = new Notification(R.drawable.nativeboinc_alpha,
					message, System.currentTimeMillis());
		} else {
			mClientEventNotification.when = System.currentTimeMillis();
		}
		mClientEventNotification.tickerText = title;
		
		if (empty)
			mClientEventNotification.flags |= Notification.FLAG_AUTO_CANCEL;
		else
			mClientEventNotification.flags &= ~Notification.FLAG_AUTO_CANCEL;
		
		mClientEventNotification.setLatestEventInfo(mAppContext,
				title, message, pendingIntent);
		
		mNotificationManager.notify(NotificationId.BOINC_CLIENT_EVENT, mClientEventNotification);
	}
	
	public void removeClientNotification() {
		mNotificationManager.cancel(NotificationId.BOINC_CLIENT_EVENT);
	}
	
	/**
	 * handling events on installer operations
	 */
	
	public void handleOnOperation(String distribName, String projectUrl, String opDescription) {
		updateDistribInstallProgress(distribName, projectUrl, opDescription, -1, ProgressItem.STATE_IN_PROGRESS);
		
		if (distribName.equals(InstallerService.BOINC_CLIENT_ITEM_NAME))
			notifyInstallClientOperation(opDescription);
		else if (distribName.equals(InstallerService.BOINC_DUMP_ITEM_NAME))
			notifyDumpFilesOperation(opDescription);
		else if (distribName.equals(InstallerService.BOINC_REINSTALL_ITEM_NAME))
			notifyReinstallBoincOperation(opDescription);
		// ignore non distrib notifications
		else if (distribName.length()!=0)
			notifyInstallProjectAppsOperation(distribName, opDescription);
	}
	
	public void handleOnOperationProgress(String distribName, String projectUrl,
			String opDescription,int progress) {
		updateDistribInstallProgress(distribName, projectUrl, opDescription, progress,
				ProgressItem.STATE_IN_PROGRESS);
		
		if (distribName.equals(InstallerService.BOINC_CLIENT_ITEM_NAME))
			notifyInstallClientProgress(opDescription, progress);
		else if (distribName.equals(InstallerService.BOINC_DUMP_ITEM_NAME))
			notifyDumpFilesProgress(opDescription, progress);
		// ignore non distrib notifications
		else if (distribName.length()!=0)
			notifyInstallProjectAppsProgress(distribName, opDescription, progress);
	}
	
	public void handleOnOperationError(String distribName, String projectUrl, String errorMessage) {
		updateDistribInstallProgress(distribName, projectUrl, errorMessage, -1,
				ProgressItem.STATE_ERROR_OCCURRED);
		
		if (distribName.equals(InstallerService.BOINC_CLIENT_ITEM_NAME))
			notifyInstallClientFinish(errorMessage);
		else if (distribName.equals(InstallerService.BOINC_DUMP_ITEM_NAME))
			notifyDumpFilesFinish(errorMessage);
		else if (distribName.equals(InstallerService.BOINC_REINSTALL_ITEM_NAME))
			notifyReinstallFinish(errorMessage);
		// ignore non distrib notifications
		else if (distribName.length()!=0)
			notifyInstallProjectAppsFinish(distribName, errorMessage);
	}
	
	public void handleOnOperationCancel(String distribName, String projectUrl) {
		updateDistribInstallProgress(distribName, projectUrl, null, -1, ProgressItem.STATE_CANCELLED);

		if (distribName.equals(InstallerService.BOINC_CLIENT_ITEM_NAME))
			notifyInstallClientFinish(InstallerService.BOINC_CLIENT_ITEM_NAME + ": "+
						mAppContext.getString(R.string.operationCancelled));
		else if (distribName.equals(InstallerService.BOINC_DUMP_ITEM_NAME))
			notifyDumpFilesFinish(InstallerService.BOINC_DUMP_ITEM_NAME+ ": "+
						mAppContext.getString(R.string.operationCancelled));
		else if (distribName.equals(InstallerService.BOINC_REINSTALL_ITEM_NAME))
			notifyReinstallFinish(InstallerService.BOINC_REINSTALL_ITEM_NAME+ ": "+
						mAppContext.getString(R.string.operationCancelled));
		// ignore non distrib notifications
		else if (distribName.length()!=0)
			notifyInstallProjectAppsFinish(distribName, mAppContext.getString(R.string.operationCancelled));
	}
	
	public void handleOnOperationFinish(String distribName, String projectUrl) {
		updateDistribInstallProgress(distribName, projectUrl, null, -1, ProgressItem.STATE_FINISHED);
		
		// notifications
		if (distribName.equals(InstallerService.BOINC_CLIENT_ITEM_NAME))
			notifyInstallClientFinish(mAppContext.getString(R.string.installClientNotifyFinish));
		else if (distribName.equals(InstallerService.BOINC_DUMP_ITEM_NAME))
			notifyDumpFilesFinish(mAppContext.getString(R.string.dumpBoincFinish));
		else if (distribName.equals(InstallerService.BOINC_REINSTALL_ITEM_NAME))
			notifyReinstallFinish(mAppContext.getString(R.string.reinstallFinish));
		// ignore non distrib notifications
		else if (distribName.length()!=0)
			notifyInstallProjectAppsFinish(distribName,
					mAppContext.getString(R.string.installProjectNotifyFinish));
	}
}
