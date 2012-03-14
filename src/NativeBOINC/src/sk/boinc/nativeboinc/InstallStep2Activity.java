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

package sk.boinc.nativeboinc;

import hal.android.workarounds.FixedProgressDialog;

import java.io.IOException;

import sk.boinc.nativeboinc.clientconnection.ClientReceiver;
import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincStateListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUtils;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import sk.boinc.nativeboinc.util.StandardDialogs;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.text.InputType;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;

/**
 * @author mat
 *
 */
public class InstallStep2Activity extends ServiceBoincActivity implements ClientReceiver,
		NativeBoincStateListener {

	private final static String TAG = "InstallStep2Activity";
	
	private static final int DIALOG_CHANGE_PASSWORD = 1;
	private static final int DIALOG_CHANGE_HOSTNAME = 2;
	private static final int DIALOG_RESTART_PROGRESS = 3;
	
	private BoincManagerApplication mApp = null;
	
	/* next step id's (used after restarting client) */
	private static final int NEXT_STEP_NOTHING = 0;
	private static final int NEXT_STEP_PROJECT_LIST = 1;
	private static final int NEXT_STEP_EDIT_BAM = 2;
	private static final int NEXT_STEP_FINISH = 3;
	
	private static final int NO_RESTART = 0;
	private static final int DO_RESTART = 1;
	private static final int RESTARTING = 2;
	private static final int RESTARTED = 3;
	
	private Button mAddProjectButton = null;
	private Button mSyncBAMButton = null;
	
	private boolean mConnectionFailed = false;
	private ClientId mConnectedClient = null;
	private int mDoRestart = NO_RESTART;
	private int mNextInstallStep = NEXT_STEP_NOTHING;
	
	private static class SavedState {
		private final boolean connectionFailed;
		private final int doRestart;
		private final int nextInstallStep;
		
		public SavedState(InstallStep2Activity activity) {
			connectionFailed = activity.mConnectionFailed;
			doRestart = activity.mDoRestart;
			nextInstallStep = activity.mNextInstallStep;
		}
		
		public void restore(InstallStep2Activity activity) {
			activity.mConnectionFailed = connectionFailed;
			activity.mDoRestart = doRestart;
			activity.mNextInstallStep = nextInstallStep;
		}
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		mApp = (BoincManagerApplication)getApplication();
		
		setUpService(true, true, true, true, false, false);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.install_step2);
		
		/* setup buttons */
		Button changePassword = (Button)findViewById(R.id.changePassword);
		changePassword.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				showDialog(DIALOG_CHANGE_PASSWORD);
			}
		});
		
		Button changeHostname = (Button)findViewById(R.id.changeHostname);
		changeHostname.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				showDialog(DIALOG_CHANGE_HOSTNAME);
			}
		});
		
		/* bottom buttons */
		mAddProjectButton = (Button)findViewById(R.id.addProject);
		mSyncBAMButton = (Button)findViewById(R.id.syncBAM);
		Button cancelButton = (Button)findViewById(R.id.installCancel);
		
		cancelButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				InstallStep2Activity.this.onBackPressed();
			}
		});
		
		mAddProjectButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mDoRestart == NO_RESTART)
					startActivity(new Intent(InstallStep2Activity.this, ProjectListActivity.class));
				else if (mDoRestart == DO_RESTART) {
					if (Logging.DEBUG) Log.d(TAG, "next and restart");
					mDoRestart = RESTARTING;
					mNextInstallStep = NEXT_STEP_PROJECT_LIST;
					showDialog(DIALOG_RESTART_PROGRESS);
					mRunner.restartClient();
				}
			}
		});
		
		mSyncBAMButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mDoRestart == NO_RESTART)
					startActivity(new Intent(InstallStep2Activity.this, EditBAMActivity.class));
				else if (mDoRestart == DO_RESTART) {
					if (Logging.DEBUG) Log.d(TAG, "next and restart");
					mDoRestart = RESTARTING;
					mNextInstallStep = NEXT_STEP_EDIT_BAM;
					showDialog(DIALOG_RESTART_PROGRESS);
					mRunner.restartClient();
				}
			}
		});
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		if (Logging.DEBUG) Log.d(TAG, "onResume: "+mApp.getInstallerStage());
		
		if (!mApp.isInstallerRun()) {
			// installer finished then close it
			finish();
			return;
		}
		
		if (mRunner != null && !mConnectionFailed) {
			if (!mRunner.isRun()) {
				setProgressBarIndeterminateVisibility(true);
				mRunner.startClient(false);
			} else if (mConnectionManager != null)	// now try to connect
				connectWithNativeClient();
		}
	}
	
	private void connectWithNativeClient() {
		HostListDbAdapter dbAdapter = null;
		try {
			dbAdapter = new HostListDbAdapter(this);
			dbAdapter.open();
			
			if (Logging.DEBUG) Log.d(TAG, "Connect with nativeboinc");
			
			ClientId client = dbAdapter.fetchHost("nativeboinc");
			ClientId connected = mConnectionManager.getClientId();
			
			if (connected == null || !client.equals(connected)) {
				// if not connected
				mConnectionManager.connect(client, true);
				// starting connecting
				setProgressBarIndeterminateVisibility(true);
			}
		} catch(NoConnectivityException ex) {
			// error
		} finally {
			dbAdapter.close();
		}
	}
	
	@Override
	protected void onConnectionManagerConnected() {
		mConnectedClient = mConnectionManager.getClientId();
		if (mRunner != null && mRunner.isRun() && !mConnectionFailed)
			connectWithNativeClient();
	}
	
	@Override
	protected void onConnectionManagerDisconnected() {
		mConnectedClient = null;
		setProgressBarIndeterminateVisibility(false);
	}
	
	@Override
	protected void onRunnerConnected() {
		setProgressBarIndeterminateVisibility(mRunner.serviceIsWorking());
		if (!mConnectionFailed) {
			if (!mRunner.isRun()) {
				mRunner.startClient(false);
			} else if (mConnectionManager != null)	// now try to connect
				connectWithNativeClient();
		}
	}
	
	@Override
	protected void onRunnerDisconnected() {
		setProgressBarIndeterminateVisibility(false);
	}
	
	
	@Override
	public void onBackPressed() {
		if (mDoRestart == NO_RESTART) {
			finish();
			startActivity(new Intent(this, InstallFinishActivity.class));
		} else if (mDoRestart == DO_RESTART) {
			if (Logging.DEBUG) Log.d(TAG, "cancel and restart");
			mDoRestart = RESTARTING;
			mNextInstallStep = NEXT_STEP_FINISH;
			showDialog(DIALOG_RESTART_PROGRESS);
			mRunner.restartClient();
		}
	}
	
	@Override
	protected Dialog onCreateDialog(int dialogId, Bundle args) {
		Dialog dialog = StandardDialogs.onCreateDialog(this, dialogId, args);
		if (dialog != null)
			return dialog;
		
		switch(dialogId) {
		case DIALOG_CHANGE_HOSTNAME: {
			View view = LayoutInflater.from(this).inflate(R.layout.dialog_edit, null);
			final EditText edit = (EditText)view.findViewById(android.R.id.edit);
			/* set as normal text (input type) */
			edit.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
			
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_input_get)
				.setTitle(R.string.projectUrl)
				.setView(view)
				.setPositiveButton(R.string.ok, new Dialog.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						changeHostname(edit.getText().toString());
					}
				})
				.setNegativeButton(R.string.cancel, null)
				.create();
		}
		case DIALOG_CHANGE_PASSWORD: {
			View view = LayoutInflater.from(this).inflate(R.layout.dialog_edit, null);
			final EditText edit = (EditText)view.findViewById(android.R.id.edit);
			/* set as password (input type) */
			edit.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
			
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_input_get)
				.setTitle(R.string.projectUrl)
				.setView(view)
				.setPositiveButton(R.string.ok, new Dialog.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						changePassword(edit.getText().toString());
					}
				})
				.setNegativeButton(R.string.cancel, null)
				.create();
		}
		case DIALOG_RESTART_PROGRESS:
			ProgressDialog progressDialog = new FixedProgressDialog(this);
			progressDialog.setIndeterminate(true);
			progressDialog.setCancelable(true);
			return progressDialog;
		}
		return null;
	}
	
	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		if (StandardDialogs.onPrepareDialog(this, dialogId, dialog, args))
			return;
		
		switch(dialogId) {
		case DIALOG_CHANGE_HOSTNAME: {
			EditText edit = (EditText)dialog.findViewById(android.R.id.edit);
			
			String hostname = "";
			try {
				hostname = NativeBoincUtils.getHostname(this);
			} catch(IOException ex) { }
			
			edit.setText(hostname);
			break;
		}
		case DIALOG_CHANGE_PASSWORD: {
			EditText edit = (EditText)dialog.findViewById(android.R.id.edit);
			
			String password = "";
			try {
				password = NativeBoincUtils.getAccessPassword(this);
			} catch(IOException ex) { }
			
			edit.setText(password);
			break;
		}
		case DIALOG_RESTART_PROGRESS:
			ProgressDialog pd = (ProgressDialog)dialog;
			pd.setMessage(getString(R.string.clientRestarting));
			break;
		}
	}

	@Override
	public void clientConnectionProgress(int progress) {
		setProgressBarIndeterminateVisibility(true);
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		// if after connecting
		setProgressBarIndeterminateVisibility(false);
		mAddProjectButton.setEnabled(true);
		mSyncBAMButton.setEnabled(true);
		mConnectedClient = mConnectionManager.getClientId();
		if (mDoRestart == RESTARTED) {
			// reset dorestart
			mDoRestart = NO_RESTART;
			// if connected do it
			switch(mNextInstallStep) {
			case NEXT_STEP_PROJECT_LIST:
				if (Logging.DEBUG) Log.d(TAG, "restart. go to project list");
				startActivity(new Intent(this, ProjectListActivity.class));
				break;
			case NEXT_STEP_FINISH:
				if (Logging.DEBUG) Log.d(TAG, "restart. go to finish");
				finish();
				startActivity(new Intent(this, InstallFinishActivity.class));
				break;
			case NEXT_STEP_EDIT_BAM:
				if (Logging.DEBUG) Log.d(TAG, "restart. go to project list");
				startActivity(new Intent(this, EditBAMActivity.class));
				break;
			}
		}
	}

	@Override
	public void clientDisconnected() {
		if (Logging.WARNING) Log.w(TAG, "Client disconnected");
		// if after try (failed)
		setProgressBarIndeterminateVisibility(false);

		if (mDoRestart == NO_RESTART)
			StandardDialogs.tryShowDisconnectedErrorDialog(this, mConnectionManager, mRunner,
					mConnectedClient);
		
		mConnectedClient = null;
	}

	@Override
	public void onNativeBoincClientError(String message) {
		setProgressBarIndeterminateVisibility(false);
		mConnectionFailed = true;
		// reset restart
		mDoRestart = NO_RESTART;
		StandardDialogs.showErrorDialog(this, message);
	}

	@Override
	public void onClientStart() {
		if (mDoRestart == RESTARTING) {
			if (Logging.DEBUG) Log.d(TAG, "Set as restarted");
			mDoRestart = RESTARTED;
			dismissDialog(DIALOG_RESTART_PROGRESS);
		}
		if (mConnectionManager != null)
			connectWithNativeClient();
	}
	
	@Override
	public void onClientStop(int exitCode, boolean stoppedByManager) {
		// do nothing
	}

	@Override
	public boolean clientError(int errorNum, String message) {
		setProgressBarIndeterminateVisibility(false);
		mConnectionFailed = true;
		StandardDialogs.showClientErrorDialog(this, errorNum, message);
		return true;
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}
	
	/* modify change */
	private void changeHostname(String hostname) {
		try {
			if (hostname != null && hostname.length() != 0)
				NativeBoincUtils.setHostname(this, hostname);
			
			if (Logging.DEBUG) Log.d(TAG, "Set to restart");
			mDoRestart = DO_RESTART;
		} catch(IOException ex) {
			
		}
	}
	
	private void changePassword(String password) {
		try {
			NativeBoincUtils.setAccessPassword(this, password);
			if (Logging.DEBUG) Log.d(TAG, "Set to restart");
			mDoRestart = DO_RESTART;
		} catch(IOException ex) {
			
		}
	}
}
