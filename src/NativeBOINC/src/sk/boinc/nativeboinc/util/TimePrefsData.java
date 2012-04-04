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

import edu.berkeley.boinc.lite.TimePreferences;
import android.os.Parcel;
import android.os.Parcelable;

/**
 * @author mat
 *
 */
public class TimePrefsData implements Parcelable {

	public TimePreferences timePrefs = new TimePreferences();
	
	public static final Parcelable.Creator<TimePrefsData> CREATOR
		= new Parcelable.Creator<TimePrefsData>() {
			@Override
			public TimePrefsData createFromParcel(Parcel in) {
				return new TimePrefsData(in);
			}
	
			@Override
			public TimePrefsData[] newArray(int size) {
				return new TimePrefsData[size];
			}
	};
	
	public TimePrefsData(TimePreferences timePrefs) {
		this.timePrefs = timePrefs;
	}
	
	private TimePrefsData(Parcel src) {
		timePrefs.start_hour = src.readDouble();
		timePrefs.end_hour = src.readDouble();
		
		TimePreferences.TimeSpan[] weekPrefs = timePrefs.week_prefs;
		for (int i = 0; i < 7; i++) {
			byte exists = src.readByte();
			if (exists == 1) {
				weekPrefs[i] = new TimePreferences.TimeSpan();
				weekPrefs[i].start_hour = src.readDouble();
				weekPrefs[i].end_hour = src.readDouble();
			} else
				weekPrefs[i] = null;
		}
	}
	
	@Override
	public int describeContents() {
		return 0;
	}

	@Override
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeDouble(timePrefs.start_hour);
		dest.writeDouble(timePrefs.end_hour);
		
		for (TimePreferences.TimeSpan timeSpan: timePrefs.week_prefs) {
			dest.writeByte((byte)(timeSpan != null ? 1 : 0));
			if (timeSpan != null) {
				dest.writeDouble(timeSpan.start_hour);
				dest.writeDouble(timeSpan.end_hour);
			}
		}
	}
}
