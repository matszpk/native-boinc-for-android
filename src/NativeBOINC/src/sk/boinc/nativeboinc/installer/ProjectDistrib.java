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
 * class of project distribution info
 */
public class ProjectDistrib implements Parcelable {
	public String projectName = "";
	public String projectUrl = "";
	public String version = "";
	public String filename = "";
	public String description = "";
	public String changes = "";
	
	public ProjectDistrib() { }
	
	private ProjectDistrib(Parcel in) {
		projectName = in.readString();
		projectUrl = in.readString();
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
		dest.writeString(projectName);
		dest.writeString(projectUrl);
		dest.writeString(version);
		dest.writeString(filename);
		dest.writeString(description);
		dest.writeString(changes);
	}
	
	public static final Parcelable.Creator<ProjectDistrib> CREATOR
		= new Parcelable.Creator<ProjectDistrib>() {
			@Override
			public ProjectDistrib createFromParcel(Parcel in) {
				return new ProjectDistrib(in);
			}
	
			@Override
			public ProjectDistrib[] newArray(int size) {
				return new ProjectDistrib[size];
			}
	};
}
