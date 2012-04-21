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

import edu.berkeley.boinc.lite.TimePreferences;
import sk.boinc.nativeboinc.util.TimePrefsData;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.TimePicker;

/**
 * @author mat
 *
 */
public class TimePreferencesActivity extends AbstractBoincActivity {

	public static final String ARG_TITLE = "Title";
	public static final String ARG_TIME_PREFS = "TimePreferences";
	
	public static final String RESULT_TIME_PREFS = "TimePreferences";
	
	private final static int DIALOG_TIME_PICKER = 1;
	
	private TimePreferences mTimePreferences;
	private int mSelectedDay;
	private boolean mSelectedStartHour;
	
	private Button mEverydayStart;
	private Button mEverydayEnd;
	
	private static final int[] sDayStartIds = {
		R.id.sundayStart, R.id.mondayStart, R.id.tuesdayStart, R.id.wednesdayStart,
		R.id.thursdayStart, R.id.fridayStart, R.id.saturdayStart
	};
	
	private static final int[] sDayEndIds = {
		R.id.sundayEnd, R.id.mondayEnd, R.id.tuesdayEnd, R.id.wednesdayEnd,
		R.id.thursdayEnd, R.id.fridayEnd, R.id.saturdayEnd
	};
	
	private static final int[] sDayEnableIds = {
		R.id.sundayEnable, R.id.mondayEnable, R.id.tuesdayEnable, R.id.wednesdayEnable,
		R.id.thursdayEnable, R.id.fridayEnable, R.id.saturdayEnable
	};
	
	private Button[] mDayStart = new Button[7];
	private Button[] mDayEnd = new Button[7];
	private CheckBox[] mDayEnable = new CheckBox[7];
	
	private static class SavedState {
		private final TimePreferences timePreferences;
		private final int selectedDay;
		private final boolean selectedStartHour;
		
		public SavedState(TimePreferencesActivity activity) {
			timePreferences = activity.mTimePreferences;
			selectedDay = activity.mSelectedDay;
			selectedStartHour = activity.mSelectedStartHour;
		}
		
		public void restore(TimePreferencesActivity activity) {
			activity.mTimePreferences = timePreferences;
			activity.mSelectedDay = selectedDay;
			activity.mSelectedStartHour = selectedStartHour;
		}
	}
	
	private class OnChangeDayEnableListener implements CompoundButton.OnCheckedChangeListener {
		private final int mDay;
		
		public OnChangeDayEnableListener(int day) {
			mDay = day;
		}
		
		@Override
		public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
			mDayStart[mDay].setEnabled(isChecked);
			mDayEnd[mDay].setEnabled(isChecked);
		}
	};
	
	private class OnClickHourListener implements View.OnClickListener {
		private final int mDay;
		private final boolean mStartHour;
		
		public OnClickHourListener(int day, boolean startHour) {
			mDay = day;
			mStartHour = startHour;
		}

		@Override
		public void onClick(View v) {
			mSelectedDay = mDay;
			mSelectedStartHour = mStartHour;
			showDialog(DIALOG_TIME_PICKER);
		}
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.time_prefs);
		
		Intent intent = getIntent();
		setTitle(intent.getStringExtra(ARG_TITLE));
		mTimePreferences = ((TimePrefsData)intent.getParcelableExtra(ARG_TIME_PREFS)).timePrefs;
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance(); 
		if (savedState != null)
			savedState.restore(this);
		
		mEverydayStart = (Button)findViewById(R.id.everyDayStart);
		mEverydayStart.setOnClickListener(new OnClickHourListener(-1, true));
		mEverydayEnd = (Button)findViewById(R.id.everyDayEnd);
		mEverydayEnd.setOnClickListener(new OnClickHourListener(-1, false));
		
		for (int i = 0; i < 7; i++) {
			mDayStart[i] = (Button)findViewById(sDayStartIds[i]);
			mDayStart[i].setOnClickListener(new OnClickHourListener(i, true));
			mDayEnd[i] = (Button)findViewById(sDayEndIds[i]);
			mDayEnd[i].setOnClickListener(new OnClickHourListener(i, false));
			
			mDayEnable[i] = (CheckBox)findViewById(sDayEnableIds[i]);
			mDayEnable[i].setOnCheckedChangeListener(new OnChangeDayEnableListener(i));
		}
		
		updateTimePreferences();
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	@Override
	public void onBackPressed() {
		/* if user returns from activity */
		finalSetupTimePreferences();
		
		Intent intent = new Intent();
		intent.putExtra(RESULT_TIME_PREFS, new TimePrefsData(mTimePreferences));
		setResult(RESULT_OK, intent);
		finish();
	}
	
	/* dialog handling */
	@Override
	protected Dialog onCreateDialog(int dialogId, Bundle args) {
		if (dialogId == DIALOG_TIME_PICKER) {
			LayoutInflater inflater = LayoutInflater.from(this);
			View view = inflater.inflate(R.layout.time_picker, null); 
			final TimePicker picker = (TimePicker)view.findViewById(R.id.timePicker);
			picker.setIs24HourView(true);
			
			return new AlertDialog.Builder(this)
				.setTitle(R.string.setHour)
				.setView(picker)
				.setCancelable(true)
				.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						double value = (double)picker.getCurrentHour() +
								(double)picker.getCurrentMinute()/60.0;
						
						if (mSelectedDay == -1) { // everyday
							if (mSelectedStartHour) {
								mTimePreferences.start_hour = value;
								mEverydayStart.setText(mTimePreferences.startHourString());
							} else {
								mTimePreferences.end_hour = value;
								mEverydayEnd.setText(mTimePreferences.endHourString());
							}
						} else {
							TimePreferences.TimeSpan day = mTimePreferences.week_prefs[mSelectedDay];
							if (mSelectedStartHour) {
								day.start_hour = value;
								mDayStart[mSelectedDay].setText(day.startHourString());
							} else {
								day.end_hour = value;
								mDayEnd[mSelectedDay].setText(day.endHourString());
							}
						}
					}
				})
				.setNegativeButton(R.string.cancel, null)
				.create();
		}
		return null;
	}
	
	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		if (dialogId == DIALOG_TIME_PICKER) {
			double value;
			int hour, minute;
			TimePicker timePicker = (TimePicker)dialog.findViewById(R.id.timePicker);
			
			if (mSelectedDay == -1) { // everyday
				if (mSelectedStartHour)
					value = mTimePreferences.start_hour;
				else
					value = mTimePreferences.end_hour;
			} else { // one of day of week
				TimePreferences.TimeSpan day = mTimePreferences.week_prefs[mSelectedDay];
				if (mSelectedStartHour)
					value = day.start_hour;
				else
					value = day.end_hour;
			}
			
			hour = (int)Math.floor(value);
			minute = (int)Math.round((value-(double)hour)*60.0);
			minute = Math.min(59, minute);
			
			// set up time picker
			timePicker.setCurrentHour(hour);
			timePicker.setCurrentMinute(minute);
		}
	}
	
	private void updateTimePreferences() {
		TimePreferences.TimeSpan[] weekPrefs = mTimePreferences.week_prefs;
		
		mEverydayStart.setText(mTimePreferences.startHourString());
		mEverydayEnd.setText(mTimePreferences.endHourString());

		/* update week prefs */
		for (int i = 0; i < 7; i++) {
			if (weekPrefs[i] != null) {
				mDayStart[i].setText(weekPrefs[i].startHourString());
				mDayEnd[i].setText(weekPrefs[i].endHourString());
			} else {
				mDayEnable[i].setChecked(false);
				weekPrefs[i] = new TimePreferences.TimeSpan();
				weekPrefs[i].start_hour = 0;
				weekPrefs[i].end_hour = 0;
				mDayStart[i].setText("00:00");
				mDayEnd[i].setText("00:00");
			}
		}
	}
	
	private void finalSetupTimePreferences() {
		TimePreferences.TimeSpan[] weekPrefs = mTimePreferences.week_prefs;
		for (int i = 0; i < 7; i++) {
			if (!mDayEnable[i].isChecked())
				weekPrefs[i] = null;
		}
	}
}
