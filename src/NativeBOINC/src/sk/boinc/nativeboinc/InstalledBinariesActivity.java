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

import edu.berkeley.boinc.nativeboinc.ClientEvent;
import sk.boinc.nativeboinc.installer.InstallOp;
import sk.boinc.nativeboinc.installer.InstalledBinary;
import sk.boinc.nativeboinc.installer.InstallerProgressListener;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.nativeclient.MonitorListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.util.StandardDialogs;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;


/**
 * @author mat
 *
 */
public class InstalledBinariesActivity extends ListActivity implements InstallerProgressListener,
	MonitorListener {
	
	@Override
	public int getInstallerChannelId() {
		return InstallerService.DEFAULT_CHANNEL_ID;
	}
	
	private InstalledBinary[] mInstalledBinaries = null;
		
	private InstallerService mInstaller = null;
	private NativeBoincService mRunner = null;
	
	private ServiceConnection mInstallerConn = new ServiceConnection() {
		@Override
		public void onServiceDisconnected(ComponentName name) {
			// do nothing
			if (mInstaller != null)
				mInstaller.removeInstallerListener(InstalledBinariesActivity.this);
			mInstaller = null;
		}
		
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mInstaller = ((InstallerService.LocalBinder)service).getService();
			mInstaller.addInstallerListener(InstalledBinariesActivity.this);
			
			updateInstalledBinaries();
		}
	};
	
	private ServiceConnection mRunnerConn = new ServiceConnection() {
		@Override
		public void onServiceDisconnected(ComponentName name) {
			// do nothing
			if (mRunner != null)
				mRunner.removeMonitorListener(InstalledBinariesActivity.this);
			mRunner = null;
		}
		
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
			mRunner.addMonitorListener(InstalledBinariesActivity.this);
		}
	};
	
	private void bindInstallerService() {
		bindService(new Intent(this, InstallerService.class), mInstallerConn, BIND_AUTO_CREATE);
	}
	
	private void unbindInstallerService() {
		mInstaller.removeInstallerListener(InstalledBinariesActivity.this);
		unbindService(mInstallerConn);
		mInstaller = null;
	}
	
	private void bindRunnerService() {
		bindService(new Intent(this, NativeBoincService.class), mRunnerConn, BIND_AUTO_CREATE);
	}
	
	private void unbindRunnerService() {
		mRunner.removeMonitorListener(InstalledBinariesActivity.this);
		unbindService(mRunnerConn);
		mRunner = null;
	}
	
	/* update installed list */
	private void updateInstalledBinaries() {
		if (mInstaller == null) return;
		/* synchronize before retrieving installed binaries */
		mInstaller.synchronizeInstalledProjects();
		mInstalledBinaries = mInstaller.getInstalledBinaries();
		
		getListView().setAdapter(new ArrayAdapter<InstalledBinary>(InstalledBinariesActivity.this,
				android.R.layout.simple_list_item_1, mInstalledBinaries));
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		bindInstallerService();
		bindRunnerService();
		
		getListView().setOnItemClickListener(new AdapterView.OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
				if (mInstalledBinaries == null)
					return;
				
				InstalledBinary binary = mInstalledBinaries[position];
				StandardDialogs.showDistribInfoDialog(InstalledBinariesActivity.this, binary.name,
						binary.version, binary.description, binary.changes);
			}
		});
	}
	
	@Override
	protected void onDestroy() {
		unbindInstallerService();
		unbindRunnerService();
		super.onDestroy();
	}
	
	@Override
	public Dialog onCreateDialog(int dialogId, Bundle args) {
		return StandardDialogs.onCreateDialog(this, dialogId, args);
	}

	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		StandardDialogs.onPrepareDialog(this, dialogId, dialog, args);
	}

	@Override
	public void onChangeInstallerIsWorking(boolean isWorking) {
		
	}

	@Override
	public boolean onOperationError(InstallOp installOp, String distribName,
			String errorMessage) {
		return false;
	}

	@Override
	public void onOperation(String distribName, String opDescription) {
		// do nothing
	}

	@Override
	public void onOperationProgress(String distribName, String opDescription,
			int progress) {
		// do nothing
	}

	@Override
	public void onOperationCancel(InstallOp installOp, String distribName) {
		// do nothing
	}

	@Override
	public void onOperationFinish(InstallOp installOp, String distribName) {
		// update when operation finished
		if (installOp.equals(InstallOp.ProgressOperation) &&
				distribName != null && distribName.length() != 0)
			updateInstalledBinaries();
	}

	@Override
	public void onMonitorEvent(ClientEvent event) {
		// update when project are detached
		if (event.type == ClientEvent.EVENT_DETACHED_PROJECT)
			updateInstalledBinaries();
	}

	@Override
	public void onMonitorDoesntWork() {
	}
}
