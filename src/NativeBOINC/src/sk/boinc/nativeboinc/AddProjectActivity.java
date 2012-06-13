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
import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.ClientProjectReceiver;
import sk.boinc.nativeboinc.clientconnection.PollOp;
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
public class AddProjectActivity extends ServiceBoincActivity implements ClientProjectReceiver {
	private static final String TAG = "AddProjectActivity";
		
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
	
	private boolean mAccountCreationDisabled = false;
	private int mMinPasswordLength = 0;
	private String mTermsOfUse = "";
	private boolean mTermOfUseDisplayed = false;
	
	public boolean mDoAccountCreation = true;
	
	private int mProjectConfigProgressState = ProgressState.NOT_RUN;
	private boolean mAddingProjectInProgress = false;
	// if other adding project works in the background
	private boolean mOtherAddingProjectInProgress = false;
	
	private ClientId mConnectedClient = null;
	
	private static class SavedState {
	
		private final int mProjectConfigProgressState;
		private final boolean mAddingProjectInProgress;
		private final boolean mTermOfUseDisplayed;
		private final int mMinPasswordLength;
		private final boolean mOtherAddingProjectInProgress;
		
		public SavedState(AddProjectActivity activity) {
			mProjectConfigProgressState = activity.mProjectConfigProgressState;
			mAddingProjectInProgress = activity.mAddingProjectInProgress;
			mOtherAddingProjectInProgress = activity.mOtherAddingProjectInProgress;
			mTermOfUseDisplayed = activity.mTermOfUseDisplayed;
			mMinPasswordLength = activity.mMinPasswordLength;
		}
		
		public void restore(AddProjectActivity activity) {
			activity.mProjectConfigProgressState = mProjectConfigProgressState;
			activity.mAddingProjectInProgress = mAddingProjectInProgress;
			activity.mOtherAddingProjectInProgress = mOtherAddingProjectInProgress;
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
		
		boolean isError = mConnectionManager.handlePendingClientErrors(null, this);
		isError |= mConnectionManager.handlePendingPollErrors(null, this);
		
		if (mConnectedClient == null) {
			clientDisconnected(mConnectionManager.isDisconnectedByManager()); // if disconnected
			isError = true;
		}
		
		if (isError) return;
	
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
				!mAddingProjectInProgress && !mOtherAddingProjectInProgress);
	}

	@Override
	public void clientConnectionProgress(BoincOp boincOp, int progress) {
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		mConnectedClient = mConnectionManager.getClientId();
	}

	@Override
	public void clientDisconnected(boolean disconnectedByManager) {
		if (Logging.WARNING) Log.w(TAG, "Client disconnected");
		
		// getProjectConfig - set as failed - prevents repeating operation
		mProjectConfigProgressState = ProgressState.FAILED;
		if (mAddingProjectInProgress)
			dismissDialog(DIALOG_ADD_PROJECT_PROGRESS);
		mAddingProjectInProgress = false;
		mOtherAddingProjectInProgress = false;
		
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
				dismissDialog(DIALOG_ADD_PROJECT_PROGRESS);
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
				dismissDialog(DIALOG_ADD_PROJECT_PROGRESS);
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
}
