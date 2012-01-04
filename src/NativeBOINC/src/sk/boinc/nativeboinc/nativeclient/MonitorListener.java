/**
 * 
 */
package sk.boinc.nativeboinc.nativeclient;

import edu.berkeley.boinc.nativeboinc.ClientEvent;

/**
 * @author mat
 *
 */
public interface MonitorListener {
	public void onMonitorEvent(ClientEvent event);
}
