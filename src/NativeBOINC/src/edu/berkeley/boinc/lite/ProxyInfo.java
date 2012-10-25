/* 
 * NativeBOINC - Native BOINC Client with Manager
 * Copyright (C) 2011, Mateusz Szpakowski
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

/**
 * @author mat
 *
 */
public class ProxyInfo {
	// the following is populated if user has specified an HTTP proxy
	public boolean use_http_proxy;
	public boolean use_http_auth;
	public String http_server_name;
	public int http_server_port;
	public String http_user_name;
	public String http_user_passwd;

	public boolean use_socks_proxy;
	public String socks_server_name;
	public int socks_server_port;
	public String socks5_user_name;
	public String socks5_user_passwd;

	// a list of hosts for which we should NOT go through a proxy
	public String noproxy_hosts;
}
