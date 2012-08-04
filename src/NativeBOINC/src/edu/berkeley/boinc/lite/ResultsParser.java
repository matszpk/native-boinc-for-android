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

package edu.berkeley.boinc.lite;

import java.util.ArrayList;

import sk.boinc.nativeboinc.debug.Logging;

import android.util.Log;

public class ResultsParser extends BoincBaseParser {
	private static final String TAG = "ResultsParser";

	private ArrayList<Result> mResults = new ArrayList<Result>();
	private Result mResult = null;
	private boolean mInActiveTask = false;

	public ArrayList<Result> getResults() {
		return mResults;
	}

	/**
	 * Parse the RPC result (results) and generate vector of results info
	 * 
	 * @param rpcResult
	 *            String returned by RPC call of core client
	 * @return vector of results info
	 */
	public static ArrayList<Result> parse(String rpcResult) {
		try {
			ResultsParser parser = new ResultsParser();
			BoincBaseParser.parse(parser, rpcResult);
			return parser.getResults();
		} catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}

	}

	@Override
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("result")) {
			if (Logging.INFO) {
				if (mResult != null) {
					// previous <result> not closed - dropping it!
					Log.i(TAG, "Dropping unfinished <result> data");
				}
			}
			mResult = new Result();
		}
		else if (localName.equalsIgnoreCase("active_task")) {
			mInActiveTask = true;
		}
	}
 
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mResult != null) {
				// We are inside <result>
				if (localName.equalsIgnoreCase("result")) {
					// Closing tag of <result> - add to vector and be ready for
					// next one
					if (!mResult.name.equals("")) {
						// name is a must
						mResults.add(mResult);
					}
					mResult = null;
				}
				else {
					// Not the closing tag - we decode possible inner tags
					if (mInActiveTask) {
						// we are in <active_task>
						if (localName.equalsIgnoreCase("active_task")) {
							// Closing of <active_task>
							mResult.active_task = true;
							mInActiveTask = false;
						}
						else if (localName.equalsIgnoreCase("active_task_state")) {
							mResult.active_task_state = Integer.parseInt(getCurrentElement());
						} 
						else if (localName.equalsIgnoreCase("app_version_num")) {
							mResult.app_version_num = Integer.parseInt(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("scheduler_state")) {
							mResult.scheduler_state = Integer.parseInt(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("checkpoint_cpu_time")) {
							mResult.checkpoint_cpu_time = Double.parseDouble(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("current_cpu_time")) {
							mResult.current_cpu_time = Double.parseDouble(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("fraction_done")) {
							mResult.fraction_done = Float.parseFloat(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("elapsed_time")) {
							mResult.elapsed_time = Double.parseDouble(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("swap_size")) {
							mResult.swap_size = Double.parseDouble(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("working_set_size_smoothed")) {
							mResult.working_set_size_smoothed = Double.parseDouble(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("estimated_cpu_time_remaining")) {
							mResult.estimated_cpu_time_remaining = Double.parseDouble(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("supports_graphics")) {
							mResult.supports_graphics = !getCurrentElement().equals("0");
						}
						else if (localName.equalsIgnoreCase("graphic_mode_acked")) {
							mResult.graphics_mode_acked = Integer.parseInt(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("too_large")) {
							mResult.too_large = !getCurrentElement().equals("0");
						}
						else if (localName.equalsIgnoreCase("needs_shmem")) {
							mResult.needs_shmem = !getCurrentElement().equals("0");
						}
						else if (localName.equalsIgnoreCase("edf_scheduled")) {
							mResult.edf_scheduled = !getCurrentElement().equals("0");
						}
						else if (localName.equalsIgnoreCase("pid")) {
							mResult.pid = Integer.parseInt(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("slot")) {
							mResult.slot = Integer.parseInt(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("graphics_exec_path")) {
							mResult.graphics_exec_path = getCurrentElement();
						}
						else if (localName.equalsIgnoreCase("slot_path")) {
							mResult.slot_path = getCurrentElement();
						}
					}
					else {
					// Not in <active_task>
						if (localName.equalsIgnoreCase("name")) {
							mResult.name = getCurrentElement();
						}
						else if (localName.equalsIgnoreCase("wu_name")) {
							mResult.wu_name = getCurrentElement();
						}
						else if (localName.equalsIgnoreCase("project_url")) {
							mResult.project_url = getCurrentElement();
						}
						else if (localName.equalsIgnoreCase("version_num")) {
							mResult.version_num = Integer.parseInt(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("ready_to_report")) {
							mResult.ready_to_report = !getCurrentElement().equals("0");
						}
						else if (localName.equalsIgnoreCase("got_server_ack")) {
							mResult.got_server_ack = !getCurrentElement().equals("0");
						}
						else if (localName.equalsIgnoreCase("final_cpu_time")) {
							mResult.final_cpu_time = Double.parseDouble(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("final_elapsed_time")) {
							mResult.final_elapsed_time = Double.parseDouble(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("state")) {
							mResult.state = Integer.parseInt(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("report_deadline")) {
							mResult.report_deadline = (long)Double.parseDouble(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("received_time")) {
							mResult.received_time = (long)Double.parseDouble(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("estimated_cpu_time_remaining")) {
							mResult.estimated_cpu_time_remaining = Double.parseDouble(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("exit_status")) {
							mResult.exit_status = Integer.parseInt(getCurrentElement());
						}
						else if (localName.equalsIgnoreCase("suspended_via_gui")) {
							mResult.suspended_via_gui = !getCurrentElement().equals("0");
						}
						else if (localName.equalsIgnoreCase("project_suspended_via_gui")) {
							mResult.project_suspended_via_gui = !getCurrentElement().equals("0");
						}
						else if (localName.equalsIgnoreCase("resources")) {
							mResult.resources = getCurrentElement();
						}
					}
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
