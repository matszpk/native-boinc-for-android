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

import sk.boinc.nativeboinc.clientconnection.ClientReceiver;
import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincStateListener;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import sk.boinc.nativeboinc.util.StandardDialogs;
import android.app.Dialog;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.Button;

/**
 * @author mat
 *
 */
public class InstallStep2Activity extends ServiceBoincActivity implements ClientReceiver,
		NativeBoincStateListener {

	private final static String TAG = "InstallStep2Activity";
	
	private static final String STATE_CONNECTION_FAILED = "ConnFailed";
	
	private BoincManagerApplication mApp = null;
	
	private Button mNextButton = null;
	
	private boolean mConnectionFailed = false;
	private ClientId mConnectedClient = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		if (savedInstanceState != null)
			mConnectionFailed = savedInstanceState.getBoolean(STATE_CONNECTION_FAILED);
		
		mApp = (BoincManagerApplication)getApplication();
		
		setUpService(true, true, true, true, false, false);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.install_step2);
		
		mNextButton = (Button)findViewById(R.id.installNext);
		Button cancelButton = (Button)findViewById(R.id.installCancel);
		
		cancelButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				InstallStep2Activity.this.onBackPressed();
			}
		});
		
		mNextButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				startActivity(new Intent(InstallStep2Activity.this, ProjectListActivity.class));
			}
		});
	}
	
	@Override
	protected void onSaveInstanceState(Bundle outState) {
		outState.putBoolean(STATE_CONNECTION_FAILED, mConnectionFailed);
		super.onSaveInstanceState(outState);
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		if (Logging.DEBUG) Log.d(TAG, "onResume: "+mApp.getInstallerStage());
		
		if (!mApp.isInstallerRun()) {
			// installer finished then close it
			finish();
			return;
		}
		
		if (mRunner != null && !mConnectionFailed) {
			if (!mRunner.isRun()) {
				setProgressBarIndeterminateVisibility(true);
				mRunner.startClient(false);
			} else if (mConnectionManager != null)	// now try to connect
				connectWithNativeClient();
		}
	}
	
	private void connectWithNativeClient() {
		HostListDbAdapter dbAdapter = null;
		try {
			dbAdapter = new HostListDbAdapter(this);
			dbAdapter.open();
			
			if (Logging.DEBUG) Log.d(TAG, "Connect with nativeboinc");
			
			ClientId client = dbAdapter.fetchHost("nativeboinc");
			ClientId connected = mConnectionManager.getClientId();
			
			if (connected == null || !client.equals(connected)) {
				// if not connected
				mConnectionManager.connect(client, true);
				// starting connecting
				setProgressBarIndeterminateVisibility(true);
			}
		} catch(NoConnectivityException ex) {
			// error
		} finally {
			dbAdapter.close();
		}
	}
	
	@Override
	protected void onConnectionManagerConnected() {
		mConnectedClient = mConnectionManager.getClientId();
		if (mRunner != null && mRunner.isRun() && !mConnectionFailed)
			connectWithNativeClient();
	}
	
	@Override
	protected void onConnectionManagerDisconnected() {
		mConnectedClient = null;
		setProgressBarIndeterminateVisibility(false);
	}
	
	@Override
	protected void onRunnerConnected() {
		if (!mConnectionFailed) {
			if (!mRunner.isRun()) {
				setProgressBarIndeterminateVisibility(true);
				mRunner.startClient(false);
			} else if (mConnectionManager != null)	// now try to connect
				connectWithNativeClient();
		}
	}
	
	@Override
	protected void onRunnerDisconnected() {
		setProgressBarIndeterminateVisibility(false);
	}
	
	
	@Override
	public void onBackPressed() {
		finish();
		startActivity(new Intent(this, InstallFinishActivity.class));
	}
	
	@Override
	protected Dialog onCreateDialog(int dialogId, Bundle args) {
		return StandardDialogs.onCreateDialog(this, dialogId, args);
	}
	
	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		StandardDialogs.onPrepareDialog(this, dialogId, dialog, args);
	}

	@Override
	public void clientConnectionProgress(int progress) {
		setProgressBarIndeterminateVisibility(true);
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		// if after connecting
		setProgressBarIndeterminateVisibility(false);
		mNextButton.setEnabled(true);
		mConnectedClient = mConnectionManager.getClientId();
	}

	@Override
	public void clientDisconnected() {
		if (Logging.WARNING) Log.w(TAG, "Client disconnected");
		// if after try (failed)
		setProgressBarIndeterminateVisibility(false);

		StandardDialogs.tryShowDisconnectedErrorDialog(this, mConnectionManager, mRunner,
				mConnectedClient);
		
		mConnectedClient = null;
	}

	@Override
	public void onNativeBoincClientError(String message) {
		setProgressBarIndeterminateVisibility(false);
		mConnectionFailed = true;
		StandardDialogs.showErrorDialog(this, message);
	}

	@Override
	public void onClientStart() {
		if (mConnectionManager != null)
			connectWithNativeClient();
	}
	
	@Override
	public void onClientStop(int exitCode, boolean stoppedByManager) {
		// do nothing
	}

	@Override
	public boolean clientError(int errorNum, String message) {
		setProgressBarIndeterminateVisibility(false);
		mConnectionFailed = true;
		StandardDialogs.showClientErrorDialog(this, errorNum, message);
		return true;
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		
	}
}
