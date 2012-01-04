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

package edu.berkeley.boinc.nativeboinc;

import java.io.IOException;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;
import edu.berkeley.boinc.lite.RpcClient;
import edu.berkeley.boinc.lite.SimpleReplyParser;

/**
 * @author mat
 *
 */
public class ExtendedRpcClient extends RpcClient {
	private static final String TAG = "ExtendedRpcClient";
	
	/**
	 * apply new app info
	 * @param projectUrl - project url
	 * @return true for success, false for otherwise
	 */
	public boolean updateProjectApps(String projectUrl) {
		try {
			mRequest.setLength(0);
			mRequest.append("<update_project_apps>\n  <project_url>");
			mRequest.append(projectUrl);
			mRequest.append("</project_url>\n</update_project_apps>\n");
			sendRequest(mRequest.toString());
			return SimpleReplyParser.isSuccess(receiveReply());
		} catch (IOException e) {
			if (Logging.WARNING) Log.w(TAG, "error in updateProjectApps()", e);
			return false;
		}
	}
	
	/**
	 * apply new app info poll
	 * @return reply
	 */
	public UpdateProjectAppsReply updateProjectAppsPoll(String projectUrl) {
		try {
			mRequest.setLength(0);
			mRequest.append("<update_project_apps_poll>\n  <project_url>");
			mRequest.append("</project_url>\n</update_project_apps_poll>\n");
			sendRequest(mRequest.toString());
			return UpdateProjectAppsReplyParser.parse(receiveReply());
		} catch (IOException e) {
			if (Logging.WARNING) Log.w(TAG, "error in error in updateProjectAppsPoll()", e);
			return null;
		}
	}
	
	/**
	 * get authorization code for monitor
	 * @return authentication code if success, otherwise null
	 */
	public String authorizeMonitor() {
		try {
			sendRequest("<auth_monitor/>\n");
			return StringReplyParser.parse(receiveReply());
		}
		catch (IOException e) {
			if (Logging.WARNING) Log.w(TAG, "error in authenticateMonitor()", e);
			return null;
		}
	}
}
