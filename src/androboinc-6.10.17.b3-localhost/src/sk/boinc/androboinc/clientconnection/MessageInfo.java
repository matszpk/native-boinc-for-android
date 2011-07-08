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
 * Description of BOINC-client message for AndroBOINC purpose
 * Reflects the class Message of BOINC-library (and also class Project)
 */
public class MessageInfo {
	public int    seqNo = 0;     // Message.seqno - sequence number of message - unique ID
	public int    priority;      // Message.priority
	public String project;       // Project.getName()
	public String time;          // Message.timestamp converted to date-String
	public String body;          // Message.body
}
