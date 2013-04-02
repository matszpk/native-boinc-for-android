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

/**
 * @author mat
 *
 */
public class InstallOp {
	public static final int OP_PROGRESS_OP = 0;
	public static final int OP_UPDATE_CLIENT_DISTRIB = 1;
	public static final int OP_UPDATE_PROJECT_DISTRIBS = 2;
	public static final int OP_GET_BINARIES_TO_INSTALL = 3;
	public static final int OP_GET_BINARIES_FROM_SDCARD = 4;
	public static final int OP_DELETE_PROJECT_BINARIES = 5;
	
	public static final InstallOp ProgressOperation = new InstallOp(OP_PROGRESS_OP);
	public static final InstallOp UpdateClientDistrib = new InstallOp(OP_UPDATE_CLIENT_DISTRIB);
	public static final InstallOp UpdateProjectDistribs = new InstallOp(OP_UPDATE_PROJECT_DISTRIBS);
	public static final InstallOp GetBinariesToInstall = new InstallOp(OP_GET_BINARIES_TO_INSTALL);
	public static final InstallOp GetBinariesFromSDCard(String path) {
		// fix path
		if (path.charAt(0) == '/')
			path = path.substring(1);
		if (path.charAt(path.length()-1) == '/')
			path = path.substring(0, path.length()-1);
		return new InstallOp(OP_GET_BINARIES_FROM_SDCARD, path);
	}
	public static final InstallOp DeleteProjectBinaries = new InstallOp(OP_DELETE_PROJECT_BINARIES);
	
	public final int opCode;
	public final String path;
	
	protected InstallOp(int opCode) {
		this.opCode = opCode;
		path = null;
	}
	
	protected InstallOp(int opCode, String path) {
		this.opCode = opCode;
		this.path = path;
	}
	
	@Override
	public boolean equals(Object ob) {
		if (this == null)
			return false;
		if (this == ob)
			return true;
		
		if (ob instanceof InstallOp) {
			InstallOp op = (InstallOp)ob;
			if (this.opCode != op.opCode)
				return false;
			
			if (opCode == OP_GET_BINARIES_FROM_SDCARD) {
				if (path != null)
					return path.equals(op.path);
				else // if also null
					return op.path == null;
			} else
				return true;
		}
		return false;
	}
	
	@Override
	public int hashCode() {
		if (opCode == OP_GET_BINARIES_FROM_SDCARD && path != null)
			return opCode ^ path.hashCode();
		return opCode;
	}
	
	@Override
	public String toString() {
		return "["+opCode+","+path+"]";
	}
}
