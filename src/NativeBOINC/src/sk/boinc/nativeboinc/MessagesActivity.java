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

import edu.berkeley.boinc.lite.Message;

import sk.boinc.nativeboinc.bridge.AutoRefresh;
import sk.boinc.nativeboinc.clientconnection.AutoRefreshListener;
import sk.boinc.nativeboinc.clientconnection.ClientUpdateMessagesReceiver;
import sk.boinc.nativeboinc.clientconnection.MessageInfo;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.service.ConnectionManagerService;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.ScreenOrientationHandler;
import android.app.Activity;
import android.app.ListActivity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;


public class MessagesActivity extends ListActivity implements ClientUpdateMessagesReceiver,
		AutoRefreshListener {
	private static final String TAG = "MessagesActivity";

	private ScreenOrientationHandler mScreenOrientation;

	private boolean mRequestUpdates = false;
	private boolean mViewUpdatesAllowed = false;
	private boolean mViewDirty = false;

	private ArrayList<MessageInfo> mUnfilteredMessages = new ArrayList<MessageInfo>();
	private ArrayList<MessageInfo> mMessages = new ArrayList<MessageInfo>();

	private boolean mUpdateMessagesInProgress = false;
	private long mLastUpdateTime = -1;
	private boolean mAfterRecreating = false;
	
	private int mFilterType = Message.MSG_INFO;
	
	private static class SavedState {
		private final ArrayList<MessageInfo> unfilteredMessages;
		private final ArrayList<MessageInfo> messages;
		private final boolean updateMessagesInProgress;
		private final long lastUpdateTime;
		private final int filterType;

		public SavedState(MessagesActivity activity) {
			unfilteredMessages = activity.mUnfilteredMessages;
			messages = activity.mMessages;
			updateMessagesInProgress = activity.mUpdateMessagesInProgress;
			lastUpdateTime = activity.mLastUpdateTime;
			filterType = activity.mFilterType;
		}
		public void restoreState(MessagesActivity activity) {
			activity.mUnfilteredMessages = unfilteredMessages;
			activity.mMessages = messages;
			activity.mUpdateMessagesInProgress = updateMessagesInProgress;
			activity.mLastUpdateTime = lastUpdateTime;
			activity.mFilterType = filterType;
		}
	}

	private class MessageListAdapter extends BaseAdapter {
		private Context mContext;

		public MessageListAdapter(Context context) {
			mContext = context;
		}

		@Override
		public int getCount() {
            return mMessages.size();
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
			return mMessages.get(position);
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
						R.layout.messages_list_item, parent, false);
			}
			else {
				layout = convertView;
			}
			int color = mContext.getResources().getColorStateList(android.R.color.secondary_text_dark).getDefaultColor();
			if (mMessages.get(position).priority == 2) {
				color = mContext.getResources().getColor(R.color.orange);
			}
			TextView tv;
			tv = (TextView)layout.findViewById(R.id.messageTimestamp);
			tv.setText(mMessages.get(position).time);
			tv.setTextColor(color);
			tv = (TextView)layout.findViewById(R.id.messageProject);
			tv.setText(mMessages.get(position).project);
			tv.setTextColor(color);
			tv = (TextView)layout.findViewById(R.id.messageBody);
			tv.setText(mMessages.get(position).body);
			tv.setTextColor(color);
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
			mConnectionManager.registerStatusObserver(MessagesActivity.this);
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
		getApplicationContext().bindService(new Intent(MessagesActivity.this, ConnectionManagerService.class),
				mServiceConnection, Context.BIND_AUTO_CREATE);
	}

	private void doUnbindService() {
		if (Logging.DEBUG) Log.d(TAG, "doUnbindService()");
		getApplicationContext().unbindService(mServiceConnection);
	}


	private TextView mFilterTypeText = null;
	private String[] mFilterLabelsArray = null;
	private String[] mFilterMenuItemsArray = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.messages);
		
		SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		mFilterType = sharedPrefs.getInt(PreferenceName.LAST_MESSAGES_FILTER, Message.MSG_INFO);
		mFilterLabelsArray = getResources().getStringArray(R.array.messageFilterLabels);
		mFilterMenuItemsArray = getResources().getStringArray(R.array.messageFilterMenuItems);
		
		mFilterTypeText = (TextView)findViewById(R.id.filterText);
		
		registerForContextMenu(getListView());
		registerForContextMenu(findViewById(R.id.emptyContent));
		
		setListAdapter(new MessageListAdapter(this));
		mScreenOrientation = new ScreenOrientationHandler(this);
		doBindService();
		// Restore state on configuration change (if applicable)
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null) {
			// Yes, we have the saved state, this is activity re-creation after configuration change
			savedState.restoreState(this);
			if (!mMessages.isEmpty()) {
				// We restored messages - view will be updated on resume (before we will get refresh)
				mViewDirty = true;
			}
			mAfterRecreating = true;
		}
		ListView lv = getListView();
		lv.setStackFromBottom(true);
		lv.setTranscriptMode(ListView.TRANSCRIPT_MODE_NORMAL);
	}
	
	private void updateFilterType() {
		int filterType = mFilterType;
		if (filterType < 0 || filterType > 3)
			filterType = 0;
		mFilterTypeText.setText(mFilterLabelsArray[filterType]);
		filterMessages();
		((BaseAdapter)getListAdapter()).notifyDataSetChanged();
	}

	@Override
	protected void onResume() {
		super.onResume();
		updateFilterType();
		
		mScreenOrientation.setOrientation();
		mRequestUpdates = true;
		if (mConnectedClient != null) {
			// We are connected right now, request fresh data
			if (Logging.DEBUG) Log.d(TAG, "onResume() - Starting refresh of data");
			mConnectionManager.updateMessages();
		}
		
		Log.d(TAG, "onUpdaMessagesprogress:"+mUpdateMessagesInProgress);
		if (mConnectedClient != null) {
			if (mUpdateMessagesInProgress) {
				ArrayList<MessageInfo> messages = mConnectionManager.getPendingMessages();
				if (messages != null) // if already updated
					updatedMessages(messages);
				
				mConnectionManager.addToScheduledUpdates(this, AutoRefresh.MESSAGES, -1);
			} else { // if after update
				int autoRefresh = mConnectionManager.getAutoRefresh();
				
				if (autoRefresh != -1) {
					long period = SystemClock.elapsedRealtime()-mLastUpdateTime;
					if (period >= autoRefresh*1000) {
						// if later than 4 seconds
						if (Logging.DEBUG) Log.d(TAG, "do update messages");
						mConnectionManager.updateMessages();
					} else { // only add auto updates
						if (Logging.DEBUG) Log.d(TAG, "do add to schedule update messages");
						mConnectionManager.addToScheduledUpdates(this, AutoRefresh.MESSAGES,
								(int)(autoRefresh*1000-period));
					}
				}
			}
		}
		
		mViewUpdatesAllowed = true;
		if (mViewDirty) {
			// There were some updates received while we were not visible
			// The data are stored, but view is not updated yet; Do it now
			sortMessages();
			filterMessages();
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
			mConnectionManager.cancelScheduledUpdates(AutoRefresh.MESSAGES);
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
	public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfo) {
		menu.setHeaderTitle(R.string.messageFilters);
		for (int i = 1; i <= Message.MSG_INTERNAL_ERROR; i++) {
			MenuItem item = menu.add(0, i, 0, mFilterMenuItemsArray[i]);
			item.setCheckable(true);
			item.setChecked(i == mFilterType);
		}
	}
	
	@Override
	public boolean onContextItemSelected(MenuItem item) {
		mFilterType = item.getItemId();
		updateFilterType();
		
		SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		sharedPrefs.edit().putInt(PreferenceName.LAST_MESSAGES_FILTER, mFilterType).commit();
		return true;
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
				mConnectionManager.updateMessages();
				if (!mUpdateMessagesInProgress && !mAfterRecreating) {
					if (Logging.DEBUG) Log.d(TAG, "do update messages");
					mUpdateMessagesInProgress = true;
					mConnectionManager.updateMessages();
				} else {
					if (Logging.DEBUG) Log.d(TAG, "do add to scheduled updates");
					int autoRefresh = mConnectionManager.getAutoRefresh();
					
					if (autoRefresh != -1) {
						long period = SystemClock.elapsedRealtime()-mLastUpdateTime;
						mConnectionManager.addToScheduledUpdates(this, AutoRefresh.MESSAGES,
								(int)(autoRefresh*1000-period));
					} else
						mConnectionManager.addToScheduledUpdates(this, AutoRefresh.MESSAGES, -1);
					mAfterRecreating = false;
				}
			}
		}
	}

	@Override
	public void clientDisconnected() {
		if (Logging.DEBUG) Log.d(TAG, "Client is disconnected");
		mConnectedClient = null;
		mUpdateMessagesInProgress = false;
		mUnfilteredMessages.clear();
		mMessages.clear();
		((BaseAdapter)getListAdapter()).notifyDataSetChanged();
		mViewDirty = false;
		mAfterRecreating = false;
	}
	
	@Override
	public boolean clientError(int err_num, String message) {
		// do not consume
		mUpdateMessagesInProgress = false;
		mAfterRecreating = false;
		return false;
	}

	@Override
	public boolean updatedMessages(ArrayList<MessageInfo> messages) {
		mUpdateMessagesInProgress = false;
		mLastUpdateTime = SystemClock.elapsedRealtime();
		if (mUnfilteredMessages.size() != messages.size()) {
			// Number of messages has changed (increased)
			// This is the only case when we need an update, because content of messages
			// never changes, only fresh arrived messages are added to list
			mUnfilteredMessages = messages;
			filterMessages();
			if (mViewUpdatesAllowed) {
				// We are visible, update the view with fresh data
				if (Logging.DEBUG) Log.d(TAG, "Messages are updated, refreshing view");
				sortMessages();
				filterMessages();
				((BaseAdapter)getListAdapter()).notifyDataSetChanged();
			}
			else {
				// We are not visible, do not perform costly tasks now
				if (Logging.DEBUG) Log.d(TAG, "Messages are updated, but view refresh is delayed");
				mViewDirty = true;
			}
		}
		return mRequestUpdates;
	}


	private void sortMessages() {
		Comparator<MessageInfo> comparator = new Comparator<MessageInfo>() {
			@Override
			public int compare(MessageInfo object1, MessageInfo object2) {
				return object1.seqNo - object2.seqNo;
			}
		};
		Collections.sort(mUnfilteredMessages, comparator);
	}
	
	private void filterMessages() {
		mMessages.clear();
		for (MessageInfo msgInfo: mUnfilteredMessages)
			if (msgInfo.priority >= mFilterType || msgInfo.priority == 0)
				mMessages.add(msgInfo);
	}
	
	@Override
	public void onClientIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void onStartAutoRefresh(int requestType) {
		if (requestType == AutoRefresh.MESSAGES) // in progress
			mUpdateMessagesInProgress = true;
	}
}
