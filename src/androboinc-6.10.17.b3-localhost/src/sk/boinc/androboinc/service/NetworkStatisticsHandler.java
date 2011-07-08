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

package sk.boinc.androboinc.service;

import sk.boinc.androboinc.debug.Logging;
import sk.boinc.androboinc.debug.NetStats;
import sk.boinc.androboinc.util.NetStatsStorage;
import sk.boinc.androboinc.util.PreferenceName;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.Log;


public class NetworkStatisticsHandler implements NetStats, OnSharedPreferenceChangeListener {
	private static final String TAG = "NetworkStatisticsHandler";

	private static final int CHKPNT_THRES_SIZE = 100000;

	private Handler mHandler = new Handler();
	private Context mContext = null;
	private boolean mCollection = false;

	private long mTotalReceived = 0;
	private long mTotalSent     = 0;
	private long mUncommittedReceived = 0;
	private long mUncommittedSent     = 0;

	public NetworkStatisticsHandler(Context context) {
		mContext = context;
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(mContext);
		globalPrefs.registerOnSharedPreferenceChangeListener(this);
		mCollection = globalPrefs.getBoolean(PreferenceName.COLLECT_STATS, false);
		if (mCollection) {
			// Collection is ON, retrieve old data
			SharedPreferences netStats = mContext.getSharedPreferences(NetStatsStorage.NET_STATS_FILE, Context.MODE_PRIVATE);
			mTotalReceived = netStats.getLong(NetStatsStorage.NET_STATS_TOTAL_RCVD, 0);
			mTotalSent = netStats.getLong(NetStatsStorage.NET_STATS_TOTAL_SENT, 0);
		}
	}

	public void cleanup() {
		commitPending();
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(mContext);
		globalPrefs.unregisterOnSharedPreferenceChangeListener(this);
		mContext = null;
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
		if (key.equals(PreferenceName.COLLECT_STATS)) {
			boolean collection = sharedPreferences.getBoolean(PreferenceName.COLLECT_STATS, false);
			if (collection == mCollection) return; // unchanged
			// Store new value
			if (Logging.DEBUG) Log.d(TAG, "Collection flag changed from " + mCollection + " to " + collection);
			mCollection = collection;
			if (mCollection) {
				// Just turned on - clear all the data (just to be sure)
				mTotalReceived = 0;
				mTotalSent     = 0;
				mUncommittedReceived = 0;
				mUncommittedSent     = 0;
			}
			else {
				// Just turned off - clear the pending data and commit zeroes
				clearStats();
			}
		}
	}

	@Override
	public void connectionOpened() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				if (!mCollection) return;
				if (Logging.DEBUG) Log.d(TAG, "New connection opened");
				// For the case that new connection opens while old was not closed yet:
				// Commit the data from old connection
				commitPending();
			}
		});
	}

	@Override
	public void bytesReceived(final int numBytes) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				if (!mCollection) return;
				mUncommittedReceived += numBytes;
				if (Logging.DEBUG) Log.d(TAG, "Received " + numBytes + " bytes, uncommitted: " + mUncommittedReceived + " bytes");
				checkpoint();
			}
		});
	}

	@Override
	public void bytesTransferred(final int numBytes) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				if (!mCollection) return;
				mUncommittedSent += numBytes;
				if (Logging.DEBUG) Log.d(TAG, "Sent " + numBytes + " bytes, uncommitted: " + mUncommittedSent + " bytes");
				checkpoint();
			}
		});
	}

	@Override
	public void connectionClosed() {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				if (!mCollection) return;
				// Commit pending counters
				commitPending();
				if (Logging.DEBUG) Log.d(TAG, "Connection closed, total received: " + mTotalReceived + " bytes, sent: " + mTotalSent + " bytes");
			}
		});
	}


	private void commitPending() {
		if ( (mUncommittedReceived != 0) || (mUncommittedSent != 0) ) {
			mTotalReceived += mUncommittedReceived;
			mTotalSent += mUncommittedSent;
			SharedPreferences netStats = mContext.getSharedPreferences(NetStatsStorage.NET_STATS_FILE, Context.MODE_PRIVATE);
			SharedPreferences.Editor editor = netStats.edit();
			editor.putLong(NetStatsStorage.NET_STATS_TOTAL_RCVD, mTotalReceived);
			editor.putLong(NetStatsStorage.NET_STATS_TOTAL_SENT, mTotalSent);
			editor.commit();
			mUncommittedReceived = 0;
			mUncommittedSent = 0;
		}
	}

	private void checkpoint() {
		if ((mUncommittedReceived + mUncommittedSent) > CHKPNT_THRES_SIZE) {
			// Time to save statistics
			if (Logging.DEBUG) Log.d(TAG, "Uncommitted total: " + (mUncommittedReceived + mUncommittedSent) + ", checkpoint");
			commitPending();
		}
	}

	private void clearStats() {
		SharedPreferences netStats = mContext.getSharedPreferences(NetStatsStorage.NET_STATS_FILE, Context.MODE_PRIVATE);
		SharedPreferences.Editor editor = netStats.edit();
		editor.putLong(NetStatsStorage.NET_STATS_TOTAL_RCVD, 0);
		editor.putLong(NetStatsStorage.NET_STATS_TOTAL_SENT, 0);
		editor.commit();
		mTotalReceived = 0;
		mTotalSent     = 0;
		mUncommittedReceived = 0;
		mUncommittedSent = 0;
	}
}
