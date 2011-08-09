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

import java.util.Vector;

/**
 * @author mat
 *
 */
public interface InstallerListener {
	public final static int FINISH_PROGRESS = 10000;
	
	public abstract void onOperation(String opDescription);
	public abstract void onOperationProgress(String opDescription, int progress);
	public abstract void onOperationError(String errorMessage);
	public abstract void onOperationCancel();
	public abstract void onOperationFinish();
	
	public abstract void currentProjectDistribs(Vector<ProjectDistrib> projectDistribs);
}
