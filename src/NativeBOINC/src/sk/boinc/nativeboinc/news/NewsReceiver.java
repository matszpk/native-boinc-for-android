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
import java.util.Map;
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
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.IBinder;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * @author mat
 *
 */
public class NewsReceiver extends BroadcastReceiver implements OnSharedPreferenceChangeListener {

	private static final int INSTALLER_CHANNEL_ID = 2;
	
	/* six hours */
	private static final long NEWS_UPDATE_PERIOD = 6*3600*1000;
	
	private static final String TAG = "NewsReceiver";
	
	private static final String UPDATE_NEWS_INTENT = "sk.boinc.nativeboinc.news.UPDATE_NEWS";
	
	private Context mContext = null;
	
	/* constructor for receiver */
	public NewsReceiver() {
	}
	
	/* constructor for preferences changed listener */
	public NewsReceiver(Context context) {
		mContext = context;
	}
	
	public static final void initialize(Context context) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
		if (!prefs.getBoolean(PreferenceName.NATIVE_NEWS_UPDATE, true)) // if not enabled
			return;
		
		if (Logging.DEBUG) Log.d(TAG, "Initializing");
		AlarmManager alarmManager = (AlarmManager)context.getSystemService(Context.ALARM_SERVICE);
		
		Intent intent = new Intent(UPDATE_NEWS_INTENT);
		PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, intent, 0);
		
		alarmManager.cancel(pendingIntent); // remove old alarms
		alarmManager.setRepeating(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime(),
				NEWS_UPDATE_PERIOD, pendingIntent);
	}
	
	public static final void shutdown(Context context) {
		if (Logging.DEBUG) Log.d(TAG, "Shutdown");
		
		AlarmManager alarmManager = (AlarmManager)context.getSystemService(Context.ALARM_SERVICE);
		
		Intent intent = new Intent(UPDATE_NEWS_INTENT);
		PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, intent, 0);
		
		// canceling
		alarmManager.cancel(pendingIntent);
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
			//if (Logging.DEBUG) Log.d(TAG, "Installer connected");
			mInstaller = ((InstallerService.LocalBinder)service).getService();
			mWaitingSem.release();
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			//if (Logging.DEBUG) Log.d(TAG, "Installer disconnected");
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
		public boolean onOperationError(InstallOp installOp,
				String distribName, String errorMessage) {
			// release it
			mBinariesToUpdate = null;
			mWaitingSem.release();
			return true;
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
	
	/**
	 * this bridge used to redirect event to activities or other non local objects
	 */
	public static final class NewsFetcherBridge {
		private Handler mHandler = null;
		private ArrayList<NewsFetcherListener> mListeners = new ArrayList<NewsFetcherListener>();
		
		public NewsFetcherBridge(Handler handler) {
			mHandler = handler;
		}
		
		public synchronized void addListener(NewsFetcherListener listener) {
			mListeners.add(listener);
		}
		public synchronized void removeListener(NewsFetcherListener listener) {
			mListeners.remove(listener);
		}
		
		/* handling events */
		public synchronized void notifyNewsFetched(final boolean isNewFetched) {
			mHandler.post(new Runnable() {
				@Override
				public void run() {
					NewsFetcherListener[] listeners = null;
					synchronized(NewsFetcherBridge.this) {
						listeners = mListeners.toArray(new NewsFetcherListener[0]);
					}
					for (NewsFetcherListener listener: listeners) {
						listener.onNewsReceived(isNewFetched);
					}
				}
			});
		}
		
		public synchronized void notifyError() {
			mHandler.post(new Runnable() {
				@Override
				public void run() {
					NewsFetcherListener[] listeners = null;
					synchronized(NewsFetcherBridge.this) {
						listeners = mListeners.toArray(new NewsFetcherListener[0]);
					}
					for (NewsFetcherListener listener: listeners) {
						listener.onNewsReceiveError();
					}
				}
			});
		}
	}
	
	public static final class NewsFetcherTask extends AsyncTask<Void, Void, Void> {

		private final boolean mUsedOutsideReceiver;
		private BoincManagerApplication mApp;
		private NotificationController mNotificationController;
				
		public NewsFetcherTask(BoincManagerApplication app, boolean usedOutsideReceiver) {
			mApp = app;
			mNotificationController = app.getNotificationController();
			mUsedOutsideReceiver = usedOutsideReceiver;
			if (usedOutsideReceiver) // if used outside
				app.setNewsFetcherTask(this);
		}
		
		/**
		 * download and handle news
		 */
		private void handleNews() {
			NewsFetcherBridge bridge = mApp.getNewsFetcherBridge();
			if (Logging.DEBUG) Log.d(TAG, "Updating news");
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
				if (Logging.DEBUG) Log.d(TAG, "Error in fetching news");
				// if fail
				bridge.notifyError();
				return;
			} finally {
				try {
					if (inStream != null)
						inStream.close();
				} catch(IOException ex) { }
			}
			
			if (Thread.interrupted())
				return;
			
			if (Logging.DEBUG) Log.d(TAG, "Joinning old news with new news");
			
			if (newsMessages != null && !newsMessages.isEmpty()) {
				/* firstly, we read old news */
				ArrayList<NewsMessage> oldNews = NewsUtil.readNews(mApp);
				if (oldNews == null) // if cant read news
					oldNews = new ArrayList<NewsMessage>(1);
				
				/* now we join new News with old version */
				SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(mApp);
				long latestNewsTime = sharedPrefs.getLong(PreferenceName.LATEST_NEWS_TIME, 0);
				
				if (Logging.DEBUG) Log.d(TAG, "Latest news time:"+latestNewsTime);
				
				if (Thread.interrupted())
					return;
				
				/* find oldest news */
				int firstOldMessageIndex;
				for (firstOldMessageIndex = 0; firstOldMessageIndex < newsMessages.size();
						firstOldMessageIndex++)
					if (latestNewsTime >= newsMessages.get(firstOldMessageIndex).getTimestamp())
						break;
				
				if (firstOldMessageIndex == 0) { // no news found
					if (Logging.DEBUG) Log.d(TAG, "No news has been found.");
					bridge.notifyNewsFetched(false);
					return;
				}
				
				sharedPrefs.edit().putLong(PreferenceName.LATEST_NEWS_TIME,
						newsMessages.get(0).getTimestamp()).commit();
				
				// join messages
				newsMessages = new ArrayList<NewsMessage>(newsMessages.subList(0, firstOldMessageIndex));
				newsMessages.addAll(oldNews);
				if (newsMessages.size() > 200) // trim to 200 news
					newsMessages = new ArrayList<NewsMessage>(newsMessages.subList(0, 200));
				
				// write to file
				if (!NewsUtil.writeNews(mApp, newsMessages))
					if (Logging.WARNING) Log.w(TAG, "Cant write news.xml");
				
				if (!mUsedOutsideReceiver)
					mNotificationController.notifyNewsMessages(newsMessages.get(0));
				
				// if new news available
				bridge.notifyNewsFetched(true);
				if (Logging.DEBUG) Log.d(TAG, "news "+ newsMessages.size() + " saved."); 
			} else // nothing new fetched
				bridge.notifyNewsFetched(false);
		}
		
		/**
		 * handle update binaries
		 */
		private void handleUpdateBinaries() {
			// current binaries
			Map<String, String> currentVersions = NewsUtil.readCurrentBinaries(mApp);
			
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
				
				if (binariesToUpdate != null && binariesToUpdate.length != 0) {
					if (currentVersions != null &&
							!NewsUtil.versionsListHaveNewBinaries(currentVersions, binariesToUpdate))
						return; // do nothing
					
					mNotificationController.notifyNewBinaries();
				}
			} finally {
				if (Logging.DEBUG) Log.d(TAG, "Finish check updates");
				mApp.unbindService(conn);
				conn.destroy();
			}
		}
		
		@Override
		protected Void doInBackground(Void... params) {
			synchronized (NewsReceiver.class) {
				handleNews();
				if (!mUsedOutsideReceiver)
					handleUpdateBinaries();
			}
			return null;
		}
		
		private synchronized void finish() {
			if (mUsedOutsideReceiver) {
				if (Logging.DEBUG) Log.d(TAG, "remove news fetcher from app");
				mApp.removeNewsFetcherTask();
			}
			mApp = null;
			mNotificationController = null;
		}
		
		@Override
		protected void onCancelled() {
			finish();
		}
		
		@Override
		protected void onPostExecute(Void result) {
			finish();
		}
	};

	@Override
	public void onReceive(Context context, Intent intent) {
		if (intent.getAction() == null || !intent.getAction().equals(UPDATE_NEWS_INTENT))
			return;
		
		new NewsFetcherTask((BoincManagerApplication)context.getApplicationContext(), false)
				.execute(new Void[0]);
	}

	/* when preferences changed */
	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
			String key) {
		if (key.equals(PreferenceName.NATIVE_NEWS_UPDATE)) {
			boolean isEnabled = sharedPreferences.getBoolean(key, true);
			if (Logging.INFO) Log.i(TAG, "Change notify about news:"+isEnabled);
			if (isEnabled)
				initialize(mContext);
			else
				shutdown(mContext);
		}
	}
}
