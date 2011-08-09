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
public class AddProjectResult implements Parcelable {
	public static final String TAG = "AddProjectResult";
	
	private String mName;
	private String mEmail;
	private String mPassword;
	private boolean mCreateAccount;
	
	@Override
	public int describeContents() {
		return 0;
	}
	
	private AddProjectResult(Parcel in) {
		mName = in.readString();
		mEmail = in.readString();
		mPassword = in.readString();
		mCreateAccount = in.readInt()!=0;
	}
	
	public AddProjectResult(String name, String email, String password, boolean createAccount) {
		mName = name;
		mEmail = email;
		mPassword = password;
		mCreateAccount = createAccount;
	}
	

	public static final Parcelable.Creator<AddProjectResult> CREATOR
		= new Parcelable.Creator<AddProjectResult>() {
			@Override
			public AddProjectResult createFromParcel(Parcel in) {
				return new AddProjectResult(in);
			}
	
			@Override
			public AddProjectResult[] newArray(int size) {
				return new AddProjectResult[size];
			}
	};

	@Override
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeString(mName);
		dest.writeString(mEmail);
		dest.writeString(mPassword);
		dest.writeInt(mCreateAccount ? 1 : 0);
	}
	
	public String getName() {
		return mName;
	}
	
	public String getEmail() {
		return mEmail;
	}
	
	public String getPassword() {
		return mPassword;
	}
	
	public boolean isCreateAccount() {
		return mCreateAccount;
	}
}
