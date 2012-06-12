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

	public abstract boolean isWorking();
	
	public abstract int getAutoRefresh();
	
	/*
	 * some methods retuns boolean value:
	 * 		returns true if was ran, otherwise returns false
	 */
	
	/* this calls should enqueued only once, because read state of the client */
	public abstract boolean updateClientMode();
	public abstract boolean updateHostInfo();
	
	public abstract boolean updateProjects();
	public abstract boolean updateTasks();
	public abstract boolean updateTransfers();
	public abstract boolean updateMessages();
	public abstract boolean updateNotices();
	
	public abstract void addToScheduledUpdates(ClientReceiver callback, int refreshType, int period); 
	public abstract void cancelScheduledUpdates(int refreshType);

	public abstract boolean handlePendingClientErrors(BoincOp boincOp, ClientReceiver receiver);
	public abstract boolean handlePendingPollErrors(BoincOp boincOp, ClientPollReceiver receiver);
	public abstract Object getPendingOutput(BoincOp boincOp);
	public abstract boolean isOpBeingExecuted(BoincOp boincOp);
	
	public abstract boolean isNativeConnected();
	
	/* this calls should enqueued only once, because read state of the client */
	public abstract boolean getBAMInfo();
	
	/* this tasks should be enqueued only once at this same time, because
	 * can be performed only once at this same time */
	public abstract boolean attachToBAM(String name, String url, String password);
	public abstract boolean synchronizeWithBAM();
	
	/* this tasks should be enqueued always because changes state of the client */
	public abstract boolean stopUsingBAM();
	
	/* this calls should enqueued only once, because read state of the client */
	public abstract boolean getAllProjectsList(boolean excludeAttachedProjects);
	
	/* this tasks should be enqueued only once at this same time, because
	 * can be performed only once at this same time */
	public abstract boolean lookupAccount(AccountIn accountIn);
	public abstract boolean createAccount(AccountIn accountIn);
	public abstract boolean projectAttach(String url, String authCode, String projectName);
	public abstract boolean getProjectConfig(String url);
	public abstract boolean addProject(AccountIn accountIn, boolean create);
	public abstract void cancelPollOperations(int opFlags);
	
	/* this calls should enqueued only once, because read state of the client */
	public abstract boolean getGlobalPrefsWorking();
	/* this tasks should be enqueued always because changes state of the client */
	public abstract boolean setGlobalPrefsOverride(String data);
	public abstract boolean setGlobalPrefsOverrideStruct(GlobalPreferences globalPrefs);
	
	/* this tasks should be enqueued always because changes state of the client */
	public abstract boolean runBenchmarks();
	public abstract boolean setRunMode(int mode);
	public abstract boolean setNetworkMode(int mode);
	public abstract boolean doNetworkCommunication();
	
	public abstract void shutdownCore();
	
	/* this tasks should be enqueued always because changes state of the client */
	public abstract boolean projectOperation(int operation, String projectUrl);
	public abstract boolean projectsOperation(int operation, String[] projectUrls);
	public abstract boolean taskOperation(int operation, String projectUrl, String taskName);
	public abstract boolean tasksOperation(int operation, TaskDescriptor[] tasks);
	public abstract boolean transferOperation(int operation, String projectUrl, String fileName);
	public abstract boolean transfersOperation(int operation, TransferDescriptor[] transfers);
}
