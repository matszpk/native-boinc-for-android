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

package edu.berkeley.boinc.lite;

import java.util.ArrayList;

/**
 * @author mat
 *
 */
public class ProjectConfig {
	public int error_num = 0;
	public String error_msg = "";
	
	public String name = "";
	public String master_url = "";
	public int local_revision = 0;
	public int min_passwd_length = 0;
	public boolean account_manager = false;
	public boolean use_username = false;
	public boolean account_creation_disabled = false;
	public boolean client_account_creation_disabled = false;
	public boolean sched_stopped = false;
	public boolean web_stopped = false;
	public int min_client_version = 0;
	public String terms_of_use = "";
	public ArrayList<String> platforms = new ArrayList<String>();
}
