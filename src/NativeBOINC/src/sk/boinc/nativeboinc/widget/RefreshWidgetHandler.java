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

package sk.boinc.nativeboinc.widget;

import java.util.ArrayList;

import edu.berkeley.boinc.nativeboinc.ClientEvent;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.MonitorListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincReplyListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincStateListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincTasksListener;
import sk.boinc.nativeboinc.nativeclient.WorkerOp;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.TaskItem;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Handler;
import android.os.Parcelable;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * @author mat
 *
 */
public class RefreshWidgetHandler extends Handler implements NativeBoincStateListener, MonitorListener,
		NativeBoincReplyListener, NativeBoincTasksListener, OnSharedPreferenceChangeListener {

	private static final String TAG = "RefreshWidgetHandler";
	
	public static final int WIDGET_REFRESHER_ID = 2;
	
	public final static String UPDATE_PROGRESS = "UPDATE_PROGRESS";
	public final static String UPDATE_TASKS = "UPDATE_TASKS";
	
	private final Context mContext;
	private int mWidgetUpdatePeriod = 0;
	private boolean mDoAttachAutoRefresher = false;
	
	@Override
	public int getRunnerServiceChannelId() {
		return WIDGET_REFRESHER_ID;
	}
	
	
	public RefreshWidgetHandler(Context context) {
		mContext = context;
		
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(mContext);
		// initial value of update period
		mWidgetUpdatePeriod = Integer.parseInt(globalPrefs.getString(
				PreferenceName.WIDGET_UPDATE, "10"))*1000;
	}
	
	private void prepareUpdateWidgets() {
		
		if (NativeBoincWidgetProvider.isWidgetEnabled(mContext)) {
			if (Logging.DEBUG) Log.d(TAG, "Prepare Update NativeBoincWidgets");
			
			Intent intent = new Intent(NativeBoincWidgetProvider.NATIVE_BOINC_WIDGET_PREPARE_UPDATE);
			PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0,
					intent, PendingIntent.FLAG_UPDATE_CURRENT);
			try {
				pendingIntent.send();
			} catch (Exception ex) { }
		}
		
		if (TabletWidgetProvider.isWidgetEnabled(mContext)) {
			if (Logging.DEBUG) Log.d(TAG, "Prepare Update TabletWidgets");
			
			Intent intent = new Intent(TabletWidgetProvider.NATIVE_BOINC_WIDGET_PREPARE_UPDATE);
			PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0,
					intent, PendingIntent.FLAG_UPDATE_CURRENT);
			try {
				pendingIntent.send();
			} catch (Exception ex) { }
		}
	}
	
	private void updateWidgets() {
		if (NativeBoincWidgetProvider.isWidgetEnabled(mContext)) {
			if (Logging.DEBUG) Log.d(TAG, "Update NativeBoincWidgets");
			
			Intent intent = new Intent(NativeBoincWidgetProvider.NATIVE_BOINC_WIDGET_UPDATE);
			PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0,
					intent, PendingIntent.FLAG_UPDATE_CURRENT);
			try {
				pendingIntent.send();
			} catch (Exception ex) { }
		}
		
		/* tablet widget */
		if (TabletWidgetProvider.isWidgetEnabled(mContext)) {
			if (Logging.DEBUG) Log.d(TAG, "Update TabletWidgets");
			Intent intent = new Intent(TabletWidgetProvider.NATIVE_BOINC_WIDGET_UPDATE);
			PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0,
					intent, PendingIntent.FLAG_UPDATE_CURRENT);
			try {
				pendingIntent.send();
			} catch (Exception ex) { }
		}
	}

	
	private class AutoRefresher implements Runnable {

		@Override
		public void run() {
			prepareUpdateWidgets();
			
			if (mAutoRefresher != null)
				postDelayed(mAutoRefresher, mWidgetUpdatePeriod);
		}
	};
	
	private AutoRefresher mAutoRefresher = null;
	
	private synchronized void tryAttachAutoRefresher() {
		if (NativeBoincWidgetProvider.isWidgetEnabled(mContext) ||
				TabletWidgetProvider.isWidgetEnabled(mContext)) {
			// do this when widgets are enabled
			if (mAutoRefresher == null) {
				if (Logging.DEBUG) Log.d(TAG, "Attach autorefresher by handler");
				// immediate refresh
				mAutoRefresher = new AutoRefresher();
				post(mAutoRefresher);
			}
		}
		// otherwise, widget to enable handler
		mDoAttachAutoRefresher = true;
	}
	
	public synchronized void manuallyAttachAutoRefresher() {
		if (mDoAttachAutoRefresher) // when can be enabled
			if (mAutoRefresher == null) {
				if (Logging.DEBUG) Log.d(TAG, "Attach autorefresher manually");
				mAutoRefresher = new AutoRefresher();
				// immediate refresh
				post(mAutoRefresher);
			}
	}
	
	private synchronized void detachAutoRefresh() {
		if (mAutoRefresher != null) {
			if (Logging.DEBUG) Log.d(TAG, "Detach autorefresher by handler");
			removeCallbacks(mAutoRefresher);
			mAutoRefresher = null;
		}
		mDoAttachAutoRefresher = false;
	}
	
	public synchronized void manuallyDetachAutoRefresher() {
		if (!NativeBoincWidgetProvider.isWidgetEnabled(mContext) &&
				!TabletWidgetProvider.isWidgetEnabled(mContext)) {
			// only do this when all widget are disabled
			if (mAutoRefresher != null) {
				if (Logging.DEBUG) Log.d(TAG, "Detach autorefresher manually");
				removeCallbacks(mAutoRefresher);
				mAutoRefresher = null;
			}
		}
	}
	
	/* used by preferences changer */
	public synchronized void reattachAutoRefresher() {
		if (mDoAttachAutoRefresher) {
			if (mAutoRefresher != null) {
				if (Logging.DEBUG) Log.d(TAG, "Reattach autorefresher");
				removeCallbacks(mAutoRefresher);
				// new delay (immediate refresh)
				post(mAutoRefresher);
			}
		}
	}
	
	@Override
	public boolean onNativeBoincClientError(String message) {
		// update widgets
		updateWidgets();
		return false;
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onClientStart() {
		// to update button
		updateWidgets();
		// update state
		prepareUpdateWidgets();
		// do nothing, we awaiting for monitor events or monitorDoesntWork
	}

	@Override
	public void onClientStop(int exitCode, boolean stoppedByManager) {
		if (Logging.DEBUG) Log.d(TAG, "On client stop");
		// detach autorefresh
		detachAutoRefresh();
		// update widgets
		updateWidgets();
	}

	@Override
	public boolean onNativeBoincServiceError(WorkerOp workerOp, String message) {
		// ignore this error
		return false;
	}
	
	@Override
	public void onMonitorEvent(ClientEvent event) {
		if (Logging.DEBUG) Log.d(TAG, "Handle client event:"+ event.type);
		switch (event.type) {
		case ClientEvent.EVENT_RUN_TASKS:
			tryAttachAutoRefresher();
			break;
		case ClientEvent.EVENT_SUSPEND_ALL_TASKS:
			detachAutoRefresh();
			// update state of widgets
			prepareUpdateWidgets();
			break;
		default:
			// do update widgets
			prepareUpdateWidgets();
		}
	}

	@Override
	public void onMonitorDoesntWork() {
		if (Logging.DEBUG) Log.d(TAG, "When monitor doesnt work");
		// if monitor doesnt work, wse enable autorefresher
		tryAttachAutoRefresher();
	}
	
	/* handling progress and tasks (called by widgets) */

	@Override
	public void onProgressChange(double progress) {
		if (NativeBoincWidgetProvider.isWidgetEnabled(mContext)) {
			Intent intent = new Intent(NativeBoincWidgetProvider.NATIVE_BOINC_WIDGET_UPDATE);
			intent.putExtra(UPDATE_PROGRESS, progress);
			PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0, intent,
					PendingIntent.FLAG_UPDATE_CURRENT);
			try {
				pendingIntent.send();
			} catch (Exception ex) { }
		}
	}
	
	@Override
	public void getTasks(ArrayList<TaskItem> tasks) {
		if (TabletWidgetProvider.isWidgetEnabled(mContext)) {
			Intent intent = new Intent(TabletWidgetProvider.NATIVE_BOINC_WIDGET_UPDATE);
			
			Parcelable[] taskItems = new Parcelable[tasks.size()];
			for (int i = 0; i < taskItems.length; i++)
				taskItems[i] = tasks.get(i);
			
			intent.putExtra(UPDATE_TASKS, taskItems);
			
			PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0, intent,
					PendingIntent.FLAG_UPDATE_CURRENT);
			try {
				pendingIntent.send();
			} catch (Exception ex) { }
		}
	}
	
	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
			String key) {
		if (key.equals(PreferenceName.WIDGET_UPDATE)) {
			int newPeriod = Integer.parseInt(sharedPreferences.getString(
					PreferenceName.WIDGET_UPDATE, "10"))*1000;
			if (Logging.DEBUG) Log.d(TAG, "When widget update changed to "+newPeriod);
			mWidgetUpdatePeriod = newPeriod;
			
			// reregister callback
			reattachAutoRefresher();
		}
	}
}
