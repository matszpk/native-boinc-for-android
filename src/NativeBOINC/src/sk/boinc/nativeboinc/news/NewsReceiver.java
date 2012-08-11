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
package sk.boinc.nativeboinc.news;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Semaphore;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.NotificationController;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.installer.ClientDistrib;
import sk.boinc.nativeboinc.installer.InstallOp;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.installer.InstallerUpdateListener;
import sk.boinc.nativeboinc.installer.ProjectDistrib;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.UpdateItem;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.IBinder;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * @author mat
 *
 */
public class NewsReceiver extends BroadcastReceiver {

	private static final int INSTALLER_CHANNEL_ID = 2;
	
	private static final String TAG = "NewsReceiver";
	
	private static final String UPDATE_NEWS_INTENT = "sk.boinc.nativeboinc.news.UPDATE_NEWS";
	
	public static final void initialize(Context context) {
		AlarmManager alarmManager = (AlarmManager)context.getSystemService(Context.ALARM_SERVICE);
		
		Intent intent = new Intent(UPDATE_NEWS_INTENT);
		PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, intent, 0);
		
		alarmManager.setRepeating(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime()+
				AlarmManager.INTERVAL_HOUR, AlarmManager.INTERVAL_HOUR, pendingIntent);
	}
	
	/**
	 * connection for installer service, it holds installer service, and can await for connection 
	 */
	private static class InstallerServiceConnection implements ServiceConnection {

		private Semaphore mWaitingSem = new Semaphore(1);
		private InstallerService mInstaller = null;
		
		public InstallerServiceConnection() {
			// initial semaphore acquiring
			try {
				mWaitingSem.acquire();
			} catch(InterruptedException ex) { }
		}
		
		public InstallerService getInstaller() {
			return mInstaller;
		}
		
		public void awaitForConenction() {
			try {
				mWaitingSem.acquire();
			} catch(InterruptedException ex) {
			} finally {
				// after it we releasing semaphore
				mWaitingSem.release();
			}
		}
		
		public void destroy() {
			mInstaller = null;
			mWaitingSem.release();
			mWaitingSem = null;
		}
		
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			// TODO Auto-generated method stub
			if (Logging.DEBUG) Log.d(TAG, "Installer connected");
			mInstaller = ((InstallerService.LocalBinder)service).getService();
			mWaitingSem.release();
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (Logging.DEBUG) Log.d(TAG, "Installer disconnected");
			mInstaller = null;
		}
	}
	
	private static class UpdateListResultListener implements InstallerUpdateListener {

		private Semaphore mWaitingSem = new Semaphore(1);
		private UpdateItem[] mBinariesToUpdate = null;
		
		public UpdateListResultListener() {
			try {
				mWaitingSem.acquire();
			} catch(InterruptedException ex) { }
		}
		
		public void awaitForResults() {
			try {
				mWaitingSem.acquire();
			} catch(InterruptedException ex) {
			} finally {
				mWaitingSem.release();
			}
		}
		
		@Override
		public int getInstallerChannelId() {
			return INSTALLER_CHANNEL_ID;
		}

		@Override
		public void onChangeInstallerIsWorking(boolean isWorking) {
			// unused
		}

		@Override
		public boolean onOperationError(InstallOp installOp, String distribName, String errorMessage) {
			// release it
			mBinariesToUpdate = null;
			mWaitingSem.release();
			return false;
		}

		@Override
		public void currentProjectDistribList(ArrayList<ProjectDistrib> projectDistribs) {
			// unused
		}

		@Override
		public void currentClientDistrib(ClientDistrib clientDistrib) {
			// unused
		}

		@Override
		public void binariesToUpdateOrInstall(UpdateItem[] updateItems) {
			// TODO Auto-generated method stub
			mBinariesToUpdate = updateItems;
			mWaitingSem.release();
		}

		@Override
		public void binariesToUpdateFromSDCard(String[] projectNames) {
			// unused
		}
		
		public UpdateItem[] getBinariesToUpdate() {
			return mBinariesToUpdate;
		}
	}
	
	private static final class NewsReceiverTask extends AsyncTask<Void, Void, Void> {

		private final BoincManagerApplication mApp;
		private final NotificationController mNotificationController;
		
		public NewsReceiverTask(BoincManagerApplication app) {
			mApp = app;
			mNotificationController = app.getNotificationController();
		}
		
		/**
		 * download and handle news
		 */
		private void handleNews() {
			/* download manager file */
			InputStream inStream = null;
			List<NewsMessage> newsMessages = null;
			try {
				URL url = new URL(mApp.getString(R.string.newsRemoteFile));
				URLConnection urlConn = url.openConnection();
				
				urlConn.setConnectTimeout(12000);
				urlConn.setReadTimeout(12000);
				
				inStream = urlConn.getInputStream();
				newsMessages = NewsParser.parse(inStream);
			} catch(IOException ex) {
				// if fail
			} finally {
				try {
					if (inStream != null)
						inStream.close();
				} catch(IOException ex) { }
			}
			
			if (newsMessages != null && !newsMessages.isEmpty()) {
				/* firstly, we read old news */
				ArrayList<NewsMessage> oldNews = null;
				inStream = null;
				oldNews = NewsParser.parse(inStream);
				if (oldNews == null) {
					// if fail
					return;
				}
				
				/* now we join new News with old version */
				SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(mApp);
				long latestNewsTime = sharedPrefs.getLong(PreferenceName.LATEST_NEWS_TIME, 0);
				
				/* find oldest news */
				int oldestMessageIndex;
				for (oldestMessageIndex = 0; oldestMessageIndex < newsMessages.size();
						oldestMessageIndex++)
					if (latestNewsTime > newsMessages.get(oldestMessageIndex).getTimestamp()) {
						oldestMessageIndex--;
						break;
					}
				
				if (oldestMessageIndex == -1) // no news found
					return;
				
				sharedPrefs.edit().putLong(PreferenceName.LATEST_NEWS_TIME,
						newsMessages.get(0).getTimestamp());
				
				// join messages
				newsMessages = newsMessages.subList(0, oldestMessageIndex+1);
				newsMessages.addAll(oldNews);
				
				// write to file
				if (!NewsUtil.writeNews(mApp, newsMessages)) {
					// if fail
				}
				
				mNotificationController.notifyNewsMessages(newsMessages.get(0));
			}
		}
		
		/**
		 * handle update binaries
		 */
		private void handleUpdateBinaries() {
			InstallerServiceConnection conn = new InstallerServiceConnection();
			mApp.bindService(new Intent(mApp, InstallerService.class), conn, Context.BIND_AUTO_CREATE);
			// awaiting for connection
			conn.awaitForConenction();
			try {
				InstallerService installer = conn.getInstaller();
				UpdateListResultListener listener = new UpdateListResultListener();
				installer.addInstallerListener(listener);
				installer.getBinariesToUpdateOrInstall(INSTALLER_CHANNEL_ID);
				
				listener.awaitForResults();
				installer.removeInstallerListener(listener);
				UpdateItem[] binariesToUpdate = listener.getBinariesToUpdate();
				
				if (binariesToUpdate != null && binariesToUpdate.length != 0)
					mNotificationController.notifyNewBinaries();
			} finally {
				mApp.unbindService(conn);
				conn.destroy();
			}
		}
		
		@Override
		protected Void doInBackground(Void... params) {
			handleNews();
			handleUpdateBinaries();
			return null;
		}		
	};

	@Override
	public void onReceive(Context context, Intent intent) {
		if (intent.getAction() == null || !intent.getAction().equals(UPDATE_NEWS_INTENT))
			return;
		
		new NewsReceiverTask((BoincManagerApplication)context.getApplicationContext())
				.execute(new Void[0]);
	}
}

