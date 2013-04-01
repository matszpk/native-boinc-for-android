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

import java.util.List;

import sk.boinc.nativeboinc.bugcatch.BugReportInfo;
import sk.boinc.nativeboinc.nativeclient.AbstractNativeBoincListener;
import sk.boinc.nativeboinc.util.StandardDialogs;
import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;

/**
 * @author mat
 *
 */
public class BugCatcherActivity extends ServiceBoincActivity implements AbstractNativeBoincListener {

	private static final String TAG = "BugCatcherActivity";
	
	private boolean mDoRestart = false;
	private List<BugReportInfo> mBugReports = null;
	
	private static class SavedState {
		private final boolean doRestart;
		private final List<BugReportInfo> bugReports;
		
		public SavedState(BugCatcherActivity activity) {
			doRestart = activity.mDoRestart;
			bugReports = activity.mBugReports;
		}
		
		public void restore(BugCatcherActivity activity) {
			activity.mDoRestart = doRestart;
			activity.mBugReports = bugReports;
		}
	}
	
	private class BugReportListAdapter extends BaseAdapter {

		private Context mContext;
		
		public BugReportListAdapter(Context context) {
			mContext = context;
		}
		
		@Override
		public int getCount() {
			if (mBugReports == null)
				return 0;
			return mBugReports.size();
		}

		@Override
		public Object getItem(int position) {
			return mBugReports.get(position);
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
				view = inflater.inflate(android.R.layout.simple_list_item_2, null);
			} else
				view = convertView;
			
			
			return view;
		}
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		setUpService(false, false, true, true, false, false);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.bug_catch);
		
	}
	
	private void updateRunnerError() {
		if (mRunner == null)
			return;
		
		mRunner.handlePendingErrorMessage(this);
	}
	
	@Override
	public void onRunnerConnected() {
		setProgressBarIndeterminateVisibility(mRunner.serviceIsWorking());
		// update runner error (show)
		updateRunnerError();
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	@Override
	public void onResume() {
		super.onResume();
		// update runner error (show)
		updateRunnerError();
	}
	
	@Override
	public boolean onNativeBoincClientError(String message) {
		StandardDialogs.showErrorDialog(this, message);
		return true;
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}
}
