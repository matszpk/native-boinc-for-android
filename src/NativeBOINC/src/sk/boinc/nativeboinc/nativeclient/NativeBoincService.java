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

import sk.boinc.nativeboinc.BoincManagerActivity;
import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.NotificationController;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.installer.ClientDistrib;
import sk.boinc.nativeboinc.installer.InstallerProgressListener;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.installer.InstallerUpdateListener;
import sk.boinc.nativeboinc.installer.ProjectDistrib;
import sk.boinc.nativeboinc.util.NotificationId;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.ProcessUtils;

import edu.berkeley.boinc.lite.Result;
import edu.berkeley.boinc.nativeboinc.ClientEvent;
import edu.berkeley.boinc.nativeboinc.ExtendedRpcClient;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
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
public class NativeBoincService extends Service implements MonitorListener,
		InstallerProgressListener, InstallerUpdateListener {
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
		"<run_if_battery_nl_than>10</run_if_battery_nl_than>\n" +
		"</global_preferences>";
	
	private LocalBinder mBinder = new LocalBinder();
	
	private String mPendingError = null;
	private Object mPendingErrorSync = new Object(); // syncer
	
	private boolean mPreviousStateOfIsWorking = false;
	private boolean mIsWorking = false;
	
	public class ListenerHandler extends Handler {

		public void onClientStart() {
			// notify user
			startServiceInForeground();
			mNotificationController.removeClientNotification();
			
			AbstractNativeBoincListener[] listeners = mListeners.toArray(
					new AbstractNativeBoincListener[0]);
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincStateListener)
					((NativeBoincStateListener)listener).onClientStart();
		}
		
		public void onClientStop(int exitCode, boolean stoppedByManager) {
			// notify user
			stopServiceInForeground();
			mNotificationController.notifyClientEvent(getString(R.string.nativeClientShutdown),
					ExitCode.getExitCodeMessage(NativeBoincService.this,
							exitCode, stoppedByManager));
			
			AbstractNativeBoincListener[] listeners = mListeners.toArray(
					new AbstractNativeBoincListener[0]);
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincStateListener)
					((NativeBoincStateListener)listener).onClientStop(exitCode, stoppedByManager);
		}

		public void onNativeBoincError(String message) {
			boolean called = false;
			
			AbstractNativeBoincListener[] listeners = mListeners.toArray(
					new AbstractNativeBoincListener[0]);
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincStateListener) {
					((NativeBoincStateListener)listener).onNativeBoincClientError(message);
					called = true;
				}
			
			synchronized(mPendingErrorSync) {
				if (!called)
					mPendingError = message;
				else	// if already handled
					mPendingError = null;
			}
		}
		
		public void nativeBoincServiceError(String message) {
			boolean called = false;
			
			AbstractNativeBoincListener[] listeners = mListeners.toArray(
					new AbstractNativeBoincListener[0]);
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincServiceListener) {
					((NativeBoincServiceListener)listener).onNativeBoincServiceError(message);
					called = true;
				}
			
			synchronized(mPendingErrorSync) {
				if (!called)
					mPendingError = message;
				else	// if already handled
					mPendingError = null;
			}
		}

		public void onProgressChange(NativeBoincReplyListener callback, double progress) {
			if (mListeners.contains(callback))
				callback.onProgressChange(progress);
		}
		
		public void getResults(NativeBoincResultsListener callback, ArrayList<Result> results) {
			if (mListeners.contains(callback))
				callback.getResults(results);
		}
		
		public void updatedProjectApps(String projectUrl) {
			AbstractNativeBoincListener[] listeners = mListeners.toArray(
					new AbstractNativeBoincListener[0]);
			
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincUpdateListener)
					((NativeBoincUpdateListener)listener).updatedProjectApps(projectUrl);
		}
		
		public void notifyChangeIsWorking(boolean isWorking) {
			AbstractNativeBoincListener[] listeners = mListeners.toArray(
					new AbstractNativeBoincListener[0]);
			
			for (AbstractNativeBoincListener listener: listeners)
				listener.onChangeRunnerIsWorking(isWorking);
		}
	}
	
	private ListenerHandler mListenerHandler = null;
	private MonitorThread.ListenerHandler mMonitorListenerHandler;
	
	private NotificationController mNotificationController = null;
	
	@Override
	public IBinder onBind(Intent intent) {
		if (Logging.DEBUG) Log.d(TAG, "OnBind");
		startService(new Intent(this, NativeBoincService.class));
		return mBinder;
	}
	
	private WakeLock mDimWakeLock = null;
	private WakeLock mPartialWakeLock = null;
	
	private BoincManagerApplication mApp = null;
	
	@Override
	public void onCreate() {
		if (Logging.DEBUG) Log.d(TAG, "onCreate");
		PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
		
		mApp = (BoincManagerApplication)getApplication();
		mNotificationController = mApp.getNotificationController();
		
		mDimWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, BoincManagerApplication.GLOBAL_ID);
		mPartialWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, BoincManagerApplication.GLOBAL_ID);
		mListenerHandler = new ListenerHandler();
		
		mMonitorListenerHandler = MonitorThread.createListenerHandler();
		
		addMonitorListener(this);
	}
	
	private Runnable mRunnerStopper = null;
	
	private static final int DELAYED_DESTROY_INTERVAL = 4000;
	
	@Override
	public void onRebind(Intent intent) {
		if (mListenerHandler != null && mRunnerStopper != null) {
			if (Logging.DEBUG) Log.d(TAG, "Rebind");
			mListenerHandler.removeCallbacks(mRunnerStopper);
			mRunnerStopper = null;
		}
	}
	
	@Override
	public boolean onUnbind(Intent intent) {
		if (mNativeBoincThread == null) {
			if (Logging.DEBUG) Log.d(TAG, "nativeboincthread is null");
			mRunnerStopper = new Runnable() {
				@Override
				public void run() {
					
					if (Logging.DEBUG) Log.d(TAG, "Stop NativeBoincService");
					mRunnerStopper = null;
					stopSelf();
				}
			};
			mListenerHandler.postDelayed(mRunnerStopper, DELAYED_DESTROY_INTERVAL);
		}
		return true;
	}
	@Override
	public void onDestroy() {
		if (Logging.DEBUG) Log.d(TAG, "onDestroy");
		if (isRun()) {
			mWorkerThread.shutdownClient();
			
			try { // waits 5 seconds
				Thread.sleep(5000);
			} catch(InterruptedException ex) { }
			
			killAllNativeBoincs(this);
			killAllBoincZombies();
		}
		
		if (mWorkerThread != null)
			mWorkerThread.stopThread();
		if (mWakeLockHolder != null)
			mWakeLockHolder.destroy();
		if (mMonitorThread != null)
			mMonitorThread.quitFromThread();
		
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
		
		mNotificationController = null;
	}
	
	private List<AbstractNativeBoincListener> mListeners = new ArrayList<AbstractNativeBoincListener>();
	
	private NativeBoincWorkerThread mWorkerThread = null;
	private MonitorThread mMonitorThread = null;
	
	private boolean mShutdownCommandWasPerformed = false;
	
	private Notification mServiceNotification = null;
	
	/*
	 * notifications
	 */
	private void startServiceInForeground() {
		String message = getString(R.string.nativeClientWorking);
		
		Intent intent = new Intent(this, BoincManagerActivity.class);
		PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, 0);
		
		if (mServiceNotification == null) {
			mServiceNotification = new Notification(R.drawable.nativeboinc_alpha, message,
					System.currentTimeMillis());
		} else {
			mServiceNotification.when = System.currentTimeMillis();
		}
		mServiceNotification.setLatestEventInfo(getApplicationContext(),
				message, message, pendingIntent);
		
		startForeground(NotificationId.NATIVE_BOINC_SERVICE, mServiceNotification);
	}
	
	private void stopServiceInForeground() {
		stopForeground(true);
	}
	
	/* kill processes routines */
	private static void killBoincProcesses(List<String> pidStrings) {
		for (String pid: pidStrings) {
			android.os.Process.sendSignal(Integer.parseInt(pid), 2);
			try {
				Thread.sleep(100);	// 0.4 second
			} catch (InterruptedException e) { }
			android.os.Process.sendSignal(Integer.parseInt(pid), 2);
			try {
				Thread.sleep(100);	// 0.4 second
			} catch (InterruptedException e) { }
			android.os.Process.sendSignal(Integer.parseInt(pid), 2);
			try {
				Thread.sleep(400);	// 0.4 second
			} catch (InterruptedException e) { }
			/* fallback killing (by using SIGKILL signal) */
			android.os.Process.sendSignal(Integer.parseInt(pid), 9);
		}
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
	
	private static void killAllNativeBoincs(Context context) {
		Log.d(TAG, "Kill all native boincs");
		String nativeBoincPath = context.getFileStreamPath("boinc_client").getAbsolutePath();
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
		killBoincProcesses(pids);
	}
	
	private static void killAllBoincZombies() {
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
		
	private class NativeBoincThread extends Thread {
		private static final String TAG = "NativeBoincThread";
		
		private boolean mIsRun = false;
		private boolean mSecondStart = false; 
		private int mBoincPid = -1;
		
		public NativeBoincThread(boolean secondStart) {
			this.mSecondStart = secondStart;
		}
		
		@Override
		public void run() {
			SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(
					NativeBoincService.this);
			
			boolean usePartial = globalPrefs.getBoolean(PreferenceName.POWER_SAVING, false);
			
			mShutdownCommandWasPerformed = false;
			
			String programName = NativeBoincService.this.getFileStreamPath("boinc_client")
					.getAbsolutePath();
			
			mBoincPid = ProcessUtils.exec(programName,
					NativeBoincService.this.getFileStreamPath("boinc").getAbsolutePath(),
					new String[] { "--allow_remote_gui_rpc" });
			
			if (mBoincPid == -1) {
				if (Logging.ERROR) Log.e(TAG, "Running boinc_client failed");
				mIsRun = false;
				mNativeBoincThread = null;
				notifyClientError(NativeBoincService.this.getString(R.string.runNativeClientError));
				return;
			}
			
			try {
				Thread.sleep(300); // sleep 0.3 seconds
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
						Thread.sleep(300); // sleep 0.2 seconds
					} catch(InterruptedException ex) { }
				}
			}
			
			if (!isRpcOpened) {
				if (Logging.ERROR) Log.e(TAG, "Connecting with native client failed");
				mIsRun = false;
				mNativeBoincThread = null;
				notifyClientError(getString(R.string.connectNativeClientError));
				killNativeBoinc();
				killAllBoincZombies();
				return;
			}
			
			/* authorize access */
			String password = null;
			try {
				password = getAccessPassword();
			} catch(IOException ex) {
				if (Logging.ERROR) Log.e(TAG, "Authorizing with native client failed");
				mIsRun = false;
				mNativeBoincThread = null;
				notifyClientError(getString(R.string.getAccessPasswordError));
				rpcClient.close();
				killNativeBoinc();
				killAllBoincZombies();
				return;
			}
			boolean isAuthorized = false;
			for (int i = 0; i < 3; i++) {
				if (Logging.DEBUG) Log.d(TAG, "Try to authorize with password:" + password);
				if (rpcClient.authorize(password)) {
					isAuthorized = true;
					break;
				}
			}
			if (!isAuthorized) {
				if (Logging.ERROR) Log.e(TAG, "Authorizing with native client failed");
				mIsRun = false;
				mNativeBoincThread = null;
				notifyClientError(getString(R.string.nativeAuthorizeError));
				killNativeBoinc();
				killAllBoincZombies();
				return;
			} 
			
			if (Logging.DEBUG) Log.d(TAG, "Acquire wake lock");
			if (!usePartial) {
				if (mDimWakeLock != null)
					mDimWakeLock.acquire();	// screen lock
			} else {
				if (mPartialWakeLock != null)
					mPartialWakeLock.acquire();	// partial lock
			}
			
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
			
			// configure client
			if (mSecondStart) {
				if (Logging.DEBUG) Log.d(TAG, "Configure native client");
				if (!rpcClient.setGlobalPrefsOverride(NativeBoincService.INITIAL_BOINC_CONFIG))
					notifyClientError(NativeBoincService.this.getString(R.string.nativeClientConfigError));
				else if (!rpcClient.readGlobalPrefsOverride())
					notifyClientError(NativeBoincService.this.getString(R.string.nativeClientConfigError));
			}
			
			mIsRun = true;
			notifyClientStart();
			
			int exitCode = 0;
			
			try { /* waiting to quit */
				if (Logging.DEBUG) Log.d(TAG, "Waiting for boinc_client");
				exitCode = ProcessUtils.waitForProcess(mBoincPid);
			} catch(InterruptedException ex) {
			}
			
			rpcClient = null;
			
			if (Logging.DEBUG) Log.d(TAG, "boinc_client has been finished");
			
			killNativeBoinc();
			killAllBoincZombies();
			
			if (Logging.DEBUG) Log.d(TAG, "Release wake lock");
			if (mDimWakeLock != null && mDimWakeLock.isHeld())
				mDimWakeLock.release();	// screen unlock
			if (mPartialWakeLock != null && mPartialWakeLock.isHeld())
				mPartialWakeLock.release();	// screen unlock
			
			mIsRun = false;
			mNativeBoincThread = null;
			notifyClientStop(exitCode, mShutdownCommandWasPerformed);
		}
		
		public void killNativeBoinc() {
			android.os.Process.sendSignal(mBoincPid, 2);
			try {
				Thread.sleep(400);	// 0.4 second
			} catch (InterruptedException e) { }
			/* fallback killing (by using SIGKILL signal) */
			android.os.Process.sendSignal(mBoincPid, 9);
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
	
		
	private NativeBoincThread mNativeBoincThread = null;
	private NativeKillerThread mNativeKillerThread = null;
	private WakeLockHolder mWakeLockHolder = null;
	
	public synchronized void addNativeBoincListener(AbstractNativeBoincListener listener) {
		mListeners.add(listener);
	}
	
	public synchronized void removeNativeBoincListener(AbstractNativeBoincListener listener) {
		mListeners.remove(listener);
	}
	
	public void addMonitorListener(MonitorListener listener) {
		mMonitorListenerHandler.addMonitorListener(listener);
	}
	
	public void removeMonitorListener(MonitorListener listener) {
		mMonitorListenerHandler.removeMonitorListener(listener);
	}
	
	/**
	 * first starting up client (ruunning in current thread)
	 */
	public boolean firstStartClient() {
		if (Logging.DEBUG) Log.d(TAG, "Starting FirstStartThread");
		
		//String[] envArray = getEnvArray();
		
		int boincPid = -1;
		
		String programName = NativeBoincService.this.getFileStreamPath("boinc_client")
				.getAbsolutePath();
		
		boincPid = ProcessUtils.exec(programName,
				NativeBoincService.this.getFileStreamPath("boinc").getAbsolutePath(),
				new String[] { "--allow_remote_gui_rpc" });
		
		Log.d(TAG, "First start client, pid:"+boincPid);
		
		if (boincPid == -1) {
			if (Logging.ERROR) Log.d(TAG, "Running boinc_client failed");
			return false;
		}
		
		/* waiting and killing */
		try {
			Thread.sleep(500); // sleep 0.5 seconds
		} catch(InterruptedException ex) { }
		/* killing boinc client */
		
		android.os.Process.sendSignal(boincPid, 2);
		try {
			Thread.sleep(100);	// 0.1 second
		} catch (InterruptedException e) { }
		android.os.Process.sendSignal(boincPid, 2);
		try {
			Thread.sleep(100);	// 0.1 second
		} catch (InterruptedException e) { }
		android.os.Process.sendSignal(boincPid, 2);
		try {
			Thread.sleep(400);	// 0.4 second
		} catch (InterruptedException e) { }
		/* fallback killing (by using SIGKILL signal) */
		android.os.Process.sendSignal(boincPid, 9);
		
		return true;
	}
	
	public String getPendingErrorMessage() {
		synchronized(mPendingErrorSync) {
			String current = mPendingError;
			mPendingError = null;
			return current;
		}
	}
	
	/**
	 * starting up client
	 * @param secondStart - if true marks as second start during installation
	 */
	public void startClient(boolean secondStart) {
		if (Logging.DEBUG) Log.d(TAG, "Starting NativeBoincThread");
		if (mNativeBoincThread == null) {
			
			String text = getString(R.string.nativeClientStarting);
			mNotificationController.notifyClientEvent(text, text);
			
			// inform that, service is working
			mIsWorking = true;
			notifyChangeIsWorking();
			
			mApp.bindRunnerService();
			
			mNativeBoincThread = new NativeBoincThread(secondStart);
			mNativeBoincThread.start();
			mWakeLockHolder = new WakeLockHolder(this, mPartialWakeLock, mDimWakeLock);
		}
	}
	
	/**
	 * shutting down native client
	 */
	public void shutdownClient() {
		if (Logging.DEBUG) Log.d(TAG, "Shutting down native client");
		if (mNativeBoincThread != null) {
			mShutdownCommandWasPerformed = true;
			/* start killer */
			String text = getString(R.string.nativeClientStopping);
			mNotificationController.notifyClientEvent(text, text);
			// inform that, service is working
			mIsWorking = true;
			notifyChangeIsWorking();
			
			mNativeKillerThread = new NativeKillerThread();
			mNativeKillerThread.start();
			mWorkerThread.shutdownClient();
		}
	}
	
	public boolean ifStoppedByManager() {
		return mShutdownCommandWasPerformed;
	}
	
	public boolean serviceIsWorking() {
		return mIsWorking;
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
		if (mWorkerThread != null)
			mWorkerThread.getGlobalProgress(callback);
	}
	
	public void getResults(NativeBoincResultsListener callback) {
		if (Logging.DEBUG) Log.d(TAG, "Get results");
		if (mWorkerThread != null)
			mWorkerThread.getResults(callback);
	}
	
	public void updateProjectApps(String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "update project binaries");
		if (mWorkerThread != null)
			mWorkerThread.updateProjectApps(projectUrl);
	}
	
	/* fallback kill zombies */
	public static void killZombieClient(Context context) {
		killAllNativeBoincs(context);
		killAllBoincZombies();
	}
	
	/* notifying methods */
	private synchronized void notifyClientStart() {
		// inform that, service finished work
		mIsWorking = false;
		notifyChangeIsWorking();
		
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onClientStart();
			}
		});
	}
	
	private synchronized void notifyClientStop(final int exitCode, final boolean stoppedByManager) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
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
				// inform that, service finisged work
				mIsWorking = false;
				notifyChangeIsWorking();
				
				mListenerHandler.onClientStop(exitCode, stoppedByManager);
			}
		});
	}
	
	private synchronized void notifyClientError(final String message) {
		// inform that, service finished work
		mIsWorking = false;
		notifyChangeIsWorking();
		
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onNativeBoincError(message);
			}
		});
	}
	
	private synchronized void notifyChangeIsWorking() {
		final boolean currentIsWorking = mIsWorking;
		if (mPreviousStateOfIsWorking != currentIsWorking) {
			if (Logging.DEBUG) Log.d(TAG, "Change is working:"+currentIsWorking);
			mPreviousStateOfIsWorking = currentIsWorking;
			mListenerHandler.post(new Runnable() {
				@Override
				public void run() {
					mListenerHandler.notifyChangeIsWorking(currentIsWorking);
				}
			});
		}
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
	
	/**
	 * add project handling, queues and installer service bounding
	 */
	
	/**
	 * contains project urls
	 */
	private ArrayList<String> mPendingProjectAppsToInstall = new ArrayList<String>();
	/**
	 * contains project names (not projectUrls)
	 */
	private ArrayList<String> mInstalledProjectApps = new ArrayList<String>();

	private InstallerService mInstaller = null;
	
	private ServiceConnection mInstallerConn = new ServiceConnection() {
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			Log.d(TAG, "Installer is unbounded");
			mInstaller.removeInstallerListener(NativeBoincService.this);
			mInstaller = null;
		}
		
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			Log.d(TAG, "Installer is bounded");
			mInstaller = ((InstallerService.LocalBinder)service).getService();
			mInstaller.addInstallerListener(NativeBoincService.this);
			// update installed distrib list before installing applications (IMPORTANT)
			mInstaller.synchronizeInstalledProjects();
			
			mInstallerInBounding = false;
			
			// do install
			synchronized(mPendingProjectAppsToInstall) {
				for (String projectUrl: mPendingProjectAppsToInstall)
					installProjectApplication(projectUrl);
				mPendingProjectAppsToInstall.clear();
			}
		}
	};
	
	private boolean mInstallerInBounding = false;
	private boolean mIfProjectDistribsListUpdated = false;
	
	/**
	 * real project installtion routine
	 * @param projectUrl
	 */
	private void installProjectApplication(String projectUrl) {
		String projectName = mInstaller.resolveProjectName(projectUrl);
		
		if (projectName == null) {	// do nothing if not found
			if (!mIfProjectDistribsListUpdated) { // if not updated before
				// add to pending tasks
				synchronized (mPendingProjectAppsToInstall) {
					mPendingProjectAppsToInstall.add(projectUrl);
				}
				// try to update project list
				mInstaller.updateProjectDistribList();
			} else {
				// simply finish task
				finishProjectApplicationInstallation(projectName);
			}
			return;
		}
		
		synchronized (mInstalledProjectApps) {
			mInstalledProjectApps.add(projectName);
		}
		mInstaller.installProjectApplicationsAutomatically(projectName, projectUrl);
	}
	
	private void startInstallProjectApplication(String projectUrl) {
		if (mInstaller == null) {
			Log.d(TAG, "Installer service not bound");
			/* if not initialized */
			synchronized(mPendingProjectAppsToInstall) {
				mPendingProjectAppsToInstall.add(projectUrl);
			}
			
			if (!mInstallerInBounding) {
				/* if already bounding */
				mInstallerInBounding = true;
				bindService(new Intent(this, InstallerService.class), mInstallerConn, BIND_AUTO_CREATE);
			}
		} else
			installProjectApplication(projectUrl);
	}

	@Override
	public void onMonitorEvent(ClientEvent event) {
		String title = null;
		switch(event.type) {
		case ClientEvent.EVENT_ATTACHED_PROJECT:
			mNotificationController.notifyClientEvent(getString(R.string.eventAttachedProject), 
					getString(R.string.eventAttachedProjectMessage, event.projectUrl));
			
			startInstallProjectApplication(event.projectUrl);
			break;
		case ClientEvent.EVENT_DETACHED_PROJECT:
			mNotificationController.notifyClientEvent(getString(R.string.eventDetachedProject), 
					getString(R.string.eventDetachedProjectMessage, event.projectUrl));
			break;
		case ClientEvent.EVENT_RUN_BENCHMARK:
			title = getString(R.string.eventBencharkRun);
			mNotificationController.notifyClientEvent(title, title);
			break;
		case ClientEvent.EVENT_FINISH_BENCHMARK:
			title = getString(R.string.eventBencharkFinished);
			mNotificationController.notifyClientEvent(title, title);
			break;
		case ClientEvent.EVENT_SUSPEND_ALL_TASKS:
			title = getString(R.string.eventSuspendAll);
			mNotificationController.notifyClientEvent(title, title);
			break;
		case ClientEvent.EVENT_RUN_TASKS:
			title = getString(R.string.eventRunTasks);
			mNotificationController.notifyClientEvent(title, title);
			break;
		}
	}
	
	private void finishProjectApplicationInstallation(String projectName) {
		boolean ifLast = false;
		synchronized (mInstalledProjectApps) {
			mInstalledProjectApps.remove(projectName);
			ifLast = mInstalledProjectApps.isEmpty();
		}

		if (ifLast)
			unboundInstallService();
	}
	
	private void unboundInstallService() {
		mIfProjectDistribsListUpdated = false;	// reset this indicator
		/* do unbound installer service */
		Log.d(TAG, "Unbound installer service");
		unbindService(mInstallerConn);
		mInstaller.removeInstallerListener(this);
		mInstaller = null;
	}

	@Override
	public void onOperation(String distribName, String opDescription) {
		// do nothing, ignore
	}

	@Override
	public void onOperationProgress(String distribName, String opDescription,
			int progress) {
		// do nothing
	}

	@Override
	public void onOperationError(String distribName, String errorMessage) {
		if (distribName == null || distribName.length() == 0) {
			// is not distrib (updating project distrib list simply failed)
			synchronized (mPendingProjectAppsToInstall) {
				mPendingProjectAppsToInstall.clear();
			}
			
			boolean ifLast = false; 
			synchronized (mInstalledProjectApps) {
				ifLast = mInstalledProjectApps.isEmpty();
			}
			if (ifLast)
				unboundInstallService();
		}
		finishProjectApplicationInstallation(distribName);
	}

	@Override
	public void onOperationCancel(String distribName) {
		finishProjectApplicationInstallation(distribName);
	}

	@Override
	public void onOperationFinish(String distribName) {
		finishProjectApplicationInstallation(distribName);
	}

	@Override
	public void currentProjectDistribList(ArrayList<ProjectDistrib> projectDistribs) {
		// TODO Auto-generated method stub
		mIfProjectDistribsListUpdated = true;
		// try again
		synchronized(mPendingProjectAppsToInstall) {
			for (String projectUrl: mPendingProjectAppsToInstall)
				installProjectApplication(projectUrl);
			mPendingProjectAppsToInstall.clear();
		}
	}

	@Override
	public void currentClientDistrib(ClientDistrib clientDistrib) {
		// TODO Auto-generated method stub
	}

	@Override
	public void onInstallerWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		
	}
}
