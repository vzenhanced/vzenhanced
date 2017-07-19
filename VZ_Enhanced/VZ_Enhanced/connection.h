/*
	VZ Enhanced is a caller ID notifier that can forward and block phone calls.
	Copyright (C) 2013-2017 Eric Kutcher

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

#ifndef _CONNECTION_H
#define _CONNECTION_H

#include "ssl_client.h"

#define CONNECTION_KILL		0
#define CONNECTION_ACTIVE	1
#define CONNECTION_CANCEL	2
#define CONNECTION_CREATE	4

struct manageinfo
{
	contactinfo *ci;
	unsigned char manage_type;	// 0 = request all contacts, 1 = request specific contact.
};

THREAD_RETURN CheckForUpdates( void *pArguments );

THREAD_RETURN Connection( void *pArguments );

THREAD_RETURN CallPhoneNumber( void *pArguments );
THREAD_RETURN ForwardIncomingCall( void *pArguments );
THREAD_RETURN IgnoreIncomingCall( void *pArguments );

THREAD_RETURN DeleteContact( void *pArguments );
THREAD_RETURN UpdateContactInformation( void *pArguments );
THREAD_RETURN ManageContactInformation( void *pArguments );	// Can add a contact, get the full contact list, and get a single contact.
THREAD_RETURN ExportContactList( void *pArguments );
THREAD_RETURN ImportContactList( void *pArguments );

struct UPLOAD_INFO
{
	DWORD sent;
	DWORD size;
};

struct UPDATE_CHECK_INFO
{
	char *notes;
	char *download_url;
	unsigned long version;
	bool got_update;
};

struct CONNECTION
{
	SSL *ssl_socket;
	SOCKET socket;
	bool secure;
	unsigned char state;	// 0 = kill, 1 = active
};

void CleanupConnection( CONNECTION *con, char *host = NULL, bool write_to_log = true );

extern CONNECTION main_con;			// Our polling connection. Receives incoming notifications.
extern CONNECTION worker_con;		// User initiated server request connection. (Add/Edit/Remove/Import/Export contacts, etc.)
extern CONNECTION incoming_con;		// Automatic server request connection. (Ignore/Forward incoming call, Make call)
extern CONNECTION update_con;		// Connection for update checks.

extern unsigned char contact_update_in_progress;
extern unsigned char contact_import_in_progress;

#endif
