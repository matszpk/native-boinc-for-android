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

package sk.boinc.nativeboinc.bridge;

import java.text.SimpleDateFormat;
import java.util.Date;

import sk.boinc.nativeboinc.R;
import android.content.Context;
import android.content.res.Resources;


/**
 * The <code>Formatter</code> holds the objects which will be reused in single thread.
 * They are usually not thread-safe, but since each instance of this class
 * will be accessed by single thread only, they can be reused then.
 * <p>
 * For formatting a lot of objects is needed only for short time, so
 * reusing of objects could prevent some garbage collection.
 */
public class Formatter {
	private Context mContext = null;
	private Resources mResources = null;
	private StringBuilder mSb = new StringBuilder();
	private SimpleDateFormat mDateFormatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");

	public Formatter(Context context) {
		mContext = context;
		mResources = mContext.getResources();
	}

	public void cleanup() {
		mContext = null;
		mResources = null;
		mSb = null;
		mDateFormatter = null;
	}

	public final Resources getResources() {
		return mResources;
	}

	public final StringBuilder getStringBuilder() {
		mSb.setLength(0);
		return mSb;
	}

	public final String formatDate(long time) {
		Date date = new Date((long)(time*1000));
		return mDateFormatter.format(date).toString();
	}

	public static final String formatElapsedTime(long seconds) {
		long hours = seconds / 3600;
		int remain = (int)(seconds % 3600);
		int minutes = remain / 60;
		remain = remain % 60;
		return String.format("%02d:%02d:%02d", hours, minutes, remain);
	}

	public final String formatSize(long size) {
		if (size < 0)
			return mResources.getString(R.string.unknown);
		if (size > 1000000000L) {
			// Gigabytes
			return String.format("%.1f %s", ((float)size/1000000000.0F), mResources.getString(R.string.unitGB));
		}
		else if (size > 1000000L) {
			// Megabytes
			return String.format("%.1f %s", ((float)size/1000000.0F), mResources.getString(R.string.unitMB));
		}
		else if (size > 1000) {
			// Kilobytes
			return String.format("%.1f %s", ((float)size/1000.0F), mResources.getString(R.string.unitKB));
		}
		else {
			// Bytes
			return String.format("%d %s", size, mResources.getString(R.string.unitB));
		}
	}

	public final String formatBinSize(long size) {
		if (size < 0)
			return mResources.getString(R.string.unknown);
		if (size > 1073741824L) {
			// Gibibytes
			return String.format("%.1f %s", ((float)size/1073741824.0F), mResources.getString(R.string.unitGiB));
		}
		else if (size > 1048576L) {
			// Mebibytes
			return String.format("%.1f %s", ((float)size/1048576.0F), mResources.getString(R.string.unitMiB));
		}
		else if (size > 1024) {
			// Kibibytes
			return String.format("%.1f %s", ((float)size/1024.0F), mResources.getString(R.string.unitKiB));
		}
		else {
			// Bytes
			return String.format("%d %s", size, mResources.getString(R.string.unitB));
		}
	}

	public final String formatSpeed(float speed) {
		if (speed > 1000000.0F) {
			// Megabytes per second
			return String.format("%.1f %s", (speed/1000000.0F), mResources.getString(R.string.unitMBps));
		}
		else if (speed > 1000.0F) {
			// Kilobytes per second
			return String.format("%.1f %s", (speed/1000.0F), mResources.getString(R.string.unitKBps));
		}
		else {
			// Bytes
			return String.format("%.1f %s", speed, mResources.getString(R.string.unitBps));
		}
	}
	
	public final String formatDefault(String string) {
		return (string.equals("")) ? mResources.getString(R.string.defaultText) : string;
	}
	
	public final String formatGFlops(double flops) {
		return String.format("%.0f %s", flops*1e-9, mResources.getString(R.string.gflops));
	}
	
	public final String toHtmlContent(String htmlContent) {
		mSb.setLength(0);
		mSb.append("<html><body>");
		mSb.append(htmlContent);
		mSb.append("</body></html>");
		return mSb.toString();
	}
	
	public final String toHtmlLink(String htmlContent) {
		mSb.setLength(0);
		mSb.append("<html><body><a href=\"");
		mSb.append(htmlContent);
		mSb.append("\">");
		mSb.append(htmlContent);
		mSb.append("</a></body></html>");
		return mSb.toString();
	}
}
