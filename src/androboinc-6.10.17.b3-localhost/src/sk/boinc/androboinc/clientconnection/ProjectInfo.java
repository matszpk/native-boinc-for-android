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

package sk.boinc.androboinc.clientconnection;


/**
 * Description of BOINC project for AndroBOINC purpose
 * Reflects the class Project of BOINC-library
 */
public class ProjectInfo {
	public String masterUrl;    // Project.master_url - Master URL of the project - unique ID
	public int    statusId;     // Status in numerical form
	// integer representation: 0 = active, 1 = suspended, 2 = no new work, 3 = suspended & no new work
	public static final int SUSPENDED = 1;  // bit 1 of statusId
	public static final int NNW = 2;        // bit 2 of statusId
	public int    resShare;     // Resources share in numerical form

	public String project;      // Project.getName()
	public String account;      // Project.user_name
	public String team;         // Project.team_name
	public double user_credit;  // Project.user_total_credit
	public double user_rac;     // Project.user_expavg_credit
	public double host_credit;  // Project.host_total_credit
	public double host_rac;     // Project.host_expavg_credit
	public String share;        // Project.resource_share converted to percentage
	public String status;       // Combined Project.suspended_via_gui & Project.dont_request_more_work converted to string
}
