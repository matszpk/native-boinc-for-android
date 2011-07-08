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

import sk.boinc.androboinc.util.ClientId;


public interface ClientRequestHandler {
	public abstract void registerStatusObserver(ClientReplyReceiver observer);
	public abstract void unregisterStatusObserver(ClientReplyReceiver observer);
	public abstract void connect(ClientId host, boolean retrieveInitialData) throws NoConnectivityException;
	public abstract void disconnect();

	public abstract ClientId getClientId();

	public abstract void updateClientMode(ClientReplyReceiver callback);
	public abstract void updateHostInfo(ClientReplyReceiver callback);
	public abstract void updateProjects(ClientReplyReceiver callback);
	public abstract void updateTasks(ClientReplyReceiver callback);
	public abstract void updateTransfers(ClientReplyReceiver callback);
	public abstract void updateMessages(ClientReplyReceiver callback);
	public abstract void cancelScheduledUpdates(ClientReplyReceiver callback);

	public abstract void runBenchmarks();
	public abstract void setRunMode(ClientReplyReceiver callback, int mode);
	public abstract void setNetworkMode(ClientReplyReceiver callback, int mode);
	public abstract void shutdownCore();
	public abstract void doNetworkCommunication();
	public abstract void projectOperation(ClientReplyReceiver callback, int operation, String projectUrl);
	public abstract void taskOperation(ClientReplyReceiver callback, int operation, String projectUrl, String taskName);
	public abstract void transferOperation(ClientReplyReceiver callback, int operation, String projectUrl, String fileName);
}
