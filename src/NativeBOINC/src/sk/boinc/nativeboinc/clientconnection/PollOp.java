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

package sk.boinc.nativeboinc.clientconnection;

/**
 * @author mat
 *
 */
public interface PollOp {
	// polling operation - 
	public static final int POLL_ATTACH_TO_BAM = 0;
	public static final int POLL_SYNC_WITH_BAM = 1;
	public static final int POLL_LOOKUP_ACCOUNT = 2;
	public static final int POLL_CREATE_ACCOUNT = 3;
	public static final int POLL_PROJECT_ATTACH = 4;
	public static final int POLL_PROJECT_CONFIG = 5;
	
	public static final int POLL_BAM_OPERATION_MASK = 1<<0;
	public static final int POLL_LOOKUP_ACCOUNT_MASK = 1<<1;
	public static final int POLL_CREATE_ACCOUNT_MASK = 1<<2;
	public static final int POLL_PROJECT_ATTACH_MASK = 1<<3;
	public static final int POLL_PROJECT_CONFIG_MASK = 1<<4;
	
	public static final int POLL_ALL_MASK = (1<<5)-1;
}
