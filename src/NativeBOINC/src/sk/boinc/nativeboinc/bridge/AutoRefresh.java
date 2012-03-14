/* 
 * AndroBOINC - BOINC Manager for Android
 * Copyright (C) 2010, Pavol Michalec
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

package sk.boinc.nativeboinc.bridge;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import sk.boinc.nativeboinc.clientconnection.AutoRefreshListener;
import sk.boinc.nativeboinc.clientconnection.ClientReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientRequestHandler;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.PreferenceName;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.Log;


public class AutoRefresh implements OnSharedPreferenceChangeListener {
	private static final String TAG = "AutoRefresh";

	public final static int CLIENT_MODE = 1;
	public final static int PROJECTS    = 2;
	public final static int TASKS       = 3;
	public final static int TRANSFERS   = 4;
	public final static int MESSAGES    = 5;
	public final static int NOTICES     = 6;

	private final static int RUN_UPDATE = 1;

	private final static int NO_CONNECTIVITY = -1;

	private class UpdateRequest {
		public final ClientReceiver callback;
		public final int requestType;

		public UpdateRequest(final ClientReceiver callback, final int requestType) {
			this.callback = callback;
			this.requestType = requestType;
		}

		@Override
		public boolean equals(Object o) {
			if (!(o instanceof UpdateRequest)) return false;
			return (o.hashCode() == this.hashCode());
		}

		@Override
		public int hashCode() {
			return (callback.hashCode() + requestType);
		}
	}

	Handler mHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			if (mClientRequests == null) return;
			UpdateRequest request = (UpdateRequest)msg.obj;
			
			if (mScheduledUpdates.remove(request)) {
				// So far so good, update was still scheduled
				if (Logging.DEBUG) Log.d(TAG, "triggering automatic update (" + request.callback.toString() + "," + request.requestType + ")");
				
				if (request.callback instanceof AutoRefreshListener) // send notify about starting refresh
					((AutoRefreshListener)request.callback).onStartAutoRefresh(request.requestType);
				
				// We run required update
				switch (request.requestType) {
				case CLIENT_MODE:
					mClientRequests.updateClientMode();
					break;
				case PROJECTS:
					mClientRequests.updateProjects();
					break;
				case TASKS:
					mClientRequests.updateTasks();
					break;
				case TRANSFERS:
					mClientRequests.updateTransfers();
					break;
				case MESSAGES:
					mClientRequests.updateMessages();
					break;
				case NOTICES:
					mClientRequests.updateNotices();
					break;
				default:
					if (Logging.ERROR) Log.e(TAG, "Unhandled request type: " + request.requestType);
				}
			}
			else {
				// Request removed meanwhile, but message was not removed
				if (Logging.WARNING) Log.w(TAG, "Orphaned message received - update already removed: (" + request.callback.toString() + "," + request.requestType + ")");
			}
		}
	};

	private ClientRequestHandler mClientRequests;
	private Set<UpdateRequest> mScheduledUpdates = new HashSet<UpdateRequest>();
	private int mConnectionType = ConnectivityManager.TYPE_MOBILE;
	private int mAutoRefresh = 0;


	public AutoRefresh(final Context context, final ClientRequestHandler clientRequests) {
		mClientRequests = clientRequests;
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(context);
		globalPrefs.registerOnSharedPreferenceChangeListener(this);
		final ConnectivityManager cm = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
		final NetworkInfo ni = (cm == null) ? null : cm.getActiveNetworkInfo();
		mConnectionType = (ni == null) ? NO_CONNECTIVITY : ni.getType();
		if (mConnectionType == ConnectivityManager.TYPE_WIFI) {
			// The current connection type is WiFi
			mAutoRefresh = Integer.parseInt(globalPrefs.getString(PreferenceName.AUTO_UPDATE_WIFI, "10"));
			if (Logging.DEBUG) Log.d(TAG, "Auto-refresh interval is set to: " + mAutoRefresh + " seconds (WiFi)");
		}
		else if (mConnectionType == ConnectivityManager.TYPE_MOBILE) {
			mAutoRefresh = Integer.parseInt(globalPrefs.getString(PreferenceName.AUTO_UPDATE_MOBILE, "10"));
			if (Logging.DEBUG) Log.d(TAG, "Auto-refresh interval is set to: " + mAutoRefresh + " seconds (Mobile)");
		}
		else if (mConnectionType == NO_CONNECTIVITY) {
			// This is the case when ni == null or cm == null
			//mAutoRefresh = 0;      // auto-refresh is disabled
			// mConnectionType = -1 means no further change on preferences update
			//if (Logging.INFO) Log.i(TAG, "Networking not active, disabled auto-refresh");
			mAutoRefresh = Integer.parseInt(globalPrefs.getString(PreferenceName.AUTO_UPDATE_LOCALHOST, "10"));
			if (Logging.DEBUG) Log.d(TAG, "Auto-refresh interval is set to: " + mAutoRefresh + " seconds (Localhost)");
		}
		else {
			mConnectionType = ConnectivityManager.TYPE_MOBILE;
			mAutoRefresh = Integer.parseInt(globalPrefs.getString(PreferenceName.AUTO_UPDATE_MOBILE, "10"));
			if (Logging.DEBUG) Log.d(TAG, "Auto-refresh interval is set to: " + mAutoRefresh + " seconds (other connection, default to Mobile)");			
		}
	}

	public void cleanup() {
		Iterator<UpdateRequest> it = mScheduledUpdates.iterator();
		while (it.hasNext()) {
			// Found pending auto-update; remove its schedule now
			UpdateRequest req = it.next();
			mHandler.removeMessages(RUN_UPDATE, req);
			if (Logging.DEBUG) Log.d(TAG, "cleanup(): Removed schedule for entry (" + req.callback.toString() + "," + req.requestType + ")");
		}
		mScheduledUpdates.clear();
		mClientRequests = null;
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
		if (key.equals(PreferenceName.AUTO_UPDATE_WIFI) && (mConnectionType == ConnectivityManager.TYPE_WIFI)) {
			mAutoRefresh = Integer.parseInt(sharedPreferences.getString(PreferenceName.AUTO_UPDATE_WIFI, "0"));
			if (Logging.DEBUG) Log.d(TAG, "Auto-refresh interval for WiFi changed to: " + mAutoRefresh + " seconds");
		}
		else if (key.equals(PreferenceName.AUTO_UPDATE_MOBILE) && (mConnectionType == ConnectivityManager.TYPE_MOBILE)) {
			mAutoRefresh = Integer.parseInt(sharedPreferences.getString(PreferenceName.AUTO_UPDATE_MOBILE, "0"));
			if (Logging.DEBUG) Log.d(TAG, "Auto-refresh interval for Mobile changed to: " + mAutoRefresh + " seconds");
		}
	}

	public void scheduleAutomaticRefresh(final ClientReceiver callback, final int requestType) {
		if (mAutoRefresh == 0) return;
		UpdateRequest request = new UpdateRequest(callback, requestType);
		if (mScheduledUpdates.contains(request)) {
			if (Logging.DEBUG) Log.d(TAG, "Antry (" + request.callback.toString() + "," + request.requestType + ") already scheduled, removing the old schedule");
			removeAutomaticRefresh(request);
		}
		mScheduledUpdates.add(request);
		
		mHandler.sendMessageDelayed(mHandler.obtainMessage(RUN_UPDATE, request), (mAutoRefresh * 1000));
		if (Logging.DEBUG) Log.d(TAG, "Scheduled automatic refresh for (" + request.callback.toString() + "," + request.requestType + ")");
	}

	public void unscheduleAutomaticRefresh(final ClientReceiver callback) {
		Iterator<UpdateRequest> it = mScheduledUpdates.iterator();
		while (it.hasNext()) {
			// Found pending auto-update; remove its schedule now
			UpdateRequest req = it.next();
			if (req.callback == callback) {
				mHandler.removeMessages(RUN_UPDATE, req);
				mScheduledUpdates.remove(req);
				if (Logging.DEBUG) Log.d(TAG, "unscheduleAutomaticRefresh(): Removed schedule for entry (" + req.callback.toString() + "," + req.requestType + ")");
			}
		}
	}
	
	public void unscheduleAutomaticRefresh(final int refreshType) {
		UpdateRequest[] list = mScheduledUpdates.toArray(new UpdateRequest[0]);
		for (UpdateRequest req: list) {
			// Found pending auto-update; remove its schedule now
			if (req.requestType == refreshType) {
				mHandler.removeMessages(RUN_UPDATE, req);
				mScheduledUpdates.remove(req);
				if (Logging.DEBUG) Log.d(TAG, "unscheduleAutomaticRefresh(): Removed schedule for entry (" + req.callback.toString() + "," + req.requestType + ")");
			}
		}
	}


	private void removeAutomaticRefresh(UpdateRequest request) {
		UpdateRequest[] list = mScheduledUpdates.toArray(new UpdateRequest[0]);
		if (Logging.DEBUG) Log.d(TAG, "mHandler.hasMessages(RUN_UPDATE)=" + mHandler.hasMessages(RUN_UPDATE));
		if (Logging.DEBUG) Log.d(TAG, "mHandler.hasMessages(RUN_UPDATE, request)=" + mHandler.hasMessages(RUN_UPDATE, request));
		for (UpdateRequest req: list) {
			if (req.equals(request)) {
				// The same request - retrieve the original object, as it was the one
				// which was used for posting the delayed message
				if (Logging.DEBUG) Log.d(TAG, "mHandler.hasMessages(RUN_UPDATE, req)=" + mHandler.hasMessages(RUN_UPDATE, req));
				mHandler.removeMessages(RUN_UPDATE, req);
				mScheduledUpdates.remove(req);
				break;
			}
		}
	}
}
