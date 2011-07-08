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

import java.util.Vector;


public interface ClientReplyReceiver {
	public static final int PROGRESS_CONNECTING = 0;
	public static final int PROGRESS_AUTHORIZATION_PENDING = 1;
	public static final int PROGRESS_INITIAL_DATA = 2;
	public static final int PROGRESS_XFER_STARTED = 3;
	public static final int PROGRESS_XFER_FINISHED = 4;

	public abstract void clientConnectionProgress(int progress);
	public abstract void clientConnected(VersionInfo clientVersion);
	public abstract void clientDisconnected();

	public abstract boolean updatedClientMode(ModeInfo modeInfo);
	public abstract boolean updatedHostInfo(HostInfo hostInfo);

	public abstract boolean updatedProjects(Vector<ProjectInfo> projects);
	public abstract boolean updatedTasks(Vector<TaskInfo> tasks);
	public abstract boolean updatedTransfers(Vector<TransferInfo> transfers);
	public abstract boolean updatedMessages(Vector<MessageInfo> messages);
}
