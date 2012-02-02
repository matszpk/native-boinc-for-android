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

package sk.boinc.nativeboinc.util;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * @author mat
 *
 */
public class ProjectItem implements Parcelable {

	public final static String TAG = "ProjectItem";
	
	private String mUrl;
	private String mName;
	private String mVersion;
	
	@Override
	public int describeContents() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeString(mUrl);
		dest.writeString(mName);
		dest.writeString(mVersion);
	}
	
	private ProjectItem(Parcel in) {
		mUrl = in.readString();
		mName = in.readString();
		mVersion = in.readString();
	}
	
	public ProjectItem(String name, String url) {
		mName = name;
		mUrl = url;
		mVersion = null;
	}
	
	public ProjectItem(String name, String url, String version) {
		mName = name;
		mUrl = url;
		mVersion = version;
	}
	
	public static final Parcelable.Creator<ProjectItem> CREATOR
		= new Parcelable.Creator<ProjectItem>() {
			@Override
			public ProjectItem createFromParcel(Parcel in) {
				return new ProjectItem(in);
			}
	
			@Override
			public ProjectItem[] newArray(int size) {
				return new ProjectItem[size];
			}
	};

	public String getName() {
		return mName;
	}

	public String getUrl() {
		return mUrl;
	}
	
	public String getVersion() {
		return mVersion;
	}
}
