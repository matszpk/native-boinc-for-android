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
public class TransferDescriptor {
	public String projectUrl;
	public String fileName;
	
	public TransferDescriptor(String projectUrl, String fileName) {
		this.projectUrl = projectUrl;
		this.fileName = fileName;
	}
	
	@Override
	public String toString() {
		return "Transfer:"+projectUrl+":"+fileName;
	}
	
	@Override
	public boolean equals(Object object) {
		if (this == object)
			return true;
		if (object == null)
			return false;
		if (object instanceof TransferDescriptor) {
			TransferDescriptor transferDesc = (TransferDescriptor)object;
			return this.projectUrl.equals(transferDesc.projectUrl) &&
					this.fileName.equals(transferDesc.fileName);
		}
		return false;
	}
	
	@Override
	public int hashCode() {
		return projectUrl.hashCode() ^ fileName.hashCode();
	}
}
