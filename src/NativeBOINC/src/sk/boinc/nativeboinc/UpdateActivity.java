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

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.installer.ClientDistrib;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.installer.InstallerUpdateListener;
import sk.boinc.nativeboinc.installer.ProjectDistrib;
import sk.boinc.nativeboinc.util.ProgressState;
import sk.boinc.nativeboinc.util.StandardDialogs;
import sk.boinc.nativeboinc.util.UpdateItem;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class UpdateActivity extends ServiceBoincActivity implements InstallerUpdateListener {
	
	private static final String TAG = "UpdateActivity";
	
	private TextView mAvailableText;
	private ListView mBinariesList;
	private Button mConfirmButton;
	
	private UpdateItem[] mUpdateItems = null;
	
	private int mGetUpdateItemsProgressState = ProgressState.NOT_RUN;

	@Override
	public int getInstallerChannelId() {
		return InstallerService.DEFAULT_CHANNEL_ID;
	}
	
	private static class SavedState {
		private final UpdateItem[] updateItems; 
		private final int getUpdateItemsProgressState;
		
		public SavedState(UpdateActivity activity) {
			getUpdateItemsProgressState = activity.mGetUpdateItemsProgressState;
			updateItems = activity.mUpdateItems;
		}
		
		public void restore(UpdateActivity activity) {
			activity.mGetUpdateItemsProgressState = getUpdateItemsProgressState;
			activity.mUpdateItems = updateItems;
		}
	}
	
	private void setConfirmButtonEnabled() {
		if (mUpdateItems == null) {
			mConfirmButton.setEnabled(false);
			return;
		}
		int count = 0;
		for (UpdateItem item: mUpdateItems)
			if (item.checked)
				count++;
		
		mConfirmButton.setEnabled(count != 0);
	}
	
	private class ItemOnClickListener implements View.OnClickListener {
		private int mPosition;
		
		public ItemOnClickListener(int position) {
			mPosition = position;
		}
		
		public void setUp(int position) {
			mPosition = position;
		}
		
		@Override
		public void onClick(View view) {
			UpdateItem item = mUpdateItems[mPosition];
			item.checked = !item.checked;
			setConfirmButtonEnabled();
		}
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
				LayoutInflater inflater = LayoutInflater.from(mContext);
				view = inflater.inflate(R.layout.checkable_list_item_1, null);
			}
			
			TextView nameText = (TextView)view.findViewById(android.R.id.text1);
			final CheckBox checkBox = (CheckBox)view.findViewById(android.R.id.checkbox);
			
			final UpdateItem item = mUpdateItems[position];
			nameText.setText(item.name + " " + item.version);
			
			checkBox.setChecked(item.checked);
			
			ItemOnClickListener itemOnClickListener = (ItemOnClickListener)view.getTag();
			if (itemOnClickListener == null) {
				itemOnClickListener = new ItemOnClickListener(position);
				view.setTag(itemOnClickListener);
			} else
				itemOnClickListener.setUp(position);
			
			checkBox.setOnClickListener(itemOnClickListener);
			return view;
		}
	}
	
	private UpdateListAdapter mUpdateListAdapter = null;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		setUpService(false, false, false, false, true, true);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		super.onCreate(savedInstanceState);
		setContentView(R.layout.update);
		
		mAvailableText = (TextView)findViewById(R.id.updateAvailableText);
		mBinariesList = (ListView)findViewById(R.id.updateBinariesList);
		
		mUpdateListAdapter = new UpdateListAdapter(this);
		mBinariesList.setAdapter(mUpdateListAdapter);
		
		mBinariesList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view,
					int position, long id) {
				CheckBox checkBox = (CheckBox)view.findViewById(android.R.id.checkbox);
				UpdateItem item = mUpdateItems[position];
				item.checked = !item.checked;
				checkBox.setChecked(item.checked);
				setConfirmButtonEnabled();
			}
		});
		
		mBinariesList.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
			@Override
			public boolean onItemLongClick(AdapterView<?> parent, View view,
					int position, long id) {
				UpdateItem item = mUpdateItems[position];
				StandardDialogs.showDistribInfoDialog(UpdateActivity.this, item.name, item.version,
						item.description, item.changes);
				return true;
			}
		});
		
		mConfirmButton = (Button)findViewById(R.id.updateOk);
		mConfirmButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mInstaller == null)
					return;
				// runs update/installation
				ArrayList<UpdateItem> selectedUpdateItems = new ArrayList<UpdateItem>();
				for (UpdateItem item: mUpdateItems)
					if (item.checked) {
						selectedUpdateItems.add(item);
						if (Logging.DEBUG) Log.d(TAG, "Selected item:"+item.name);
					}
					
				mInstaller.reinstallUpdateItems(selectedUpdateItems);
				finish();
				startActivity(new Intent(UpdateActivity.this, ProgressActivity.class));
			}
		});
		
		Button cancelButton = (Button)findViewById(R.id.updateCancel);
		cancelButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				onBackPressed();
			}
		});
	}
	
	@Override
	public void onInstallerConnected() {
		Log.d(TAG, "onInstallerConnected");
		updateActivityState();
	}
	
	@Override
	public void onRunnerConnected() {
		Log.d(TAG, "onInstallerConnected");
		updateActivityState();
	}
	
	private void updateBinariesView() {
		if (mUpdateItems == null || mUpdateItems.length == 0)
			mAvailableText.setText(R.string.updateNoNew);
		else
			mAvailableText.setText(R.string.updateNewAvailable);
		
		mUpdateListAdapter.notifyDataSetChanged();
	}
	
	private void updateActivityState() {
		// display/hide progress
		setProgressBarIndeterminateVisibility(mInstaller.isWorking());
		
		/* display error if pending */
		if (mInstaller != null) {
			if (mInstaller.handlePendingError(this))
				return;
		}
		
		/* handle update items operation */
		if (mUpdateItems == null) {
			if (mGetUpdateItemsProgressState == ProgressState.IN_PROGRESS) {
				mUpdateItems = mInstaller.getPendingBinariesToUpdateOrInstall();
				
				if (mUpdateItems != null)
					updateBinariesView();
					
			} else if (mGetUpdateItemsProgressState == ProgressState.NOT_RUN) {
				mGetUpdateItemsProgressState = ProgressState.IN_PROGRESS;
				mInstaller.getBinariesToUpdateOrInstall();
			}
			// if finished but failed
		} else  // do it
			updateBinariesView();
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		if (mInstaller != null)
			updateActivityState();
	}
	
	@Override
	public void onBackPressed() {
		if (mInstaller != null)
			mInstaller.cancelSimpleOperation();
		finish();
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	@Override
	protected Dialog onCreateDialog(int dialogId, Bundle args) {
		return StandardDialogs.onCreateDialog(this, dialogId, args);
	}
	
	@Override
	protected void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		StandardDialogs.onPrepareDialog(this, dialogId, dialog, args);
	}

	@Override
	public void currentProjectDistribList(ArrayList<ProjectDistrib> projectDistribs) {
		// do nothing
	}
	
	@Override
	public void currentClientDistrib(ClientDistrib clientDistrib) {
		// do nothing
	}
	
	@Override
	public void onChangeInstallerIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}

	@Override
	public boolean onOperationError(String distribName, String errorMessage) {
		if (InstallerService.isSimpleOperation(distribName) &&
				mGetUpdateItemsProgressState == ProgressState.IN_PROGRESS) {
			mGetUpdateItemsProgressState = ProgressState.FAILED;
			StandardDialogs.showInstallErrorDialog(this, distribName, errorMessage);
			return true;
		}
		return false;
	}

	@Override
	public void binariesToUpdateOrInstall(UpdateItem[] updateItems) {
		mUpdateItems = updateItems;
		mGetUpdateItemsProgressState = ProgressState.FINISHED;
		updateBinariesView();
	}

	@Override
	public void binariesToUpdateFromSDCard(String[] projectNames) {
		// TODO Auto-generated method stub
		
	}
}

