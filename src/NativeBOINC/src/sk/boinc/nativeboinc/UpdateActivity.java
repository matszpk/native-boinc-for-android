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

import java.util.Vector;

import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.ClientDistrib;
import sk.boinc.nativeboinc.nativeclient.CpuType;
import sk.boinc.nativeboinc.nativeclient.InstallerListener;
import sk.boinc.nativeboinc.nativeclient.InstallerService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.ProjectDistrib;
import sk.boinc.nativeboinc.service.ConnectionManagerService;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import sk.boinc.nativeboinc.util.UpdateItem;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

/**
 * @author mat
 *
 */
public class UpdateActivity extends Activity implements NativeBoincListener, InstallerListener {
	private static final String TAG = "UpdateActivity";
	
	public static final String ATTACHED_PROJECT_URLS_TAG = "AttachedProjectUrlList";
	
	private TextView mAvailableText;
	private ListView mBinariesList;
	
	private UpdateItem[] mUpdateItems = null;
	private String[] mAttachedProjectUrls = null;
	private int mCurrentlyInstalledItem = 0;
	private boolean mUpdateStarts = false;
	
	private ProgressDialog mProgressDialog = null;
	
	private ConnectionManagerService mConnectionManager = null;
	
	private ServiceConnection mConnectionServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mConnectionManager = ((ConnectionManagerService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "onServiceConnected()");
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mConnectionManager = null;
			if (Logging.WARNING) Log.w(TAG, "onServiceDisconnected()");
		}
	};
	
	private InstallerService mInstaller = null;
	
	private ServiceConnection mInstallerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mInstaller = ((InstallerService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "installer.onServiceConnected()");
			mInstaller.addInstallerListener(UpdateActivity.this);
			
			/* */
			mInstaller.synchronizeInstalledProjects();
			mInstaller.updateClientDistribList();
		}
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			mInstaller = null;
			if (Logging.DEBUG) Log.d(TAG, "installer.onServiceDisconnected()");
		}
	};
	
	private NativeBoincService mRunner = null;
	
	private ServiceConnection mRunnerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceConnected()");
			mRunner.addNativeBoincListener(UpdateActivity.this);
		}
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			mRunner = null;
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceDisconnected()");
		}
	};
	
	private void doBindConnectionManagerService() {
		bindService(new Intent(UpdateActivity.this, ConnectionManagerService.class),
				mConnectionServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindConnectionManagerService() {
		unbindService(mConnectionServiceConnection);
		mConnectionManager = null;
	}
	
	private void doBindInstallerService() {
		bindService(new Intent(UpdateActivity.this, InstallerService.class),
				mInstallerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindInstallerService() {
		unbindService(mInstallerServiceConnection);
		mInstaller = null;
	}
	
	private void doBindRunnerService() {
		bindService(new Intent(UpdateActivity.this, NativeBoincService.class),
				mRunnerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindRunnerService() {
		unbindService(mRunnerServiceConnection);
		mRunner = null;
	}
	
	private class UpdateListAdapter extends BaseAdapter {

		private Context mContext;
		
		public UpdateListAdapter(Context context) {
			mContext = context;
		}
		
		@Override
		public int getCount() {
			if (mUpdateItems == null)
				return 0;
			return mUpdateItems.length;
		}

		@Override
		public Object getItem(int position) {
			return mUpdateItems[position];
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(final int position, View inView, ViewGroup parent) {
			View view = inView;
			if (view == null) {
				LayoutInflater inflater = (LayoutInflater)mContext.getSystemService(LAYOUT_INFLATER_SERVICE);
				view = inflater.inflate(R.layout.update_list_item, null);
			}
			
			TextView nameText = (TextView)view.findViewById(R.id.updateItemName);
			TextView cpuTypeText = (TextView)view.findViewById(R.id.updateItemCpuType);
			final CheckBox checkBox = (CheckBox)view.findViewById(R.id.updateItemCheckbox);
			
			final UpdateItem item = mUpdateItems[position];
			nameText.setText(item.name + " " + item.version);
			cpuTypeText.setText("(" + CpuType.getCpuDisplayName(getResources(), item.cpuType) + ")");
			
			checkBox.setChecked(item.checked);
			checkBox.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					item.checked = !item.checked;
				}
			});
			
			view.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					item.checked = !item.checked;
					checkBox.setChecked(item.checked);
				}
			});
			
			return view;
		}
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.update);
		
		doBindConnectionManagerService();
		doBindInstallerService();
		doBindRunnerService();
		
		mAvailableText = (TextView)findViewById(R.id.updateAvailableText);
		mBinariesList = (ListView)findViewById(R.id.updateBinariesList);
		
		mAttachedProjectUrls = getIntent().getStringArrayExtra(ATTACHED_PROJECT_URLS_TAG);
		
		Button confirmButton = (Button)findViewById(R.id.updateOk);
		confirmButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				mBinariesList.setEnabled(false);
				
				int count = 0;
				for (int i = 0; i < mUpdateItems.length; i++)
					if (mUpdateItems[i].checked)
						count++;
				/* do update */
				if (count != 0) {
					mUpdateStarts = true;
					mRunner.shutdownClient();
				}
				else
					UpdateActivity.this.finish();
			}
		});
		
		Button cancelButton = (Button)findViewById(R.id.updateCancel);
		cancelButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				mInstaller.cancelOperation();
				finish();
			}
		});
	}
	
	@Override
	public void onResume() {
		super.onResume();
		if (mInstaller == null)
			doBindInstallerService();
		if (mRunner == null)
			doBindRunnerService();
		if (mConnectionManager == null)
			doBindConnectionManagerService();
	}
	
	@Override
	public void onDestroy() {
		super.onDestroy();
		if (mRunner != null) {
			mRunner.removeNativeBoincListener(this);
			mRunner = null;
		}
		
		if (mInstaller != null) {
			mInstaller.removeInstallerListener(this);
			mInstaller = null;
		}
			
		doUnbindRunnerService();
		doUnbindInstallerService();
		doUnbindConnectionManagerService();
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent keyEvent) {
		if (keyCode == KeyEvent.KEYCODE_BACK && mInstaller != null) {
			mInstaller.cancelOperation();
		}
		return super.onKeyDown(keyCode, keyEvent);
	}

	@Override
	public void onClientStateChanged(boolean isRun) {
		if (mUpdateStarts) {
			if (isRun) {
				HostListDbAdapter dbHelper = new HostListDbAdapter(this);
				dbHelper.open();
				ClientId clientId = dbHelper.fetchHost("nativeboinc");
				dbHelper.close();
				if (clientId != null) {
					try {
						mConnectionManager.connect(clientId, false);
					} catch(NoConnectivityException ex) { }
				}
				finish();
			} else {	// do reinstall
				mCurrentlyInstalledItem = 0;
				reinstallNextItem();
			}
		}
	}
	
	private void reinstallNextItem() {
		for (; mCurrentlyInstalledItem < mUpdateItems.length; mCurrentlyInstalledItem++)
			if (mUpdateItems[mCurrentlyInstalledItem].checked)
				break;	// install this item
		if (mCurrentlyInstalledItem < mUpdateItems.length) {
			UpdateItem updateItem = mUpdateItems[mCurrentlyInstalledItem];
			
			Toast.makeText(this, getString(R.string.updateUpdating)+" "+updateItem.name,
					Toast.LENGTH_SHORT);
			
			mCurrentlyInstalledItem++;
			mInstaller.reinstallUpdateItem(updateItem);
		} else {	// finish activity
			mRunner.startClient();
		}
	}

	private void cancelOnError() {
		if (mProgressDialog != null) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
		setProgressBarIndeterminateVisibility(false);
		mRunner.startClient();
		finish();
	}
	
	private void showErrorDialog(String message) {
		new AlertDialog.Builder(this).setTitle(getString(R.string.error))
			.setMessage(message)
			.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog, int which) {
					cancelOnError();
				}
			}).setOnCancelListener(new DialogInterface.OnCancelListener() {
				@Override
				public void onCancel(DialogInterface dialog) {
					cancelOnError();
				}
			}).create().show();
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
		showErrorDialog(message);
	}

	@Override
	public void onClientConfigured() {
		// do nothing
	}

	@Override
	public void onOperation(String opDescription) {
		setProgressBarIndeterminateVisibility(true);
		Toast.makeText(this, opDescription, Toast.LENGTH_SHORT).show();
	}

	@Override
	public void onOperationProgress(String opDescription, int progress) {
		if (mProgressDialog == null) {
			mProgressDialog = new ProgressDialog(this);
			mProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
		    mProgressDialog.setMessage(opDescription);
		    mProgressDialog.setMax(10000);
		    mProgressDialog.setProgress(0);
		    mProgressDialog.setCancelable(true);
		    mProgressDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
				@Override
				public void onCancel(DialogInterface dialog) {
					mInstaller.cancelOperation();
					finish();
				}
			});
		    mProgressDialog.show();
		}
		setProgressBarIndeterminateVisibility(true);
		mProgressDialog.setProgress(progress);
		if (progress == 10000) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
	}

	@Override
	public void onOperationError(String errorMessage) {
		showErrorDialog(errorMessage);
	}

	@Override
	public void onOperationCancel() {
		Toast.makeText(this, R.string.operationCancelled, Toast.LENGTH_LONG).show();
		if (mProgressDialog != null) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
		setProgressBarIndeterminateVisibility(false);
		mRunner.startClient();
	}

	@Override
	public void onOperationFinish() {
		if (mProgressDialog != null) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
		setProgressBarIndeterminateVisibility(false);
		// do next item
		reinstallNextItem();
	}

	@Override
	public void currentProjectDistribList(Vector<ProjectDistrib> projectDistribs) {
		mUpdateItems = mInstaller.getBinariesToUpdateOrInstall(mAttachedProjectUrls);
		setProgressBarIndeterminateVisibility(false);
		/* view items to update */
		mBinariesList.setAdapter(new UpdateListAdapter(this));
		if (mUpdateItems.length != 0)
			mAvailableText.setText(R.string.updateNewAvailable);
		else
			mAvailableText.setText(R.string.updateNoNew);
	}
	
	@Override
	public void currentClientDistrib(ClientDistrib clientDistrib) {
		mInstaller.updateProjectDistribList();
	}
}
