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
	public abstract void onMonitorEvent(ClientEvent event);
	public abstract void onMonitorDoesntWork();
}
