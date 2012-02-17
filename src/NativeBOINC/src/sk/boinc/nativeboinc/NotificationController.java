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

	private static final String TAG = "NotificationsController";
	
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
	private DistribNotification getClientNotification() {
		if (mClientInstallNotification != null) {
			mClientInstallNotification.notification.when = System.currentTimeMillis();
			return mClientInstallNotification;
		}
		
		Intent intent = new Intent(mAppContext, ProgressActivity.class);
		
		PendingIntent pendingIntent = PendingIntent.getActivity(mAppContext, 0, intent, 0);
		RemoteViews contentView = new RemoteViews(mAppContext.getPackageName(),
				R.layout.install_notification);
		
		Notification notification = new Notification(R.drawable.nativeboinc_alpha,
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
		
		DistribNotification clientNotification = getClientNotification();
		
		clientNotification.notification.contentView = clientNotification.contentView;
		clientNotification.contentView.setProgressBar(R.id.operationProgress,
				10000, 0, true);
		clientNotification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		
		mNotificationManager.notify(NotificationId.INSTALL_BOINC_CLIENT,
				clientNotification.notification);
	}
	
	public synchronized void notifyInstallClientOperation(String description) {
		String notifyText = InstallerService.BOINC_CLIENT_ITEM_NAME + ": " + description;
		
		DistribNotification clientNotification = getClientNotification();
		
		clientNotification.notification.contentView = clientNotification.contentView;
		clientNotification.contentView.setProgressBar(R.id.operationProgress,
				10000, 0, true);
		clientNotification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		
		mNotificationManager.notify(NotificationId.INSTALL_BOINC_CLIENT,
				clientNotification.notification);
	}
	
	public synchronized void notifyInstallClientProgress(String description, int progress) {
		String notifyText = InstallerService.BOINC_CLIENT_ITEM_NAME + ": " + description;
		
		DistribNotification clientNotification = getClientNotification();
		
		clientNotification.notification.contentView = clientNotification.contentView;
		clientNotification.contentView.setProgressBar(R.id.operationProgress,
				10000, progress, false);
		clientNotification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		
		mNotificationManager.notify(NotificationId.INSTALL_BOINC_CLIENT,
				clientNotification.notification);
	}
	
	public synchronized void notifyInstallClientFinish(String description) {
		//String notifyText = mAppContext.getString(R.string.installClientNotifyFinish);
		
		Notification notification = getClientNotification().notification;
		
		notification.tickerText = description;
		notification.setLatestEventInfo(mAppContext, description, description,
				notification.contentIntent);
		
		mNotificationManager.notify(NotificationId.INSTALL_BOINC_CLIENT, notification);
	}
	
	/***
	 * create new or reuse existing notification for client
	 */
	private DistribNotification getProjectNotification(String projectName) {
		DistribNotification distribNotification = mProjectInstallNotifications.get(projectName);
		
		if (distribNotification != null) {
			Notification notification = distribNotification.notification;
			notification.when = System.currentTimeMillis();
			return distribNotification;
		}
		
		Intent intent = new Intent(mAppContext, ProgressActivity.class);
		
		PendingIntent pendingIntent = PendingIntent.getActivity(mAppContext, 0, intent, 0);
		RemoteViews contentView = new RemoteViews(mAppContext.getPackageName(),
				R.layout.install_notification);
		
		String notifyText = mAppContext.getString(
				R.string.installProjectNotifyBegin) + " " + projectName;
		
		int notificationId = mCurrentProjectInstallNotificationId.incrementAndGet();
		
		Notification notification = new Notification(R.drawable.nativeboinc_alpha,
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
		
		DistribNotification notification = getProjectNotification(projectName);
		
		notification.notification.contentView = notification.contentView;
		notification.contentView.setProgressBar(R.id.operationProgress, 10000, 0, true);
		notification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		mNotificationManager.notify(notification.notificationId, notification.notification);
	}
	
	public synchronized void notifyInstallProjectAppsOperation(String projectName,
			String description) {
		DistribNotification notification = getProjectNotification(projectName);
		
		String notifyText = projectName + ": " + description;
		
		notification.notification.contentView = notification.contentView;
		notification.contentView.setProgressBar(R.id.operationProgress, 10000, 0, true);
		notification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		mNotificationManager.notify(notification.notificationId, notification.notification);
	}
	
	public synchronized void notifyInstallProjectAppsProgress(String projectName,
			String description, int progress) {
		DistribNotification notification = getProjectNotification(projectName);
		
		String notifyText = projectName + ": " + description;
		
		notification.notification.contentView = notification.contentView;
		notification.contentView.setProgressBar(R.id.operationProgress,
				10000, progress, false);
		notification.contentView.setTextViewText(R.id.operationDesc, notifyText);
		mNotificationManager.notify(notification.notificationId, notification.notification);
	}
	
	public synchronized void notifyInstallProjectAppsFinish(String projectName, String description) {
		DistribNotification notification = getProjectNotification(projectName);
		
		String notifyText = projectName + ": " + description;
					mAppContext.getString(R.string.installProjectNotifyFinish);
		
		notification.notification.tickerText = notifyText;
		notification.notification.setLatestEventInfo(mAppContext, notifyText, notifyText,
				notification.notification.contentIntent);
		mNotificationManager.notify(notification.notificationId, notification.notification);
	}
	
	/*
	 * get current status methods
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
	
	/**
	 * 
	 * Native Boinc Service notifications
	 * 
	 */
	public void notifyClientEvent(String title, String message) {
		Intent intent = new Intent(mAppContext, BoincManagerActivity.class);
		PendingIntent pendingIntent = PendingIntent.getActivity(mAppContext, 0, intent, 0);
		
		if (mClientEventNotification == null) {
			mClientEventNotification = new Notification(R.drawable.nativeboinc_alpha,
					message, System.currentTimeMillis());
		} else {
			mClientEventNotification.tickerText = title;
			mClientEventNotification.when = System.currentTimeMillis();
		}
		
		mClientEventNotification.setLatestEventInfo(mAppContext,
				title, message, pendingIntent);
		
		mNotificationManager.notify(NotificationId.BOINC_CLIENT_EVENT, mClientEventNotification);
	}
	
	public void removeClientNotification() {
		mNotificationManager.cancel(NotificationId.BOINC_CLIENT_EVENT);
	}
	
}
