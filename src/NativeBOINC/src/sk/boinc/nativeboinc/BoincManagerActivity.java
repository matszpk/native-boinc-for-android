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

package sk.boinc.nativeboinc;

import hal.android.workarounds.FixedProgressDialog;

import java.util.ArrayList;

import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.ClientPollReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientUpdateNoticesReceiver;
import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.clientconnection.NoticeInfo;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincStateListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.service.ConnectionManagerService;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.ScreenOrientationHandler;
import sk.boinc.nativeboinc.util.StandardDialogs;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.app.TabActivity;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.DialogInterface.OnCancelListener;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.widget.TabHost;
import android.widget.TextView;
import android.widget.Toast;


public class BoincManagerActivity extends TabActivity implements ClientUpdateNoticesReceiver,
		ClientPollReceiver, NativeBoincStateListener {
	private static final String TAG = "BoincManagerActivity";

	private static final int DIALOG_CONNECT_PROGRESS = 1;
	private static final int DIALOG_SHUTDOWN         = 2;
	private static final int DIALOG_RESTART_PROGRESS = 3;
	private static final int DIALOG_START_PROGRESS   = 4;
	private static final int DIALOG_UPGRADE_INFO     = 5;
	private static final int DIALOG_SHUTDOWN_PROGRESS   = 6;

	private static final int ACTIVITY_SELECT_HOST   = 1;
	private static final int ACTIVITY_MANAGE_CLIENT = 2;
	private static final int ACTIVITY_NATIVE_CLIENT = 3;
	
	public static final String PARAM_CONNECT_NATIVE_CLIENT = "ConnectNative";

	private static final int BACK_PRESS_PERIOD = 5;

	private BoincManagerApplication mApp;
	private ScreenOrientationHandler mScreenOrientation;
	//private WakeLock mWakeLock;
	//private boolean mScreenAlwaysOn = false;
	private boolean mBackPressedRecently = false;
	private Handler mHandler = new Handler();
	private boolean mJustUpgraded = false;

	private StringBuilder mSb = new StringBuilder();
	private int mConnectProgressIndicator = -1;
	private boolean mProgressDialogAllowed = false;

	private ClientId mConnectedClient = null;
	private VersionInfo mConnectedClientVersion = null;
	private ClientId mSelectedClient = null;
	private boolean mInitialDataRetrievalStarted = false;
	private boolean mInitialDataAvailable = false;

	private boolean mDoConnectNativeClient = false;
	
	private boolean mConnectClientAfterRestart = false;
	private boolean mConnectClientAfterStart = false;
	
	private boolean mIsInstallerRan = false;
	private boolean mShowShutdownDialog = false;
	
	private boolean mShowStartDialog = false;
	
	// used to delayed showing ChangeLog dialog
	private boolean mInDuringInstallation = false;
	
	// true when 'Shutdown' option selected and shutdown confirmed
	private boolean mStoppedInMainActivity = false;
	
	private boolean mIsPaused = false;
	private boolean mIsRecreated = false;
	
	private static class SavedState {
		private final boolean isInstallerRan;
		private final boolean initialDataAvailable;
		private final int connectProgressIndicator;
		private final boolean connectClientAfterStart;
		private final boolean connectClientAfterRestart;
		private final boolean doConnectNativeClient;
		private final boolean showShutdownDialog;
		private final boolean stoppedInMainActivity;

		public SavedState(BoincManagerActivity activity) {
			isInstallerRan = activity.mIsInstallerRan;
			initialDataAvailable = activity.mInitialDataAvailable;
			connectClientAfterStart = activity.mConnectClientAfterStart;
			connectClientAfterRestart = activity.mConnectClientAfterRestart;
			if (Logging.DEBUG) Log.d(TAG, "saved: initialDataAvailable=" + initialDataAvailable);
			connectProgressIndicator = activity.mConnectProgressIndicator;
			if (Logging.DEBUG) Log.d(TAG, "saved: connectProgressIndicator=" + connectProgressIndicator);
			doConnectNativeClient = activity.mDoConnectNativeClient;
			showShutdownDialog = activity.mShowShutdownDialog;
			stoppedInMainActivity = activity.mStoppedInMainActivity;
		}
		public void restoreState(BoincManagerActivity activity) {
			activity.mInitialDataAvailable = initialDataAvailable;
			if (Logging.DEBUG) Log.d(TAG, "restored: mInitialDataAvailable=" + activity.mInitialDataAvailable);
			activity.mConnectProgressIndicator = connectProgressIndicator;
			if (Logging.DEBUG) Log.d(TAG, "restored: mConnectProgressIndicator=" +
					activity.mConnectProgressIndicator);
			activity.mIsInstallerRan = isInstallerRan;
			activity.mConnectClientAfterStart = connectClientAfterStart;
			activity.mConnectClientAfterRestart = connectClientAfterRestart;
			activity.mDoConnectNativeClient = doConnectNativeClient;
			activity.mShowShutdownDialog = showShutdownDialog;
			activity.mStoppedInMainActivity = stoppedInMainActivity;
		}
	}

	private ConnectionManagerService mConnectionManager = null;

	private ServiceConnection mServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mConnectionManager = ((ConnectionManagerService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "onServiceConnected()");
			mConnectionManager.registerStatusObserver(BoincManagerActivity.this);
			// If service is already connected to client, it will call back the clientConnected()
			// So the mConnectedClient will be set now.
			if (mSelectedClient != null) {
				ClientId currentlyOpenedClient = mConnectionManager.getClientId();
				if (currentlyOpenedClient != null && currentlyOpenedClient.equals(mSelectedClient)) {
					// reuse opened connection
					if (Logging.DEBUG) Log.d(TAG, "Reuse opened connection");
					mConnectedClient = currentlyOpenedClient;
					// available, because opened connection on previous session
					mInitialDataAvailable = true;
				} else {
					// Some client was selected at the time when service was not bound yet
					// Now the service is available, so connection can proceed
					connectOrReconnect();
				}
			}
			// handle power management
			if (!mIsRecreated) // if not recreated
				mConnectionManager.acquireLockScreenOn();
			/* display error if pending */
			updateActivityState();
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mConnectionManager = null;
			// This should not happen normally, because it's local service 
			// running in the same process...
			if (Logging.WARNING) Log.w(TAG, "onServiceDisconnected()");
			// We also reset client reference to prevent mess
			mConnectedClient = null;
			mSelectedClient = null;
			setProgressBarIndeterminateVisibility(false);
		}
	};
	
	private void checkToRunInstaller() {
		HostListDbAdapter dbHelper = new HostListDbAdapter(BoincManagerActivity.this);
		dbHelper.open();
		// check nativeboinc entry
		ClientId clientId = dbHelper.fetchHost("nativeboinc");
		dbHelper.close();
		
		if (!mIsInstallerRan) {
			if (mApp.isInstallerRun()) {
				// open again installer activity
				mIsInstallerRan = true;
				mApp.runInstallerActivity(BoincManagerActivity.this);
			} else if ((!InstallerService.isClientInstalled(this) || clientId == null)) {
				// run installation wizard
				mIsInstallerRan = true;
				startActivity(new Intent(BoincManagerActivity.this,
						InstallStep1Activity.class));
			}
		}
	}
	
	private NativeBoincService mRunner = null;
	
	private ServiceConnection mRunnerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceConnected()");
			
			onRunnerStart();
		}
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			mRunner = null;
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceDisconnected()");
		}
	};

	private void doBindService() {
		bindService(new Intent(BoincManagerActivity.this, ConnectionManagerService.class),
				mServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doBindRunnerService() {
		bindService(new Intent(BoincManagerActivity.this, NativeBoincService.class),
				mRunnerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindService() {
		if (Logging.DEBUG) Log.d(TAG, "doUnbindService()");
		unbindService(mServiceConnection);
		mConnectionManager = null;
	}
	
	private void doUnbindRunnerService() {
		if (Logging.DEBUG) Log.d(TAG, "doUnbindRunnerService()");
		if (mRunner != null)
			mRunner.removeNativeBoincListener(this);
		unbindService(mRunnerServiceConnection);
		mRunner = null;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		if (Logging.DEBUG) Log.d(TAG, "onCreate()");

		mApp = (BoincManagerApplication)getApplication();
		mJustUpgraded = mApp.getJustUpgradedStatus();

		// Create handler for screen orientation
		mScreenOrientation = new ScreenOrientationHandler(this);

		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		// Restore state on configuration change (if applicable)
		if (savedState != null) {
			// Yes, we have the saved state, this is activity re-creation after configuration change
			savedState.restoreState(this);
			mIsRecreated = true;
		} else { // restore from intent extra
			mDoConnectNativeClient = getIntent().getBooleanExtra(PARAM_CONNECT_NATIVE_CLIENT, false);
			mIsRecreated = false;
		}
		
		if (Logging.DEBUG) Log.d(TAG, "doOpenNativeClient:"+mDoConnectNativeClient);
		
		doBindService();
		doBindRunnerService();

		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		setContentView(R.layout.main_view);

		TabHost tabHost = getTabHost();
		Resources res = getResources();

		// Tab 1 - Projects
		tabHost.addTab(tabHost.newTabSpec("tab_projects")
				.setIndicator(getString(R.string.projects), res.getDrawable(R.drawable.ic_tab_projects))
				.setContent(new Intent(this, ProjectsActivity.class)));

		// Tab 2 - Tasks
		tabHost.addTab(tabHost.newTabSpec("tab_tasks")
				.setIndicator(getString(R.string.tasks), res.getDrawable(R.drawable.ic_tab_tasks))
				.setContent(new Intent(this, TasksActivity.class)));

		// Tab 3 - Transfers
		tabHost.addTab(tabHost.newTabSpec("tab_transfers")
				.setIndicator(getString(R.string.transfers), res.getDrawable(R.drawable.ic_tab_transfers))
				.setContent(new Intent(this, TransfersActivity.class)));

		// Tab 4 - Messages
		tabHost.addTab(tabHost.newTabSpec("tab_messages")
				.setIndicator(getString(R.string.messages), res.getDrawable(R.drawable.ic_tab_messages))
				.setContent(new Intent(this, MessagesActivity.class)));
		
		// Tab 5 - Notices
		tabHost.addTab(tabHost.newTabSpec("tab_notices")
				.setIndicator(getString(R.string.notices), res.getDrawable(R.drawable.ic_tab_notices))
				.setContent(new Intent(this, NoticesActivity.class)));

		// Set all tabs one by one, to start all activities now
		// It is better to receive early updates of data
		tabHost.setCurrentTabByTag("tab_messages");
		tabHost.setCurrentTabByTag("tab_tasks");
		tabHost.setCurrentTabByTag("tab_transfers");
		tabHost.setCurrentTabByTag("tab_projects");
		tabHost.setCurrentTabByTag("tab_notices");
		// Set saved tab (the last selected on previous run) as current
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		int lastActiveTab = globalPrefs.getInt(PreferenceName.LAST_ACTIVE_TAB, 1);
		tabHost.setCurrentTab(lastActiveTab);

		/*
		 * adds on click listeners. clicking on tab causes refreshing content
		 */
		getTabWidget().getChildAt(0).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.d(TAG, "Click tab: 0");
				TabHost tabHost = getTabHost();
				if (tabHost.getCurrentTab() != 0)
					getTabHost().setCurrentTab(0);
				
				if (Logging.DEBUG) Log.d(TAG, "Update projects");
				if (mConnectedClient != null)
					mConnectionManager.updateProjects();
			}
		});
		getTabWidget().getChildAt(1).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.d(TAG, "Click tab: 1");
				TabHost tabHost = getTabHost();
				if (tabHost.getCurrentTab() != 1)
					getTabHost().setCurrentTab(1);
				
				if (Logging.DEBUG) Log.d(TAG, "Update tasks");
				if (mConnectedClient != null)
					mConnectionManager.updateTasks();
			}
		});
		getTabWidget().getChildAt(2).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.d(TAG, "Click tab: 2");
				TabHost tabHost = getTabHost();
				if (tabHost.getCurrentTab() != 2)
					getTabHost().setCurrentTab(2);
				
				if (Logging.DEBUG) Log.d(TAG, "Update transfers");
				if (mConnectedClient != null)
					mConnectionManager.updateTransfers();
			}
		});
		getTabWidget().getChildAt(3).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.d(TAG, "Click tab: 3");
				TabHost tabHost = getTabHost();
				if (tabHost.getCurrentTab() != 3)
					getTabHost().setCurrentTab(3);
				
				if (Logging.DEBUG) Log.d(TAG, "Update messages");
				if (mConnectedClient != null)
					mConnectionManager.updateMessages();
			}
		});
		
		getTabWidget().getChildAt(4).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.d(TAG, "Click tab: 4");
				TabHost tabHost = getTabHost();
				if (tabHost.getCurrentTab() != 4)
					getTabHost().setCurrentTab(4);
				
				if (Logging.DEBUG) Log.d(TAG, "Update notices");
				if (mConnectedClient != null)
					mConnectionManager.updateNotices();
			}
		});
		
		
		getTabHost().setOnTabChangedListener(new TabHost.OnTabChangeListener() {
			
			@Override
			public void onTabChanged(String tabId) {
				Log.d(TAG, "Change tab: "+ tabId);
			}
		});
		
		if (savedState == null) {
			// Just normal start
			String autoConnectHost = globalPrefs.getString(PreferenceName.AUTO_CONNECT_HOST, null);
			if (((autoConnectHost != null) && globalPrefs.getBoolean(PreferenceName.AUTO_CONNECT, false)) ||
					(autoConnectHost == null) || mDoConnectNativeClient) {
				// if no autoConnect host and if doConnectNativeClient (should open native client)
				if (Logging.DEBUG) Log.d(TAG, "doOpenNativeClient 2:"+mDoConnectNativeClient);
				if (autoConnectHost == null || mDoConnectNativeClient)	// use nativeboinc as autoconnect
					autoConnectHost = "nativeboinc";
				// We should auto-connect to recently connected host
				HostListDbAdapter dbHelper = new HostListDbAdapter(this);
				dbHelper.open();
				mSelectedClient = dbHelper.fetchHost(autoConnectHost);
				dbHelper.close();
			}
			if (Logging.INFO) {
				if (mSelectedClient != null) 
					Log.i("ClientSelection", "Selecting "+mSelectedClient.getNickname());
				else
					Log.i("ClientSelection", "Nothing selected");
			}
		}
		
		/* check whether installer should be run (if should be, then run) */
		checkToRunInstaller();
	}

	private void updateActivityState() {
		// display/hide progress
		boolean connectionManagerIsWorking = mConnectionManager != null &&
				mConnectionManager.isWorking();
		boolean runnerIsWorking = mRunner != null &&
				mRunner.serviceIsWorking();
		
		if (connectionManagerIsWorking || runnerIsWorking)
			setProgressBarIndeterminateVisibility(true);
		else
			setProgressBarIndeterminateVisibility(false);
		
		if (mShowShutdownDialog && mRunner != null && !mRunner.isRun()) {
			// dismiss dialog
			mShowShutdownDialog = false;
			dismissDialog(DIALOG_SHUTDOWN);
		}
		
		/* display error if pending */
		if (mRunner != null && mConnectionManager != null) {
			
			boolean isError = mConnectionManager.handlePendingClientErrors(null, this);
			isError |= mConnectionManager.handlePendingPollErrors(null, this);
			isError |= mRunner.handlePendingErrorMessage(this);
			
			if (isError) // if error then do nothing
				return;
			
			if (mStoppedInMainActivity && !mRunner.isRun()) {
				mStoppedInMainActivity = false;
				dismissDialog(DIALOG_SHUTDOWN_PROGRESS);
			}
		}
		if (Build.VERSION.SDK_INT >= 11)
			invalidateOptionsMenu();
	}
	
	private void onRunnerStart() {
		if (Logging.DEBUG) Log.d(TAG, "reregister runner listener");
		mRunner.addNativeBoincListener(this);
		if (!mConnectClientAfterRestart || mConnectClientAfterStart) { 
			// if not connected after restart or is second phase
			if (mRunner.isRun())
				onClientStart();	// if client start
			
			if (mSelectedClient != null) {
				// We just returned from activity which selected client to connect to
				if (mConnectionManager != null) {
					// Service is bound, we can use it
					connectOrReconnect();
				}
				else {
					// Service not bound at the moment (too slow start? or disconnected itself?)
					// We trigger re-bind again (does not hurt if it's duplicate)
					if (Logging.DEBUG) Log.d(TAG,
							"onResume() - Client selected, but service not yet available => binding again");
					doBindService();
				}
			}
			else if (mInitialDataRetrievalStarted) {
				// We started retrieval of important data, which will take some time
				// We display progress dialog about it
				showProgressDialog(PROGRESS_INITIAL_DATA);
			}
		} else {
			if (mConnectionManager != null)
				doBindService();
			if (Logging.DEBUG) Log.d(TAG, "Clearly, after restart");
			// treat restart as start now! (second phase)
			mConnectClientAfterStart = true;
		}
		// update activity state
		updateActivityState();
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		mIsPaused = false;
		if (Logging.DEBUG) Log.d(TAG, "onResume()");
		mBackPressedRecently = false;
		mScreenOrientation.setOrientation();
		// Update name of connected client (or show "not connected")
		updateTitle();
		// Show Information about upgrade, if applicable
		mInDuringInstallation = mApp.isInstallerRun() || !InstallerService.isClientInstalled(this);
		
		if (Logging.DEBUG) Log.d(TAG, "mInDuringInstallation:"+mInDuringInstallation);
		if (mJustUpgraded && !mInDuringInstallation) {
			mJustUpgraded = false; // Do not show again
			mProgressDialogAllowed = false;
			// Now show the dialog about upgrade
			showDialog(DIALOG_UPGRADE_INFO);
		}
		else {
			// Progress dialog is allowed since now
			mProgressDialogAllowed = true;
		}
		// If applicable (e.g scheduled), connect to the host
		// Even in case that ChangeLog is being shown on upgrade, connect will still be done,
		// but progress dialog is suppressed (will be enabled on dismiss of ChangeLog dialog)
		
		if (mRunner == null) {
			if (Logging.DEBUG) Log.d(TAG, "onResume() - runner not present");
			doBindRunnerService();
		} else // when runner starts
			onRunnerStart();
	}

	@Override
	protected void onPause() {
		super.onPause();
		if (Logging.DEBUG) Log.d(TAG, "onPause()");
		mProgressDialogAllowed = false;
		// If we did not perform deferred connect so far, we needn't do that anymore
		if (mSelectedClient != null) {
			if (!mSelectedClient.isNativeClient() || !mApp.isBoincClientRun() ||
					(!mConnectClientAfterRestart && !mConnectClientAfterStart))
				// only when is native client is not started by manager
				mSelectedClient = null;
		}
		if (Logging.DEBUG) Log.d(TAG, "unregister runner listener");
		if (mRunner != null)
			mRunner.removeNativeBoincListener(this);
		mIsPaused = true;
	}

	@Override
	protected void onStop() {
		super.onStop();
		if (Logging.DEBUG) Log.d(TAG, "onStop()");
		if (isFinishing()) {
			// Activity is not only invisible, but someone requested it to finish
			if (Logging.DEBUG) Log.d(TAG, "Activity is finishing NOW");
			// Save currently selected tab, so it can be restored on next run
			SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(this);
			SharedPreferences.Editor editor = globalPrefs.edit();
			if (globalPrefs.getBoolean(PreferenceName.AUTO_CONNECT, false) &&
					(mConnectedClient != null)) {
				// Automatic connect is enabled and we are still connected;
				// We save currently connected client's ID, so next time
				// We can connect automatically
				editor.putString(PreferenceName.AUTO_CONNECT_HOST, mConnectedClient.getNickname());
			}
			else {
				// Automatic connect disabled or we are not connected;
				// Remove previously saved client's ID
				editor.remove(PreferenceName.AUTO_CONNECT_HOST);
			}
			editor.putInt(PreferenceName.LAST_ACTIVE_TAB, getTabHost().getCurrentTab());
			editor.commit();
		}
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		if (Logging.DEBUG) Log.d(TAG, "onDestroy()");
		removeDialog(DIALOG_CONNECT_PROGRESS);
		if (mConnectionManager != null) {
			mConnectionManager.unregisterStatusObserver(this);
			mConnectedClient = null;
		}
		if (mRunner != null) {
			mRunner.removeNativeBoincListener(this);
			mRunner = null;
		}
		doUnbindService();
		doUnbindRunnerService();
		mScreenOrientation = null;
	}
	
	@Override
	public void finish() {
		// finish
		if (mConnectionManager != null)
			mConnectionManager.releaseLockScreenOn();
		super.finish();
	}

	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (event.getRepeatCount() == 0) {
			if (keyCode == KeyEvent.KEYCODE_BACK) {
				// Back button pressed
				if (!mBackPressedRecently) {
					// Back button was not pressed recently
					mBackPressedRecently = true;
					Toast.makeText(this, getString(R.string.closeWarning), Toast.LENGTH_SHORT).show();
					mHandler.postDelayed(new Runnable() {
						@Override
						public void run() {
							// reset flag
							mBackPressedRecently = false;
						}}, 
						BACK_PRESS_PERIOD * 1000);
					// Return true, so default handling of Back button is suppressed
					return true;
				}
			}
			else if (mBackPressedRecently) {
				// Pressed other than Back button (after previous press of Back)
				mBackPressedRecently = false;
			}
		}
		return super.onKeyDown(keyCode, event);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		super.onCreateOptionsMenu(menu);
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.main_menu, menu);
		return true;
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		super.onPrepareOptionsMenu(menu);
		MenuItem item;
		item = menu.findItem(R.id.menuDisconnect);
		item.setVisible(mConnectedClient != null);
		item = menu.findItem(R.id.menuStartUp);
		if (mRunner != null)
			item.setVisible(!mRunner.isRun());
		else
			item.setVisible(false);
		item = menu.findItem(R.id.menuShutdown);
		if (mRunner != null)
			item.setVisible(mRunner.isRun());
		else
			item.setVisible(false);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.menuStartUp: {
			mConnectClientAfterStart = true;
			showDialog(DIALOG_START_PROGRESS);
			if (mRunner != null)
				mRunner.startClient(false);
			return true;
		}
		case R.id.menuShutdown:
			mShowShutdownDialog = true;
			showDialog(DIALOG_SHUTDOWN);
			return true;
		case R.id.menuManage:
			// Launch new activity
			startActivityForResult(new Intent(this, ManageClientActivity.class), ACTIVITY_MANAGE_CLIENT);
			return true;
		case R.id.menuNativeClient:
			startActivityForResult(new Intent(this, NativeClientActivity.class), ACTIVITY_NATIVE_CLIENT);
			return true;
		case R.id.menuPreferences:
			// Launch new activity for adjusting the preferences
			startActivity(new Intent(this, AppPreferencesActivity.class));
			return true;
		case R.id.menuConnect:
			// Launch new activity to select a client
			startActivityForResult(new Intent(this, HostListActivity.class), ACTIVITY_SELECT_HOST);
			return true;
		case R.id.menuDisconnect:
			// Disconnect from currently connected client
			boincDisconnect();
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	protected Dialog onCreateDialog(int dialogId, Bundle args) {
		Dialog dialog = StandardDialogs.onCreateDialog(this, dialogId, args);
		if (dialog != null)
			return dialog;
		
		View v;
		TextView text;
		switch (dialogId) {
		case DIALOG_CONNECT_PROGRESS:
			if (Logging.DEBUG) Log.d(TAG, "onCreateDialog(DIALOG_CONNECT_PROGRESS)");
			ProgressDialog progressDialog = new FixedProgressDialog(this);
            progressDialog.setIndeterminate(true);
			progressDialog.setCancelable(true);
			progressDialog.setOnCancelListener(new OnCancelListener() {
				@Override
				public void onCancel(DialogInterface dialog) {
					// Connecting canceled
					mConnectProgressIndicator = -1;
					// Disconnect the client
					boincDisconnect();
				}
			});
            return progressDialog;
		case DIALOG_UPGRADE_INFO:
			v = LayoutInflater.from(this).inflate(R.layout.dialog, null);
			text = (TextView)v.findViewById(R.id.dialogText);
			mApp.setChangelogText(text);
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_info)
				.setTitle(getString(R.string.upgradedTo) + " " + mApp.getApplicationVersion())
				.setView(v)
				.setNegativeButton(R.string.dismiss, 
					new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int whichButton) {
							// Progress dialog is allowed since now
							mProgressDialogAllowed = true;
							if (Logging.DEBUG) Log.d(TAG, "Progress dialog allowed again");
						}					
					})
				.setOnCancelListener(new OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						// Progress dialog is allowed since now
						mProgressDialogAllowed = true;
						if (Logging.DEBUG) Log.d(TAG, "Progress dialog allowed again");
					}
				})
        		.create();
		case DIALOG_SHUTDOWN:
			if (mRunner != null && !mRunner.isRun()) {
				mShowShutdownDialog = false;
				return null;
			}
			return new AlertDialog.Builder(this)
	    		.setIcon(android.R.drawable.ic_dialog_alert)
	    		.setTitle(R.string.warning)
	    		.setMessage(R.string.shutdownAskText)
	    		.setPositiveButton(R.string.shutdown,
	    			new DialogInterface.OnClickListener() {
	    				public void onClick(DialogInterface dialog, int whichButton) {
	    					mShowShutdownDialog = false;
	    					showDialog(DIALOG_SHUTDOWN_PROGRESS);
	    					mStoppedInMainActivity = true;
	    					mRunner.shutdownClient();
	    				}
	    			})
	    		.setNegativeButton(R.string.cancel,
	    			new DialogInterface.OnClickListener() {
	    				public void onClick(DialogInterface dialog, int whichButton) {
	    					mShowShutdownDialog = false;
	    				}
	    			})
	    		.setOnCancelListener(new DialogInterface.OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						mShowShutdownDialog = false;
					}
				})
	    		.create();
		case DIALOG_START_PROGRESS:
		case DIALOG_SHUTDOWN_PROGRESS:
		case DIALOG_RESTART_PROGRESS: {
			if (dialogId != DIALOG_SHUTDOWN)
				mShowStartDialog = true;
			progressDialog = new FixedProgressDialog(this);
			progressDialog.setIndeterminate(true);
			progressDialog.setCancelable(true);
			return progressDialog;
		}
		}
		return null;
	}

	@Override
	protected void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		if (StandardDialogs.onPrepareDialog(this, dialogId, dialog, args))
			return;
		
		switch (dialogId) {
		case DIALOG_CONNECT_PROGRESS:
			ProgressDialog pd = (ProgressDialog)dialog;
			switch (mConnectProgressIndicator) {
			case PROGRESS_CONNECTING:
				pd.setMessage(getString(R.string.connecting));
				break;
			case PROGRESS_AUTHORIZATION_PENDING:
				pd.setMessage(getString(R.string.authorization));
				break;
			case PROGRESS_INITIAL_DATA:
				pd.setMessage(getString(R.string.retrievingInitialData));				
				break;
			default:
				pd.setMessage(getString(R.string.error));
				if (Logging.ERROR) Log.e(TAG, "Unhandled progress indicator: " + mConnectProgressIndicator);
			}
			break;
		case DIALOG_START_PROGRESS:
			pd = (ProgressDialog)dialog;
			pd.setMessage(getString(R.string.nativeClientStarting));
			break;
		case DIALOG_RESTART_PROGRESS:
			pd = (ProgressDialog)dialog;
			pd.setMessage(getString(R.string.clientRestarting));
			break;
		case DIALOG_SHUTDOWN_PROGRESS:
			pd = (ProgressDialog)dialog;
			pd.setMessage(getString(R.string.nativeClientStopping));
			break;
		}
	}
		
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (Logging.DEBUG) Log.d(TAG, "onActivityResult()");
		switch (requestCode) {
		case ACTIVITY_SELECT_HOST:
			if (resultCode == RESULT_OK) {
				// Finished successfully - selected the host to which we should connect
				mSelectedClient = data.getParcelableExtra(ClientId.TAG);
			}
			break;
		case ACTIVITY_MANAGE_CLIENT:
			// In case the ManageClientActivity was invoked when connection was already active
			// and child activity did not disconnect, the mInitialDataAvailable is still true
			// But if ManageClientActivity disconnected current client and connected other one,
			// all tabs cleared old data and did not receive new one yet (because ManageClientActivity
			// does not request initial data due to speed and data volume aspects).
			if ( (mConnectedClient != null) && (!mInitialDataAvailable) ) {
				if (Logging.DEBUG) Log.d(TAG, "connected:"+mConnectedClient+","+mInitialDataAvailable);
				// We are connected to some client right now, but initial data were
				// NOT retrieved yet. We trigger it now...
				retrieveInitialData();
				// This is very early stage, tabs (hopefully) did not request own data yet
			}
			break;
		case ACTIVITY_NATIVE_CLIENT:
			if (data != null && data.getBooleanExtra(NativeClientActivity.RESULT_DATA_RESTARTED, false)) {
				if (Logging.DEBUG) Log.d(TAG, "Native client restarted, do connect");
				mConnectClientAfterRestart = true;
				HostListDbAdapter dbHelper = new HostListDbAdapter(this);
				dbHelper.open();
				mSelectedClient = dbHelper.fetchHost("nativeboinc");
				dbHelper.close();
				showDialog(DIALOG_RESTART_PROGRESS);
				
				if (mRunner != null && mRunner.isRestarted())
					onClientStart();
			}
		default:
			break;
		}
	}


	@Override
	public void clientConnectionProgress(BoincOp boincOp, int progress) {
		switch (progress) {
		case PROGRESS_INITIAL_DATA:
			// We are already connected, so hopefully we can display client ID in title bar
			// as well as progress spinner
			ClientId clientId = mConnectionManager.getClientId();
			if (clientId != null) {
				setTitle(clientId.getNickname());
			}
			// No break here, we drop to next case (dialog update)
		case PROGRESS_CONNECTING:
		case PROGRESS_AUTHORIZATION_PENDING:
			// Update dialog to display corresponding status, i.e.
			// "Connecting", "Authorization", "Retrieving"
			showProgressDialog(progress);
			break;
		case PROGRESS_XFER_STARTED:
			break;
		case PROGRESS_XFER_FINISHED:
			break;
		case PROGRESS_XFER_POLL:
			break;
		default:
			if (Logging.ERROR) Log.e(TAG, "Unhandled progress indicator: " + progress);
		}
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		mConnectedClient = mConnectionManager.getClientId();
		mConnectedClientVersion = clientVersion;
		if (mConnectedClient != null) {
			if (mDoConnectNativeClient) {
				if (!mConnectedClient.isNativeClient() || !mApp.isBoincClientRun()) {
					// do connect to native client
					boincConnect();
				} else {
					// if already connected
					updateTitle();
					mSelectedClient = null;
					mDoConnectNativeClient = false;
				}
			} else {
				// Connected client is retrieved
				if (Logging.DEBUG) Log.d(TAG, "Client " + mConnectedClient.getNickname() + " is connected");
				updateTitle();
				mSelectedClient = null; // For case of auto-connect on startup while service is already connected
			}
			if (Build.VERSION.SDK_INT >= 11)
				invalidateOptionsMenu();
		}
		else {
			// Received connected notification, but client is unknown!
			if (Logging.ERROR) Log.e(TAG, "Client not connected despite notification");
		}
		dismissProgressDialog();
	}

	@Override
	public void clientDisconnected(boolean disconnectedByManager) {
		if (Logging.DEBUG) Log.d(TAG, "Client " +
				( (mConnectedClient != null) ?
						mConnectedClient.getNickname() : "<not connected>" ) +
						" is disconnected");
		mDoConnectNativeClient = false;
		ClientId prevConnectedClient = mConnectedClient;
		mConnectedClient = null;
		mConnectedClientVersion = null;
		mInitialDataAvailable = false;
		updateTitle();
		dismissProgressDialog();
		if (mSelectedClient != null) {
			// Connection to another client is deferred, we proceed with it now
			if (!mConnectClientAfterRestart)
				// connect when not after restart
				boincConnect();
			
		} else {
			StandardDialogs.tryShowDisconnectedErrorDialog(this, mConnectionManager, mRunner,
					prevConnectedClient, disconnectedByManager);
			if (Build.VERSION.SDK_INT >= 11)
				invalidateOptionsMenu();
		}
	}
	

	@Override
	public boolean clientError(BoincOp boincOp, int errorNum, String message) {
		if (!mIsPaused && (!mConnectionManager.isNativeConnected() || !mRunner.ifStoppedByManager())) {
			StandardDialogs.showClientErrorDialog(this, errorNum, message);
			return true;
		}
		return false;
	}
	
	@Override
	public boolean onPollError(int errorNum, int operation, String errorMessage, String param) {
		if (!mIsPaused && (!mConnectionManager.isNativeConnected() || !mRunner.ifStoppedByManager())) {
			StandardDialogs.showPollErrorDialog(this, errorNum, operation, errorMessage, param);
			return true;
		}
		return false;
	}
	
	@Override
	public void onPollCancel(int opFlags) {
		// do nothing
	}
	
	@Override
	public void onClientIsWorking(boolean isWorking) {
		if ((mRunner != null && mRunner.serviceIsWorking()) || isWorking)
			setProgressBarIndeterminateVisibility(true);
		else if ((mRunner == null || !mRunner.serviceIsWorking()) && !isWorking)
			setProgressBarIndeterminateVisibility(false);
	}

	@Override
	public boolean updatedNotices(ArrayList<NoticeInfo> notices) {
		if (mInitialDataRetrievalStarted) {
			dismissProgressDialog();
			mInitialDataRetrievalStarted = false;
			if (Logging.DEBUG) Log.d(TAG, "Explicit initial data retrieval finished");
			mInitialDataAvailable = true;
		}
		return false;
	}

	@Override
	public void onClientStart() {
		if (mConnectClientAfterStart || mConnectClientAfterRestart) {
			if (Logging.DEBUG) Log.d(TAG, "on client start: after start");
			if (mConnectClientAfterRestart) // after restarting
				dismissDialog(DIALOG_RESTART_PROGRESS);
			else // if normal start
				dismissDialog(DIALOG_START_PROGRESS);
			
			mConnectClientAfterStart = false;
			mConnectClientAfterRestart = false;
			
			HostListDbAdapter dbHelper = new HostListDbAdapter(this);
			dbHelper.open();
			mSelectedClient = dbHelper.fetchHost("nativeboinc");
			dbHelper.close();
					
			if (mSelectedClient != null)
				boincConnect();
			
			if (Build.VERSION.SDK_INT >= 11)
				invalidateOptionsMenu();
		}
	}
	
	@Override
	public void onClientStop(int exitCode, boolean stoppedByManager) {
		if (mRunner.ifStoppedByManager()) {
			if (mConnectedClient != null && mConnectedClient.isNativeClient())
				boincDisconnect();
		}
		
		if (mStoppedInMainActivity) {
			mStoppedInMainActivity = false;
			dismissDialog(DIALOG_SHUTDOWN_PROGRESS);
		}
		
		if (Build.VERSION.SDK_INT >= 11)
			invalidateOptionsMenu();
	}
	
	@Override
	public boolean onNativeBoincClientError(String message) {
		if (mShowStartDialog) {
			if (mConnectClientAfterRestart) // after restarting
				dismissDialog(DIALOG_RESTART_PROGRESS);
			else // if normal start
				dismissDialog(DIALOG_START_PROGRESS);
		}
		
		mStoppedInMainActivity = false;
		mConnectClientAfterStart = false;
		mConnectClientAfterRestart = false;
		
		if (Build.VERSION.SDK_INT >= 11)
			invalidateOptionsMenu();
		
		if (!mIsPaused) {
			StandardDialogs.showErrorDialog(this, message);
			return true;
		}
		return false;
	}
	
	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		if ((mConnectionManager != null && mConnectionManager.isWorking()) || isWorking)
			setProgressBarIndeterminateVisibility(true);
		else if ((mConnectionManager == null || !mConnectionManager.isWorking()) && !isWorking)
			setProgressBarIndeterminateVisibility(false);
	}
	
	private void updateTitle() {
		if (mConnectedClient != null) {
			// We are connected to host - update title to host nickname
			if (Logging.DEBUG) Log.d(TAG, "Host nickname: " + mConnectedClient.getNickname());
			mSb.setLength(0);
			mSb.append(mConnectedClient.getNickname());
			if (mConnectedClientVersion != null) {
				mSb.append(" (");
				mSb.append(mConnectedClientVersion.version);
				mSb.append(")");
			}
			setTitle(mSb.toString());
		}
		else {
			// We are not connected - set title to indicate that
			setTitle(getString(R.string.notConnected));
		}
	}

	private void showProgressDialog(final int progress) {
		if (mProgressDialogAllowed) {
			mConnectProgressIndicator = progress;
			showDialog(DIALOG_CONNECT_PROGRESS);
		}
		else if (mConnectProgressIndicator != -1) {
			// Not allowed to show progress dialog (i.e. activity restarting/terminating),
			// but we are showing previous progress dialog - dismiss it
			removeDialog(DIALOG_CONNECT_PROGRESS);
			mConnectProgressIndicator = -1;
		}
	}

	private void dismissProgressDialog() {
		if (mConnectProgressIndicator != -1) {
			removeDialog(DIALOG_CONNECT_PROGRESS);
			mConnectProgressIndicator = -1;
		}
	}

	private void boincConnect() {
		try {
			if (mConnectionManager == null) {
				doBindService();
				return;
			}
			if (mSelectedClient == null) {
				if (Logging.WARNING) Log.w(TAG, "boinc connect selected client is numm");
				return;
			}
			if (Logging.DEBUG) Log.d(TAG, "boinc connect");
			mConnectionManager.connect(mSelectedClient, true);
			mSelectedClient = null;
			mInitialDataAvailable = true; // Not really, but data will be available on connected notification
		}
		catch (NoConnectivityException e) {
			if (Logging.DEBUG) Log.d(TAG, "No connectivity - cannot connect");
			// TODO: Show notification about connectivity
		}
	}

	private void boincDisconnect() {
		if (mConnectionManager != null)
			mConnectionManager.disconnect();
	}

	private void connectOrReconnect() {
		if (mConnectedClient == null) {
			// No client connected now, we can safely connect to new one
			boincConnect();
		}
		else {
			// We are currently connected and some client was selected to connect
			// We must check whether it is not the same
			if (mSelectedClient.equals(mConnectedClient)) {
				// The same client was selected, as the one already connected
				// We will not change connection - reset mSelectedClient
				if (Logging.DEBUG) Log.d(TAG, "Selected the same client as already connected: " +
						mSelectedClient.getNickname() + ", keeping existing connection");
				mSelectedClient = null;
			}
			else {
				/*if (Logging.DEBUG) Log.d(TAG, "Selected new client: " +
						mSelectedClient.getNickname() +
						", while already connected to: " + mConnectedClient.getNickname() +
						", disconnecting it first");*/
				boincDisconnect();
				// The boincConnect() will be triggered after the clientDisconnected() notification
			}
		}
	}

	private void retrieveInitialData() {
		if (Logging.DEBUG) Log.d(TAG, "Explicit initial data retrieval starting");
		mConnectionManager.updateTasks(); // will get whole state
		mConnectionManager.updateTransfers();
		mConnectionManager.updateMessages();
		mConnectionManager.updateNotices();
		mInitialDataRetrievalStarted = true;
	}
}