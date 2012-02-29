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

import sk.boinc.nativeboinc.installer.InstalledBinary;
import sk.boinc.nativeboinc.installer.InstallerService;
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
public class InstalledBinariesActivity extends ListActivity {
	
	private InstalledBinary[] mInstalledBinaries = null;
	
	private ServiceConnection mInstallerConn = new ServiceConnection() {
		@Override
		public void onServiceDisconnected(ComponentName name) {
			// do nothing
		}
		
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			InstallerService installer = ((InstallerService.LocalBinder)service).getService();
			
			/* synchronize before retrieving installed binaries */
			installer.synchronizeInstalledProjects();
			mInstalledBinaries = installer.getInstalledBinaries();
			
			getListView().setAdapter(new ArrayAdapter<InstalledBinary>(InstalledBinariesActivity.this,
					android.R.layout.simple_list_item_1, mInstalledBinaries));
		}
	};
	
	private void bindInstallerService() {
		bindService(new Intent(this, InstallerService.class), mInstallerConn, BIND_AUTO_CREATE);
	}
	
	private void unbindInstallerService() {
		unbindService(mInstallerConn);
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		bindInstallerService();
		
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
}
