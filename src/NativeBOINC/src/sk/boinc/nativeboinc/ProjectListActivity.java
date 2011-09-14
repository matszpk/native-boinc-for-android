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

import sk.boinc.nativeboinc.util.ProjectItem;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Parcelable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class ProjectListActivity extends ListActivity {	
	private static final int DIALOG_ENTER_URL = 1;
	
	public static final String TAG_OTHER_PROJECT_OPTION = "OtherProjectOption"; 
	
	private Parcelable[] mProjects;
	private boolean mOtherProjectOption = true;
	
	private class ProjectListAdapter extends BaseAdapter {
		private Context mContext;
		
		public ProjectListAdapter(Context context) {
			mContext = context;
		}
		
		@Override
		public int getCount() {
			return (mOtherProjectOption) ? mProjects.length+1 : mProjects.length;
		}

		@Override
		public Object getItem(int index) {
			if (mProjects.length <= index)
				return null;
			return mProjects[index];
		}

		@Override
		public long getItemId(int index) {
			return index;
		}

		@Override
		public View getView(int pos, View inView, ViewGroup parent) {
			View view = inView;
			if (view == null) {
				LayoutInflater inflater = (LayoutInflater)mContext.getSystemService(LAYOUT_INFLATER_SERVICE);
				view = inflater.inflate(android.R.layout.simple_list_item_2, null);
			}
			
			TextView text1 = (TextView)view.findViewById(android.R.id.text1);
			TextView text2 = (TextView)view.findViewById(android.R.id.text2);
			if (pos < mProjects.length) {
				ProjectItem projectItem = (ProjectItem)mProjects[pos];
				text1.setText(projectItem.getName());
				text2.setText(projectItem.getUrl());
			} else if (pos == mProjects.length) {
				text1.setText(getString(R.string.otherProject));
				text2.setText(getString(R.string.otherProjectDetails));
			}
			return view;
		}
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.project_list);
		
		Intent intent = getIntent();
		mProjects = intent.getParcelableArrayExtra(ProjectItem.TAG);
		mOtherProjectOption = intent.getBooleanExtra(TAG_OTHER_PROJECT_OPTION, true);
		
		if (mProjects == null)
			mProjects = new ProjectItem[0];
		setListAdapter(new ProjectListAdapter(this));
	}
		
	@Override
    protected void onListItemClick(ListView listView, View view, int pos, long id) {
		super.onListItemClick(listView, view, pos, id);
		
		if (pos < mProjects.length) {
			Intent intent = new Intent();
			intent.putExtra(ProjectItem.TAG, mProjects[pos]);
			setResult(RESULT_OK, intent);
			finish();
		} else if (pos == mProjects.length) {
			showDialog(DIALOG_ENTER_URL);
		}
	}
	
	@Override
	protected Dialog onCreateDialog(int id) {
		if (id == DIALOG_ENTER_URL) {
			View view = LayoutInflater.from(this).inflate(R.layout.project_url, null);
			final EditText urlEdit = (EditText)view.findViewById(R.id.projectUrlUrl);
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_input_get)
				.setTitle(R.string.projectUrl)
				.setView(view)
				.setPositiveButton(R.string.ok, new Dialog.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						Intent intent = new Intent();
						intent.putExtra(ProjectItem.TAG, new ProjectItem("", urlEdit.getText().toString()));
						setResult(RESULT_OK, intent);
						finish();
					}
				})
				.setNegativeButton(R.string.cancel, new Dialog.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
					}
				})
				.create();
		}
		return null;
	}	
}
