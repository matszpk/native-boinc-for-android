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

import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.nativeclient.NativeBoincStateListener;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

/**
 * @author mat
 *
 */
public class AccessPasswordActivity extends ServiceBoincActivity implements NativeBoincStateListener {
	private EditText mAccessPassword;
	
	private boolean mShutdownFromConfirm = false;
	
	private ClientId mClientId = null;
		
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		setUpService(true, false, true, true, false, false);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.access_password);
		
		mAccessPassword = (EditText)findViewById(R.id.accessPassword);
		Button confirmButton = (Button)findViewById(R.id.accessPasswordOk);
		
		confirmButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mAccessPassword.getText().length() != 0) {
					mShutdownFromConfirm = true;
					mRunner.shutdownClient();	// do shutdown
				}
			}
		});
		
		Button cancelButton = (Button)findViewById(R.id.accessPasswordCancel);
		
		cancelButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				finish();
			}
		});
	}
	
	@Override
	public void onClientStateChanged(boolean isRun) {
		if (isRun) {
			if (mShutdownFromConfirm) {
				// after start
				mShutdownFromConfirm = false;
				try {
					mConnectionManager.connect(mClientId, false);
				} catch(NoConnectivityException ex) { }
				finish();
			}
		} else {
			if (mShutdownFromConfirm) {
				// after shutdown
				HostListDbAdapter hostListDbHelper = new HostListDbAdapter(this);
				String accessPassword = mAccessPassword.getText().toString();
				try {
					mRunner.setAccessPassword(accessPassword);
				} catch(IOException ex) { }
				
				hostListDbHelper.open();
				mClientId = hostListDbHelper.fetchHost("nativeboinc");
				if (mClientId != null) {
					mClientId.setPassword(accessPassword);
					hostListDbHelper.updateHost(mClientId);
				}
				hostListDbHelper.close();
				
				// restart client
				mRunner.startClient(false);
			}
		}
	}

	@Override
	public void onNativeBoincError(String message) {
		Toast.makeText(this, message, Toast.LENGTH_LONG).show();
		finish();
	}
}
