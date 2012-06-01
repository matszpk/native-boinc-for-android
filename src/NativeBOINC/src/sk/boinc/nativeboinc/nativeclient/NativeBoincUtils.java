/**
 * 
 */
package sk.boinc.nativeboinc.nativeclient;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.List;

import android.content.Context;

/**
 * @author mat
 *
 */
public class NativeBoincUtils {

	public static final String INITIAL_BOINC_CONFIG = "<global_preferences>\n" +
			"<run_on_batteries>1</run_on_batteries>\n" +
			"<run_if_user_active>1</run_if_user_active>\n" +
			"<run_gpu_if_user_active>0</run_gpu_if_user_active>\n" +
			"<idle_time_to_run>30.000000</idle_time_to_run>\n" +
			"<suspend_cpu_usage>0.000000</suspend_cpu_usage>\n" +
			"<start_hour>0.000000</start_hour>\n" +
			"<end_hour>0.000000</end_hour>\n" +
			"<net_start_hour>0.000000</net_start_hour>\n" +
			"<net_end_hour>0.000000</net_end_hour>\n" +
			"<leave_apps_in_memory>0</leave_apps_in_memory>\n" +
			"<confirm_before_connecting>1</confirm_before_connecting>\n" +
			"<hangup_if_dialed>0</hangup_if_dialed>\n" +
			"<dont_verify_images>0</dont_verify_images>\n" +
			"<work_buf_min_days>0.100000</work_buf_min_days>\n" +
			"<work_buf_additional_days>0.250000</work_buf_additional_days>\n" +
			"<max_ncpus_pct>100.000000</max_ncpus_pct>\n" +
			"<cpu_scheduling_period_minutes>60.000000</cpu_scheduling_period_minutes>\n" +
			"<disk_interval>60.000000</disk_interval>\n" +
			"<disk_max_used_gb>10.000000</disk_max_used_gb>\n" +
			"<disk_max_used_pct>100.000000</disk_max_used_pct>\n" +
			"<disk_min_free_gb>0.000000</disk_min_free_gb>\n" +
			"<vm_max_used_pct>100.000000</vm_max_used_pct>\n" +
			"<ram_max_used_busy_pct>50.000000</ram_max_used_busy_pct>\n" +
			"<ram_max_used_idle_pct>90.000000</ram_max_used_idle_pct>\n" +
			"<max_bytes_sec_up>0.000000</max_bytes_sec_up>\n" +
			"<max_bytes_sec_down>0.000000</max_bytes_sec_down>\n" +
			"<cpu_usage_limit>100.000000</cpu_usage_limit>\n" +
			"<daily_xfer_limit_mb>0.000000</daily_xfer_limit_mb>\n" +
			"<daily_xfer_period_days>0</daily_xfer_period_days>\n" +
			"<run_if_battery_nl_than>10</run_if_battery_nl_than>\n" +
			"<run_if_temp_lt_than>100</run_if_temp_lt_than>\n" +
			"</global_preferences>";
	
	/* kill processes routines */
	private static void killBoincProcesses(List<String> pidStrings) {
		for (String pid: pidStrings) {
			android.os.Process.sendSignal(Integer.parseInt(pid), 2);
			try {
				Thread.sleep(100);	// 0.4 second
			} catch (InterruptedException e) { }
			android.os.Process.sendSignal(Integer.parseInt(pid), 2);
			try {
				Thread.sleep(100);	// 0.4 second
			} catch (InterruptedException e) { }
			android.os.Process.sendSignal(Integer.parseInt(pid), 2);
			try {
				Thread.sleep(400);	// 0.4 second
			} catch (InterruptedException e) { }
			/* fallback killing (by using SIGKILL signal) */
			android.os.Process.sendSignal(Integer.parseInt(pid), 9);
		}
	}
	
	private static void killProcesses(List<String> pidStrings) {
		for (String pid: pidStrings) {
			android.os.Process.sendSignal(Integer.parseInt(pid), 2);
			try {
				Thread.sleep(400);	// 0.4 second
			} catch (InterruptedException e) { }
			/* fallback killing (by using SIGKILL signal) */
			android.os.Process.sendSignal(Integer.parseInt(pid), 9);
		}
	}
	
	public static void killAllNativeBoincs(Context context) {
		String nativeBoincPath = context.getFileStreamPath("boinc_client").getAbsolutePath();
		File procDir = new File("/proc/");
		/* scan /proc directory */
		List<String> pids = new ArrayList<String>();
		BufferedReader cmdLineReader = null;
		
		for (File dir: procDir.listFiles()) {
			String dirName = dir.getName();
			if (!dir.isDirectory())
				continue;
			/* if process directory */
			boolean isNumber = true;
			for (int i = 0; i < dirName.length(); i++)
				if (!Character.isDigit(dirName.charAt(i))) {
					isNumber = false;
					break;
				}
			if (!isNumber)
				continue;
			
			try {
				cmdLineReader = new BufferedReader(new FileReader(
						new File(dir.getAbsolutePath()+"/cmdline")));
				
				String cmdLine = cmdLineReader.readLine();
				if (cmdLine != null && cmdLine.startsWith(nativeBoincPath)) /* is found */
					pids.add(dirName);				
			} catch (IOException ex) {
				continue;
			} finally {
				try {
					cmdLineReader.close();
				} catch(IOException ex) { }
			}
		}
		
		/* do kill processes */
		killBoincProcesses(pids);
	}
	
	public static void killAllBoincZombies() {
		File procDir = new File("/proc/");
		List<String> pids = new ArrayList<String>();
		BufferedReader cmdLineReader = null;
		
		String uid = Integer.toString(android.os.Process.myUid());
		String myPid = Integer.toString(android.os.Process.myPid());
		
		for (File dir: procDir.listFiles()) {
			String dirName = dir.getName();
			
			if (!dir.isDirectory())
				continue;
			/* if process directory */
			boolean isNumber = true;
			for (int i = 0; i < dirName.length(); i++)
				if (!Character.isDigit(dirName.charAt(i))) {
					isNumber = false;
					break;
				}
			if (!isNumber)
				continue;
			
			if (dirName.equals(myPid)) // if my pid
				continue;
			
			try {
				cmdLineReader = new BufferedReader(new FileReader(
						new File(dir.getAbsolutePath()+"/status")));
				
				while (true) {
					String line = cmdLineReader.readLine();
					if (line == null)
						break;
					
					if (line.startsWith("Uid:")) {
						// extract first number
						int i = 4;
						int start = 0, end = 4;
						int length = line.length();
						for (i = 4; i < length; i++)
							if (Character.isDigit(line.charAt(i)))	// skip all spaces
								break;
						// if digit
						if (Character.isDigit(line.charAt(i))) {
							start = i;
							for (; i < length; i++)
								if (!Character.isDigit(line.charAt(i)))
									break;
							end = i;
						}
						String processUid = line.substring(start, end);
						if (uid.equals(processUid)) { // if uid matches
							pids.add(dirName);
						}
						break;
					}
				}
			} catch(IOException ex) {
				continue;
			} finally {
				try {
					if (cmdLineReader != null)
						cmdLineReader.close();
				} catch(IOException ex) { }
			}
		}
		
		// kill processes
		killProcesses(pids);
	}
	
	/* fallback kill zombies */
	public static void killZombieClient(Context context) {
		killAllNativeBoincs(context);
		killAllBoincZombies();
	}
	
	/**
	 * reads access password (from gui_rpc_auth.cfg)
	 * @return access password
	 * @throws IOException
	 */
	public static String getAccessPassword(Context context) throws IOException {
		BufferedReader inReader = null;
		try {
			inReader = new BufferedReader(new FileReader(
					context.getFilesDir().getAbsolutePath()+"/boinc/gui_rpc_auth.cfg"));
			String output = inReader.readLine();
			if (output == null)
				throw new IOException("Empty file!");
			return output;
		} finally {
			if (inReader != null)
				inReader.close();
		}
	}
	
	public static void setAccessPassword(Context context,String password) throws IOException {
		OutputStreamWriter writer = null;
		try {
			writer = new FileWriter(context.getFilesDir().getAbsolutePath()+"/boinc/gui_rpc_auth.cfg");
			writer.write(password);
		} finally {
			if (writer != null)
				writer.close();
		}
	}
	
	/**
	 * set host name
	 */
	public static String getHostname(Context context) throws IOException {
		BufferedReader inReader = null;
		try {
			inReader = new BufferedReader(new FileReader(
					context.getFilesDir().getAbsolutePath()+"/boinc/hostname.cfg"));
			return inReader.readLine();
		} finally {
			if (inReader != null)
				inReader.close();
		}
	}
	
	public static void setHostname(Context context, String hostname) throws IOException {
		if (hostname == null || hostname.length() == 0) {
			File fileToRemove = new File(context.getFilesDir().getAbsolutePath()+
					"/boinc/hostname.cfg");
			fileToRemove.delete();
		}
		
		OutputStreamWriter writer = null;
		try {
			writer = new FileWriter(context.getFilesDir().getAbsolutePath()+"/boinc/hostname.cfg");
			writer.write(hostname);
		} finally {
			if (writer != null)
				writer.close();
		}
	}
	
	/**
	 * remote host list utilities
	 */
	public static ArrayList<String> getRemoteHostList(Context context) throws IOException {
		BufferedReader inReader = null;
		ArrayList<String> list = null;
		try {
			File remoteHostsFile = new File(context.getFilesDir().getAbsolutePath()+
					"/boinc/remote_hosts.cfg");
			
			/* return null if not exists (if anything host can connect with client) */
			if (!remoteHostsFile.exists())
				return null;
				
			inReader = new BufferedReader(new FileReader(
					context.getFilesDir().getAbsolutePath()+"/boinc/remote_hosts.cfg"));
			
			list = new ArrayList<String>(1);
			
			while (true) {
				String host = inReader.readLine();
				if (host == null)
					break;
				host = host.trim();
				if (host.length() == 0 || host.charAt(0) == '#' || host.charAt(0) == ';')
					continue;	// if comment (skip them)
				list.add(host);
			}
		} finally {
			if (inReader != null)
				inReader.close();
		}
		return list;
	}
	
	public static void setRemoteHostList(Context context, ArrayList<String> outList) throws IOException {
		FileWriter writer = null;
		try {
			writer = new FileWriter(context.getFilesDir().getAbsolutePath()+"/boinc/remote_hosts.cfg");
			
			StringBuilder sB = new StringBuilder();
			
			for (String host: outList) {
				sB.append(host);
				sB.append('\n');
			}
			
			// write to file
			writer.write(sB.toString());
		} finally {
			if (writer != null)
				writer.close();
		}
	}
}
