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

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import sk.boinc.nativeboinc.bridge.AutoRefresh;
import sk.boinc.nativeboinc.clientconnection.AutoRefreshListener;
import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.ClientUpdateNoticesReceiver;
import sk.boinc.nativeboinc.clientconnection.NoticeInfo;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.service.ConnectionManagerService;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.ScreenOrientationHandler;
import android.app.Activity;
import android.app.ListActivity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.os.SystemClock;
import android.text.Html;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class NoticesActivity extends ListActivity implements ClientUpdateNoticesReceiver,
	AutoRefreshListener {

	private static final String TAG = "NoticesActivity";
	
	private boolean mRequestUpdates = false;
	private boolean mViewUpdatesAllowed = false;
	private boolean mViewDirty = false;
	
	private ScreenOrientationHandler mScreenOrientation;
	
	private ArrayList<NoticeInfo> mNotices = new ArrayList<NoticeInfo>();
	
	private boolean mUpdateNoticesInProgress = false;
	private long mLastUpdateTime = -1;
	private boolean mAfterRecreating = false;
	
	private static class SavedState {
		private final ArrayList<NoticeInfo> notices;
		private final boolean updateNoticesInProgress;
		private final long lastUpdateTime;
		
		public SavedState(NoticesActivity activity) {
			notices = activity.mNotices;
			updateNoticesInProgress = activity.mUpdateNoticesInProgress;
			lastUpdateTime = activity.mLastUpdateTime;
		}
		public void restoreState(NoticesActivity activity) {
			activity.mNotices = notices;
			activity.mUpdateNoticesInProgress = updateNoticesInProgress;
			activity.mLastUpdateTime = lastUpdateTime;
		}
	}
	
	private class NoticeListAdapter extends BaseAdapter {
		private Context mContext;

		public NoticeListAdapter(Context context) {
			mContext = context;
		}

		@Override
		public int getCount() {
            return mNotices.size();
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
			return mNotices.get(position);
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
						R.layout.notices_list_item, parent, false);
			}
			else {
				layout = convertView;
			}
			TextView tv;
			NoticeInfo noticeInfo = mNotices.get(position); 
			tv = (TextView)layout.findViewById(R.id.noticeCreateTime);
			tv.setText(noticeInfo.create_time);
			tv = (TextView)layout.findViewById(R.id.noticeTitle);
			if (noticeInfo.title != null && noticeInfo.title.length() != 0) {
				tv.setText(noticeInfo.title_project);
				tv.setVisibility(View.VISIBLE);
			} else {
				tv.setVisibility(View.GONE);
				tv.setText("");
			}
			tv = (TextView)layout.findViewById(R.id.noticeDescription);
			tv.setText(Html.fromHtml(noticeInfo.description));
			tv = (TextView)layout.findViewById(R.id.noticeLink);
			tv.setText(Html.fromHtml(noticeInfo.link_html));
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
			mConnectionManager.registerStatusObserver(NoticesActivity.this);
			
			if (mConnectionManager.getClientId() == null) {
				if (Logging.DEBUG) Log.d(TAG, "when is now disconnected");
				// if after disconnection
				clientDisconnected(false);
			}
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (mConnectionManager != null)
				mConnectionManager.unregisterStatusObserver(NoticesActivity.this);
			mConnectionManager = null;
			// This should not happen normally, because it's local service 
			// running in the same process...
			if (Logging.WARNING) Log.w(TAG, "onServiceDisconnected()");
		}
	};

	private void doBindService() {
		if (Logging.DEBUG) Log.d(TAG, "doBindService()");
		getApplicationContext().bindService(new Intent(NoticesActivity.this, ConnectionManagerService.class),
				mServiceConnection, Context.BIND_AUTO_CREATE);
	}

	private void doUnbindService() {
		if (Logging.DEBUG) Log.d(TAG, "doUnbindService()");
		getApplicationContext().unbindService(mServiceConnection);
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setListAdapter(new NoticeListAdapter(this));
		mScreenOrientation = new ScreenOrientationHandler(this);
		doBindService();
		
		// Restore state on configuration change (if applicable)
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null) {
			// Yes, we have the saved state, this is activity re-creation after configuration change
			savedState.restoreState(this);
			if (!mNotices.isEmpty()) {
				// We restored notices - view will be updated on resume (before we will get refresh)
				mViewDirty = true;
			}
			mAfterRecreating = true;
		}
		ListView lv = getListView();
		lv.setStackFromBottom(true);
		lv.setTranscriptMode(ListView.TRANSCRIPT_MODE_NORMAL);
		lv.setFastScrollEnabled(true);
		
		lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {

			@Override
			public void onItemClick(AdapterView<?> parent, View view, int position,
					long id) {
				NoticeInfo notice = mNotices.get(position);
				if (notice.link != null && notice.link.length() != 0) {
					Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(notice.link));
					startActivity(intent);
				}
			}
		});
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		mScreenOrientation.setOrientation();
		mRequestUpdates = true;
		
		Log.d(TAG, "onUpdateNoticesProgress:"+mUpdateNoticesInProgress);
		if (mConnectedClient != null) {
			if (mUpdateNoticesInProgress) {
				ArrayList<NoticeInfo> notices = (ArrayList<NoticeInfo>)mConnectionManager
						.getPendingOutput(BoincOp.UpdateNotices);
				
				if (notices != null) // if already updated
					updatedNotices(notices);
				
				mConnectionManager.addToScheduledUpdates(this, AutoRefresh.NOTICES, -1);
			} else { // if after update
				int autoRefresh = mConnectionManager.getAutoRefresh();
				
				if (autoRefresh != -1) {
					long period = SystemClock.elapsedRealtime()-mLastUpdateTime;
					if (period >= autoRefresh*1000) {
						// if later than 4 seconds
						if (Logging.DEBUG) Log.d(TAG, "do update notices");
						mConnectionManager.updateNotices();
					} else { // only add auto updates
						if (Logging.DEBUG) Log.d(TAG, "do add to schedule update notices");
						mConnectionManager.addToScheduledUpdates(this, AutoRefresh.NOTICES,
								(int)(autoRefresh*1000-period));
					}
				}
			}
		}
		
		mViewUpdatesAllowed = true;
		if (mViewDirty) {
			// There were some updates received while we were not visible
			// The data are stored, but view is not updated yet; Do it now
			sortNotices();
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
			mConnectionManager.cancelScheduledUpdates(AutoRefresh.NOTICES);
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
	
	@Override
	public boolean clientError(BoincOp boincOp, int err_num, String message) {
		if (!boincOp.equals(BoincOp.UpdateNotices))
			return false;
		// do not consume
		mUpdateNoticesInProgress = false;
		mAfterRecreating = false;
		return false;
	}

	@Override
	public void clientConnectionProgress(BoincOp boincOp, int progress) {
		// TODO Auto-generated method stub
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		mConnectedClient = mConnectionManager.getClientId();
		if (mConnectedClient != null) {
			// Connected client is retrieved
			if (Logging.DEBUG) Log.d(TAG, "Client is connected");
			if (mRequestUpdates) {
				if (!mUpdateNoticesInProgress && !mAfterRecreating) {
					if (Logging.DEBUG) Log.d(TAG, "do update notices");
					mUpdateNoticesInProgress = true;
					mConnectionManager.updateNotices();
				} else {
					if (Logging.DEBUG) Log.d(TAG, "do add to scheduled updates");
					int autoRefresh = mConnectionManager.getAutoRefresh();
					
					if (autoRefresh != -1) {
						long period = SystemClock.elapsedRealtime()-mLastUpdateTime;
						mConnectionManager.addToScheduledUpdates(this, AutoRefresh.NOTICES,
								(int)(autoRefresh*1000-period));
					} else
						mConnectionManager.addToScheduledUpdates(this, AutoRefresh.NOTICES, -1);
					mAfterRecreating = false;
				}
			}
		}
	}

	@Override
	public void clientDisconnected(boolean disconnectedByManager) {
		if (Logging.DEBUG) Log.d(TAG, "Client is disconnected");
		mConnectedClient = null;
		mUpdateNoticesInProgress = false;
		mNotices.clear();
		((BaseAdapter)getListAdapter()).notifyDataSetChanged();
		mViewDirty = false;
		mAfterRecreating = false;
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public boolean updatedNotices(ArrayList<NoticeInfo> notices) {
		mUpdateNoticesInProgress = false;
		mLastUpdateTime = SystemClock.elapsedRealtime();
		if (Logging.DEBUG) Log.d(TAG, "After update notices");
		if (mNotices.size() != notices.size()) {
			// Number of notices has changed (increased)
			// This is the only case when we need an update, because content of notices
			// never changes, only fresh arrived notices are added to list
			mNotices = notices;
			if (mViewUpdatesAllowed) {
				// We are visible, update the view with fresh data
				if (Logging.DEBUG) Log.d(TAG, "Notices are updated, refreshing view");
				sortNotices();
				((BaseAdapter)getListAdapter()).notifyDataSetChanged();
			}
			else {
				// We are not visible, do not perform costly tasks now
				if (Logging.DEBUG) Log.d(TAG, "Notices are updated, but view refresh is delayed");
				mViewDirty = true;
			}
		}
		return mRequestUpdates;
	}
	
	private void sortNotices() {
		Comparator<NoticeInfo> comparator = new Comparator<NoticeInfo>() {
			@Override
			public int compare(NoticeInfo object1, NoticeInfo object2) {
				return object1.seqNo - object2.seqNo;
			}
		};
		Collections.sort(mNotices, comparator);
	}
	

	@Override
	public void onStartAutoRefresh(int requestType) {
		if (requestType == AutoRefresh.NOTICES) // in progress
			mUpdateNoticesInProgress = true;
	}
}
