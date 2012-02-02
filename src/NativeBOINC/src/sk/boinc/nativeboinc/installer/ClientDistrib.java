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

package sk.boinc.nativeboinc.installer;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * @author mat
 *
 */
public class ClientDistrib implements Parcelable {
	public String version = "";
	public String filename = "";
	public String description = "";
	public String changes = "";
	
	public ClientDistrib() { }
	
	private ClientDistrib(Parcel in) {
		version = in.readString();
		filename = in.readString();
		description = in.readString();
		changes = in.readString();
	}
	
	@Override
	public int describeContents() {
		return 0;
	}
	@Override
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeString(version);
		dest.writeString(filename);
		dest.writeString(description);
		dest.writeString(changes);
	}
	
	public static final Parcelable.Creator<ClientDistrib> CREATOR
		= new Parcelable.Creator<ClientDistrib>() {
			@Override
			public ClientDistrib createFromParcel(Parcel in) {
				return new ClientDistrib(in);
			}
	
			@Override
			public ClientDistrib[] newArray(int size) {
				return new ClientDistrib[size];
			}
	};
}
