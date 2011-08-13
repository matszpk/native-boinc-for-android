/**
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
	    					mRunner.shutdownClient();
	    					ShutdownDialogActivity.this.finish();
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
