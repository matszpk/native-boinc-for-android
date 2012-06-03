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

import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.installer.InstallerProgressListener;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.util.ProgressItem;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class ProgressActivity extends ServiceBoincActivity implements InstallerProgressListener {
	
	private static final String TAG = "ProgressActivity";
	
	private ProgressItem[] mCurrentProgress;
	
	private class ProgressCancelOnClickListener implements View.OnClickListener {

		private String mDistribName;
		private String mProjectUrl;
		
		public ProgressCancelOnClickListener(String distribName, String projectUrl) {
			this.mDistribName = distribName;
			this.mProjectUrl = projectUrl;
		}
		
		@Override
		public void onClick(View v) {
			/* */
			if (Logging.DEBUG) Log.d(TAG, "Canceled item:"+mDistribName);
			
			if (mDistribName.equals(InstallerService.BOINC_CLIENT_ITEM_NAME))
				mInstaller.cancelClientInstallation();
			else if (mDistribName.equals(InstallerService.BOINC_DUMP_ITEM_NAME))
				mInstaller.cancelDumpFiles();
			else if (mDistribName.equals(InstallerService.BOINC_REINSTALL_ITEM_NAME))
				mInstaller.cancelReinstallation();
			else {
				if (mProjectUrl == null || mProjectUrl.length()==0)
					return;
				
				mInstaller.cancelProjectAppsInstallation(mProjectUrl);
			}
		}
	}
	
	private Map<String, ProgressCancelOnClickListener> mCurrentCancelClickListeners =
			new HashMap<String, ProgressCancelOnClickListener>();
	
	private static class ItemViewTag {
		public final TextView progressInfo;
		public final ProgressBar progressBar;
		public final Button cancelButton;
		
		public ItemViewTag(View view) {
			progressInfo = (TextView)view.findViewById(R.id.progressInfo);
			progressBar = (ProgressBar)view.findViewById(R.id.progressBar);
			cancelButton = (Button)view.findViewById(R.id.progressCancel);
		}
	}
	
	/* we update item outside adapter, because this notifyDataChanged makes some problems
	 * with cancel clicking */
	private void updateCreatedItemView(View view, int position) {
		ItemViewTag tag = (ItemViewTag)view.getTag();
		if (tag == null) {
			tag = new ItemViewTag(view);
			view.setTag(tag);
		}
		
		final ProgressItem item = mCurrentProgress[position];
		if (Logging.DEBUG) Log.d(TAG, "position:"+position+",name:"+item.name+"state:"+item.state);
		
		String itemName = InstallerService.resolveItemName(getResources(), item.name);
		
		switch(item.state) {
		case ProgressItem.STATE_IN_PROGRESS:
			tag.progressInfo.setText(itemName + ": " + item.opDesc);
			break;
		case ProgressItem.STATE_CANCELLED:
			tag.progressInfo.setText(itemName + ": " + getString(R.string.operationCancelled));
			break;
		case ProgressItem.STATE_ERROR_OCCURRED:
			tag.progressInfo.setText(itemName + ": " + item.opDesc);
			break;
		case ProgressItem.STATE_FINISHED:
			tag.progressInfo.setText(itemName + ": " + getString(R.string.operationFinished));
			break;
		}
		
		if (item.state == ProgressItem.STATE_IN_PROGRESS) {
			tag.progressBar.setVisibility(View.VISIBLE);
			if (item.progress >= 0) {
				tag.progressBar.setIndeterminate(false);
				tag.progressBar.setProgress(item.progress);
			} else
				tag.progressBar.setIndeterminate(true);
		} else
			tag.progressBar.setVisibility(View.GONE);
		
		// disable button if end
		tag.cancelButton.setEnabled(item.state == ProgressItem.STATE_IN_PROGRESS);
	}
	
	private class ProgressItemAdapter extends BaseAdapter {
		private Context mContext;
		
		public ProgressItemAdapter(Context context) {
			mContext = context;
		}

		@Override
		public int getCount() {
			if (mCurrentProgress == null)
				return 0;
			return mCurrentProgress.length;
		}

		@Override
		public Object getItem(int position) {
			return mCurrentProgress[position];
		}

		@Override
		public long getItemId(int position) {
			return 0;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			View view = convertView;
			if (convertView == null) {
				LayoutInflater inflater = (LayoutInflater)mContext
						.getSystemService(LAYOUT_INFLATER_SERVICE);
				view = inflater.inflate(R.layout.progress_list_item, null);
			}
			
			final ProgressItem item = mCurrentProgress[position];
			
			Button cancelButton = (Button)view.findViewById(R.id.progressCancel);
			updateCreatedItemView(view, position);
					
			ProgressCancelOnClickListener listener = mCurrentCancelClickListeners.get(item.name);
			// if new view created and listener not null then install listener
			if (listener != null) {
				if (Logging.DEBUG) Log.d(TAG, "Install cancel item listener:"+item.name);
				cancelButton.setOnClickListener(listener);
			}
			
			return view;
		}
	}
	
	private BoincManagerApplication mApp = null;
	
	private ListView mProgressList;
	private ProgressItemAdapter mProgressItemAdapter;
	
	private Button mCancelAll = null;
	private Button mNextButton = null;
	private Button mBackButton = null;
	
	private TextView mProgressText = null;
	
	private int mInstallerStage = -1;
	
	private NotificationController mNotificationController = null;
	
	private boolean mFirstTestOfTaskRan = true;
	private boolean mAreAllTaskNotRan = false;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		mApp = (BoincManagerApplication) getApplication();
		mNotificationController = mApp.getNotificationController();
		
		setUpService(false, false, false, false, true, true);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.install_progress);
		
		mProgressItemAdapter = new ProgressItemAdapter(this);
		
		mProgressList = (ListView)findViewById(R.id.installProgressList);
		mProgressList.setAdapter(mProgressItemAdapter);
		
		mCancelAll = (Button)findViewById(R.id.progressCancelAll);
		mBackButton = (Button)findViewById(R.id.progressBack);
		mNextButton = (Button)findViewById(R.id.progressNext);
		mProgressText = (TextView)findViewById(R.id.installProgressText);
		
		mInstallerStage = mApp.getInstallerStage();
		
		if (mInstallerStage != BoincManagerApplication.INSTALLER_NO_STAGE) {
			mNextButton.setVisibility(View.VISIBLE);
			
			mNextButton.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					mNotificationController.removeAllNotInProgress();
					finish();
					toNextInstallerStep();
				}
			});
		}
		
		mCancelAll.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mInstaller != null)
					mInstaller.cancelAll();
			}
		});
		
		mBackButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				ProgressActivity.this.onBackPressed();
			}
		});
	}
	
	/* update item view outside list adapter */
	private void updateItemView(int position) {
		if (mProgressList == null)
			return;
		int firstVisible = mProgressList.getFirstVisiblePosition();
		int lastVisible = mProgressList.getLastVisiblePosition();
		if (firstVisible > position || lastVisible < position)
			return;
		
		View view = mProgressList.getChildAt(position-firstVisible);
		if (view != null)
			updateCreatedItemView(view, position);
	}

	public boolean areAllTasksNotRan() {
		for (ProgressItem progress: mCurrentProgress)
			if (progress.state == ProgressItem.STATE_IN_PROGRESS)
				return false;
		return true;
	}
	
	public boolean areAllTasksFinishedSuccessfully() {
		for (ProgressItem progress: mCurrentProgress)
			if (progress.state != ProgressItem.STATE_FINISHED)
				return false;
		return true;
	}

	private void ifNothingInBackground() {
		if (areAllTasksNotRan()) {
			if (!mFirstTestOfTaskRan && mAreAllTaskNotRan)
				return; // no changes
			
			if (mInstallerStage != BoincManagerApplication.INSTALLER_NO_STAGE) {
				if (areAllTasksFinishedSuccessfully()) {
					if (mApp.isInstallerRun()) // if installer
						mBackButton.setEnabled(false);
					mNextButton.setEnabled(true);
				}
			}
			
			mCancelAll.setEnabled(false);
			mProgressText.setText(getString(R.string.noInstallOpsText));
			
			mAreAllTaskNotRan = true;
			mFirstTestOfTaskRan = false;
		} else {
			if (!mFirstTestOfTaskRan && !mAreAllTaskNotRan)
				return; // no changes
			
			mCancelAll.setEnabled(true);
			
			if (mInstallerStage != BoincManagerApplication.INSTALLER_NO_STAGE) {
				mBackButton.setEnabled(true);
				mNextButton.setEnabled(false);
			}
			
			mProgressText.setText(getString(R.string.installProgressText));
			
			mAreAllTaskNotRan = false;
			mFirstTestOfTaskRan = false;
		}
	}
	
	private void getProgressFromInstaller() {
		if (mInstaller != null) {
			setProgressBarIndeterminateVisibility(mInstaller.isWorking());
			
			mCurrentProgress = mInstaller.getProgressItems();
			if (mCurrentProgress == null)
				mCurrentProgress = new ProgressItem[0];
			Arrays.sort(mCurrentProgress, mProgressCompatator);
			/* we update data with using adapter */
			mCurrentCancelClickListeners.clear();
			
			for (ProgressItem progress: mCurrentProgress)
				mCurrentCancelClickListeners.put(progress.name,
						new ProgressCancelOnClickListener(progress.name, progress.projectUrl));
			
			mProgressItemAdapter.notifyDataSetChanged();
			
			ifNothingInBackground();
		}
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		getProgressFromInstaller();
	}
	
	@Override
	public void onBackPressed() {
		mNotificationController.removeAllNotInProgress();
		finish();
		if (mApp.isInstallerRun() && areAllTasksFinishedSuccessfully())
			// if all tasks finished successfully go to next
			toNextInstallerStep();
	}
	
	/* go to next activity in installation wizard */
	private void toNextInstallerStep() {
		if (mInstallerStage == BoincManagerApplication.INSTALLER_CLIENT_INSTALLING_STAGE ||
				mInstallerStage == BoincManagerApplication.INSTALLER_PROJECT_STAGE)
			startActivity(new Intent(ProgressActivity.this, InstallStep2Activity.class));
		else if (mInstallerStage == BoincManagerApplication.INSTALLER_PROJECT_INSTALLING_STAGE ||
				mInstallerStage == BoincManagerApplication.INSTALLER_FINISH_STAGE)
			startActivity(new Intent(ProgressActivity.this, InstallFinishActivity.class));
	}
	
	private static final String[] sItemNameOrder = {
		InstallerService.BOINC_DUMP_ITEM_NAME,
		InstallerService.BOINC_REINSTALL_ITEM_NAME,
		InstallerService.BOINC_CLIENT_ITEM_NAME
	};
	
	private Comparator<ProgressItem> mProgressCompatator = new Comparator<ProgressItem>() {
		@Override
		public int compare(ProgressItem p1, ProgressItem p2) {
			int order1, order2;
			for (order1 = 0; order1 < sItemNameOrder.length; order1++)
				if (sItemNameOrder[order1].equals(p1.name))
					break;
			for (order2 = 0; order2 < sItemNameOrder.length; order2++)
				if (sItemNameOrder[order2].equals(p2.name))
					break;
			
			// determine order
			if (order1 != sItemNameOrder.length || order2 != sItemNameOrder.length)
				return Integer.signum(order1-order2);
			
			// if normal project names
			return p1.name.compareTo(p2.name);
		}
	};
	
	@Override
	protected void onInstallerConnected() {
		getProgressFromInstaller();
	}
	
	@Override
	protected void onInstallerDisconnected() {
		setProgressBarIndeterminateVisibility(false);
	}

	private int addNewProgressItem(String distribName) {
		ProgressItem progress = mInstaller.getProgressItem(distribName);
		if (progress == null) // if not found
			return -1;
		
		int putIndex = -Arrays.binarySearch(mCurrentProgress, progress, mProgressCompatator)-1;
		
		if (putIndex<0)	// if duplicate
			return -1;
		// generate new array of progresses
		ProgressItem[] newProgressArray = new ProgressItem[mCurrentProgress.length+1];
		System.arraycopy(mCurrentProgress, 0, newProgressArray, 0, putIndex);
		newProgressArray[putIndex] = progress;
		if (putIndex<mCurrentProgress.length)
			System.arraycopy(mCurrentProgress, putIndex, newProgressArray, putIndex+1,
					mCurrentProgress.length-putIndex);
		
		mCurrentProgress = newProgressArray;
		
		mCurrentCancelClickListeners.put(distribName,
				new ProgressCancelOnClickListener(distribName, progress.projectUrl));
		return putIndex;
	}
	
	private int getProgressItem(String distribName) {
		ProgressItem progress = null;
		int position = 0;
		if (mCurrentProgress == null) {
			mCurrentProgress = new ProgressItem[0];
			position = addNewProgressItem(distribName);
			// add to list by updating
			mProgressItemAdapter.notifyDataSetChanged();
			return position;
		}
		
		while (position < mCurrentProgress.length) {
			if (mCurrentProgress[position].name.equals(distribName)) {
				progress = mCurrentProgress[position];
				break;
			}
			position++;
		}
		
		if (progress == null) { // if not found
			position = addNewProgressItem(distribName);
			// add to list by updating
			mProgressItemAdapter.notifyDataSetChanged();
		}
		return position;
	}
	
	@Override
	public void onOperation(String distribName, String opDescription) {
		if (InstallerService.isSimpleOperation(distribName))
			return;
		
		int position = getProgressItem(distribName);
		if (position == -1)
			return;
		
		ProgressItem progress = mCurrentProgress[position];
		
		progress.state = ProgressItem.STATE_IN_PROGRESS;
		progress.opDesc = opDescription;
		progress.progress = -1;
		updateItemView(position);
		
		ifNothingInBackground();
	}

	@Override
	public void onOperationProgress(String distribName, String opDescription, int progressValue) {
		if (InstallerService.isSimpleOperation(distribName))
			return;
		
		int position = getProgressItem(distribName);
		if (position == -1)
			return;
		
		ProgressItem progress = mCurrentProgress[position];
				
		progress.state = ProgressItem.STATE_IN_PROGRESS;
		progress.opDesc = opDescription;
		progress.progress = progressValue;
		updateItemView(position);
		
		ifNothingInBackground();
	}

	@Override
	public boolean onOperationError(String distribName, String errorMessage) {
		if (InstallerService.isSimpleOperation(distribName))
			return false;
		
		int position = getProgressItem(distribName);
		if (position == -1)
			return false;
		
		ProgressItem progress = mCurrentProgress[position];
		
		progress.state = ProgressItem.STATE_ERROR_OCCURRED;
		progress.opDesc = errorMessage;
		progress.progress = -1;
		updateItemView(position);
		
		ifNothingInBackground();
		return true;
	}
	
	@Override
	public void onOperationCancel(String distribName) {
		if (InstallerService.isSimpleOperation(distribName))
			return;
		
		int position = getProgressItem(distribName);
		if (position == -1)
			return;
		
		ProgressItem progress = mCurrentProgress[position];
		
		progress.state = ProgressItem.STATE_CANCELLED;
		progress.opDesc = "";
		progress.progress = -1;
		
		updateItemView(position);
		ifNothingInBackground();
	}

	@Override
	public void onOperationFinish(String distribName) {
		if (InstallerService.isSimpleOperation(distribName))
			return;
		
		if (Logging.DEBUG) Log.d(TAG, "On operation finish:"+distribName);
		int position = getProgressItem(distribName);
		if (position == -1)
			return;
		
		ProgressItem progress = mCurrentProgress[position];
		
		progress.state = ProgressItem.STATE_FINISHED;
		progress.opDesc = "";
		progress.progress = -1;
		
		updateItemView(position);
		ifNothingInBackground();
	}

	@Override
	public void onChangeInstallerIsWorking(boolean isWorking) {
		if (Logging.DEBUG) Log.d(TAG, "Change is working");
		setProgressBarIndeterminateVisibility(isWorking);
	}
}
