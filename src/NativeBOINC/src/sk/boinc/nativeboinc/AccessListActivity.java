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

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.AbstractNativeBoincListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUtils;
import sk.boinc.nativeboinc.util.StandardDialogs;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.text.InputType;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class AccessListActivity extends ServiceBoincActivity implements AbstractNativeBoincListener {

	private static final String TAG = "AccessListActivity";
	
	public static final String RESULT_RESTARTED = "Restarted";
	
	// displayed when error at loading access list
	private static final int DIALOG_ENTER_EDIT_HOST = 4;
	private static final int DIALOG_ENTER_ADD_HOST = 5;
	
	// menu item ids
	private static final int SELECT_ALL_ID = 1;
	private static final int UNSELECT_ALL_ID = 2;
	private static final int EDIT_ID = 3;
	private static final int DELETE_ID = 4;
	
	private ListView mAccessListView = null;
	private Button mAddHostButton = null;
	
	private boolean mAccessListLoaded = false;
	private ArrayList<String> mAccessList = new ArrayList<String>();
	private HashSet<String> mSelectedHosts = new HashSet<String>();
	private boolean mDoRestart = false;
	
	// used by enter dialog
	private int mEditPosition = -1;
	
	private static class SavedState {
		private final boolean accessListLoaded;
		private final ArrayList<String> accessList;
		private final HashSet<String> selectedHosts;
		private final int editPosition;
		private final boolean doRestart;
		
		public SavedState(AccessListActivity activity) {
			accessListLoaded = activity.mAccessListLoaded;
			accessList = activity.mAccessList;
			selectedHosts = activity.mSelectedHosts;
			editPosition = activity.mEditPosition;
			doRestart = activity.mDoRestart;
		}
		
		public void restore(AccessListActivity activity) {
			activity.mAccessList = accessList;
			activity.mAccessListLoaded = accessListLoaded;
			activity.mSelectedHosts = selectedHosts;
			activity.mEditPosition = editPosition;
			activity.mDoRestart = doRestart;
		}
	}
	
	private class ItemOnCheckClickListener implements View.OnClickListener {
		private String mHostAddress;
		
		public ItemOnCheckClickListener(String hostAddress) {
			mHostAddress = hostAddress;
		}
		
		public void setHostAddress(String hostAddress) {
			mHostAddress = hostAddress;
		}
		
		@Override
		public void onClick(View v) {
			if (Logging.DEBUG) Log.d(TAG, "On click, host:"+mHostAddress);
			
			if (mSelectedHosts.contains(mHostAddress))
				mSelectedHosts.remove(mHostAddress);
			else
				mSelectedHosts.add(mHostAddress);
		}
	}
	
	private class AccessListAdapter extends BaseAdapter {

		private Context mContext;
		
		public AccessListAdapter(Context context) {
			mContext = context;
		}
		
		@Override
		public int getCount() {
			if (mAccessList == null)
				return 0;
			return mAccessList.size();
		}

		@Override
		public Object getItem(int position) {
			return mAccessList.get(position);
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			View view = null;
			if (convertView == null) {
				LayoutInflater inflater = LayoutInflater.from(mContext);
				view = inflater.inflate(R.layout.checkable_list_item_1, null);
			} else
				view = convertView;
			
			CheckBox checkBox = (CheckBox)view.findViewById(android.R.id.checkbox);
			TextView text = (TextView)view.findViewById(android.R.id.text1);
			
			String hostAddress = mAccessList.get(position);
			checkBox.setChecked(mSelectedHosts.contains(hostAddress));
			
			/* install click listener */
			ItemOnCheckClickListener clickListener = (ItemOnCheckClickListener)view.getTag();
			if (clickListener == null) {
				clickListener = new ItemOnCheckClickListener(hostAddress);
				view.setTag(clickListener);
			} else
				clickListener.setHostAddress(hostAddress);
			
			
			checkBox.setOnClickListener(clickListener);
			text.setText(hostAddress);
			
			// setup enabled
			boolean enabled = isEnabled(position);
			checkBox.setEnabled(enabled);
			text.setEnabled(enabled);
			
			return view;
		}
	}
	
	private AccessListAdapter mAccessListAdapter = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		setUpService(false, false, true, true, false, false);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		super.onCreate(savedInstanceState);
		setContentView(R.layout.access_list);
		
		if (!mAccessListLoaded) {
			try {
				ArrayList<String> temporary = NativeBoincUtils.getRemoteHostList(this);
				if (temporary != null) // if not null
					mAccessList = temporary;
			} catch(IOException ex) {
				StandardDialogs.showErrorDialog(this, getString(R.string.remoteHostsLoadError));
			}
		}
		
		mAccessListView = (ListView)findViewById(android.R.id.list);
		mAccessListAdapter = new AccessListAdapter(this);
		mAccessListView.setAdapter(mAccessListAdapter);
		
		// register for context menu
		registerForContextMenu(mAccessListView);
		
		/* buttons */
		Button cancelButton = (Button)findViewById(R.id.cancel);
		cancelButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				// finish this activity
				finish();
			}
		});
		
		mAddHostButton = (Button)findViewById(R.id.addHost);
		mAddHostButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				mEditPosition = -1; // add host
				showDialog(DIALOG_ENTER_ADD_HOST);
			}
		});
		
		Button okButton = (Button)findViewById(R.id.ok);
		okButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				try {
					if (mDoRestart) {
						// write host list
						NativeBoincUtils.setRemoteHostList(AccessListActivity.this, mAccessList);
						// finish activity
						Intent data = new Intent();
						data.putExtra(RESULT_RESTARTED, true);
						setResult(RESULT_OK, data);
						finish();
					} else {
						finish();
					}
				} catch(IOException ex) {
					// when happens error
					StandardDialogs.showErrorDialog(AccessListActivity.this,
							getString(R.string.remoteHostsSaveError));
				}
			}
		});
	}
	
	@Override
	public void onRunnerConnected() {
		setProgressBarIndeterminateVisibility(mRunner.serviceIsWorking());
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		mAccessListAdapter.notifyDataSetChanged();
	}
	
	@Override
	public Dialog onCreateDialog(int dialogId, Bundle args) {
		Dialog dialog = StandardDialogs.onCreateDialog(this, dialogId, args);
		if (dialog != null)
			return dialog;
		
		
		if (dialogId == DIALOG_ENTER_EDIT_HOST || dialogId == DIALOG_ENTER_ADD_HOST) {
			View view = LayoutInflater.from(this).inflate(R.layout.dialog_edit, null);
			final EditText hostEdit = (EditText)view.findViewById(android.R.id.edit);
			hostEdit.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
			// get item position
			if (Logging.DEBUG) Log.d(TAG, "on enter host. position:"+mEditPosition);
			
			int titleId = R.string.addHost;
			if (dialogId == DIALOG_ENTER_EDIT_HOST)
				titleId = R.string.hostEdit;
			
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_input_get)
				.setTitle(titleId)
				.setView(view)
				.setPositiveButton(R.string.ok, new Dialog.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						if (mEditPosition == -1)
							// add host
							mAccessList.add(hostEdit.getText().toString());
						else // modify host
							mAccessList.set(mEditPosition, hostEdit.getText().toString());
						// clear edit position
						mEditPosition = -1;
						mDoRestart = true;
						mAccessListAdapter.notifyDataSetChanged();
					}
				})
				.setNegativeButton(R.string.cancel, null)
				.create();
		}
		return null;
	}
	
	@Override
	protected void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		if (StandardDialogs.onPrepareDialog(this, dialogId, dialog, args))
			return; // if standard dialog
		
		final EditText hostEdit = (EditText)dialog.findViewById(android.R.id.edit);
		
		if (dialogId == DIALOG_ENTER_ADD_HOST)
				hostEdit.setText("");
		else  if (dialogId == DIALOG_ENTER_EDIT_HOST)
			hostEdit.setText(mAccessList.get(mEditPosition));
	}
	
	@Override
	public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfo) {
		super.onCreateContextMenu(menu, view, menuInfo);
		
		int selectedCount = mSelectedHosts.size();
		int accessListSize = mAccessList.size();
		if (selectedCount != accessListSize)
			menu.add(0, SELECT_ALL_ID, 0, R.string.selectAll);
		if (selectedCount != 0)
			menu.add(0, UNSELECT_ALL_ID, 0, R.string.unselectAll);
		
		menu.add(0, EDIT_ID, 0, R.string.hostEdit);
		menu.add(0, DELETE_ID, 0, R.string.hostDelete);
	}
	
	@Override
	public boolean onContextItemSelected(MenuItem item) {
		AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
		switch(item.getItemId()) {
		case SELECT_ALL_ID:
			mSelectedHosts.addAll(mAccessList);
			mAccessListAdapter.notifyDataSetChanged();
			return true;
		case UNSELECT_ALL_ID:
			mSelectedHosts.clear();
			mAccessListAdapter.notifyDataSetChanged();
			return true;
		case EDIT_ID: {
			mEditPosition = info.position;
			showDialog(DIALOG_ENTER_EDIT_HOST);
			return true;
		}
		case DELETE_ID:
			if (mSelectedHosts.isEmpty()) { // single delete
				String hostAddress = mAccessList.remove(info.position);
				mSelectedHosts.remove(hostAddress);
			} else { // multiple delete
				mAccessList.removeAll(mSelectedHosts);
				mSelectedHosts.clear();
			}
			mDoRestart = true;
			mAccessListAdapter.notifyDataSetChanged();
			return true;
		}
		return false;
	}

	@Override
	public void onNativeBoincClientError(String message) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}
}
