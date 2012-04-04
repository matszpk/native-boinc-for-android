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

package sk.boinc.nativeboinc;


import java.util.Collections;
import java.util.Comparator;
import java.util.ArrayList;
import java.util.HashSet;

import sk.boinc.nativeboinc.bridge.AutoRefresh;
import sk.boinc.nativeboinc.clientconnection.AutoRefreshListener;
import sk.boinc.nativeboinc.clientconnection.ClientOp;
import sk.boinc.nativeboinc.clientconnection.ClientUpdateTasksReceiver;
import sk.boinc.nativeboinc.clientconnection.TaskDescriptor;
import sk.boinc.nativeboinc.clientconnection.TaskInfo;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.service.ConnectionManagerService;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.ScreenOrientationHandler;
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
import android.os.SystemClock;
import android.text.Html;
import android.text.TextUtils;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.AdapterView.AdapterContextMenuInfo;


public class TasksActivity extends ListActivity implements ClientUpdateTasksReceiver,
		AutoRefreshListener {
	private static final String TAG = "TasksActivity";

	private static final int DIALOG_DETAILS = 1;
	private static final int DIALOG_WARN_ABORT = 2;

	private static final int SUSPEND = 1;
	private static final int RESUME = 2;
	private static final int ABORT = 3;
	private static final int PROPERTIES = 4;
	private static final int SELECT_ALL = 5;
	private static final int UNSELECT_ALL = 6;

	private static final int SB_INIT_CAPACITY = 256;

	private ScreenOrientationHandler mScreenOrientation;

	private ClientId mConnectedClient = null;
	private boolean mRequestUpdates = false;
	private boolean mViewUpdatesAllowed = false;
	private boolean mViewDirty = false;

	private ArrayList<TaskInfo> mTasks = new ArrayList<TaskInfo>();
	private ArrayList<TaskInfo> mPendingTasks = new ArrayList<TaskInfo>();
	private HashSet<TaskDescriptor> mSelectedTasks = new HashSet<TaskDescriptor>();
	private boolean mShowDetailsDialog = false;
	private boolean mShowWarnAbortDialog = false;
	private TaskInfo mChoosenTask = null; 

	private boolean mUpdateTasksInProgress = false;
	private long mLastUpdateTime = -1;
	private boolean mAfterRecreating = false;
	
	private StringBuilder mSb = new StringBuilder(SB_INIT_CAPACITY);

	private static class SavedState {
		private final ArrayList<TaskInfo> tasks;
		private final ArrayList<TaskInfo> pendingTasks;
		private final HashSet<TaskDescriptor> selectedTasks;
		private final TaskInfo choosenTask;
		private final boolean showDetailsDialog;
		private final boolean showWarnAbortDialog;
		private final boolean updateTasksInProgress;
		private final long lastUpdateTime;

		public SavedState(TasksActivity activity) {
			tasks = activity.mTasks;
			if (Logging.DEBUG) Log.d(TAG, "saved: tasks.size()=" + tasks.size());
			pendingTasks = activity.mPendingTasks;
			selectedTasks = activity.mSelectedTasks;
			//position = activity.mPosition;
			choosenTask = activity.mChoosenTask;
			showDetailsDialog = activity.mShowDetailsDialog;
			showWarnAbortDialog = activity.mShowWarnAbortDialog;
			updateTasksInProgress = activity.mUpdateTasksInProgress;
			lastUpdateTime = activity.mLastUpdateTime;
		}
		public void restoreState(TasksActivity activity) {
			activity.mTasks = tasks;
			if (Logging.DEBUG) Log.d(TAG, "restored: mTasks.size()=" + activity.mTasks.size());
			activity.mPendingTasks = pendingTasks;
			activity.mSelectedTasks = selectedTasks;
			//activity.mPosition = position;
			activity.mChoosenTask = choosenTask;
			activity.mShowDetailsDialog = showDetailsDialog;
			activity.mShowWarnAbortDialog = showWarnAbortDialog;
			activity.mUpdateTasksInProgress = updateTasksInProgress;
			activity.mLastUpdateTime = lastUpdateTime;
		}
	}
	
	private class TaskOnClickListener implements View.OnClickListener {
		private int mItemPosition;
		
		public TaskOnClickListener(int position) {
			mItemPosition = position;
		}
		
		public void setItemPosition(int position) {
			mItemPosition = position;
		}
		
		@Override
		public void onClick(View view) {
			TaskInfo task = mTasks.get(mItemPosition);
			if (Logging.DEBUG) Log.d(TAG, "Task "+task.taskName+" change checked:"+mItemPosition);
			
			TaskDescriptor taskDesc = new TaskDescriptor(task.projectUrl, task.taskName);
			if (mSelectedTasks.contains(taskDesc)) // remove
				mSelectedTasks.remove(taskDesc);
			else // add to selected
				mSelectedTasks.add(taskDesc);
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
			return mTasks.get(position);
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
			
			TaskInfo task = mTasks.get(position);
			
			CheckBox checkBox = (CheckBox)layout.findViewById(R.id.check);
			checkBox.setChecked(mSelectedTasks.contains(new TaskDescriptor(task.projectUrl, task.taskName)));
			
			TaskOnClickListener clickListener = (TaskOnClickListener)layout.getTag();
			if (clickListener == null) {
				clickListener = new TaskOnClickListener(position);
				layout.setTag(clickListener);
			} else {
				clickListener.setItemPosition(position);
			}
			checkBox.setOnClickListener(clickListener);
			
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
					progressWaiting.setSecondaryProgress(1000);
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
			mAfterRecreating = true;
		}
	}

	@Override
	protected void onResume() {
		super.onResume();
		mScreenOrientation.setOrientation();
		mRequestUpdates = true;
		
		Log.d(TAG, "onUpdataskprogress:"+mUpdateTasksInProgress);
		if (mConnectedClient != null) {
			if (mUpdateTasksInProgress) {
				ArrayList<TaskInfo> tasks = mConnectionManager.getPendingTasks();
				if (tasks != null) // if already updated
					updatedTasks(tasks);
				
				mConnectionManager.addToScheduledUpdates(this, AutoRefresh.TASKS, -1);
			} else { // if after update
				int autoRefresh = mConnectionManager.getAutoRefresh();
				
				if (autoRefresh != -1) {
					long period = SystemClock.elapsedRealtime()-mLastUpdateTime;
					if (period >= autoRefresh*1000) {
						// if later than 4 seconds
						if (Logging.DEBUG) Log.d(TAG, "do update tasks");
						mConnectionManager.updateTasks();
					} else { // only add auto updates
						if (Logging.DEBUG) Log.d(TAG, "do add to schedule update tasks");
						mConnectionManager.addToScheduledUpdates(this, AutoRefresh.TASKS,
								(int)(autoRefresh*1000-period));
					}
				}
			}
		}
		mViewUpdatesAllowed = true;
		if (mViewDirty) {
			// There were some updates received while we were not visible
			// The data are stored, but view is not updated yet; Do it now
			mTasks = mPendingTasks;
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
			mViewDirty = false;
			if (Logging.DEBUG) Log.d(TAG, "Delayed refresh of view was done now");
		}
		if (mShowDetailsDialog) {
			if (mChoosenTask != null) {
				showDialog(DIALOG_DETAILS);
			} else {
				if (Logging.DEBUG) Log.d(TAG, "If failed to recreate details");
				mShowDetailsDialog = false;
			}
		}
		if (mShowWarnAbortDialog) {
			if (mChoosenTask != null) {
				showDialog(DIALOG_WARN_ABORT);
			} else {
				if (Logging.DEBUG) Log.d(TAG, "If failed to recreate warn abort");
				mShowWarnAbortDialog = false;
			}
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
			mConnectionManager.cancelScheduledUpdates(AutoRefresh.TASKS);
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
		return new SavedState(this);
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		Activity parent = getParent();
		if (parent != null) {
			return parent.onKeyDown(keyCode, event);
		}
		return super.onKeyDown(keyCode, event);
	}

	private void onDismissDetailsDialog() {
		if (Logging.DEBUG) Log.d(TAG, "On dismiss details");
		mShowDetailsDialog = false;
	}
	
	private void onDismissWarnAbortDialog() {
		if (Logging.DEBUG) Log.d(TAG, "On dismiss warn abort");
		mShowWarnAbortDialog = false;
	}
	
	@Override
	protected Dialog onCreateDialog(int id) {
		switch (id) {
		case DIALOG_DETAILS:
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_info)
				.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
				.setNegativeButton(R.string.ok, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						onDismissDetailsDialog();
					}
				})
				.setOnCancelListener(new DialogInterface.OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						onDismissDetailsDialog();
					}
				})
				.create();
		case DIALOG_WARN_ABORT:
	    	return new AlertDialog.Builder(this)
	    		.setIcon(android.R.drawable.ic_dialog_alert)
	    		.setTitle(R.string.warning)
				.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
	    		.setPositiveButton(R.string.taskAbort,
	    			new DialogInterface.OnClickListener() {
	    				public void onClick(DialogInterface dialog, int whichButton) {
	    					//TaskInfo task = (TaskInfo)getListAdapter().getItem(mPosition);
	    					
	    					onDismissWarnAbortDialog();
	    					
	    					if (mSelectedTasks.isEmpty()) {
		    					mConnectionManager.taskOperation(ClientOp.TASK_ABORT,
		    							mChoosenTask.projectUrl, mChoosenTask.taskName);
	    					} else
	    						mConnectionManager.tasksOperation(ClientOp.TASK_ABORT,
	    								mSelectedTasks.toArray(new TaskDescriptor[0]));
	    				}
	    			})
	    		.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						onDismissWarnAbortDialog();
					}
				})
				.setOnCancelListener(new DialogInterface.OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						onDismissWarnAbortDialog();
					}
				})
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
			text.setText(Html.fromHtml(prepareTaskDetails(mChoosenTask)));
			break;
		case DIALOG_WARN_ABORT:
			text = (TextView)dialog.findViewById(R.id.dialogText);
			if (mSelectedTasks.isEmpty())
				text.setText(getString(R.string.warnAbortTask, mChoosenTask.taskName));
			else
				text.setText(getString(R.string.warnAbortTasks));
			break;
		}
	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {
		mChoosenTask = (TaskInfo)getListAdapter().getItem(position);
		mShowDetailsDialog = true;
		showDialog(DIALOG_DETAILS);
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
		super.onCreateContextMenu(menu, v, menuInfo);
		AdapterContextMenuInfo info = (AdapterContextMenuInfo)menuInfo;
		TaskInfo task = (TaskInfo)getListAdapter().getItem(info.position);
		menu.setHeaderTitle(R.string.taskCtxMenuTitle);
		if (mSelectedTasks.size() != mTasks.size())
			menu.add(0, SELECT_ALL, 0, R.string.selectAll);
		
		if (!mSelectedTasks.isEmpty())
			menu.add(0, UNSELECT_ALL, 0, R.string.unselectAll);
		
		if (mSelectedTasks.isEmpty()) {
			// only one task
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
		} else {
			// selected tasks
			menu.add(0, RESUME, 0, R.string.taskResume);
			menu.add(0, SUSPEND, 0, R.string.taskSuspend);
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
			mChoosenTask = task;
			showDialog(DIALOG_DETAILS);
			return true;
		case SUSPEND:
			if (mSelectedTasks.isEmpty())
				mConnectionManager.taskOperation(ClientOp.TASK_SUSPEND, task.projectUrl, task.taskName);
			else
				mConnectionManager.tasksOperation(ClientOp.TASK_SUSPEND,
						mSelectedTasks.toArray(new TaskDescriptor[0]));
			return true;
		case RESUME:
			if (mSelectedTasks.isEmpty())
				mConnectionManager.taskOperation(ClientOp.TASK_RESUME, task.projectUrl, task.taskName);
			else
				mConnectionManager.tasksOperation(ClientOp.TASK_RESUME,
						mSelectedTasks.toArray(new TaskDescriptor[0]));
			return true;
		case ABORT:
			mChoosenTask = task;
			mShowWarnAbortDialog = true;
			showDialog(DIALOG_WARN_ABORT);
			return true;
		case SELECT_ALL:
			for (TaskInfo thisTask: mTasks)
				mSelectedTasks.add(new TaskDescriptor(thisTask.projectUrl, thisTask.taskName));
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
			return true;
		case UNSELECT_ALL:
			mSelectedTasks.clear();
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
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
				if (!mUpdateTasksInProgress && !mAfterRecreating) {
					if (Logging.DEBUG) Log.d(TAG, "do update tasks");
					mUpdateTasksInProgress = true;
					mConnectionManager.updateTasks();
				} else {
					if (Logging.DEBUG) Log.d(TAG, "do add to scheduled updates");
					int autoRefresh = mConnectionManager.getAutoRefresh();
					
					if (autoRefresh != -1) {
						long period = SystemClock.elapsedRealtime()-mLastUpdateTime;
						mConnectionManager.addToScheduledUpdates(this, AutoRefresh.TASKS,
								(int)(autoRefresh*1000-period));
					} else
						mConnectionManager.addToScheduledUpdates(this, AutoRefresh.TASKS, -1);
					mAfterRecreating = false;
				}
			}
		}
	}

	@Override
	public void clientDisconnected() {
		if (Logging.DEBUG) Log.d(TAG, "Client is disconnected");
		mConnectedClient = null;
		mUpdateTasksInProgress = false;
		mTasks.clear();
		updateSelectedTasks();
		((BaseAdapter)getListAdapter()).notifyDataSetChanged();
		mViewDirty = false;
		mAfterRecreating = false;
	}
	
	@Override
	public boolean clientError(int err_num, String message) {
		// do not consume
		mUpdateTasksInProgress = false;
		mAfterRecreating = false;
		return false;
	}
	
	@Override
	public boolean updatedTasks(ArrayList<TaskInfo> tasks) {
		mPendingTasks = tasks;
		mUpdateTasksInProgress = false;
		mLastUpdateTime = SystemClock.elapsedRealtime();
		
		sortTasks();
		updateSelectedTasks();
		
		if (mViewUpdatesAllowed) {
			// We are visible, update the view with fresh data
			mTasks = mPendingTasks;
			if (Logging.DEBUG) Log.d(TAG, "Tasks are updated, refreshing view");
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
		}
		else {
			// We are not visible, do not perform costly tasks now
			if (Logging.DEBUG) Log.d(TAG, "Tasks are updated, but view refresh is delayed");
			mViewDirty = true;
		}
		return mRequestUpdates;
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
		Collections.sort(mPendingTasks, comparator);
	}
	
	private void updateSelectedTasks() {
		ArrayList<TaskDescriptor> toRetain = new ArrayList<TaskDescriptor>();
		for (TaskInfo taskInfo: mPendingTasks)
			toRetain.add(new TaskDescriptor(taskInfo.projectUrl, taskInfo.taskName));
		mSelectedTasks.retainAll(toRetain);
		//if (Logging.DEBUG) Log.d(TAG, "update selected tasks:"+mSelectedTasks);
	}

	private String prepareTaskDetails(TaskInfo task) {
		//TaskInfo task = mTasks.get(position);
		mSb.setLength(0);
		mSb.append(getString(R.string.taskDetailedInfoCommon, 
				TextUtils.htmlEncode(task.taskName),
				TextUtils.htmlEncode(task.project),
				TextUtils.htmlEncode(task.application),
				task.received_time,
				task.progress,
				task.elapsed,
				task.toCompletion,
				task.deadline,
				task.rsc_fpops_est,
				task.rsc_memory_bound));
		if (task.virtMemSize != null) {
			mSb.append(getString(R.string.taskDetailedInfoRun, 
					task.virtMemSize,
					task.workSetSize,
					task.cpuTime,
					task.chckpntTime));
			if (task.pid != 0)
				mSb.append(getString(R.string.taskDetailedInfoRunPid,
						task.pid));
			if (task.directory != null)
				mSb.append(getString(R.string.taskDetailedInfoRunDir,
						task.directory));
		}
		if (task.resources != null) {
			mSb.append(getString(R.string.taskDetailedInfoRes, task.resources));
		}
		mSb.append(getString(R.string.taskDetailedInfoEnd, task.state));
		if (Logging.DEBUG) Log.d(TAG, "mSb.length()=" + mSb.length() + ", mSb.capacity()=" + mSb.capacity());
		return mSb.toString();
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onStartAutoRefresh(int requestType) {
		Log.d(TAG, "on start auto refresh");
		if (requestType == AutoRefresh.TASKS) // in progress
			mUpdateTasksInProgress = true;
	}
}
