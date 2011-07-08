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

import sk.boinc.androboinc.R;
import sk.boinc.androboinc.clientconnection.HostInfo;
import android.content.res.Resources;


public class HostInfoCreator {
	public static HostInfo create(final edu.berkeley.boinc.lite.HostInfo hostInfo, final Formatter formatter) {
		HostInfo hi = new HostInfo();
		hi.hostCpId = hostInfo.host_cpid;
		StringBuilder sb = formatter.getStringBuilder();
		Resources resources = formatter.getResources();
		sb.append(
				resources.getString(R.string.hostInfoPart1,
						hostInfo.domain_name,
						hostInfo.ip_addr,
						hostInfo.host_cpid,
						hostInfo.os_name,
						hostInfo.os_version,
						hostInfo.p_ncpus,
						hostInfo.p_vendor,
						hostInfo.p_model,
						hostInfo.p_features)
				);
		sb.append(
				resources.getString(R.string.hostInfoPart2,
						String.format("%.2f", hostInfo.p_fpops/1000000),
						String.format("%.2f", hostInfo.p_iops/1000000),
						String.format("%.2f", hostInfo.p_membw/1000000),
						formatter.formatDate(hostInfo.p_calculated),
						String.format("%.0f %s", hostInfo.m_nbytes/1024/1024, resources.getString(R.string.unitMiB)),
						String.format("%.0f %s", hostInfo.m_cache/1024, resources.getString(R.string.unitKiB)),
						String.format("%.0f %s", hostInfo.m_swap/1024/1024, resources.getString(R.string.unitMiB)),
						String.format("%.1f %s", hostInfo.d_total/1000000000, resources.getString(R.string.unitGB)),
						String.format("%.1f %s", hostInfo.d_free/1000000000, resources.getString(R.string.unitGB)))
				);
		hi.htmlText = sb.toString();
		return hi;
	}
}
