/**
 * 
 */
package sk.boinc.nativeboinc.installer;

/**
 * @author mat
 *
 */
public class ProjectDescriptor {
	public String projectName = null;
	public String masterUrl = null;
	
	public ProjectDescriptor() { }
	
	public ProjectDescriptor(String name, String url) {
		this.projectName = name;
		this.masterUrl = url;
	}
}
