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

package edu.berkeley.boinc.lite;

import java.util.Vector;

public class CcState {
	public VersionInfo version_info;
	public HostInfo host_info;
	public Vector<Project> projects = new Vector<Project>();
	public Vector<App> apps = new Vector<App>();
//	public Vector<AppVersion> app_versions = new Vector<AppVersion>();
	public Vector<Workunit> workunits = new Vector<Workunit>();
	public Vector<Result> results = new Vector<Result>();
}
