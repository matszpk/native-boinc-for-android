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
import sk.boinc.nativeboinc.installer.InstallOp;
import sk.boinc.nativeboinc.installer.InstallationOps;
import sk.boinc.nativeboinc.installer.InstallerProgressListener;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.nativeclient.AbstractNativeBoincListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUtils;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.ScreenOrientationHandler;
import sk.boinc.nativeboinc.util.StandardDialogs;
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
import android.preference.ListPreference;
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
		AbstractInstallerListener, InstallerProgressListener {

	private static final String TAG = "NativeClientActivity";
	
	private static final int ACTIVITY_ACCESS_LIST = 1;
	private static final int ACTIVITY_CONFIG_FILE = 2;
	private static final int ACTIVITY_BUG_CATCHER = 3;
	
	private static final int DIALOG_APPLY_AFTER_RESTART = 1;
	private static final int DIALOG_ENTER_DUMP_DIRECTORY = 2;
	private static final int DIALOG_REINSTALL_QUESTION = 3;
	private static final int DIALOG_ENTER_UPDATE_DIRECTORY = 4;
	private static final int DIALOG_DUMP_WARNING = 5;
	
	/* information for main activity (for reconnect) */
	public static final String RESULT_DATA_RESTARTED = "Restarted";
	
	private static final String DUMP_DIRECTORY_ARG = "DumpDir";
	
	private ScreenOrientationHandler mScreenOrientation;
	
	private InstallerService mInstaller = null;
	private boolean mDelayedInstallerListenerRegistration = false;
	
	private NativeBoincService mRunner = null;
	private boolean mDelayedRunnerListenerRegistration = false;
	
	private BoincManagerApplication mApp;
	
	private boolean mDoRestart = false;
	private boolean mAllowRemoteHosts;
	private boolean mAllowRemoteHostsDetermined = false;
	
	private String mOldHostname = null;
	
	@Override
	public int getInstallerChannelId() {
		return InstallerService.DEFAULT_CHANNEL_ID;
	}
	
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
			// update preferences
			updatePreferencesEnabled();
			// update show error dialog
			updateServicesError();
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (mInstaller != null)
				mInstaller.removeInstallerListener(NativeClientActivity.this);
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
			// update show error dialog
			updateServicesError();
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (mRunner != null)
				mRunner.removeNativeBoincListener(NativeClientActivity.this);
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
		if (mRunner != null)
			mRunner.removeNativeBoincListener(this);
		unbindService(mRunnerConnection);
		mRunner = null;
	}
	
	private void bindInstallerService() {
		if (Logging.DEBUG) Log.d(TAG, "bind installer");
		bindService(new Intent(this, InstallerService.class), mInstallerConnection,
				BIND_AUTO_CREATE);
	}
	
	private void unbindInstallerService() {
		if (mInstaller != null)
			mInstaller.removeInstallerListener(this);
		unbindService(mInstallerConnection);
		mInstaller = null;
	}
	
	
	private static class SavedState {
		private final String oldHostname;
		private final boolean doRestart;
		private final boolean allowRemoteHosts;
		private final boolean allowRemoteHostsDetermined;
		
		public SavedState(NativeClientActivity activity) {
			this.doRestart = activity.mDoRestart;
			this.oldHostname = activity.mOldHostname;
			this.allowRemoteHosts = activity.mAllowRemoteHosts;
			this.allowRemoteHostsDetermined = activity.mAllowRemoteHostsDetermined;
		}
		
		public void restore(NativeClientActivity activity) {
			activity.mDoRestart = this.doRestart;
			activity.mOldHostname = this.oldHostname;
			activity.mAllowRemoteHosts = this.allowRemoteHosts;
			activity.mAllowRemoteHostsDetermined = this.allowRemoteHostsDetermined;
		}
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		super.onCreate(savedInstanceState);
		
		mApp = (BoincManagerApplication)getApplication();
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		else { // if created
			/* fetch old hostname from boinc file */
			try {
				mOldHostname = NativeBoincUtils.getHostname(this);
			} catch(IOException ex) { }
			if (mOldHostname == null)
				mOldHostname = "";
		}
		
		bindRunnerService();
		bindInstallerService();
		
		mScreenOrientation = new ScreenOrientationHandler(this);
		addPreferencesFromResource(R.xml.nativeboinc);
		
		// Display latest news
		Preference pref = findPreference(PreferenceName.NATIVE_LATEST_NEWS);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				startActivity(new Intent(NativeClientActivity.this, NewsActivity.class));
				return true;
			}
		});
		
		/* native autostart */
		ListPreference listPref = (ListPreference)findPreference(PreferenceName.NATIVE_AUTOSTART);
		// disable all option for boinc client
		listPref.setEnabled(!mApp.isNoBoincInstallation());
		listPref.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
			
			@Override
			public boolean onPreferenceChange(Preference preference, Object newValue) {
				ListPreference pref = (ListPreference)preference;
				int index = pref.findIndexOfValue((String)newValue);
				CharSequence[] allDescriptions = pref.getEntries();
				pref.setSummary(allDescriptions[index]);
				return true;
			}
		});
		
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
				String oldPassword = null;
				try {
					oldPassword = NativeBoincUtils.getAccessPassword(NativeClientActivity.this);
				} catch(IOException ex) { }
				
				String newPassword = (String)newValue;
				
				if (!newPassword.equals(oldPassword) && newPassword.length() != 0) { // if not same password and not empty
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
		pref = (Preference)findPreference(PreferenceName.NATIVE_HOST_LIST);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				startActivityForResult(new Intent(NativeClientActivity.this, AccessListActivity.class),
						ACTIVITY_ACCESS_LIST);
				return true;
			}
		});
		
		/* move installation preference */
		pref = (Preference)findPreference(PreferenceName.NATIVE_MOVE_INSTALLATION);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				mInstaller.moveInstallationTo();
				mDoRestart = false;
				startActivity(new Intent(NativeClientActivity.this, ProgressActivity.class));
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
				if (mInstaller != null && !mInstaller.isBeingDumpedFiles())
					showDialog(DIALOG_ENTER_DUMP_DIRECTORY);
				return true;
			}
		});
		
		/* reinstall boinc */
		pref = (Preference)findPreference(PreferenceName.NATIVE_REINSTALL);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				if (mInstaller != null && !mInstaller.isBeingReinstalled())
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
		
		/* update binaries from sdcard */
		pref = (Preference)findPreference(PreferenceName.NATIVE_UPDATE_FROM_SDCARD);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				showDialog(DIALOG_ENTER_UPDATE_DIRECTORY);
				return true;
			}
		});
		
		/* delete project bins */
		pref = (Preference)findPreference(PreferenceName.NATIVE_DELETE_PROJ_BINS);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				startActivity(new Intent(NativeClientActivity.this, DeleteProjectBinsActivity.class));
				return true;
			}
		});
		
		/* show config file preference */
		pref = (Preference)findPreference(PreferenceName.NATIVE_CONFIG_FILE);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				startActivityForResult(new Intent(NativeClientActivity.this, ConfigFileActivity.class),
						ACTIVITY_CONFIG_FILE);
				return true;
			}
		});
		
		/* show bug catcher activity */
		pref = (Preference)findPreference(PreferenceName.NATIVE_BUG_CATCHER);
		pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
			@Override
			public boolean onPreferenceClick(Preference preference) {
				startActivityForResult(new Intent(NativeClientActivity.this, BugCatcherActivity.class),
						ACTIVITY_BUG_CATCHER);
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
	
	private void updateServicesError() {
		if (mRunner != null && mInstaller != null) {
			mRunner.handlePendingErrorMessage(this);
			mInstaller.handlePendingErrors(null, this);
		}
	}
	
	private void updatePreferencesEnabled() {
		if (mInstaller == null) return;
		
		// update enabled/disabled
		Preference pref = (Preference)findPreference(PreferenceName.NATIVE_DUMP_BOINC_DIR);
		pref.setEnabled(!mInstaller.isBeingDumpedFiles());
		
		pref = (Preference)findPreference(PreferenceName.NATIVE_REINSTALL);
		pref.setEnabled(!mInstaller.isBeingReinstalled());
		pref = findPreference(PreferenceName.NATIVE_MOVE_INSTALLATION);
		pref.setEnabled(!mInstaller.IsBeingInstallationMoved());
	}
	
	private void updateMoveToPreference() {
		Preference pref = findPreference(PreferenceName.NATIVE_MOVE_INSTALLATION);
		
		if (BoincManagerApplication.isSDCardInstallation(this)) { // if in sdcard
			pref.setTitle(R.string.nativeMoveToIntMemTitle);
			pref.setSummary(R.string.nativeMoveToIntMemSummary);
		} else { // if in internal memory
			pref.setTitle(R.string.nativeMoveToSDCardTitle);
			pref.setSummary(R.string.nativeMoveToSDCardSummary);
		}
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		/* native autostart */
		ListPreference listPref = (ListPreference)findPreference(PreferenceName.NATIVE_AUTOSTART);
		int index = listPref.findIndexOfValue((String)listPref.getValue());
		CharSequence[] allDescriptions = listPref.getEntries();
		listPref.setSummary(allDescriptions[index]);
		
		String hostName = null;
		/* fetch hostname from boinc file */
		try {
			hostName = NativeBoincUtils.getHostname(this);
		} catch(IOException ex) { }
		if (hostName == null)
			hostName = "";
		
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
		
		/* move installation */
		updateMoveToPreference();
		
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

		mScreenOrientation.setOrientation();
		
		updatePreferencesEnabled();
		// progress bar update
		updateProgressBarState();
		// update error dialogs
		updateServicesError();
	}
	
	@Override
	protected void onDestroy() {
		mScreenOrientation = null;
		mApp.resetRestartAfterReinstall();
		unbindRunnerService();
		unbindInstallerService();
		super.onDestroy();
	}
	
	@Override
	public void onBackPressed() {
		if (isRestartRequired() && mRunner != null && mRunner.isRun()) {
			showDialog(DIALOG_APPLY_AFTER_RESTART);
		} else {
			if (mApp.restartedAfterReinstall()) {
				// inform, that client restarted after reinstall
				Intent data = new Intent();
				data.putExtra(RESULT_DATA_RESTARTED, true);
				setResult(RESULT_OK, data);
				finish();
			}
			finish();
		}
	}
	
	@Override
	public Dialog onCreateDialog(int dialogId, Bundle args) {
		Dialog dialog = StandardDialogs.onCreateDialog(this, dialogId, args);
		if (dialog != null)
			return dialog;
		
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
						String dumpDir = edit.getText().toString();
						if (dumpDir.length() != 0) {
							if (!InstallationOps.isDestinationExists(dumpDir)) {
								mInstaller.dumpBoincFiles(dumpDir);
								startActivity(new Intent(NativeClientActivity.this, ProgressActivity.class));
							} else {
								Bundle dumpDialogArgs = new Bundle();
								dumpDialogArgs.putString(DUMP_DIRECTORY_ARG, dumpDir);
								showDialog(DIALOG_DUMP_WARNING, dumpDialogArgs);
							}
						}
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
						mDoRestart = false;
						startActivity(new Intent(NativeClientActivity.this, ProgressActivity.class)); 
					}
				})
				.setNegativeButton(R.string.noText, null)
				.create();
		case DIALOG_ENTER_UPDATE_DIRECTORY:
			View view = LayoutInflater.from(this).inflate(R.layout.dialog_edit, null);
			final EditText edit = (EditText)view.findViewById(android.R.id.edit);
			edit.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
			
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_input_get)
				.setTitle(R.string.enterUpdateDir)
				.setView(view)
				.setPositiveButton(R.string.ok, new Dialog.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						String updateDir = edit.getText().toString();
						
						if (updateDir.length() != 0) {
							Intent intent = new Intent(NativeClientActivity.this,
									UpdateFromSDCardActivity.class);
							intent.putExtra(UpdateFromSDCardActivity.UPDATE_DIR, updateDir);
							startActivity(intent);
						}
					}
				})
				.setNegativeButton(R.string.cancel, null)
				.create();
		case DIALOG_DUMP_WARNING:
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.warning)
				.setMessage("")	// this is trick for alert dialog
				.setPositiveButton(R.string.yesText, null)
				.setNegativeButton(R.string.noText, null)
				.create();
		}
		return null;
	}
	
	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		if (StandardDialogs.onPrepareDialog(this, dialogId, dialog, args))
			return; // if standard dialog
		
		EditText edit = null;
		switch (dialogId) {
		case DIALOG_ENTER_UPDATE_DIRECTORY:
		case DIALOG_ENTER_DUMP_DIRECTORY:
			edit = (EditText)dialog.findViewById(android.R.id.edit);
			edit.setText("");
			break;
		case DIALOG_DUMP_WARNING: {
			final String dumpDir = args.getString(DUMP_DIRECTORY_ARG);
			AlertDialog alertDialog = ((AlertDialog)dialog);
			alertDialog.setMessage(getString(R.string.dumpBoincWarn, dumpDir));
			alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, getString(R.string.yesText),
				new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						mInstaller.dumpBoincFiles(dumpDir);
						startActivity(new Intent(NativeClientActivity.this, ProgressActivity.class));
					}
				});
			break;
			}
		}
	}
	
	@Override
	public void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == ACTIVITY_BUG_CATCHER && resultCode == RESULT_OK) {
			if (data != null && data.getBooleanExtra(BugCatcherActivity.RESULT_RESTARTED, false)) {
				if (Logging.DEBUG) Log.d(TAG, "on bug catcher finish: client restarted");
				mDoRestart = true;
			}
		}
		if (requestCode == ACTIVITY_CONFIG_FILE && resultCode == RESULT_OK) {
			if (data != null && data.getBooleanExtra(ConfigFileActivity.RESULT_RESTARTED, false)) {
				if (Logging.DEBUG) Log.d(TAG, "on config file finish: client restarted");
				mDoRestart = true;
			}
		}
		if (requestCode == ACTIVITY_ACCESS_LIST && resultCode == RESULT_OK) {
			if (data != null && data.getBooleanExtra(AccessListActivity.RESULT_RESTARTED, false)) {
				if (Logging.DEBUG) Log.d(TAG, "on access list finish: client restarted");
				mDoRestart = true;
			}
		}
	}
	
	/* check whether restart is required */
	private boolean isRestartRequired() {
		CheckBoxPreference checkPref = (CheckBoxPreference)findPreference(PreferenceName.NATIVE_REMOTE_ACCESS);
		
		EditTextPreference hostnamePref = (EditTextPreference)findPreference(PreferenceName.NATIVE_HOSTNAME);
		
		// if do restart or new allowRemoteHost is different value
		return mDoRestart || (mAllowRemoteHosts != checkPref.isChecked()) ||
				!(mOldHostname.equals(hostnamePref.getText()));
	}

	@Override
	public boolean onNativeBoincClientError(String message) {
		StandardDialogs.showErrorDialog(this, message);
		return true;
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

	private void changePreferencesEnabled(String distribName) {
		if (distribName.equals(InstallerService.BOINC_DUMP_ITEM_NAME)) {
			Preference pref = (Preference)findPreference(PreferenceName.NATIVE_DUMP_BOINC_DIR);
			pref.setEnabled(true); // enable it
		} else if (distribName.equals(InstallerService.BOINC_REINSTALL_ITEM_NAME)) {
			Preference pref = (Preference)findPreference(PreferenceName.NATIVE_REINSTALL);
			pref.setEnabled(true); // enable it
		} else if (distribName.equals(InstallerService.BOINC_MOVETO_ITEM_NAME)) {
			Preference pref = (Preference)findPreference(PreferenceName.NATIVE_MOVE_INSTALLATION);
			pref.setEnabled(true); // enable it
			updateMoveToPreference();
		}
	}
	
	@Override
	public boolean onOperationError(InstallOp installOp, String distribName, String errorMessage) {
		if (!installOp.equals(InstallOp.ProgressOperation)) { // only simple operation
			// if global install error
			StandardDialogs.showInstallErrorDialog(this, distribName, errorMessage);
			return true;
		} else
			changePreferencesEnabled(distribName);
		return false;
	}
	
	@Override
	public void onOperation(String distribName, String opDescription) {
	}

	@Override
	public void onOperationProgress(String distribName, String opDescription,
			int progress) {
	}

	@Override
	public void onOperationCancel(InstallOp installOp, String distribName) {
		if (installOp.equals(InstallOp.ProgressOperation)) // only simple operation
			changePreferencesEnabled(distribName);
	}

	@Override
	public void onOperationFinish(InstallOp installOp, String distribName) {
		if (installOp.equals(InstallOp.ProgressOperation)) // only simple operation
			changePreferencesEnabled(distribName);
	}
}
