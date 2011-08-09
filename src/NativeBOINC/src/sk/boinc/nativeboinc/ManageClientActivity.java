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

import java.util.Arrays;
import java.util.Comparator;
import java.util.Vector;

import edu.berkeley.boinc.lite.AccountIn;
import edu.berkeley.boinc.lite.AccountMgrInfo;
import edu.berkeley.boinc.lite.ProjectConfig;
import edu.berkeley.boinc.lite.ProjectListEntry;

import sk.boinc.nativeboinc.clientconnection.ClientManageReceiver;
import sk.boinc.nativeboinc.clientconnection.HostInfo;
import sk.boinc.nativeboinc.clientconnection.MessageInfo;
import sk.boinc.nativeboinc.clientconnection.ModeInfo;
import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.clientconnection.ProjectInfo;
import sk.boinc.nativeboinc.clientconnection.TaskInfo;
import sk.boinc.nativeboinc.clientconnection.TransferInfo;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.InstallerListener;
import sk.boinc.nativeboinc.nativeclient.InstallerService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.ProjectDistrib;
import sk.boinc.nativeboinc.service.ConnectionManagerService;
import sk.boinc.nativeboinc.util.AddProjectResult;
import sk.boinc.nativeboinc.util.BAMAccount;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import sk.boinc.nativeboinc.util.ProjectConfigOptions;
import sk.boinc.nativeboinc.util.ProjectItem;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.DialogInterface.OnCancelListener;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.text.Html;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.Window;
import android.widget.TextView;
import android.widget.Toast;


public class ManageClientActivity extends PreferenceActivity implements ClientManageReceiver,
	InstallerListener, NativeBoincListener {
	private static final String TAG = "ManageClientActivity";

	private static final int DIALOG_WARN_SHUTDOWN = 0;
	private static final int DIALOG_RETRIEVAL_PROGRESS = 1;
	private static final int DIALOG_HOST_INFO = 2;
	private static final int DIALOG_CLIENT_ERROR = 3;

	private static final int ACTIVITY_SELECT_HOST = 1;
	private static final int ACTIVITY_EDIT_BAM_INFO = 2;
	private static final int ACTIVITY_SELECT_PROJECT = 3;
	private static final int ACTIVITY_ADD_PROJECT = 4;

	private int mConnectProgressIndicator = -1;
	private boolean mProgressDialogAllowed = false;
	private ModeInfo mClientMode = null;
	private HostInfo mHostInfo = null;
	
	private int mClientErrorNum = 0;
	private Vector<String> mClientErrorMessages = null;
	
	private AccountMgrInfo mBAMInfo = null;
	private boolean mPeriodicModeRetrievalAllowed = false;
	
	private String mProjectUrl = null;

	private ConnectionManagerService mConnectionManager = null;
	private InstallerService mInstaller = null;
	private boolean mDelayedObserverRegistration = false;
	private ClientId mConnectedClient = null;
	private ClientId mSelectedClient = null;
	
	private boolean mShutdownByProjectAttach = false;
	
	private ProgressDialog mProgressDialog = null;
	
	private class SavedState {
		public final HostInfo hostInfo;

		public SavedState() {
			hostInfo = mHostInfo;
			if (Logging.DEBUG) Log.d(TAG, "saved: hostInfo=" + hostInfo);
		}
		public void restoreState(ManageClientActivity activity) {
			activity.mHostInfo = hostInfo;
			if (Logging.DEBUG) Log.d(TAG, "restored: mHostInfo=" + activity.mHostInfo);
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
	
	private ServiceConnection mInstallerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mInstaller = ((InstallerService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "installer.onServiceConnected()");
			mInstaller.addInstallerListener(ManageClientActivity.this);
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
			mRunner.addNativeBoincListener(ManageClientActivity.this);
		}
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			mRunner = null;
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceDisconnected()");
		}
	};

	private void doBindService() {
		if (Logging.DEBUG) Log.d(TAG, "doBindService()");
		bindService(new Intent(ManageClientActivity.this, ConnectionManagerService.class),
				mServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doBindInstallerService() {
		if (Logging.DEBUG) Log.d(TAG, "doBindService()");
		bindService(new Intent(ManageClientActivity.this, InstallerService.class),
				mInstallerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doBindRunnerService() {
		bindService(new Intent(ManageClientActivity.this, NativeBoincService.class),
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
		// The super-class PreferenceActivity calls setContentView()
		// in its onCreate(). So we must request window features here.
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);

		super.onCreate(savedInstanceState);

		doBindService();
		doBindInstallerService();
		doBindRunnerService();

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
				boincGetBAMInfo();
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
				if (isNativeConnected()) {
					mInstaller.updateProjectDistribs();
				} else
					boincGetAllProjectsList();
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
				if (!isNativeConnected())
					showDialog(DIALOG_WARN_SHUTDOWN);
				else
					boincShutdownClient();
				return true;
			}
		});
		
		// on change access password
		pref = findPreference("changeAccessPassword");
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			public boolean onPreferenceClick(Preference preference) {
				startActivity(new Intent(ManageClientActivity.this, AccessPasswordActivity.class));
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

	@Override
	protected void onResume() {
		super.onResume();
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
		refreshAccessPasswordOption();

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
				mConnectionManager.updateClientMode(this);
			}
			else {
				// We are not connected - update display accordingly
				mClientMode = null;
				refreshClientMode();
			}
		}
	}

	@Override
	protected void onPause() {
		super.onPause();
		// No more repeated displays
		mPeriodicModeRetrievalAllowed = false;
		mProgressDialogAllowed = false;
		dismissProgressDialog();
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
		if (mRunner != null) {
			mRunner.removeNativeBoincListener(this);
			mRunner = null;
		}
		if (mInstaller != null) {
			mInstaller.removeInstallerListener(this);
			mInstaller = null;
		}
		doUnbindService();
		doUnbindInstallerService();
		doUnbindRunnerService();
		super.onDestroy();
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent keyEvent) {
		// do nothing
		if (keyCode == KeyEvent.KEYCODE_BACK && mInstaller != null) {
			if (Logging.DEBUG) Log.d(TAG, "Cancel installer operation");
			mInstaller.cancelOperation();
			mShutdownByProjectAttach = false;
		}
		return super.onKeyDown(keyCode, keyEvent);
	}

	@Override
	public Object onRetainNonConfigurationInstance() {
		final SavedState savedState = new SavedState();
		return savedState;
	}

	@Override
	protected Dialog onCreateDialog(int id) {
		ProgressDialog progressDialog;
		switch (id) {
		case DIALOG_WARN_SHUTDOWN:
        	return new AlertDialog.Builder(this)
        		.setIcon(android.R.drawable.ic_dialog_alert)
        		.setTitle(R.string.warning)
        		.setMessage(R.string.warnShutdownText)
        		.setPositiveButton(R.string.shutdown,
        			new DialogInterface.OnClickListener() {
        				public void onClick(DialogInterface dialog, int whichButton) {
        					boincShutdownClient();
        				}
        			})
        		.setNegativeButton(R.string.cancel, null)
        		.create();
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
		case DIALOG_CLIENT_ERROR:
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.clientError)
				.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
				.setNegativeButton(R.string.ok, null)
				.setOnCancelListener(new OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						// We don't need data anymore - allow them to be garbage collected
						mClientErrorMessages = null;
					}
				})
				.create();
		}
		return null;
	}

	@Override
	protected void onPrepareDialog(int id, Dialog dialog) {
		switch (id) {
		case DIALOG_RETRIEVAL_PROGRESS:
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
		case DIALOG_HOST_INFO: {
			TextView text = (TextView)dialog.findViewById(R.id.dialogText);
			text.setText(Html.fromHtml(mHostInfo.htmlText));
			break;
		}
		case DIALOG_CLIENT_ERROR: {
			TextView text = (TextView)dialog.findViewById(R.id.dialogText);
			StringBuilder sB = new StringBuilder();
			sB.append(getString(R.string.clientErrorNum));
			sB.append(mClientErrorNum);
			sB.append("\n");
			for (String msg: mClientErrorMessages) {
				sB.append(msg);
				sB.append("\n");
			}
			text.setText(sB.toString());
			break;
		}
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
			mConnectionManager.updateHostInfo(this);
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
		switch (requestCode) {
		case ACTIVITY_SELECT_HOST:
			if (resultCode == RESULT_OK) {
				// Finished successfully - selected the host to which we should connect
				mSelectedClient = data.getParcelableExtra(ClientId.TAG);
			} 
			break;
		case ACTIVITY_EDIT_BAM_INFO:
			if (resultCode == RESULT_OK) {
				BAMAccount bamAccount = data.getParcelableExtra(BAMAccount.TAG);
				/* attach to BAM manager */
				boincAttachToBAM(bamAccount.getName(), bamAccount.getUrl(),
						bamAccount.getPassword());
			}
			break;
		case ACTIVITY_SELECT_PROJECT:
			if (resultCode == RESULT_OK) {
				ProjectItem project = data.getParcelableExtra(ProjectItem.TAG);
				/* contact with project */
				mProjectUrl = project.getUrl();
				boincProjectConfig(project.getUrl());
			}
			break;
		case ACTIVITY_ADD_PROJECT:
			if (resultCode == RESULT_OK) {
				AddProjectResult result = data.getParcelableExtra(AddProjectResult.TAG);
				// adding project
				if (result.isCreateAccount()) {
					boincCreateAccount(result.getName(), result.getEmail(), result.getPassword());
				} else {	// lookup account
					boincLookupAccount(result.getEmail(), result.getPassword());
				}
			}
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
				mConnectedClient = mConnectionManager.getClientId();
				refreshClientName();
				refreshAccessPasswordOption();
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
		refreshClientName();
		refreshAccessPasswordOption();
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
			mConnectionManager.updateClientMode(this);
		}
		else {
			// Received connected notification, but client is unknown!
			if (Logging.ERROR) Log.e(TAG, "Client not connected despite notification");
		}
	}

	@Override
	public void clientDisconnected() {
		if (Logging.DEBUG) Log.d(TAG, "Client " + ( (mConnectedClient != null) ?
				mConnectedClient.getNickname() : "<not connected>" ) + " is disconnected");
		mConnectedClient = null;
		refreshClientName();
		mClientMode = null;
		refreshClientMode();
		setProgressBarIndeterminateVisibility(false);
		dismissProgressDialog();
		if (mSelectedClient != null) {
			// Connection to another client is deferred, we proceed with it now
			boincConnect();
		}
	}
	
	@Override
	public boolean clientError(int errorNum, Vector<String> messages) {
		mClientErrorNum = errorNum;
		mClientErrorMessages = messages;
		showDialog(DIALOG_CLIENT_ERROR);
		return false;
	}

	@Override
	public boolean updatedClientMode(ModeInfo modeInfo) {
		if (Logging.DEBUG) Log.d(TAG, "Client run/network mode info updated, refreshing view");
		mClientMode = modeInfo;
		refreshClientMode();
		dismissProgressDialog();
		return mPeriodicModeRetrievalAllowed;
	}

	@Override
	public boolean updatedHostInfo(HostInfo hostInfo) {
		if (Logging.DEBUG) Log.d(TAG, "Host info received, displaying");
		mHostInfo = hostInfo;
		dismissProgressDialog();
		if (mHostInfo != null) {
			showDialog(DIALOG_HOST_INFO);
		}
		return false;
	}
	
	@Override
	public boolean currentBAMInfo(AccountMgrInfo accountMgrInfo) {
		if (Logging.DEBUG) Log.d(TAG, "BAM info received, using");
		mBAMInfo = accountMgrInfo;
		if (mBAMInfo.acct_mgr_url.length() == 0) {
			// view dialog
			startActivityForResult(new Intent(this, EditBAMActivity.class), ACTIVITY_EDIT_BAM_INFO);
		} else if (!mBAMInfo.have_credentials) {
			// view dialog with data
			Intent intent = new Intent(this, EditBAMActivity.class);
			intent.putExtra(BAMAccount.TAG, new BAMAccount(mBAMInfo.acct_mgr_name, "", ""));
			startActivityForResult(intent, ACTIVITY_EDIT_BAM_INFO);
		} else {
			boincSynchronizeWithBAM();
		}
		return false;
	}
	
	@Override
	public boolean currentAllProjectsList(Vector<ProjectListEntry> allProjects) {
		ProjectItem[] projectItems = new ProjectItem[allProjects.size()];
		for (int i = 0; i < allProjects.size(); i++) {
			ProjectListEntry entry = allProjects.get(i);
			projectItems[i] = new ProjectItem(entry.name, entry.url);
		}
		
		Arrays.sort(projectItems, new Comparator<ProjectItem>() {
			@Override
			public int compare(ProjectItem item1 , ProjectItem item2) {
				return item1.getName().compareTo(item2.getName());
			}
		});
		
		Intent intent = new Intent(this, ProjectListActivity.class);
		intent.putExtra(ProjectItem.TAG, projectItems);
		startActivityForResult(intent, ACTIVITY_SELECT_PROJECT);
		return false;
	}
	
	@Override
	public boolean currentAuthCode(String authCode) {
		boincProjectAttach(mProjectUrl, authCode, "");
		return false;
	}
	
	@Override
	public boolean currentProjectConfig(ProjectConfig projectConfig) {
		// start AddProject dialog
		Intent intent = new Intent(this, AddProjectActivity.class);
		intent.putExtra(ProjectConfigOptions.TAG, new ProjectConfigOptions(projectConfig));
		startActivityForResult(intent, ACTIVITY_ADD_PROJECT);
		return false;
	}
	
	@Override
	public boolean onAfterProjectAttach() {
		if (isNativeConnected()) {
			mShutdownByProjectAttach = true;
			mConnectionManager.updateProjects(this);
		}
		return false;
	}
	
	@Override
	public boolean updatedMessages(Vector<MessageInfo> messages) {
		// Never requested, nothing to do
		return false;
	}

	@Override
	public boolean updatedProjects(Vector<ProjectInfo> projects) {
		if (mShutdownByProjectAttach) {
			if (Logging.DEBUG) Log.d(TAG, "retrieve project list - for synchronizing");
			mInstaller.synchronizeInstalledProjects(projects);
			mRunner.shutdownClient();
		}
		return false;
	}

	@Override
	public boolean updatedTasks(Vector<TaskInfo> tasks) {
		// Never requested, nothing to do
		return false;
	}

	@Override
	public boolean updatedTransfers(Vector<TransferInfo> transfers) {
		// Never requested, nothing to do
		return false;
	}
	
	/*
	 * InstallerListener methods
	 */
	@Override
	public void onOperation(String opDescription) {
		setProgressBarIndeterminateVisibility(true);
		Toast.makeText(this, opDescription, Toast.LENGTH_SHORT).show();
	}
	
	@Override
	public void onOperationProgress(String opDescription, int progress) {
		if (mProgressDialog == null) {
			mProgressDialog = new ProgressDialog(this);
			mProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
		    mProgressDialog.setMessage(opDescription);
		    mProgressDialog.setMax(10000);
		    mProgressDialog.setProgress(0);
		    mProgressDialog.setCancelable(true);
		    mProgressDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
				@Override
				public void onCancel(DialogInterface dialog) {
					mInstaller.cancelOperation();
					finish();
				}
			});
		    mProgressDialog.show();
		}
		setProgressBarIndeterminateVisibility(true);
		mProgressDialog.setProgress(progress);
		if (progress == 10000) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
	}
	
	@Override
	public void onOperationError(String errorMessage) {
		Toast.makeText(this, errorMessage, Toast.LENGTH_LONG).show();
		if (mProgressDialog != null) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
		setProgressBarIndeterminateVisibility(false);
		mShutdownByProjectAttach = false;
	}
	
	@Override
	public void onOperationCancel() {
		Toast.makeText(this, R.string.operationCancelled, Toast.LENGTH_LONG).show();
		if (mProgressDialog != null) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
		setProgressBarIndeterminateVisibility(false);
	}
	
	@Override
	public void onOperationFinish() {
		if (mProgressDialog != null) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
		setProgressBarIndeterminateVisibility(false);
		
		if (mShutdownByProjectAttach) {
			mRunner.startClient(); // restart native client
		}
	}

	@Override
	public void currentProjectDistribs(Vector<ProjectDistrib> projectDistribs) {
		ProjectItem[] projectItems = new ProjectItem[projectDistribs.size()];
		
		/* prepare list */
		for (int i = 0; i < projectDistribs.size(); i++) {
			ProjectDistrib distrib = projectDistribs.get(i);
			projectItems[i] = new ProjectItem(distrib.projectName, distrib.projectUrl);
		}
		
		Intent intent = new Intent(this, ProjectListActivity.class);
		intent.putExtra(ProjectItem.TAG, projectItems);
		startActivityForResult(intent, ACTIVITY_SELECT_PROJECT);
	}
	
	/* native boinc listener */
	public void onClientStateChanged(boolean isRun) {
		if (isRun) {
			if(mShutdownByProjectAttach) {
				/* reconnect with native client */
				mShutdownByProjectAttach = false;
				HostListDbAdapter dbHelper = new HostListDbAdapter(this);
				dbHelper.open();
				mSelectedClient = dbHelper.fetchHost("nativeboinc");
				dbHelper.close();
				if (mSelectedClient != null)
					boincConnect();
			}
		} else {
			if (Logging.DEBUG) Log.d(TAG, "onclientStateChanged() - shutdownByProjectAttach:" +
					mShutdownByProjectAttach);
			if (mShutdownByProjectAttach) {
				/* get updated projects for installation */
				mInstaller.installBoincApplicationAutomatically(mProjectUrl);
			}
		}
	}
	
	public void onClientFirstStart() {
		// do nothing
	}
	
	public void onAfterClientFirstKill() {
		// do nothing
	}
	
	public void onClientError(String message) {
		Toast.makeText(this, message, Toast.LENGTH_LONG).show();
		mShutdownByProjectAttach = false;
		finish();
	}
	
	public void onClientConfigured() {
		// do nothing
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
	
	private void refreshAccessPasswordOption() {
		Preference pref = findPreference("changeAccessPassword");
		pref.setEnabled(isNativeConnected());
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
		}
		else {
			// No info available (i.e. client never connected or disconnected)
			// actRunMode preference
			pref.setSummary(getString(R.string.retrievingData));
			// actNetworkMode preference
			pref = findPreference("actNetworkMode");
			pref.setSummary(getString(R.string.retrievingData));
		
		}
	}

	private void showProgressDialog(final int progress) {
		if (mProgressDialogAllowed) {
			mConnectProgressIndicator = progress;
			showDialog(DIALOG_RETRIEVAL_PROGRESS);
		}
		else if (mConnectProgressIndicator != -1) {
			// Not allowed to show progress dialog (i.e. activity restarting/terminating),
			// but we are showing previous progress dialog - dismiss it
			dismissDialog(DIALOG_RETRIEVAL_PROGRESS);
			mConnectProgressIndicator = -1;
		}
	}

	private void dismissProgressDialog() {
		if (mConnectProgressIndicator != -1) {
			dismissDialog(DIALOG_RETRIEVAL_PROGRESS);
			mConnectProgressIndicator = -1;
		}
	}

	private void boincConnect() {
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
				if (Logging.DEBUG) Log.d(TAG, "Selected new client: " + mSelectedClient.getNickname() +
						", while already connected to: " + mConnectedClient.getNickname() +
						", disconnecting it first");
				boincDisconnect();
				// The boincConnect() will be triggered after the clientDisconnected() notification
			}
		}
	}
	
	private boolean isNativeConnected() {
		if (mConnectedClient == null)
			return false;
		String clientAddress = mConnectedClient.getAddress();
		return clientAddress.equals("127.0.0.1") || clientAddress.equals("localhost");
	}
	
	/* used by synchronizeWithBAM */
	private void boincGetBAMInfo() {
		mConnectionManager.getBAMInfo(this);
	}
	
	private void boincAttachToBAM(String name, String url, String password) {
		mConnectionManager.attachToBAM(this, name, url, password);
		Toast.makeText(this, getString(R.string.clientSyncBAMNotify), Toast.LENGTH_LONG).show();
	}
	
	private void boincSynchronizeWithBAM() {
		mConnectionManager.synchronizeWithBAM(this);
		Toast.makeText(this, getString(R.string.clientSyncBAMNotify), Toast.LENGTH_LONG).show();
	}
	
	private void boincStopUsingBAM() {
		mConnectionManager.stopUsingBAM(this);
		Toast.makeText(this, getString(R.string.clientStopBAMNotify), Toast.LENGTH_LONG).show();
	}
	
	private void boincGetAllProjectsList() {
		mConnectionManager.getAllProjectsList(this);
	}
	
	private void boincProjectConfig(String url) {
		mConnectionManager.getProjectConfig(this, url);
		Toast.makeText(this, getString(R.string.clientProjectConfig), Toast.LENGTH_LONG).show();
	}

	private void boincCreateAccount(String name, String email, String password) {
		AccountIn accountIn = new AccountIn();
		accountIn.url = mProjectUrl;
		accountIn.user_name = name;
		accountIn.email_addr = email;
		accountIn.passwd = password;
		mConnectionManager.createAccount(this, accountIn);
		Toast.makeText(this, getString(R.string.clientCreatingAccount), Toast.LENGTH_LONG).show();
	}
	
	private void boincLookupAccount(String email, String password) {
		AccountIn accountIn = new AccountIn();
		accountIn.url = mProjectUrl;
		accountIn.email_addr = email;
		accountIn.passwd = password;
		mConnectionManager.lookupAccount(this, accountIn);
		Toast.makeText(this, getString(R.string.clientLookingUpAccount), Toast.LENGTH_LONG).show();
	}
	
	private void boincProjectAttach(String url, String authCode, String projectName) {
		mConnectionManager.projectAttach(this, url, authCode, projectName);
		Toast.makeText(this, getString(R.string.clientAttachingToProject), Toast.LENGTH_LONG).show();
	}

	private void boincChangeRunMode(int mode) {
		mConnectionManager.setRunMode(this, mode);
	}

	private void boincChangeNetworkMode(int mode) {
		mConnectionManager.setNetworkMode(this, mode);
	}
	
	private void boincRunCpuBenchmarks() {
		mConnectionManager.runBenchmarks();
		Toast.makeText(this, getString(R.string.clientRunBenchNotify), Toast.LENGTH_LONG).show();
	}

	private void boincDoNetworkCommunication() {
		mConnectionManager.doNetworkCommunication();
		Toast.makeText(this, getString(R.string.clientDoNetCommNotify), Toast.LENGTH_LONG).show();
	}

	private void boincShutdownClient() {
		mConnectionManager.shutdownCore();
		Toast.makeText(this, getString(R.string.clientShutdownNotify), Toast.LENGTH_LONG).show();
	}
}
