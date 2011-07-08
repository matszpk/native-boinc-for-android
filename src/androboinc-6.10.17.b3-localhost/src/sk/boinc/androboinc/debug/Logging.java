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

package sk.boinc.androboinc.debug;


public interface Logging {
	/**
	 * Important errors, like internal code inconsistency or external events 
	 * having serious impact on program usability.
	 * <p>These errors should always be reported, as they can help to analyze crashes reported to Market.
	 */
	public static final boolean ERROR = true;

	/**
	 * Less important errors, which could affect program usability in some circumstances.
	 * <p>These errors should always be reported.
	 */
	public static final boolean WARNING = true;

	/**
	 * Events which are usually based on external inputs and should have low impact on usability.
	 * In some situations (e.g. network problems), there can happen a lot of these events, therefore
	 * logged details must NOT be very resources-expensive.
	 * <p>These events can be reported in release build.
	 */
	public static final boolean INFO = true;

	/**
	 * Logged events for debugging purpose. Any resources-expensive stuff should use this.
	 * <p>Only turned on in debug builds, turned off at release builds.
	 */
	public static final boolean DEBUG = false;
}
