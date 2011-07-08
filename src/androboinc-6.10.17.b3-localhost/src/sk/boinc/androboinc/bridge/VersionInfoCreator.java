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

package sk.boinc.androboinc.bridge;

import sk.boinc.androboinc.clientconnection.VersionInfo;


public class VersionInfoCreator {
	public static VersionInfo create(final edu.berkeley.boinc.lite.VersionInfo versionInfo) {
		VersionInfo vi = new VersionInfo();
		vi.versNum = versionInfo.major * 100000 + versionInfo.minor * 1000 + versionInfo.release;
		vi.version = String.format("%d.%d.%d", versionInfo.major, versionInfo.minor, versionInfo.release);
		return vi;
	}
}
