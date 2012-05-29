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

import java.util.ArrayList;

import sk.boinc.nativeboinc.util.UpdateItem;

/**
 * @author mat
 *
 */
public interface InstallerUpdateListener extends AbstractInstallerListener {
	public abstract void currentProjectDistribList(ArrayList<ProjectDistrib> projectDistribs);
	public abstract void currentClientDistrib(ClientDistrib clientDistrib);
	public abstract void binariesToUpdateOrInstall(UpdateItem[] updateItems);
	public abstract void binariesToUpdateFromSDCard(String[] projectNames);
}
