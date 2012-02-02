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


import edu.berkeley.boinc.lite.AccountIn;
import edu.berkeley.boinc.lite.ProjectConfig;
import sk.boinc.nativeboinc.clientconnection.ClientProjectReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientReplyReceiver;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.AddProjectResult;
import sk.boinc.nativeboinc.util.ProjectItem;
import android.app.AlertDialog;
import android.app.Dialog;
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
	private final static int DIALOG_ERROR = 2;
	
	private BoincManagerApplication mApp = null;
	
	private LinearLayout mNicknameLayout;
	private EditText mNickname;
	private EditText mEmail;
	private EditText mPassword;
	private Button mConfirmButton;
	private TextView mMinPasswordText;
	
	private ProjectItem mProjectItem = null;
	private ProjectConfig mProjectConfig = null;
	
	private boolean mAccountCreationDisabled = false;
	//private int mMinPasswordLength;
	private String mTermsOfUse = "";
	private boolean mTermOfUseDisplayed = false;
	
	public boolean mDoAccountCreation = true;
	
	private final static String ARG_ERROR = "Error";
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		setUpService(true, true, false, false, false, false);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.add_project);
		
		mApp = (BoincManagerApplication)getApplication();
				
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
					
					mConnectionManager.addProject(AddProjectActivity.this, accountIn, 
							mDoAccountCreation);
				}
			}
		});
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			if (mApp.isInstallerRun())
			mApp.installerIsRun(false);
			finish();
			return true; 
		}
		return super.onKeyDown(keyCode, event);
	}

	private void afterLoadingProjectConfig() {
		if (Logging.DEBUG) Log.d(TAG, "After Loading Project config");
		mMinPasswordText.setText(getString(R.string.addProjectMinPassword) + ": " +
				mProjectConfig.min_passwd_length);
		
		if (mTermsOfUse.length() != 0 && !mTermOfUseDisplayed)
			showDialog(DIALOG_TERMS_OF_USE);
	}
	
	@Override
	protected void onConnectionManagerConnected() {
		mProjectConfig = mConnectionManager.getPendingProjectConfig(mProjectItem.getUrl());
		if (mProjectConfig == null) { // should be fetched
			if (Logging.DEBUG) Log.d(TAG, "get project config from client");
			mConnectionManager.getProjectConfig(this, mProjectItem.getUrl());
		} else
			afterLoadingProjectConfig();
	}
	
	@Override
	protected Dialog onCreateDialog(int dialogId, Bundle args) {
		View v;
		TextView text;
		switch(dialogId) {
		case DIALOG_TERMS_OF_USE: {
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
		}
		case DIALOG_ERROR: {
			if (dialogId==DIALOG_ERROR)
				return new AlertDialog.Builder(this)
					.setIcon(android.R.drawable.ic_dialog_alert)
					.setTitle(R.string.installError)
					.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
					.setNegativeButton(R.string.ok, null)
					.create();
		}
		}
		return null;
	}
	
	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		if (dialogId == DIALOG_ERROR) {
			TextView textView = (TextView)dialog.findViewById(R.id.dialogText);
			textView.setText(args.getString(ARG_ERROR));
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
		int minPasswordLength = 0;
		if (mProjectConfig != null)
			minPasswordLength = mProjectConfig.min_passwd_length;
		
		if (mDoAccountCreation && ((mNickname.getText().toString().length() == 0) ||
				(mPassword.getText().length() < minPasswordLength))) {
			mConfirmButton.setEnabled(false);
			return;
		}
		mConfirmButton.setEnabled(true);
	}

	@Override
	public boolean onPollError(int errorNum, int operation, String param) {
		if (Logging.WARNING) Log.w(TAG, "Poller error:"+errorNum+":"+operation+":"+param);
		setProgressBarIndeterminateVisibility(false);
		return true;
	}

	@Override
	public void clientConnectionProgress(int progress) {
		setProgressBarIndeterminateVisibility(true);
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		// TODO Auto-generated method stub
	}

	@Override
	public void clientDisconnected() {
		if (Logging.WARNING) Log.w(TAG, "Client disconnected");
		setProgressBarIndeterminateVisibility(false);
		
		Bundle args = new Bundle();
		args.putString(ARG_ERROR, getString(R.string.clientDisconnected));
		showDialog(DIALOG_ERROR, args);
	}

	@Override
	public boolean currentAuthCode(String authCode) {
		return false;
	}

	@Override
	public boolean currentProjectConfig(ProjectConfig projectConfig) {
		mProjectConfig = projectConfig;
		setProgressBarIndeterminateVisibility(false);
		afterLoadingProjectConfig();
		return true;
	}

	@Override
	public boolean onAfterProjectAttach() {
		setProgressBarIndeterminateVisibility(false);
		/* finish */
		finish();
		return true;
	}
}
