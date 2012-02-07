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

import sk.boinc.nativeboinc.clientconnection.ClientReplyReceiver;
import sk.boinc.nativeboinc.clientconnection.HostInfo;
import sk.boinc.nativeboinc.clientconnection.MessageInfo;
import sk.boinc.nativeboinc.clientconnection.ModeInfo;
import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.clientconnection.ProjectInfo;
import sk.boinc.nativeboinc.clientconnection.TaskInfo;
import sk.boinc.nativeboinc.clientconnection.TransferInfo;
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
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
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


public class BoincManagerActivity extends TabActivity implements ClientReplyReceiver,
		NativeBoincStateListener {
	private static final String TAG = "BoincManagerActivity";

	private static final int DIALOG_CONNECT_PROGRESS = 1;
	private static final int DIALOG_NETWORK_DOWN     = 2;
	private static final int DIALOG_ABOUT            = 3;
	private static final int DIALOG_LICENSE          = 4;
	private static final int DIALOG_UPGRADE_INFO        = 5;

	private static final int ACTIVITY_SELECT_HOST   = 1;
	private static final int ACTIVITY_MANAGE_CLIENT = 2;

	private static final int BACK_PRESS_PERIOD = 5;

	private BoincManagerApplication mApp;
	private ScreenOrientationHandler mScreenOrientation;
	private WakeLock mWakeLock;
	private boolean mScreenAlwaysOn = false;
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

	private boolean mClientStartFromMenu = false;
	private boolean mClientShutdownFromMenu = false;
	
	private class SavedState {
		private final boolean initialDataAvailable;
		private final int connectProgressIndicator;

		public SavedState() {
			initialDataAvailable = mInitialDataAvailable;
			if (Logging.DEBUG) Log.d(TAG, "saved: initialDataAvailable=" + initialDataAvailable);
			connectProgressIndicator = mConnectProgressIndicator;
			if (Logging.DEBUG) Log.d(TAG, "saved: connectProgressIndicator=" + connectProgressIndicator);
		}
		public void restoreState(BoincManagerActivity activity) {
			activity.mInitialDataAvailable = initialDataAvailable;
			if (Logging.DEBUG) Log.d(TAG, "restored: mInitialDataAvailable=" + activity.mInitialDataAvailable);
			activity.mConnectProgressIndicator = connectProgressIndicator;
			if (Logging.DEBUG) Log.d(TAG, "restored: mConnectProgressIndicator=" +
					activity.mConnectProgressIndicator);
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
		}
	};
	
	private InstallerService mInstaller = null;
	
	private ServiceConnection mInstallerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mInstaller = ((InstallerService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "installer.onServiceConnected()");
			
			HostListDbAdapter dbHelper = new HostListDbAdapter(BoincManagerActivity.this);
			dbHelper.open();
			// check nativeboinc entry
			ClientId clientId = dbHelper.fetchHost("nativeboinc");
			dbHelper.close();
			
			if (mApp.isInstallerRun()) {
				// open again installer activity
				mApp.runInstallerActivity(BoincManagerActivity.this);
			} else if ((!mInstaller.isClientInstalled() || clientId == null)) {
				// run installation wizard
				startActivity(new Intent(BoincManagerActivity.this,
						InstallStep1Activity.class));
			}
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mInstaller = null;
			if (Logging.DEBUG) Log.d(TAG, "installer.onServiceDisconnected()");
		}
	};
	
	private NativeBoincService mRunner = null;
	
	private ServiceConnection mRunnerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceConnected()");
			mRunner.addNativeBoincListener(BoincManagerActivity.this);
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
	
	private void doBindInstallerService() {
		if (Logging.DEBUG) Log.d(TAG, "doBindInstallerService()");
		bindService(new Intent(BoincManagerActivity.this, InstallerService.class),
				mInstallerServiceConnection, Context.BIND_AUTO_CREATE);
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
	
	private void doUnbindInstallerService() {
		if (Logging.DEBUG) Log.d(TAG, "doUnbindInstallerService()");
		unbindService(mInstallerServiceConnection);
		mInstaller = null;
	}
	
	private void doUnbindRunnerService() {
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

		// Obtain screen wake-lock
		PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
		mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, BoincManagerApplication.GLOBAL_ID);

		doBindService();
		doBindInstallerService();
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

		// Set all tabs one by one, to start all activities now
		// It is better to receive early updates of data
		tabHost.setCurrentTabByTag("tab_messages");
		tabHost.setCurrentTabByTag("tab_tasks");
		tabHost.setCurrentTabByTag("tab_transfers");
		tabHost.setCurrentTabByTag("tab_projects");
		// Set saved tab (the last selected on previous run) as current
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		int lastActiveTab = globalPrefs.getInt(PreferenceName.LAST_ACTIVE_TAB, 1);
		tabHost.setCurrentTab(lastActiveTab);

		// Restore state on configuration change (if applicable)
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null) {
			// Yes, we have the saved state, this is activity re-creation after configuration change
			savedState.restoreState(this);
		}
		else {
			// Just normal start
			String autoConnectHost = globalPrefs.getString(PreferenceName.AUTO_CONNECT_HOST, null);
			if (((autoConnectHost != null) && globalPrefs.getBoolean(PreferenceName.AUTO_CONNECT, false)) ||
					(autoConnectHost == null)) {
				if (autoConnectHost == null)	// use nativeboinc as autoconnect
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
	}

	@Override
	protected void onResume() {
		super.onResume();
		if (Logging.DEBUG) Log.d(TAG, "onResume()");
		mBackPressedRecently = false;
		mScreenOrientation.setOrientation();
		// We are either starting up or returning from sub-activity, which
		// could be the AppPreferencesActivity - we must check the wake-lock now
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		boolean screenAlwaysOn = globalPrefs.getBoolean(PreferenceName.LOCK_SCREEN_ON, false);
		if (screenAlwaysOn != mScreenAlwaysOn) {
			// The setting is different than the old one - let's change the lock
			mScreenAlwaysOn = screenAlwaysOn;
			if (mScreenAlwaysOn) {
				mWakeLock.acquire();
				if (Logging.DEBUG) Log.d(TAG, "Acquired screen lock");
			}
			else {
				mWakeLock.release();
				if (Logging.DEBUG) Log.d(TAG, "Released screen lock");
			}
		}
		// Update name of connected client (or show "not connected")
		updateTitle();
		// Show Information about upgrade, if applicable
		if (mJustUpgraded) {
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
		
		if (mInstaller == null) {
			if (Logging.DEBUG) Log.d(TAG, "onResume() - installer not present");
			doBindInstallerService();
		}
		if (mRunner == null) {
			if (Logging.DEBUG) Log.d(TAG, "onResume() - runner not present");
			doBindRunnerService();
		} else {
			if (Logging.DEBUG) Log.d(TAG, "reregister runner listener");
			mRunner.addNativeBoincListener(this);
		}
	}

	@Override
	protected void onPause() {
		super.onPause();
		if (Logging.DEBUG) Log.d(TAG, "onPause()");
		mProgressDialogAllowed = false;
		// If we did not perform deferred connect so far, we needn't do that anymore
		mSelectedClient = null;
		if (Logging.DEBUG) Log.d(TAG, "unregister runner listener");
		if (mRunner != null)
			mRunner.removeNativeBoincListener(this);
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
		if (mInstaller != null) {
			//mInstaller.removeInstallerListener(this);
			mInstaller = null;
		}
		if (mRunner != null) {
			mRunner.removeNativeBoincListener(this);
			mRunner = null;
		}
		doUnbindService();
		doUnbindInstallerService();
		doUnbindRunnerService();
		if (mWakeLock.isHeld()) {
			// We locked the screen previously - release it, as we are closing now
			mWakeLock.release();
			if (Logging.DEBUG) Log.d(TAG, "Released screen lock");
		}
		mWakeLock = null;
		mScreenOrientation = null;
	}

	@Override
	public Object onRetainNonConfigurationInstance() {
		final SavedState savedState = new SavedState();
		return savedState;
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
		item.setVisible(!mRunner.isRun());
		item = menu.findItem(R.id.menuShutdown);
		item.setVisible(mRunner.isRun());
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.menuStartUp:
			mClientStartFromMenu = true;
			mRunner.startClient(false);
			return true;
		case R.id.menuShutdown:
			new AlertDialog.Builder(this)
	    		.setIcon(android.R.drawable.ic_dialog_alert)
	    		.setTitle(R.string.warning)
	    		.setMessage(R.string.shutdownAskText)
	    		.setPositiveButton(R.string.shutdown,
	    			new DialogInterface.OnClickListener() {
	    				public void onClick(DialogInterface dialog, int whichButton) {
	    					mClientShutdownFromMenu = true;
	    					mRunner.shutdownClient();
	    				}
	    			})
	    		.setNegativeButton(R.string.cancel, null)
	    		.create().show();
			return true;
		case R.id.menuManage:
			// Launch new activity
			startActivityForResult(new Intent(this, ManageClientActivity.class), ACTIVITY_MANAGE_CLIENT);
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
	protected Dialog onCreateDialog(int id) {
		View v;
		TextView text;
		switch (id) {
		case DIALOG_CONNECT_PROGRESS:
			if (Logging.DEBUG) Log.d(TAG, "onCreateDialog(DIALOG_CONNECT_PROGRESS)");
			ProgressDialog dialog = new FixedProgressDialog(this);
            dialog.setIndeterminate(true);
			dialog.setCancelable(true);
			dialog.setOnCancelListener(new OnCancelListener() {
				@Override
				public void onCancel(DialogInterface dialog) {
					// Connecting canceled
					mConnectProgressIndicator = -1;
					// Disconnect the client
					boincDisconnect();
				}
			});
            return dialog;
		case DIALOG_NETWORK_DOWN:
			v = LayoutInflater.from(this).inflate(R.layout.dialog, null);
			text = (TextView)v.findViewById(R.id.dialogText);
			text.setText(R.string.networkUnavailable);
        	return new AlertDialog.Builder(this)
        		.setIcon(android.R.drawable.ic_dialog_alert)
        		.setTitle(R.string.error)
        		.setView(v)
        		.setNegativeButton(R.string.close, null)
        		.create();
		case DIALOG_ABOUT:
			v = LayoutInflater.from(this).inflate(R.layout.dialog, null);
			text = (TextView)v.findViewById(R.id.dialogText);
			mApp.setAboutText(text);
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_info)
				.setTitle(R.string.aboutTitle)
				.setView(v)
				.setPositiveButton(R.string.homepage,
					new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int whichButton) {
							Uri uri = Uri.parse(getString(R.string.aboutHomepageUrl));
							startActivity(new Intent(Intent.ACTION_VIEW, uri));
						}
					})
				.setNeutralButton(R.string.license,
					new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int whichButton) {
							showDialog(DIALOG_LICENSE);
						}
					})
        		.setNegativeButton(R.string.dismiss, null)
        		.create();
		case DIALOG_LICENSE:
			v = LayoutInflater.from(this).inflate(R.layout.dialog, null);
			text = (TextView)v.findViewById(R.id.dialogText);
			mApp.setLicenseText(text);
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_info)
				.setTitle(R.string.license)
				.setView(v)
				.setPositiveButton(R.string.sources, 
					new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int whichButton) {
							Uri uri = Uri.parse(getString(R.string.aboutHomepageUrl));
							startActivity(new Intent(Intent.ACTION_VIEW, uri));
						}
					})
        		.setNegativeButton(R.string.dismiss, null)
        		.create();
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
		}
		return null;
	}

	@Override
	protected void onPrepareDialog(int id, Dialog dialog) {
		switch (id) {
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
				// We are connected to some client right now, but initial data were
				// NOT retrieved yet. We trigger it now...
				retrieveInitialData();
				// This is very early stage, tabs (hopefully) did not request own data yet
			}
			break;
		default:
			break;
		}
	}


	@Override
	public void clientConnectionProgress(int progress) {
		switch (progress) {
		case PROGRESS_INITIAL_DATA:
			// We are already connected, so hopefully we can display client ID in title bar
			// as well as progress spinner
			ClientId clientId = mConnectionManager.getClientId();
			if (clientId != null) {
				setTitle(clientId.getNickname());
			}
			setProgressBarIndeterminateVisibility(true);
			// No break here, we drop to next case (dialog update)
		case PROGRESS_CONNECTING:
		case PROGRESS_AUTHORIZATION_PENDING:
			// Update dialog to display corresponding status, i.e.
			// "Connecting", "Authorization", "Retrieving"
			showProgressDialog(progress);
			break;
		case PROGRESS_XFER_STARTED:
			setProgressBarIndeterminateVisibility(true);
			break;
		case PROGRESS_XFER_FINISHED:
			setProgressBarIndeterminateVisibility(false);
			break;
		default:
			if (Logging.ERROR) Log.e(TAG, "Unhandled progress indicator: " + progress);
		}
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		setProgressBarIndeterminateVisibility(false);
		mConnectedClient = mConnectionManager.getClientId();
		mConnectedClientVersion = clientVersion;
		if (mConnectedClient != null) {
			// Connected client is retrieved
			if (Logging.DEBUG) Log.d(TAG, "Client " + mConnectedClient.getNickname() + " is connected");
			updateTitle();
			mSelectedClient = null; // For case of auto-connect on startup while service is already connected
		}
		else {
			// Received connected notification, but client is unknown!
			if (Logging.ERROR) Log.e(TAG, "Client not connected despite notification");
		}
		dismissProgressDialog();
	}

	@Override
	public void clientDisconnected() {
		if (Logging.DEBUG) Log.d(TAG, "Client " +
				( (mConnectedClient != null) ?
						mConnectedClient.getNickname() : "<not connected>" ) +
						" is disconnected");
		mConnectedClient = null;
		mConnectedClientVersion = null;
		mInitialDataAvailable = false;
		updateTitle();
		setProgressBarIndeterminateVisibility(false);
		dismissProgressDialog();
		if (mSelectedClient != null) {
			// Connection to another client is deferred, we proceed with it now
			boincConnect();
		}
	}
	
	@Override
	public boolean updatedClientMode(ModeInfo modeInfo) {
		// TODO: Handle client mode
		// If run mode is suspended, show notification about it (pause symbol?)
		// In such case also request periodic updates of status
		return false;
	}

	@Override
	public boolean updatedHostInfo(HostInfo hostInfo) {
		// Never requested, nothing to do
		return false;
	}
	
	@Override
	public boolean updatedProjects(ArrayList<ProjectInfo> projects) {
		// Never requested, nothing to do
		return false;
	}

	@Override
	public boolean updatedTasks(ArrayList<TaskInfo> tasks) {
		// Never requested, nothing to do
		return false;
	}

	@Override
	public boolean updatedTransfers(ArrayList<TransferInfo> transfers) {
		// Never requested, nothing to do
		return false;
	}

	@Override
	public boolean updatedMessages(ArrayList<MessageInfo> messages) {
		if (mInitialDataRetrievalStarted) {
			dismissProgressDialog();
			mInitialDataRetrievalStarted = false;
			if (Logging.DEBUG) Log.d(TAG, "Explicit initial data retrieval finished");
			mInitialDataAvailable = true;
		}
		return false;
	}


	public void onClientStateChanged(boolean isRun) {
		if (isRun) {
			if (mClientStartFromMenu) {
				mClientStartFromMenu = false;
				HostListDbAdapter dbHelper = new HostListDbAdapter(this);
				dbHelper.open();
				mSelectedClient = dbHelper.fetchHost("nativeboinc");
				dbHelper.close();
				if (mSelectedClient != null)
					boincConnect();
			}
		} else {
			if (mClientShutdownFromMenu) {
				mClientShutdownFromMenu = false;
				if (mConnectedClient != null && 
						(mConnectedClient.getAddress().equals("127.0.0.1") ||
								mConnectedClient.getAddress().equals("localhost")))
					boincDisconnect();
			}
		}
	}
	
	@Override
	public void onNativeBoincError(String message) {
		mClientStartFromMenu = false;
		mClientShutdownFromMenu = false;
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
				if (Logging.DEBUG) Log.d(TAG, "Selected new client: " +
						mSelectedClient.getNickname() +
						", while already connected to: " + mConnectedClient.getNickname() +
						", disconnecting it first");
				boincDisconnect();
				// The boincConnect() will be triggered after the clientDisconnected() notification
			}
		}
	}

	private void retrieveInitialData() {
		if (Logging.DEBUG) Log.d(TAG, "Explicit initial data retrieval starting");
		mConnectionManager.updateTasks(null); // will get whole state
		mConnectionManager.updateTransfers(null);
		mConnectionManager.updateMessages(null);
		mInitialDataRetrievalStarted = true;
	}

	@Override
	public void clientError(int err_num, String message) {
		// TODO Auto-generated method stub
		
	}
}