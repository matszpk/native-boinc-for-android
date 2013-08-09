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

import edu.berkeley.boinc.lite.AccountMgrInfo;

import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.ClientAccountMgrReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientManageReceiver;
import sk.boinc.nativeboinc.clientconnection.HostInfo;
import sk.boinc.nativeboinc.clientconnection.ModeInfo;
import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.clientconnection.PollOp;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.service.ConnectionManagerService;
import sk.boinc.nativeboinc.util.BAMAccount;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.ScreenOrientationHandler;
import sk.boinc.nativeboinc.util.StandardDialogs;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.DialogInterface.OnCancelListener;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.text.Html;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.Window;
import android.widget.TextView;
import android.widget.Toast;


public class ManageClientActivity extends PreferenceActivity implements ClientManageReceiver,
		ClientAccountMgrReceiver {
	private static final String TAG = "ManageClientActivity";

	private static final int DIALOG_WARN_SHUTDOWN = 0;
	private static final int DIALOG_RETRIEVAL_PROGRESS = 1;
	private static final int DIALOG_HOST_INFO = 2;
	private static final int DIALOG_ATTACH_BAM_PROGRESS = 3;

	private static final int ACTIVITY_SELECT_HOST = 1;
	
	private int mConnectProgressIndicator = -1;
	private boolean mProgressDialogAllowed = false;
	private ModeInfo mClientMode = null;
	private boolean mDoUpdateHostInfo = false;
	private HostInfo mHostInfo = null;
	
	private boolean mPeriodicModeRetrievalAllowed = false;
	
	private ConnectionManagerService mConnectionManager = null;
	private boolean mDelayedObserverRegistration = false;
	private ClientId mConnectedClient = null;
	private ClientId mSelectedClient = null;
	
	private boolean mDoGetBAMInfo = false;
	private AccountMgrInfo mBAMInfo = null;
	private boolean mSyncingBAMInProgress = false;
	
	private boolean mShowShutdownDialog = false;
	
	private ScreenOrientationHandler mScreenOrientation;
	
	private static class SavedState {
		private final boolean doUpdateHostInfo;
		private final HostInfo hostInfo;
		private final boolean doGetBAMInfo;
		private final AccountMgrInfo bamInfo;
		private final boolean syncingBAMInProgress;
		private final boolean showShutdownDialog;
		
		public SavedState(ManageClientActivity activity) {
			doUpdateHostInfo = activity.mDoUpdateHostInfo;
			hostInfo = activity.mHostInfo;
			doGetBAMInfo = activity.mDoGetBAMInfo;
			bamInfo = activity.mBAMInfo;
			syncingBAMInProgress = activity.mSyncingBAMInProgress;
			showShutdownDialog = activity.mShowShutdownDialog;
			
		}
		public void restoreState(ManageClientActivity activity) {
			activity.mDoUpdateHostInfo = doUpdateHostInfo;
			activity.mHostInfo = hostInfo;
			if (Logging.DEBUG) Log.d(TAG, "restored: mHostInfo=" + activity.mHostInfo);
			activity.mDoGetBAMInfo = doGetBAMInfo;
			activity.mBAMInfo = bamInfo;
			activity.mSyncingBAMInProgress = syncingBAMInProgress;
			activity.mShowShutdownDialog = showShutdownDialog;
		}
	}

	private ServiceConnection mServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mConnectionManager = ((ConnectionManagerService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "onServiceConnected()");
			if (mDelayedObserverRegistration) {
				mConnectionManager.registerStatusObserver(ManageClientActivity.this);
				mDelayedObserverRegistration = false;
			}
			if (mSelectedClient != null) {
				// Some client was selected at the time when service was not bound yet
				// Now the service is available, so connection can proceed
				connectOrReconnect();
			}
			
			updateActivityState();
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (mConnectionManager != null)
				mConnectionManager.unregisterStatusObserver(ManageClientActivity.this);
			mConnectionManager = null;
			// This should not happen normally, because it's local service 
			// running in the same process...
			if (Logging.WARNING) Log.w(TAG, "onServiceDisconnected()");
			// We also reset client reference to prevent mess
			mConnectedClient = null;
			mSelectedClient = null;
			if (mSyncingBAMInProgress)
				StandardDialogs.dismissDialog(ManageClientActivity.this, DIALOG_ATTACH_BAM_PROGRESS);
			mDoGetBAMInfo = false;
			mSyncingBAMInProgress = false;
			setProgressBarIndeterminateVisibility(false);
			// update particular prefs
			updateParticularPreferences();
		}
	};
	
	private void doBindService() {
		if (Logging.DEBUG) Log.d(TAG, "doBindService()");
		bindService(new Intent(ManageClientActivity.this, ConnectionManagerService.class),
				mServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindService() {
		if (Logging.DEBUG) Log.d(TAG, "doUnbindService()");
		unbindService(mServiceConnection);
		mConnectionManager = null;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// The super-class PreferenceActivity calls setContentView()
		// in its onCreate(). So we must request window features here.
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);

		super.onCreate(savedInstanceState);

		mScreenOrientation = new ScreenOrientationHandler(this);

		doBindService();
		// Initializes the preference activity.
		addPreferencesFromResource(R.xml.manage_client);

		Preference pref;
		ListPreference listPref;

		// Currently selected client
		pref = findPreference("selectedHost");
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			public boolean onPreferenceClick(Preference preference) {
				// We use other activity for selection of the host
				startActivityForResult(new Intent(ManageClientActivity.this,
						HostListActivity.class), ACTIVITY_SELECT_HOST);
				return true;
			}
		});
		
		// Synchronize with BAM
		pref = findPreference("synchronizeWithBAM");
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			public boolean onPreferenceClick(Preference preference) {
				mConnectionManager.getBAMInfo();
				mDoGetBAMInfo = true;
				disableBAMPreferences();
				return true;
			}
		});
		
		// Stop using BAM
		pref = findPreference("stopUsingBAM");
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			public boolean onPreferenceClick(Preference preference) {
				boincStopUsingBAM();
				return true;
			}
		});
		
		// Add project
		pref = findPreference("addProject");
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			public boolean onPreferenceClick(Preference preference) {
				startActivity(new Intent(ManageClientActivity.this, ProjectListActivity.class));
				return true;
			}
		});
		
		// Local preferences
		pref = findPreference("localPreferences");
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			public boolean onPreferenceClick(Preference preference) {
				startActivity(new Intent(ManageClientActivity.this, LocalPreferencesActivity.class));
				return true;
			}
		});
		
		// Global preferences
		pref = findPreference("globalPreferences");
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			public boolean onPreferenceClick(Preference preference) {
				boincClearLocalPrefs();
				return true;
			}
		});

		// Run mode
		listPref = (ListPreference)findPreference("actRunMode");
		listPref.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
			public boolean onPreferenceChange(Preference preference, Object newValue) {
				ListPreference listPref = (ListPreference)preference;
				CharSequence[] actRunDesc = listPref.getEntries();
				int idx = listPref.findIndexOfValue((String)newValue);
				listPref.setSummary(actRunDesc[idx]);
				boincChangeRunMode(Integer.parseInt((String)newValue));
				return true;
			}
		});

		// Network mode
		listPref = (ListPreference)findPreference("actNetworkMode");
		listPref.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
			public boolean onPreferenceChange(Preference preference, Object newValue) {
				ListPreference listPref = (ListPreference)preference;
				CharSequence[] actNetworkDesc = listPref.getEntries();
				int idx = listPref.findIndexOfValue((String)newValue);
				listPref.setSummary(actNetworkDesc[idx]);
				boincChangeNetworkMode(Integer.parseInt((String)newValue));
				return true;
			}
		});
		
		// GPU mode
		listPref = (ListPreference)findPreference("actGpuMode");
		listPref.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
			public boolean onPreferenceChange(Preference preference, Object newValue) {
				ListPreference listPref = (ListPreference)preference;
				CharSequence[] actGpuDesc = listPref.getEntries();
				int idx = listPref.findIndexOfValue((String)newValue);
				listPref.setSummary(actGpuDesc[idx]);
				boincChangeGpuMode(Integer.parseInt((String)newValue));
				return true;
			}
		});

		// proxy settings
		pref = findPreference("proxySettings");
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			public boolean onPreferenceClick(Preference preference) {
				startActivity(new Intent(ManageClientActivity.this, ProxySettingsActivity.class));
				return true;
			}
		});
		
		// Run CPU benchmarks
		pref = findPreference("runBenchmark");
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			public boolean onPreferenceClick(Preference preference) {
				boincRunCpuBenchmarks();
				return true;
			}
		});

		// Do network communications
		pref = findPreference("doNetworkCommunication");
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			public boolean onPreferenceClick(Preference preference) {
				boincDoNetworkCommunication();
				return true;
			}
		});

		// Shut down connected client
		pref = findPreference("shutDownClient");
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			public boolean onPreferenceClick(Preference preference) {
				mShowShutdownDialog = true;
				Bundle args = new Bundle(); 
				args.putBoolean("IsNative", mConnectionManager.isNativeConnected());
				showDialog(DIALOG_WARN_SHUTDOWN, args);
				return true;
			}
		});
		
		// Restore state on configuration change (if applicable)
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null) {
			// Yes, we have the saved state, this is activity re-creation after configuration change
			savedState.restoreState(this);
		}
	}
	
	private void updateActivityState() {
		if (mConnectionManager != null && mConnectionManager.isWorking())
			setProgressBarIndeterminateVisibility(true);
		else
			setProgressBarIndeterminateVisibility(false);
		
		if (mConnectionManager == null)
			return;
		
		boolean isError = mConnectionManager.handlePendingClientErrors(null, this);
		// get error for account mgr operations 
		isError |= mConnectionManager.handlePendingPollErrors(null, this);
		if (mConnectedClient == null) {
			clientDisconnected(mConnectionManager.isDisconnectedByManager()); // if disconnected
			isError = true;
		}
		
		if (isError) return;
		
		// check pending of account mgr
		if (mDoGetBAMInfo) {
			AccountMgrInfo bamInfo = (AccountMgrInfo)mConnectionManager.getPendingOutput(BoincOp.GetBAMInfo);
			if (bamInfo != null)
				currentBAMInfo(bamInfo);
		}
		
		if (mDoUpdateHostInfo) {
			HostInfo hostInfo = (HostInfo)mConnectionManager.getPendingOutput(BoincOp.UpdateHostInfo);
			if (hostInfo != null)
				updatedHostInfo(hostInfo);
			else // otherwise show dialog
				showProgressDialog(PROGRESS_INITIAL_DATA);
		}
		
		if (mSyncingBAMInProgress) {
			if (!mConnectionManager.isOpBeingExecuted(BoincOp.SyncWithBAM))
				onAfterAccountMgrRPC();
		}
		
		// update particular prefs
		updateParticularPreferences();
	}

	@Override
	protected void onResume() {
		super.onResume();
		if (Logging.DEBUG) Log.d(TAG, "Resume");
		// setup orientation
		mScreenOrientation.setOrientation();
		
		// We are in foreground, so we want to receive notification when data are updated
		if (mConnectionManager != null) {
			// register right now
			mConnectionManager.registerStatusObserver(ManageClientActivity.this);
			// Check, whether we are connected now or not
			mConnectedClient = mConnectionManager.getClientId();
		}
		else {
			// During creation of activity, we'll receive onServiceConnected() callback afterwards
			mDelayedObserverRegistration = true;
			// We force the "No host connected" display for now (can change as soon as we register as observer)
			mConnectedClient = null;
		}
		// Display currently connected host (or "No host connected")
		refreshClientName();
		
		// Progress dialog is allowed since now
		mProgressDialogAllowed = true;

		if (mSelectedClient != null) {
			// We just returned from activity which selected client to connect to
			if (mConnectionManager != null) {
				// Service is bound, we can use it
				connectOrReconnect();
			}
			else {
				// Service not bound at the moment (too slow start? or disconnected itself?)
				if (Logging.INFO)Log.i(TAG,
					"onResume() - Client selected, but service not yet available => binding again");
				doBindService();
			}
		}
		else {
			// Connection to another client is NOT to be started
			// If we are connected, we should also display current run/network mode
			if (mConnectedClient != null) {
				// We are connected - retrieve the current mode
				// Maybe the old one we have is correct, but we are not sure - so we disable it for now
				// The values are still visible, but they are grayed out
				refreshClientModePending();
				mConnectionManager.updateClientMode();
			}
			else {
				// We are not connected - update display accordingly
				mClientMode = null;
				refreshClientMode();
			}
		}
		
		updateActivityState();
	}

	@Override
	protected void onPause() {
		super.onPause();
		if (Logging.DEBUG) Log.d(TAG, "Pause");
		// No more repeated displays
		mPeriodicModeRetrievalAllowed = false;
		mProgressDialogAllowed = false;
		dismissProgressDialogs();
		// Do not receive notifications about state and data availability, as we are not front activity now
		// We will change that when we resume again
		if (mConnectionManager != null) {
			mConnectionManager.unregisterStatusObserver(this);
			mConnectedClient = null; // will be again retrieved in onResume();
		}
		// In case of servicce binding unfinished (i.e. this activity is created and 
		// service bind is triggered, but callback onServiceConnected() not received yet),
		// we will NOT register observer at the time of onServiceConnected(), as we are in
		// background now and we do not want to observe connection
		mDelayedObserverRegistration = false;
		// If we did not perform deferred connect so far, we needn't do that anymore
		mSelectedClient = null;
	}

	@Override
	protected void onDestroy() {
		if (Logging.DEBUG) Log.d(TAG, "onDestroy()");
		mScreenOrientation = null;
		doUnbindService();
		super.onDestroy();
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	@Override
	public void onBackPressed() {
		if (mConnectionManager != null)
			mConnectionManager.cancelPollOperations(PollOp.POLL_BAM_OPERATION_MASK);
		finish();
	}

	@Override
	protected Dialog onCreateDialog(int dialogId, Bundle args) {
		Dialog dialog = StandardDialogs.onCreateDialog(this, dialogId, args);
		if (dialog != null)
			return dialog;
		
		ProgressDialog progressDialog;
		switch (dialogId) {
		case DIALOG_WARN_SHUTDOWN: {
			if (mShowShutdownDialog &&  mConnectionManager != null && mConnectedClient == null) {
				mShowShutdownDialog = false;
				return null; // do not create
			}
			
			int messageId = R.string.warnShutdownText;
			if (args.getBoolean("IsNative")) // if native
				messageId = R.string.shutdownAskText;
				
        	return new AlertDialog.Builder(this)
        		.setIcon(android.R.drawable.ic_dialog_alert)
        		.setTitle(R.string.warning)
        		.setMessage(messageId)
        		.setPositiveButton(R.string.shutdown,
        			new DialogInterface.OnClickListener() {
        				public void onClick(DialogInterface dialog, int whichButton) {
        					mShowShutdownDialog = false;
        					boincShutdownClient();
        				}
        			})
        		.setNegativeButton(R.string.cancel,
    				new DialogInterface.OnClickListener() {
						@Override
						public void onClick(DialogInterface dialog, int which) {
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
		}
		case DIALOG_RETRIEVAL_PROGRESS:
			if ( (mConnectProgressIndicator == -1) || !mProgressDialogAllowed ) {
				return null;
			}
			progressDialog = new FixedProgressDialog(this);
			progressDialog.setIndeterminate(true);
			progressDialog.setCancelable(true);
			progressDialog.setOnCancelListener(new OnCancelListener() {
				@Override
				public void onCancel(DialogInterface dialog) {
					// Connecting canceled
					mConnectProgressIndicator = -1;
					// Disconnect & finish the thread
					boincDisconnect();
				}
			});
			return progressDialog;
		case DIALOG_ATTACH_BAM_PROGRESS:
			progressDialog = new FixedProgressDialog(this);
			progressDialog.setIndeterminate(true);
			progressDialog.setCancelable(true);
			return progressDialog;
		case DIALOG_HOST_INFO:
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_info)
				.setTitle(R.string.menuHostInfo)
				.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
				.setNegativeButton(R.string.ok, null)
				.setOnCancelListener(new OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						// We don't need data anymore - allow them to be garbage collected
						mHostInfo = null;
					}
				})
				.create();
		}
		return null;
	}

	@Override
	protected void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		if (StandardDialogs.onPrepareDialog(this, dialogId, dialog, args))
			return;
		
		switch (dialogId) {
		case DIALOG_RETRIEVAL_PROGRESS: {
			ProgressDialog pd = (ProgressDialog)dialog;
			switch (mConnectProgressIndicator) {
			case PROGRESS_CONNECTING:
				pd.setMessage(getString(R.string.connecting));
				break;
			case PROGRESS_AUTHORIZATION_PENDING:
				pd.setMessage(getString(R.string.authorization));
				break;
			case PROGRESS_INITIAL_DATA:
				pd.setMessage(getString(R.string.retrievingData));				
				break;
			default:
				if (Logging.ERROR) Log.e(TAG, "Unhandled progress indicator: " + mConnectProgressIndicator);
			}
			break;
		}
		case DIALOG_ATTACH_BAM_PROGRESS: {
			ProgressDialog pd = (ProgressDialog)dialog;
			pd.setMessage(getString(R.string.synchronizingBAM));
			break;
		}
		case DIALOG_HOST_INFO:
			TextView text = (TextView)dialog.findViewById(R.id.dialogText);
			if (mHostInfo.htmlText != null)
				text.setText(Html.fromHtml(mHostInfo.htmlText));
			break;
		}
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		super.onCreateOptionsMenu(menu);
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.manage_client_menu, menu);
		return true;
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		super.onPrepareOptionsMenu(menu);
		return (mConnectedClient != null);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.menuHostInfo:
			// Request fresh host-info from client in any case
			showProgressDialog(PROGRESS_INITIAL_DATA);
			mDoUpdateHostInfo = true;
			mConnectionManager.updateHostInfo();
			break;
		case R.id.menuDisconnect:
			// Disconnect from currently connected client
			boincDisconnect();
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (Logging.DEBUG) Log.d(TAG, "onActivityResult()");
		if (requestCode == ACTIVITY_SELECT_HOST && resultCode == RESULT_OK) 
			// Finished successfully - selected the host to which we should connect
			mSelectedClient = data.getParcelableExtra(ClientId.TAG);
	}

	@Override
	public void clientConnectionProgress(BoincOp boincOp, int progress) {
		switch (progress) {
		case PROGRESS_INITIAL_DATA:
			// We are already connected, so hopefully we can display client ID in title bar
			// as well as progress spinner
			ClientId clientId = mConnectionManager.getClientId();
			if (clientId != null) {
				mConnectedClient = mConnectionManager.getClientId();
				refreshClientName();
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
			if (boincOp.equals(BoincOp.RunBenchmarks) || boincOp.equals(BoincOp.DoNetworkComm) ||
					boincOp.equals(BoincOp.StopUsingBAM) || boincOp.equals(BoincOp.GlobalPrefsOverride))
				updateParticularPreferences();
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
		refreshClientName();
		if (mConnectedClient != null) {
			// Connected client is retrieved
			if (Logging.DEBUG) Log.d(TAG, "Client " + mConnectedClient.getNickname() + " is connected");
			// Trigger retrieval of client run/network modes
			mPeriodicModeRetrievalAllowed = true;
			if (mConnectProgressIndicator != -1) {
				// We are still showing dialog about connection progress
				// Connect was just initiated by us (not reported by registering as observer)
				// We will update the dialog text now
				showProgressDialog(PROGRESS_INITIAL_DATA);
			}
			mConnectionManager.updateClientMode();
			
			updatePreferencesEnabled();
			
			if (Build.VERSION.SDK_INT >= 11)
				invalidateOptionsMenu();
			
			// update preferences
			updateParticularPreferences();
		}
		else {
			// Received connected notification, but client is unknown!
			if (Logging.ERROR) Log.e(TAG, "Client not connected despite notification");
		}
	}

	@Override
	public void clientDisconnected(boolean disconnectedByManager) {
		if (Logging.DEBUG) Log.d(TAG, "Client " + ( (mConnectedClient != null) ?
				mConnectedClient.getNickname() : "<not connected>" ) + " is disconnected");
		mConnectedClient = null;
		refreshClientName();
		mClientMode = null;
		refreshClientMode();
		updatePreferencesEnabled();
		setProgressBarIndeterminateVisibility(false);
		dismissProgressDialogs();
		mDoGetBAMInfo = false;
		mDoUpdateHostInfo = false;
		mSyncingBAMInProgress = false;
		if (mShowShutdownDialog) {
			mShowShutdownDialog = false;
			StandardDialogs.dismissDialog(this, DIALOG_WARN_SHUTDOWN);
		}
		if (mSelectedClient != null) {
			// Connection to another client is deferred, we proceed with it now
			boincConnect();
		}
		if (Build.VERSION.SDK_INT >= 11)
			invalidateOptionsMenu();
		
		// update preferences
		updateParticularPreferences();
	}
	
	@Override
	public boolean onAfterAccountMgrRPC() {
		dismissBAMProgressDialog();
		mSyncingBAMInProgress = false;
		// update preferences
		updateParticularPreferences();
		return true;
	}

	@Override
	public boolean onPollError(int errorNum, int operation, String errorMessage, String param) {
		setProgressBarIndeterminateVisibility(false);
		if (operation == PollOp.POLL_ATTACH_TO_BAM || operation == PollOp.POLL_SYNC_WITH_BAM) {
			dismissBAMProgressDialog();
			mDoGetBAMInfo = false;
			mSyncingBAMInProgress = false;
		}
		StandardDialogs.showPollErrorDialog(this, errorNum, operation, errorMessage, param);
		// update preferences
		updateParticularPreferences();
		return true;
	}
	
	@Override
	public  void onPollCancel(int opFlags) {
		// update preferences
		updateParticularPreferences();
	}

	@Override
	public boolean clientError(BoincOp boincOp, int errorNum, String message) {
		setProgressBarIndeterminateVisibility(false);
		if (boincOp.isBAMOperation())
			dismissBAMProgressDialog();
		else
			dismissClientProgressDialog();
		mDoGetBAMInfo = false;
		mDoUpdateHostInfo = false;
		mSyncingBAMInProgress = false;
		Log.d(TAG, "client error");
		StandardDialogs.showClientErrorDialog(this, errorNum, message);
		// update preferences
		updateParticularPreferences();
		return true;
	}
	
	@Override
	public boolean updatedClientMode(ModeInfo modeInfo) {
		if (Logging.DEBUG) Log.d(TAG, "Client run/network mode info updated, refreshing view");
		mClientMode = modeInfo;
		refreshClientMode();
		dismissClientProgressDialog();
		return mPeriodicModeRetrievalAllowed;
	}

	@Override
	public boolean updatedHostInfo(HostInfo hostInfo) {
		if (Logging.DEBUG) Log.d(TAG, "Host info received, displaying");
		mDoUpdateHostInfo = false;
		mHostInfo = hostInfo;
		dismissClientProgressDialog();
		if (mHostInfo != null) {
			showDialog(DIALOG_HOST_INFO);
		}
		return false;
	}
	
	@Override
	public boolean currentBAMInfo(AccountMgrInfo bamInfo) {
		if (Logging.DEBUG) Log.d(TAG, "BAM info received, using");
		mDoGetBAMInfo = false;
		if (bamInfo.acct_mgr_url.length() == 0) {
			// view dialog
			startActivity(new Intent(this, EditBAMActivity.class));
		} else if (!bamInfo.have_credentials) {
			// view dialog with data
			Intent intent = new Intent(this, EditBAMActivity.class);
			intent.putExtra(BAMAccount.TAG, new BAMAccount(bamInfo.acct_mgr_name, "", ""));
			startActivity(intent);
			// if end of operation (next part in edit bam activity)
			updateParticularPreferences();
		} else {
			mSyncingBAMInProgress = true;
			showDialog(DIALOG_ATTACH_BAM_PROGRESS);
			mConnectionManager.synchronizeWithBAM();
		}
		return true;
	}
	
	private void refreshClientName() {
		Preference pref = findPreference("selectedHost");
		if (mConnectedClient != null) {
			// We have data about client, so we set the info "nickname (address:port)"
			pref.setSummary(String.format("%s (%s:%d)", mConnectedClient.getNickname(),
					mConnectedClient.getAddress(), mConnectedClient.getPort()));
		}
		else {
			// Not connected to client
			pref.setSummary(getString(R.string.noHostConnected));
		}
	}
	
	private void refreshClientMode() {
		if ( (mConnectedClient != null) && (mClientMode != null) ) {
			// 1. The run-mode of currently connected client
			ListPreference listPref = (ListPreference)findPreference("actRunMode");
			CharSequence[] runDesc = listPref.getEntries();
			int idx = listPref.findIndexOfValue(Integer.toString(mClientMode.task_mode));
			listPref.setValueIndex(idx);
			listPref.setSummary(runDesc[idx]);
			// All operations depend on actRunMode
			// When this one is enabled, all others are enabled as well...
			listPref.setEnabled(true);
			// 2. The network mode of currently connected client
			listPref = (ListPreference)findPreference("actNetworkMode");
			runDesc = listPref.getEntries();
			idx = listPref.findIndexOfValue(Integer.toString(mClientMode.network_mode));
			listPref.setValueIndex(idx);
			listPref.setSummary(runDesc[idx]);
			// 3. The GPU mode of currently connected client
			listPref = (ListPreference)findPreference("actGpuMode");
			runDesc = listPref.getEntries();
			idx = listPref.findIndexOfValue(Integer.toString(mClientMode.gpu_mode));
			listPref.setValueIndex(idx);
			listPref.setSummary(runDesc[idx]);
		}
		else {
			// Has not not retrieved mode yet
			// actRunMode preference
			Preference pref = findPreference("actRunMode");
			pref.setSummary(getString(R.string.noHostConnected));
			// All operations depend on actRunMode
			// When this one is disabled, all others are disabled as well...
			pref.setEnabled(false);
			// actNetworkMode preference
			pref = findPreference("actNetworkMode");
			pref.setSummary(getString(R.string.noHostConnected));
			// actGpuMode preference
			pref = findPreference("actGpuMode");
			pref.setSummary(getString(R.string.noHostConnected));
		}
	}

	private void refreshClientModePending() {
		// All operations depend on actRunMode
		// We disable it, so all operations are grayed out and not accessible until
		// pending run/network mode retrieval is finished
		// This is used e.g. in onResume() when we are not sure whether last known modes
		// belong to the same client
		Preference pref = findPreference("actRunMode");
		pref.setEnabled(false);
		if (mClientMode != null) {
			// We have "some" info about modes, but it could be very obsolete
			// actRunMode preference
			ListPreference listPref = (ListPreference)pref;
			CharSequence[] runDesc = listPref.getEntries();
			int idx = listPref.findIndexOfValue(Integer.toString(mClientMode.task_mode));
			listPref.setValueIndex(idx);
			listPref.setSummary(runDesc[idx]);
			// actNetworkMode preference
			listPref = (ListPreference)findPreference("actNetworkMode");
			runDesc = listPref.getEntries();
			idx = listPref.findIndexOfValue(Integer.toString(mClientMode.network_mode));
			listPref.setValueIndex(idx);
			listPref.setSummary(runDesc[idx]);
			// actGpuMode preference
			listPref = (ListPreference)findPreference("actGpuMode");
			runDesc = listPref.getEntries();
			idx = listPref.findIndexOfValue(Integer.toString(mClientMode.gpu_mode));
			listPref.setValueIndex(idx);
			listPref.setSummary(runDesc[idx]);
		}
		else {
			// No info available (i.e. client never connected or disconnected)
			// actRunMode preference
			pref.setSummary(getString(R.string.retrievingData));
			// actNetworkMode preference
			pref = findPreference("actNetworkMode");
			pref.setSummary(getString(R.string.retrievingData));
			// actGpuMode preference
			pref = findPreference("actGpuMode");
			pref.setSummary(getString(R.string.retrievingData));
		}
	}
	
	/*
	 * enabling/disabling particular preferences
	 */
	private void disableRunBenchmarkPreference() {
		Preference pref = findPreference("runBenchmark");
		pref.setEnabled(false);
	}
	
	private void disableGlobalPreferencesPreference() {
		Preference pref = findPreference("globalPreferences");
		pref.setEnabled(false);
	}
	
	private void disableDoNetworkCommPreference() {
		Preference pref = findPreference("doNetworkCommunication");
		pref.setEnabled(false);
	}
	
	private void disableBAMPreferences() {
		Preference pref = findPreference("synchronizeWithBAM");
		pref.setEnabled(false);
		pref = findPreference("stopUsingBAM");
		pref.setEnabled(false);
	}
	
	private void updateParticularPreferences() {
		if (Logging.DEBUG) Log.d(TAG, "update particular prefs");
		if (mConnectionManager != null && mConnectedClient != null) {
			Preference pref = findPreference("runBenchmark");
			pref.setEnabled(!mConnectionManager.isOpBeingExecuted(BoincOp.RunBenchmarks));
			
			pref = findPreference("globalPreferences");
			pref.setEnabled(!mConnectionManager.isOpBeingExecuted(BoincOp.GlobalPrefsOverride));
			
			pref = findPreference("doNetworkCommunication");
			pref.setEnabled(!mConnectionManager.isOpBeingExecuted(BoincOp.DoNetworkComm));
			
			boolean bamIsWorking = mConnectionManager.isOpBeingExecuted(BoincOp.GetBAMInfo) ||
					mConnectionManager.isOpBeingExecuted(BoincOp.SyncWithBAM) ||
					mConnectionManager.isOpBeingExecuted(BoincOp.StopUsingBAM);
			pref = findPreference("synchronizeWithBAM");
			pref.setEnabled(!bamIsWorking);
			pref = findPreference("stopUsingBAM");
			pref.setEnabled(!bamIsWorking);
		} else {
			disableGlobalPreferencesPreference();
			disableRunBenchmarkPreference();
			disableDoNetworkCommPreference();
			disableBAMPreferences();
		}
	}
	
	
	private void updatePreferencesEnabled() {
		// get main dependency preference
		Preference pref = findPreference("localPreferences");
		// and select enabled or disabled
		pref.setEnabled(mConnectedClient != null);
	}
	
	private void showProgressDialog(final int progress) {
		if (mProgressDialogAllowed) {
			mConnectProgressIndicator = progress;
			showDialog(DIALOG_RETRIEVAL_PROGRESS);
		}
		else if (mConnectProgressIndicator != -1) {
			// Not allowed to show progress dialog (i.e. activity restarting/terminating),
			// but we are showing previous progress dialog - dismiss it
			StandardDialogs.dismissDialog(this, DIALOG_RETRIEVAL_PROGRESS);
			mConnectProgressIndicator = -1;
		}
	}

	private void dismissClientProgressDialog() {
		if (mConnectProgressIndicator != -1) {
			StandardDialogs.dismissDialog(this, DIALOG_RETRIEVAL_PROGRESS);
			mConnectProgressIndicator = -1;
		}
	}
	
	private void dismissBAMProgressDialog() {
		if (mSyncingBAMInProgress)
			StandardDialogs.dismissDialog(this, DIALOG_ATTACH_BAM_PROGRESS);
	}
	
	private void dismissProgressDialogs() {
		dismissClientProgressDialog();
		dismissBAMProgressDialog();
	}
	
	
	private void boincConnect() {
		if (mConnectionManager == null)
			return;
		try {
			mClientMode = null;
			refreshClientModePending();
			mConnectionManager.connect(mSelectedClient, false);
			mSelectedClient = null;
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
				/*if (Logging.DEBUG) Log.d(TAG, "Selected new client: " + mSelectedClient.getNickname() +
						", while already connected to: " + mConnectedClient.getNickname() +
						", disconnecting it first");*/
				boincDisconnect();
				// The boincConnect() will be triggered after the clientDisconnected() notification
			}
		}
	}
	
	/* used by synchronizeWithBAM */
	private void boincStopUsingBAM() {
		mConnectionManager.stopUsingBAM();
		Toast.makeText(this, getString(R.string.clientStopBAMNotify), Toast.LENGTH_LONG).show();
		disableBAMPreferences();
	}
	
	private void boincClearLocalPrefs() {
		mConnectionManager.setGlobalPrefsOverride("");
		disableGlobalPreferencesPreference();
	}
	
	private void boincChangeRunMode(int mode) {
		mConnectionManager.setRunMode(mode);
	}

	private void boincChangeNetworkMode(int mode) {
		mConnectionManager.setNetworkMode(mode);
	}
	
	private void boincChangeGpuMode(int mode) {
		mConnectionManager.setGpuMode(mode);
	}
	
	private void boincRunCpuBenchmarks() {
		mConnectionManager.runBenchmarks();
		Toast.makeText(this, getString(R.string.clientRunBenchNotify), Toast.LENGTH_LONG).show();
		disableRunBenchmarkPreference();
	}

	private void boincDoNetworkCommunication() {
		mConnectionManager.doNetworkCommunication();
		Toast.makeText(this, getString(R.string.clientDoNetCommNotify), Toast.LENGTH_LONG).show();
		disableDoNetworkCommPreference();
	}

	private void boincShutdownClient() {
		mConnectionManager.shutdownCore();
		Toast.makeText(this, getString(R.string.clientShutdownNotify), Toast.LENGTH_LONG).show();
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}
}
