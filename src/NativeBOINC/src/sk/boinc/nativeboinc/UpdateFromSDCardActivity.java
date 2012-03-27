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

import java.util.HashSet;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
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
public class UpdateFromSDCardActivity extends ServiceBoincActivity {
	
	public static final String UPDATE_DIR = "UpdateDir"; 
	
	private String[] mUpdateDistribList = null;
	private HashSet<String> mSelectedItems = new HashSet<String>();
	
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
			if (mSelectedItems.contains(mDistribName))
				mSelectedItems.remove(mDistribName);
			else
				mSelectedItems.add(mDistribName);
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
			if (mUpdateDistribList == null)
				return 0;
			return mUpdateDistribList.length;		
		}

		@Override
		public Object getItem(int position) {
			return mUpdateDistribList[position];
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
			
			final String distribName = mUpdateDistribList[position];
			
			nameText.setText(distribName);
			
			checkBox.setChecked(mSelectedItems.contains(distribName));
			
			ItemOnClickListener itemOnClickListener = (ItemOnClickListener)view.getTag();
			if (itemOnClickListener == null) {
				itemOnClickListener = new ItemOnClickListener(distribName);
				view.setTag(itemOnClickListener);
			} else
				itemOnClickListener.setUp(distribName);
			
			checkBox.setOnClickListener(itemOnClickListener);
			return view;
		}
	}
	
	private Button mConfirmButton = null;
	
	private TextView mAvailableText = null;
	private ListView mBinariesList = null;
	private UpdateListAdapter mUpdateListAdapter = null;
	
	private String mExternalPath = null;
	private String mUpdateDirPath = null;
	
	private static class SavedState {
		private final String updateDirPath;
		private String[] updateDistribList;
		private HashSet<String> selectedItems;
		
		public SavedState(UpdateFromSDCardActivity activity) {
			updateDirPath = activity.mUpdateDirPath;
			selectedItems = activity.mSelectedItems;
			updateDistribList = activity.mUpdateDistribList;
		}
		
		public void restore(UpdateFromSDCardActivity activity) {
			activity.mUpdateDirPath = updateDirPath;
			activity.mSelectedItems = selectedItems;
			activity.mUpdateDistribList = updateDistribList;
		}
	};
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		
		mExternalPath = Environment.getExternalStorageDirectory().toString();
		
		setUpService(false, false, false, false, true, false);
		
		super.onCreate(savedInstanceState);
		setContentView(R.layout.update);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		mAvailableText = (TextView)findViewById(R.id.updateAvailableText);
		mBinariesList = (ListView)findViewById(R.id.updateBinariesList);
		
		mUpdateListAdapter = new UpdateListAdapter(this);
		ListView listView = (ListView)findViewById(R.id.updateBinariesList);
		listView.setAdapter(mUpdateListAdapter);
		
		mBinariesList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view,
					int position, long id) {
				CheckBox checkBox = (CheckBox)view.findViewById(android.R.id.checkbox);
				String distribName = mUpdateDistribList[position];
				if (mSelectedItems.contains(distribName)) {
					mSelectedItems.remove(distribName);
					checkBox.setChecked(false);
				} else {
					mSelectedItems.add(distribName);
					checkBox.setChecked(true);
				}
					
				setConfirmButtonEnabled();
			}
		});
		
		mConfirmButton = (Button)findViewById(R.id.updateOk);
		mConfirmButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				// runs update/installation
				mInstaller.updateDistribsFromSDCard(mUpdateDirPath, mSelectedItems.toArray(new String[0]));
				finish();
				startActivity(new Intent(UpdateFromSDCardActivity.this, ProgressActivity.class));
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
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	@Override
	public void onInstallerConnected() {
		String updateDir = getIntent().getStringExtra(UPDATE_DIR);
		
		if (updateDir == null)
			return;
		
		if (mUpdateDirPath != null) // if from savedState
			return;
		
		if (mExternalPath.endsWith("/")) {
			if (updateDir.startsWith("/"))
				mUpdateDirPath = mExternalPath+updateDir.substring(1);
			else
				mUpdateDirPath = mExternalPath+updateDir;
		} else {
			if (updateDir.startsWith("/"))
				mUpdateDirPath = mExternalPath+updateDir;
			else
				mUpdateDirPath = mExternalPath+"/"+updateDir;
		}
		
		if (!mUpdateDirPath.endsWith("/")) // add last slash
			mUpdateDirPath += "/";
		
		mUpdateDistribList = mInstaller.getBinariesToUpdateFromSDCard(mUpdateDirPath);
		
		if (mUpdateDistribList == null || mUpdateDistribList.length == 0)
			mAvailableText.setText(R.string.updateNoNew);
		else
			mAvailableText.setText(R.string.updateNewAvailable);
		
		mUpdateListAdapter.notifyDataSetChanged();
	}
	
	private void setConfirmButtonEnabled() {
		mConfirmButton.setEnabled(!mSelectedItems.isEmpty());
	}
}
