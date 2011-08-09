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

import sk.boinc.nativeboinc.util.AddProjectResult;
import sk.boinc.nativeboinc.util.ProjectConfigOptions;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
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
public class AddProjectActivity extends Activity {
	
	private final static int DIALOG_TERMS_OF_USE = 1;
	
	private LinearLayout mNameLayout;
	private EditText mName;
	private EditText mEmail;
	private EditText mPassword;
	private EditText mConfirmPassword;
	private LinearLayout mConfirmPasswordLayout;
	private Button mConfirmButton;
	private TextView mMinPasswordText;
	
	private boolean mAccountCreationDisabled = false;
	private int mMinPasswordLength;
	private String mTermsOfUse = "";
	
	public boolean mDoAccountCreation = true;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.add_project);
		
		ProjectConfigOptions projectConfig = getIntent().getParcelableExtra(ProjectConfigOptions.TAG);
		mMinPasswordLength = projectConfig.getMinPasswordLength();
		mAccountCreationDisabled = projectConfig.isAccountCreationDisabled() ||
			projectConfig.isClientAccountCreationDisabled();
		mTermsOfUse = projectConfig.getTermsOfUse();
		
		StringBuilder sB = new StringBuilder();
		sB.append(getString(R.string.addProject));
		sB.append(" ");
		sB.append(projectConfig.getName());
		setTitle(sB);
		
		mNameLayout = (LinearLayout)findViewById(R.id.addProjectNameLayout);
		mName = (EditText)findViewById(R.id.addProjectName);
		mEmail = (EditText)findViewById(R.id.addProjectEmail);
		mPassword = (EditText)findViewById(R.id.addProjectPassword);
		mConfirmPassword = (EditText)findViewById(R.id.addProjectConfirmPassword);
		mConfirmPasswordLayout = (LinearLayout)findViewById(R.id.addProjectConfirmPasswordLayout);
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
		mName.addTextChangedListener(textWatcher);
		mEmail.addTextChangedListener(textWatcher);
		mPassword.addTextChangedListener(textWatcher);
		mConfirmPassword.addTextChangedListener(textWatcher);
		mMinPasswordText.setText(getString(R.string.addProjectMinPassword)+mMinPasswordLength);
		
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
				Intent intent = new Intent();
				setResult(RESULT_CANCELED, intent);
				finish();
			}
		});
		
		setConfirmButtonState();
		mConfirmButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				AddProjectResult result = new AddProjectResult(
						mName.getText().toString(), mEmail.getText().toString(),
						mPassword.getText().toString(), mDoAccountCreation);
				Intent intent = new Intent().putExtra(AddProjectResult.TAG, result);
				setResult(RESULT_OK, intent);
				finish();
			}
		});
		
		if (mTermsOfUse.length() != 0) {
			showDialog(DIALOG_TERMS_OF_USE);
		}
	}
	
	@Override
	protected Dialog onCreateDialog(int id) {
		View v;
		TextView text;
		if (id == DIALOG_TERMS_OF_USE) {
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
		return null;
	}
	
	private void updateViewMode() {
		if (mDoAccountCreation) {
			mNameLayout.setVisibility(View.VISIBLE);
			mConfirmPasswordLayout.setVisibility(View.VISIBLE);
			mMinPasswordText.setVisibility(View.VISIBLE);
		} else {
			mNameLayout.setVisibility(View.GONE);
			mConfirmPasswordLayout.setVisibility(View.GONE);
			mMinPasswordText.setVisibility(View.GONE);
		}
		setConfirmButtonState();
	}
	
	private void setConfirmButtonState() {
		if ((mEmail.getText().length() == 0) || (mPassword.getText().length() == 0)) {
			mConfirmButton.setEnabled(false);
			return;
		}
		if (mDoAccountCreation && ((mName.getText().toString().length() == 0) ||
				(mPassword.getText().length() < mMinPasswordLength) ||
				!mConfirmPassword.getText().toString().equals(mPassword.getText().toString()))) {
			mConfirmButton.setEnabled(false);
			return;
		}
		mConfirmButton.setEnabled(true);
	}
}
