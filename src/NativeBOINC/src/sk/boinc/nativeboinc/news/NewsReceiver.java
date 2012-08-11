/**
 * 
 */
package sk.boinc.nativeboinc.news;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.NotificationController;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.util.PreferenceName;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.SystemClock;
import android.preference.PreferenceManager;

/**
 * @author mat
 *
 */
public class NewsReceiver extends BroadcastReceiver {

	private static final String UPDATE_NEWS_INTENT = "sk.boinc.nativeboinc.news.UPDATE_NEWS";
	
	public static final void initialize(Context context) {
		AlarmManager alarmManager = (AlarmManager)context.getSystemService(Context.ALARM_SERVICE);
		
		Intent intent = new Intent(UPDATE_NEWS_INTENT);
		PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, intent, 0);
		
		alarmManager.setRepeating(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime()+
				AlarmManager.INTERVAL_HOUR, AlarmManager.INTERVAL_HOUR, pendingIntent);
	}
		
	private static final class NewsReceiverTask extends AsyncTask<Void, Void, Void> {

		public BoincManagerApplication mApp = null;
		
		public NewsReceiverTask(BoincManagerApplication app) {
			mApp = app;
		}
		
		@Override
		protected Void doInBackground(Void... params) {
			/* download manager file */
			
			InputStream inStream = null;
			ArrayList<NewsMessage> newsMessages = null;
			try {
				URL url = new URL(mApp.getString(R.string.newsRemoteFile));
				URLConnection urlConn = url.openConnection();
				
				urlConn.setConnectTimeout(12000);
				urlConn.setReadTimeout(12000);
				
				inStream = urlConn.getInputStream();
				newsMessages = NewsParser.parse(inStream);
			} catch(Exception ex) {
				// if fail
			} finally {
				try {
					if (inStream != null)
						inStream.close();
				} catch(IOException ex) { }
			}
			
			if (newsMessages != null) {
				// create notifications
				SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(mApp);
				long latestUpdateTime = sharedPrefs.getLong(PreferenceName.NEWS_LATEST_UPDATE_TIME, 0);
				
				
				/* update latest update time */
				sharedPrefs.edit().putLong(PreferenceName.NEWS_LATEST_UPDATE_TIME,
						System.currentTimeMillis());
				
				// whether we have news later than latestUpdate
				boolean hasNews = false;
				for (NewsMessage message: newsMessages)
					if (message.getTimestamp() >= latestUpdateTime) { // if later enws
						hasNews = true;
						break;
					}
				
				if (hasNews) {
					NotificationController notificationController = mApp.getNotificationController();
					
					notificationController.notifyNewsMessages(newsMessages);
				}
			}
			
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
