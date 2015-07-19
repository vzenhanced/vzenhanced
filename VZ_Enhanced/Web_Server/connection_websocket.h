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

#ifndef _CONNECTION_WEBSOCKET_H
#define _CONNECTION_WEBSOCKET_H

#include "connection.h"

enum WS_OPCODE
{
	WS_CONTINUATION_FRAME	= 0x00,
	WS_TEXT_FRAME			= 0x01,
	WS_BINARY_FRAME			= 0x02,
	WS_RESERVED1			= 0x03,
	WS_RESERVED2			= 0x04,
	WS_RESERVED3			= 0x05,
	WS_RESERVED4			= 0x06,
	WS_RESERVED5			= 0x07,
	WS_CONNECTION_CLOSE		= 0x08,
	WS_PING					= 0x09,
	WS_PONG					= 0x0A,
	WS_RESERVED6			= 0x0B,
	WS_RESERVED7			= 0x0C,
	WS_RESERVED8			= 0x0D,
	WS_RESERVED9			= 0x0E,
	WS_RESERVED10			= 0x0F
};

struct PAYLOAD_INFO
{
	WS_OPCODE opcode;
	char *payload_value;
};

char *DeconstructFrame( char *buf, DWORD *buf_length, WS_OPCODE &opcode, char **payload, unsigned __int64 &payload_length );

//int ConstructFrame( char *buf, int buf_length, WS_OPCODE opcode, char *payload, unsigned __int64 payload_length );
int ConstructFrameHeader( char *buf, int buf_length, WS_OPCODE opcode, /*char *payload,*/ unsigned __int64 payload_length );

void ReadWebSocketRequest( SOCKET_CONTEXT *socket_context, DWORD request_size );

void ParseWebsocketInfo( SOCKET_CONTEXT *socket_context );
void SendListData( SOCKET_CONTEXT *socket_context );
bool WriteListData( SOCKET_CONTEXT *socket_context, bool do_read );

int WriteIgnoreList( SOCKET_CONTEXT *socket_context );
int WriteIgnoreCIDList( SOCKET_CONTEXT *socket_context );
int WriteForwardList( SOCKET_CONTEXT *socket_context );
int WriteForwardCIDList( SOCKET_CONTEXT *socket_context );
int WriteContactList( SOCKET_CONTEXT *socket_context );
int WriteCallLog( SOCKET_CONTEXT *socket_context );

THREAD_RETURN SendCallLogUpdate( LPVOID pArg );

UPDATE_BUFFER *GetAvailableUpdateBuffer( DoublyLinkedList *update_buffer_pool );
UPDATE_BUFFER_STATE *GetAvailableUpdateBufferState( SOCKET_CONTEXT *socket_context );
UPDATE_BUFFER_STATE *FindUpdateBufferState( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped );

void CleanupUpdateBufferPool( DoublyLinkedList **ubp );

extern DoublyLinkedList *g_update_buffer_pool;

extern CRITICAL_SECTION g_update_buffer_pool_cs;

#endif
