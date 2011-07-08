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

package sk.boinc.androboinc.util;

import android.database.Cursor;
import android.os.Parcel;
import android.os.Parcelable;

public class ClientId implements Parcelable {
	public static final String TAG = "ClientId";
	private static final int HASH_SEED = 17;
	private static final int HASH_MULTI = 31;
	private int mHashCode = 0; // for hashCode()/equals()

	private long   mId = -1;
	private String mNickname = null;
	private String mAddress = null;
	private int    mPort = 31416;
	private String mPassword = null;

	@Override
	public int describeContents() {
		return 0;
	}

	@Override
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeLong(mId);
		dest.writeString(mNickname);
		dest.writeString(mAddress);
		dest.writeInt(mPort);
		dest.writeString(mPassword);
	}

	public static final Parcelable.Creator<ClientId> CREATOR
		= new Parcelable.Creator<ClientId>() {
			@Override
			public ClientId createFromParcel(Parcel in) {
				return new ClientId(in);
			}

			@Override
			public ClientId[] newArray(int size) {
				return new ClientId[size];
			}
	};

	private ClientId(Parcel in) {
		mId = in.readLong();
		mNickname = in.readString();
		mAddress = in.readString();
		mPort = in.readInt();
		mPassword = in.readString();
	}

	public ClientId(long id, String nickname, String address, int port, String password) {
		mId = id;
		mNickname = nickname;
		mAddress = address;
		mPort = port;
		mPassword = password;
	}

	public ClientId(Cursor c) {
		mId       = c.getLong(c.getColumnIndexOrThrow(HostListDbAdapter.KEY_ROWID));
		mNickname = c.getString(c.getColumnIndexOrThrow(HostListDbAdapter.FIELD_HOST_NICKNAME));
		mAddress  = c.getString(c.getColumnIndexOrThrow(HostListDbAdapter.FIELD_HOST_ADDRESS));
		mPort     = c.getInt(c.getColumnIndexOrThrow(HostListDbAdapter.FIELD_HOST_PORT));
		mPassword = c.getString(c.getColumnIndexOrThrow(HostListDbAdapter.FIELD_HOST_PASSWORD));
	}

	@Override
	public boolean equals(Object o) {
		if (this == o) return true;
		if (!(o instanceof ClientId)) return false;
		return (o.hashCode() == this.hashCode());
	}

	@Override
	public int hashCode() {
		if (mHashCode != 0) return mHashCode;
		mHashCode = HASH_SEED;
		mHashCode = HASH_MULTI * mHashCode + mAddress.hashCode();
		mHashCode = HASH_MULTI * mHashCode + mPort;
		mHashCode = HASH_MULTI * mHashCode + mPassword.hashCode();
		return mHashCode;
	}

	public final long getId() {
		return mId;
	}

	public final String getNickname() {
		return mNickname;
	}

	public final String getAddress() {
		return mAddress;
	}

	public final int getPort() {
		return mPort;
	}

	public final String getPassword() {
		return mPassword;
	}

	public void updateHost(String nickname, String address, int port, String password) {
		mNickname = nickname;
		mAddress = address;
		mPort = port;
		mPassword = password;
	}
}
