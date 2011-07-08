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
import android.content.DialogInterface;
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


public class TasksActivity extends ListActivity implements ClientReplyReceiver {
	private static final String TAG = "TasksActivity";

	private static final int DIALOG_DETAILS = 1;
	private static final int DIALOG_WARN_ABORT = 2;

	private static final int SUSPEND = 1;
	private static final int RESUME = 2;
	private static final int ABORT = 3;
	private static final int PROPERTIES = 4;

	private static final int SB_INIT_CAPACITY = 256;

	private ScreenOrientationHandler mScreenOrientation;

	private ClientId mConnectedClient = null;
	private boolean mRequestUpdates = false;
	private boolean mViewUpdatesAllowed = false;
	private boolean mViewDirty = false;

	private Vector<TaskInfo> mTasks = new Vector<TaskInfo>();
	private int mPosition = 0;

	private StringBuilder mSb = new StringBuilder(SB_INIT_CAPACITY);

	private class SavedState {
		private final Vector<TaskInfo> tasks;

		public SavedState() {
			tasks = mTasks;
			if (Logging.DEBUG) Log.d(TAG, "saved: tasks.size()=" + tasks.size());
		}
		public void restoreState(TasksActivity activity) {
			activity.mTasks = tasks;
			if (Logging.DEBUG) Log.d(TAG, "restored: mTasks.size()=" + activity.mTasks.size());
		}
	}

	private class TaskListAdapter extends BaseAdapter {
		private Context mContext;

		public TaskListAdapter(Context context) {
			mContext = context;
		}

		@Override
		public int getCount() {
            return mTasks.size();
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
			return mTasks.elementAt(position);
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
						R.layout.tasks_list_item, parent, false);
			}
			else {
				layout = convertView;
			}
			TaskInfo task = mTasks.elementAt(position);
			TextView tv;
			tv = (TextView)layout.findViewById(R.id.taskAppName);
			tv.setText(task.application);
			tv = (TextView)layout.findViewById(R.id.taskProjectName);
			tv.setText(task.project);
			tv = (TextView)layout.findViewById(R.id.taskDeadline);
			tv.setText(task.deadline);
			tv = (TextView)layout.findViewById(R.id.taskElapsed);
			tv.setText(task.elapsed);
			tv = (TextView)layout.findViewById(R.id.taskRemaining);
			tv.setText(task.toCompletion);
			tv = (TextView)layout.findViewById(R.id.taskProgressText);
			tv.setText(task.progress);
			ProgressBar progressRunning = (ProgressBar)layout.findViewById(R.id.taskProgressRunning);
			ProgressBar progressWaiting = (ProgressBar)layout.findViewById(R.id.taskProgressWaiting);
			ProgressBar progressSuspended = (ProgressBar)layout.findViewById(R.id.taskProgressSuspended);
			ProgressBar progressFinished = (ProgressBar)layout.findViewById(R.id.taskProgressFinished);
			switch (task.stateControl) {
			case TaskInfo.SUSPENDED:
			case TaskInfo.ABORTED:
			case TaskInfo.ERROR:
				// Either suspended or aborted task
				progressRunning.setVisibility(View.GONE);
				progressWaiting.setVisibility(View.GONE);
				progressFinished.setVisibility(View.GONE);
				progressSuspended.setProgress(task.progInd);
				progressSuspended.setVisibility(View.VISIBLE);
				break;
			case TaskInfo.DOWNLOADING:
			case TaskInfo.UPLOADING:
				// Setting progress to 0 will let the secondary progress to display itself (which is always set to 100)
				task.progInd = 0;
				// no break, we continue following case (READY_TO_REPORT)
			case TaskInfo.READY_TO_REPORT:
				// Downloading, Uploading or Ready to report
				progressWaiting.setVisibility(View.GONE);
				progressSuspended.setVisibility(View.GONE);
				progressRunning.setVisibility(View.GONE);
				progressFinished.setProgress(task.progInd);
				progressFinished.setVisibility(View.VISIBLE);
				break;
			case TaskInfo.RUNNING:
				// Running task
				progressWaiting.setVisibility(View.GONE);
				progressSuspended.setVisibility(View.GONE);
				progressFinished.setVisibility(View.GONE);
				progressRunning.setProgress(task.progInd);
				progressRunning.setVisibility(View.VISIBLE);
				break;
			default:
				// Not running, not suspended, not aborted => waiting for execution
				// or some other state (downloading, exit pending etc)
				progressRunning.setVisibility(View.GONE);
				progressSuspended.setVisibility(View.GONE);
				progressFinished.setVisibility(View.GONE);
				progressWaiting.setProgress(task.progInd);
				progressWaiting.setVisibility(View.VISIBLE);
				if (task.stateControl == TaskInfo.PREEMPTED) {
					// Task already started - set secondary progress to 100
					// The background of progress-bar will be yellow
					progressWaiting.setSecondaryProgress(100);
				}
				else {
					// Task did not started yet - set secondary progress to 0
					// The background of progress-bar will be grey
					progressWaiting.setSecondaryProgress(0);
				}
			}
			return layout;
		}
	}

	private ConnectionManagerService mConnectionManager = null;

	private ServiceConnection mServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mConnectionManager = ((ConnectionManagerService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "onServiceConnected()");
			mConnectionManager.registerStatusObserver(TasksActivity.this);
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
		getApplicationContext().bindService(new Intent(TasksActivity.this, ConnectionManagerService.class),
				mServiceConnection, Context.BIND_AUTO_CREATE);
	}

	private void doUnbindService() {
		if (Logging.DEBUG) Log.d(TAG, "doUnbindService()");
		getApplicationContext().unbindService(mServiceConnection);
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setListAdapter(new TaskListAdapter(this));
		registerForContextMenu(getListView());
		mScreenOrientation = new ScreenOrientationHandler(this);
		doBindService();
		// Restore state on configuration change (if applicable)
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null) {
			// Yes, we have the saved state, this is activity re-creation after configuration change
			savedState.restoreState(this);
			if (!mTasks.isEmpty()) {
				// We restored tasks - view will be updated on resume (before we will get refresh)
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
			mConnectionManager.updateTasks(this);
		}
		mViewUpdatesAllowed = true;
		if (mViewDirty) {
			// There were some updates received while we were not visible
			// The data are stored, but view is not updated yet; Do it now
			sortTasks();
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
			mConnectionManager.updateTasks(this);
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	protected Dialog onCreateDialog(int id) {
		switch (id) {
		case DIALOG_DETAILS:
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_info)
				.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
				.setNegativeButton(R.string.ok, null)
				.create();
		case DIALOG_WARN_ABORT:
	    	return new AlertDialog.Builder(this)
	    		.setIcon(android.R.drawable.ic_dialog_alert)
	    		.setTitle(R.string.warning)
				.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
	    		.setPositiveButton(R.string.taskAbort,
	    			new DialogInterface.OnClickListener() {
	    				public void onClick(DialogInterface dialog, int whichButton) {
	    					TaskInfo task = (TaskInfo)getListAdapter().getItem(mPosition);
	    					mConnectionManager.taskOperation(TasksActivity.this, ClientOp.TASK_ABORT, task.projectUrl, task.taskName);
	    				}
	    			})
	    		.setNegativeButton(R.string.cancel, null)
	    		.create();
		}
		return null;
	}

	@Override
	protected void onPrepareDialog(int id, Dialog dialog) {
		TextView text;
		switch (id) {
		case DIALOG_DETAILS:
			text = (TextView)dialog.findViewById(R.id.dialogText);
			text.setText(Html.fromHtml(prepareTaskDetails(mPosition)));
			break;
		case DIALOG_WARN_ABORT:
			text = (TextView)dialog.findViewById(R.id.dialogText);
			TaskInfo task = (TaskInfo)getListAdapter().getItem(mPosition);
			text.setText(getString(R.string.warnAbortTask, task.taskName));
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
		TaskInfo task = (TaskInfo)getListAdapter().getItem(info.position);
		menu.setHeaderTitle(R.string.taskCtxMenuTitle);
		if (task.stateControl == TaskInfo.SUSPENDED) {
			// project is suspended
			menu.add(0, RESUME, 0, R.string.taskResume);
		}
		else {
			// not suspended
			menu.add(0, SUSPEND, 0, R.string.taskSuspend);
		}
		if (task.stateControl != TaskInfo.ABORTED) {
			// Not aborted
			menu.add(0, ABORT, 0, R.string.taskAbort);
		}
		menu.add(0, PROPERTIES, 0, R.string.taskProperties);
	}

	@Override
	public boolean onContextItemSelected(MenuItem item) {
		AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
		TaskInfo task = (TaskInfo)getListAdapter().getItem(info.position);
		switch(item.getItemId()) {
		case PROPERTIES:
			mPosition = info.position;
			showDialog(DIALOG_DETAILS);
			return true;
		case SUSPEND:

			mConnectionManager.taskOperation(this, ClientOp.TASK_SUSPEND, task.projectUrl, task.taskName);
			return true;
		case RESUME:

			mConnectionManager.taskOperation(this, ClientOp.TASK_RESUME, task.projectUrl, task.taskName);
			return true;
		case ABORT:
			mPosition = info.position;
			showDialog(DIALOG_WARN_ABORT);
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
				// Request fresh data
				mConnectionManager.updateTasks(this);
			}
		}
	}

	@Override
	public void clientDisconnected() {
		if (Logging.DEBUG) Log.d(TAG, "Client is disconnected");
		mConnectedClient = null;
		mTasks.clear();
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
		// Just ignore
		return false;
	}

	@Override
	public boolean updatedTasks(Vector<TaskInfo> tasks) {
		mTasks = tasks;
		if (mViewUpdatesAllowed) {
			// We are visible, update the view with fresh data
			if (Logging.DEBUG) Log.d(TAG, "Tasks are updated, refreshing view");
			sortTasks();
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
		}
		else {
			// We are not visible, do not perform costly tasks now
			if (Logging.DEBUG) Log.d(TAG, "Tasks are updated, but view refresh is delayed");
			mViewDirty = true;
		}
		return mRequestUpdates;
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

	// Comparison array
	// Index in array is the stateControl of task:
	// (not set) 0
	// DOWNLOADING = 1;
	// READY_TO_START = 2;
	// RUNNING = 3;
	// PREEMPTED = 4;
	// UPLOADING = 5;
	// READY_TO_REPORT = 6;
	// SUSPENDED = 7;
	// ABORTED = 8;
	// ERROR = 9;
	// Value in array is the order of the task (lower value will be sorted before higher)
	private static final int[] cStatePriority = { 99, 5, 5, 1, 2, 4, 4, 3, 5, 5 };
	// Order is: (1) RUNNING -> (2) PREEMPTED -> (3) SUSPENDED -> (4) UPLOADING & READY_TO_REPORT -> 
	//        -> (5) ABORTED, ERROR, DOWNLOADING, READY_TO_START -> (last) others - not set states

	private void sortTasks() {
		Comparator<TaskInfo> comparator = new Comparator<TaskInfo>() {
			@Override
			public int compare(TaskInfo object1, TaskInfo object2) {
				// First criteria - state
				if ( (cStatePriority[object1.stateControl] - cStatePriority[object2.stateControl]) != 0 ) {
					// The priorities for are different - return the order
					return (cStatePriority[object1.stateControl] - cStatePriority[object2.stateControl]);
				}
				// Otherwise continue with further criteria
				// The next criteria - deadline
				int deadlineDiff = (int)(object1.deadlineNum - object2.deadlineNum);
				if (deadlineDiff != 0) {
					// not the same deadline
					return deadlineDiff;
				}
				// Last, sort by project name, then by task name
				int prjComp = object1.project.compareToIgnoreCase(object2.project);
				if (prjComp != 0) {
					return prjComp;
				}
				return object1.taskName.compareToIgnoreCase(object2.taskName);
			}
		};
		Collections.sort(mTasks, comparator);
	}

	private String prepareTaskDetails(int position) {
		TaskInfo task = mTasks.elementAt(position);
		mSb.setLength(0);
		mSb.append(getString(R.string.taskDetailedInfoCommon, 
				TextUtils.htmlEncode(task.taskName),
				TextUtils.htmlEncode(task.project),
				TextUtils.htmlEncode(task.application),
				task.progress,
				task.elapsed,
				task.toCompletion,
				task.deadline));
		if (task.virtMemSize != null) {
			mSb.append(getString(R.string.taskDetailedInfoRun, 
					task.virtMemSize,
					task.workSetSize,
					task.cpuTime,
					task.chckpntTime));
		}
		if (task.resources != null) {
			mSb.append(getString(R.string.taskDetailedInfoRes, task.resources));
		}
		mSb.append(getString(R.string.taskDetailedInfoEnd, task.state));
		if (Logging.DEBUG) Log.d(TAG, "mSb.length()=" + mSb.length() + ", mSb.capacity()=" + mSb.capacity());
		return mSb.toString();
	}
}
