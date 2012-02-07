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

package sk.boinc.nativeboinc.clientconnection;

import edu.berkeley.boinc.lite.AccountIn;
import edu.berkeley.boinc.lite.GlobalPreferences;
import sk.boinc.nativeboinc.util.ClientId;


public interface ClientRequestHandler {
	public abstract void registerStatusObserver(ClientReceiver observer);
	public abstract void unregisterStatusObserver(ClientReceiver observer);
	public abstract void connect(ClientId host, boolean retrieveInitialData) throws NoConnectivityException;
	public abstract void disconnect();

	public abstract ClientId getClientId();

	public abstract void updateClientMode(ClientReceiver callback);
	public abstract void updateHostInfo(ClientReplyReceiver callback);
	public abstract void updateProjects(ClientReplyReceiver callback);
	public abstract void updateTasks(ClientReplyReceiver callback);
	public abstract void updateTransfers(ClientReplyReceiver callback);
	public abstract void updateMessages(ClientReplyReceiver callback);
	public abstract void cancelScheduledUpdates(ClientReplyReceiver callback);

	public abstract void getBAMInfo(ClientAccountMgrReceiver callback);
	public abstract void attachToBAM(ClientAccountMgrReceiver callback, String name, String url, String password);
	public abstract void synchronizeWithBAM(ClientAccountMgrReceiver callback);
	public abstract void stopUsingBAM(ClientReplyReceiver callback);
	
	public abstract void getAllProjectsList(ClientAllProjectsListReceiver callback);
	
	public abstract void lookupAccount(AccountIn accountIn);
	public abstract void createAccount(AccountIn accountIn);
	public abstract void projectAttach(String url, String authCode, String projectName);
	public abstract void getProjectConfig(String url);
	public abstract void addProject(AccountIn accountIn, boolean create);
	
	public abstract void getGlobalPrefsWorking(ClientPreferencesReceiver callback);
	public abstract void setGlobalPrefsOverrideStruct(ClientPreferencesReceiver callback, GlobalPreferences globalPrefs);
	public abstract void setGlobalPrefsOverride(ClientPreferencesReceiver callback, String globalPrefs);
	
	public abstract void runBenchmarks();
	public abstract void setRunMode(ClientReplyReceiver callback, int mode);
	public abstract void setNetworkMode(ClientReplyReceiver callback, int mode);
	public abstract void shutdownCore();
	public abstract void doNetworkCommunication();
	public abstract void projectOperation(ClientReplyReceiver callback, int operation, String projectUrl);
	public abstract void taskOperation(ClientReplyReceiver callback, int operation, String projectUrl, String taskName);
	public abstract void transferOperation(ClientReplyReceiver callback, int operation, String projectUrl, String fileName);
}
