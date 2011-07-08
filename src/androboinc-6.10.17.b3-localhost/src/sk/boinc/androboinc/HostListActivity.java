/* 
 * AndroBOINC - BOINC Manager for Android
 * Copyright (C) 2010, Pavol Michalec
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

package sk.boinc.androboinc;

import sk.boinc.androboinc.util.ClientId;
import sk.boinc.androboinc.util.HostListDbAdapter;
import sk.boinc.androboinc.util.ScreenOrientationHandler;
import android.app.ListActivity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.database.Cursor;
import android.os.Bundle;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.CursorAdapter;
import android.widget.ListView;
import android.widget.TextView;


public class HostListActivity extends ListActivity {

	private static final int EDIT_ID   = 1;
	private static final int DELETE_ID = 2;

	private static final int ADD_NEW_HOST = 1;
	private static final int EDIT_EXISTING_HOST = 2;
	
	private ScreenOrientationHandler mScreenOrientation;

	private HostListDbAdapter mDbHelper = null;
	private Cursor mHostCursor = null;

	private class HostListAdapter extends CursorAdapter {

		public HostListAdapter(Context context, Cursor c) {
			super(context, c);
		}

		@Override
		public void bindView(View view, Context context, Cursor cursor) {
			TextView text1 = (TextView)view.findViewById(android.R.id.text1);
			text1.setText(cursor.getString(cursor.getColumnIndex(HostListDbAdapter.FIELD_HOST_NICKNAME)));
			TextView text2 = (TextView)view.findViewById(android.R.id.text2);
			String details = cursor.getString(cursor.getColumnIndex(HostListDbAdapter.FIELD_HOST_ADDRESS)) + 
				":" + cursor.getInt(cursor.getColumnIndex(HostListDbAdapter.FIELD_HOST_PORT));
			text2.setText(details);
		}

		@Override
		public View newView(Context context, Cursor cursor, ViewGroup parent) {
			LayoutInflater inflater = LayoutInflater.from(context);
			View v = inflater.inflate(android.R.layout.simple_list_item_2, parent, false);
			bindView(v, context, cursor);
			return v;
		}
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mScreenOrientation = new ScreenOrientationHandler(this);
		setContentView(R.layout.host_list);
		registerForContextMenu(getListView());
		mDbHelper = new HostListDbAdapter(this);
		mDbHelper.open();
		getHostList();
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
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		// We handle only orientation changes
		mScreenOrientation.setOrientation();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		super.onCreateOptionsMenu(menu);
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.host_list_menu, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch(item.getItemId()) {
		case R.id.menuAddHost:
			addHost();
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
		super.onCreateContextMenu(menu, v, menuInfo);
		menu.setHeaderTitle(R.string.hostOperation);
		menu.add(0, EDIT_ID, 0, R.string.hostEdit);
		menu.add(0, DELETE_ID, 0, R.string.hostDelete);
	}

	@Override
	public boolean onContextItemSelected(MenuItem item) {
		AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
		switch(item.getItemId()) {
		case EDIT_ID:
			modifyHost(info.position, info.id);
			return true;
		case DELETE_ID:
			mDbHelper.deleteHost(info.id);
			mHostCursor.requery();
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
		Intent result = new Intent().putExtra(ClientId.TAG, clientId);
		setResult(RESULT_OK, result);
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
				mHostCursor.requery();
			} 
			break;
		case EDIT_EXISTING_HOST:
			if (resultCode == RESULT_OK) {
				// Finished successfully - modified the host
				ClientId clientId = data.getParcelableExtra(ClientId.TAG);
				if (clientId.getId() != -1L) {
					// We have received valid ID of host
					mDbHelper.updateHost(clientId);
					mHostCursor.requery();
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
