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
import edu.berkeley.boinc.lite.AccountIn;
import edu.berkeley.boinc.lite.ProjectConfig;
import sk.boinc.nativeboinc.clientconnection.ClientProjectReceiver;
import sk.boinc.nativeboinc.clientconnection.PollError;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.ProgressState;
import sk.boinc.nativeboinc.util.ProjectItem;
import sk.boinc.nativeboinc.util.StandardDialogs;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class AddProjectActivity extends ServiceBoincActivity implements ClientProjectReceiver {
	private static final String TAG = "AddProjectActivity";
		
	private final static int DIALOG_TERMS_OF_USE = 1;
	private final static int DIALOG_ADD_PROJECT_PROGRESS = 2;
	
	private BoincManagerApplication mApp = null;
	
	private LinearLayout mNicknameLayout;
	private EditText mNickname;
	private EditText mEmail;
	private EditText mPassword;
	private Button mConfirmButton;
	private TextView mMinPasswordText;
	
	private ProjectItem mProjectItem = null;
	
	private boolean mAccountCreationDisabled = false;
	private int mMinPasswordLength = 0;
	private String mTermsOfUse = "";
	private boolean mTermOfUseDisplayed = false;
	
	public boolean mDoAccountCreation = true;
	
	private int mProjectConfigProgressState = ProgressState.NOT_RUN;
	private boolean mAddingProjectInProgress = false;
	
	private ClientId mConnectedClient = null;
	
	private static class SavedState {
	
		private final int mProjectConfigProgressState;
		private final boolean mAddingProjectInProgress;
		private final boolean mTermOfUseDisplayed;
		private final int mMinPasswordLength;
		
		public SavedState(AddProjectActivity activity) {
			mProjectConfigProgressState = activity.mProjectConfigProgressState;
			mAddingProjectInProgress = activity.mAddingProjectInProgress;
			mTermOfUseDisplayed = activity.mTermOfUseDisplayed;
			mMinPasswordLength = activity.mMinPasswordLength;
		}
		
		public void restore(AddProjectActivity activity) {
			activity.mProjectConfigProgressState = mProjectConfigProgressState;
			activity.mAddingProjectInProgress = mAddingProjectInProgress;
			activity.mTermOfUseDisplayed = mTermOfUseDisplayed;
			activity.mMinPasswordLength = mMinPasswordLength;
		}
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		setUpService(true, true, false, false, false, false);
		
		mApp = (BoincManagerApplication)getApplication();
		
		SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		super.onCreate(savedInstanceState);
		setContentView(R.layout.add_project);
		
		mProjectItem = (ProjectItem)getIntent().getParcelableExtra(ProjectItem.TAG);
		
		setTitle(getString(R.string.addProject) + " " + mProjectItem.getName());
		
		mNicknameLayout = (LinearLayout)findViewById(R.id.addProjectNameLayout);
		mNickname = (EditText)findViewById(R.id.addProjectName);
		mEmail = (EditText)findViewById(R.id.addProjectEmail);
		mPassword = (EditText)findViewById(R.id.addProjectPassword);
		mMinPasswordText = (TextView)findViewById(R.id.addProjectMinPasswordLength);
		
		TextWatcher textWatcher = new TextWatcher() {
			@Override
			public void afterTextChanged(Editable s) {
				setConfirmButtonState();
			}
			@Override
			public void beforeTextChanged(CharSequence s, int start, int count, int after) {
				// Not needed
			}
			@Override
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				// Not needed
			}
		};
		mNickname.addTextChangedListener(textWatcher);
		mEmail.addTextChangedListener(textWatcher);
		mPassword.addTextChangedListener(textWatcher);
		
		mConfirmButton = (Button)findViewById(R.id.addProjectOk);
		
		RadioGroup viewMode = (RadioGroup)findViewById(R.id.addProjectViewMode);
		if (mAccountCreationDisabled) {
			/* hide radiobuttons for modes */
			viewMode.setVisibility(View.GONE);
			mDoAccountCreation = false;
			updateViewMode();
		}
		else {
			viewMode.setOnCheckedChangeListener(new OnCheckedChangeListener() {
				@Override
				public void onCheckedChanged(RadioGroup group, int checkedId) {
					if (checkedId == R.id.addProjectCreate) {
						mDoAccountCreation = true;
					} else if (checkedId == R.id.addProjectUse) {
						mDoAccountCreation = false;
					}
					updateViewMode();
				}
			});
		}
		
		Button cancelButton = (Button)findViewById(R.id.addProjectCancel);
		cancelButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				setResult(RESULT_CANCELED);
				finish();
			}
		});
		
		setConfirmButtonState();
		mConfirmButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				if (mConnectionServiceConnection != null) {
					AccountIn accountIn = new AccountIn();
					if (mDoAccountCreation)
						accountIn.user_name = mNickname.getText().toString();
					
					accountIn.email_addr = mEmail.getText().toString();
					accountIn.url = mProjectItem.getUrl();
					accountIn.passwd = mPassword.getText().toString();
					
					mAddingProjectInProgress = true;
					if (mConnectionManager != null) {
						mConnectionManager.addProject(accountIn, mDoAccountCreation);
						showDialog(DIALOG_ADD_PROJECT_PROGRESS);
					}
				}
			}
		});
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		if (mConnectionManager != null) {
			mConnectedClient = mConnectionManager.getClientId();
			Log.d(TAG, "onResume");
			updateActivityState();
		}
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	private void afterLoadingProjectConfig(ProjectConfig projectConfig) {
		if (Logging.DEBUG) Log.d(TAG, "After Loading Project config");
		mProjectConfigProgressState = ProgressState.FINISHED;
		mMinPasswordLength = projectConfig.min_passwd_length;
		mTermsOfUse = projectConfig.terms_of_use;
		
		afterInitProjectConfig();
	}
	
	private void afterInitProjectConfig() {
		mMinPasswordText.setText(getString(R.string.addProjectMinPassword) + ": " +
				mMinPasswordLength);
		
		if (mTermsOfUse.length() != 0 && !mTermOfUseDisplayed)
			showDialog(DIALOG_TERMS_OF_USE);
	}
	
	private void updateActivityState() {
		setProgressBarIndeterminateVisibility(mConnectionManager.isWorking());
		
		if (mProjectConfigProgressState == ProgressState.IN_PROGRESS || mAddingProjectInProgress) {
			PollError error = mConnectionManager.getPendingPollError(mProjectItem.getUrl());
			
			if (error != null) {
				handlePollError(error.param, error.errorNum, error.operation, error.message);
				return;
			} else if (mConnectedClient == null) {
				clientDisconnected(); // if disconnected
				return;
			}
		}
		
		if (mProjectConfigProgressState != ProgressState.FINISHED) {
			if (mProjectConfigProgressState == ProgressState.NOT_RUN) {
				if (Logging.DEBUG) Log.d(TAG, "get project config from client");
				mConnectionManager.getProjectConfig(mProjectItem.getUrl());
				mProjectConfigProgressState = ProgressState.IN_PROGRESS;
			} else if (mProjectConfigProgressState == ProgressState.IN_PROGRESS) {
				ProjectConfig projectConfig = mConnectionManager
						.getPendingProjectConfig(mProjectItem.getUrl());
				// if in still progress
				if (projectConfig != null) // still in progress
					afterLoadingProjectConfig(projectConfig);
			}
			// if failed
		} else {
			// successfuly loaded
			Log.d(TAG, "Successfully loaded projectconfig");
			afterInitProjectConfig();
		}
		
		if (mAddingProjectInProgress) {
			if (!mConnectionManager.isProjectBeingAdded(mProjectItem.getUrl())) {
				// its finally added
				afterProjectAdding(mProjectItem.getUrl());
			}
		}
	}
	
	@Override
	protected void onConnectionManagerConnected() {
		mConnectedClient = mConnectionManager.getClientId();
		Log.d(TAG, "onConnManagerConnected");
		updateActivityState();
	}
	
	@Override
	protected void onConnectionManagerDisconnected() {
		mConnectedClient = null;
		setProgressBarIndeterminateVisibility(false);
	}
	
	@Override
	protected Dialog onCreateDialog(int dialogId, Bundle args) {
		Dialog dialog = StandardDialogs.onCreateDialog(this, dialogId, args);
		if (dialog != null)
			return dialog;
		
		View v;
		TextView text;
		FixedProgressDialog progressDialog = null;
		switch (dialogId) {
		case DIALOG_TERMS_OF_USE:
			mTermOfUseDisplayed = true; // displayed
			
			v = LayoutInflater.from(this).inflate(R.layout.dialog, null);
			text = (TextView)v.findViewById(R.id.dialogText);
			text.setText(mTermsOfUse);
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_info)
				.setTitle(R.string.termsOfUse)
				.setView(v)
	    		.setNegativeButton(R.string.dismiss, null)
	    		.create();
		case DIALOG_ADD_PROJECT_PROGRESS:
			progressDialog = new FixedProgressDialog(this);
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
		
		if (dialogId == DIALOG_ADD_PROJECT_PROGRESS) {
			ProgressDialog progressDialog = (ProgressDialog)dialog;
			progressDialog.setMessage(getString(R.string.addingProject));
		}
	}
	
	private void updateViewMode() {
		if (mDoAccountCreation) {
			mNicknameLayout.setVisibility(View.VISIBLE);
			mMinPasswordText.setVisibility(View.VISIBLE);
		} else {
			mNicknameLayout.setVisibility(View.GONE);
			mMinPasswordText.setVisibility(View.GONE);
		}
		setConfirmButtonState();
	}
	
	private void setConfirmButtonState() {
		if ((mEmail.getText().length() == 0) || (mPassword.getText().length() == 0)) {
			mConfirmButton.setEnabled(false);
			return;
		}
		if (mDoAccountCreation && ((mNickname.getText().toString().length() == 0) ||
				(mPassword.getText().length() < mMinPasswordLength))) {
			mConfirmButton.setEnabled(false);
			return;
		}
		mConfirmButton.setEnabled(true);
	}
	
	private void handlePollError(String projectUrl, int errorNum, int operation,
			String errorMessage) {
		// getProjectConfig - set as finished - prevents repeating operation
		mProjectConfigProgressState = ProgressState.FINISHED;
		if (mAddingProjectInProgress)
			dismissDialog(DIALOG_ADD_PROJECT_PROGRESS);
		mAddingProjectInProgress = false;
		
		StandardDialogs.showPollErrorDialog(this, errorNum, operation, errorMessage, projectUrl);
	}

	@Override
	public boolean onPollError(int errorNum, int operation, String errorMessage, String param) {
		if (Logging.WARNING) Log.w(TAG, "Poller error:"+errorNum+":"+operation+":"+param);
		handlePollError(param, errorNum, operation, errorMessage);
		return true;
	}

	@Override
	public void clientConnectionProgress(int progress) {
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		mConnectedClient = mConnectionManager.getClientId();
	}

	@Override
	public void clientDisconnected() {
		if (Logging.WARNING) Log.w(TAG, "Client disconnected");
		
		// getProjectConfig - set as finished - prevents repeating operation
		mProjectConfigProgressState = ProgressState.FAILED;
		if (mAddingProjectInProgress)
			dismissDialog(DIALOG_ADD_PROJECT_PROGRESS);
		mAddingProjectInProgress = false;
		
		StandardDialogs.tryShowDisconnectedErrorDialog(this, mConnectionManager, mRunner,
				mConnectedClient);
		
		mConnectedClient = null;
	}

	@Override
	public boolean currentAuthCode(String projectUrl, String authCode) {
		return false;
	}

	@Override
	public boolean currentProjectConfig(ProjectConfig projectConfig) {
		if (projectConfig.master_url.equals(mProjectItem.getUrl()))
			afterLoadingProjectConfig(projectConfig);
		return true;
	}

	private void afterProjectAdding(String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "After Project adding");
		if (mAddingProjectInProgress)
			dismissDialog(DIALOG_ADD_PROJECT_PROGRESS);
		mAddingProjectInProgress = false;
		
		if (mConnectionManager.isNativeConnected()) { // go to progress activity
			// awaiting for application installation
			if (mApp.isInstallerRun())
				mApp.setInstallerStage(BoincManagerApplication.INSTALLER_PROJECT_INSTALLING_STAGE);
			setResult(RESULT_OK);
			finish(); // go to project list activity
		} else {
			setResult(RESULT_OK);
			finish();
		}
	}
	
	@Override
	public boolean onAfterProjectAttach(String projectUrl) {
		afterProjectAdding(projectUrl);
		return true;
	}

	@Override
	public boolean clientError(int errorNum, String errorMessage) {
		// getProjectConfig - set as finished - prevents repeating operation
		mProjectConfigProgressState = ProgressState.FAILED;
		
		if (mAddingProjectInProgress)
			dismissDialog(DIALOG_ADD_PROJECT_PROGRESS);
		mAddingProjectInProgress = false;
		
		StandardDialogs.showClientErrorDialog(this, errorNum, errorMessage);
		return true;
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}
}
