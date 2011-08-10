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

import edu.berkeley.boinc.lite.ProjectConfig;
import android.os.Parcel;
import android.os.Parcelable;

/**
 * @author mat
 *
 */
public class ProjectConfigOptions implements Parcelable {
	public static final String TAG = "ProjectConfig";
	
	private String mName;
	private String mMasterUrl;
	private String mTermsOfUse;
	private int mMinPasswordLength;
	private boolean mAccountCreationDisabled;
	private boolean mClientAccountCreationDisabled;
	
	@Override
	public int describeContents() {
		return 0;
	}
	
	public ProjectConfigOptions(ProjectConfig config) {
		mName = config.name;
		mMasterUrl = config.master_url;
		mTermsOfUse = config.terms_of_use;
		mMinPasswordLength = config.min_passwd_length;
		mAccountCreationDisabled = config.account_creation_disabled;
		mClientAccountCreationDisabled = config.client_account_creation_disabled;
	}
	
	private ProjectConfigOptions(Parcel in) {
		mName = in.readString();
		mMasterUrl = in.readString();
		mTermsOfUse = in.readString();
		mMinPasswordLength = in.readInt();
		mAccountCreationDisabled = in.readInt()!=0;
		mClientAccountCreationDisabled = in.readInt()!=0;
	}
	
	public static final Parcelable.Creator<ProjectConfigOptions> CREATOR
		= new Parcelable.Creator<ProjectConfigOptions>() {
			@Override
			public ProjectConfigOptions createFromParcel(Parcel in) {
				return new ProjectConfigOptions(in);
			}
	
			@Override
			public ProjectConfigOptions[] newArray(int size) {
				return new ProjectConfigOptions[size];
			}
	};

	@Override
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeString(mName);
		dest.writeString(mMasterUrl);
		dest.writeString(mTermsOfUse);
		dest.writeInt(mMinPasswordLength);
		dest.writeInt(mAccountCreationDisabled ? 1 : 0);
		dest.writeInt(mClientAccountCreationDisabled ? 1 : 0);
	}
	
	public String getName() {
		return mName;
	}
	
	public String getMasterUrl() {
		return mMasterUrl;
	}
	
	public String getTermsOfUse() {
		return mTermsOfUse;
	}
	
	public int getMinPasswordLength() {
		return mMinPasswordLength;
	}
	
	public boolean isAccountCreationDisabled() {
		return mAccountCreationDisabled;
	}
	
	public boolean isClientAccountCreationDisabled() {
		return mClientAccountCreationDisabled;
	}
}
