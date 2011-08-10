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

import edu.berkeley.boinc.lite.Result;
import edu.berkeley.boinc.lite.RpcClient;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
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
	
	private static final String INITIAL_BOINC_CONFIG = "<global_preferences>\n" +
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
	
	private class ListenerHandler extends Handler implements NativeBoincListener {
		@Override
		public void onClientStateChanged(boolean isRun) {
			for (NativeBoincListener listener: mListeners)
				listener.onClientStateChanged(isRun);
		}

		@Override
		public void onClientError(String message) {
			for (NativeBoincListener listener: mListeners)
				listener.onClientError(message);
		}
		
		@Override
		public void onClientFirstStart() {
			for (NativeBoincListener listener: mListeners)
				listener.onClientFirstStart();
		}
		
		@Override
		public void onAfterClientFirstKill() {
			for (NativeBoincListener listener: mListeners)
				listener.onAfterClientFirstKill();
		}
		
		@Override
		public void onClientConfigured() {
			for (NativeBoincListener listener: mListeners)
				listener.onClientConfigured();
		}
	}
	
	private ListenerHandler mListenerHandler = null;
	
	@Override
	public IBinder onBind(Intent intent) {
		startService(new Intent(this, NativeBoincService.class));
		return mBinder;
	}
	
	private WakeLock mWakeLock = null;
	
	@Override
	public void onCreate() {
		PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
		mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, BoincManagerApplication.GLOBAL_ID);
		mListenerHandler = new ListenerHandler();
	}
	
	@Override
	public void onDestroy() {
		mListeners.clear();
		if (mWakeLock.isHeld()) {
			if (Logging.DEBUG) Log.d(TAG, "release screen lock");
			mWakeLock.release();
		}
		mWakeLock = null;
	}
	
	private List<NativeBoincListener> mListeners = new ArrayList<NativeBoincListener>();
	
	private RpcClient mRpcClient = null;
	
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
				Thread.sleep(100);	// 0.1 second
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
			mRpcClient = new RpcClient();
			boolean isRpcOpened = false;
			for (int i = 0; i < 4; i++) {
				if (mRpcClient.open("127.0.0.1", 31416)) {
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
				notifyClientError(getString(R.string.connectNativeClientError));
				killAllBoincZombies();
				killNativeBoinc();
				mRpcClient = null;
				return;
			}
			/* authorize access */
			try {
				if (mRpcClient != null && !mRpcClient.authorize(getAccessPassword())) {
					if (Logging.ERROR) Log.e(TAG, "Authorizing with native client failed");
					notifyClientError(getString(R.string.nativeAuthorizeError));
					killAllBoincZombies();
					killNativeBoinc();
					mRpcClient = null;
					mRpcClient = null;
					return;
				}
			} catch(IOException ex) {
				if (Logging.ERROR) Log.e(TAG, "Authorizing with native client failed");
				notifyClientError(getString(R.string.getAccessPasswordError));
				mRpcClient.close();
				killAllBoincZombies();
				killNativeBoinc();
				mRpcClient = null;
				return;
			}
			
			if (Logging.DEBUG) Log.d(TAG, "Acquire wake lock");
			mWakeLock.acquire();	// screen lock
			
			mIsRun = true;
			notifyClientStateChanged(true);
			
			try { /* waiting to quit */
				if (Logging.DEBUG) Log.d(TAG, "Waiting for boinc_client");
				process.waitFor();
			} catch(InterruptedException ex) { }
			
			if (Logging.DEBUG) Log.d(TAG, "boinc_client has been finished");
			mIsRun = false;
			notifyClientStateChanged(false);
			process.destroy();
			
			killNativeBoinc();
			killAllBoincZombies();	// kill all zombies
			
			if (Logging.DEBUG) Log.d(TAG, "Release wake lock");
			mWakeLock.release();	// screen unlock
		}
	};
	
	private FirstStartThread mFirstStartThread = null;
	private NativeBoincThread mNativeBoincThread = null;
	
	public void addNativeBoincListener(NativeBoincListener listener) {
		mListeners.add(listener);
	}
	
	public void removeNativeBoincListener(NativeBoincListener listener) {
		mListeners.remove(listener);
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
		}
	}
	
	/**
	 * shutting down native client
	 */
	public void shutdownClient() {
		if (Logging.DEBUG) Log.d(TAG, "Shutting down native client");
		if (mNativeBoincThread != null) {
			mRpcClient.quit();
			mRpcClient.close();
			mRpcClient = null;
		}
	}
	
	public boolean isRun() {
		if (mNativeBoincThread == null)
			return false;
		
		return mNativeBoincThread.mIsRun;
	}
	
	public void configureClient() {
		if (Logging.DEBUG) Log.d(TAG, "Configure native client");
		if (mRpcClient == null) {
			notifyClientError(getString(R.string.nativeClientConfigError));
			return;
		}
		if (!mRpcClient.setGlobalPrefsOverride(INITIAL_BOINC_CONFIG)) {
			notifyClientError(getString(R.string.nativeClientConfigError));
			return;
		}
		notifyClientConfigured();
	}
	
	/**
	 * 
	 * @return fraction done (in percents) or -1 if not run
	 */
	public double getGlobalProgress() {
		if (Logging.DEBUG) Log.d(TAG, "Get global progress");
		if (mRpcClient == null)
			return -1.0;	// do nothing
		
		Vector<Result> results = mRpcClient.getResults();
		
		if (results == null)
			return -1.0;
		
		int taskCount = 0;
		double globalProgress = 0.0;
		
		for (Result result: results)
			/* only if running */
			if (result.state == 2 && !result.suspended_via_gui && !result.project_suspended_via_gui &&
					result.active_task_state == 1) {
				globalProgress += result.fraction_done*100.0;
				taskCount++;
			}
		
		if (taskCount == 0)	// no tasks
			return -1.0;
		return globalProgress / taskCount;
	}
	
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
				if (!isRun)	// if stopped
					mNativeBoincThread = null;
				mListenerHandler.onClientStateChanged(isRun);
			}
		});
	}
	
	private synchronized void notifyClientError(final String message) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onClientError(message);
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
	
	private synchronized void notifyClientConfigured() {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onClientConfigured();
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
