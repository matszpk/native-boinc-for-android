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

import sk.boinc.nativeboinc.util.BAMAccount;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

/**
 * @author mat
 *
 */
public class EditBAMActivity extends Activity {

	private EditText mName;
	private EditText mUrl;
	private EditText mPassword;
	private Button mConfirmButton;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.edit_bam);
		
		mName = (EditText)findViewById(R.id.editBAMName);
		mUrl = (EditText)findViewById(R.id.editBAMUrl);
		mPassword = (EditText)findViewById(R.id.editBAMPassword);
		
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
		mUrl.addTextChangedListener(textWatcher);
		
		mConfirmButton = (Button)findViewById(R.id.editBAMOk);
		
		BAMAccount bamAccount = getIntent().getParcelableExtra(BAMAccount.TAG);
		if (bamAccount != null) {
			mName.setText(bamAccount.getName());
			mUrl.setText(bamAccount.getUrl());
			mPassword.setText(bamAccount.getPassword());
		} else {
			String s = getString(R.string.editBAMDefaultUrl);
			mUrl.setText(s);
		}
		
		Button cancelButton = (Button)findViewById(R.id.editBAMCancel);
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
				BAMAccount bamAccount = new BAMAccount(mName.getText().toString(),
						mUrl.getText().toString(), mPassword.getText().toString());
				Intent intent = new Intent().putExtra(BAMAccount.TAG, bamAccount);
				setResult(RESULT_OK, intent);
				finish();
			}
		});
	}
	
	private void setConfirmButtonState() {
		// Check that required fields are non-empty
		if ((mName.getText().length() == 0) || (mUrl.getText().length() == 0)) {
			// One of the required fields is empty, we must disable confirm button
			mConfirmButton.setEnabled(false);
			return;
		}
		mConfirmButton.setEnabled(true);
	}
}
