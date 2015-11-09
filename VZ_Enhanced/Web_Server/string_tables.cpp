/*
	VZ Enhanced is a caller ID notifier that can forward and block phone calls.
	Copyright (C) 2013-2015 Eric Kutcher

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "string_tables.h"

wchar_t *server_string_table[] =
{
	L":",
	L"...",
	L"Allow Keep-Alive requests",
	L"Certificate file:",
	L"Document root directory:",
	L"Enable SSL / TLS:",
	L"Enable web server:",
	L"Hostname / IPv6 address:",
	L"IPv4 address:",
	L"Key file:",
	L"Load PKCS #12 File",
	L"Load Private Key File",
	L"Load X.509 Certificate File",
	L"Password:",
	L"PKCS #12:",
	L"PKCS #12 file:",
	L"PKCS #12 password:",
	L"Port:",
	L"Public / Private key pair:",
	L"Require authentication:",
	L"Resource cache size (bytes):",
	L"Select the root directory in which to serve web pages.",
	L"SSL 2.0",
	L"SSL 3.0",
	L"SSL / TLS version:",
	L"Start web server upon startup",
	L"Thread pool count:",
	L"TLS 1.0",
	L"TLS 1.1",
	L"TLS 1.2",
	L"Username:",
	L"Verify WebSocket origin"
};

wchar_t *connection_string_table[] =
{
	L"#",
	L"Local Address",
	L"Local Port",
	L"Received Bytes",
	L"Remote Address",
	L"Remote Port",
	L"Sent Bytes"
};

wchar_t *menu_string_table[] =
{
	L"Close Connection",
	L"Connection Manager",
	L"Resolve Addresses",
	L"Restart Web Server",
	L"Select All",
	L"Start Web Server",
	L"Stop Web Server"
};

wchar_t *utilities_string_table[] =
{
	L"Acquiring cryptographic service provider failed.",
	L"Binary conversion of private key failed.",
	L"Decoding of private key failed.",
	L"Failed to find certificate in the certificate store.",
	L"Failed to import the certificate store.",
	L"Import of private key failed.",
	L"Invalid password supplied for the certificate store.",
	L"Setting certificate context property failed."
};
