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

import java.util.ArrayList;

import edu.berkeley.boinc.lite.AccountIn;
import edu.berkeley.boinc.lite.AccountMgrInfo;
import edu.berkeley.boinc.lite.GlobalPreferences;
import edu.berkeley.boinc.lite.ProjectConfig;
import edu.berkeley.boinc.lite.ProjectListEntry;
import sk.boinc.nativeboinc.util.ClientId;


public interface ClientRequestHandler {
	public abstract void registerStatusObserver(ClientReceiver observer);
	public abstract void unregisterStatusObserver(ClientReceiver observer);
	public abstract void connect(ClientId host, boolean retrieveInitialData) throws NoConnectivityException;
	public abstract void disconnect();

	public abstract ClientId getClientId();

	public abstract boolean isWorking();
	
	public abstract void updateClientMode(ClientReceiver callback);
	public abstract void updateHostInfo();
	public abstract HostInfo getPendingHostInfo();
	
	public abstract void updateProjects();
	public abstract ArrayList<ProjectInfo> getPendingProjects();
	
	public abstract void updateTasks();
	public abstract ArrayList<TaskInfo> getPendingTasks();
	
	public abstract void updateTransfers();
	public abstract ArrayList<TransferInfo> getPendingTransfers();
	
	public abstract void updateMessages();
	public abstract ArrayList<MessageInfo> getPendingMessages();
	
	public abstract void addToScheduledUpdates(ClientReplyReceiver callback, int refreshType); 
	public abstract void cancelScheduledUpdates(int refreshType);

	public abstract ClientError getPendingClientError();
	public abstract PollError getPendingPollError(String projectUrl);
	
	public abstract boolean isNativeConnected();
	
	public abstract void getBAMInfo();
	public abstract AccountMgrInfo getPendingBAMInfo();
	public abstract void attachToBAM(String name, String url, String password);
	public abstract void synchronizeWithBAM();
	public abstract void stopUsingBAM();
	public abstract boolean isBAMBeingSynchronized();
	
	public abstract void getAllProjectsList();
	public abstract ArrayList<ProjectListEntry> getPendingAllProjectsList();
	
	public abstract void lookupAccount(AccountIn accountIn);
	public abstract void createAccount(AccountIn accountIn);
	public abstract void projectAttach(String url, String authCode, String projectName);
	public abstract void getProjectConfig(String url);
	public abstract ProjectConfig getPendingProjectConfig(String projectUrl);
	public abstract void addProject(AccountIn accountIn, boolean create);
	public abstract boolean isProjectBeingAdded(String projectUrl);
	public abstract void cancelPollOperations();
	
	public abstract void getGlobalPrefsWorking();
	public abstract GlobalPreferences getPendingGlobalPrefsWorking();
	public abstract void setGlobalPrefsOverrideStruct(GlobalPreferences globalPrefs);
	public abstract void setGlobalPrefsOverride(String globalPrefs);
	public abstract boolean isGlobalPrefsBeingOverriden();
	
	public abstract void runBenchmarks();
	public abstract void setRunMode(ClientReplyReceiver callback, int mode);
	public abstract void setNetworkMode(ClientReplyReceiver callback, int mode);
	public abstract void shutdownCore();
	public abstract void doNetworkCommunication();
	public abstract void projectOperation(ClientReplyReceiver callback, int operation, String projectUrl);
	public abstract void taskOperation(ClientReplyReceiver callback, int operation, String projectUrl, String taskName);
	public abstract void transferOperation(ClientReplyReceiver callback, int operation, String projectUrl, String fileName);
}
