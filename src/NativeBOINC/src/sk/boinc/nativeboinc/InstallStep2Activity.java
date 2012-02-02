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
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.Button;

/**
 * @author mat
 *
 */
public class InstallStep2Activity extends ServiceBoincActivity implements ClientReceiver {

	private final static String TAG = "InstallStep2Activity";
	
	private BoincManagerApplication mApp = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		setUpService(true, true, false, false, false, false);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.install_step2);
		
		mApp = (BoincManagerApplication)getApplication();
		
		Button nextButton = (Button)findViewById(R.id.installNext);
		Button cancelButton = (Button)findViewById(R.id.installCancel);
		
		cancelButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				finish();
			}
		});
		
		nextButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				
				Intent intent = new Intent(InstallStep2Activity.this, ProjectListActivity.class);
				intent.putExtra(ProjectListActivity.TAG_OTHER_PROJECT_OPTION, false);
				intent.putExtra(ProjectListActivity.TAG_FROM_INSTALLER, true);
				finish();
				startActivity(intent);
			}
		});
	}
	
	@Override
	protected void onConnectionManagerConnected() {
		HostListDbAdapter dbAdapter = null;
		try {
			dbAdapter = new HostListDbAdapter(this);
			dbAdapter.open();
			
			if (Logging.DEBUG) Log.d(TAG, "Connect with nativeboinc");
			
			ClientId client = dbAdapter.fetchHost("nativeboinc");
			mConnectionManager.connect(client, true);
			
			// starting connecting
			setProgressBarIndeterminateVisibility(true);
		} catch(NoConnectivityException ex) {
			// error
		} finally {
			dbAdapter.close();
		}
	} 
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			mApp.installerIsRun(false);
			finish();
			return true; 
		}
		return super.onKeyDown(keyCode, event);
	}

	@Override
	public void clientConnectionProgress(int progress) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		// if after connecting
		setProgressBarIndeterminateVisibility(false);
	}

	@Override
	public void clientDisconnected() {
		// if after try (failed)
		setProgressBarIndeterminateVisibility(false);
	}
}
