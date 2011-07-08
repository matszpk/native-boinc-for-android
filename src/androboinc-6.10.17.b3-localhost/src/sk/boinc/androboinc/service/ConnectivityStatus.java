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
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;

public class ConnectivityStatus extends BroadcastReceiver {
	private static final String TAG = "ConnectivityStatus";

	private Context mContext;
	private ConnectivityListener mConnectivityListener;
	private boolean mConnected = false;
	private int mConnectivityType = ConnectivityManager.TYPE_MOBILE;

	public ConnectivityStatus(Context context, ConnectivityListener connectivityListener) {
		// Store callback to forward notifications about connectivity change
		mConnectivityListener = connectivityListener;
		// Register ourselves as receiver of broadcasts when connectivity changes
		mContext = context;
		mContext.registerReceiver(this, new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION));
		// Get the current network status
		final ConnectivityManager cm = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
		final NetworkInfo ni = cm.getActiveNetworkInfo();
		if (ni != null) {
			mConnected = ni.isConnected();
			mConnectivityType = ni.getType();
		}
		if (Logging.DEBUG) Log.d(TAG, "Created, registered as receiver, connected=" + mConnected);
	}

	public void cleanup() {
		mContext.unregisterReceiver(this);
		if (Logging.DEBUG) Log.d(TAG, "cleanup(), unregistered receiver");
	}

	@Override
	public void onReceive(Context context, Intent intent) {
		final String action = intent.getAction();
		if (action.equals(ConnectivityManager.CONNECTIVITY_ACTION)) {
			// Connectivity changed
			boolean previouslyConnected = mConnected;
			int previousType = mConnectivityType;
			NetworkInfo ni = intent.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);
			if (ni != null) {
				mConnected = ni.isConnected();
				mConnectivityType = ni.getType();
			}
			else {
				// NetworkInfo not available - We are not connected then...
				mConnected = false;
				if (Logging.DEBUG) Log.d(TAG, "onReceive() - NOT connected (no NetworkInfo)");
			}
			if (previouslyConnected && !mConnected) {
				// Lost connection right now
				if (Logging.DEBUG) Log.d(TAG, "Connectivity lost");
				mConnectivityListener.onConnectivityLost();
			}
			else if (!previouslyConnected && mConnected) {
				// Connection available now
				if (Logging.DEBUG) Log.d(TAG, "Connectivity restored, type " + mConnectivityType);
				mConnectivityListener.onConnectivityRestored(mConnectivityType);
			}
			else if (previouslyConnected && mConnected) {
				// It was connected before and it is connected now
				// Maybe just connectivity type changed
				if (previousType != mConnectivityType) {
					if (Logging.DEBUG) Log.d(TAG, "Connectivity type changed from " + previousType + " to " + mConnectivityType);
					mConnectivityListener.onConnectivityChangedType(mConnectivityType);
				}
			}
		}
		else {
			// Incorrect action received - unexpected (we registered only to connectivity changes)
			if (Logging.ERROR) Log.e(TAG, "Unexpected action received: " + action);
			return;
		}
	}

	public final boolean isConnected() {
		return mConnected;
	}

	public final int getConnectivityType() {
		return mConnectivityType;
	}
}
