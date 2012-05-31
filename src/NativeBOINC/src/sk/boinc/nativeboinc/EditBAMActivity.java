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
import sk.boinc.nativeboinc.clientconnection.ClientAccountMgrReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientError;
import sk.boinc.nativeboinc.clientconnection.PollError;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.BAMAccount;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.StandardDialogs;
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
	
	private static final String STATE_ATTACH_BAM_PROGRESS = "AttachBAMProgress";
	
	private BoincManagerApplication mApp = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		mApp = (BoincManagerApplication)getApplication();
		
		if (savedInstanceState != null)
			mAttachBAMInProgress = savedInstanceState.getBoolean(STATE_ATTACH_BAM_PROGRESS);
		
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
		
		showDialog(DIALOG_CHANGE_BAM_PROGRESS);
		mAttachBAMInProgress = true;
		mConnectionManager.attachToBAM(mNameEdit.getText().toString(),
				mUrlEdit.getText().toString(), mPasswordEdit.getText().toString());
	}
	
	private void updateActivityState() {
		if (mConnectionManager.isWorking())
			setProgressBarIndeterminateVisibility(true);
		else
			setProgressBarIndeterminateVisibility(false);
		
		if (mAttachBAMInProgress) {
			ClientError cError = mConnectionManager.getPendingClientError();
			// get error for account mgr operations 
			PollError pollError = mConnectionManager.getPendingPollError("");
			if (cError != null)
				clientError(cError.errorNum, cError.message);
			else if (pollError != null)
				onPollError(pollError.errorNum, pollError.operation, pollError.message, pollError.param);

			if (mConnectedClient == null)
				clientDisconnected(); // if disconnected
			
			if (pollError != null || cError != null || mConnectedClient == null)
				return;
			
			if (!mConnectionManager.isBAMBeingSynchronized())
				onAfterAccountMgrRPC();
		}
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
	protected void onSaveInstanceState(Bundle outState) {
		super.onSaveInstanceState(outState);
		outState.putBoolean(STATE_ATTACH_BAM_PROGRESS, mAttachBAMInProgress);
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
	public boolean clientError(int errorNum, String errorMessage) {
		if (mAttachBAMInProgress)
			dismissDialog(DIALOG_CHANGE_BAM_PROGRESS);
		
		mAttachBAMInProgress = false;
		StandardDialogs.showClientErrorDialog(this, errorNum, errorMessage);
		return true;
	}

	@Override
	public void clientConnectionProgress(int progress) {
		// do nothing
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		// do nothing
	}

	@Override
	public void clientDisconnected() {
		if (mAttachBAMInProgress)
			dismissDialog(DIALOG_CHANGE_BAM_PROGRESS);
		if (Logging.DEBUG) Log.d(TAG, "disconnected");
		mAttachBAMInProgress = false;
		StandardDialogs.tryShowDisconnectedErrorDialog(this, mConnectionManager, null,
				mConnectedClient);
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}

	@Override
	public boolean onPollError(int errorNum, int operation,
			String errorMessage, String param) {
		if (mAttachBAMInProgress)
			dismissDialog(DIALOG_CHANGE_BAM_PROGRESS);
		
		mAttachBAMInProgress = false;
		StandardDialogs.showPollErrorDialog(this, errorNum, operation, errorMessage, param);
		return true;
	}

	@Override
	public boolean onAfterAccountMgrRPC() {
		if (Logging.DEBUG) Log.d(TAG, "on after account mgr rpc");
		if (mAttachBAMInProgress) {
			mAttachBAMInProgress = false;
			dismissDialog(DIALOG_CHANGE_BAM_PROGRESS);
			// if end
			finish();
			if (mApp.isInstallerRun()) {
				mApp.setInstallerStage(BoincManagerApplication.INSTALLER_FINISH_STAGE);
				// go to install finish activity
				startActivity(new Intent(this, InstallFinishActivity.class));
			}
			return true;
		}
		return false;
	}

	@Override
	public boolean currentBAMInfo(AccountMgrInfo accountMgrInfo) {
		// TODO Auto-generated method stub
		return false;
	}
}
