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

package sk.boinc.nativeboinc.nativeclient;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import sk.boinc.nativeboinc.debug.Logging;

import android.os.Handler;
import android.util.Log;

import edu.berkeley.boinc.nativeboinc.ClientEvent;
import edu.berkeley.boinc.nativeboinc.ClientMonitor;

/**
 * @author mat
 *
 */
public class MonitorThread extends Thread {
	private static final String TAG = "MonitorThread";
	
	private String mAuthCode = null;
	private boolean mDoQuit = false;
	
	public static final int STATE_UNKNOWN = 0;
	public static final int STATE_TASKS_RAN = 1;
	public static final int STATE_TASKS_SUSPENDED = 2;
	public static final int STATE_BENCHMARK_RAN = 3;
	
	private int mCurrentState = STATE_UNKNOWN;
	
	public static class ListenerHandler extends Handler implements MonitorListener {
	
		private List<MonitorListener> mListeners = null;
		
		public ListenerHandler() {
			super();
			mListeners = new ArrayList<MonitorListener>();
		}
		
		public void onMonitorEvent(ClientEvent event) {
			MonitorListener[] listeners = mListeners.toArray(new MonitorListener[0]);
			for (MonitorListener callback: listeners)
				callback.onMonitorEvent(event);
		}
		
		public synchronized void addMonitorListener(MonitorListener listener) {
			mListeners.add(listener);
		}
		public synchronized void removeMonitorListener(MonitorListener listener) {
			mListeners.remove(listener);
		}
	}
	
	private ListenerHandler mListenerHandler = null;
	
	public static ListenerHandler createListenerHandler() {
		return new ListenerHandler();
	}
	
	
	public MonitorThread(final ListenerHandler listenerHandler, final String authCode) {
		mAuthCode = authCode;
		mListenerHandler = listenerHandler;
	}
	
	/**
	 * returns current client state based on monitor events
	 * @return current client state
	 */
	public int getCurrentClientState() {
		return mCurrentState;
	}
	
	@Override
	public void run() {
		ClientMonitor clientMonitor = new ClientMonitor();
		try {
			if (!clientMonitor.open("127.0.0.1", 31417, mAuthCode))
				return;
			
			if (Logging.DEBUG) Log.d(TAG, "Client monitor opened");
			
			while (!mDoQuit) {
				ClientEvent event = clientMonitor.poll();
				if (event == null)
					break;
				
				/* update current state */
				switch(event.type) {
				case ClientEvent.EVENT_RUN_TASKS:
					mCurrentState = STATE_TASKS_RAN;
					break;
				case ClientEvent.EVENT_SUSPEND_ALL_TASKS:
					mCurrentState = STATE_TASKS_SUSPENDED;
					break;
				case ClientEvent.EVENT_RUN_BENCHMARK:
					mCurrentState = STATE_BENCHMARK_RAN;
					break;
				case ClientEvent.EVENT_FINISH_BENCHMARK:
					mCurrentState = STATE_TASKS_SUSPENDED;
					break;
				default:
				}
				
				if (Logging.DEBUG) Log.d(TAG, "Notify client event. type: " + event.type +
						",projectUrl: " + event.projectUrl);
				notifyMonitorEvent(event);
			}
		} catch(IOException ex) {
			if (Logging.ERROR) Log.e(TAG, "Error at monitor event polling");
		} finally {
			if (clientMonitor.isConnected())
				clientMonitor.close();
		}
	}
	
	public void quitFromThread() {
		interrupt();
		mDoQuit = false;
	}
	
	private synchronized void notifyMonitorEvent(final ClientEvent event) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onMonitorEvent(event);
			}
		});
	}
}
