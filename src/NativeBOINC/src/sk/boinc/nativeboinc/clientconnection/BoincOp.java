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

import sk.boinc.nativeboinc.bridge.AutoRefresh;

/**
 * @author mat
 *
 */
public class BoincOp {
	public static final int OP_CONNECT = 0;
	public static final int OP_DISCONNECT = 1;
	
	public static final int OP_UPDATE_CLIENT_MODE = 2;
	public static final int OP_UPDATE_HOST_INFO = 3;
	
	public static final int OP_UPDATE_PROJECTS = 4;
	public static final int OP_UPDATE_TASKS = 5;
	public static final int OP_UPDATE_TRANSFERS = 6;
	public static final int OP_UPDATE_MESSAGES = 7;
	public static final int OP_UPDATE_NOTICES = 8;
	
	public static final int OP_GET_BAM_INFO = 9;
	public static final int OP_BAM_SYNCHRONIZE = 10;
	public static final int OP_STOP_USING_BAM = 11;
	
	public static final int OP_GET_ALL_PROJECT_LIST = 12;
	
	public static final int OP_LOOKUP_ACCOUNT = 13;
	public static final int OP_CREATE_ACCOUNT = 14;
	public static final int OP_ATTACH_PROJECT = 15;
	public static final int OP_ADD_PROJECT = 16;
	public static final int OP_GET_PROJECT_CONFIG = 17;
	
	public static final int OP_GLOBAL_PREFS_WORKING = 18;
	public static final int OP_GLOBAL_PREFS_OVERRIDE = 19;
	
	public static final int OP_RUN_BENCHMARKS = 20;
	public static final int OP_SET_RUN_MODE = 21;
	public static final int OP_SET_NETWORK_MODE = 22;
	public static final int OP_SET_GPU_MODE = 23;
	public static final int OP_SHUTDOWN_CORE = 24;
	public static final int OP_DO_NETWORK_COMM = 25;
	
	public static final int OP_PROJECT_OP = 26;
	public static final int OP_TASK_OP = 27;
	public static final int OP_TRANSFER_OP = 28;
	
	public static final int OP_SET_PROXY_SETTINGS = 29;
	public static final int OP_GET_PROXY_SETTINGS = 30;
	
	public static final BoincOp Connect = new BoincOp(OP_CONNECT);
	public static final BoincOp Disconnect = new BoincOp(OP_DISCONNECT);
	public static final BoincOp UpdateClientMode = new BoincOp(OP_UPDATE_CLIENT_MODE);
	public static final BoincOp UpdateHostInfo = new BoincOp(OP_UPDATE_HOST_INFO);
	
	public static final BoincOp UpdateProjects = new BoincOp(OP_UPDATE_PROJECTS);
	public static final BoincOp UpdateTasks = new BoincOp(OP_UPDATE_TASKS);
	public static final BoincOp UpdateTransfers = new BoincOp(OP_UPDATE_TRANSFERS);
	public static final BoincOp UpdateMessages = new BoincOp(OP_UPDATE_MESSAGES);
	public static final BoincOp UpdateNotices = new BoincOp(OP_UPDATE_NOTICES);
	
	public static final BoincOp GetBAMInfo = new BoincOp(OP_GET_BAM_INFO);
	public static final BoincOp SyncWithBAM = new BoincOp(OP_BAM_SYNCHRONIZE);
	public static final BoincOp StopUsingBAM = new BoincOp(OP_STOP_USING_BAM);
	
	public static final BoincOp GetAllProjectList = new BoincOp(OP_GET_ALL_PROJECT_LIST);
	
	public static final BoincOp AddProject = new BoincOp(OP_ADD_PROJECT);
	
	public static final BoincOp GetProjectConfig = new BoincOp(OP_GET_PROJECT_CONFIG);
	
	public static final BoincOp GlobalPrefsWorking = new BoincOp(OP_GLOBAL_PREFS_WORKING);
	public static final BoincOp GlobalPrefsOverride = new BoincOp(OP_GLOBAL_PREFS_OVERRIDE);
	
	public static final BoincOp RunBenchmarks = new BoincOp(OP_RUN_BENCHMARKS);
	public static final BoincOp SetRunMode = new BoincOp(OP_SET_RUN_MODE);
	public static final BoincOp SetNetworkMode = new BoincOp(OP_SET_NETWORK_MODE);
	public static final BoincOp SetGpuMode = new BoincOp(OP_SET_GPU_MODE);
	public static final BoincOp ShutdownCore = new BoincOp(OP_SHUTDOWN_CORE);
	public static final BoincOp DoNetworkComm = new BoincOp(OP_DO_NETWORK_COMM);
	
	public static final BoincOp ProjectOperation = new BoincOp(OP_PROJECT_OP);
	public static final BoincOp TaskOperation = new BoincOp(OP_TASK_OP);
	public static final BoincOp TransferOperation = new BoincOp(OP_TRANSFER_OP);
	
	public static final BoincOp SetProxySettings = new BoincOp(OP_SET_PROXY_SETTINGS);
	public static final BoincOp GetProxySettings = new BoincOp(OP_GET_PROXY_SETTINGS);
	
	public final int opCode;
	
	protected BoincOp(int opCode) {
		this.opCode = opCode;
	}
	
	public boolean isPollOp() {
		return (opCode == OP_BAM_SYNCHRONIZE || opCode == OP_ATTACH_PROJECT ||
				opCode == OP_GET_PROJECT_CONFIG);
	}
	
	public boolean isAutoRefreshOp(int refreshType) {
		switch(refreshType) {
		case AutoRefresh.CLIENT_MODE:
			return opCode == OP_UPDATE_CLIENT_MODE;
		case AutoRefresh.PROJECTS:
			return opCode == OP_UPDATE_PROJECTS;
		case AutoRefresh.TASKS:
			return opCode == OP_UPDATE_TASKS;
		case AutoRefresh.TRANSFERS:
			return opCode == OP_UPDATE_TRANSFERS;
		case AutoRefresh.MESSAGES:
			return opCode == OP_UPDATE_MESSAGES;
		case AutoRefresh.NOTICES:
			return opCode == OP_UPDATE_NOTICES;
		}
		return false;
	}
	
	public boolean isBAMOperation() {
		return (opCode == OP_BAM_SYNCHRONIZE);
	}
	
	public boolean isAddProject() {
		return (opCode == OP_ADD_PROJECT);
	}
	
	public boolean isCreateAccount() {
		return (opCode == OP_CREATE_ACCOUNT);
	}
	
	public boolean isLookupAccount() {
		return (opCode == OP_LOOKUP_ACCOUNT);
	}
	
	public boolean isProjectAttach() {
		return (opCode == OP_ATTACH_PROJECT);
	}
	
	public boolean isProjectConfig() {
		return (opCode == OP_GET_PROJECT_CONFIG);
	}
	
	@Override
	public boolean equals(Object ob) {
		if (this == null)
			return false;
		if (this == ob)
			return true;
		
		if (ob instanceof BoincOp) {
			BoincOp boincOp = (BoincOp)ob;
			return opCode == boincOp.opCode;
		}
		return false;
	}
	
	@Override
	public int hashCode() {
		return opCode;
	}
	
	@Override
	public String toString() {
		return String.valueOf(opCode);
	}
}
