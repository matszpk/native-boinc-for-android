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
public interface RegularOp {
	public static final int OP_UPDATE_CLIENT_MODE = 2;
	public static final int OP_UPDATE_HOST_INFO = 3;
	
	public static final int OP_UPDATE_PROJECTS = 4;
	public static final int OP_UPDATE_TASKS = 5;
	public static final int OP_UPDATE_TRANSFER = 6;
	public static final int OP_UPDATE_MESSAGES = 7;
	public static final int OP_UPDATE_NOTICES = 8;
	
	public static final int OP_GET_BAM_INFO = 9;
	public static final int OP_ATTACH_TO_BAM = 10;
	public static final int OP_SYNCHRONIZE_WITH_BAM = 11;
	public static final int OP_STOP_USING_BAM = 12;
	
	public static final int OP_GET_ALL_PROJECT_LIST = 13;
	
	public static final int OP_PROJECT_ATTACH = 14;
	public static final int OP_ADD_PROJECT = 15;
	public static final int OP_GET_PROJECT_CONFIG = 16;
	
	public static final int OP_GLOBAL_PREFS_WORKING = 17;
	public static final int OP_GLOBAL_PREFS_OVERRIDE = 18;
	
	public static final int OP_RUN_BENCHMARKS = 19;
	public static final int OP_SET_RUN_MODE = 20;
	public static final int OP_SET_NETWORK_MODE = 21;
	public static final int OP_SHUTDOWN_CORE = 22;
	
	public static final int OP_PROJECT_OP = 23;
	public static final int OP_TASK_OP = 24;
	public static final int OP_TRANSFER_OP = 25;
}
