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

import sk.boinc.nativeboinc.news.NewsMessage;
import sk.boinc.nativeboinc.util.ScreenOrientationHandler;
import android.app.ListActivity;
import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class NewsActivity extends ListActivity {

	private static final String MESSAGE_NEWS_STATE = "MessageNewsState";
	
	private ScreenOrientationHandler mScreenOrientation = null;
	
	private ArrayList<NewsMessage> mNewsMessages = null;
	
	private class NewsMessageAdapter extends BaseAdapter {

		private Context mContext = null;
		
		public NewsMessageAdapter(Context context) {
			mContext = context;
		}
		
		@Override
		public int getCount() {
			return (mNewsMessages != null) ? mNewsMessages.size() : 0; 
		}

		@Override
		public Object getItem(int id) {
			return mNewsMessages.get(id);
		}

		@Override
		public long getItemId(int position) { 
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			View view = convertView;
			
			if (convertView == null) {
				LayoutInflater inflater = LayoutInflater.from(mContext);
				view = inflater.inflate(R.layout.news_item, null);
			}
			
			NewsMessage message = mNewsMessages.get(position);
			
			TextView titleView = (TextView)view.findViewById(R.id.title);
			TextView timeView = (TextView)view.findViewById(R.id.time);
			TextView contentView = (TextView)view.findViewById(R.id.content);
			
			titleView.setText(message.getTitle());
			timeView.setText(message.getFormattedTime());
			contentView.setText(message.getContent());
			
			return view;
		}
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mScreenOrientation = new ScreenOrientationHandler(this);
		
		if (savedInstanceState != null)
			mNewsMessages = savedInstanceState.getParcelableArrayList(MESSAGE_NEWS_STATE);
		
		if (mNewsMessages != null)
			setListAdapter(new NewsMessageAdapter(this));
	}
	
	@Override
	protected void onSaveInstanceState(Bundle outState) {
		super.onSaveInstanceState(outState);
		outState.putParcelableArrayList(MESSAGE_NEWS_STATE, mNewsMessages);
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		mScreenOrientation.setOrientation();
	}
	
	@Override
	protected void onDestroy() {
		mScreenOrientation = null;
		super.onDestroy();
	}
}
