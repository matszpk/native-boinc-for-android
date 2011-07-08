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

import sk.boinc.androboinc.clientconnection.ModeInfo;
import edu.berkeley.boinc.lite.CcStatus;


public class ModeInfoCreator {
	public static ModeInfo create(final CcStatus ccStatus) {
		// Consistency checks, so the data received from remote client are those that we can process
		if ( (ccStatus.task_mode > 3) || (ccStatus.task_mode < 1) ) {
			return null;
		}
		if ( (ccStatus.network_mode > 3) || (ccStatus.network_mode < 1) ) {
			return null;
		}
		ModeInfo mi = new ModeInfo();
		mi.task_mode = ccStatus.task_mode;
		mi.network_mode = ccStatus.network_mode;
		return mi;
	}
}
