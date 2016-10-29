/*
	VZ Enhanced is a caller ID notifier that can forward and block phone calls.
	Copyright (C) 2013-2016 Eric Kutcher

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

#ifndef _CONNECTION_RESOURCE_H
#define _CONNECTION_RESOURCE_H

#include "connection.h"
#include "readwritelock.h"

void ReadRequest( SOCKET_CONTEXT *socket_context, DWORD request_size );
void SendRedirect( SOCKET_CONTEXT *socket_context );
void SendResource( SOCKET_CONTEXT *socket_context );
void GetResource( SOCKET_CONTEXT *socket_context, char *resource_path, unsigned int resource_path_length );
void OpenResourceFile( SOCKET_CONTEXT *socket_context, char *resource_path, unsigned int resource_path_length );
void GetHeaderValues( SOCKET_CONTEXT *socket_context );
bool VerifyOrigin( char *origin, int origin_length );

void CleanupResourceCache( dllrbt_tree **resource_cache );

extern dllrbt_tree *g_resource_cache;
extern unsigned long long g_resource_cache_size;
extern unsigned long long maximum_resource_cache_size;

extern READER_WRITER_LOCK g_reader_writer_lock;
extern CRITICAL_SECTION g_fairness_mutex;
extern SRWLOCK_L g_srwlock;

#endif
