/**
 * 
 */
package edu.berkeley.boinc.lite;

/**
 * @author mat
 *
 */
public class TimePreferences {
	public double start_hour, end_hour;
	
	private static final String hourToString(double value) {
		int hour = (int)Math.floor(value);
		int minute = (int)Math.round((value-(double)hour)*60.0);
		minute = Math.min(59, minute);
		return String.format("%02d:%02d",hour, minute);
	}
	
	public static final class TimeSpan {
		public double start_hour;
		public double end_hour;
		
		public String startHourString() {
			return hourToString(start_hour);
		}
		
		public String endHourString() {
			return hourToString(end_hour);
		}
	};
	
	public TimeSpan[] week_prefs  = new TimeSpan[7];
	
	public String startHourString() {
		return hourToString(start_hour);
	}
	
	public String endHourString() {
		return hourToString(end_hour);
	}
}
