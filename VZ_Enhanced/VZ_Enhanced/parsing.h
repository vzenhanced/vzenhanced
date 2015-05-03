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

#ifndef _PARSING_H
#define _PARSING_H

struct COOKIE_CONTAINER
{
	char *cookie_name;
	int name_length;
	char *cookie_value;
	int value_length;
};

char *fields_tolower( char *decoded_buffer );

char *encode_xml_entities( const char *string );
char *decode_xml_entities( char *string );

bool ParseURL( char *url, char **host, char **resource );

bool ParseXApplicationErrorCode( char *decoded_buffer, char **error_code );
bool ParseRedirect( char *decoded_buffer, char **host, char **resource );
bool ParseCookies( char *decoded_buffer, dllrbt_tree **cookie_tree, char **cookies );

bool ParseSAMLForm( char *decoded_buffer, char **host, char **resource, char **parameters, int &parameter_length );

bool GetAccountInformation( char *xml, char **client_id, char **account_id, char **account_status, char **account_type, char **principal_id, char **service_type, char **service_status, char **service_context, char ***service_phone_number, char **service_privacy_value, char ***service_notifications, char ***service_features, int &phone_lines );
bool GetCallerIDInformation( char *xml, char **call_to, char **call_from, char **caller_id, char **call_reference_id );

bool GetContactListLocation( char *xml, char **contact_list_location );
bool GetMediaLocation( char *xml, char **picture_location );
bool UpdatePictureLocations( char *xml );
bool GetContactEntryId( char *xml, char **contact_entry_id );
bool BuildContactList( char *xml );
bool UpdateConactInformationIDs( char *xml, contactinfo *ci );
bool UpdateContactList( char *xml, contactinfo *ci );

void ParseImportResponse( char *xml, int &success_count, int &failure_count );

#endif
