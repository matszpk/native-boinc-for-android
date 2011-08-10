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
public class BAMAccount implements Parcelable {
	public final static String TAG = "BAMAccount";
	
	public String mName;
	public String mUrl;
	public String mPassword;
	
	@Override
	public int describeContents() {
		return 0;
	}
	
	@Override
	public void writeToParcel(Parcel dest, int flags) { 
		dest.writeString(mName);
		dest.writeString(mUrl);
		dest.writeString(mPassword);
	}
	
	private BAMAccount(Parcel in) {
		mName = in.readString();
		mUrl = in.readString();
		mPassword = in.readString();
	}
	
	public static final Parcelable.Creator<BAMAccount> CREATOR
		= new Parcelable.Creator<BAMAccount>() {
			@Override
			public BAMAccount createFromParcel(Parcel in) {
				return new BAMAccount(in);
			}
	
			@Override
			public BAMAccount[] newArray(int size) {
				return new BAMAccount[size];
			}
	};
	
	public BAMAccount(String name, String url, String password) {
		mName = name;
		mUrl = url;
		mPassword = password;
	}
	
	public final String getName() {
		return mName;
	}

	public final String getUrl() {
		return mUrl;
	}

	public final String getPassword() {
		return mPassword;
	}
}
