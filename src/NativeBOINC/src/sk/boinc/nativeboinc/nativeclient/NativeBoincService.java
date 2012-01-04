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

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Vector;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.PreferenceName;

import edu.berkeley.boinc.lite.Result;
import edu.berkeley.boinc.nativeboinc.ExtendedRpcClient;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * @author mat
 *
 */
public class NativeBoincService extends Service {
	private final static String TAG = "NativeBoincService";

	public class LocalBinder extends Binder {
		public NativeBoincService getService() {
			return NativeBoincService.this;
		}
	}
	
	public static final String INITIAL_BOINC_CONFIG = "<global_preferences>\n" +
		"<run_on_batteries>1</run_on_batteries>\n" +
		"<run_if_user_active>1</run_if_user_active>\n" +
		"<run_gpu_if_user_active>0</run_gpu_if_user_active>\n" +
		"<idle_time_to_run>30.000000</idle_time_to_run>\n" +
		"<suspend_cpu_usage>0.000000</suspend_cpu_usage>\n" +
		"<start_hour>0.000000</start_hour>\n" +
		"<end_hour>0.000000</end_hour>\n" +
		"<net_start_hour>0.000000</net_start_hour>\n" +
		"<net_end_hour>0.000000</net_end_hour>\n" +
		"<leave_apps_in_memory>0</leave_apps_in_memory>\n" +
		"<confirm_before_connecting>1</confirm_before_connecting>\n" +
		"<hangup_if_dialed>0</hangup_if_dialed>\n" +
		"<dont_verify_images>0</dont_verify_images>\n" +
		"<work_buf_min_days>0.100000</work_buf_min_days>\n" +
		"<work_buf_additional_days>0.250000</work_buf_additional_days>\n" +
		"<max_ncpus_pct>100.000000</max_ncpus_pct>\n" +
		"<cpu_scheduling_period_minutes>60.000000</cpu_scheduling_period_minutes>\n" +
		"<disk_interval>60.000000</disk_interval>\n" +
		"<disk_max_used_gb>10.000000</disk_max_used_gb>\n" +
		"<disk_max_used_pct>100.000000</disk_max_used_pct>\n" +
		"<disk_min_free_gb>0.000000</disk_min_free_gb>\n" +
		"<vm_max_used_pct>100.000000</vm_max_used_pct>\n" +
		"<ram_max_used_busy_pct>50.000000</ram_max_used_busy_pct>\n" +
		"<ram_max_used_idle_pct>90.000000</ram_max_used_idle_pct>\n" +
		"<max_bytes_sec_up>0.000000</max_bytes_sec_up>\n" +
		"<max_bytes_sec_down>0.000000</max_bytes_sec_down>\n" +
		"<cpu_usage_limit>100.000000</cpu_usage_limit>\n" +
		"<daily_xfer_limit_mb>0.000000</daily_xfer_limit_mb>\n" +
		"<daily_xfer_period_days>0</daily_xfer_period_days>\n" +
		"</global_preferences>";
	
	private LocalBinder mBinder = new LocalBinder();
	
	public class ListenerHandler extends Handler {
		public void onClientStateChanged(boolean isRun) {
			for (AbstractNativeBoincListener listener: mListeners)
				if (listener instanceof NativeBoincStateListener)
					((NativeBoincStateListener)listener).onClientStateChanged(isRun);
		}

		public void onNativeBoincError(String message) {
			for (AbstractNativeBoincListener listener: mListeners)
				if (listener instanceof NativeBoincStateListener)
					((NativeBoincStateListener)listener).onNativeBoincError(message);
		}
		
		public void onClientFirstStart() {
			for (AbstractNativeBoincListener listener: mListeners)
				if (listener instanceof NativeBoincStateListener)
					((NativeBoincStateListener)listener).onClientFirstStart();
		}
		
		public void onAfterClientFirstKill() {
			for (AbstractNativeBoincListener listener: mListeners)
				if (listener instanceof NativeBoincStateListener)
					((NativeBoincStateListener)listener).onAfterClientFirstKill();
		}
		
		public void onClientConfigured() {
			for (AbstractNativeBoincListener listener: mListeners)
				if (listener instanceof NativeBoincReplyListener)
					((NativeBoincReplyListener)listener).onClientConfigured();
		}
		
		public void nativeBoincServiceError(AbstractNativeBoincListener callback, String message) {
			if (mListeners.contains(callback)) 
				callback.onNativeBoincError(message);
		}

		public void onProgressChange(NativeBoincReplyListener callback, double progress) {
			if (mListeners.contains(callback))
				callback.onProgressChange(progress);
		}
		
		public void onClientConfigured(NativeBoincReplyListener callback) {
			if (mListeners.contains(callback))
				callback.onClientConfigured();
		}
		
		public void getResults(NativeBoincResultsListener callback, Vector<Result> results) {
			if (mListeners.contains(callback))
				callback.getResults(results);
		}
	}
	
	private ListenerHandler mListenerHandler = null;
	private MonitorThread.ListenerHandler mMonitorListenerHandler;
	
	@Override
	public IBinder onBind(Intent intent) {
		startService(new Intent(this, NativeBoincService.class));
		return mBinder;
	}
	
	private WakeLock mDimWakeLock = null;
	private WakeLock mPartialWakeLock = null;
	
	@Override
	public void onCreate() {
		PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
		mDimWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, BoincManagerApplication.GLOBAL_ID);
		mPartialWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, BoincManagerApplication.GLOBAL_ID);
		mListenerHandler = new ListenerHandler();
		
		mMonitorListenerHandler = MonitorThread.createListenerHandler();
	}
	
	@Override
	public void onDestroy() {
		mListeners.clear();
		if (mDimWakeLock.isHeld()) {
			if (Logging.DEBUG) Log.d(TAG, "release screen lock");
			mDimWakeLock.release();
		}
		mDimWakeLock = null;
		if (mPartialWakeLock.isHeld()) {
			if (Logging.DEBUG) Log.d(TAG, "release screen lock");
			mPartialWakeLock.release();
		}
		mPartialWakeLock = null;
	}
	
	private List<AbstractNativeBoincListener> mListeners = new ArrayList<AbstractNativeBoincListener>();
	
	private NativeBoincWorkerThread mWorkerThread = null;
	
	private MonitorThread mMonitorThread = null;
	
	private static String[] getEnvArray() {
		Map<String, String> envMap = System.getenv();
		String[] envArray = new String[envMap.size()];
		
		{
			int i = 0;
			for (Map.Entry<String, String> envVar: envMap.entrySet())
				envArray[i++] = envVar.getKey()+"="+envVar.getValue();
		}
		return envArray;
	}
	
	private static void killProcesses(List<String> pidStrings) {
		for (String pid: pidStrings) {
			android.os.Process.sendSignal(Integer.parseInt(pid), 2);
			try {
				Thread.sleep(400);	// 0.4 second
			} catch (InterruptedException e) { }
			/* fallback killing (by using SIGKILL signal) */
			android.os.Process.sendSignal(Integer.parseInt(pid), 9);
		}
	}
	
	private void killNativeBoinc() {
		String nativeBoincPath = NativeBoincService.this.getFileStreamPath("boinc_client")
			.getAbsolutePath();
		File procDir = new File("/proc/");
		/* scan /proc directory */
		List<String> pids = new ArrayList<String>();
		BufferedReader cmdLineReader = null;
		
		for (File dir: procDir.listFiles()) {
			String dirName = dir.getName();
			if (!dir.isDirectory())
				continue;
			/* if process directory */
			boolean isNumber = true;
			for (int i = 0; i < dirName.length(); i++)
				if (!Character.isDigit(dirName.charAt(i))) {
					isNumber = false;
					break;
				}
			if (!isNumber)
				continue;
			
			try {
				cmdLineReader = new BufferedReader(new FileReader(
						new File(dir.getAbsolutePath()+"/cmdline")));
				
				String cmdLine = cmdLineReader.readLine();
				if (cmdLine != null && cmdLine.startsWith(nativeBoincPath)) /* is found */
					pids.add(dirName);				
			} catch (IOException ex) {
				continue;
			} finally {
				try {
					cmdLineReader.close();
				} catch(IOException ex) { }
			}
		}
		
		/* do kill processes */
		killProcesses(pids);
	}
	
	private void killAllBoincZombies() {
		File procDir = new File("/proc/");
		List<String> pids = new ArrayList<String>();
		BufferedReader cmdLineReader = null;
		
		String uid = Integer.toString(android.os.Process.myUid());
		String myPid = Integer.toString(android.os.Process.myPid());
		
		for (File dir: procDir.listFiles()) {
			String dirName = dir.getName();
			
			if (!dir.isDirectory())
				continue;
			/* if process directory */
			boolean isNumber = true;
			for (int i = 0; i < dirName.length(); i++)
				if (!Character.isDigit(dirName.charAt(i))) {
					isNumber = false;
					break;
				}
			if (!isNumber)
				continue;
			
			if (dirName.equals(myPid)) // if my pid
				continue;
			
			try {
				cmdLineReader = new BufferedReader(new FileReader(
						new File(dir.getAbsolutePath()+"/status")));
				
				while (true) {
					String line = cmdLineReader.readLine();
					if (line == null)
						break;
					
					if (line.startsWith("Uid:")) {
						// extract first number
						int i = 4;
						int start = 0, end = 4;
						int length = line.length();
						for (i = 4; i < length; i++)
							if (Character.isDigit(line.charAt(i)))	// skip all spaces
								break;
						// if digit
						if (Character.isDigit(line.charAt(i))) {
							start = i;
							for (; i < length; i++)
								if (!Character.isDigit(line.charAt(i)))
									break;
							end = i;
						}
						String processUid = line.substring(start, end);
						if (uid.equals(processUid)) { // if uid matches
							pids.add(dirName);
						}
						break;
					}
				}
			} catch(IOException ex) {
				continue;
			} finally {
				try {
					if (cmdLineReader != null)
						cmdLineReader.close();
				} catch(IOException ex) { }
			}
		}
		
		// kill processes
		killProcesses(pids);
	}
	
	private class FirstStartThread extends Thread {
		@Override
		public void run() {
			String[] envArray = getEnvArray();
			
			try {
				if (Logging.DEBUG) Log.d(TAG, "Running boinc_client program");
				/* run native boinc client */
				Runtime.getRuntime().exec(new String[] {
						NativeBoincService.this.getFileStreamPath("boinc_client").getAbsolutePath(),
						"--allow_remote_gui_rpc" }, envArray,
						NativeBoincService.this.getFileStreamPath("boinc"));
			} catch(IOException ex) {
				if (Logging.ERROR) Log.d(TAG, "First running boinc_client failed");
				notifyClientError(NativeBoincService.this.getString(R.string.runNativeClientError));
				return;
			}
			
			notifyClientFirstStart();
			
			/* waiting and killing */
			try {
				Thread.sleep(300); // sleep 0.3 seconds
			} catch(InterruptedException ex) { }
			/* killing boinc client */
			killNativeBoinc();
			
			notifyClientFirstKill();
		}
	}
	
	private class NativeBoincThread extends Thread {
		private static final String TAG = "NativeBoincThread";
		
		private boolean mIsRun = false;
		
		@Override
		public void run() {
			SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(
					NativeBoincService.this);
			
			boolean usePartial = globalPrefs.getBoolean(PreferenceName.POWER_SAVING, false);
			
			String[] envArray = getEnvArray();
			
			Process process = null;
			try {
				if (Logging.DEBUG) Log.d(TAG, "Running boinc_client program");
				/* run native boinc client */
				process = Runtime.getRuntime().exec(new String[] {
						NativeBoincService.this.getFileStreamPath("boinc_client").getAbsolutePath(),
						"--allow_remote_gui_rpc" }, envArray,
						NativeBoincService.this.getFileStreamPath("boinc"));
			} catch(IOException ex) {
				if (Logging.ERROR) Log.d(TAG, "Running boinc_client failed");
				notifyClientStateChanged(false);
				mIsRun = false;
				notifyClientError(NativeBoincService.this.getString(R.string.runNativeClientError));
				return;
			}
			
			try {
				Thread.sleep(200); // sleep 0.2 seconds
			} catch(InterruptedException ex) { }
			/* open client */
			if (Logging.DEBUG) Log.d(TAG, "Connecting with native client");
			
			ExtendedRpcClient rpcClient = new ExtendedRpcClient();
			boolean isRpcOpened = false;
			for (int i = 0; i < 4; i++) {
				if (rpcClient.open("127.0.0.1", 31416)) {
					isRpcOpened = true;
					break;
				} else {
					try {
						Thread.sleep(200); // sleep 0.2 seconds
					} catch(InterruptedException ex) { }
				}
			}
			
			if (!isRpcOpened) {
				if (Logging.ERROR) Log.e(TAG, "Connecting with native client failed");
				mIsRun = false;
				notifyClientError(getString(R.string.connectNativeClientError));
				killAllBoincZombies();
				killNativeBoinc();
				return;
			}
			/* authorize access */
			try {
				if (rpcClient != null && !rpcClient.authorize(getAccessPassword())) {
					if (Logging.ERROR) Log.e(TAG, "Authorizing with native client failed");
					mIsRun = false;
					notifyClientError(getString(R.string.nativeAuthorizeError));
					killAllBoincZombies();
					killNativeBoinc();
					return;
				}
			} catch(IOException ex) {
				if (Logging.ERROR) Log.e(TAG, "Authorizing with native client failed");
				mIsRun = false;
				notifyClientError(getString(R.string.getAccessPasswordError));
				rpcClient.close();
				killAllBoincZombies();
				killNativeBoinc();
				return;
			}
			
			if (Logging.DEBUG) Log.d(TAG, "Acquire wake lock");
			if (!usePartial)
				mDimWakeLock.acquire();	// screen lock
			else
				mPartialWakeLock.acquire();	// partial lock
			
			String monitorAuthCode = null;
			// authorize monitor
			if (Logging.DEBUG) Log.d(TAG, "Trying to authorize monitor access");
			try {
				Thread.sleep(300);
			} catch(InterruptedException ex) { }
			
			if (rpcClient != null) {
				monitorAuthCode = rpcClient.authorizeMonitor();
				if (monitorAuthCode == null) {
					if (Logging.INFO) Log.i(TAG, "Cant authorize monitor access"); 
				}
			}
			
			// worker thread
			ConditionVariable lock = new ConditionVariable(false);
			mWorkerThread = new NativeBoincWorkerThread(mListenerHandler, NativeBoincService.this, lock, rpcClient);
			mWorkerThread.start();
			
			boolean runningOk = lock.block(200); // Locking until new thread fully runs
			if (!runningOk) {
				if (Logging.ERROR) Log.e(TAG, "NativeBoincWorkerThread did not start in 0.2 second");
			} else 
				if (Logging.DEBUG) Log.d(TAG, "NativeBoincWorkerThread started successfully");
			
			// monitor thread
			if (monitorAuthCode != null) {
				mMonitorThread = new MonitorThread(mMonitorListenerHandler, monitorAuthCode);
				mMonitorThread.start();
			}
			
			mIsRun = true;
			notifyClientStateChanged(true);
			
			try { /* waiting to quit */
				if (Logging.DEBUG) Log.d(TAG, "Waiting for boinc_client");
				process.waitFor();
			} catch(InterruptedException ex) {
			}
			
			rpcClient = null;
			
			if (Logging.DEBUG) Log.d(TAG, "boinc_client has been finished");
			process.destroy();
			
			killNativeBoinc();
			killAllBoincZombies();	// kill all zombies
			
			if (Logging.DEBUG) Log.d(TAG, "Release wake lock");
			if (mDimWakeLock.isHeld())
				mDimWakeLock.release();	// screen unlock
			if (mPartialWakeLock.isHeld())
				mPartialWakeLock.release();	// screen unlock
			
			notifyClientStateChanged(false);
			mIsRun = false;
			
			mNativeBoincThread = null;
		}
	};
	
	private class NativeKillerThread extends Thread {
		private boolean mDontKill = false;
		
		@Override
		public void run() {
			try {
				Thread.sleep(10000);
			} catch(InterruptedException ex) { }
			
			if (!mDontKill && mNativeBoincThread != null) {
				if (Logging.DEBUG) Log.d(TAG, "Killer:Killing native boinc");
				mNativeBoincThread.interrupt();
			}
			else {
				if (Logging.DEBUG) Log.d(TAG, "Killer:Dont Killing native boinc");
			}
		}
		
		public void disableKiller() {
			mDontKill = true;
			interrupt();
		}
	};
	
		
	private FirstStartThread mFirstStartThread = null;
	private NativeBoincThread mNativeBoincThread = null;
	private NativeKillerThread mNativeKillerThread = null;
	private WakeLockHolder mWakeLockHolder = null;
	
	public void addNativeBoincListener(AbstractNativeBoincListener listener) {
		mListeners.add(listener);
	}
	
	public void removeNativeBoincListener(AbstractNativeBoincListener listener) {
		mListeners.remove(listener);
	}
	
	public void addMonitorListener(MonitorListener listener) {
		if (mMonitorThread != null)
			mMonitorThread.addMonitorListener(listener);
	}
	
	public void removeMonitorListener(MonitorListener listener) {
		if (mMonitorThread != null)
			mMonitorThread.removeMonitorListener(listener);
	}
	
	/**
	 * first starting up client
	 */
	public void firstStartClient() {
		mFirstStartThread = new FirstStartThread();
		if (Logging.DEBUG) Log.d(TAG, "Starting FirstStartThread");
		mFirstStartThread.start();
	}
	
	/**
	 * starting up client
	 */
	public void startClient() {
		if (Logging.DEBUG) Log.d(TAG, "Starting NativeBoincThread");
		if (mNativeBoincThread == null) {
			mNativeBoincThread = new NativeBoincThread();
			mNativeBoincThread.start();
			mWakeLockHolder = new WakeLockHolder(this, mPartialWakeLock, mDimWakeLock);
			//mWakeLockHolder.start();
		}
	}
	
	/**
	 * shutting down native client
	 */
	public void shutdownClient() {
		if (Logging.DEBUG) Log.d(TAG, "Shutting down native client");
		if (mNativeBoincThread != null) {
			/* start killer */
			mNativeKillerThread = new NativeKillerThread();
			mNativeKillerThread.start();
			mWorkerThread.shutdownClient();
		}
	}
	
	public boolean isRun() {
		if (mNativeBoincThread == null)
			return false;
		
		return mNativeBoincThread.mIsRun;
	}
	
	/**
	 * returns current client state based on monitor events
	 * @return current client state
	 */
	public int getCurrentClientState() {
		if (mMonitorThread != null)
			return mMonitorThread.getCurrentClientState();
		return MonitorThread.STATE_UNKNOWN;
	}
	
	public void configureClient(NativeBoincReplyListener listener) {
		if (Logging.DEBUG) Log.d(TAG, "Configure native client");
		if (mWorkerThread != null) {
			mWorkerThread.configureClient(listener);
		}
	}
	
	/**
	 * get Listener Handler
	 * @return listener handler
	 */
	public ListenerHandler getListenerHandler() {
		return mListenerHandler;
	}
	
	/**
	 * getGlobalProgress (concurrent version) 
	 */
	public void getGlobalProgress(NativeBoincReplyListener callback) {
		if (Logging.DEBUG) Log.d(TAG, "Get global progress");
		if (mWorkerThread != null) {
			mWorkerThread.getGlobalProgress(callback);
		}
	}
	
	public void getResults(NativeBoincResultsListener callback) {
		if (Logging.DEBUG) Log.d(TAG, "Get results");
		if (mWorkerThread != null) {
			mWorkerThread.getResults(callback);
		}
	}
	
	/**
	 * 
	 */
	
	/* fallback kill zombies */
	public void killZombieClient() {
		killNativeBoinc();
		killAllBoincZombies();
	}
	
	/* notifying methods */
	private synchronized void notifyClientStateChanged(final boolean isRun) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				if (!isRun) {	// if stopped
					mNativeBoincThread = null;
					if (mNativeKillerThread != null) {
						mNativeKillerThread.disableKiller();
						mNativeKillerThread = null;
					}
					if (mWakeLockHolder != null) {
						mWakeLockHolder.destroy();
						mWakeLockHolder = null;
					}
					if (mWorkerThread != null) {
						mWorkerThread.stopThread();
						mWorkerThread = null;
					}
					if (mMonitorThread != null) {
						mMonitorThread.quitFromThread();
						mMonitorThread = null;
					}
				}
				mListenerHandler.onClientStateChanged(isRun);
			}
		});
	}
	
	private synchronized void notifyClientError(final String message) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onNativeBoincError(message);
			}
		});
	}
	
	private synchronized void notifyClientFirstStart() {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onClientFirstStart();
			}
		});
	}
	
	private synchronized void notifyClientFirstKill() {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onAfterClientFirstKill();
			}
		});
	}
	
	/**
	 * reads access password (from gui_rpc_auth.cfg)
	 * @return access password
	 * @throws IOException
	 */
	public String getAccessPassword() throws IOException {
		BufferedReader inReader = null;
		try {
			inReader = new BufferedReader(new FileReader(
					getFilesDir().getAbsolutePath()+"/boinc/gui_rpc_auth.cfg"));
			return inReader.readLine();
		} finally {
			if (inReader != null)
				inReader.close();
		}
	}
	
	public void setAccessPassword(String password) throws IOException {
		OutputStreamWriter writer = null;
		try {
			writer = new FileWriter(getFilesDir().getAbsolutePath()+"/boinc/gui_rpc_auth.cfg");
			writer.write(password);
		} finally {
			if (writer != null)
				writer.close();
		}
	}
}
