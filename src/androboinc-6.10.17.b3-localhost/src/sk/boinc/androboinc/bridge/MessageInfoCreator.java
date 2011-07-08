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

package sk.boinc.androboinc.bridge;

import sk.boinc.androboinc.clientconnection.MessageInfo;
import edu.berkeley.boinc.lite.Message;


public class MessageInfoCreator {
	public static MessageInfo create(final Message msg, final Formatter formatter) {
		MessageInfo mi = new MessageInfo();
		mi.seqNo = msg.seqno;
		mi.project = msg.project;
		if (mi.project.length() == 0) {
			mi.project = "---";
		}
		mi.time = formatter.formatDate(msg.timestamp);
		mi.body = msg.body;
		mi.priority = msg.priority;
		return mi;
	}
}
