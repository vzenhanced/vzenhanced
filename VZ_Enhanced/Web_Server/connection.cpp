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

#include "globals.h"

#include "file_operations.h"
#include "utilities.h"

#include <process.h>

#include "connection.h"
#include "connection_resource.h"
#include "connection_websocket.h"

#include "lite_user32.h"

#include "menus.h"

bool in_server_thread = false;

bool g_bEndServer = false;			// set to TRUE on CTRL-C
bool g_bRestart = true;				// set to TRUE to CTRL-BRK
HANDLE g_hIOCP = INVALID_HANDLE_VALUE;
SOCKET g_sdListen = INVALID_SOCKET;
bool g_use_ipv6 = false;

LPFN_ACCEPTEX fpAcceptEx = NULL;

WSAEVENT g_hCleanupEvent[ 1 ];

LONG total_clients = 0;
DoublyLinkedList *client_context_list = NULL;
SOCKET_CONTEXT *listen_context = NULL;

CRITICAL_SECTION context_list_cs;		// Guard access to the global context list
CRITICAL_SECTION close_connection_cs;	// Close connection critical section.

PCCERT_CONTEXT g_pCertContext = NULL;

bool use_ssl = false;
unsigned char ssl_version = 2;
bool use_authentication = false;
bool verify_origin = false;
bool allow_keep_alive_requests = false;

wchar_t *document_root_directory = NULL;
int document_root_directory_length = 0;

HANDLE ping_semaphore = NULL;
char ping_buffer[ 2 ];

char *g_domain = NULL;
unsigned short g_port = 80;

void AddConnectionInfo( SOCKET_CONTEXT *socket_context )
{
	if ( socket_context != NULL )
	{
		socket_context->connection_info.rx_bytes = 0;
		socket_context->connection_info.tx_bytes = 0;
		socket_context->connection_info.psc = socket_context;

		socklen_t len = 0;
		USHORT port = 0;
		DWORD ip_length = 0;

		if ( g_use_ipv6 )
		{
			// Remote
			struct sockaddr_in6 addr6;
			_memzero( &addr6, sizeof( struct sockaddr_in6 ) );

			len = sizeof( struct sockaddr_in6 );
			_getpeername( socket_context->Socket, ( SOCKADDR * )&addr6, &len );

			port = _ntohs( addr6.sin6_port );
			addr6.sin6_port = 0;	// Prevents WSAAddressToString from returning the port.

			ip_length = 46;
			_WSAAddressToStringW( ( SOCKADDR * )&addr6, sizeof( struct sockaddr_in6 ), NULL, socket_context->connection_info.r_ip, &ip_length );

			__snwprintf( socket_context->connection_info.r_port, 6, L"%hu", port );

			_GetNameInfoW( ( SOCKADDR * )&addr6, sizeof( struct sockaddr_in6 ), ( PWCHAR )&socket_context->connection_info.r_host_name, NI_MAXHOST, NULL, 0, 0 );

			// Local
			_memzero( &addr6, sizeof( struct sockaddr_in6 ) );

			len = sizeof( struct sockaddr_in6 );
			_getsockname( socket_context->Socket, ( SOCKADDR * )&addr6, &len );

			port = _ntohs( addr6.sin6_port );
			addr6.sin6_port = 0;	// Prevents WSAAddressToString from returning the port.

			ip_length = 46;
			_WSAAddressToStringW( ( SOCKADDR * )&addr6, sizeof( struct sockaddr_in6 ), NULL, socket_context->connection_info.l_ip, &ip_length );

			__snwprintf( socket_context->connection_info.l_port, 6, L"%hu", port );

			_GetNameInfoW( ( SOCKADDR * )&addr6, sizeof( struct sockaddr_in6 ), ( PWCHAR )&socket_context->connection_info.l_host_name, NI_MAXHOST, NULL, 0, 0 );
		}
		else
		{
			// Remote
			struct sockaddr_in addr;
			_memzero( &addr, sizeof( struct sockaddr_in ) );

			len = sizeof( struct sockaddr_in );
			_getpeername( socket_context->Socket, ( SOCKADDR * )&addr, &len );

			port = _ntohs( addr.sin_port );
			addr.sin_port = 0;	// Prevents WSAAddressToString from returning the port.

			ip_length = 46;
			_WSAAddressToStringW( ( SOCKADDR * )&addr, sizeof( struct sockaddr_in ), NULL, socket_context->connection_info.r_ip, &ip_length );

			__snwprintf( socket_context->connection_info.r_port, 6, L"%hu", port );

			_GetNameInfoW( ( SOCKADDR * )&addr, sizeof( struct sockaddr_in ), ( PWCHAR )&socket_context->connection_info.r_host_name, NI_MAXHOST, NULL, 0, 0 );

			// Local
			_memzero( &addr, sizeof( struct sockaddr_in ) );

			len = sizeof( struct sockaddr_in );
			_getsockname( socket_context->Socket, ( SOCKADDR * )&addr, &len );

			port = _ntohs( addr.sin_port );
			addr.sin_port = 0;	// Prevents WSAAddressToString from returning the port.

			ip_length = 46;
			_WSAAddressToStringW( ( SOCKADDR * )&addr, sizeof( struct sockaddr_in ), NULL, socket_context->connection_info.l_ip, &ip_length );

			__snwprintf( socket_context->connection_info.l_port, 6, L"%hu", port );

			_GetNameInfoW( ( SOCKADDR * )&addr, sizeof( struct sockaddr_in ), ( PWCHAR )&socket_context->connection_info.l_host_name, NI_MAXHOST, NULL, 0, 0 );
		}

		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
		lvi.iSubItem = 0;
		lvi.iItem = _SendMessageW( g_hWnd_connections, LVM_GETITEMCOUNT, 0, 0 );
		lvi.lParam = ( LPARAM )&socket_context->connection_info;	// lParam = our contactinfo structure from the connection thread.
		_SendMessageW( g_hWnd_connections, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
	}
}

void RemoveConnectionInfo( SOCKET_CONTEXT *socket_context )
{
	if ( socket_context != NULL )
	{
		skip_connections_draw = true;

		LVFINDINFO lvfi;
		_memzero( &lvfi, sizeof( LVFINDINFO ) );
		lvfi.flags = LVFI_PARAM;
		lvfi.lParam = ( LPARAM )&socket_context->connection_info;

		int index = _SendMessageW( g_hWnd_connections, LVM_FINDITEM, -1, ( LPARAM )&lvfi );

		if ( index != -1 )
		{
			_SendMessageW( g_hWnd_connections, LVM_DELETEITEM, index, 0 );
		}

		skip_connections_draw = false;
	}
}

SEND_BUFFER *GetAvailableSendBuffer( SOCKET_CONTEXT *socket_context )
{
	SEND_BUFFER *sb = NULL;
	if ( socket_context != NULL && socket_context->ssl != NULL )
	{
		DoublyLinkedList *node = socket_context->ssl->sd.send_buffer_pool;
		while ( node != NULL )
		{
			if ( !( ( SEND_BUFFER * )node->data )->in_use )
			{
				sb = ( SEND_BUFFER * )node->data;
				break;
			}

			node = node->next;
		}
	}

	return sb;
}

SEND_BUFFER *FindSendBuffer( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped )
{
	SEND_BUFFER *sb = NULL;
	if ( socket_context != NULL && socket_context->ssl != NULL && lpWSAOverlapped != NULL )
	{
		DoublyLinkedList *node = socket_context->ssl->sd.send_buffer_pool;
		while ( node != NULL )
		{
			if ( ( ( SEND_BUFFER * )node->data )->overlapped == lpWSAOverlapped )
			{
				sb = ( SEND_BUFFER * )node->data;
				break;
			}

			node = node->next;
		}
	}

	return sb;
}

bool TrySend( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, IO_OPERATION next_operation )
{
	int nRet = 0;
	DWORD dwFlags = 0;

	bool sent = true;

	InterlockedIncrement( &socket_context->io_context.ref_count );

	socket_context->io_context.IOOperation = ClientIoWrite;
	socket_context->io_context.NextIOOperation = next_operation;

	if ( use_ssl == true )
	{
		SEND_BUFFER *sb = NULL;

		EnterCriticalSection( &socket_context->write_cs );

		sb = GetAvailableSendBuffer( socket_context );
		if ( sb == NULL )
		{
			sb = ( SEND_BUFFER * )GlobalAlloc( GPTR, sizeof( SEND_BUFFER ) );
			DoublyLinkedList *dll_node = DLL_CreateNode( ( void * )sb );
			DLL_AddNode( &socket_context->ssl->sd.send_buffer_pool, dll_node, -1 );
		}

		sb->in_use = true;
		sb->wsabuf = &( socket_context->io_context.wsabuf );
		sb->IOOperation = &( socket_context->io_context.IOOperation );
		sb->NextIOOperation = &( socket_context->io_context.NextIOOperation );
		sb->overlapped = lpWSAOverlapped;

		LeaveCriticalSection( &socket_context->write_cs );

		nRet = SSL_WSASend( socket_context, &( socket_context->io_context.wsabuf ), sb, sent );
		if ( nRet != SEC_E_OK )
		{
			sent = false;
		}
	}
	else
	{
		nRet = _WSASend( socket_context->Socket, &( socket_context->io_context.wsabuf ), 1, NULL, dwFlags, lpWSAOverlapped, NULL );
		if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
		{
			sent = false;
		}
	}

	if ( sent == false )
	{
		InterlockedDecrement( &socket_context->io_context.ref_count );
	}

	return sent;
}

bool TryReceive( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, IO_OPERATION next_operation )
{
	int nRet = 0;
	DWORD dwFlags = 0;

	bool sent = true;

	InterlockedIncrement( &socket_context->io_context.ref_count );

	socket_context->io_context.IOOperation = next_operation;

	if ( use_ssl == true )
	{
		nRet = SSL_WSARecv( socket_context, lpWSAOverlapped, sent );
		if ( nRet != SEC_E_OK )
		{
			sent = false;
		}
	}
	else
	{
		nRet = _WSARecv( socket_context->Socket, &( socket_context->io_context.wsabuf ), 1, NULL, &dwFlags, lpWSAOverlapped, NULL );
		if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
		{
			sent = false;
		}
	}

	if ( sent == false )
	{
		InterlockedDecrement( &socket_context->io_context.ref_count );
	}

	return sent;
}

SECURITY_STATUS DecryptRecv( SOCKET_CONTEXT *socket_context, DWORD &dwIoSize, bool &extra_data )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	WSABUF wsa_decrypt;

	DWORD bytes_decrypted = 0;

	if ( socket_context->ssl->rd.scRet == SEC_E_INCOMPLETE_MESSAGE )
	{
		socket_context->ssl->cbIoBuffer += dwIoSize;
	}
	else
	{
		socket_context->ssl->cbIoBuffer = dwIoSize;
	}

	dwIoSize = 0;
	extra_data = false;

	wsa_decrypt = socket_context->io_context.wsabuf;

	// Decode our message.
	while ( socket_context->ssl->pbIoBuffer != NULL && socket_context->ssl->cbIoBuffer > 0 )
	{
		scRet = SSL_WSARecv_Decrypt( socket_context->ssl, &wsa_decrypt, bytes_decrypted );

		dwIoSize += bytes_decrypted;

		wsa_decrypt.buf += bytes_decrypted;
		wsa_decrypt.len -= bytes_decrypted;

		switch ( scRet )
		{
			// We've successfully decoded a portion of the buffer.
			case SEC_E_OK:
			{
				// Decode more records if there are any.
				continue;
			}
			break;

			// The message was decrypted, but not all of it was copied to our wsabuf.
			// There may also be incomplete records left to decrypt.
			// DecryptRecv must be called again after processing wsabuf.
			case SEC_I_CONTINUE_NEEDED:
			{
				extra_data = true;

				return scRet;
			}
			break;

			// The message was incomplete. Request more data from the client.
			case SEC_E_INCOMPLETE_MESSAGE:
			{
				return scRet;
			}
			break;

			// Client wants us to perform another handshake.
			case SEC_I_RENEGOTIATE:
			{
				bool sent = false;
				socket_context->io_context.IOOperation = ClientIoHandshakeReply;
				InterlockedIncrement( &socket_context->io_context.ref_count );
				SSL_WSAAccept( socket_context, &( socket_context->io_context.overlapped ), sent );

				if ( sent == false )
				{
					InterlockedDecrement( &socket_context->io_context.ref_count );
				}

				return scRet;
			}
			break;

			// Client has signaled a shutdown, or some other error occurred.
			//case SEC_I_CONTEXT_EXPIRED:
			default:
			{
				socket_context->ssl->cbIoBuffer = 0;

				BeginClose( socket_context, ClientIoShutdown );

				return scRet;
			}
			break;
		}
	}

	socket_context->ssl->cbIoBuffer = 0;

	return scRet;
}

void BeginClose( SOCKET_CONTEXT *socket_context, IO_OPERATION IOOperation )
{
	if ( socket_context != NULL )
	{
		InterlockedIncrement( &socket_context->io_context.ref_count );
		socket_context->io_context.IOOperation = IOOperation;
		PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )socket_context, &( socket_context->io_context.overlapped ) );
	}
}

void SendPing( SOCKET_CONTEXT *socket_context )
{
	int nRet = 0;
	DWORD dwFlags = 0;

	bool sent = true;

	InterlockedIncrement( &socket_context->io_context.ref_ping_count );

	socket_context->io_context.PingIOOperation = ClientIoWrite;
	socket_context->io_context.PingNextIOOperation = ClientIoReadWebSocketRequest;

	socket_context->io_context.wsapingbuf.buf = ping_buffer;
	socket_context->io_context.wsapingbuf.len = 2;

	if ( use_ssl == true )
	{
		SEND_BUFFER *sb = NULL;

		EnterCriticalSection( &socket_context->write_cs );

		sb = GetAvailableSendBuffer( socket_context );
		if ( sb == NULL )
		{
			sb = ( SEND_BUFFER * )GlobalAlloc( GPTR, sizeof( SEND_BUFFER ) );
			DoublyLinkedList *dll_node = DLL_CreateNode( ( void * )sb );
			DLL_AddNode( &socket_context->ssl->sd.send_buffer_pool, dll_node, -1 );
		}

		sb->in_use = true;
		sb->wsabuf = &( socket_context->io_context.wsapingbuf );
		sb->IOOperation = &( socket_context->io_context.PingIOOperation );
		sb->NextIOOperation = &( socket_context->io_context.PingNextIOOperation );
		sb->overlapped = &( socket_context->io_context.ping_overlapped );

		LeaveCriticalSection( &socket_context->write_cs );

		nRet = SSL_WSASend( socket_context, &( socket_context->io_context.wsapingbuf ), sb, sent );
		if ( nRet != SEC_E_OK )
		{
			sent = false;
		}
	}
	else
	{
		nRet = _WSASend( socket_context->Socket, &( socket_context->io_context.wsapingbuf ), 1, NULL, dwFlags, &( socket_context->io_context.ping_overlapped ), NULL );
		if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
		{
			sent = false;
		}
	}

	if ( sent == false )
	{
		InterlockedDecrement( &socket_context->io_context.ref_ping_count );
		BeginClose( socket_context );
	}
}


// Stale connections will be closed in 1 minute.
// WebSocket connections will be closed if a ping request fails.
DWORD WINAPI Poll( LPVOID WorkThreadContext )
{
	while ( g_bEndServer == false )
	{
		WaitForSingleObject( ping_semaphore, 30000 );

		if ( g_bEndServer == true )
		{
			break;
		}

		EnterCriticalSection( &context_list_cs );

		DoublyLinkedList *context_node = client_context_list;

		while ( context_node != NULL )
		{
			if ( g_bEndServer == true )
			{
				break;
			}

			SOCKET_CONTEXT *socket_context = ( SOCKET_CONTEXT * )context_node->data;

			if ( socket_context->connection_type == CON_TYPE_WEBSOCKET )
			{
				InterlockedIncrement( &socket_context->ping_sent );

				if ( socket_context->ping_sent == 2 )
				{
					SendPing( socket_context );
				}
				else if ( socket_context->ping_sent > 2 )
				{
					BeginClose( socket_context );
				}
			}
			else
			{
				InterlockedIncrement( &socket_context->timeout );

				if ( socket_context->timeout >= 2 )
				{
					BeginClose( socket_context );
				}
			}

			context_node = context_node->next;
		}

		LeaveCriticalSection( &context_list_cs );
	}

	CloseHandle( ping_semaphore );
	ping_semaphore = NULL;

	ExitThread( 0 );
	return 0;
}

THREAD_RETURN Server( LPVOID pArguments )
{
	DWORD dwThreadCount = 0;

	if ( ws2_32_state == WS2_32_STATE_SHUTDOWN )
	{
		#ifndef WS2_32_USE_STATIC_LIB
			if ( InitializeWS2_32() == false ){ _ExitThread( 0 ); return 0; }
		#else
			StartWS2_32();
		#endif
	}

	g_bRestart = true;
	g_bEndServer = true;

	in_server_thread = true;

	EnableDisableMenus( true );

	InitializeCriticalSection( &context_list_cs );
	InitializeCriticalSection( &close_connection_cs );
	InitializeCriticalSection( &g_update_buffer_pool_cs );

	if ( use_rwl_library == true )
	{
		InitializeCriticalSection( &g_fairness_mutex );
		_InitializeSRWLock( &g_srwlock );
	}
	else
	{
		InitializeReaderWriterLock( &g_reader_writer_lock );
	}

	HANDLE *g_ThreadHandles = ( HANDLE * )GlobalAlloc( GMEM_FIXED, sizeof ( HANDLE ) * max_threads );

	for ( unsigned int i = 0; i < max_threads; ++i )
	{
		g_ThreadHandles[ i ] = INVALID_HANDLE_VALUE;
	}

	g_hCleanupEvent[ 0 ] = _WSACreateEvent();
	if ( g_hCleanupEvent[ 0 ] == WSA_INVALID_EVENT )
	{
		g_bRestart = false;
	}

	// Global value for the websocket ping.
	ping_buffer[ 0 ] = -119;	// 0x89; Ping byte
	ping_buffer[ 1 ] = 0;		// Sanity.

	while ( g_bRestart == true )
	{
		g_bRestart = false;
		g_bEndServer = false;

		g_use_ipv6 = false;

		use_ssl = cfg_enable_ssl;
		ssl_version = cfg_ssl_version;
		use_authentication = cfg_use_authentication;
		verify_origin = cfg_verify_origin;
		allow_keep_alive_requests = cfg_allow_keep_alive_requests;
		maximum_resource_cache_size = cfg_resource_cache_size;

		dwThreadCount = cfg_thread_count;

		if ( use_ssl == true )
		{
			if ( ssl_state == SSL_STATE_SHUTDOWN )
			{
				if ( SSL_library_init() == 0 ){ break; }
			}
		}

		g_hIOCP = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
        if ( g_hIOCP == NULL )
		{
			break;
		}

		if ( use_ssl == true && g_pCertContext == NULL )
		{
			if ( cfg_certificate_type == 1 )	// Public/Private Key Pair.
			{
				g_pCertContext = LoadPublicPrivateKeyPair( cfg_certificate_cer_file_name, cfg_certificate_key_file_name );
			}
			else	// PKCS #12 File.
			{
				g_pCertContext = LoadPKCS12( cfg_certificate_pkcs_file_name, cfg_certificate_pkcs_password );
			}

			if ( g_pCertContext == NULL )
			{
				break;
			}
		}

		if ( document_root_directory == NULL )
		{
			document_root_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof ( wchar_t ) * ( g_document_root_directory_length + 1 ) );
			_wmemcpy_s( document_root_directory, g_document_root_directory_length + 1, cfg_document_root_directory, g_document_root_directory_length );
			document_root_directory[ g_document_root_directory_length ] = 0;	// Sanity.
			document_root_directory_length = g_document_root_directory_length;
		}

		g_resource_cache = dllrbt_create( dllrbt_compare );

		PreloadIndexFile( g_resource_cache, &g_resource_cache_size, maximum_resource_cache_size );

		if ( encoded_authentication == NULL )
		{
			encoded_authentication = CreateAuthentication( cfg_authentication_username, cfg_authentication_password, encoded_authentication_length );
		}

		_WSAResetEvent( g_hCleanupEvent[ 0 ] );

		for ( DWORD dwCPU = 0; dwCPU < dwThreadCount; ++dwCPU )
		{
			HANDLE hThread;
			DWORD dwThreadId;

			// Create worker threads to service the overlapped I/O requests.
			hThread = _CreateThread( NULL, 0, Connection, g_hIOCP, 0, &dwThreadId );
			if ( hThread == NULL )
			{
				break;
			}

			g_ThreadHandles[ dwCPU ] = hThread;
			hThread = INVALID_HANDLE_VALUE;
		}

		if ( CreateListenSocket() == true && CreateAcceptSocket() == true )
		{
			_WSAWaitForMultipleEvents( 1, g_hCleanupEvent, TRUE, WSA_INFINITE, FALSE );
		}

		g_bEndServer = true;

		// Exit our polling thread if it's active.
		if ( ping_semaphore != NULL )
		{
			ReleaseSemaphore( ping_semaphore, 1, NULL );
		}

		// Cause worker threads to exit
		if ( g_hIOCP != NULL )
		{
			for ( DWORD i = 0; i < dwThreadCount; ++i )
			{
				PostQueuedCompletionStatus( g_hIOCP, 0, 0, NULL );
			}
		}

		// Make sure worker threads exit.
		if ( WaitForMultipleObjects( dwThreadCount, g_ThreadHandles, TRUE, 1000 ) == WAIT_OBJECT_0 )
		{
			for ( DWORD i = 0; i < dwThreadCount; ++i )
			{
				if ( g_ThreadHandles[ i ] != INVALID_HANDLE_VALUE )
				{
					CloseHandle( g_ThreadHandles[ i ] );
					g_ThreadHandles[ i ] = INVALID_HANDLE_VALUE;
				}
			}
		}

		if ( g_sdListen != INVALID_SOCKET )
		{
			_shutdown( g_sdListen, SD_BOTH );
			_closesocket( g_sdListen );
			g_sdListen = INVALID_SOCKET;
		}

		FreeListenContext();

		FreeClientContexts();

		if ( g_hIOCP != NULL )
		{
			CloseHandle( g_hIOCP );
			g_hIOCP = NULL;
		}

		if ( g_domain != NULL )
		{
			GlobalFree( g_domain );
			g_domain = NULL;
		}

		if ( encoded_authentication != NULL )
		{
			GlobalFree( encoded_authentication );
			encoded_authentication = NULL;
		}
		encoded_authentication_length = 0;

		if ( document_root_directory != NULL )
		{
			GlobalFree( document_root_directory );
			document_root_directory = NULL;
		}
		document_root_directory_length = 0;

		CleanupUpdateBufferPool( &g_update_buffer_pool );	// g_update_buffer_pool is set to NULL.

		CleanupResourceCache( &g_resource_cache );	// g_resource_cache is set to NULL. 
		g_resource_cache_size = 0;

		if ( use_ssl == true )
		{
			if ( g_pCertContext != NULL )
			{
				_CertFreeCertificateContext( g_pCertContext );
				g_pCertContext = NULL;
			}

			ResetCredentials();
		}
	}

	GlobalFree( g_ThreadHandles );
	g_ThreadHandles = NULL;

	if ( g_hCleanupEvent[ 0 ] != WSA_INVALID_EVENT )
	{
		_WSACloseEvent( g_hCleanupEvent[ 0 ] );
		g_hCleanupEvent[ 0 ] = WSA_INVALID_EVENT;
	}

	if ( use_rwl_library == true )
	{
		DeleteCriticalSection( &g_fairness_mutex );
		// The SRWLOCK g_srwlock does not need to be deleted/uninitialized.
	}
	else
	{
		DeleteReaderWriterLock( &g_reader_writer_lock );
	}

	DeleteCriticalSection( &g_update_buffer_pool_cs );
	DeleteCriticalSection( &close_connection_cs );
	DeleteCriticalSection( &context_list_cs );

	EnableDisableMenus( false );

	if ( server_semaphore != NULL )
	{
		ReleaseSemaphore( server_semaphore, 1, NULL );
	}

	in_server_thread = false;

	_ExitThread( 0 );
	return 0;
}

SOCKET CreateSocket( bool IPv6 )
{
	int nRet = 0;
	int nZero = 0;
	SOCKET socket = INVALID_SOCKET;

	socket = _WSASocketW( ( IPv6 ? AF_INET6 : AF_INET ), SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED ); 
	if ( socket == INVALID_SOCKET )
	{
		return socket;
	}

	// Disable send buffering on the socket.
	nZero = 0;
	nRet = _setsockopt( socket, SOL_SOCKET, SO_SNDBUF, ( char * )&nZero, sizeof( nZero ) );
	if ( nRet == SOCKET_ERROR )
	{
		return socket;
	}

	return socket;
}

bool CreateListenSocket()
{
	bool ret = false;

	int nRet = 0;

	DWORD bytes = 0;
	GUID acceptex_guid = WSAID_ACCEPTEX;	// GUID to Microsoft specific extensions

	struct addrinfoW hints;
	struct addrinfoW *addrlocal = NULL;

	// Resolve the interface
	_memzero( &hints, sizeof( addrinfoW ) );
    hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_IP;

	wchar_t cport[ 6 ];
	__snwprintf( cport, 6, L"%hu", cfg_port );

	g_port = cfg_port;

	if ( g_domain != NULL )
	{
		GlobalFree( g_domain );
		g_domain = NULL;
	}

	// Use Hostname or IPv6 Address.
	if ( cfg_address_type == 0 )
	{
		g_domain = GetUTF8Domain( cfg_hostname );

		nRet = _GetAddrInfoW( cfg_hostname, cport, &hints, &addrlocal );
		if ( nRet == WSAHOST_NOT_FOUND )
		{
			g_use_ipv6 = true;

			hints.ai_family = AF_INET6;	// Try IPv6
			nRet = _GetAddrInfoW( cfg_hostname, cport, &hints, &addrlocal );
		}

		if ( nRet != 0 )
		{
			ret = false;
			goto CLEANUP;
		}

		// Check the IPv6 address' formatting. It should be surrounded by brackets.
		// GetAddrInfoW supports it with or without, but we want it to have it.
		if ( g_use_ipv6 )
		{
			if ( g_domain != NULL && *g_domain != '[' )
			{
				int g_domain_length = lstrlenA( g_domain );
				char *new_g_domain = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( g_domain_length + 3 ) );	// 2 brackets and the NULL character.
				new_g_domain[ 0 ] = '[';
				_memcpy_s( new_g_domain + 1, g_domain_length + 2, g_domain, g_domain_length );
				new_g_domain[ g_domain_length + 1 ] = ']';
				new_g_domain[ g_domain_length + 2 ] = 0;	// Sanity.

				GlobalFree( g_domain );
				g_domain = new_g_domain;
			}
		}
	}
	else	// Use IPv4 Address.
	{
		struct sockaddr_in src_addr;
		_memzero( &src_addr, sizeof( sockaddr_in ) );

		src_addr.sin_family = AF_INET;
		src_addr.sin_addr.s_addr = _htonl( cfg_ip_address );

		wchar_t wcs_ip[ 16 ];
		DWORD wcs_ip_length = 16;
		_WSAAddressToStringW( ( SOCKADDR * )&src_addr, sizeof( struct sockaddr_in ), NULL, wcs_ip, &wcs_ip_length );

		g_domain = GetUTF8Domain( wcs_ip );

		if ( _GetAddrInfoW( wcs_ip, cport, &hints, &addrlocal ) != 0 )
		{
			ret = false;
			goto CLEANUP;
		}
	}

	if ( addrlocal == NULL )
	{
        ret = false;
		goto CLEANUP;
	}

	g_sdListen = CreateSocket( g_use_ipv6 );
	if ( g_sdListen == INVALID_SOCKET)
	{
		ret = false;
		goto CLEANUP;
	}

    nRet = _bind( g_sdListen, addrlocal->ai_addr, ( int )addrlocal->ai_addrlen );
	if ( nRet == SOCKET_ERROR)
	{
		ret = false;
		goto CLEANUP;
	}

	nRet = _listen( g_sdListen, SOMAXCONN );
	if ( nRet == SOCKET_ERROR )
	{
		ret = false;
		goto CLEANUP;
	}

	// We need only do this once.
	if ( fpAcceptEx == NULL )
	{
		// Load the AcceptEx extension function from the provider for this socket.
		nRet = _WSAIoctl( g_sdListen, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof( acceptex_guid ), &fpAcceptEx, sizeof( fpAcceptEx ), &bytes, NULL, NULL );
		if ( nRet == SOCKET_ERROR )
		{
			ret = false;
			goto CLEANUP;
		}
	}

	ret = true;

CLEANUP:

	if ( addrlocal != NULL )
	{
		_FreeAddrInfoW( addrlocal );
	}

	return ret;
}

bool CreateAcceptSocket()
{
	int nRet = 0;
	DWORD dwRecvNumBytes = 0;

	// The listening socket context uses the SocketAccept member to store the socket for client connection. 
	if ( listen_context == NULL )
	{
		listen_context = UpdateCompletionPort( g_sdListen, ClientIoAccept, true );
		if ( listen_context == NULL )
		{
			return false;
		}
	}

	listen_context->Socket = CreateSocket( g_use_ipv6 );	// The accept socket will inherit the listen socket's properties when it completes. IPv6 doesn't actually have to be set here.
	if ( listen_context->Socket == INVALID_SOCKET )
	{
		return false;
	}

	// Accept a connection without waiting for any data. (dwReceiveDataLength = 0)
	nRet = fpAcceptEx( g_sdListen, listen_context->Socket, ( LPVOID )( listen_context->io_context.buffer ), 0, sizeof( SOCKADDR_STORAGE ) + 16, sizeof( SOCKADDR_STORAGE ) + 16, &dwRecvNumBytes, &( listen_context->io_context.overlapped ) );
	if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
	{
		return false;
	}

	return true;
}

// Worker thread that handles all I/O requests on any socket handle added to the IOCP.
DWORD WINAPI Connection( LPVOID WorkThreadContext )
{
	HANDLE hIOCP = ( HANDLE )WorkThreadContext;
	BOOL bSuccess = FALSE;
	WSAOVERLAPPED *overlapped = NULL;
	SOCKET_CONTEXT *socket_context = NULL;
	DWORD dwFlags = 0;
	DWORD dwIoSize = 0;

	volatile LONG *ref_count = NULL;
	WSABUF *wsabuf = NULL;
	IO_OPERATION *io_operation = NULL;
	IO_OPERATION *next_io_operation = NULL;

	UPDATE_BUFFER_STATE *ubs = NULL;

	while ( true )
	{
		// Service io completion packets
		bSuccess = GetQueuedCompletionStatus( hIOCP, &dwIoSize, ( PDWORD_PTR )&socket_context, ( LPOVERLAPPED * )&overlapped, INFINITE );

		if ( g_bEndServer == true )
		{
			break;
		}

		if ( socket_context == NULL )
		{
			continue;
		}

		InterlockedExchange( &socket_context->timeout, 0 );	// Reset timeout counter.

		if ( overlapped == &( socket_context->io_context.overlapped ) )
		{
			io_operation = &( socket_context->io_context.IOOperation );
			next_io_operation = &( socket_context->io_context.NextIOOperation );
			wsabuf = &( socket_context->io_context.wsabuf );
			ref_count = &( socket_context->io_context.ref_count );
		}
		else if ( overlapped == &( socket_context->io_context.ping_overlapped ) )
		{
			io_operation = &( socket_context->io_context.PingIOOperation );
			next_io_operation = &( socket_context->io_context.PingNextIOOperation );
			wsabuf = &( socket_context->io_context.wsapingbuf );
			ref_count = &( socket_context->io_context.ref_ping_count );
		}
		else
		{
			EnterCriticalSection( &socket_context->write_cs );

			ubs = FindUpdateBufferState( socket_context, overlapped );

			LeaveCriticalSection( &socket_context->write_cs );

			wsabuf = &( ubs->wsabuf );
			io_operation = &( ubs->IOOperation );
			next_io_operation = &( ubs->NextIOOperation );
			ref_count = &( socket_context->io_context.ref_update_count );
		}

		if ( *io_operation != ClientIoAccept )
		{
			InterlockedDecrement( ref_count );

			// If the completion failed, and we have pending operations, then continue until we have no more in the queue.
			if ( bSuccess == FALSE && ( socket_context->io_context.ref_count > 0 ||
										socket_context->io_context.ref_ping_count > 0 ||
										socket_context->io_context.ref_update_count > 0 ) )
			{
				continue;
			}

			if ( bSuccess == FALSE || ( bSuccess == TRUE && dwIoSize == 0 ) )
			{
				// The connection was either shutdown by the client, or closed abruptly.
				if ( *io_operation != ClientIoShutdown && *io_operation != ClientIoClose )
				{
					*io_operation = ( use_ssl == true ? ClientIoShutdown : ClientIoClose );
				}
			}
		}

		switch ( *io_operation )
		{
			case ClientIoAccept:
			{
				// Allow the accept socket to inherit the properties of the listen socket.
				// The socket_context here is actually from listen_context->data.
				int nRet = _setsockopt( socket_context->Socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, ( char * )&g_sdListen, sizeof( g_sdListen ) );
				if ( nRet == SOCKET_ERROR )
				{
					_WSASetEvent( g_hCleanupEvent[ 0 ] );
					ExitThread( 0 );
					return 0;
				}

				// Create a new socket context with the inherited socket.
				socket_context = UpdateCompletionPort( socket_context->Socket, ClientIoAccept, false );
				if ( socket_context == NULL )
				{
					_WSASetEvent( g_hCleanupEvent[ 0 ] );
					ExitThread( 0 );
					return 0;
				}

				bool sent = true;

				InterlockedIncrement( &socket_context->io_context.ref_count );
				
				if ( use_ssl == true )
				{
					socket_context->io_context.IOOperation = ClientIoHandshakeReply;
					SSL_WSAAccept( socket_context, &( socket_context->io_context.overlapped ), sent );
				}
				else
				{
					socket_context->io_context.IOOperation = ClientIoReadRequest;
					nRet = _WSARecv( socket_context->Socket, &socket_context->io_context.wsabuf, 1, NULL, &dwFlags, &( socket_context->io_context.overlapped ), NULL );
					if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
					{
						sent = false;
					}
				}

				if ( sent == false )
				{
					InterlockedDecrement( &socket_context->io_context.ref_count );
					BeginClose( socket_context );
				}

				// Check to see if we need to enter the critical section to create the polling thread.
				if ( ping_semaphore == NULL )
				{
					EnterCriticalSection( &context_list_cs );

					// Check again if we had threads queue up.
					if ( ping_semaphore == NULL )
					{
						ping_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

						CloseHandle( _CreateThread( NULL, 0, Poll, NULL, 0, NULL ) );
					}

					LeaveCriticalSection( &context_list_cs );
				}

				// Post another outstanding AcceptEx.
				if ( CreateAcceptSocket() == false )
				{
					_WSASetEvent( g_hCleanupEvent[ 0 ] );
					ExitThread( 0 );
					return 0;
				}
			}
			break;

			case ClientIoHandshakeReply:
			{
				// We process data from the client and write our reply.

				socket_context->connection_info.rx_bytes += dwIoSize;

				socket_context->ssl->cbIoBuffer += dwIoSize;

				bool sent = false;
				InterlockedIncrement( ref_count );
				*next_io_operation = ClientIoHandshakeResponse;
				SECURITY_STATUS sRet = SSL_WSAAccept_Reply( socket_context, &( socket_context->io_context.overlapped ), sent );

				if ( sent == false )
				{
					InterlockedDecrement( ref_count );
				}

				if ( sRet == SEC_E_OK )	// If true, then no send was made.
				{
					*io_operation = ClientIoReadRequest;

					if ( socket_context->ssl->cbIoBuffer > 0 )
					{
						// The request was sent with the handshake.
						ReadRequest( socket_context, socket_context->ssl->cbIoBuffer );
					}
					else
					{
						socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer;
						socket_context->io_context.wsabuf.len = MAX_BUFFER_SIZE;

						// Read the request from the client.
						bool ret = TryReceive( socket_context, &( socket_context->io_context.overlapped ), *io_operation );
						if ( ret == false )
						{
							BeginClose( socket_context );
						}
					}
				}
				else if ( sRet == SEC_E_INCOMPLETE_MESSAGE )
				{
					SendRedirect( socket_context );
				}
				else if ( sRet != SEC_I_CONTINUE_NEEDED && sRet != SEC_I_INCOMPLETE_CREDENTIALS )	// Stop handshake and close the connection.
				{
					BeginClose( socket_context );
				}
			}
			break;

			case ClientIoHandshakeResponse:
			{
				// We post a read for data from the client. What gets read is processed by SSL_WSAAccept_Reply.

				bool sent = false;
				InterlockedIncrement( ref_count );
				*io_operation = ClientIoHandshakeReply;
				SECURITY_STATUS sRet = SSL_WSAAccept_Response( socket_context, &( socket_context->io_context.overlapped ), sent );

				if ( sent == false )
				{
					InterlockedDecrement( ref_count );
				}

				if ( sRet == SEC_E_OK )	// If true, then no recv was made.
				{
					*io_operation = ClientIoReadRequest;

					if ( socket_context->ssl->cbIoBuffer > 0 )
					{
						// The request was sent with the handshake.
						ReadRequest( socket_context, socket_context->ssl->cbIoBuffer );
					}
					else
					{
						socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer;
						socket_context->io_context.wsabuf.len = MAX_BUFFER_SIZE;

						// Read the request from the client.
						bool ret = TryReceive( socket_context, &( socket_context->io_context.overlapped ), *io_operation );
						if ( ret == false )
						{
							BeginClose( socket_context );
						}
					}
				}
				else if ( sRet != SEC_I_CONTINUE_NEEDED && sRet != SEC_E_INCOMPLETE_MESSAGE && sRet != SEC_I_INCOMPLETE_CREDENTIALS )
				{
					BeginClose( socket_context );
				}
			}
			break;

			case ClientIoWrite:
			{
				socket_context->connection_info.tx_bytes += dwIoSize;

				if ( dwIoSize < wsabuf->len )
				{
					// We do a regular WSASend here since that's what we last did in SSL_WSASend. socket_context->Socket == socket_context->ssl->s.
					InterlockedIncrement( ref_count );
					wsabuf->buf += dwIoSize;
					wsabuf->len -= dwIoSize;
					int nRet = _WSASend( socket_context->Socket, wsabuf, 1, NULL, dwFlags, overlapped, NULL );
					if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
					{
						InterlockedDecrement( ref_count );
						BeginClose( socket_context );
					}
				}
				else
				{
					*io_operation = *next_io_operation;

					// Allow our SSL send buffer to be reused.
					if ( use_ssl == true )
					{
						EnterCriticalSection( &socket_context->write_cs );

						SEND_BUFFER *sb = FindSendBuffer( socket_context, overlapped );
						if ( sb != NULL )
						{
							sb->in_use = false;
						}

						LeaveCriticalSection( &socket_context->write_cs );
					}

					if ( overlapped == &( socket_context->io_context.overlapped ) )
					{
						wsabuf->buf = socket_context->io_context.buffer;
						wsabuf->len = MAX_BUFFER_SIZE;

						InterlockedIncrement( ref_count );
						PostQueuedCompletionStatus( hIOCP, 1, ( ULONG_PTR )socket_context, overlapped );
					}
					else if ( overlapped != &( socket_context->io_context.ping_overlapped ) )	// Update overlaps.
					{
						EnterCriticalSection( &socket_context->write_cs );

						ubs->in_use = false;

						EnterCriticalSection( &g_update_buffer_pool_cs );

						InterlockedDecrement( ubs->count );	// This value belongs to the g_update_buffer_pool.

						LeaveCriticalSection( &g_update_buffer_pool_cs );

						LeaveCriticalSection( &socket_context->write_cs );
					}
				}
			}
			break;

			case ClientIoWriteRequestResource:
			{
				SendResource( socket_context );
			}
			break;

			case ClientIoWriteWebSocketLists:
			{
				SendListData( socket_context );
			}
			break;

			case ClientIoReadRequest:
			{
				ReadRequest( socket_context, dwIoSize );
			}
			break;

			case ClientIoReadMoreRequest:
			{
				socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer;
				socket_context->io_context.wsabuf.len = MAX_BUFFER_SIZE;

				bool ret = TryReceive( socket_context, &( socket_context->io_context.overlapped ), ClientIoReadRequest );
				if ( ret == false )
				{
					BeginClose( socket_context );
				}
			}
			break;

			case ClientIoReadWebSocketRequest:
			{
				// We're going to ignore ping and update events.
				if ( overlapped == &( socket_context->io_context.overlapped ) )
				{
					ReadWebSocketRequest( socket_context, dwIoSize );
				}
			}
			break;

			case ClientIoReadMoreWebSocketRequest:
			{
				socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer;
				socket_context->io_context.wsabuf.len = MAX_BUFFER_SIZE;

				bool ret = TryReceive( socket_context, &( socket_context->io_context.overlapped ), ClientIoReadWebSocketRequest );
				if ( ret == false )
				{
					BeginClose( socket_context );
				}
			}
			break;

			case ClientIoShutdown:
			{
				socket_context->connection_type = CON_TYPE_HTTP;

				if ( use_ssl == true )
				{
					bool sent = false;

					*next_io_operation = ClientIoClose;

					InterlockedIncrement( ref_count );
					SECURITY_STATUS sRet = SSL_WSAShutdown( socket_context, &( socket_context->io_context.overlapped ), sent );

					if ( sent == false )
					{
						InterlockedDecrement( ref_count );
					}

					if ( sRet != SEC_E_OK )
					{
						*io_operation = ClientIoClose;
						InterlockedIncrement( ref_count );
						PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )socket_context, &( socket_context->io_context.overlapped ) );
					}
				}
				else
				{
					*io_operation = ClientIoClose;
					InterlockedIncrement( ref_count );
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )socket_context, &( socket_context->io_context.overlapped ) );
				}
			}
			break;

			case ClientIoClose:
			{
				socket_context->connection_type = CON_TYPE_HTTP;

				if ( *ref_count > 0 )
				{
					if ( socket_context->Socket != INVALID_SOCKET )
					{
						SOCKET s = socket_context->Socket;
						socket_context->Socket = INVALID_SOCKET;
						_closesocket( s );	// Saves us from having to post if there's already a pending IO operation. Should force the operation to complete.
					}
				}
				else	// Done when no other IO operation is pending.
				{
					CloseClient( socket_context, false );
				}
			}
			break;
		}
	}

	ExitThread( 0 );
	return 0;
} 

// Allocate a context structures for the socket and add the socket to the IOCP.
// Additionally, add the context structure to the global list of context structures.
SOCKET_CONTEXT *UpdateCompletionPort( SOCKET sd, IO_OPERATION ClientIo, bool bIsListen )
{
	SOCKET_CONTEXT *socket_context = ( SOCKET_CONTEXT * )GlobalAlloc( GPTR, sizeof( SOCKET_CONTEXT ) );
	if ( socket_context )
	{
		socket_context->Socket = sd;

		socket_context->io_context.IOOperation = ClientIo;
		socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer;
		socket_context->io_context.wsabuf.len = MAX_BUFFER_SIZE;

		socket_context->resource.hFile_resource = INVALID_HANDLE_VALUE;

		if ( bIsListen == false && use_ssl == true )
		{
			DWORD protocol = 0;
			switch ( ssl_version )
			{
				case 4:	protocol |= SP_PROT_TLS1_2;
				case 3:	protocol |= SP_PROT_TLS1_1;
				case 2:	protocol |= SP_PROT_TLS1;
				case 1:	protocol |= SP_PROT_SSL3;
				case 0:	{ if ( ssl_version < 4 ) { protocol |= SP_PROT_SSL2; } }
			}

			SSL *ssl = SSL_new( protocol );
			if ( ssl == NULL )
			{
				GlobalFree( socket_context );
				socket_context = NULL;

				return NULL;
			}

			ssl->s = sd;

			socket_context->ssl = ssl;
		}

		InitializeCriticalSection( &socket_context->write_cs );

		g_hIOCP = CreateIoCompletionPort( ( HANDLE )sd, g_hIOCP, ( DWORD_PTR )socket_context, 0 );
		if ( g_hIOCP == NULL )
		{
			if ( socket_context->ssl != NULL )
			{
				SSL_free( socket_context->ssl );
				socket_context->ssl = NULL;
			}

			GlobalFree( socket_context );
			socket_context = NULL;
		}
		else
		{
			// Add all socket contexts (except the listening one) to our linked list.
			if ( bIsListen == false )
			{
				socket_context->context_node.data = socket_context;

				EnterCriticalSection( &context_list_cs );

				EnterCriticalSection( &close_connection_cs );

				// This must be done in a critical section, otherwise the listview will break.
				AddConnectionInfo( socket_context );

				LeaveCriticalSection( &close_connection_cs );

				++total_clients;

				DLL_AddNode( &client_context_list, &socket_context->context_node, 0 );

				LeaveCriticalSection( &context_list_cs );
			}
		}
	}

	return socket_context;
}

void CloseClient( SOCKET_CONTEXT *socket_context, bool bGraceful )
{
	if ( socket_context != NULL )
	{
		EnterCriticalSection( &context_list_cs );

		DLL_RemoveNode( &client_context_list, &socket_context->context_node );

		--total_clients;

		EnterCriticalSection( &close_connection_cs );

		// This must be done in a critical section, otherwise the listview will break.
		RemoveConnectionInfo( socket_context );

		LeaveCriticalSection( &close_connection_cs );

		LeaveCriticalSection( &context_list_cs );

		if ( bGraceful == false )
		{
			// Force the subsequent closesocket to be abortive.
			LINGER  lingerStruct;

			lingerStruct.l_onoff = 1;
			lingerStruct.l_linger = 0;
			_setsockopt( socket_context->Socket, SOL_SOCKET, SO_LINGER, ( char * )&lingerStruct, sizeof( lingerStruct ) );
		}

		if ( socket_context->Socket != INVALID_SOCKET )
		{
			_shutdown( socket_context->Socket, SD_BOTH );
			_closesocket( socket_context->Socket );
			socket_context->Socket = INVALID_SOCKET;
		}

		// Go through our update buffer states to see if any were in use before we closed the client.
		// If it is, then decrement its equivalent update buffer pool index.

		while ( socket_context->io_context.update_buffer_state != NULL )
		{
			DoublyLinkedList *del_node = socket_context->io_context.update_buffer_state;
			socket_context->io_context.update_buffer_state = socket_context->io_context.update_buffer_state->next;

			if ( ( ( UPDATE_BUFFER_STATE * )del_node->data )->in_use )
			{
				InterlockedDecrement( ( ( UPDATE_BUFFER_STATE * )del_node->data )->count );
			}

			GlobalFree( ( UPDATE_BUFFER * )del_node->data );
			GlobalFree( del_node );
		}

		if ( socket_context->ssl != NULL )
		{
			SSL_free( socket_context->ssl );
			socket_context->ssl = NULL;
		}

		if ( socket_context->resource.resource_buf != NULL )
		{
			if ( socket_context->resource.use_cache == false )
			{
				GlobalFree( socket_context->resource.resource_buf );
			}
			socket_context->resource.resource_buf = NULL;
		}

		if ( socket_context->resource.websocket_upgrade_key != NULL )
		{
			GlobalFree( socket_context->resource.websocket_upgrade_key );
			socket_context->resource.websocket_upgrade_key = NULL;
		}

		if ( socket_context->resource.hFile_resource != INVALID_HANDLE_VALUE )
		{
			CloseHandle( socket_context->resource.hFile_resource );
			socket_context->resource.hFile_resource = INVALID_HANDLE_VALUE;
		}

		while ( socket_context->list_data != NULL )
		{
			DoublyLinkedList *del_node = socket_context->list_data;
			socket_context->list_data = socket_context->list_data->next;

			GlobalFree( ( ( PAYLOAD_INFO * )del_node->data )->payload_value );
			GlobalFree( ( PAYLOAD_INFO * )del_node->data );
			GlobalFree( del_node );
		}

		DeleteCriticalSection( &socket_context->write_cs );

		GlobalFree( socket_context );
	}

	return;    
}

// Free all context structure in the global list of context structures.
void FreeClientContexts()
{
	DoublyLinkedList *context_node = client_context_list;
	DoublyLinkedList *del_context_node = NULL;

	while ( context_node != NULL )
	{
		del_context_node = context_node;
		context_node = context_node->next;

		CloseClient( ( SOCKET_CONTEXT * )del_context_node->data, false );
	}

	client_context_list = NULL;

	total_clients = 0;

	return;
}

void FreeListenContext()
{
	if ( listen_context != NULL )
	{
		if ( listen_context->Socket != INVALID_SOCKET )
		{
			_shutdown( listen_context->Socket, SD_BOTH );
			_closesocket( listen_context->Socket );
			listen_context->Socket = INVALID_SOCKET;
		}

		GlobalFree( listen_context );
		listen_context = NULL;
	}

	return;
}
