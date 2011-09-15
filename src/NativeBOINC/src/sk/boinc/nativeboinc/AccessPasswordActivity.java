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

import sk.boinc.nativeboinc.R.id;
import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.service.ConnectionManagerService;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

/**
 * @author mat
 *
 */
public class AccessPasswordActivity extends Activity implements NativeBoincListener {
	private final static String TAG = "AccessPasswordActivity";
	
	private EditText mAccessPassword;
	
	private ConnectionManagerService mConnectionManager = null;
	private NativeBoincService mRunner = null;
	
	private boolean mShutdownFromConfirm = false;
	
	private ClientId mClientId = null;
	
	private ServiceConnection mConnectionServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mConnectionManager = ((ConnectionManagerService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "onServiceConnected()");
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mConnectionManager = null;
			// This should not happen normally, because it's local service 
			// running in the same process...
			if (Logging.WARNING) Log.w(TAG, "onServiceDisconnected()");
			// We also reset client reference to prevent mess
		}
	};
	
	private ServiceConnection mRunnerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "installer.onServiceConnected()");
			mRunner.addNativeBoincListener(AccessPasswordActivity.this);
			
			TextView currentAccessPassword = (TextView)findViewById(id.currentAccessPassword);
			
			try {
				String accessPasswordString = mRunner.getAccessPassword();
				currentAccessPassword.setText(getString(R.string.installCurrentPassword) +
						": " + accessPasswordString);
			} catch(IOException ex) {
				finish();	// cancelling
			}
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mRunner = null;
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceDisconnected()");
		}
	};
	
	private void doBindConnectionManagerService() {
		bindService(new Intent(AccessPasswordActivity.this, ConnectionManagerService.class),
				mConnectionServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindConnectionManagerService() {
		unbindService(mConnectionServiceConnection);
		mConnectionManager = null;
	}
	
	private void doBindRunnerService() {
		bindService(new Intent(AccessPasswordActivity.this, NativeBoincService.class),
				mRunnerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindRunnerService() {
		unbindService(mRunnerServiceConnection);
		mRunner = null;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.access_password);
		
		doBindConnectionManagerService();
		doBindRunnerService();
		
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
	protected void onResume() {
		super.onResume();
		if (mConnectionManager == null)
			doBindConnectionManagerService();
		if (mRunner == null)
			doBindRunnerService();
	}
	
	@Override
	protected void onDestroy() {
		if (mRunner != null) {
			mRunner.removeNativeBoincListener(AccessPasswordActivity.this);
			mRunner = null;
		}
		if (mConnectionManager != null)
			mConnectionManager = null;
		
		super.onDestroy();
		
		doUnbindRunnerService();
		doUnbindConnectionManagerService();
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
				mRunner.startClient();
			}
		}
	}

	@Override
	public void onClientFirstStart() {
		// do nothing		
	}

	@Override
	public void onAfterClientFirstKill() {
		// do nothing
	}

	@Override
	public void onClientError(String message) {
		Toast.makeText(this, message, Toast.LENGTH_LONG).show();
		finish();
	}

	@Override
	public void onClientConfigured() {
		// do nothing
	}
}
