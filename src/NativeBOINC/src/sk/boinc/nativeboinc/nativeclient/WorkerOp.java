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

package sk.boinc.nativeboinc.nativeclient;

/**
 * @author mat
 *
 */
public class WorkerOp {
	public final static int OP_GET_GLOBAL_PROGRESS = 1;
	public final static int OP_GET_TASKS = 2;
	public final static int OP_GET_PROJECTS = 3;
	public final static int OP_UPDATE_PROJECT_APPS = 4;
	public final static int OP_DO_NET_COMM = 5;
	
	public final static WorkerOp GetGlobalProgress = new WorkerOp(OP_GET_GLOBAL_PROGRESS);
	public final static WorkerOp GetTasks = new WorkerOp(OP_GET_TASKS);
	public final static WorkerOp GetProjects = new WorkerOp(OP_GET_PROJECTS);
	public final static WorkerOp DoNetComm = new WorkerOp(OP_DO_NET_COMM);
	
	public final static WorkerOp UpdateProjectApps(String projectUrl) {
		return new WorkerOp(OP_UPDATE_PROJECT_APPS, projectUrl);
	}
	
	public int opCode;
	public String projectUrl;
	
	protected WorkerOp(int opCode) {
		this.opCode = opCode;
		this.projectUrl = null;
	}
	
	protected WorkerOp(int opCode, String projectUrl) {
		this.opCode = opCode;
		this.projectUrl = projectUrl;
	}
	
	@Override
	public boolean equals(Object ob) {
		if (this == null)
			return false;
		if (this == ob)
			return true;
		
		if (ob instanceof WorkerOp) {
			WorkerOp op = (WorkerOp)ob;
			if (this.opCode != op.opCode)
				return false;
			
			if (opCode == OP_UPDATE_PROJECT_APPS) {
				if (projectUrl != null)
					return projectUrl.equals(op.projectUrl);
				else
					return op.projectUrl == null;
			} else
				return true;
		}
		return false;
	}
	
	@Override
	public int hashCode() {
		if (opCode == OP_UPDATE_PROJECT_APPS && projectUrl != null)
			return opCode ^ projectUrl.hashCode();
		return opCode;
	}
	
	@Override
	public String toString() {
		return "["+opCode+","+projectUrl+"]";
	}
}
