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

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

import sk.boinc.nativeboinc.BoincManagerActivity;
import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.BugCatchErrorActivity;
import sk.boinc.nativeboinc.ClientMonitorErrorActivity;
import sk.boinc.nativeboinc.NotificationController;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.installer.ClientDistrib;
import sk.boinc.nativeboinc.installer.InstallOp;
import sk.boinc.nativeboinc.installer.InstallerProgressListener;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.installer.InstallerUpdateListener;
import sk.boinc.nativeboinc.installer.ProjectDistrib;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import sk.boinc.nativeboinc.util.NotificationId;
import sk.boinc.nativeboinc.util.PendingController;
import sk.boinc.nativeboinc.util.PendingErrorHandler;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.ProcessUtils;
import sk.boinc.nativeboinc.util.TaskItem;
import sk.boinc.nativeboinc.util.UpdateItem;

import edu.berkeley.boinc.lite.Project;
import edu.berkeley.boinc.nativeboinc.BatteryInfo;
import edu.berkeley.boinc.nativeboinc.ClientEvent;
import edu.berkeley.boinc.nativeboinc.ExtendedRpcClient;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
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
import android.widget.Toast;

/**
 * @author mat
 *
 */
public class NativeBoincService extends Service implements MonitorListener,
		InstallerProgressListener, InstallerUpdateListener {
	
	private static final int NATIVEBOINC_ID = 1; // channelId
	
	private final static String TAG = "NativeBoincService";
	
	public static final int DEFAULT_CHANNEL_ID = 0; // for activity
	public static final int MAX_CHANNEL_ID = 6; // for activity
	
	private static final String PARTIAL_WAKELOCK_NAME = "RunnerPartial";
	private static final String SCREEN_WAKELOCK_NAME = "RunnerScreen";
	
	public class LocalBinder extends Binder {
		public NativeBoincService getService() {
			return NativeBoincService.this;
		}
	}
	
	private LocalBinder mBinder = new LocalBinder();
	
	private String mPendingError = null;
	private Object mPendingErrorSync = new Object(); // syncer
	
	private PendingController<WorkerOp>[] mWorkerPendingChannels =
			new PendingController[MAX_CHANNEL_ID];
	
	private boolean mPreviousStateOfIsWorking = false;
	private boolean mIsWorking = false;
	
	private AtomicBoolean mStartingClient = new AtomicBoolean(false);
	private AtomicBoolean mShuttingDownClient = new AtomicBoolean(false);
	
	/*
	 * cpu usage suspend happens (indicator)
	 */
	private boolean mCpuThrottleSuspendAll = false;
	
	@Override
	public int getInstallerChannelId() {
		return NATIVEBOINC_ID;
	}
	
	public class ListenerHandler extends Handler {

		public void onClientStart() {
			// notify user
			startServiceInForeground();
			mNotificationController.removeClientNotification();
			
			// client monitor error handling
			if (mMonitorThread == null) {
				Intent intent = new Intent(NativeBoincService.this, ClientMonitorErrorActivity.class);
				intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				startActivity(intent);
				
				/* and notify about this */
				mMonitorListenerHandler.post(new Runnable() {
					@Override
					public void run() {
						mMonitorListenerHandler.onMonitorDoesntWork();
					}
				});
			}
			
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			}
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincStateListener)
					((NativeBoincStateListener)listener).onClientStart();
		}
		
		public void onClientStop(int exitCode, boolean stoppedByManager) {
			// notify user
			stopServiceInForeground();
			// save stop reason
			mNotificationController.notifyClientEvent(getString(R.string.nativeClientShutdown),
					ExitCode.getExitCodeMessage(NativeBoincService.this,
							exitCode, stoppedByManager), true);
			
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			}
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincStateListener)
					((NativeBoincStateListener)listener).onClientStop(exitCode, stoppedByManager);
		}

		public void onNativeBoincError(String message) {
			boolean called = false;
			
			// notify about error
			mNotificationController.notifyClientEvent(message, message, true);
			
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			}
			for (AbstractNativeBoincListener listener: listeners)
				if (listener.onNativeBoincClientError(message))
					called = true;
			
			synchronized(mPendingErrorSync) {
				if (Logging.DEBUG) Log.d(TAG, "Is PendingNativeBoincError set:"+(!called));
				if (!called) /* set up pending if not already handled */
					mPendingError = message;
				else	// if already handled
					mPendingError = null;
			}
		}
		
		public void nativeBoincServiceError(int channelId, WorkerOp workerOp, String message) {
			PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
			channel.finish(workerOp);
			
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			}
			
			boolean called = false;
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincServiceListener) {
					NativeBoincServiceListener callback = (NativeBoincServiceListener)listener;
					if (callback.getRunnerServiceChannelId() == channelId)
						if (callback.onNativeBoincServiceError(workerOp, message))
							called = true;
				}
			
			if (!called)
				channel.finishWithError(workerOp, message);
		}

		public void onProgressChange(int channelId, double progress) {
			PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
			
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			} 
			
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincReplyListener) {
					NativeBoincReplyListener callback = (NativeBoincReplyListener)listener;
					if (callback.getRunnerServiceChannelId() == channelId)
						callback.onProgressChange(progress);
				}
			
			channel.finishWithOutput(WorkerOp.GetGlobalProgress, progress);
		}
		
		public void getTasks(int channelId, ArrayList<TaskItem> tasks) {
			PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
			
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			} 
			
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincTasksListener) {
					NativeBoincTasksListener callback = (NativeBoincTasksListener)listener;
					if (callback.getRunnerServiceChannelId() == channelId)
						callback.getTasks(tasks);
				}
			
			channel.finishWithOutput(WorkerOp.GetTasks, tasks);
		}
		
		public void getProjects(int channelId, ArrayList<Project> projects) {
			PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
			
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			}
			
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincProjectsListener) {
					NativeBoincProjectsListener callback = (NativeBoincProjectsListener)listener;
					if (callback.getRunnerServiceChannelId() == channelId)
						callback.getProjects(projects);
				}
			
			channel.finishWithOutput(WorkerOp.GetProjects, projects);
		}
		
		public void updatedProjectApps(int channelId, String projectUrl) {
			PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
			
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			}
			
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincUpdateListener) {
					NativeBoincUpdateListener callback = (NativeBoincUpdateListener)listener;
					if (callback.getRunnerServiceChannelId() == channelId)
						callback.updatedProjectApps(projectUrl);
				}
			
			channel.finish(WorkerOp.UpdateProjectApps(projectUrl));
		}
		
		public void networkCommunicationDone(int channelId) {
			PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
			
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			}
			
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincNetCommListener) {
					NativeBoincNetCommListener callback = (NativeBoincNetCommListener)listener;
					if (callback.getRunnerServiceChannelId() == channelId)
						callback.onNetworkCommunicationDone();
				}
			
			channel.finish(WorkerOp.DoNetComm);
		}
		
		public void sendBatteryInfoDone(int channelId) {
			PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
			
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			}
			
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincBatteryInfoListener) {
					NativeBoincBatteryInfoListener callback = (NativeBoincBatteryInfoListener)listener;
					if (callback.getRunnerServiceChannelId() == channelId)
						callback.onSendBatteryInfoDone();
				}
			
			channel.finish(WorkerOp.SendBatteryInfo);
		}
		
		public void notifyChangeIsWorking(boolean isWorking) {
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			}
			
			for (AbstractNativeBoincListener listener: listeners)
				listener.onChangeRunnerIsWorking(isWorking);
		}
		
		/* automatic installation events */
		public void beginProjectInstallation(String projectUrl) {
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			}
			
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincAutoInstallListener)
					((NativeBoincAutoInstallListener)listener).beginProjectInstallation(projectUrl);
		}
		
		public void projectsNotFound(List<String> projectUrls) {
			AbstractNativeBoincListener[] listeners = null;
			synchronized (NativeBoincService.this) {
				listeners = mListeners.toArray(new AbstractNativeBoincListener[0]);
			}
			
			for (AbstractNativeBoincListener listener: listeners)
				if (listener instanceof NativeBoincAutoInstallListener)
					((NativeBoincAutoInstallListener)listener).projectsNotFound(projectUrls);
		}
	}
	
	private boolean mBatteryNotifying = false;
	private BatteryInfo mBatteryInfo = null;
	private static final int BATTERY_NOTIFY_PERIOD = 5000;
					
	/*
	 * notify battery info handler
	 */
	private BroadcastReceiver mBatteryStateReceiver = new BroadcastReceiver() {
		
		@Override
		public void onReceive(Context context, Intent intent) {
			if (intent.getAction().equals(Intent.ACTION_BATTERY_CHANGED)) {
				BatteryInfo batteryInfo = new BatteryInfo();
				batteryInfo.present = intent.getBooleanExtra("present", false);
				batteryInfo.plugged = intent.getIntExtra("plugged", 0)!=0; 
				if (batteryInfo.present) {
					int scale = (int)intent.getIntExtra("scale", -1);
					batteryInfo.level = (int)intent.getIntExtra("level", 0);
					if (scale > 0)
						batteryInfo.level = (batteryInfo.level*100)/scale;
					batteryInfo.temperature = ((int)intent.getIntExtra("temperature", 0))/10.0;
				}
				mBatteryInfo = batteryInfo;
			}
		}
	};
	
	private Runnable mBatteryNotifier = new Runnable() {

		@Override
		public void run() {
			if (mBatteryInfo != null) {
				BatteryInfo batInfo = new BatteryInfo(mBatteryInfo);
				sendBatteryInfo(NATIVEBOINC_ID, batInfo);
			}
			// next run
			if (mListenerHandler != null && mBatteryNotifying)
				mListenerHandler.postDelayed(mBatteryNotifier, BATTERY_NOTIFY_PERIOD);
		}
	};
	
	private ListenerHandler mListenerHandler = null;
	private MonitorThread.ListenerHandler mMonitorListenerHandler;
	
	private NotificationController mNotificationController = null;
	
	private int mBindCounter = 0;
	
	@Override
	public IBinder onBind(Intent intent) {
		if (Logging.DEBUG) Log.d(TAG, "OnBind");
		mBindCounter++;
		if (Logging.DEBUG) Log.d(TAG, "Bind. Bind counter: " + mBindCounter);
		startService(new Intent(this, NativeBoincService.class));
		return mBinder;
	}
	
	private WakeLock mDimWakeLock = null;
	private WakeLock mPartialWakeLock = null;
	
	private BoincManagerApplication mApp = null;
	
	@Override
	public void onCreate() {
		if (Logging.DEBUG) Log.d(TAG, "onCreate");
		mCpuThrottleSuspendAll = false;
		PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
		
		mApp = (BoincManagerApplication)getApplication();
		mNotificationController = mApp.getNotificationController();
		
		mDimWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, SCREEN_WAKELOCK_NAME);
		mPartialWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, PARTIAL_WAKELOCK_NAME);
		mListenerHandler = new ListenerHandler();
		
		mMonitorListenerHandler = MonitorThread.createListenerHandler();
		
		for (int i = 0; i < MAX_CHANNEL_ID; i++)
			mWorkerPendingChannels[i] = new PendingController<WorkerOp>("NB:PendingCtrl"+i);
		
		addMonitorListener(this);
	}
	
	private Runnable mRunnerStopper = null;
	
	private static final int DELAYED_DESTROY_INTERVAL = 4000;
	
	@Override
	public void onRebind(Intent intent) {
		mBindCounter++;
		if (Logging.DEBUG) Log.d(TAG, "Rebind. Bind counter: " + mBindCounter);
		if (mListenerHandler != null && mRunnerStopper != null) {
			if (Logging.DEBUG) Log.d(TAG, "Rebind");
			mListenerHandler.removeCallbacks(mRunnerStopper);
			mRunnerStopper = null;
		}
	}
	
	@Override
	public boolean onUnbind(Intent intent) {
		mBindCounter--;
		if (Logging.DEBUG) Log.d(TAG, "Unbind. Bind counter: " + mBindCounter);
		if (mNativeBoincThread == null && mBindCounter == 0) {
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
			if (mWorkerThread != null) {
				mWorkerThread.shutdownClient();
				
				try { // waits 5 seconds
					Thread.sleep(5000);
				} catch(InterruptedException ex) { }
			}
			
			NativeBoincUtils.killAllNativeBoincs(this);
			NativeBoincUtils.killAllBoincZombies();
		}
		
		if (mWorkerThread != null)
			mWorkerThread.stopThread();
		if (mWakeLockHolder != null)
			mWakeLockHolder.destroy();
		if (mMonitorThread != null)
			mMonitorThread.quitFromThread();
		// force stop
		stopNotifyingBatteryState();
		// reset starting/shuttingdown indicators
		mShuttingDownClient.set(false);
		mStartingClient.set(false);
		
		synchronized(this) {
			mListeners.clear();
		}
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
		
		for (int i = 0; i < mWorkerPendingChannels.length; i++)
			mWorkerPendingChannels[i].cancelAll();
	}
	
	private List<AbstractNativeBoincListener> mListeners = new ArrayList<AbstractNativeBoincListener>();
	
	private NativeBoincWorkerThread mWorkerThread = null;
	private MonitorThread mMonitorThread = null;
	
	private boolean mShutdownCommandWasPerformed = false;
	
	private Notification mServiceNotification = null;
	
	private boolean mDoRestart = false;
	// if restarted (used by BoincManagerActivity to determine whether reconnecting required)
	private boolean mStartAtRestarting = false; //  
	private boolean mIsRestarted = false;
	
	/*
	 * notifications
	 */
	private void startServiceInForeground() {
		String message = getString(R.string.nativeClientWorking);
		
		Intent intent = new Intent(this, BoincManagerActivity.class);
		intent.putExtra(BoincManagerActivity.PARAM_CONNECT_NATIVE_CLIENT, true);
		intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent,
				PendingIntent.FLAG_UPDATE_CURRENT);
		
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
	
	/* holds execution and waiting:
	 * native boinc thread after running boinc, tries to connect with boinc, causes a missing
	 * of process creation, whose needed to full tracing (must be catched by tracer) */
	private class BugCatchBoincHolder extends Thread {
		private volatile int mBoincPid = -1;
		private int mExitCode;
		private boolean mIsSDCard;
		private String mBoincDir;
		private String[] mBoincArgs;
		private String mProgramName;
		
		public BugCatchBoincHolder(boolean isSDCard, String programName, String boincDir,
				String[] boincArgs) {
			mIsSDCard = isSDCard;
			mProgramName = programName;
			mBoincArgs = boincArgs;
			mBoincDir = boincDir;
		}
		
		@Override
		public void run() {
			if (mIsSDCard)
				mBoincPid = ProcessUtils.bugCatchExecSD(mProgramName, mBoincDir, mBoincArgs);
			else
				mBoincPid = ProcessUtils.bugCatchExec(mProgramName, mBoincDir, mBoincArgs);
			// init bug catcher
			if (ProcessUtils.bugCatchInit(mBoincPid) == -1) {
				if (Logging.WARNING) Log.w(TAG, "Cant attach process!!!");
				mListenerHandler.post(new Runnable() {
					@Override
					public void run() {
						// info about error
						Intent intent = new Intent(NativeBoincService.this, BugCatchErrorActivity.class);
						intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
						startActivity(intent);
					}
				});
			} else {
				// if run ok
				mListenerHandler.post(new Runnable() {
					@Override
					public void run() {
						Toast.makeText(mApp, R.string.bugCatcher, Toast.LENGTH_LONG).show();
					}
				});
			}
			
			try {
				mExitCode = ProcessUtils.bugCatchWaitForProcess(NativeBoincService.this, mBoincPid);
			} catch(InterruptedException ex) { }
		}
		
		public int getExitCode() {
			return mExitCode;
		}
		public int getBoincPid() {
			return mBoincPid;
		}
	}
		
	private class NativeBoincThread extends Thread {
		private static final String TAG = "NativeBoincThread";
		
		private boolean mIsRun = false;
		private boolean mSecondStart = false; 
		private int mBoincPid = -1;
		private boolean mBoincIsKilled = false;
		
		public NativeBoincThread(boolean secondStart) {
			this.mSecondStart = secondStart;
		}
		
		@Override
		public void run() {
			SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(
					NativeBoincService.this);
			
			boolean isSDCard = BoincManagerApplication.isSDCardInstallation(NativeBoincService.this);
			String boincDir = BoincManagerApplication.getBoincDirectory(NativeBoincService.this, isSDCard);
			
			boolean bugCatcherEnabled = globalPrefs.getBoolean(PreferenceName.BUG_CATCHER_ENABLED, false);
			
			boolean usePartial = globalPrefs.getBoolean(PreferenceName.POWER_SAVING, false);
			
			mShutdownCommandWasPerformed = false;
			
			String programName = NativeBoincService.this.getFileStreamPath("boinc_client")
					.getAbsolutePath();
			
			String[] boincArgs = new String[] { "--parent_lifecycle" };
			
			boolean allowRemoteAccess = globalPrefs.getBoolean(PreferenceName.NATIVE_REMOTE_ACCESS, true);
			
			/* choose boinc arguments (depends on AllowRemoteAccess option) */
			if (allowRemoteAccess)
				boincArgs = new String[] { "--parent_lifecycle", "--allow_remote_gui_rpc" };
			
			BugCatchBoincHolder bugCatchBoincHolder = null;
			
			if (!bugCatcherEnabled) {
				if (isSDCard)
					mBoincPid = ProcessUtils.execSD(programName, boincDir, boincArgs);
				else
					mBoincPid = ProcessUtils.exec(programName, boincDir, boincArgs);
			} else { // instead running directly we are using holder
				bugCatchBoincHolder = new BugCatchBoincHolder(isSDCard, programName, boincDir, boincArgs);
				bugCatchBoincHolder.start();
				for (int i = 0; i < 12; i++) { // obtaining boinc pid
					mBoincPid = bugCatchBoincHolder.getBoincPid();
					if (mBoincPid != -1)
						break; // is ran
					try { // sleep by 0.1 second
						Thread.sleep(100);
					} catch(InterruptedException ex) { }
				}
			}
			
			if (mBoincPid == -1) {
				if (Logging.ERROR) Log.e(TAG, "Running boinc_client failed");
				mNativeBoincThread = null;
				notifyClientError(NativeBoincService.this.getString(R.string.runNativeClientError));
				return;
			}
			
			try {
				Thread.sleep(500); // sleep 0.5 seconds
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
						Thread.sleep(500); // sleep 0.5 seconds
					} catch(InterruptedException ex) { }
				}
			}
			
			if (!isRpcOpened) {
				if (Logging.ERROR) Log.e(TAG, "Connecting with native client failed");
				mNativeBoincThread = null;
				notifyClientError(getString(R.string.connectNativeClientError));
				killNativeBoinc();
				NativeBoincUtils.killAllBoincZombies();
				return;
			}
			
			/* authorize access */
			String password = null;
			try {
				password = NativeBoincUtils.getAccessPassword(NativeBoincService.this);
			} catch(IOException ex) {
				if (Logging.ERROR) Log.e(TAG, "Authorizing with native client failed");
				mNativeBoincThread = null;
				notifyClientError(getString(R.string.getAccessPasswordError));
				rpcClient.close();
				killNativeBoinc();
				NativeBoincUtils.killAllBoincZombies();
				return;
			}
			boolean isAuthorized = false;
			for (int i = 0; i < 4; i++) {
				if (Logging.DEBUG) Log.d(TAG, "Try to authorize...");
				if (rpcClient.authorize(password)) {
					isAuthorized = true;
					break;
				}
			}
			if (!isAuthorized) {
				if (Logging.ERROR) Log.e(TAG, "Authorizing with native client failed");
				mNativeBoincThread = null;
				notifyClientError(getString(R.string.nativeAuthorizeError));
				killNativeBoinc();
				NativeBoincUtils.killAllBoincZombies();
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
				// try 3 times connect with client monitor
				for (int i = 0; i < 3; i++) {
					monitorAuthCode = rpcClient.authorizeMonitor();
					if (monitorAuthCode != null)
						break; // if success
					try {
						Thread.sleep(300);
					} catch(InterruptedException ex) { }
				}
				if (monitorAuthCode == null) {
					if (Logging.INFO) Log.i(TAG, "Cant authorize monitor access"); 
				}
			}
			
			// worker thread
			ConditionVariable lock = new ConditionVariable(false);
			NativeBoincWorkerThread workerThread; 
			workerThread = new NativeBoincWorkerThread(mListenerHandler, NativeBoincService.this, lock, rpcClient);
			workerThread.start();
			
			boolean runningOk = lock.block(200); // Locking until new thread fully runs
			if (!runningOk) {
				if (Logging.ERROR) Log.e(TAG, "NativeBoincWorkerThread did not start in 0.2 second");
			} else 
				if (Logging.DEBUG) Log.d(TAG, "NativeBoincWorkerThread started successfully");
			mWorkerThread = workerThread;
			
			// monitor thread
			if (monitorAuthCode != null) {
				mMonitorThread = new MonitorThread(mMonitorListenerHandler, monitorAuthCode);
				mMonitorThread.start();
			}
			
			// configure client
			if (mSecondStart) {
				if (Logging.DEBUG) Log.d(TAG, "Configure native client");
				if (!rpcClient.setGlobalPrefsOverride(NativeBoincUtils.INITIAL_BOINC_CONFIG))
					notifyClientError(NativeBoincService.this.getString(R.string.nativeClientConfigError));
				else if (!rpcClient.readGlobalPrefsOverride())
					notifyClientError(NativeBoincService.this.getString(R.string.nativeClientConfigError));
			}
			
			mIsRun = true;
			// restart after reinstall/update
			mApp.setRestartedAfterReinstall();
			notifyClientStart();
			
			int exitCode = 0;
			
			try { /* waiting to quit */
				if (Logging.DEBUG) Log.d(TAG, "Waiting for boinc_client");
				if (!bugCatcherEnabled)
					exitCode = ProcessUtils.waitForProcess(mBoincPid);
				else { // if bug catcher enabled
					bugCatchBoincHolder.join();
					exitCode = bugCatchBoincHolder.getExitCode();
				}
			} catch(InterruptedException ex) {
			}
			
			rpcClient = null;
			
			if (Logging.DEBUG) Log.d(TAG, "boinc_client has been finished");
			
			if (!mBoincIsKilled) {
				if (Logging.DEBUG) Log.d(TAG, "Killing");
				killNativeBoinc();
				NativeBoincUtils.killAllBoincZombies();
			}
			
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
		
		public void setBoincIsKilled() {
			mBoincIsKilled = true;
		}
	};
	
	private class NativeKillerThread extends Thread {
		private boolean mDontKill = false;
		
		@Override
		public void run() {
			try {
				Thread.sleep(40000); // wait long time to kill
			} catch(InterruptedException ex) { }
			
			if (!mDontKill && mNativeBoincThread != null) {
				if (Logging.DEBUG) Log.d(TAG, "Killer:Killing native boinc");
				mNativeBoincThread.setBoincIsKilled();
				mNativeBoincThread.killNativeBoinc();
				NativeBoincUtils.killAllBoincZombies();
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
		if (mListeners != null)
			mListeners.add(listener);
	}
	
	public synchronized void removeNativeBoincListener(AbstractNativeBoincListener listener) {
		if (mListeners != null)
			mListeners.remove(listener);
	}
	
	public void addMonitorListener(MonitorListener listener) {
		if (mMonitorListenerHandler != null)
			mMonitorListenerHandler.addMonitorListener(listener);
	}
	
	public void removeMonitorListener(MonitorListener listener) {
		if (mMonitorListenerHandler != null)
			mMonitorListenerHandler.removeMonitorListener(listener);
	}
	
	/*
	 * check whether monitor is works
	 */
	public boolean isMonitorWorks() {
		return mMonitorThread != null && mMonitorThread.isWorking();
	}
	
	/**
	 * first starting up client (ruunning in current thread)
	 */
	public static boolean firstStartClient(Context context) {
		if (Logging.DEBUG) Log.d(TAG, "Starting FirstStartThread");
		
		int boincPid = -1;
		
		String programName = context.getFileStreamPath("boinc_client").getAbsolutePath();
		
		boolean isSDCard = BoincManagerApplication.isSDCardInstallation(context);
		String boincDir = BoincManagerApplication.getBoincDirectory(context, isSDCard);
		if (isSDCard)
			boincPid = ProcessUtils.execSD(programName, boincDir,
					new String[] { "--allow_remote_gui_rpc" });
		else
			boincPid = ProcessUtils.exec(programName, boincDir,
					new String[] { "--allow_remote_gui_rpc" });
		
		if (Logging.DEBUG) Log.d(TAG, "First start client, pid:"+boincPid);
		
		if (boincPid == -1) {
			if (Logging.ERROR) Log.e(TAG, "Running boinc_client failed");
			return false;
		}
		
		/* waiting and killing */
		try {
			Thread.sleep(700); // sleep 0.7 seconds
		} catch(InterruptedException ex) { }
		/* killing boinc client */
		File guiRPCPathFile = new File(boincDir+"/gui_rpc_auth.cfg");
		if (!guiRPCPathFile.exists()) {
			if (Logging.WARNING) Log.w(TAG, "gui_rpc_auth.cfg at the first start doesnt exists!");
			try {
				Thread.sleep(500); // sleep 0.5 seconds
			} catch(InterruptedException ex) { }
		}
		
		if (!guiRPCPathFile.exists()) {
			if (Logging.WARNING) Log.w(TAG, "gui_rpc_auth.cfg at the first start doesnt exists again!");
		}
		
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
	
	public boolean handlePendingErrorMessage(AbstractNativeBoincListener listener) {
		synchronized(mPendingErrorSync) {
			if (mPendingError == null)
				return false;
			if (listener.onNativeBoincClientError(mPendingError)) {
				if (Logging.DEBUG) Log.d(TAG, "PendingNativeBoincError is handled");
				mPendingError = null;
				return true;
			}
			return false;
		}
	}
	
	private void clearPendingErrorMessage() {
		synchronized(mPendingErrorSync) {
			mPendingError = null;
		}
	}
	
	public boolean handlePendingServiceErrorMessages(final WorkerOp workerOp,
			final NativeBoincServiceListener listener) {
		PendingController<WorkerOp> channel = mWorkerPendingChannels[listener.getRunnerServiceChannelId()];
		
		return channel.handlePendingErrors(workerOp, new PendingErrorHandler<WorkerOp>() {
			@Override
			public boolean handleError(WorkerOp op, Object error) {
				return listener.onNativeBoincServiceError(workerOp, (String)error);
			}
		});
	}
	
	/**
	 * starting up client
	 * @param secondStart - if true marks as second start during installation
	 */
	public void startClient(boolean secondStart) {
		if (Logging.DEBUG) Log.d(TAG, "Starting NativeBoincThread");
		if (mNativeBoincThread == null && !mStartingClient.getAndSet(true)) {
			clearPendingErrorMessage();
			
			String text = getString(R.string.nativeClientStarting);
			mNotificationController.notifyClientEvent(text, text, false);
			
			// inform that service is working
			mIsWorking = true;
			notifyChangeIsWorking();
			
			mApp.bindRunnerService();
			
			/// update native client id (in host db)
			HostListDbAdapter dbAdapter = new HostListDbAdapter(this);
			try {
				dbAdapter.open();
				ClientId clientId = dbAdapter.fetchHost("nativeboinc");
				if (clientId == null) {
					notifyClientError(getString(R.string.getAccessPasswordError));
					return; // no client id, do nothing
				}
				String password = NativeBoincUtils.getAccessPassword(this);
				clientId.setPassword(password);
				dbAdapter.updateHost(clientId);
			} catch(IOException ex) {
			} finally {
				dbAdapter.close();
			}
			
			// last chance to install certificates (when sdcard was removed before start)
			InstallerService.prepareCaBundleFileIfNeeded(this, true);
			
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
		if (mNativeBoincThread != null && !mShuttingDownClient.getAndSet(true)) {
			clearPendingErrorMessage();
			stopNotifyingBatteryState();
			
			mShutdownCommandWasPerformed = true;
			/* start killer */
			String text = getString(R.string.nativeClientStopping);
			mNotificationController.notifyClientEvent(text, text, false);
			// inform that, service is working
			mIsWorking = true;
			notifyChangeIsWorking();
			
			mStartAtRestarting = false;
			mIsRestarted = false;
			
			mNativeKillerThread = new NativeKillerThread();
			mNativeKillerThread.start();
			if (mWorkerThread != null)
				mWorkerThread.shutdownClient();
		}
	}
	
	/**
	 * restarts native client
	 * @return
	 */
	public void restartClient() {
		if (Logging.DEBUG) Log.d(TAG, "Restarting client");
		if (mNativeBoincThread == null) {
			// normal start
			startClient(false); 
		} else {
			// restart (shutdown and start)
			mDoRestart = true;
			shutdownClient();
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
	
	public boolean isRestarted() {
		return mIsRestarted;
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
	 * methods returns boolean value, that indicates whether task has been ran (true) or not (false) 
	 */
	public boolean getGlobalProgress(int channelId) {
		if (Logging.DEBUG) Log.d(TAG, "Get global progress");
		
		PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
		
		if (mWorkerThread != null) {
			if (channel.begin(WorkerOp.GetGlobalProgress)) {
				mWorkerThread.getGlobalProgress(channelId);
				return true;
			}
		}
		return false;
	}
	
	public boolean getTasks(int channelId) {
		if (Logging.DEBUG) Log.d(TAG, "Get results");
		
		PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
		
		if (mWorkerThread != null) {
			if (channel.begin(WorkerOp.GetTasks)) {
				mWorkerThread.getTasks(channelId);
				return true;
			}
		}
		return false; 
	}
	
	public boolean getProjects(int channelId) {
		if (Logging.DEBUG) Log.d(TAG, "Get projects");
		
		PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
		
		if (mWorkerThread != null) {
			if (channel.begin(WorkerOp.GetProjects)) {
				mWorkerThread.getProjects(channelId);
				return true;
			}
		}
		return false;
	}
	
	public boolean updateProjectApps(int channelId, String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "update project binaries");
		
		PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
		
		WorkerOp workerOp = WorkerOp.UpdateProjectApps(projectUrl);
		if (mWorkerThread != null) {
			if (channel.begin(workerOp)) {
				mWorkerThread.updateProjectApps(channelId, projectUrl);
				return true;
			}
		}
		return false;
	}
	
	public boolean doNetworkCommunication(int channelId) {
		if (Logging.DEBUG) Log.d(TAG, "do network communicartion");
		
		PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
		
		WorkerOp workerOp = WorkerOp.DoNetComm;
		if (mWorkerThread != null) {
			if (channel.begin(workerOp)) {
				mWorkerThread.doNetworkCommunication(channelId);
				return true;
			}
		}
		return false;
	}
	
	public boolean sendBatteryInfo(int channelId, BatteryInfo batteryInfo) {
		if (Logging.DEBUG) Log.d(TAG, "send battery info");
		
		PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
		
		WorkerOp workerOp = WorkerOp.SendBatteryInfo;
		if (mWorkerThread != null) {
			if (channel.begin(workerOp)) {
				mWorkerThread.sendBatteryInfo(channelId, batteryInfo);
				return true;
			}
		}
		return false;
	}
	
	/* retrieve pending output for specified operation */
	public Object getPedingWorkerOutput(int channelId, WorkerOp workerOp) {
		PendingController<WorkerOp> channel = mWorkerPendingChannels[channelId];
		return channel.takePendingOutput(workerOp);
	}
	
	/* notifying methods */
	private synchronized void notifyClientStart() {
		// inform that, service finished work
		mIsWorking = false;
		mStartingClient.set(false);
		if (mStartAtRestarting || mApp.restartedAfterReinstall()) {
			mIsRestarted = true;
			mStartAtRestarting = false;
		}
		
		notifyChangeIsWorking();
		
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onClientStart();
			}
		});
	}
	
	private synchronized void notifyClientStop(final int exitCode, final boolean stoppedByManager) {
		mStartAtRestarting = false;
		mIsRestarted = false;
		
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
				mShuttingDownClient.set(false);
				// inform that, service finished work
				mIsWorking = false;
				notifyChangeIsWorking();
				
				mListenerHandler.onClientStop(exitCode, stoppedByManager);
				// if after shutdown we clear all pending objects
				for (int i = 0; i < mWorkerPendingChannels.length; i++)
					mWorkerPendingChannels[i].cancelAll();
				
				if (mDoRestart) {
					if (Logging.DEBUG) Log.d(TAG, "After shutdown, start native client");
					mDoRestart = false;
					mStartAtRestarting = true;
					startClient(false);
				}
			}
		});
	}
	
	private synchronized void notifyClientError(final String message) {
		// inform that, service finished work
		mIsWorking = false;
		mStartAtRestarting = false;
		mIsRestarted = false;
		mShuttingDownClient.set(false);
		mStartingClient.set(false);
		notifyChangeIsWorking();
		
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onNativeBoincError(message);
				// if after shutdown we clear all pending objects
				for (int i = 0; i < mWorkerPendingChannels.length; i++)
					mWorkerPendingChannels[i].cancelAll();
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
	 * start notify battery info
	 */
	private void startNotifyingBatteryState() {
		if (!mBatteryNotifying) {
			if (mListenerHandler != null) {
				registerReceiver(mBatteryStateReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
				mListenerHandler.post(mBatteryNotifier);
			}
			mBatteryNotifying = true;
		}
	}
	
	public void stopNotifyingBatteryState() {
		if (mBatteryNotifying) {
			if (mListenerHandler != null)
				mListenerHandler.removeCallbacks(mBatteryNotifier);
			unregisterReceiver(mBatteryStateReceiver);
			mBatteryNotifying = false;
		}
	}
	
	/**
	 * add project handling, queues and installer service bounding
	 */
	
	/**
	 * contains project urls
	 */
	private LinkedList<String> mPendingProjectAppsToInstall = new LinkedList<String>();
	/**
	 * contains key=project name, value=projectUrl 
	 */
	private HashMap<String, String> mInstalledProjectApps = new HashMap<String, String>();
	
	/**
	 * project urls whose marked as to install before adding project finished
	 */
	private HashSet<String> mProjectsMarkedToInstall = new HashSet<String>();
	
	private InstallerService mInstaller = null;
	
	private ServiceConnection mInstallerConn = new ServiceConnection() {
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (Logging.DEBUG) Log.d(TAG, "Installer is unbounded");
			if (mInstaller != null)
				mInstaller.removeInstallerListener(NativeBoincService.this);
			mInstaller = null;
		}
		
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			if (Logging.DEBUG) Log.d(TAG, "Installer is bounded");
			mInstaller = ((InstallerService.LocalBinder)service).getService();
			mInstaller.addInstallerListener(NativeBoincService.this);
			// update installed distrib list before installing applications (IMPORTANT)
			mInstaller.synchronizeInstalledProjects();
			
			mInstallerInBounding = false;
			
			// do install
			synchronized(mPendingProjectAppsToInstall) {
				String[] toInstall = mPendingProjectAppsToInstall.toArray(new String[0]);
				mPendingProjectAppsToInstall.clear();
				mProjectsMarkedToInstall.clear();
				
				for (String projectUrl: toInstall)
					installProjectApplication(projectUrl);
			}
		}
	};
	
						
	public void markProjectUrlToInstall(String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "Mark project url:"+projectUrl);
		synchronized(mPendingProjectAppsToInstall) {
			mProjectsMarkedToInstall.add(projectUrl);
		}
	}
	
	public void unmarkProjectUrlToInstall(String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "Unmark project url:"+projectUrl);
		synchronized(mPendingProjectAppsToInstall) {
			mProjectsMarkedToInstall.remove(projectUrl);
		}
	}
	/**
	 * check whether project currently installed by auto installer
	 */
	public int getProjectStateForAutoInstaller(String projectUrl) {
		boolean beingPending = false;
		boolean beingInstalled = false;
		synchronized(mPendingProjectAppsToInstall) {
			// if processed by autoinstaller or is marked by foreign object
			beingPending = mPendingProjectAppsToInstall.contains(projectUrl) ||
					mProjectsMarkedToInstall.contains(projectUrl);
		}
		
		if (beingPending) {
			if (Logging.DEBUG) Log.d(TAG, "ProjectState "+projectUrl+" is "+
					ProjectAutoInstallerState.IN_AUTOINSTALLER);
			return ProjectAutoInstallerState.IN_AUTOINSTALLER;
		}
		
		synchronized(mInstalledProjectApps) {
			beingInstalled = mInstalledProjectApps.values().contains(projectUrl);
		}
		
		int state = beingInstalled ? ProjectAutoInstallerState.BEING_INSTALLED :
			ProjectAutoInstallerState.NOT_IN_AUTOINSTALLER;
		if (Logging.DEBUG) Log.d(TAG, "ProjectState "+projectUrl+" is "+ state);
		return state;
	}
						
	private boolean mInstallerInBounding = false;
	private boolean mIfProjectDistribsListUpdated = false;
	private AtomicBoolean mProjectDistribsListUpdating = new AtomicBoolean(false);
	
	private void ifDistribNotFoundOrProvidedByProject(String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "again not found. to finish");
		// simply finish autoinstallation
		finishAutoInstallationIfNeeded();
		// send notifyProjectsNotFound
		List<String> toSend = new ArrayList<String>();
		toSend.add(projectUrl);
		notifyProjectsNotFound(toSend);
		// we update waiting for benchmark (because we dont install it)
		SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		// unset waiting for benchmark
		sharedPrefs.edit().putBoolean(PreferenceName.WAITING_FOR_BENCHMARK, false)
				.commit();
	}
	
	/**
	 * real project installtion routine
	 * @param projectUrl
	 */
	private void installProjectApplication(String projectUrl) {
		ProjectDistrib projectDistrib = mInstaller.findProjectDistrib(projectUrl);
		
		if (projectDistrib == null) {	// do nothing if not found
			if (Logging.DEBUG) Log.d(TAG, "attached not found in distribs!!!");
			if (!mIfProjectDistribsListUpdated) { // if not updated before
				// add to pending tasks
				if (Logging.DEBUG) Log.d(TAG, "try to update project distribs");
				synchronized (mPendingProjectAppsToInstall) {
					mPendingProjectAppsToInstall.add(projectUrl);
				}
				// try to update project list (with own channel id)
				if (mProjectDistribsListUpdating.compareAndSet(false, true)) {
					if (Logging.DEBUG) Log.d(TAG, "trying update project distribs");
					// update project distribs list
					mInstaller.updateProjectDistribList(NATIVEBOINC_ID, true);
				}
			} else
				ifDistribNotFoundOrProvidedByProject(projectUrl);
			return;
		}
		
		if (!projectDistrib.binariesProvidedByNativeBOINC()) { // end it if binaries provided by project
			ifDistribNotFoundOrProvidedByProject(projectUrl); 
			return;
		}
		
		synchronized (mInstalledProjectApps) {
			mInstalledProjectApps.put(projectDistrib.projectName, projectUrl);
		}
		
		// notify event
		notifyBeginProjectInstallation(projectUrl);
		// go to install
		mInstaller.installProjectApplicationsAutomatically(projectDistrib.projectName, projectUrl);
	}
	
	private void startInstallProjectApplication(String projectUrl) {
		if (mInstaller == null) {
			if (Logging.DEBUG) Log.d(TAG, "Installer service not bound");
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
					getString(R.string.eventAttachedProjectMessage, event.projectUrl), false);
			
			startInstallProjectApplication(event.projectUrl);
			break;
		case ClientEvent.EVENT_DETACHED_PROJECT:
			mNotificationController.notifyClientEvent(getString(R.string.eventDetachedProject), 
					getString(R.string.eventDetachedProjectMessage, event.projectUrl), false);
			break;
		case ClientEvent.EVENT_RUN_BENCHMARK:
			title = getString(R.string.eventBencharkRun);
			mNotificationController.notifyClientEvent(title, title, true);
			break;
		case ClientEvent.EVENT_FINISH_BENCHMARK:
			title = getString(R.string.eventBencharkFinished);
			mNotificationController.notifyClientEvent(title, title, true);
			break;
		case ClientEvent.EVENT_SUSPEND_ALL_TASKS:
			if (event.suspendReason != ClientEvent.SUSPEND_REASON_CPU_THROTTLE) {
				// notify only if reason is not CPU THROTTLE!
				title = getString(R.string.eventSuspendAll);
				mNotificationController.notifyClientEvent(title, title, false);
			} else
				mCpuThrottleSuspendAll = true; // inform, that tasks has been suspended by cpu throttle
			break;
		case ClientEvent.EVENT_RUN_TASKS:
			if (!mCpuThrottleSuspendAll) { // if not reason cpu throttle
				title = getString(R.string.eventRunTasks);
				mNotificationController.notifyClientEvent(title, title, false);
			}
			mCpuThrottleSuspendAll = false;
			break;
		case ClientEvent.EVENT_BATTERY_NOT_DETECTED:
			// start notify client about battery state
			startNotifyingBatteryState();
			Toast.makeText(mApp, R.string.nativeClientNotifyingBatteryInfo, Toast.LENGTH_LONG).show();
			break;
		}
	}
	
	@Override
	public void onMonitorDoesntWork() {
		Intent intent = new Intent(NativeBoincService.this, ClientMonitorErrorActivity.class);
		intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		startActivity(intent);
	}
	
	private void finishAutoInstallationIfNeeded() {
		boolean ifLast = false;
		synchronized (mInstalledProjectApps) {
			ifLast = mInstalledProjectApps.isEmpty();
			if (Logging.DEBUG) Log.d(TAG, "is last:"+ifLast);
		}
		boolean pendingIsEmpty = false;
		synchronized(mPendingProjectAppsToInstall) {
			pendingIsEmpty = mPendingProjectAppsToInstall.isEmpty();
		}

		if (ifLast && pendingIsEmpty)
			unboundInstallService();
	}
	
	private void finishProjectApplicationInstallation(String projectName) {
		boolean ifLast = false;
		synchronized (mInstalledProjectApps) {
			mInstalledProjectApps.remove(projectName);
			ifLast = mInstalledProjectApps.isEmpty();
			if (Logging.DEBUG) Log.d(TAG, "is last:"+ifLast);
		}
		boolean pendingIsEmpty = false;
		synchronized(mPendingProjectAppsToInstall) {
			pendingIsEmpty = mPendingProjectAppsToInstall.isEmpty();
		}

		if (ifLast && pendingIsEmpty)
			unboundInstallService();
	}
	
	private void unboundInstallService() {
		// if during installation step
		if (mApp.isInstallerRun()) {
			if (Logging.DEBUG) Log.d(TAG, "Set installer stage as finish");
			mApp.setInstallerStage(BoincManagerApplication.INSTALLER_FINISH_STAGE);
		}
		
		mIfProjectDistribsListUpdated = false;	// reset this indicator
		mProjectDistribsListUpdating.set(false);
		/* do unbound installer service */
		if (Logging.DEBUG) Log.d(TAG, "Unbound installer service");
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

	private void onOperationErrorOrCancel(InstallOp installOp, String distribName) {
		mProjectDistribsListUpdating.set(false);
		// if error from project distribs update
		if (installOp.equals(InstallOp.UpdateProjectDistribs)) {
			// is not distrib (updating project distrib list simply failed)
			if (Logging.DEBUG) Log.d(TAG, "on operation failed");
			
			ArrayList<String> projectsNotFoundList = new ArrayList<String>(); 
			synchronized (mPendingProjectAppsToInstall) {
				projectsNotFoundList.addAll(mPendingProjectAppsToInstall);
				mPendingProjectAppsToInstall.clear();
				mProjectsMarkedToInstall.clear();
			}
			
			// notify autoinstall event
			notifyProjectsNotFound(projectsNotFoundList);
			
			boolean ifLast = false; 
			synchronized (mInstalledProjectApps) {
				ifLast = mInstalledProjectApps.isEmpty();
			}
			if (ifLast)
				unboundInstallService();
		} else if (distribName != null && distribName.length() != 0) // if project name
			finishProjectApplicationInstallation(distribName);
	}
	
	/* we do not consume error events, because only one listener can handle error (in this case is
	 * active activity) */
	@Override
	public boolean onOperationError(InstallOp installOp, String distribName, String errorMessage) {
		onOperationErrorOrCancel(installOp, distribName);
		// do not consume error data
		return false;
	}

	@Override
	public void onOperationCancel(InstallOp installOp, String distribName) {
		onOperationErrorOrCancel(installOp, distribName);
	}

	@Override
	public void onOperationFinish(InstallOp installOp, String distribName) {
		if (installOp.equals(InstallOp.ProgressOperation)) // if installation operation
			finishProjectApplicationInstallation(distribName);
	}

	@Override
	public void currentProjectDistribList(ArrayList<ProjectDistrib> projectDistribs) {
		if (Logging.DEBUG) Log.d(TAG, "on currentProjectDistrib"); 
		mIfProjectDistribsListUpdated = true;
		mProjectDistribsListUpdating.set(false);
		// try again
		String projectUrl = null;
		while (true) {
			// synchronize around all single poll 
			synchronized(mPendingProjectAppsToInstall) {
				projectUrl = mPendingProjectAppsToInstall.poll();
				
				if (projectUrl == null) // if end
					break;
				
				mProjectsMarkedToInstall.remove(projectUrl);
				if (Logging.DEBUG) Log.d(TAG, "after updating:"+projectUrl);
				installProjectApplication(projectUrl);
			}
		}
	}

	@Override
	public void currentClientDistrib(ClientDistrib clientDistrib) {
	}

	@Override
	public void onChangeInstallerIsWorking(boolean isWorking) {
	}

	@Override
	public void binariesToUpdateOrInstall(UpdateItem[] updateItems) {
	}

	@Override
	public void binariesToUpdateFromSDCard(String[] projectNames) {
	}
	
	/* event handling for project installation */
	private synchronized void notifyBeginProjectInstallation(final String projectUrl) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.beginProjectInstallation(projectUrl);
			}
		});
	}
	
	private synchronized void notifyProjectsNotFound(final List<String> projectUrls) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.projectsNotFound(projectUrls);
			}
		});
	}
}
