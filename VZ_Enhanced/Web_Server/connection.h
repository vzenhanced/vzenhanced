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

#ifndef _CONNECTION_H
#define _CONNECTION_H

#include "lite_ntdll.h"

#include <winsock2.h>
#include <mswsock.h>

#include "dllrbt.h"
#include "doublylinkedlist.h"
#include "ssl_server.h"

#define MAX_BUFFER_SIZE				16384	// Maximum size of an SSL record is 16KB.
#define MAX_RESOURCE_BUFFER_SIZE	524288	// 512KB buffer for our resource files.

#define CON_TYPE_HTTP				0
#define CON_TYPE_WEBSOCKET			1

#define UPDATE_BUFFER_POOL_SIZE		8

enum OVERLAP_TYPE
{
	OVERLAP_PING,
	OVERLAP_CALL_LOG,
	OVERLAP_CONTACT_LIST,
	OVERLAP_FORWARD_LIST,
	OVERLAP_IGNORE_LIST,
	OVERLAP_FORWARD_CID_LIST,
	OVERLAP_IGNORE_CID_LIST,
	OVERLAP_UPDATE,
	OVERLAP_CLOSE
};

enum CONNECTION_HEADER_TYPE
{
	CONNECTION_HEADER_NOT_SET,
	CONNECTION_HEADER_CLOSE,
	CONNECTION_HEADER_KEEP_ALIVE,
	CONNECTION_HEADER_UPGRADE
};

enum STATUS_CODE
{
	STATUS_CODE_0,		// Not Set
	STATUS_CODE_101,	// Switching Protocols
	STATUS_CODE_200,	// OK
	STATUS_CODE_400,	// Bad Request
	STATUS_CODE_401,	// Unauthorized
	STATUS_CODE_403,	// Forbidden
	STATUS_CODE_404,	// Not Found
	STATUS_CODE_500,	// Internal Server Error
	STATUS_CODE_501		// Not Implemented
};

struct SOCKET_CONTEXT;

struct RESOURCE_CACHE_ITEM
{
	char *resource_path;
	char *resource_data;
	DWORD resource_data_size;
};

struct UPDATE_BUFFER
{
	char			*buffer;
	volatile LONG	count;
};

struct UPDATE_BUFFER_STATE
{
	WSAOVERLAPPED	overlapped;	// We can use this and the overlapped value in our connection thread to find this state information.

	WSABUF			wsabuf;

	volatile LONG	*count;		// Points to UPDATE_BUFFER::count

	IO_OPERATION	IOOperation;
	IO_OPERATION	NextIOOperation;

	bool			in_use;
};

// Data to be associated for every I/O operation on a socket
struct IO_CONTEXT
{
	char						buffer[ MAX_BUFFER_SIZE ];

	WSAOVERLAPPED				overlapped;
	WSAOVERLAPPED				ping_overlapped;

	WSABUF						wsabuf;
	WSABUF						wsapingbuf;

	SOCKET						SocketAccept;

	DoublyLinkedList			*update_buffer_state;

	volatile LONG				ref_count;		// We should try to keep this with at most 1 pending operation.
	volatile LONG				ref_ping_count;
	volatile LONG				ref_update_count;

	int							nBufferOffset;

	IO_OPERATION				IOOperation;
	IO_OPERATION				NextIOOperation;

	IO_OPERATION				PingIOOperation;
	IO_OPERATION				PingNextIOOperation;
};

struct RESOURCE_DATA
{
	HANDLE						hFile_resource;

	char						*resource_buf;
	char						*websocket_upgrade_key;

	DWORD						resource_buf_size;
	DWORD						resource_buf_offset;

	DWORD						total_read;
	DWORD						file_size;

	STATUS_CODE					status_code;

	unsigned char				connection_type;	// 0 = not set, 1 = close, 2 = keep-alive, 3 = upgrade
	bool						is_authorized;
	bool						use_chunked_transfer;
	bool						use_cache;
	bool						has_valid_origin;
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

// Data to be associated with every socket added to the IOCP
struct SOCKET_CONTEXT
{
	IO_CONTEXT					io_context;

	CONNECTION_INFO				connection_info;	// lParam value for the Connection Manager.

	RESOURCE_DATA				resource;

	CRITICAL_SECTION			write_cs;

	SSL							*ssl;

	DoublyLinkedList			*context_node;		// Self reference to this context in the list of client contexts. Makes it easy to clean up the list.

	node_type					*ignore_node;
	node_type					*forward_node;
	node_type					*contact_node;
	node_type					*call_node;
	node_type					*ignore_cid_node;
	node_type					*forward_cid_node;
	DoublyLinkedList			*call_list;

	DoublyLinkedList			*list_data;

    LPFN_ACCEPTEX               fnAcceptEx;

	SOCKET                      Socket;

	volatile LONG				timeout;
	volatile LONG				ping_sent;

	unsigned char				connection_type;

	OVERLAP_TYPE				list_type;
};

bool CreateListenSocket();

bool CreateAcceptSocket();

DWORD WINAPI Connection( LPVOID WorkContext );

DoublyLinkedList *UpdateCompletionPort( SOCKET sd, IO_OPERATION ClientIo, bool bIsListen );

void CloseClient( DoublyLinkedList **node, bool bGraceful );

void FreeClientContexts();
void FreeListenContext();

bool DecodeRecv( SOCKET_CONTEXT *socket_context, DWORD &dwIoSize );

bool TrySend( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, IO_OPERATION next_operation );
bool TryReceive( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, IO_OPERATION next_operation );

SECURITY_STATUS WSAAPI SSL_WSASend( SOCKET_CONTEXT *socket_context, WSABUF *send_buf, SEND_BUFFER *sb, bool &sent );
SECURITY_STATUS WSAAPI SSL_WSARecv( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent );
SECURITY_STATUS WSAAPI SSL_WSAAccept( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent );
SECURITY_STATUS SSL_WSAAccept_Reply( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent );
SECURITY_STATUS SSL_WSAAccept_Response( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent );
SECURITY_STATUS SSL_WSAShutdown( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent );
SECURITY_STATUS SSL_WSARecv_Decode( SSL *ssl, LPWSABUF lpBuffers, DWORD &lpNumberOfBytesDecoded );

THREAD_RETURN Server( LPVOID pArguments );

void BeginClose( SOCKET_CONTEXT *socket_context, IO_OPERATION IOOperation = ClientIoClose );

SEND_BUFFER *GetAvailableSendBuffer( SOCKET_CONTEXT *socket_context );
SEND_BUFFER *FindSendBuffer( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped );

extern HANDLE g_hIOCP;

extern bool g_bEndServer;
extern bool g_bRestart;	
extern WSAEVENT g_hCleanupEvent[ 1 ];

extern CRITICAL_SECTION context_list_cs;		// Guard access to the global context list
extern CRITICAL_SECTION close_connection_cs;	// Close connection critical section.

extern LONG total_clients;
extern DoublyLinkedList *client_context_list;

extern char *g_domain;
extern unsigned short g_port;

extern bool use_authentication;
extern bool verify_origin;
extern bool allow_keep_alive_requests;

extern wchar_t *document_root_directory;
extern int document_root_directory_length;

#endif
