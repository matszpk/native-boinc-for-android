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

import java.io.IOException;

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.installer.AbstractInstallerListener;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.nativeclient.AbstractNativeBoincListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUtils;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.ScreenOrientationHandler;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.text.InputType;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.widget.EditText;

/**
 * @author mat
 *
 */
public class NativeClientActivity extends PreferenceActivity implements AbstractNativeBoincListener,
		AbstractInstallerListener {

	private static final String TAG = "NativeClientActivity";
	
	private static final int ACTIVITY_ACCESS_LIST = 1;
	
	private static final int DIALOG_APPLY_AFTER_RESTART = 1;
	private static final int DIALOG_ENTER_DUMP_DIRECTORY = 2;
	private static final int DIALOG_REINSTALL_QUESTION = 3;
	
	/* information for main activity (for reconnect) */
	public static final String RESULT_DATA_RESTARTED = "Restarted";
	
	private ScreenOrientationHandler mScreenOrientation;
	
	private InstallerService mInstaller = null;
	private boolean mDelayedInstallerListenerRegistration = false;
	
	private NativeBoincService mRunner = null;
	private boolean mDelayedRunnerListenerRegistration = false;
	
	private boolean mDoRestart = false;
	private boolean mAllowRemoteHosts;
	private boolean mAllowRemoteHostsDetermined = false;
	
	private ServiceConnection mInstallerConnection = new ServiceConnection() {

		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			if (Logging.DEBUG) Log.d(TAG, "on Installer connected");
			mInstaller = ((InstallerService.LocalBinder)service).getService();
			
			if (mDelayedInstallerListenerRegistration) {
				mInstaller.addInstallerListener(NativeClientActivity.this);
				mDelayedInstallerListenerRegistration = false;
			}
			
			updateProgressBarState();
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (Logging.DEBUG) Log.d(TAG, "on Installer disconnected");
			mInstaller = null;
		}
	};
	
	private ServiceConnection mRunnerConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			if (Logging.DEBUG) Log.d(TAG, "on Runner connected");
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
			
			if (mDelayedRunnerListenerRegistration) {
				mRunner.addNativeBoincListener(NativeClientActivity.this);
				mDelayedRunnerListenerRegistration = false;
			}
			
			updateProgressBarState();
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (Logging.DEBUG) Log.d(TAG, "on Runner disconnected");
			mRunner = null;
		}
	};
	
	private void bindRunnerService() {
		if (Logging.DEBUG) Log.d(TAG, "bind runner");
		bindService(new Intent(this, NativeBoincService.class), mRunnerConnection,
				BIND_AUTO_CREATE);
	}
	
	private void unbindRunnerService() {
		unbindService(mRunnerConnection);
		mRunner = null;
	}
	
	private void bindInstallerService() {
		if (Logging.DEBUG) Log.d(TAG, "bind installer");
		bindService(new Intent(this, InstallerService.class), mInstallerConnection,
				BIND_AUTO_CREATE);
	}
	
	private void unbindInstallerService() {
		unbindService(mInstallerConnection);
		mInstaller = null;
	}
	
	
	private static class SavedState {
		private final boolean doRestart;
		private final boolean allowRemoteHosts;
		private final boolean allowRemoteHostsDetermined;
		
		public SavedState(NativeClientActivity activity) {
			this.doRestart = activity.mDoRestart;
			this.allowRemoteHosts = activity.mAllowRemoteHosts;
			this.allowRemoteHostsDetermined = activity.mAllowRemoteHostsDetermined;
		}
		
		public void restore(NativeClientActivity activity) {
			activity.mDoRestart = this.doRestart;
			activity.mAllowRemoteHosts = this.allowRemoteHosts;
			activity.mAllowRemoteHostsDetermined = this.allowRemoteHostsDetermined;
		}
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		super.onCreate(savedInstanceState);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		bindRunnerService();
		bindInstallerService();
		
		mScreenOrientation = new ScreenOrientationHandler(this);
		addPreferencesFromResource(R.xml.nativeboinc);
		
		/* hostname preference */
		EditTextPreference editPref = (EditTextPreference)findPreference(PreferenceName.NATIVE_HOSTNAME);
		
		editPref.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
			@Override
			public boolean onPreferenceChange(Preference preference, Object newValue) {
				EditTextPreference pref = (EditTextPreference)preference;
				String newHostName = (String)newValue;
				if (newHostName != null && newHostName.length() != 0)
					pref.setSummary(getString(R.string.nativeHostnameSummaryCurrent)+
							": "+newHostName);
				else
					pref.setSummary(R.string.nativeHostnameSummaryNone);
				
				try {
					NativeBoincUtils.setHostname(NativeClientActivity.this, newHostName);
				} catch(IOException ex) { }
				
				mDoRestart = true;
				return true;
			}
		});
		
		/* allow remote client preference */
		CheckBoxPreference checkPref = (CheckBoxPreference)findPreference(
				PreferenceName.NATIVE_REMOTE_ACCESS);
		
		if (!mAllowRemoteHostsDetermined) {
			if (Logging.DEBUG) Log.d(TAG, "isAllowRemoteHosts (in start):"+checkPref.isChecked());
			mAllowRemoteHosts = checkPref.isChecked();
			mAllowRemoteHostsDetermined = true;
		}
	
		/* access password preference */
		editPref = (EditTextPreference)findPreference(PreferenceName.NATIVE_ACCESS_PASSWORD);
		
		editPref.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {

			@Override
			public boolean onPreferenceChange(Preference preference,
					Object newValue) {
				EditTextPreference pref = (EditTextPreference)preference;
				String oldPassword = null;
				try {
					oldPassword = NativeBoincUtils.getAccessPassword(NativeClientActivity.this);
				} catch(IOException ex) { }
				
				String newPassword = (String)newValue;
				pref.setSummary(getString(R.string.nativeAccessPasswordSummary)+" "+newPassword);
				
				if (!newPassword.equals(oldPassword)) { // if not same password
					Log.d(TAG, "In changing password");
					try {
						NativeBoincUtils.setAccessPassword(NativeClientActivity.this, newPassword);
					} catch(IOException ex) { }
					
					mDoRestart = true;
				}
				return true;
			}
		});
		
		/* host list preference */
		Preference pref = (Preference)findPreference(PreferenceName.NATIVE_HOST_LIST);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				startActivityForResult(new Intent(NativeClientActivity.this, AccessListActivity.class),
						ACTIVITY_ACCESS_LIST);
				return true;
			}
		});
		
		/* installed binaries preference */
		pref = (Preference)findPreference(PreferenceName.NATIVE_INSTALLED_BINARIES);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				startActivity(new Intent(NativeClientActivity.this, InstalledBinariesActivity.class));
				return true;
			}
		});
		
		/* dump boinc files */
		pref = (Preference)findPreference(PreferenceName.NATIVE_DUMP_BOINC_DIR);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				showDialog(DIALOG_ENTER_DUMP_DIRECTORY);
				return true;
			}
		});
		
		/* reinstall boinc */
		pref = (Preference)findPreference(PreferenceName.NATIVE_REINSTALL);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				showDialog(DIALOG_REINSTALL_QUESTION);
				return true;
			}
		});
		
		/* update binaries preference */
		pref = (Preference)findPreference(PreferenceName.NATIVE_UPDATE_BINARIES);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				startActivity(new Intent(NativeClientActivity.this, UpdateActivity.class));
				return true;
			}
		});
		
		/* show logs preference */
		pref = (Preference)findPreference(PreferenceName.NATIVE_SHOW_LOGS);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				startActivity(new Intent(NativeClientActivity.this, BoincLogsActivity.class));
				return true;
			}
		});
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	@Override
	protected void onPause() {
		if (mRunner != null) {
			if (Logging.DEBUG) Log.d(TAG, "Unregister runner listener");
			mRunner.removeNativeBoincListener(this);
			mDelayedRunnerListenerRegistration = false;
		}
		if (mInstaller != null) {
			if (Logging.DEBUG) Log.d(TAG, "Unregister installer listener");
			mInstaller.removeInstallerListener(this);
			mDelayedInstallerListenerRegistration = false;
		}
		super.onPause();
	}
	
	private void updateProgressBarState() {
		// display/hide progress
		boolean installerIsWorking = mInstaller != null &&
				mInstaller.isWorking();
		boolean runnerIsWorking = mRunner != null &&
				mRunner.serviceIsWorking();
		
		setProgressBarIndeterminateVisibility(installerIsWorking || runnerIsWorking);
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		String hostName = null;
		/* fetch hostname from boinc file */
		try {
			hostName = NativeBoincUtils.getHostname(this);
		} catch(IOException ex) { }
		
		EditTextPreference editPref = (EditTextPreference)findPreference(PreferenceName.NATIVE_HOSTNAME);
		
		/* update host name */
		if (hostName != null)
			editPref.setText(hostName);
		
		String text = editPref.getText();
		/* updating preferences summaries */
		if (text != null && text.length() != 0)
			editPref.setSummary(getString(R.string.nativeHostnameSummaryCurrent)+
					": "+editPref.getText());
		else
			editPref.setSummary(R.string.nativeHostnameSummaryNone);
		
		/* update access password */
		String accessPassword = null;
		try {
			accessPassword = NativeBoincUtils.getAccessPassword(this);
		} catch(IOException ex) { }
		
		editPref = (EditTextPreference)findPreference(PreferenceName.NATIVE_ACCESS_PASSWORD);
		editPref.setText(accessPassword);
		editPref.setSummary(getString(R.string.nativeAccessPasswordSummary)+" "+accessPassword);
		
		/* add listener */
		if (mRunner != null) {
			if (Logging.DEBUG) Log.d(TAG, "Normal register runner listener");
			mRunner.addNativeBoincListener(this);
			mDelayedRunnerListenerRegistration = false;
		} else
			mDelayedRunnerListenerRegistration = true;
	
		if (mInstaller != null) {
			if (Logging.DEBUG) Log.d(TAG, "Normal register installer listener");
			mInstaller.addInstallerListener(this);
			mDelayedInstallerListenerRegistration = false;
		} else
			mDelayedInstallerListenerRegistration = true;
		
		// progress bar update
		updateProgressBarState();
		
		mScreenOrientation.setOrientation();
	}
	
	@Override
	protected void onDestroy() {
		mScreenOrientation = null;
		unbindRunnerService();
		unbindInstallerService();
		super.onDestroy();
	}
	
	@Override
	public void onBackPressed() {
		if (isRestartRequired() && mRunner != null && mRunner.isRun()) {
			showDialog(DIALOG_APPLY_AFTER_RESTART);
		} else
			finish();
	}
	
	@Override
	public Dialog onCreateDialog(int dialogId, Bundle args) {
		switch(dialogId) {
		case DIALOG_ENTER_DUMP_DIRECTORY: {
			View view = LayoutInflater.from(this).inflate(R.layout.dialog_edit, null);
			final EditText edit = (EditText)view.findViewById(android.R.id.edit);
			edit.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
			
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_input_get)
				.setTitle(R.string.enterDumpDir)
				.setView(view)
				.setPositiveButton(R.string.ok, new Dialog.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						mInstaller.dumpBoincFiles(edit.getText().toString());
						
						startActivity(new Intent(NativeClientActivity.this, ProgressActivity.class));
					}
				})
				.setNegativeButton(R.string.cancel, null)
				.create();
		}
		case DIALOG_APPLY_AFTER_RESTART: {
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.warning)
				.setMessage(R.string.applyAfterRestart)
				.setPositiveButton(R.string.restart, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						if (mRunner != null) {
							mDoRestart = true;
							mRunner.restartClient();
							// finish activity with notification
							Intent data = new Intent();
							data.putExtra(RESULT_DATA_RESTARTED, true);
							setResult(RESULT_OK, data);
							finish();
						}
					}
				})
				.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						// finish activity
						finish();
					}
				})
				.setOnCancelListener(new DialogInterface.OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						// finish activity
						finish();
					}
				})
				.create();
		}
		case DIALOG_REINSTALL_QUESTION:
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.warning)
				.setMessage(R.string.reinstallQuestion)
				.setPositiveButton(R.string.yesText, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						mInstaller.reinstallBoinc();
						startActivity(new Intent(NativeClientActivity.this, ProgressActivity.class)); 
					}
				})
				.setNegativeButton(R.string.noText, null)
				.create();
		}
		return null;
	}
	
	@Override
	public void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == ACTIVITY_ACCESS_LIST && resultCode == RESULT_OK) {
			if (data != null && data.getBooleanExtra(AccessListActivity.RESULT_RESTARTED, false)) {
				if (Logging.DEBUG) Log.d(TAG, "on acces list finish: client restarted");
				mDoRestart = true;
			}
		}
	}
	
	/* check whether restart is required */
	private boolean isRestartRequired() {
		CheckBoxPreference checkPref = (CheckBoxPreference)findPreference(PreferenceName.NATIVE_REMOTE_ACCESS);
		// if do restart or new allowRemoteHost is different value
		return mDoRestart || (mAllowRemoteHosts != checkPref.isChecked());
	}

	@Override
	public void onNativeBoincClientError(String message) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		if ((mInstaller != null && mInstaller.isWorking()) || isWorking)
			setProgressBarIndeterminateVisibility(true);
		else if ((mInstaller == null || !mInstaller.isWorking()) && !isWorking)
			setProgressBarIndeterminateVisibility(false);
	}

	@Override
	public void onChangeInstallerIsWorking(boolean isWorking) {
		if ((mRunner != null && mRunner.serviceIsWorking()) || isWorking)
			setProgressBarIndeterminateVisibility(true);
		else if ((mRunner == null || !mRunner.serviceIsWorking()) && !isWorking)
			setProgressBarIndeterminateVisibility(false);
	}

	@Override
	public void onOperationError(String distribName, String errorMessage) {
		// TODO Auto-generated method stub
		
	}
}
