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

import edu.berkeley.boinc.lite.AccountMgrInfo;
import hal.android.workarounds.FixedProgressDialog;
import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.ClientAccountMgrReceiver;
import sk.boinc.nativeboinc.clientconnection.PollOp;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.BAMAccount;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.StandardDialogs;
import sk.boinc.nativeboinc.util.StringUtil;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;

/**
 * @author mat
 *
 */
public class EditBAMActivity extends ServiceBoincActivity implements ClientAccountMgrReceiver {

	private static final String TAG = "EditBAMActivity";
	
	private final static int DIALOG_CHANGE_BAM_PROGRESS = 1;
	
	private EditText mNameEdit;
	private EditText mUrlEdit;
	private EditText mPasswordEdit;
	private Button mConfirmButton;
	
	private ClientId mConnectedClient = null;
	private boolean mAttachBAMInProgress = false; // if 
	private boolean mOtherBAMOpInProgress = false;
	
	private BoincManagerApplication mApp = null;
	
	private static class SavedState {
		private boolean mAttachBAMInProgress = false; // if 
		private boolean mOtherBAMOpInProgress = false;
		
		public SavedState(EditBAMActivity activity) {
			mAttachBAMInProgress = activity.mAttachBAMInProgress;
			mOtherBAMOpInProgress = activity.mOtherBAMOpInProgress; 
		}
		
		public void restore(EditBAMActivity activity) {
			activity.mAttachBAMInProgress = mAttachBAMInProgress;
			activity.mOtherBAMOpInProgress = mOtherBAMOpInProgress;
		}
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		mApp = (BoincManagerApplication)getApplication();
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		setUpService(true, true, false, false, false, false);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.edit_bam);
		
		mNameEdit = (EditText)findViewById(R.id.editBAMName);
		mUrlEdit = (EditText)findViewById(R.id.editBAMUrl);
		mPasswordEdit = (EditText)findViewById(R.id.editBAMPassword);
		
		TextWatcher textWatcher = new TextWatcher() {
			@Override
			public void afterTextChanged(Editable s) {
				mConfirmButton.setEnabled(isFormValid());
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
		mNameEdit.addTextChangedListener(textWatcher);
		mUrlEdit.addTextChangedListener(textWatcher);
		
		mConfirmButton = (Button)findViewById(R.id.editBAMOk);
		
		BAMAccount bamAccount = getIntent().getParcelableExtra(BAMAccount.TAG);
		if (bamAccount != null) {
			mNameEdit.setText(bamAccount.getName());
			mUrlEdit.setText(bamAccount.getUrl());
			mPasswordEdit.setText(bamAccount.getPassword());
		} else {
			String s = getString(R.string.editBAMDefaultUrl);
			mUrlEdit.setText(s);
		}
		
		mPasswordEdit.setOnKeyListener(new View.OnKeyListener() {
			@Override
			public boolean onKey(View v, int keyCode, KeyEvent event) {
				if (keyCode == KeyEvent.KEYCODE_ENTER && event.getAction() == KeyEvent.ACTION_DOWN) {
					doAttachToBAM();
					return true;
				}
				return false;
			}
		});
		
		Button cancelButton = (Button)findViewById(R.id.editBAMCancel);
		cancelButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				Intent intent = new Intent();
				setResult(RESULT_CANCELED, intent);
				finish();
			}
		});
		
		mConfirmButton.setEnabled(isFormValid());
		mConfirmButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				doAttachToBAM();
			}
		});
	}
	
	private void doAttachToBAM() {
		if (!isFormValid()) // if not valid
			return;
		if (mConnectionManager == null)
			return;
		
		showDialog(DIALOG_CHANGE_BAM_PROGRESS);
		mAttachBAMInProgress = true;
		mConnectionManager.attachToBAM(mNameEdit.getText().toString(),
				StringUtil.normalizeHttpUrl(mUrlEdit.getText().toString()),
				mPasswordEdit.getText().toString());
	}
	
	private void updateActivityState() {
		if (mConnectionManager.isWorking())
			setProgressBarIndeterminateVisibility(true);
		else
			setProgressBarIndeterminateVisibility(false);
		
		boolean isError = mConnectionManager.handlePendingClientErrors(null, this);
		// get error for account mgr operations
		isError |= mConnectionManager.handlePendingPollErrors(null, this);
		if (mConnectedClient == null) {
			clientDisconnected(mConnectionManager.isDisconnectedByManager()); // if disconnected
			isError = true;
		}
		
		if (isError) return;
			
		if (!mConnectionManager.isOpBeingExecuted(BoincOp.SyncWithBAM)) {
			if (mAttachBAMInProgress)
				onAfterAccountMgrRPC();
			mOtherBAMOpInProgress = false;
		} else if (!mAttachBAMInProgress) // still is working
			mOtherBAMOpInProgress = true;
		
		setConfirmButtonEnabled();
	}
	
	@Override
	protected void onConnectionManagerConnected() {
		mConnectedClient = mConnectionManager.getClientId();
		updateActivityState();
	}
	
	@Override
	protected void onConnectionManagerDisconnected() {
		mConnectedClient = null;
		setProgressBarIndeterminateVisibility(false);
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		if (mConnectionManager != null) {
			mConnectedClient = mConnectionManager.getClientId();
			updateActivityState();
		}
	}
	
	private boolean isFormValid() {
		// Check that required fields are non-empty
		if((mNameEdit.getText().length() == 0) || (mUrlEdit.getText().length() == 0))
			// One of the required fields is empty, we must disable confirm button
			return false;
		return true;
	}
	
	private void setConfirmButtonEnabled() {
		mConfirmButton.setEnabled(mConnectedClient != null && isFormValid() &&
				!mAttachBAMInProgress && !mOtherBAMOpInProgress);
	}
	
	@Override
	public Dialog onCreateDialog(int dialogId, Bundle args) {
		Dialog dialog = StandardDialogs.onCreateDialog(this, dialogId, args);
		if (dialog != null)
			return dialog;
		
		if (dialogId == DIALOG_CHANGE_BAM_PROGRESS) {
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
		
		if (dialogId == DIALOG_CHANGE_BAM_PROGRESS) {
			ProgressDialog progressDialog = (ProgressDialog)dialog;
			progressDialog.setMessage(getString(R.string.attachBAMInProgress));
		}
	}
	
	@Override
	public boolean clientError(BoincOp boincOp, int errorNum, String errorMessage) {
		if (boincOp.isBAMOperation()) {
			if (mAttachBAMInProgress) {
				StandardDialogs.dismissDialog(this, DIALOG_CHANGE_BAM_PROGRESS);
				mAttachBAMInProgress = false;
			}
			mOtherBAMOpInProgress = false;
			setConfirmButtonEnabled();
			
			StandardDialogs.showClientErrorDialog(this, errorNum, errorMessage);
			return true;
		}
		return false;
	}

	@Override
	public void clientConnectionProgress(BoincOp boincOp, int progress) {
		// do nothing
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		// do nothing
	}

	@Override
	public void clientDisconnected(boolean disconnectedByManager) {
		if (mAttachBAMInProgress)
			StandardDialogs.dismissDialog(this, DIALOG_CHANGE_BAM_PROGRESS);
		if (Logging.DEBUG) Log.d(TAG, "disconnected");
		mAttachBAMInProgress = false;
		mOtherBAMOpInProgress = false;
		ClientId disconnectedHost = mConnectedClient;
		
		mConnectedClient = null; // used by setConfirmButtonEnabled
		setConfirmButtonEnabled();
		StandardDialogs.tryShowDisconnectedErrorDialog(this, mConnectionManager, null,
				disconnectedHost, disconnectedByManager);
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}

	@Override
	public boolean onPollError(int errorNum, int operation, String errorMessage, String param) {
		if (operation == PollOp.POLL_ATTACH_TO_BAM || operation == PollOp.POLL_SYNC_WITH_BAM) {
			if (mAttachBAMInProgress && operation == PollOp.POLL_ATTACH_TO_BAM) {
				StandardDialogs.dismissDialog(this, DIALOG_CHANGE_BAM_PROGRESS);
				mAttachBAMInProgress = false;
			}
			mOtherBAMOpInProgress = false;
			setConfirmButtonEnabled();
			
			StandardDialogs.showPollErrorDialog(this, errorNum, operation, errorMessage, param);
			return true;
		}
		return false;
	}
	
	@Override
	public void onPollCancel(int opFlags) {
		if (Logging.DEBUG) Log.d(TAG, "Poller cancel:"+opFlags);
		
		if ((opFlags & PollOp.POLL_BAM_OPERATION_MASK) != 0) {
			mOtherBAMOpInProgress = false;
			setConfirmButtonEnabled();
		}
	}

	@Override
	public boolean onAfterAccountMgrRPC() {
		if (Logging.DEBUG) Log.d(TAG, "on after account mgr rpc");
		if (mAttachBAMInProgress) {
			mAttachBAMInProgress = false;
			StandardDialogs.dismissDialog(this, DIALOG_CHANGE_BAM_PROGRESS);
			// if end
			finish();
			if (mApp.isInstallerRun()) {
				mApp.setInstallerStage(BoincManagerApplication.INSTALLER_FINISH_STAGE);
				// go to install finish activity
				startActivity(new Intent(this, InstallFinishActivity.class));
			}
		}
		mOtherBAMOpInProgress = false;
		setConfirmButtonEnabled();
		return true;
	}

	@Override
	public boolean currentBAMInfo(AccountMgrInfo accountMgrInfo) {
		return false;
	}
}
