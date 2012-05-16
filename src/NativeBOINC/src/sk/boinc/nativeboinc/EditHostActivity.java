/* 
 * AndroBOINC - BOINC Manager for Android
 * Copyright (C) 2010, Pavol Michalec
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

import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;


public class EditHostActivity extends AbstractBoincActivity {

	private EditText mNickname;
	private EditText mAddress;
	private EditText mPort;
	private EditText mPassword;
	private Button mConfirmButton;
	private Long mRowId;
	private HostListDbAdapter mDbHelper = null;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.edit_host);

		mDbHelper = new HostListDbAdapter(this);
		mDbHelper.open();

		mNickname = (EditText)findViewById(R.id.editHostNickname);
		mAddress = (EditText)findViewById(R.id.editHostAddress);
		mPort = (EditText)findViewById(R.id.editHostPort);
		mPassword = (EditText)findViewById(R.id.editHostPassword);

		mRowId = -1L;
		ClientId clientId = getIntent().getParcelableExtra(ClientId.TAG);
		if (clientId != null) {
			// Edit of existing host
			mRowId = clientId.getId();
			mNickname.setText(clientId.getNickname());
			mAddress.setText(clientId.getAddress());
			mPort.setText(Integer.toString(clientId.getPort()));
			mPassword.setText(clientId.getPassword());
		}
		else {
			// No input data - add new host
			// We just set default port
			mPort.setText(Integer.toString(BoincManagerApplication.DEFAULT_PORT));
		}
		TextWatcher textWatcher = new TextWatcher() {
			@Override
			public void afterTextChanged(Editable s) {
				mConfirmButton.setEnabled(isFormValid());
				//setConfirmButtonState();
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
		mAddress.addTextChangedListener(textWatcher);

		Button cancelButton = (Button)findViewById(R.id.editHostCancel);
		cancelButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				Intent intent = new Intent();
				setResult(RESULT_CANCELED, intent);
				finish();
			}
		});

		mPassword.setOnKeyListener(new View.OnKeyListener() {
			@Override
			public boolean onKey(View v, int keyCode, KeyEvent event) {
				if (keyCode == KeyEvent.KEYCODE_ENTER && event.getAction() == KeyEvent.ACTION_DOWN) {
					doAddHost();
					return true;
				}
				return false;
			}
		});
		
		mConfirmButton = (Button)findViewById(R.id.editHostOk);
		mConfirmButton.setEnabled(isFormValid());
		
		mConfirmButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				doAddHost();
			}
		});
	}

	@Override
	protected void onDestroy() {
		if (mDbHelper != null) {
			mDbHelper.close();
			mDbHelper = null;
		}
		super.onDestroy();
	}

	private void doAddHost() {
		if (!isFormValid())
			return;
		
		int port = BoincManagerApplication.DEFAULT_PORT;
		if (mPort.getText().length() > 0) {
			port = Integer.parseInt(mPort.getText().toString());
		}
		ClientId clientId = new ClientId(
				mRowId,
				mNickname.getText().toString(),
				mAddress.getText().toString(),
				port,
				mPassword.getText().toString()
				);
		Intent intent = new Intent().putExtra(ClientId.TAG, clientId);
		setResult(RESULT_OK, intent);
		finish();
	}
	
	private boolean isFormValid() {
		// Check that required fields are non-empty
		if ( (mNickname.getText().length() == 0) || (mAddress.getText().length() == 0) )
			// One of the required fields is empty, we must disable confirm button
			return false;
		// Check that mNickname is unique
		if (!mDbHelper.hostUnique(mRowId, mNickname.getText().toString()))
			return false;
		return true;
	}
}
