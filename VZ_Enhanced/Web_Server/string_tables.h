/*
	VZ Enhanced is a caller ID notifier that can forward and block phone calls.
	Copyright (C) 2013 Eric Kutcher

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

#ifndef _STRING_TABLES_H
#define _STRING_TABLES_H

// Values with ":", "&", or "-" are replaced with _
// Values with "..." are replaced with ___ 
// Values with # are replaced with NUM
// Keep the defines case sensitive.
// Try to keep them in order.

extern wchar_t *server_string_table[];

#define ST__								server_string_table[ 0 ]
#define ST_BTN___							server_string_table[ 1 ]
#define ST_Certificate_file_				server_string_table[ 2 ]
#define ST_Document_root_directory_			server_string_table[ 3 ]
#define ST_Enable_SSL_						server_string_table[ 4 ]
#define ST_Enable_web_server_				server_string_table[ 5 ]
#define ST_Hostname_						server_string_table[ 6 ]
#define ST_IP_address_						server_string_table[ 7 ]
#define ST_Key_file_						server_string_table[ 8 ]
#define ST_Load_PKCS_NUM12_File				server_string_table[ 9 ]
#define ST_Load_Private_Key_File			server_string_table[ 10 ]
#define ST_Load_X_509_Certificate_File		server_string_table[ 11 ]
#define ST_Password_						server_string_table[ 12 ]
#define ST_PKCS_NUM12_						server_string_table[ 13 ]
#define ST_PKCS_NUM12_file_					server_string_table[ 14 ]
#define ST_PKCS_NUM12_password_				server_string_table[ 15 ]
#define ST_Port_							server_string_table[ 16 ]
#define ST_Public_Private_key_pair_			server_string_table[ 17 ]
#define ST_Require_authentication_			server_string_table[ 18 ]
#define ST_Select_the_root_directory		server_string_table[ 19 ]
#define ST_SSL_2_0							server_string_table[ 20 ]
#define ST_SSL_3_0							server_string_table[ 21 ]
#define ST_SSL_version_						server_string_table[ 22 ]
#define ST_Start_web_server_upon_startup	server_string_table[ 23 ]
#define ST_Thread_count_					server_string_table[ 24 ]
#define ST_TLS_1_0							server_string_table[ 25 ]
#define ST_TLS_1_1							server_string_table[ 26 ]
#define ST_TLS_1_2							server_string_table[ 27 ]
#define ST_Username_						server_string_table[ 28 ]
#define ST_Verify_WebSocket_origin			server_string_table[ 29 ]

extern wchar_t *connection_string_table[];

#define ST_NUM					connection_string_table[ 0 ]
#define ST_Local_Address		connection_string_table[ 1 ]
#define ST_Local_Port			connection_string_table[ 2 ]
#define ST_Received_Bytes		connection_string_table[ 3 ]
#define ST_Remote_Address		connection_string_table[ 4 ]
#define ST_Remote_Port			connection_string_table[ 5 ]
#define ST_Sent_Bytes			connection_string_table[ 6 ]

extern wchar_t *menu_string_table[];

#define ST_Close_Connection		menu_string_table[ 0 ]
#define ST_Connection_Manager	menu_string_table[ 1 ]
#define ST_Resolve_Addresses	menu_string_table[ 2 ]
#define ST_Restart_Web_Server	menu_string_table[ 3 ]
#define ST_Select_All			menu_string_table[ 4 ]
#define ST_Start_Web_Server		menu_string_table[ 5 ]
#define ST_Stop_Web_Server		menu_string_table[ 6 ]

extern wchar_t *utilities_string_table[];

#define ST_Acquiring_cryptographic_service_provider_failed			utilities_string_table[ 0 ]
#define ST_Binary_conversion_of_private_key_failed					utilities_string_table[ 1 ]
#define ST_Decoding_of_private_key_failed							utilities_string_table[ 2 ]
#define ST_Failed_to_find_certificate_in_the_certificate_store		utilities_string_table[ 3 ]
#define ST_Failed_to_import_the_certificate_store					utilities_string_table[ 4 ]
#define ST_Import_of_private_key_failed								utilities_string_table[ 5 ]
#define ST_Invalid_password_supplied_for_the_certificate_store		utilities_string_table[ 6 ]
#define ST_Setting_certificate_context_property_failed				utilities_string_table[ 7 ]

#endif
