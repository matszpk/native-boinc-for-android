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

import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;

/**
 * @author mat
 *
 */
public class ShutdownDialogActivity extends Activity {
	
	private static final int DIALOG_SHUTDOWN_ASK = 1;
	
	private NativeBoincService mRunner = null;
	
	private ServiceConnection mRunnerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
		}
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			mRunner = null;
		}
	};
	
	private void doBindRunnerService() {
		bindService(new Intent(ShutdownDialogActivity.this, NativeBoincService.class),
				mRunnerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindRunnerService() {
		unbindService(mRunnerServiceConnection);
		mRunner = null;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		doBindRunnerService();
		
		showDialog(DIALOG_SHUTDOWN_ASK);
	}
	
	
	@Override
	protected void onResume() {
		super.onResume();
		int screenSize = getResources().getConfiguration().screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK;
		if (Build.VERSION.SDK_INT < 11 || screenSize != 4)
			// only when is not honeycomb or screenSize is not xlarge
			setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
	}
	
	@Override
	protected void onDestroy() {
		
		super.onDestroy();
		if (mRunner != null) {
			mRunner = null;
		}
		doUnbindRunnerService();
	}
	
	@Override
	protected Dialog onCreateDialog(int id) {
		if (id == DIALOG_SHUTDOWN_ASK) {
			return new AlertDialog.Builder(this)
	    		.setIcon(android.R.drawable.ic_dialog_alert)
	    		.setTitle(R.string.warning)
	    		.setMessage(R.string.shutdownAskText)
	    		.setPositiveButton(R.string.shutdown,
	    			new DialogInterface.OnClickListener() {
	    				public void onClick(DialogInterface dialog, int whichButton) {
	    					if (mRunner != null) {
		    					mRunner.shutdownClient();
		    					ShutdownDialogActivity.this.finish();
	    					}
	    				}
	    			})
	    		.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						ShutdownDialogActivity.this.finish();
					}
				})
				.setOnCancelListener(new DialogInterface.OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						ShutdownDialogActivity.this.finish();
					}
				})
	    		.create();
		}
		return null;
	}
}
