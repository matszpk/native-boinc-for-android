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


import java.util.List;

import hal.android.workarounds.FixedProgressDialog;
import edu.berkeley.boinc.lite.AccountIn;
import edu.berkeley.boinc.lite.ProjectConfig;
import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.ClientProjectReceiver;
import sk.boinc.nativeboinc.clientconnection.PollOp;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincAutoInstallListener;
import sk.boinc.nativeboinc.nativeclient.ProjectAutoInstallerState;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.ProgressState;
import sk.boinc.nativeboinc.util.ProjectItem;
import sk.boinc.nativeboinc.util.StandardDialogs;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class AddProjectActivity extends ServiceBoincActivity implements ClientProjectReceiver,
		NativeBoincAutoInstallListener {
	private static final String TAG = "AddProjectActivity";
	
	public static final String PARAM_ADD_FOR_NATIVE_CLIENT = "AddForNativeClient";
		
	private final static int DIALOG_TERMS_OF_USE = 1;
	private final static int DIALOG_ADD_PROJECT_PROGRESS = 2;
	
	private BoincManagerApplication mApp = null;
	
	private View mCreateAccountContainer = null;
	private View mUseAccountContainer = null;
	
	// create account tab
	private EditText mCreateNickname;
	private EditText mCreateEmail;
	private EditText mCreatePassword;
	private Button mConfirmButton;
	private TextView mMinPasswordText;
	
	// use account tab
	private EditText mUseEmail;
	private EditText mUsePassword;
	
	private ProjectItem mProjectItem = null;
	
	private boolean mAddProjectForNativeClient = false;
	
	private boolean mAccountCreationDisabled = false;
	private int mMinPasswordLength = 0;
	private String mTermsOfUse = "";
	private boolean mTermOfUseDisplayed = false;
	
	public boolean mDoAccountCreation = true;
	
	private int mProjectConfigProgressState = ProgressState.NOT_RUN;
	private boolean mAddingProjectInProgress = false;
	private boolean mAddingProjectSuccess = false; // if successfully finished
	// if other adding project works in the background
	private boolean mOtherAddingProjectInProgress = false;
	
	private boolean mToMarkProjectUrl = false; // if runner not connected when ok pressed
	private boolean mToUnmarkProjectUrl = false;
	
	private ClientId mConnectedClient = null;
	
	private static class SavedState {
	
		private final int mProjectConfigProgressState;
		private final boolean mAddingProjectInProgress;
		private final boolean mTermOfUseDisplayed;
		private final int mMinPasswordLength;
		private final boolean mOtherAddingProjectInProgress;
		private final boolean mAddingProjectSuccess;
		
		public SavedState(AddProjectActivity activity) {
			mProjectConfigProgressState = activity.mProjectConfigProgressState;
			mAddingProjectInProgress = activity.mAddingProjectInProgress;
			mOtherAddingProjectInProgress = activity.mOtherAddingProjectInProgress;
			mTermOfUseDisplayed = activity.mTermOfUseDisplayed;
			mMinPasswordLength = activity.mMinPasswordLength;
			mAddingProjectSuccess = activity.mAddingProjectSuccess;
		}
		
		public void restore(AddProjectActivity activity) {
			activity.mProjectConfigProgressState = mProjectConfigProgressState;
			activity.mAddingProjectInProgress = mAddingProjectInProgress;
			activity.mOtherAddingProjectInProgress = mOtherAddingProjectInProgress;
			activity.mTermOfUseDisplayed = mTermOfUseDisplayed;
			activity.mMinPasswordLength = mMinPasswordLength;
			activity.mAddingProjectSuccess = mAddingProjectSuccess;
		}
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		mAddProjectForNativeClient = getIntent().getBooleanExtra(PARAM_ADD_FOR_NATIVE_CLIENT, false);
		
		if (Logging.DEBUG) Log.d(TAG, "AddProject for native client");
		
		if (!mAddProjectForNativeClient)
			setUpService(true, true, false, false, false, false);
		else
			setUpService(true, true, true, true, false, false);
		
		mApp = (BoincManagerApplication)getApplication();
		
		SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		super.onCreate(savedInstanceState);
		setContentView(R.layout.add_project);
		
		mProjectItem = (ProjectItem)getIntent().getParcelableExtra(ProjectItem.TAG);
		
		if (mProjectItem.getName().length() != 0)
			setTitle(getString(R.string.addProject) + " " + mProjectItem.getName());
		else
			setTitle(getString(R.string.addProject) + " " + mProjectItem.getUrl());
		
		mCreateAccountContainer = findViewById(R.id.createAccountContainer);
		mUseAccountContainer = findViewById(R.id.useAccountContainer);
		
		mCreateNickname = (EditText)findViewById(R.id.create_name);
		mCreateEmail = (EditText)findViewById(R.id.create_email);
		mCreatePassword = (EditText)findViewById(R.id.create_password);
		mMinPasswordText = (TextView)findViewById(R.id.create_minPasswordLength);
		
		mUseEmail = (EditText)findViewById(R.id.use_email);
		mUsePassword = (EditText)findViewById(R.id.use_password);
		
		TextWatcher textWatcher = new TextWatcher() {
			@Override
			public void afterTextChanged(Editable s) {
				setConfirmButtonEnabled();
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
		mCreateNickname.addTextChangedListener(textWatcher);
		mCreateEmail.addTextChangedListener(textWatcher);
		mCreatePassword.addTextChangedListener(textWatcher);
		
		mUseEmail.addTextChangedListener(textWatcher);
		mUsePassword.addTextChangedListener(textWatcher);
		
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
		
		View.OnKeyListener onAfterEnterListener = new View.OnKeyListener() {
			@Override
			public boolean onKey(View v, int keyCode, KeyEvent event) {
				if (keyCode == KeyEvent.KEYCODE_ENTER && event.getAction() == KeyEvent.ACTION_DOWN) {
					doAddBoincProject();
					return true;
				}
				return false;
			}
		};
		
		mCreatePassword.setOnKeyListener(onAfterEnterListener);
		mUsePassword.setOnKeyListener(onAfterEnterListener);
		
		Button cancelButton = (Button)findViewById(R.id.addProjectCancel);
		cancelButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				onBackPressed();
			}
		});
		
		setConfirmButtonEnabled();
		
		mConfirmButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				doAddBoincProject();
			}
		});
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		if (mConnectionManager != null) {
			mConnectedClient = mConnectionManager.getClientId();
			Log.d(TAG, "onResume");
			if (!mAddProjectForNativeClient || mRunner != null)
				updateActivityState();
		}
	}
	
	@Override
	public void onBackPressed() {
		if (mConnectionManager != null)
			mConnectionManager.cancelPollOperations(PollOp.POLL_CREATE_ACCOUNT_MASK |
					PollOp.POLL_LOOKUP_ACCOUNT_MASK |
					PollOp.POLL_PROJECT_ATTACH_MASK |
					PollOp.POLL_PROJECT_CONFIG_MASK);
		
		if (mAddProjectForNativeClient && mRunner != null)
			mRunner.unmarkProjectUrlToInstall(mProjectItem.getUrl());
		
		setResult(RESULT_CANCELED);
		finish();
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	private void doAddBoincProject() {
		if (!isFormValid()) // if form is not valid
			return;
		
		if (mConnectionServiceConnection != null) {
			AccountIn accountIn = new AccountIn();
			accountIn.url = mProjectItem.getUrl();
			if (mDoAccountCreation) {
				accountIn.user_name = mCreateNickname.getText().toString();
				accountIn.email_addr = mCreateEmail.getText().toString();
				accountIn.passwd = mCreatePassword.getText().toString();
			} else {
				accountIn.email_addr = mUseEmail.getText().toString();
				accountIn.passwd = mUsePassword.getText().toString();
			}
			
			mAddingProjectInProgress = true;
			if (mConnectionManager != null) {
				if (mAddProjectForNativeClient)  {
					if (mRunner != null)
						mRunner.markProjectUrlToInstall(accountIn.url);
					else
						mToMarkProjectUrl = true;
				}
				if (mConnectionManager.addProject(accountIn, mDoAccountCreation))
					showDialog(DIALOG_ADD_PROJECT_PROGRESS);
				// do nothing if other works
			}
			setConfirmButtonEnabled();
		}
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
		
		mConnectionManager.handlePendingClientErrors(null, this);
		mConnectionManager.handlePendingPollErrors(null, this);
		
		if (mConnectedClient == null) {
			clientDisconnected(mConnectionManager.isDisconnectedByManager()); // if disconnected
			return; // if disconnected
		}
		
		/* some error can be handled, but this is not end */
		
		if (mProjectConfigProgressState != ProgressState.FINISHED) {
			if (mProjectConfigProgressState == ProgressState.NOT_RUN) {
				// do get project config (handles it)
				doGetProjectConfig();
			} else if (mProjectConfigProgressState == ProgressState.IN_PROGRESS) {
				ProjectConfig projectConfig = (ProjectConfig)mConnectionManager
						.getPendingOutput(BoincOp.GetProjectConfig);
				
				if (projectConfig != null) // if finish
					afterLoadingProjectConfig(projectConfig);
			}
			// if failed
		} else {
			// successfuly loaded
			Log.d(TAG, "Successfully loaded projectconfig");
			afterInitProjectConfig();
		}
		
		if (!mConnectionManager.isOpBeingExecuted(BoincOp.AddProject)) {
			if (mAddingProjectInProgress) {
				// its finally added
				afterProjectAdding(mProjectItem.getUrl());
			}
			mOtherAddingProjectInProgress = false;
		} else if (!mAddingProjectInProgress) // if other task works
			mOtherAddingProjectInProgress = true;
		setConfirmButtonEnabled();
		
		// handle runner autoinstall pendings
		if ((mAddingProjectSuccess || mAddingProjectInProgress) &&
				mAddProjectForNativeClient && mRunner.isMonitorWorks()) {
			int autoInstallerState = mRunner.getProjectStateForAutoInstaller(mProjectItem.getUrl());
			switch(autoInstallerState) {
			case ProjectAutoInstallerState.NOT_IN_AUTOINSTALLER:
				// if not in autoinstaller (after installation)
				mRunner.unmarkProjectUrlToInstall(mProjectItem.getUrl());
				setResult(RESULT_OK);
				finish();
				break;
			case ProjectAutoInstallerState.BEING_INSTALLED:
				// trigger event
				beginProjectInstallation(mProjectItem.getUrl());
				break;
			}
		}
	}
	
	@Override
	protected void onConnectionManagerConnected() {
		mConnectedClient = mConnectionManager.getClientId();
		Log.d(TAG, "onConnManagerConnected");
		if (!mAddProjectForNativeClient || mRunner != null)
			updateActivityState();
	}
	
	@Override
	protected void onRunnerConnected() {
		Log.d(TAG, "onConnRunnerConnected");
		if (mAddProjectForNativeClient && mConnectionManager != null)
			updateActivityState();
		if (mToMarkProjectUrl) {
			mRunner.markProjectUrlToInstall(mProjectItem.getUrl());
			mToMarkProjectUrl = false;
		}
		if (mToUnmarkProjectUrl) {
			mRunner.unmarkProjectUrlToInstall(mProjectItem.getUrl());
			mToUnmarkProjectUrl = false;
		}
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
			mCreateAccountContainer.setVisibility(View.VISIBLE);
			mUseAccountContainer.setVisibility(View.GONE);
		} else {
			mCreateAccountContainer.setVisibility(View.GONE);
			mUseAccountContainer.setVisibility(View.VISIBLE);
		}
		setConfirmButtonEnabled();
	}
	
	private boolean isFormValid() {
		if (mDoAccountCreation) {
			if (((mCreateNickname.getText().toString().length() == 0) ||
					(mCreateEmail.getText().length() == 0) || (mCreatePassword.getText().length() == 0) ||
					(mCreatePassword.getText().length() < mMinPasswordLength)))
				return false;
		} else {
			if ((mUseEmail.getText().length() == 0) || (mUsePassword.getText().length() == 0))
				return false;
		}
		
		return true;
	}
	
	public void setConfirmButtonEnabled() {
		mConfirmButton.setEnabled(mConnectedClient != null && isFormValid() &&
				!mAddingProjectInProgress && !mAddingProjectSuccess && !mOtherAddingProjectInProgress);
	}

	@Override
	public void clientConnectionProgress(BoincOp boincOp, int progress) {
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		mConnectedClient = mConnectionManager.getClientId();
	}

	private void tryToUnmarkProjectUrlToInstall() {
		if (mAddProjectForNativeClient) {
			if (mRunner != null)
				mRunner.unmarkProjectUrlToInstall(mProjectItem.getUrl());
			else
				mToUnmarkProjectUrl = true;
		}
	}
	
	@Override
	public void clientDisconnected(boolean disconnectedByManager) {
		if (Logging.WARNING) Log.w(TAG, "Client disconnected");
		
		// getProjectConfig - set as failed - prevents repeating operation
		mProjectConfigProgressState = ProgressState.FAILED;
		if (mAddingProjectInProgress)
			StandardDialogs.dismissDialog(this, DIALOG_ADD_PROJECT_PROGRESS);
		mAddingProjectInProgress = false;
		mOtherAddingProjectInProgress = false;
		
		tryToUnmarkProjectUrlToInstall();
		
		ClientId disconnectedHost = mConnectedClient;
		mConnectedClient = null;
		setConfirmButtonEnabled();
		StandardDialogs.tryShowDisconnectedErrorDialog(this, mConnectionManager, mRunner,
				disconnectedHost, disconnectedByManager);
	}

	@Override
	public boolean currentAuthCode(String projectUrl, String authCode) {
		return false;
	}

	private void doGetProjectConfig() {
		if (Logging.DEBUG) Log.d(TAG, "get project config from client");
		if (mConnectionManager.getProjectConfig(mProjectItem.getUrl()))
			mProjectConfigProgressState = ProgressState.IN_PROGRESS;
	}
	
	@Override
	public boolean currentProjectConfig(String projectUrl, ProjectConfig projectConfig) {
		Log.d(TAG, "Current projectconfig: "+mProjectConfigProgressState);
		if (mProjectConfigProgressState == ProgressState.NOT_RUN) {
			doGetProjectConfig();
			return true;
		}
		if (projectUrl.equals(mProjectItem.getUrl()))
			afterLoadingProjectConfig(projectConfig);
		return true;
	}

	private void afterProjectAdding(String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "After Project adding");
		
		// mark is successfuly finished, and next operation can perfomed
		mAddingProjectSuccess = true;
		
		mAddingProjectInProgress = false;
		
		if (!mAddProjectForNativeClient || !mRunner.isMonitorWorks()) {
			StandardDialogs.dismissDialog(this, DIALOG_ADD_PROJECT_PROGRESS);
			
			if (mAddProjectForNativeClient) { // go to progress activity
				// awaiting for application installation
				if (mApp.isInstallerRun())
					mApp.setInstallerStage(BoincManagerApplication.INSTALLER_PROJECT_INSTALLING_STAGE);
			}
			
			// do this when add project for normal client or client monitor doesnt work
			setResult(RESULT_OK);
			finish();
		}
	}
	
	@Override
	public boolean onAfterProjectAttach(String projectUrl) {
		if (mAddingProjectInProgress)
			afterProjectAdding(projectUrl);
		mOtherAddingProjectInProgress = false;
		setConfirmButtonEnabled();
		return true;
	}

	@Override
	public boolean onPollError(int errorNum, int operation, String errorMessage, String param) {
		if (operation != PollOp.POLL_CREATE_ACCOUNT && operation != PollOp.POLL_LOOKUP_ACCOUNT && 
			operation != PollOp.POLL_PROJECT_ATTACH && operation != PollOp.POLL_PROJECT_CONFIG)
			return false;
		
		if (Logging.WARNING) Log.w(TAG, "Poller error:"+errorNum+":"+operation+":"+param);
		
		// getProjectConfig - set as failed - prevents repeating operation
		if (operation == PollOp.POLL_PROJECT_CONFIG) {
			if (mProjectConfigProgressState == ProgressState.IN_PROGRESS)
				mProjectConfigProgressState = ProgressState.FAILED;
			else if (mProjectConfigProgressState == ProgressState.NOT_RUN)
				// if previous project config failed, try to get our project config
				doGetProjectConfig();
			
			if (Logging.DEBUG) Log.d(TAG, "configprogress:"+mProjectConfigProgressState);
		}
		
		if (operation == PollOp.POLL_PROJECT_ATTACH ||
				operation == PollOp.POLL_LOOKUP_ACCOUNT || operation == PollOp.POLL_CREATE_ACCOUNT) {
			if (mAddingProjectInProgress) {
				tryToUnmarkProjectUrlToInstall();
				
				StandardDialogs.dismissDialog(this, DIALOG_ADD_PROJECT_PROGRESS);
				mAddingProjectInProgress = false;
			}
			mOtherAddingProjectInProgress = false;
			setConfirmButtonEnabled();
		}
		
		StandardDialogs.showPollErrorDialog(this, errorNum, operation, errorMessage, param);
		return true;
	}
	
	@Override
	public void onPollCancel(int opFlags) {
		if (Logging.DEBUG) Log.d(TAG, "Poller cancel:"+opFlags);
		
		if ((opFlags & PollOp.POLL_PROJECT_CONFIG_MASK) != 0 &&
				mProjectConfigProgressState == ProgressState.NOT_RUN)
			doGetProjectConfig();
		
		if ((opFlags & (PollOp.POLL_PROJECT_ATTACH_MASK | PollOp.POLL_LOOKUP_ACCOUNT_MASK |
				PollOp.POLL_CREATE_ACCOUNT_MASK)) != 0) {
			if (mAddingProjectInProgress)
				tryToUnmarkProjectUrlToInstall();
			
			mOtherAddingProjectInProgress = false;
			setConfirmButtonEnabled();
		}
	}
	
	@Override
	public boolean clientError(BoincOp boincOp, int errorNum, String errorMessage) {
		if (!boincOp.isProjectConfig() && !boincOp.isAddProject())
			return false;
		
		if (boincOp.isProjectConfig()) {
			if (mProjectConfigProgressState == ProgressState.IN_PROGRESS)
				mProjectConfigProgressState = ProgressState.FAILED;
			else if (mProjectConfigProgressState == ProgressState.NOT_RUN)
				// if previous project config failed, try to get our project config
				doGetProjectConfig();
		}
		
		if (boincOp.isAddProject()) {
			if (mAddingProjectInProgress && boincOp.isAddProject()) {
				tryToUnmarkProjectUrlToInstall();
				
				StandardDialogs.dismissDialog(this, DIALOG_ADD_PROJECT_PROGRESS);
				mAddingProjectInProgress = false;
			}
			mOtherAddingProjectInProgress = false;
			setConfirmButtonEnabled();
		}
		
		StandardDialogs.showClientErrorDialog(this, errorNum, errorMessage);
		return true;
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}

	@Override
	public void beginProjectInstallation(String projectUrl) {
		/* if after adding project (project attach event) or if project in auto installer */
		boolean afterAddingProject = mAddingProjectSuccess || mAddingProjectInProgress;
		
		if (Logging.DEBUG) Log.d(TAG, "Project begin install:"+
				mAddProjectForNativeClient+","+afterAddingProject+":"+projectUrl); 
		if (mAddProjectForNativeClient && afterAddingProject &&
				projectUrl.equals(mProjectItem.getUrl())) {
			StandardDialogs.dismissDialog(this, DIALOG_ADD_PROJECT_PROGRESS);
			
			if (Logging.DEBUG) Log.d(TAG, "Project begin install, go to progress");
			mRunner.unmarkProjectUrlToInstall(mProjectItem.getUrl());
			
			Intent intent = new Intent();
			intent.putExtra(ProjectListActivity.RESULT_START_PROGRESS, true);
			setResult(RESULT_OK, intent);
			finish();
		}
	}

	@Override
	public void projectsNotFound(List<String> projectUrls) {
		if (!projectUrls.contains(mProjectItem.getUrl())) // if not in project not found
			return;
		
		boolean afterAddingProject = mAddingProjectSuccess || mAddingProjectInProgress;
		
		if (mAddProjectForNativeClient && afterAddingProject) {
			StandardDialogs.dismissDialog(this, DIALOG_ADD_PROJECT_PROGRESS);
			
			if (Logging.DEBUG) Log.d(TAG, "Project not found, we skip progress");
			mRunner.unmarkProjectUrlToInstall(mProjectItem.getUrl());
			
			setResult(RESULT_OK);
			finish();
		}
	}

	@Override
	public boolean onNativeBoincClientError(String message) {
		return false;
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
	}
}
