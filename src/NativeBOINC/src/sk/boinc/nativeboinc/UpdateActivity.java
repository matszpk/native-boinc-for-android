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

import java.util.ArrayList;

import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.installer.ClientDistrib;
import sk.boinc.nativeboinc.installer.InstallerProgressListener;
import sk.boinc.nativeboinc.installer.InstallerUpdateListener;
import sk.boinc.nativeboinc.installer.ProjectDistrib;
import sk.boinc.nativeboinc.nativeclient.NativeBoincStateListener;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import sk.boinc.nativeboinc.util.UpdateItem;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
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
public class UpdateActivity extends ServiceBoincActivity implements NativeBoincStateListener,
		InstallerProgressListener, InstallerUpdateListener {
	public static final String ATTACHED_PROJECT_URLS_TAG = "AttachedProjectUrlList";
	
	private TextView mAvailableText;
	private ListView mBinariesList;
	
	private UpdateItem[] mUpdateItems = null;
	private String[] mAttachedProjectUrls = null;
	private int mCurrentlyInstalledItem = 0;
	private boolean mUpdateStarts = false;
	
	private ProgressDialog mProgressDialog = null;
	
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
			final CheckBox checkBox = (CheckBox)view.findViewById(R.id.updateItemCheckbox);
			
			final UpdateItem item = mUpdateItems[position];
			nameText.setText(item.name + " " + item.version);
			
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
		setUpService(true, false, true, true, true, true);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.update);
		
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
				finish();
			}
		});
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent keyEvent) {
		if (keyCode == KeyEvent.KEYCODE_BACK && mInstaller != null) {
			mInstaller.cancelAll();
		}
		return super.onKeyDown(keyCode, keyEvent);
	}

	@Override
	public void onClientStart() {
		if (mUpdateStarts) {
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
		}
	}
	
	@Override
	public void onClientStop(int exitCode, boolean stoppedByManager) {
		if (mUpdateStarts) {
			mCurrentlyInstalledItem = 0;
			reinstallNextItem();
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
			//mInstaller.reinstallUpdateItem(updateItem);
		} else {	// finish activity
			mRunner.startClient(false);
		}
	}

	private void cancelOnError() {
		if (mProgressDialog != null) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
		setProgressBarIndeterminateVisibility(false);
		mRunner.startClient(false);
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
	public void onNativeBoincClientError(String message) {
		showErrorDialog(message);
	}

	@Override
	public void onOperation(String distribName, String opDescription) {
		setProgressBarIndeterminateVisibility(true);
		Toast.makeText(this, opDescription, Toast.LENGTH_SHORT).show();
	}

	@Override
	public void onOperationProgress(String distribName, String opDescription, int progress) {
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
					mInstaller.cancelAll();
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
	public void onOperationError(String distribName, String errorMessage) {
		showErrorDialog(errorMessage);
	}

	@Override
	public void onOperationCancel(String distribName) {
		Toast.makeText(this, R.string.operationCancelled, Toast.LENGTH_LONG).show();
		if (mProgressDialog != null) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
		setProgressBarIndeterminateVisibility(false);
		mRunner.startClient(false);
	}

	@Override
	public void onOperationFinish(String distribName) {
		if (mProgressDialog != null) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
		setProgressBarIndeterminateVisibility(false);
		// do next item
		reinstallNextItem();
	}

	@Override
	public void currentProjectDistribList(ArrayList<ProjectDistrib> projectDistribs) {
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

	@Override
	public void onInstallerWorking(boolean isWorking) {
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		
	}
}
