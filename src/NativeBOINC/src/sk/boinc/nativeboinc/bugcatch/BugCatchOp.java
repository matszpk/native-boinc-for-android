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
package sk.boinc.nativeboinc.bugcatch;

/**
 * @author mat
 *
 */
public class BugCatchOp {
	public final int opCode;
	
	public static final int OP_SEND_BUGS = 0;
	public static final int OP_BUGS_TO_SDCARD = 0;
	
	public static final BugCatchOp SendBugs = new BugCatchOp(OP_SEND_BUGS);
	public static final BugCatchOp BugsToSDCard = new BugCatchOp(OP_BUGS_TO_SDCARD);
	
	protected BugCatchOp(int opCode) {
		this.opCode = opCode;
	}
	
	@Override
	public boolean equals(Object ob) {
		if (this == null)
			return false;
		if (this == ob)
			return true;
		
		if (ob instanceof BugCatchOp) {
			BugCatchOp op = (BugCatchOp)ob;
			if (this.opCode != op.opCode)
				return false;
		}
		return false;
	}
	
	@Override
	public int hashCode() {
		return opCode;
	}
	
	@Override
	public String toString() {
		return "["+opCode+"]";
	}
}
