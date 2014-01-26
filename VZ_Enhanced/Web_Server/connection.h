/*
	VZ Enhanced is a caller ID notifier that can forward and block phone calls.
	Copyright (C) 2013-2014 Eric Kutcher

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

#include "lite_ntdll.h"

#include <winsock2.h>
#include <mswsock.h>

#include "dllrbt.h"
#include "doublylinkedlist.h"
#include "ssl_server.h"

//#define MAX_BUFF_SIZE		32768
#define MAX_BUFF_SIZE		16384	// Maximum size of an SSL record.
//#define MAX_BUFF_SIZE		8192
//#define MAX_BUFF_SIZE		512

#define CON_TYPE_HTTP		0
#define CON_TYPE_WEBSOCKET	1

enum IO_OPERATION
{
    ClientIoAccept,
	ClientIoHandshakeReply,
	ClientIoHandshakeResponse,
    ClientIoRead,
	ClientIoWebSocket,
	ClientIoShutdown,
	ClientIoClose,
	ClientIoReadMore,
	ClientIoWrite
};

struct CONNECTION_INFO;

enum OVERLAP_TYPE
{
	OVERLAP_PING,
	OVERLAP_CALL_LOG,
	OVERLAP_CONTACT_LIST,
	OVERLAP_FORWARD_LIST,
	OVERLAP_IGNORE_LIST,
	OVERLAP_UPDATE,
	OVERLAP_CLOSE,
};

// Data to be associated for every I/O operation on a socket
struct IO_CONTEXT
{
	char						buffer[ MAX_BUFF_SIZE ];

	WSAOVERLAPPED				overlapped;
	WSAOVERLAPPED				ping_overlapped;
	WSAOVERLAPPED				update_overlapped;

	WSABUF                      wsabuf;

	SOCKET                      SocketAccept;

	int                         nTotalBytes;
	int                         nSentBytes;
	int							nBufferOffset;

	IO_OPERATION                IOOperation;
	IO_OPERATION                LastIOOperation;

};

struct RESOURCE_DATA
{
	HANDLE						hFile_resource;

	char						*resource_buf;

	DWORD						resource_buf_size;
	DWORD						resource_buf_offset;

	DWORD						total_read;
	DWORD						file_size;

	bool						use_cache;
};

// Data to be associated with every socket added to the IOCP
struct SOCKET_CONTEXT
{
	IO_CONTEXT					*io_context;

	RESOURCE_DATA				resource;

	SSL							*ssl;

	CONNECTION_INFO				*ci;		// lParam value for the Connection Manager.

	DoublyLinkedList			*dll;

	node_type					*ignore_node;
	node_type					*forward_node;
	node_type					*contact_node;
	node_type					*call_node;
	DoublyLinkedList			*call_list;

	DoublyLinkedList			*list_data;

    LPFN_ACCEPTEX               fnAcceptEx;

	SOCKET                      Socket;

	unsigned char				timeout;
	unsigned char				ping_sent;

	unsigned char				connection_type;
	char						ref_count;		// We should try to keep this with at most 1 pending operation.

	OVERLAP_TYPE				list_type;
};

struct CONNECTION_INFO
{
	WCHAR l_host_name[ NI_MAXHOST ];
	WCHAR r_host_name[ NI_MAXHOST ];

	wchar_t r_ip[ 16 ];
	wchar_t l_ip[ 16 ];

	wchar_t r_port[ 6 ];
	wchar_t l_port[ 6 ];

	__int64 rx_bytes;
	__int64 tx_bytes;

	SOCKET_CONTEXT *psc;
};


bool CreateListenSocket();

bool CreateAcceptSocket();

DWORD WINAPI Connection( LPVOID WorkContext );

DoublyLinkedList *UpdateCompletionPort( SOCKET sd, IO_OPERATION ClientIo, bool bIsListen );

void CloseClient( DoublyLinkedList **node, bool bGraceful );

void FreeClientContexts();
void FreeListenContext();

SECURITY_STATUS WSAAPI SSL_WSASend( SOCKET_CONTEXT *socket_context, WSABUF *send_buf, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent );
SECURITY_STATUS WSAAPI SSL_WSARecv( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent );
SECURITY_STATUS WSAAPI SSL_WSAAccept( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent );
SECURITY_STATUS SSL_WSAAccept_Reply( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent );
SECURITY_STATUS SSL_WSAAccept_Response( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent );
SECURITY_STATUS SSL_WSAShutdown( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent );
SECURITY_STATUS SSL_WSARecv_Decode( SSL *ssl, LPWSABUF lpBuffers, DWORD &lpNumberOfBytesDecoded );

THREAD_RETURN Server( LPVOID pArguments );

void BeginClose( SOCKET_CONTEXT *socket_context, IO_OPERATION IOOperation = ClientIoClose/*ClientIoForceClose*/ );

extern HANDLE g_hIOCP;

extern bool g_bEndServer;
extern bool g_bRestart;	
extern WSAEVENT g_hCleanupEvent[ 1 ];

DWORD WINAPI SendCallLogUpdate( LPVOID pArg );

#endif
