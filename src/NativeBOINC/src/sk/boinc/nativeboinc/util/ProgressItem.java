/**
 * 
 */
package sk.boinc.nativeboinc.util;


/**
 * @author mat
 *
 */
public class ProgressItem {
	public static final int STATE_IN_PROGRESS = 0;
	public static final int STATE_CANCELLED = 1;
	public static final int STATE_FINISHED = 2;
	public static final int STATE_ERROR_OCCURRED = 3;
	
	public String name = "";
	public String projectUrl = "";
	public String opDesc = "";
	public int progress = -1;
	public int state = STATE_IN_PROGRESS;
	
	public ProgressItem(String name, String projectUrl, String opDesc, int progress, int state) {
		this.name = name;
		this.projectUrl = projectUrl;
		this.opDesc = opDesc;
		this.progress = progress;
		this.state = state;
	}
}
