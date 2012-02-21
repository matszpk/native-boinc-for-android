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

import java.util.ArrayList;

public class CcState {
	public VersionInfo version_info;
	public HostInfo host_info;
	public ArrayList<Project> projects = new ArrayList<Project>();
	public ArrayList<App> apps = new ArrayList<App>();
	public ArrayList<AppVersion> app_versions = new ArrayList<AppVersion>();
	public ArrayList<Workunit> workunits = new ArrayList<Workunit>();
	public ArrayList<Result> results = new ArrayList<Result>();
	public boolean have_ati;
	public boolean have_cuda;
}
