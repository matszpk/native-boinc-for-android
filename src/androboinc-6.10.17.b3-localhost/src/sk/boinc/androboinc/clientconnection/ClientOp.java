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


public interface ClientOp {
	// Project operations - should be in sync with back-end class
	public static final int PROJECT_UPDATE  = 1;
	public static final int PROJECT_SUSPEND = 2;
	public static final int PROJECT_RESUME  = 3;
	public static final int PROJECT_NNW     = 4;
	public static final int PROJECT_ANW     = 5;

	// Result operations - should be in sync with back-end class
	public static final int TASK_SUSPEND  = 1;
	public static final int TASK_RESUME   = 2;
	public static final int TASK_ABORT    = 3;

	// Transfer operations - should be in sync with back-end class
	public static final int TRANSFER_RETRY  = 1;
	public static final int TRANSFER_ABORT  = 2;
}
