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

package sk.boinc.nativeboinc.widget;

import java.util.Arrays;
import java.util.Comparator;

import sk.boinc.nativeboinc.BoincManagerActivity;
import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.ScreenLockActivity;
import sk.boinc.nativeboinc.ShutdownDialogActivity;
import sk.boinc.nativeboinc.TaskInfoDialogActivity;
import sk.boinc.nativeboinc.clientconnection.TaskInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.util.TaskItem;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.PendingIntent.CanceledException;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Parcelable;
import android.util.Log;
import android.view.View;
import android.widget.RemoteViews;

/**
 * @author mat
 *
 */
public class TabletWidgetProvider extends AppWidgetProvider {
	private static final String TAG = "TabletWidgetProvider";
	
	public static final String NATIVE_BOINC_WIDGET_PREPARE_UPDATE = "sk.boinc.nativeboinc.widget.TABLET_WIDGET_PREPARE_UPDATE";
	public static final String NATIVE_BOINC_WIDGET_UPDATE = "sk.boinc.nativeboinc.widget.TABLET_WIDGET_UPDATE";
	public static final String NATIVE_BOINC_CLIENT_START_STOP = "sk.boinc.nativeboinc.widget.TABLET_CLIENT_START_STOP";
	public static final String NATIVE_BOINC_CLIENT_TASK_INFO = "sk.boinc.nativeboinc.widget.TABLET_CLIENT_TASK_INFO_";
	
	public static final String WIDGET_CLIENT_START = "WidgetClientStart";
	public static final String WIDGET_UPDATE_CHANGED = "WidgetUpdateChanged";
	
	@Override
	public void onEnabled(Context context) {
		super.onEnabled(context);
		
		if (Logging.DEBUG) Log.d(TAG, "Enabled native periodically");
		BoincManagerApplication appContext = (BoincManagerApplication)context.getApplicationContext();
		
		int updatePeriod = appContext.getWigetUpdatePeriod();
		
		/* first update */
		Intent intent = new Intent(NATIVE_BOINC_WIDGET_UPDATE);
		PendingIntent pendingIntent = PendingIntent.getBroadcast(appContext, 0, intent, 
				PendingIntent.FLAG_UPDATE_CURRENT);
		
		try {
			if (Logging.DEBUG) Log.d(TAG, "Send update intent");
			pendingIntent.send();
		} catch (Exception ex) { }
		
		/* updating periodically */
		AlarmManager alarmManager = (AlarmManager)appContext.getSystemService(Context.ALARM_SERVICE);
		intent = new Intent(NATIVE_BOINC_WIDGET_PREPARE_UPDATE);
		pendingIntent = PendingIntent.getBroadcast(appContext, 0, intent, 
				PendingIntent.FLAG_UPDATE_CURRENT);
		alarmManager.setRepeating(AlarmManager.RTC_WAKEUP, System.currentTimeMillis(),
				updatePeriod, pendingIntent);
	}
	
	private void disableAutoRefresh(Context context) {
		if (Logging.DEBUG) Log.d(TAG, "Disabled native periodically");
		BoincManagerApplication appContext = (BoincManagerApplication)context.getApplicationContext();
		/* cancel updating periodically */
		AlarmManager alarmManager = (AlarmManager)appContext.getSystemService(Context.ALARM_SERVICE);
		Intent intent = new Intent(NATIVE_BOINC_WIDGET_PREPARE_UPDATE);
		PendingIntent pendingIntent = PendingIntent.getBroadcast(appContext, 0, intent, 
				PendingIntent.FLAG_UPDATE_CURRENT);
		alarmManager.cancel(pendingIntent);
	}
	
	@Override
	public void onDisabled(Context context) {
		super.onDisabled(context);
		disableAutoRefresh(context);
	}
	
	/* check widgets */
	private static boolean isWidgetEnabled(Context context) {
		AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(context);
		
		ComponentName thisAppWidget = new ComponentName(context.getPackageName(),
				TabletWidgetProvider.class.getName());
		int[] widgetIds = appWidgetManager.getAppWidgetIds(thisAppWidget);
		
		return (widgetIds != null && widgetIds.length != 0);
	}
	
	@Override
	public void onReceive(Context context, Intent inputIntent) {
		super.onReceive(context, inputIntent);
		
		if (inputIntent == null || inputIntent.getAction() == null)
			return;
		
		BoincManagerApplication appContext = (BoincManagerApplication)context.getApplicationContext();
		
		if (!isWidgetEnabled(appContext))
			return;
		
		if (inputIntent.getAction().equals(NATIVE_BOINC_WIDGET_PREPARE_UPDATE)) {
			if (Logging.DEBUG) Log.d(TAG, "Widget on prepare update from receive");
			
			final NativeBoincService runner = appContext.getRunnerService();
			
			if (runner != null)
				runner.getTasks(appContext);
			
			boolean updateChanged = inputIntent.getBooleanExtra(WIDGET_UPDATE_CHANGED, false);
			boolean clientStart = inputIntent.getBooleanExtra(WIDGET_CLIENT_START, false);
					
			if (updateChanged || clientStart) {
				// change update period
				int updatePeriod = appContext.getWigetUpdatePeriod();
				if (updateChanged) {
					if (Logging.DEBUG) Log.d(TAG, "Reenable update periodically "+updatePeriod);
				} else
					if (Logging.DEBUG) Log.d(TAG, "Enable update periodically at start "+updatePeriod);
				
				AlarmManager alarmManager = (AlarmManager)appContext.getSystemService(Context.ALARM_SERVICE);
				Intent intent = new Intent(NATIVE_BOINC_WIDGET_PREPARE_UPDATE);
				PendingIntent pendingIntent = PendingIntent.getBroadcast(appContext, 0, intent, 
						PendingIntent.FLAG_UPDATE_CURRENT);
				
				if (updateChanged) // if reenable
					alarmManager.cancel(pendingIntent);
				alarmManager.setRepeating(AlarmManager.RTC_WAKEUP, System.currentTimeMillis(),
						updatePeriod, pendingIntent);
			}
		} else if (inputIntent.getAction().equals(NATIVE_BOINC_WIDGET_UPDATE)) {
			if (Logging.DEBUG) Log.d(TAG, "Widget on update from receive");
			
			AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(context);
			RemoteViews views = new RemoteViews(context.getPackageName(), R.layout.tablet_widget);
			
			/* start manager button */
			Intent intent = new Intent(appContext, BoincManagerActivity.class);
			intent.putExtra(BoincManagerActivity.PARAM_CONNECT_NATIVE_CLIENT, true);
			intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			PendingIntent pendingIntent = PendingIntent.getActivity(appContext, 0, intent, 0);
			views.setOnClickPendingIntent(R.id.widgetManager, pendingIntent);
			
			/* start manager button */
			intent = new Intent(appContext, ScreenLockActivity.class);
			intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			pendingIntent = PendingIntent.getActivity(appContext, 0, intent, 0);
			views.setOnClickPendingIntent(R.id.widgetLock, pendingIntent);
			
			intent = new Intent(NATIVE_BOINC_CLIENT_START_STOP);
			pendingIntent = PendingIntent.getBroadcast(appContext, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
			views.setOnClickPendingIntent(R.id.widgetStart, pendingIntent);
			views.setOnClickPendingIntent(R.id.widgetStop, pendingIntent);
			
			/* update task list */
			final NativeBoincService runner = appContext.getRunnerService();
			boolean isRun = runner != null && runner.isRun();
			Parcelable[] taskItems = inputIntent.getParcelableArrayExtra(BoincManagerApplication.UPDATE_TASKS);
			
			if (taskItems != null) {
				// sort task list
				TaskItem[] sortedTaskItems = sortTaskItems(taskItems);
				updateTaskViews(appContext, views, sortedTaskItems);
			} else
				hideTaskViews(views);
			
			if (isRun) {
				views.setViewVisibility(R.id.widgetStart, View.GONE);
				views.setViewVisibility(R.id.widgetStop, View.VISIBLE);
			} else {
				views.setViewVisibility(R.id.widgetStart, View.VISIBLE);
				views.setViewVisibility(R.id.widgetStop, View.GONE);
			}
			
			ComponentName thisAppWidget = new ComponentName(context.getPackageName(), getClass().getName());
			int ids[] = appWidgetManager.getAppWidgetIds(thisAppWidget);
			appWidgetManager.updateAppWidget(ids, views);
			
			if (!isRun) {
				// disable autorefresh when not ran
				disableAutoRefresh(context);
			}
		} else if (inputIntent.getAction().equals(NATIVE_BOINC_CLIENT_START_STOP)) {
			if (Logging.DEBUG) Log.d(TAG, "Client start/stop from widget receive");
			
			final NativeBoincService runner = appContext.getRunnerService();
			if (runner != null) {
				if (runner.isRun()) {
					Intent intent = new Intent(appContext, ShutdownDialogActivity.class);
					intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
					PendingIntent pendingIntent = PendingIntent.getActivity(appContext, 0, intent, 0);
					try {
						pendingIntent.send();
					} catch(CanceledException ex) { }
				} else
					runner.startClient(false);
			} else {
				appContext.bindRunnerAndStart();
			}
		} else {
			StringBuilder sb = new StringBuilder();
			sb.append(NATIVE_BOINC_CLIENT_TASK_INFO);
			String action = inputIntent.getAction();
			TaskItem taskItem = (TaskItem)inputIntent.getParcelableExtra(TaskInfoDialogActivity.ARG_TASK_INFO);
			
			for (int i = 0; i < 10; i++) {
				sb.append(i);
				if (sb.toString().equals(action)) {
					Intent intent = new Intent(appContext, TaskInfoDialogActivity.class);
					intent.putExtra(TaskInfoDialogActivity.ARG_TASK_INFO, taskItem);
					intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
					PendingIntent pendingIntent = PendingIntent.getActivity(appContext, 0, intent,
							PendingIntent.FLAG_UPDATE_CURRENT);
					
					try {
						pendingIntent.send();
					} catch(CanceledException ex) { }
					break;
				}
				sb.delete(sb.length()-1, sb.length());
			}
		}
	}

	/**
	 * code from TasksActivity
	 */
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
	
	/* sort tasks */
	private static TaskItem[] sortTaskItems(Parcelable[] taskItems) {
		TaskItem[] sorted = new TaskItem[taskItems.length];
		for (int i = 0; i < taskItems.length; i++)
			sorted[i] = (TaskItem)taskItems[i];
		
		Comparator<TaskItem> comparator = new Comparator<TaskItem>() {
			@Override
			public int compare(TaskItem object1, TaskItem object2) {
				// First criteria - state
				if ( (cStatePriority[object1.taskInfo.stateControl] -
						cStatePriority[object2.taskInfo.stateControl]) != 0 ) {
					// The priorities for are different - return the order
					return (cStatePriority[object1.taskInfo.stateControl] -
							cStatePriority[object2.taskInfo.stateControl]);
				}
				// Otherwise continue with further criteria
				// The next criteria - deadline
				int deadlineDiff = (int)(object1.taskInfo.deadlineNum - object2.taskInfo.deadlineNum);
				if (deadlineDiff != 0) {
					// not the same deadline
					return deadlineDiff;
				}
				// Last, sort by project name, then by task name
				int prjComp = object1.taskInfo.project.compareToIgnoreCase(object2.taskInfo.project);
				if (prjComp != 0) {
					return prjComp;
				}
				return object1.taskInfo.taskName.compareToIgnoreCase(object2.taskInfo.taskName);
			}
		};
		Arrays.sort(sorted, comparator);
		return sorted;
	}
	
	/**
	 * task element view ids arrays
	 */
	
	private static final int[] sTaskViews = {
		R.id.item1, R.id.item2, R.id.item3, R.id.item4, R.id.item5,
		R.id.item6, R.id.item7, R.id.item8, R.id.item9, R.id.item10
	};
	
	private static final int[] sTaskSeparators = {
		R.id.separator1, R.id.separator2, R.id.separator3, R.id.separator4, R.id.separator5,
		R.id.separator6, R.id.separator7, R.id.separator8, R.id.separator9, R.id.separator10,
	};
	
	private static final int[] sTaskAppNames = {
		R.id.taskAppName1, R.id.taskAppName2, R.id.taskAppName3, R.id.taskAppName4, R.id.taskAppName5,
		R.id.taskAppName6, R.id.taskAppName7, R.id.taskAppName8, R.id.taskAppName9, R.id.taskAppName10
	};
	
	private static final int[] sTaskDeadlines = {
		R.id.taskDeadline1, R.id.taskDeadline2, R.id.taskDeadline3,
		R.id.taskDeadline4, R.id.taskDeadline5, R.id.taskDeadline6,
		R.id.taskDeadline7, R.id.taskDeadline8, R.id.taskDeadline9, R.id.taskDeadline10
	};
	
	private static final int[] sTaskProjectNames = {
		R.id.taskProjectName1, R.id.taskProjectName2, R.id.taskProjectName3,
		R.id.taskProjectName4, R.id.taskProjectName5, R.id.taskProjectName6,
		R.id.taskProjectName7, R.id.taskProjectName8, R.id.taskProjectName9, R.id.taskProjectName10
	};
	
	private static final int[] sTaskProgressRunnings = {
		R.id.taskProgressRunning1, R.id.taskProgressRunning2, R.id.taskProgressRunning3,
		R.id.taskProgressRunning4, R.id.taskProgressRunning5, R.id.taskProgressRunning6,
		R.id.taskProgressRunning7, R.id.taskProgressRunning8, R.id.taskProgressRunning9,
		R.id.taskProgressRunning10
	};
	
	private static final int[] sTaskProgressSuspendeds = {
		R.id.taskProgressSuspended1, R.id.taskProgressSuspended2, R.id.taskProgressSuspended3,
		R.id.taskProgressSuspended4, R.id.taskProgressSuspended5, R.id.taskProgressSuspended6,
		R.id.taskProgressSuspended7, R.id.taskProgressSuspended8, R.id.taskProgressSuspended9,
		R.id.taskProgressSuspended10
	};
	
	private static final int[] sTaskProgressWaitings = {
		R.id.taskProgressWaiting1, R.id.taskProgressWaiting2, R.id.taskProgressWaiting3,
		R.id.taskProgressWaiting4, R.id.taskProgressWaiting5, R.id.taskProgressWaiting6,
		R.id.taskProgressWaiting7, R.id.taskProgressWaiting8, R.id.taskProgressWaiting9,
		R.id.taskProgressWaiting10
	};
	
	private static final int[] sTaskProgressFinisheds = {
		R.id.taskProgressFinished1, R.id.taskProgressFinished2, R.id.taskProgressFinished3,
		R.id.taskProgressFinished4, R.id.taskProgressFinished5, R.id.taskProgressFinished6,
		R.id.taskProgressFinished7, R.id.taskProgressFinished8, R.id.taskProgressFinished9,
		R.id.taskProgressFinished10
	};
	
	private static final int[] sTaskProgressRunningLayouts = {
		R.id.taskProgressRunningLayout1, R.id.taskProgressRunningLayout2, R.id.taskProgressRunningLayout3,
		R.id.taskProgressRunningLayout4, R.id.taskProgressRunningLayout5, R.id.taskProgressRunningLayout6,
		R.id.taskProgressRunningLayout7, R.id.taskProgressRunningLayout8, R.id.taskProgressRunningLayout9,
		R.id.taskProgressRunningLayout10
	};
	
	private static final int[] sTaskProgressSuspendedLayouts = {
		R.id.taskProgressSuspendedLayout1, R.id.taskProgressSuspendedLayout2, R.id.taskProgressSuspendedLayout3,
		R.id.taskProgressSuspendedLayout4, R.id.taskProgressSuspendedLayout5, R.id.taskProgressSuspendedLayout6,
		R.id.taskProgressSuspendedLayout7, R.id.taskProgressSuspendedLayout8, R.id.taskProgressSuspendedLayout9,
		R.id.taskProgressSuspendedLayout10
	};
	
	private static final int[] sTaskProgressWaitingLayouts = {
		R.id.taskProgressWaitingLayout1, R.id.taskProgressWaitingLayout2, R.id.taskProgressWaitingLayout3,
		R.id.taskProgressWaitingLayout4, R.id.taskProgressWaitingLayout5, R.id.taskProgressWaitingLayout6,
		R.id.taskProgressWaitingLayout7, R.id.taskProgressWaitingLayout8, R.id.taskProgressWaitingLayout9,
		R.id.taskProgressWaitingLayout10
	};
	
	private static final int[] sTaskProgressFinishedLayouts = {
		R.id.taskProgressFinishedLayout1, R.id.taskProgressFinishedLayout2, R.id.taskProgressFinishedLayout3,
		R.id.taskProgressFinishedLayout4, R.id.taskProgressFinishedLayout5, R.id.taskProgressFinishedLayout6,
		R.id.taskProgressFinishedLayout7, R.id.taskProgressFinishedLayout8, R.id.taskProgressFinishedLayout9,
		R.id.taskProgressFinishedLayout10
	};
	
	private static final int[] sTaskProgressTexts = {
		R.id.taskProgressText1, R.id.taskProgressText2, R.id.taskProgressText3,
		R.id.taskProgressText4, R.id.taskProgressText5, R.id.taskProgressText6,
		R.id.taskProgressText7, R.id.taskProgressText8, R.id.taskProgressText9, R.id.taskProgressText10
	};
	
	private static final int[] sTaskElapseds = {
		R.id.taskElapsed1, R.id.taskElapsed2, R.id.taskElapsed3, R.id.taskElapsed4, R.id.taskElapsed5,
		R.id.taskElapsed6, R.id.taskElapsed7, R.id.taskElapsed8, R.id.taskElapsed9, R.id.taskElapsed10
	};
	
	private static final int[] sTaskRemainings = {
		R.id.taskRemaining1, R.id.taskRemaining2, R.id.taskRemaining3,
		R.id.taskRemaining4, R.id.taskRemaining5, R.id.taskRemaining6,
		R.id.taskRemaining7, R.id.taskRemaining8, R.id.taskRemaining9, R.id.taskRemaining10
	};
	
	private static final int TOTAL_ITEMS = 10;
	
	private static void updateTaskViews(BoincManagerApplication appContext, RemoteViews views,
			final TaskItem[] taskItems) {
		final int visibleItems = Math.min(TOTAL_ITEMS, taskItems.length);
		/* set up visibility of items */
		for (int i = 0; i < visibleItems; i++) {
			views.setViewVisibility(sTaskViews[i], View.VISIBLE);
			views.setViewVisibility(sTaskSeparators[i], View.VISIBLE);
		}
		for (int i = visibleItems; i < TOTAL_ITEMS; i++) {
			views.setViewVisibility(sTaskViews[i], View.GONE);
			views.setViewVisibility(sTaskSeparators[i], View.GONE);
		}
		
		/* update content */
		StringBuilder sb = new StringBuilder();
		sb.append(NATIVE_BOINC_CLIENT_TASK_INFO);
		
		for (int i = 0; i < visibleItems; i++) {
			TaskItem item = taskItems[i];
			
			sb.append(i);
			Intent intent = new Intent(sb.toString());
			intent.putExtra(TaskInfoDialogActivity.ARG_TASK_INFO, item);
			PendingIntent pendingIntent = PendingIntent.getBroadcast(appContext, 0, intent,
					PendingIntent.FLAG_UPDATE_CURRENT);
			views.setOnClickPendingIntent(sTaskViews[i], pendingIntent);
			sb.delete(sb.length()-1, sb.length());
			
			views.setTextViewText(sTaskAppNames[i], item.taskInfo.application);
			views.setTextViewText(sTaskProjectNames[i], item.taskInfo.project);
			views.setTextViewText(sTaskDeadlines[i], item.taskInfo.deadline);
			views.setTextViewText(sTaskElapseds[i], item.taskInfo.elapsed);
			views.setTextViewText(sTaskRemainings[i], item.taskInfo.toCompletion);
			views.setTextViewText(sTaskProgressTexts[i], item.taskInfo.progress);
			
			switch(item.taskInfo.stateControl) {
			case TaskInfo.SUSPENDED:
			case TaskInfo.ABORTED:
			case TaskInfo.ERROR:
				views.setViewVisibility(sTaskProgressRunningLayouts[i], View.GONE);
				views.setViewVisibility(sTaskProgressFinishedLayouts[i], View.GONE);
				views.setViewVisibility(sTaskProgressWaitingLayouts[i], View.GONE);
				views.setViewVisibility(sTaskProgressSuspendedLayouts[i], View.VISIBLE);
				views.setProgressBar(sTaskProgressSuspendeds[i], 1000, item.taskInfo.progInd, false);
				break;
			case TaskInfo.DOWNLOADING:
			case TaskInfo.UPLOADING:
				// Setting progress to 0 will let the secondary progress to display itself (which is always set to 100)
				item.taskInfo.progInd = 0;
				// no break, we continue following case (READY_TO_REPORT)
			case TaskInfo.READY_TO_REPORT:
				views.setViewVisibility(sTaskProgressRunningLayouts[i], View.GONE);
				views.setViewVisibility(sTaskProgressSuspendedLayouts[i], View.GONE);
				views.setViewVisibility(sTaskProgressWaitingLayouts[i], View.GONE);
				views.setViewVisibility(sTaskProgressFinishedLayouts[i], View.VISIBLE);
				views.setProgressBar(sTaskProgressFinisheds[i], 1000, item.taskInfo.progInd, false);
				break;
			case TaskInfo.RUNNING:
				views.setViewVisibility(sTaskProgressFinishedLayouts[i], View.GONE);
				views.setViewVisibility(sTaskProgressSuspendedLayouts[i], View.GONE);
				views.setViewVisibility(sTaskProgressWaitingLayouts[i], View.GONE);
				views.setViewVisibility(sTaskProgressRunningLayouts[i], View.VISIBLE);
				views.setProgressBar(sTaskProgressRunnings[i], 1000, item.taskInfo.progInd, false);
				break;
			default:
				views.setViewVisibility(sTaskProgressFinishedLayouts[i], View.GONE);
				views.setViewVisibility(sTaskProgressSuspendedLayouts[i], View.GONE);
				views.setViewVisibility(sTaskProgressRunningLayouts[i], View.GONE);
				views.setViewVisibility(sTaskProgressWaitingLayouts[i], View.VISIBLE);
				views.setProgressBar(sTaskProgressWaitings[i], 1000, item.taskInfo.progInd, false);
				if (item.taskInfo.stateControl == TaskInfo.PREEMPTED)  {
					// Task already started - set secondary progress to 100
					// The background of progress-bar will be yellow
					views.setInt(sTaskProgressWaitings[i], "setSecondaryProgress", 1000);
				}
				else {
					// Task did not started yet - set secondary progress to 0
					// The background of progress-bar will be grey
					views.setInt(sTaskProgressWaitings[i], "setSecondaryProgress", 0);
				}
				break;
			}
		}
	}
	
	private static void hideTaskViews(RemoteViews views) {
		for (int i = 0; i < TOTAL_ITEMS; i++) {
			views.setViewVisibility(sTaskViews[i], View.GONE);
			views.setViewVisibility(sTaskSeparators[i], View.GONE);
		}
	}
}
