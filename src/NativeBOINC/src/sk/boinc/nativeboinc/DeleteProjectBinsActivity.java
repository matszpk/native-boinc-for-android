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
import java.util.HashSet;

import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
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
import sk.boinc.nativeboinc.installer.DeleteProjectBinsListener;
import sk.boinc.nativeboinc.installer.InstallOp;
import sk.boinc.nativeboinc.installer.InstalledBinary;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.util.StandardDialogs;

/**
 * @author mat
 *
 */
public class DeleteProjectBinsActivity extends ServiceBoincActivity implements
		DeleteProjectBinsListener {

	@Override
	public int getInstallerChannelId() {
		return InstallerService.DEFAULT_CHANNEL_ID;
	}
	
	private TextView mAvailableText;
	private ListView mProjectsList;
	private Button mConfirmButton;
	
	private InstalledBinary[] mInstalledBinaries;
	private HashSet<String> mSelectedBinaries = new HashSet<String>();
	private boolean mDeleteBinariesInProgress = false;
	
	private void setConfirmButtonEnabled() {
		if (mDeleteBinariesInProgress)
			mConfirmButton.setEnabled(false);
		else
			mConfirmButton.setEnabled(!mSelectedBinaries.isEmpty());
	}
	
	private class ItemOnClickListener implements View.OnClickListener {
		private String mDistribName;

		public ItemOnClickListener(String distribName) {
			mDistribName = distribName;
		}

		public void setUp(String distribName) {
			mDistribName = distribName;
		}

		@Override
		public void onClick(View view) {
			if (mSelectedBinaries.contains(mDistribName))
				mSelectedBinaries.remove(mDistribName);
			else
				mSelectedBinaries.add(mDistribName);
			setConfirmButtonEnabled();
		}
	}
	
	private class ProjectBinariesAdapter extends BaseAdapter {

		private Context mContext = null;
		
		public ProjectBinariesAdapter(Context context) {
			mContext = context;
		}
		
		@Override
		public int getCount() {
			if (mInstalledBinaries == null)
				return 0;
			return mInstalledBinaries.length;
		}

		@Override
		public Object getItem(int position) {
			if (mInstalledBinaries == null)
				return null;
			return mInstalledBinaries[position];
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			View view = convertView;
			if (view == null) {
				LayoutInflater inflater = LayoutInflater.from(mContext);
				view = inflater.inflate(R.layout.checkable_list_item_1, null);
			}
			
			TextView text = (TextView)view.findViewById(android.R.id.text1);
			final CheckBox checkBox = (CheckBox)view.findViewById(android.R.id.checkbox);
			
			final InstalledBinary binary = mInstalledBinaries[position];
			text.setText(binary.name);
			
			checkBox.setChecked(mSelectedBinaries.contains(binary.name));
			
			ItemOnClickListener itemOnClickListener = (ItemOnClickListener)view.getTag();
			if (itemOnClickListener == null) {
				itemOnClickListener = new ItemOnClickListener(binary.name);
				view.setTag(itemOnClickListener);
			} else
				itemOnClickListener.setUp(binary.name);
			
			checkBox.setOnClickListener(itemOnClickListener);
			return view;
		}
	}
	
	private ProjectBinariesAdapter mProjectsBinariesAdapter = null;
	
	private static class SavedState {
		private final InstalledBinary[] installedBinaries;
		private final HashSet<String> selectedBinaries;
		private final boolean deleteBinariesInProgress;
		
		public SavedState(DeleteProjectBinsActivity activity) {
			installedBinaries = activity.mInstalledBinaries;
			selectedBinaries = activity.mSelectedBinaries;
			deleteBinariesInProgress = activity.mDeleteBinariesInProgress;
		}
		
		public void restore(DeleteProjectBinsActivity activity) {
			activity.mInstalledBinaries = installedBinaries;
			activity.mSelectedBinaries = selectedBinaries;
			activity.mDeleteBinariesInProgress = deleteBinariesInProgress;
		}
	}
	
	@Override
	public void onInstallerConnected() {
		if (mInstalledBinaries == null) {
			InstalledBinary[] tmp = mInstaller.getInstalledBinaries();
			mInstalledBinaries = new InstalledBinary[tmp.length-1];
			int j = 0;
			for (int i = 0; i < tmp.length; i++)
				if (!tmp[i].name.equals(InstallerService.BOINC_CLIENT_ITEM_NAME))
					mInstalledBinaries[j++] = tmp[i];
		}
		
		updateActivityState();
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		setUpService(false, false, false, false, true, true);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.delete_projbins);
		
		mAvailableText = (TextView)findViewById(R.id.noAvailableText);
		mProjectsList = (ListView)findViewById(R.id.projectsList);
		
		mProjectsBinariesAdapter = new ProjectBinariesAdapter(this);
		mProjectsList.setAdapter(mProjectsBinariesAdapter);
		
		mProjectsList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view,
					int position, long id) {
				CheckBox checkBox = (CheckBox)view.findViewById(android.R.id.checkbox);
				String distribName = mInstalledBinaries[position].name;
				
				if (mSelectedBinaries.contains(distribName)) {
					mSelectedBinaries.remove(distribName);
					checkBox.setChecked(false);
				} else {
					mSelectedBinaries.add(distribName);
					checkBox.setChecked(true);
				}
				setConfirmButtonEnabled();
			}
		});
		
		mConfirmButton = (Button)findViewById(R.id.ok);
		mConfirmButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mInstaller == null)
					return;
				
				if (!mSelectedBinaries.isEmpty()) {
					mDeleteBinariesInProgress = true;
					mInstaller.deleteProjectBinaries(new ArrayList<String>(mSelectedBinaries));
					setConfirmButtonEnabled();
				}
			}
		});
		
		Button cancelButton = (Button)findViewById(R.id.cancel);
		cancelButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				onBackPressed();
			}
		});
	}
	
	private void updateProjectsListView() {
		if (mInstalledBinaries.length == 0)
			mAvailableText.setVisibility(View.VISIBLE);
		else
			mAvailableText.setVisibility(View.GONE);
		
		mProjectsBinariesAdapter.notifyDataSetChanged();
	}
	
	private void updateActivityState() {
		setProgressBarIndeterminateVisibility(mInstaller.isWorking());
		
		if (mInstaller.handlePendingErrors(InstallOp.DeleteProjectBinaries, this))
			return;
		
		if (mDeleteBinariesInProgress) {
			if (!mInstaller.isOperationBeingExecuted(InstallOp.DeleteProjectBinaries))
				onDeleteProjectBinaries();
		}
		
		setConfirmButtonEnabled();		
		updateProjectsListView();
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
	public void onChangeInstallerIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}

	@Override
	public boolean onOperationError(InstallOp installOp, String distribName,
			String errorMessage) {
		if (installOp.equals(InstallOp.DeleteProjectBinaries) && mDeleteBinariesInProgress) {
			mDeleteBinariesInProgress = false;
			StandardDialogs.showInstallErrorDialog(this, distribName, errorMessage);
			return true;
		}
		return false;
	}

	@Override
	public void onDeleteProjectBinaries() {
		finish();
	}
}
