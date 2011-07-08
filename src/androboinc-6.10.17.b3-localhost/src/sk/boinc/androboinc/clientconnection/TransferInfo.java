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

package sk.boinc.androboinc.clientconnection;

/**
 * Description of BOINC transfer for AndroBOINC purpose
 * Reflects the classes of BOINC-library
 */
public class TransferInfo {
	public String fileName;     // unique ID
	public String projectUrl;   // Transfer.project_url
	public int    stateControl; // state control in numerical form
	public static final int SUSPENDED = 1; // bit 1 of stateControl
	public static final int ABORTED = 2;   // bit 2 of stateControl
	public static final int FAILED = 4;    // bit 3 of stateControl
	public static final int RUNNING = 8;   // bit 4 of stateControl
	public static final int STARTED = 16;  // bit 5 of stateControl
	public int    progInd;      // Progress indication in numerical form
	public String project;      // Project.getName()
	public String progress;     // converted to percentage
	public String size;         // converted to string
	public String elapsed;      // converted to time-string
	public String speed;        // converted to string
	public String state;        // converted to string
}
