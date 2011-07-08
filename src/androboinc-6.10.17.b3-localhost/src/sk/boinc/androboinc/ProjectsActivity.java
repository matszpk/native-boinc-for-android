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

import java.util.Collections;
import java.util.Comparator;
import java.util.Vector;

import sk.boinc.androboinc.clientconnection.ClientOp;
import sk.boinc.androboinc.clientconnection.ClientReplyReceiver;
import sk.boinc.androboinc.clientconnection.HostInfo;
import sk.boinc.androboinc.clientconnection.MessageInfo;
import sk.boinc.androboinc.clientconnection.ModeInfo;
import sk.boinc.androboinc.clientconnection.ProjectInfo;
import sk.boinc.androboinc.clientconnection.TaskInfo;
import sk.boinc.androboinc.clientconnection.TransferInfo;
import sk.boinc.androboinc.clientconnection.VersionInfo;
import sk.boinc.androboinc.debug.Logging;
import sk.boinc.androboinc.service.ConnectionManagerService;
import sk.boinc.androboinc.util.ClientId;
import sk.boinc.androboinc.util.ScreenOrientationHandler;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.text.Html;
import android.text.TextUtils;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.AdapterView.AdapterContextMenuInfo;


public class ProjectsActivity extends ListActivity implements ClientReplyReceiver {
	private static final String TAG = "ProjectsActivity";

	private static final int DIALOG_DETAILS = 1;

	private static final int SUSPEND = 1;
	private static final int RESUME = 2;
	private static final int NNW = 3;
	private static final int ANW = 4;
	private static final int UPDATE = 5;
	private static final int PROPERTIES = 6;

	private ScreenOrientationHandler mScreenOrientation;

	private boolean mRequestUpdates = false;
	private boolean mViewUpdatesAllowed = false;
	private boolean mViewDirty = false;

	private Vector<ProjectInfo> mProjs = new Vector<ProjectInfo>();
	private int mPosition = 0;

	private class SavedState {
		private final Vector<ProjectInfo> projs;

		public SavedState() {
			projs = mProjs;
			if (Logging.DEBUG) Log.d(TAG, "saved: projs.size()=" + projs.size());
		}
		public void restoreState(ProjectsActivity activity) {
			activity.mProjs = projs;
			if (Logging.DEBUG) Log.d(TAG, "restored: mProjs.size()=" + activity.mProjs.size());
		}
	}


	private class ProjectListAdapter extends BaseAdapter {
		private Context mContext;

		public ProjectListAdapter(Context context) {
			mContext = context;
		}

		@Override
		public int getCount() {
            return mProjs.size();
        }

		@Override
		public boolean areAllItemsEnabled() {
			return true;
		}

		@Override
		public boolean isEnabled(int position) {
			return true;
		}

		@Override
		public Object getItem(int position) {
			return mProjs.elementAt(position);
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			View layout;
			if (convertView == null) {
				layout = LayoutInflater.from(mContext).inflate(
						R.layout.projects_list_item, parent, false);
			}
			else {
				layout = convertView;
			}
			ProjectInfo proj = mProjs.elementAt(position);
			TextView title = (TextView)layout.findViewById(R.id.projectName);
			title.setText(proj.project);
			ProgressBar shareActive = (ProgressBar)layout.findViewById(R.id.projectShareActive);
			ProgressBar shareNNW = (ProgressBar)layout.findViewById(R.id.projectShareNNW);
			ProgressBar shareSuspended = (ProgressBar)layout.findViewById(R.id.projectShareSuspended);
			if ((proj.statusId & ProjectInfo.SUSPENDED) == ProjectInfo.SUSPENDED) {
				shareActive.setVisibility(View.GONE);
				shareNNW.setVisibility(View.GONE);
				shareSuspended.setProgress(proj.resShare);
				shareSuspended.setVisibility(View.VISIBLE);
			}
			else if ((proj.statusId & ProjectInfo.NNW) == ProjectInfo.NNW) {
				shareActive.setVisibility(View.GONE);
				shareSuspended.setVisibility(View.GONE);
				shareNNW.setProgress(proj.resShare);
				shareNNW.setVisibility(View.VISIBLE);
			}
			else {
				shareNNW.setVisibility(View.GONE);
				shareSuspended.setVisibility(View.GONE);
				shareActive.setProgress(proj.resShare);
				shareActive.setVisibility(View.VISIBLE);
			}
			TextView shareText = (TextView)layout.findViewById(R.id.projectShareText);
			shareText.setText(proj.share);
			TextView status = (TextView)layout.findViewById(R.id.projectDetails);
			status.setText(getString(R.string.projectCredits, 
					proj.user_credit, proj.user_rac, 
					proj.host_credit, proj.host_rac));
			return layout;
		}
	}

	private ConnectionManagerService mConnectionManager = null;
	private ClientId mConnectedClient = null;

	private ServiceConnection mServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mConnectionManager = ((ConnectionManagerService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "onServiceConnected()");
			mConnectionManager.registerStatusObserver(ProjectsActivity.this);
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mConnectionManager = null;
			// This should not happen normally, because it's local service 
			// running in the same process...
			if (Logging.WARNING) Log.w(TAG, "onServiceDisconnected()");
		}
	};

	private void doBindService() {
		if (Logging.DEBUG) Log.d(TAG, "doBindService()");
		getApplicationContext().bindService(new Intent(ProjectsActivity.this, ConnectionManagerService.class),
				mServiceConnection, Context.BIND_AUTO_CREATE);
	}

	private void doUnbindService() {
		if (Logging.DEBUG) Log.d(TAG, "doUnbindService()");
		getApplicationContext().unbindService(mServiceConnection);
	}


	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setListAdapter(new ProjectListAdapter(this));
		registerForContextMenu(getListView());
		mScreenOrientation = new ScreenOrientationHandler(this);
		doBindService();
		// Restore state on configuration change (if applicable)
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null) {
			// Yes, we have the saved state, this is activity re-creation after configuration change
			savedState.restoreState(this);
			if (!mProjs.isEmpty()) {
				// We restored projects - view will be updated on resume (before we will get refresh)
				mViewDirty = true;
			}
		}
	}

	@Override
	protected void onResume() {
		super.onResume();
		mScreenOrientation.setOrientation();
		mRequestUpdates = true;
		if (mConnectedClient != null) {
			// We are connected right now, request fresh data
			if (Logging.DEBUG) Log.d(TAG, "onResume() - Starting refresh of data");
			mConnectionManager.updateProjects(this);
		}
		mViewUpdatesAllowed = true;
		if (mViewDirty) {
			// There were some updates received while we were not visible
			// The data are stored, but view is not updated yet; Do it now
			sortProjects();
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
			mViewDirty = false;
			if (Logging.DEBUG) Log.d(TAG, "Delayed refresh of view was done now");
		}
	}

	@Override
	protected void onPause() {
		super.onPause();
		// We shall not request data updates
		mRequestUpdates = false;
		mViewUpdatesAllowed = false;
		// Also remove possibly scheduled automatic updates
		if (mConnectionManager != null) {
			mConnectionManager.cancelScheduledUpdates(this);
		}
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		if (mConnectionManager != null) {
			mConnectionManager.unregisterStatusObserver(this);
			mConnectedClient = null;
		}
		doUnbindService();
		mScreenOrientation = null;
	}

	@Override
	public Object onRetainNonConfigurationInstance() {
		final SavedState savedState = new SavedState();
		return savedState;
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		Activity parent = getParent();
		if (parent != null) {
			return parent.onKeyDown(keyCode, event);
		}
		return super.onKeyDown(keyCode, event);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		super.onCreateOptionsMenu(menu);
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.refresh_menu, menu);
		return true;
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		super.onPrepareOptionsMenu(menu);
		MenuItem item = menu.findItem(R.id.menuRefresh);
		item.setVisible(mConnectedClient != null);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.menuRefresh:
			mConnectionManager.updateProjects(this);
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	protected Dialog onCreateDialog(int id) {
		switch (id) {
		case DIALOG_DETAILS:
			if (Logging.DEBUG) Log.d(TAG, "onCreateDialog(DIALOG_DETAILS)");
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_info)
				.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
				.setNegativeButton(R.string.ok, null)
				.create();
		}
		return null;
	}

	@Override
	protected void onPrepareDialog(int id, Dialog dialog) {
		switch (id) {
		case DIALOG_DETAILS:
			TextView text = (TextView)dialog.findViewById(R.id.dialogText);
			text.setText(Html.fromHtml(prepareProjectDetails(mPosition)));
			break;
		}
	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {
		mPosition = position;
		showDialog(DIALOG_DETAILS);
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
		super.onCreateContextMenu(menu, v, menuInfo);
		AdapterContextMenuInfo info = (AdapterContextMenuInfo)menuInfo;
		ProjectInfo proj = (ProjectInfo)getListAdapter().getItem(info.position);
		menu.setHeaderTitle(R.string.projectCtxMenuTitle);
		menu.add(0, UPDATE, 0, R.string.projectUpdate);
		if ((proj.statusId & ProjectInfo.SUSPENDED) == ProjectInfo.SUSPENDED) {
			// project is suspended
			menu.add(0, RESUME, 0, R.string.projectResume);
		}
		else {
			// not suspended
			menu.add(0, SUSPEND, 0, R.string.projectSuspend);
		}
		if ((proj.statusId & ProjectInfo.NNW) == ProjectInfo.NNW) {
			// No-New-Work is currently set
			menu.add(0, ANW, 0, R.string.projectANW);
		}
		else {
			// New work is allowed
			menu.add(0, NNW, 0, R.string.projectNNW);
		}
		menu.add(0, PROPERTIES, 0, R.string.projectProperties);
	}

	@Override
	public boolean onContextItemSelected(MenuItem item) {
		AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
		ProjectInfo proj = (ProjectInfo)getListAdapter().getItem(info.position);
		switch(item.getItemId()) {
		case PROPERTIES:
			mPosition = info.position;
			showDialog(DIALOG_DETAILS);
			return true;
		case UPDATE:
			mConnectionManager.projectOperation(this, ClientOp.PROJECT_UPDATE, proj.masterUrl);
			return true;
		case SUSPEND:
			mConnectionManager.projectOperation(this, ClientOp.PROJECT_SUSPEND, proj.masterUrl);
			return true;
		case RESUME:
			mConnectionManager.projectOperation(this, ClientOp.PROJECT_RESUME, proj.masterUrl);
			return true;
		case NNW:
			mConnectionManager.projectOperation(this, ClientOp.PROJECT_NNW, proj.masterUrl);
			return true;
		case ANW:
			mConnectionManager.projectOperation(this, ClientOp.PROJECT_ANW, proj.masterUrl);
			return true;
		}
		return super.onContextItemSelected(item);
	}


	@Override
	public void clientConnectionProgress(int progress) {
		// We don't care about progress indicator in this activity, just ignore this
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		mConnectedClient = mConnectionManager.getClientId();
		if (mConnectedClient != null) {
			// Connected client is retrieved
			if (Logging.DEBUG) Log.d(TAG, "Client is connected");
			if (mRequestUpdates) {
				mConnectionManager.updateProjects(this);
			}
		}
	}

	@Override
	public void clientDisconnected() {
		if (Logging.DEBUG) Log.d(TAG, "Client is disconnected");
		mConnectedClient = null;
		mProjs.clear();
		((BaseAdapter)getListAdapter()).notifyDataSetChanged();
		mViewDirty = false;
	}

	@Override
	public boolean updatedClientMode(ModeInfo modeInfo) {
		// Just ignore
		return false;
	}

	@Override
	public boolean updatedHostInfo(HostInfo hostInfo) {
		// Just ignore
		return false;
	}

	@Override
	public boolean updatedProjects(Vector<ProjectInfo> projects) {
		mProjs = projects;
		if (mViewUpdatesAllowed) {
			// We are visible, update the view with fresh data
			if (Logging.DEBUG) Log.d(TAG, "Projects are updated, refreshing view");
			sortProjects();
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
		}
		else {
			// We are not visible, do not perform costly tasks now
			if (Logging.DEBUG) Log.d(TAG, "Projects are updated, but view refresh is delayed");
			mViewDirty = true;
		}
		return mRequestUpdates;
	}

	@Override
	public boolean updatedTasks(Vector<TaskInfo> tasks) {
		// Just ignore
		return false;
	}

	@Override
	public boolean updatedTransfers(Vector<TransferInfo> transfers) {
		// Just ignore
		return false;
	}

	@Override
	public boolean updatedMessages(Vector<MessageInfo> messages) {
		// Just ignore
		return false;
	}


	private void sortProjects() {
		Comparator<ProjectInfo> comparator = new Comparator<ProjectInfo>() {
			@Override
			public int compare(ProjectInfo object1, ProjectInfo object2) {
				return object1.project.compareToIgnoreCase(object2.project);
			}
		};
		Collections.sort(mProjs, comparator);
	}

	private String prepareProjectDetails(int position) {
		ProjectInfo proj = mProjs.elementAt(position);
		return getString(R.string.projectDetailedInfo, 
				TextUtils.htmlEncode(proj.project),
				TextUtils.htmlEncode(proj.account),
				TextUtils.htmlEncode(proj.team),
				proj.user_credit, 
				proj.user_rac, 
				proj.host_credit, 
				proj.host_rac,
				proj.share,
				proj.status);
	}
}
