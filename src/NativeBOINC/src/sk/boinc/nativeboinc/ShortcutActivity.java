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

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import sk.boinc.nativeboinc.util.ScreenOrientationHandler;
import android.app.ListActivity;
import android.content.Context;
import android.content.Intent;
import android.content.Intent.ShortcutIconResource;
import android.database.Cursor;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CursorAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.AdapterView.AdapterContextMenuInfo;

/**
 * @author mat
 *
 */
public class ShortcutActivity extends ListActivity {


	private static final String TAG = "HostListActivity";
	
	private static final int EDIT_ID   = 1;
	private static final int DELETE_ID = 2;
	private static final int SELECT_ALL_ID = 3;
	private static final int UNSELECT_ALL_ID = 4;

	private static final int ADD_NEW_HOST = 1;
	private static final int EDIT_EXISTING_HOST = 2;
	
	private ScreenOrientationHandler mScreenOrientation;

	private HostListDbAdapter mDbHelper = null;
	private Cursor mHostCursor = null;

	/* nick names of selected clientIds */
	private HashSet<Long> mSelectedClientIds = new HashSet<Long>();
	
	private class HostItemOnCheckedClickListener implements View.OnClickListener {
		private long mRowId;
		
		public void setRowId(long rowId) {
			mRowId = rowId;
		}
		
		@Override
		public void onClick(View view) {
			if (mSelectedClientIds.contains(mRowId)) {
				if (Logging.DEBUG) Log.d(TAG, "Onclick:"+mRowId+",unselect");
				mSelectedClientIds.remove(mRowId);
			} else {
				if (Logging.DEBUG) Log.d(TAG, "Onclick:"+mRowId+",select");
				mSelectedClientIds.add(mRowId);
			}
		}
	}
	
	private class HostListAdapter extends CursorAdapter {

		public HostListAdapter(Context context, Cursor c) {
			super(context, c);
		}
		
		@Override
		public void bindView(View view, Context context, Cursor cursor) {
			TextView text1 = (TextView)view.findViewById(android.R.id.text1);
			String nickname = cursor.getString(cursor.getColumnIndex(HostListDbAdapter.FIELD_HOST_NICKNAME));
			text1.setText(nickname);
			TextView text2 = (TextView)view.findViewById(android.R.id.text2);
			String details = cursor.getString(cursor.getColumnIndex(HostListDbAdapter.FIELD_HOST_ADDRESS)) + 
				":" + cursor.getInt(cursor.getColumnIndex(HostListDbAdapter.FIELD_HOST_PORT));
			text2.setText(details);
			
			CheckBox checked = (CheckBox)view.findViewById(android.R.id.checkbox);
			if (!nickname.equals("nativeboinc")) {
				checked.setEnabled(true);
				long rowId = cursor.getLong(cursor.getColumnIndex(HostListDbAdapter.KEY_ROWID));
				// update checkbox state
				checked.setChecked(mSelectedClientIds.contains(rowId));
				/* update click listener */
				HostItemOnCheckedClickListener clickListener = (HostItemOnCheckedClickListener)view.getTag();
				if (clickListener == null) { // create new
					clickListener = new HostItemOnCheckedClickListener();
					view.setTag(clickListener);
				}
				clickListener.setRowId(rowId);
				checked.setOnClickListener(clickListener);
			} else {
				// disable when is nativeboinc
				view.setTag(null);
				checked.setChecked(false);
				checked.setEnabled(false);
			}
		}

		@Override
		public View newView(Context context, Cursor cursor, ViewGroup parent) {
			LayoutInflater inflater = LayoutInflater.from(context);
			View v = inflater.inflate(R.layout.checkable_list_item_2, parent, false);
			bindView(v, context, cursor);
			return v;
		}
	}
	
	private static class SavedState {
		private final HashSet<Long> selectedClientIds;
		
		public SavedState(ShortcutActivity activity) {
			selectedClientIds = activity.mSelectedClientIds;
		}
		
		public void restore(ShortcutActivity activity) {
			activity.mSelectedClientIds = selectedClientIds;
		}
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mScreenOrientation = new ScreenOrientationHandler(this);
		setContentView(R.layout.host_list);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		Button backButton = (Button)findViewById(R.id.back);
		backButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				finish();
			}
		});
		
		Button addNewHostButton = (Button)findViewById(R.id.newAddHost);
		addNewHostButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				addHost();
			}
		});
		
		registerForContextMenu(getListView());
		View emptyContent = findViewById(R.id.emptyContent);
		registerForContextMenu(emptyContent);
		
		mDbHelper = new HostListDbAdapter(this);
		mDbHelper.open();
		getHostList();
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}

	@Override
	protected void onResume() {
		super.onResume();
		mScreenOrientation.setOrientation();
	}

	@Override
	protected void onDestroy() {
		if (mDbHelper != null) {
			mDbHelper.close();
			mDbHelper = null;
		}
		mScreenOrientation = null;
		super.onDestroy();
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
		super.onCreateContextMenu(menu, v, menuInfo);
		
		int position = -1;
		ClientId clientId = null;
		
		if (menuInfo != null) {
			position = ((AdapterContextMenuInfo)menuInfo).position;
			Cursor c = mHostCursor;
			c.moveToPosition(position);
			clientId = new ClientId(c);
		}
		
		menu.setHeaderTitle(R.string.hostOperation);
		
		int count = mHostCursor.getCount();
		int selectedSize = mSelectedClientIds.size();
		
		if (selectedSize < count-1)
			menu.add(0, SELECT_ALL_ID, 0, R.string.selectAll);
		if (selectedSize != 0)
			menu.add(0, UNSELECT_ALL_ID, 0, R.string.unselectAll);
		
		if (clientId != null && !clientId.getNickname().equals("nativeboinc")) {
			// if nativeboinc
			menu.add(0, EDIT_ID, 0, R.string.hostEdit);
			menu.add(0, DELETE_ID, 0, R.string.hostDelete);
		} else {
			if (selectedSize != 0)
				menu.add(0, DELETE_ID, 0, R.string.hostDelete);
		}
	}

	@Override
	public boolean onContextItemSelected(MenuItem item) {
		AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
		switch(item.getItemId()) {
		case EDIT_ID:
			modifyHost(info.position, info.id);
			return true;
		case DELETE_ID:
			if (mSelectedClientIds.isEmpty()) {
				// if not selected
				mDbHelper.deleteHost(info.id);
				mSelectedClientIds.remove(info.id);
			} else {
				// selected
				for (Long rowId: mSelectedClientIds)
					mDbHelper.deleteHost(rowId);
				mSelectedClientIds.clear();
			}
			getHostList();
			return true;
		case SELECT_ALL_ID: {
			int pos = mHostCursor.getPosition();
			mHostCursor.moveToFirst();
			
			while (mHostCursor.moveToNext()) {
				String nickname = mHostCursor.getString(
						mHostCursor.getColumnIndex(HostListDbAdapter.FIELD_HOST_NICKNAME));
				if (!nickname.equals("nativeboinc")) {
					mSelectedClientIds.add(mHostCursor.getLong(
							mHostCursor.getColumnIndex(HostListDbAdapter.KEY_ROWID)));
				}
			}
			// restore position
			mHostCursor.moveToPosition(pos);
			
			((HostListAdapter)getListAdapter()).notifyDataSetChanged();
			return true;
		}
		case UNSELECT_ALL_ID:
			mSelectedClientIds.clear();
			((HostListAdapter)getListAdapter()).notifyDataSetChanged();
			return true;
		}
		return super.onContextItemSelected(item);
	}

	@Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
		super.onListItemClick(l, v, position, id);
		Cursor c = mHostCursor;
		c.moveToPosition(position);
		ClientId clientId = new ClientId(c);
		
		Intent shortcutIntent = new Intent(this, BoincManagerActivity.class);
		ShortcutIconResource sIcon = ShortcutIconResource.fromContext(this, R.drawable.nativeboinc);
		shortcutIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		shortcutIntent.putExtra(BoincManagerActivity.PARAM_CLIENT_TO_CONNECT, clientId.parcelToString());
		Intent intent = new Intent();
		intent.putExtra(Intent.EXTRA_SHORTCUT_NAME, clientId.getNickname());
		intent.putExtra(Intent.EXTRA_SHORTCUT_ICON_RESOURCE, sIcon);
		intent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
		setResult(RESULT_OK, intent);
		finish();
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		switch (requestCode) {
		case ADD_NEW_HOST:
			if (resultCode == RESULT_OK) {
				// Finished successfully - added the host
				ClientId clientId = data.getParcelableExtra(ClientId.TAG);
				mDbHelper.addHost(clientId);
				//mHostCursor.requery();
				getHostList();
			} 
			break;
		case EDIT_EXISTING_HOST:
			if (resultCode == RESULT_OK) {
				// Finished successfully - modified the host
				ClientId clientId = data.getParcelableExtra(ClientId.TAG);
				if (clientId.getId() != -1L) {
					// We have received valid ID of host
					mDbHelper.updateHost(clientId);
					//mHostCursor.requery();
					getHostList();
				}
			} 
			break;
		default:
			break;
		}
	}

	private void getHostList() {
		// Get all rows from the database and create the item list
		mHostCursor = mDbHelper.fetchAllHosts();
		startManagingCursor(mHostCursor);
		
		setListAdapter(new HostListAdapter(this, mHostCursor));
	}

	private void addHost() {
		startActivityForResult(new Intent(this, EditHostActivity.class), ADD_NEW_HOST);
	}

	private void modifyHost(int position, long id) {
		Cursor c = mHostCursor;
		c.moveToPosition(position);
		ClientId clientId = new ClientId(c);
		Intent intent = new Intent(this, EditHostActivity.class).putExtra(ClientId.TAG, clientId);
		startActivityForResult(intent, EDIT_EXISTING_HOST);
	}
}
