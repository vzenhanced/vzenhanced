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

#include "globals.h"

#include "file_operations.h"
#include "utilities.h"

#include <process.h>

#include "connection.h"

#include "lite_shell32.h"
#include "lite_advapi32.h"
#include "lite_crypt32.h"
#include "lite_user32.h"

#include "menus.h"

bool in_server_thread = false;

bool g_bEndServer = false;			// set to TRUE on CTRL-C
bool g_bRestart = true;				// set to TRUE to CTRL-BRK
HANDLE g_hIOCP = INVALID_HANDLE_VALUE;
SOCKET g_sdListen = INVALID_SOCKET;

WSAEVENT g_hCleanupEvent[ 1 ];

DoublyLinkedList *client_context_list = NULL;
DoublyLinkedList *listen_context = NULL;

CRITICAL_SECTION g_CriticalSection;		// Guard access to the global context list

PCCERT_CONTEXT g_pCertContext = NULL;

bool use_ssl = false;
unsigned char ssl_version = 2;
bool use_authentication = false;
bool verify_origin = false;

wchar_t *document_root_directory = NULL;
int document_root_directory_length = 0;

HANDLE ping_semaphore = NULL;

char *g_domain = NULL;
unsigned short g_port = 80;

char *GetUTF8Domain( wchar_t *domain )
{
	int domain_length = WideCharToMultiByte( CP_UTF8, 0, domain, -1, NULL, 0, NULL, NULL );
	char *utf8_domain = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * domain_length ); // Size includes the null character.
	WideCharToMultiByte( CP_UTF8, 0, domain, -1, utf8_domain, domain_length, NULL, NULL );

	return utf8_domain;
}

void AddConnectionInfo( SOCKET_CONTEXT *socket_context )
{
	CONNECTION_INFO *ci = ( CONNECTION_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CONNECTION_INFO ) );
	ci->rx_bytes = 0;
	ci->tx_bytes = 0;

	ci->psc = socket_context;

	// Remote
	struct sockaddr_in addr;
	_memzero( &addr, sizeof( sockaddr_in ) );

	socklen_t len = sizeof( sockaddr_in );
	_getpeername( socket_context->Socket, ( struct sockaddr * )&addr, &len );

	addr.sin_family = AF_INET;

	USHORT port = _ntohs( addr.sin_port );
	addr.sin_port = 0;

	DWORD ip_length = sizeof( ci->r_ip );
	_WSAAddressToStringW( ( SOCKADDR * )&addr, sizeof( struct sockaddr_in ), NULL, ci->r_ip, &ip_length );

	__snwprintf( ci->r_port, sizeof( ci->r_port ), L"%hu", port );

	_GetNameInfoW( ( SOCKADDR * )&addr, sizeof( struct sockaddr_in ), ( PWCHAR )&ci->r_host_name, NI_MAXHOST, NULL, 0, 0 );

	// Local
	_memzero( &addr, sizeof( sockaddr_in ) );

	len = sizeof( sockaddr_in );
	_getsockname( socket_context->Socket, ( struct sockaddr * )&addr, &len );

	port = _ntohs( addr.sin_port );
	addr.sin_port = 0;

	ip_length = sizeof( ci->l_ip );
	_WSAAddressToStringW( ( SOCKADDR * )&addr, sizeof( struct sockaddr_in ), NULL, ci->l_ip, &ip_length );

	__snwprintf( ci->l_port, sizeof( ci->l_port ), L"%hu", port );

	_GetNameInfoW( ( SOCKADDR * )&addr, sizeof( struct sockaddr_in ), ( PWCHAR )&ci->l_host_name, NI_MAXHOST, NULL, 0, 0 );

	socket_context->ci = ci;

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
	lvi.iSubItem = 0;
	lvi.iItem = _SendMessageW( g_hWnd_connections, LVM_GETITEMCOUNT, 0, 0 );
	lvi.lParam = ( LPARAM )ci;	// lParam = our contactinfo structure from the connection thread.
	_SendNotifyMessageW( g_hWnd_connections, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
}

void RemoveConnectionInfo( SOCKET_CONTEXT *socket_context )
{
	if ( socket_context != NULL && socket_context->ci != NULL )
	{
		skip_connections_draw = true;

		LVFINDINFO lvfi;
		_memzero( &lvfi, sizeof( LVFINDINFO ) );
		lvfi.flags = LVFI_PARAM;
		lvfi.lParam = ( LPARAM )socket_context->ci;

		int index = _SendMessageW( g_hWnd_connections, LVM_FINDITEM, -1, ( LPARAM )&lvfi );

		if ( index != -1 )
		{
			_SendMessageW( g_hWnd_connections, LVM_DELETEITEM, index, 0 );
		}

		GlobalFree( socket_context->ci );
		socket_context->ci = NULL;

		skip_connections_draw = false;
	}
}

bool TrySend( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, IO_OPERATION current_operation, IO_OPERATION next_operation )
{
	int nRet = 0;
	DWORD dwFlags = 0;

	bool sent = true;
	socket_context->ref_count++;

	socket_context->io_context->LastIOOperation = current_operation;
	socket_context->io_context->IOOperation = next_operation;

	if ( use_ssl == true )
	{
		nRet = SSL_WSASend( socket_context, &( socket_context->io_context->wsabuf ), lpWSAOverlapped, sent );

		if ( nRet != SEC_E_OK )
		{
			sent = false;
		}
	}
	else
	{
		nRet = _WSASend( socket_context->Socket, &( socket_context->io_context->wsabuf ), 1, NULL, dwFlags, lpWSAOverlapped, NULL );
	
		if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
		{
			sent = false;
		}
	}

	if ( sent == false )
	{
		socket_context->ref_count--;
	}

	return sent;
}

bool TryReceive( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, IO_OPERATION current_operation, IO_OPERATION next_operation )
{
	int nRet = 0;
	DWORD dwFlags = 0;

	bool sent = true;

	socket_context->io_context->nTotalBytes = 0;
	socket_context->io_context->nSentBytes = 0;
	socket_context->io_context->wsabuf.buf = socket_context->io_context->buffer,
	socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE;

	
	socket_context->ref_count++;

	socket_context->io_context->LastIOOperation = current_operation;
	socket_context->io_context->IOOperation = next_operation;

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
		nRet = _WSARecv( socket_context->Socket, &( socket_context->io_context->wsabuf ), 1, NULL, &dwFlags, lpWSAOverlapped, NULL );
	
		if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
		{
			sent = false;
		}
	}

	if ( sent == false )
	{
		socket_context->ref_count--;
	}

	return sent;
}

void BeginClose( SOCKET_CONTEXT *socket_context, IO_OPERATION IOOperation )
{
	if ( socket_context != NULL )
	{
		socket_context->io_context->nTotalBytes = 1;
		socket_context->ref_count++;
		socket_context->io_context->IOOperation = IOOperation;
		PostQueuedCompletionStatus( g_hIOCP, 1, ( ULONG_PTR )socket_context->dll, &( socket_context->io_context->overlapped ) );
	}
}

bool VerifyOrigin( char *origin, int origin_length )
{
	char *origin_position = origin;

	// Check the protocol.
	if ( use_ssl == true )
	{
		if ( _memcmp( origin_position, "https://", 8 ) != 0 )
		{
			return false;
		}
		else
		{
			origin_position += 8;
		}
	}
	else
	{
		if ( _memcmp( origin_position, "http://", 7 ) != 0 )
		{
			return false;
		}
		else
		{
			origin_position += 7;
		}
	}

	// Now check the domain and its port (if it's non-default).

	char *has_colon = _StrChrA( origin_position, ':' );

	int local_length = lstrlenA( g_domain );

	if ( has_colon == NULL )
	{
		// Make sure the domain is using default ports.
		if ( ( use_ssl == false && g_port == 80 ) || ( use_ssl == true && g_port == 443 ) )
		{
			int origin_string_length = ( origin_length - ( origin_position - origin ) );

			// Compare the domains.
			if ( local_length != origin_string_length || _memcmp( g_domain, origin_position, origin_string_length ) != 0 )
			{
				return false;
			}
		}
		else	// If we're not using default ports, then the origin is wrong.
		{
			return false;
		}
	}
	else	// Handle domains with non-default ports.
	{
		int origin_string_length = ( has_colon - origin_position );

		// Compare the domains.
		if ( local_length != origin_string_length || _memcmp( g_domain, origin_position, origin_string_length ) != 0 )
		{
			return false;
		}

		origin_position = has_colon + 1;

		// Compare the ports.
		char cport[ 6 ];
		local_length = __snprintf( cport, 6, "%hu", g_port );
		origin_string_length = origin_length - ( origin_position - origin );
		if ( local_length != origin_string_length || _memcmp( cport, origin_position, origin_string_length ) != 0 )
		{
			return false;
		}
	}

	return true;
}

bool RequestAuthentication( SOCKET_CONTEXT *socket_context )
{
	int reply_buf_length = 0;

	char *header_start = socket_context->io_context->wsabuf.buf;

	socket_context->io_context->wsabuf.buf = socket_context->io_context->buffer;
	socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE;

	socket_context->io_context->IOOperation = ClientIoShutdown;

	bool auth_passed = false;

	char *end_of_header = fields_tolower( header_start );
	if ( end_of_header != NULL )
	{
		char *auth = _StrStrA( header_start, "authorization:" );
		if ( auth != NULL )
		{
			auth = _StrStrA( auth, "Basic " );
			if ( auth != NULL )
			{
				auth = auth + 6;
				char *auth_end = _StrStrA( auth, "\r\n" );
				if ( auth_end != NULL )
				{
					int key_length = ( auth_end - auth );

					if ( key_length == encoded_authentication_length && encoded_authentication != NULL && _memcmp( auth, encoded_authentication, encoded_authentication_length ) == 0 )
					{
						auth_passed = true;
					}

					/*char *key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( key_length + 1 ) );
					_memcpy_s( key, ( key_length + 1 ), auth, key_length );
					key[ key_length ] = 0;	// Sanity.

					char *decoded_key = NULL;
					DWORD dwLength = 0;
					_CryptStringToBinaryA( ( LPCSTR )key, key_length, CRYPT_STRING_BASE64, ( BYTE * )decoded_key, &dwLength, NULL, NULL );

					decoded_key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( dwLength + 1 ) );
					_CryptStringToBinaryA( ( LPCSTR )key, key_length, CRYPT_STRING_BASE64, ( BYTE * )decoded_key, &dwLength, NULL, NULL );
					decoded_key[ dwLength ] = 0; // Sanity.

					GlobalFree( key );

					if ( dwLength == 7 && _memcmp( decoded_key, "abc:123", 7 ) == 0 )
					{
						auth_passed = true;
					}*/
				}
			}
		}
	}

	if ( auth_passed == false )
	{
		reply_buf_length = __snprintf( socket_context->io_context->wsabuf.buf, MAX_BUFF_SIZE,
				"HTTP/1.1 401 Unauthorized\r\n" \
				"Content-Type: text/html; charset=UTF-8\r\n" \
				"WWW-Authenticate: Basic realm=\"Authorization Required\"\r\n" \
				"Content-Length: 110\r\n" \
				"Connection: close\r\n\r\n" \
				"<!DOCTYPE html><html><head><title>401 Unauthorized</title></head><body><h1>401 Unauthorized</h1></body></html>" );
	
		socket_context->io_context->nTotalBytes = reply_buf_length;
		socket_context->io_context->nSentBytes  = 0;
		socket_context->io_context->wsabuf.len  = reply_buf_length;

		bool ret = TrySend( socket_context, &( socket_context->io_context->overlapped ), socket_context->io_context->LastIOOperation, ClientIoShutdown );
		if ( ret == false )
		{
			BeginClose( socket_context );
		}
	}

	return auth_passed;
}

bool SwitchProtocols( SOCKET_CONTEXT *socket_context )
{
	bool protocol_switched = false;

	char *header_start = socket_context->io_context->wsabuf.buf;

	socket_context->io_context->wsabuf.buf = socket_context->io_context->buffer;
	socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE;

	char *end_of_header = fields_tolower( header_start );
	if ( end_of_header != NULL )
	{
		char *find_origin = _StrStrA( header_start, "origin:" );
		if ( find_origin != NULL )
		{
			find_origin += 7;

			// Skip whitespace that could appear after the ":", but before the field value.
			while ( find_origin < end_of_header )
			{
				if ( find_origin[ 0 ] != ' ' && find_origin[ 0 ] != '\t' )
				{
					break;
				}

				++find_origin;
			}

			char *find_origin_end = _StrStrA( find_origin, "\r\n" );

			// Skip whitespace that could appear before the "\r\n", but after the field value.
			while ( ( find_origin_end - 1 ) >= find_origin )
			{
				if ( *( find_origin_end - 1 ) != ' ' && *( find_origin_end - 1 ) != '\t' )
				{
					break;
				}

				--find_origin_end;
			}

			// Verify the origin.
			if ( verify_origin == true )
			{
				char tmp_end = *find_origin_end;
				*find_origin_end = 0;	// Sanity

				bool verified = VerifyOrigin( find_origin, ( find_origin_end - find_origin ) );

				*find_origin_end = tmp_end;	// Restore.

				
				if ( verified == false )
				{
					BeginClose( socket_context, ClientIoShutdown );

					return false;
				}
			}

			char *find_key = _StrStrA( header_start, "sec-websocket-key:" );
			if ( find_key != NULL )
			{
				find_key += 18;

				// Skip whitespace that could appear after the ":", but before the field value.
				while ( find_key < end_of_header )
				{
					if ( find_key[ 0 ] != ' ' && find_key[ 0 ] != '\t' )
					{
						break;
					}

					++find_key;
				}

				char *find_key_end = _StrStrA( find_key, "\r\n" );

				// Skip whitespace that could appear before the "\r\n", but after the field value.
				while ( ( find_key_end - 1 ) >= find_key )
				{
					if ( *( find_key_end - 1 ) != ' ' && *( find_key_end - 1 ) != '\t' )
					{
						break;
					}

					--find_key_end;
				}

				int key_length = ( find_key_end - find_key );
				int total_key_length = key_length + 36;
				char *key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( total_key_length + 1 ) );
				_memcpy_s( key, ( total_key_length + 1 ), find_key, key_length );
				_memcpy_s( key + key_length, ( total_key_length + 1 ) - key_length, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36 );
				key[ total_key_length ] = 0;	// Sanity.
				
				HCRYPTHASH hHash = NULL;
				HCRYPTPROV hProvider = NULL;
				_CryptAcquireContextW( &hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT );
				_CryptCreateHash( hProvider, CALG_SHA1, NULL, 0, &hHash );
				_CryptHashData( hHash, ( BYTE * )key, total_key_length, 0 );

				DWORD sha1_value_length, dwByteCount = sizeof( DWORD );
				_CryptGetHashParam( hHash, HP_HASHSIZE, ( BYTE * )&sha1_value_length, &dwByteCount, 0 );

				char *sha1_value = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( sha1_value_length + 1 ) );

				_CryptGetHashParam( hHash, HP_HASHVAL, ( BYTE * )sha1_value, &sha1_value_length, 0 );
				sha1_value[ sha1_value_length ] = 0; // Sanity.

				char *encoded_key = NULL;
				DWORD dwLength = 0;
				_CryptBinaryToStringA( ( BYTE * )sha1_value, sha1_value_length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &dwLength );

				encoded_key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * dwLength );
				_CryptBinaryToStringA( ( BYTE * )sha1_value, sha1_value_length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, ( LPSTR )encoded_key, &dwLength );
				encoded_key[ dwLength ] = 0; // Sanity.

				int reply_buf_length = __snprintf( socket_context->io_context->wsabuf.buf, MAX_BUFF_SIZE,
								"HTTP/1.1 101 Switching Protocols\r\n" \
								"Upgrade: websocket\r\n" \
								"Connection: Upgrade\r\n" \
								"Sec-WebSocket-Accept: %s\r\n\r\n", encoded_key );

				GlobalFree( encoded_key );
				GlobalFree( sha1_value );
				_CryptDestroyHash( hHash );
				_CryptReleaseContext( hProvider, 0 );
				GlobalFree( key );

				protocol_switched = true;

				socket_context->connection_type = CON_TYPE_WEBSOCKET;	// Websocket connection type.

				socket_context->io_context->nTotalBytes = reply_buf_length;
				socket_context->io_context->nSentBytes  = 0;
				socket_context->io_context->wsabuf.len  = reply_buf_length;

				bool ret = TrySend( socket_context, &( socket_context->io_context->overlapped ), ClientIoWrite, ClientIoReadMore );
			
				if ( ret == false )
				{
					BeginClose( socket_context );
				}
			}
			else	// No WebSocket key was sent.
			{
				if ( use_authentication == true )
				{
					// Offset the header past the first line.
					// We've already looked at it and it's not important for authentication.
					socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE - ( header_start - socket_context->io_context->wsabuf.buf );
					socket_context->io_context->wsabuf.buf = header_start;

					if ( RequestAuthentication( socket_context ) == false )
					{
						return false;
					}
					else
					{
						socket_context->io_context->wsabuf.buf = socket_context->io_context->buffer;
						socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE;
					}
				}
			}
		}
		else	// No Origin header exists for the WebSocket.
		{
			if ( use_authentication == true )
			{
				// Offset the header past the first line.
				// We've already looked at it and it's not important for authentication.
				socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE - ( header_start - socket_context->io_context->wsabuf.buf );
				socket_context->io_context->wsabuf.buf = header_start;

				if ( RequestAuthentication( socket_context ) == false )
				{
					return false;
				}
				else
				{
					socket_context->io_context->wsabuf.buf = socket_context->io_context->buffer;
					socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE;
				}
			}
		}
	}

	if ( protocol_switched == false )
	{
		int reply_buf_length = __snprintf( socket_context->io_context->wsabuf.buf, MAX_BUFF_SIZE,
				"HTTP/1.1 404 Not Found\r\n" \
				"Content-Type: text/html\r\n" \
				"Content-Length: 104\r\n" \
				"Connection: close\r\n\r\n" \
				"<!DOCTYPE html><html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>" );

		socket_context->io_context->nTotalBytes = reply_buf_length;
		socket_context->io_context->nSentBytes  = 0;
		socket_context->io_context->wsabuf.len  = reply_buf_length;

		bool ret = TrySend( socket_context, &( socket_context->io_context->overlapped ), socket_context->io_context->LastIOOperation, ClientIoShutdown );
		if ( ret == false )
		{
			BeginClose( socket_context );
		}
	}

	return protocol_switched;
}

void SendResource( SOCKET_CONTEXT *socket_context )
{
	unsigned char request_type = 0;	// 0 = Normal file, 1 = Not found, 2 = Switch Protocols (WebSocket)
	unsigned int reply_buf_length = 0;

	socket_context->io_context->IOOperation = ClientIoWrite;

	socket_context->io_context->wsabuf.buf = socket_context->io_context->buffer;
	socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE;

	if ( socket_context->resource.hFile_resource == INVALID_HANDLE_VALUE && socket_context->resource.use_cache == false )
	{
		/* Chrome doesn't send an Authorization header with a websocket upgrade. Until it does, keep this commented out.
		if ( use_authentication == true )
		{
			if ( RequestAuthentication( socket_context ) == false )
			{
				return;
			}
		}*/

		// First line of the HTTP header. Method Resource Protocol (GET /index.html HTTP/1.1)
		char *line_end = _StrStrA( socket_context->io_context->wsabuf.buf, "\r\n" );
		if ( line_end != NULL )
		{
			char *end = line_end;
			*end = 0;
			char *begin = _StrChrA( socket_context->io_context->wsabuf.buf, '/' );
			if ( begin != NULL )
			{
				begin++;
				end = _StrChrA( begin, ' ' );

				if ( end != NULL )
				{
					*end = 0;

					unsigned int dec_len = 0;
					char *decoded_url = url_decode( begin, end - begin, &dec_len );

					char *bad_directory_1 = _StrStrA( decoded_url, "./" );
					char *bad_directory_2 = _StrStrA( decoded_url, ".\\" );
					if ( bad_directory_1 == NULL && bad_directory_2 == NULL )
					{
						int path_length = MultiByteToWideChar( CP_UTF8, 0, decoded_url, -1, NULL, 0 ) + ( document_root_directory_length + 1 + 10 + 1 );	// Add an extra space for a trailing '\'
						wchar_t *w_resource = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * path_length );

						_memcpy_s( w_resource, sizeof( wchar_t ) * path_length, document_root_directory, sizeof( wchar_t ) * document_root_directory_length );
						_memcpy_s( w_resource + document_root_directory_length, ( sizeof( wchar_t ) * path_length ) - ( sizeof( wchar_t ) * document_root_directory_length ), L"\\", sizeof( wchar_t ) );

						MultiByteToWideChar( CP_UTF8, 0, decoded_url, -1, w_resource + ( document_root_directory_length + 1 ), path_length - ( document_root_directory_length + 1 ) - 10 );

						// See if path is a file or folder on our system.
						DWORD fa = GetFileAttributesW( w_resource );
						if ( fa != INVALID_FILE_ATTRIBUTES )
						{
							// Authorization workaround for Chrome.
							if ( use_authentication == true )
							{
								// Offset the header past the first line.
								// We've already looked at it and it's not important for authentication.
								socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE - ( ( line_end + 2 ) - socket_context->io_context->wsabuf.buf );
								socket_context->io_context->wsabuf.buf = line_end + 2;

								if ( RequestAuthentication( socket_context ) == false )
								{
									GlobalFree( w_resource );
									GlobalFree( decoded_url );

									return;
								}
								else
								{
									socket_context->io_context->wsabuf.buf = socket_context->io_context->buffer;
									socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE;
								}
							}

							// If the resource is a folder, then send an index file if it exists.
							if ( ( fa & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
							{
								if ( index_file_buf != NULL && dec_len == 0 )
								{
									socket_context->resource.use_cache = true;
								}
								else
								{
									// See if the directory ends with a slash.
									if ( dec_len > 0 && ( *( decoded_url + ( dec_len - 1 ) ) != '/' && *( decoded_url + ( dec_len - 1 ) ) != '\\' ) )
									{
										*( w_resource + ( path_length - 12 ) ) = L'\\';	// Add the slash if there isn't one.
									}
									else	// Decrease the path length if there's already a slash.
									{
										--path_length;
									}

									_memcpy_s( w_resource + ( path_length - 11 ), sizeof( wchar_t ) * 11, L"index.html\0", sizeof( wchar_t ) * 11 );
									socket_context->resource.hFile_resource = CreateFile( w_resource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
									if ( socket_context->resource.hFile_resource == INVALID_HANDLE_VALUE )
									{
										*( w_resource + ( path_length - 2 ) ) = 0;
										//_memcpy_s( w_resource + ( path_length - 11 ), sizeof( wchar_t ) * 11, L"index.htm\0", sizeof( wchar_t ) * 10 );
										socket_context->resource.hFile_resource = CreateFile( w_resource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
									}
								}
							}
							else	// Otherwise, the resource is a file.
							{
								socket_context->resource.hFile_resource = CreateFile( w_resource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
							}

							// If we were able to open the file or the resource was cached, then send the header first.
							if ( socket_context->resource.hFile_resource != INVALID_HANDLE_VALUE || socket_context->resource.use_cache == true )
							{
								if ( socket_context->resource.hFile_resource != INVALID_HANDLE_VALUE )
								{
									socket_context->resource.file_size = GetFileSize( socket_context->resource.hFile_resource, NULL );

									socket_context->resource.resource_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 524288 );

									ReadFile( socket_context->resource.hFile_resource, socket_context->resource.resource_buf, 524288, &( socket_context->resource.resource_buf_size ), NULL );
								}
								else	// socket_context->resource.use_cache == true
								{
									socket_context->resource.file_size = index_file_buf_length;

									socket_context->resource.resource_buf_size = index_file_buf_length;
								}

								reply_buf_length = __snprintf( socket_context->io_context->wsabuf.buf, MAX_BUFF_SIZE,
									"HTTP/1.1 200 OK\r\n" \
									"Content-Type: %s\r\n" \
									"Content-Length: %d\r\n" \
									"Connection: close\r\n\r\n", GetMIMEByFileName( decoded_url ), socket_context->resource.file_size );
							}
							else
							{
								request_type = 1;
							}
						}
						else if ( dec_len == 7 && _memcmp( decoded_url, "connect", 7 ) == 0 )
						{
							// Offset the header past the first line.
							// We've already looked at it and it's not important for switching protocols.
							socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE - ( ( line_end + 2 ) - socket_context->io_context->wsabuf.buf );
							socket_context->io_context->wsabuf.buf = line_end + 2;
							request_type = 2;
						}
						else
						{
							request_type = 1;
						}

						GlobalFree( w_resource );
					}
					else
					{
						request_type = 1;
					}

					GlobalFree( decoded_url );
				}
				else
				{
					request_type = 1;
				}
			}
			else
			{
				request_type = 1;
			}
		}
		else
		{
			request_type = 1;
		}
	}

	switch ( request_type )
	{
		case 0:	// Normal file.
		{
			if ( socket_context->resource.total_read < socket_context->resource.file_size )
			{
				if ( socket_context->resource.resource_buf_offset == socket_context->resource.resource_buf_size )
				{
					unsigned int rem = min( MAX_BUFF_SIZE - reply_buf_length, socket_context->resource.resource_buf_size );
					if ( socket_context->resource.use_cache == true )
					{
						_memcpy_s( socket_context->io_context->wsabuf.buf + reply_buf_length, MAX_BUFF_SIZE - reply_buf_length, index_file_buf, rem );
					}
					else
					{
						ReadFile( socket_context->resource.hFile_resource, socket_context->resource.resource_buf, 524288, &( socket_context->resource.resource_buf_size ), NULL );

						_memcpy_s( socket_context->io_context->wsabuf.buf + reply_buf_length, MAX_BUFF_SIZE - reply_buf_length, socket_context->resource.resource_buf, rem );
					}
					reply_buf_length += rem;
					socket_context->resource.resource_buf_offset = rem;

					socket_context->resource.total_read += rem;
				}
				else
				{
					int rem = min( MAX_BUFF_SIZE - reply_buf_length, socket_context->resource.resource_buf_size - socket_context->resource.resource_buf_offset );

					if ( socket_context->resource.use_cache == true )
					{
						_memcpy_s( socket_context->io_context->wsabuf.buf + reply_buf_length, MAX_BUFF_SIZE - reply_buf_length, index_file_buf + socket_context->resource.resource_buf_offset, rem );
					}
					else
					{
						_memcpy_s( socket_context->io_context->wsabuf.buf + reply_buf_length, MAX_BUFF_SIZE - reply_buf_length, socket_context->resource.resource_buf + socket_context->resource.resource_buf_offset, rem );
					}

					reply_buf_length += rem;
					socket_context->resource.resource_buf_offset += rem;

					socket_context->resource.total_read += rem;
				}
			}
		}
		break;

		case 1:	// File Not Found.
		{
			reply_buf_length = __snprintf( socket_context->io_context->wsabuf.buf, MAX_BUFF_SIZE,
									"HTTP/1.1 404 Not Found\r\n" \
									"Content-Type: text/html\r\n" \
									"Content-Length: 104\r\n" \
									"Connection: close\r\n\r\n" \
									"<!DOCTYPE html><html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>" );
		}
		break;

		case 2:	// Switch Protocols (WebSocket)
		{
			SwitchProtocols( socket_context );

			return;
		}
		break;

		/*case 3:	// Forbidden.
		{
			reply_buf_length = __snprintf( socket_context->io_context->wsabuf.buf, MAX_BUFF_SIZE,
									"HTTP/1.1 403 Forbidden\r\n" \
									"Content-Type: text/html\r\n" \
									"Content-Length: 104\r\n" \
									"Connection: close\r\n\r\n" \
									"<!DOCTYPE html><html><head><title>403 Forbidden</title></head><body><h1>403 Forbidden</h1></body></html>" );
		}
		break;*/
	}

	int nRet = 0;
	DWORD dwSendNumBytes = 0;
	DWORD dwFlags = 0;

	socket_context->io_context->nTotalBytes = reply_buf_length;
	socket_context->io_context->nSentBytes  = 0;
	socket_context->io_context->wsabuf.len  = reply_buf_length;
	dwFlags = 0;

	if ( request_type == 1 )
	{
		bool ret = TrySend( socket_context, &( socket_context->io_context->overlapped ), socket_context->io_context->LastIOOperation, ClientIoShutdown );
		if ( ret == false )
		{
			BeginClose( socket_context );
		}
	}
	else if ( request_type == 0 && reply_buf_length > 0 )
	{
		bool ret = TrySend( socket_context, &( socket_context->io_context->overlapped ), socket_context->io_context->LastIOOperation, ClientIoWrite );
		if ( ret == false )
		{
			BeginClose( socket_context );
		}
	}
	else
	{
		CloseHandle( socket_context->resource.hFile_resource );
		socket_context->resource.hFile_resource = INVALID_HANDLE_VALUE;

		socket_context->resource.resource_buf_size = 0;
		socket_context->resource.total_read = 0;
		socket_context->resource.file_size = 0;
		socket_context->resource.resource_buf_offset = 0;

		if ( socket_context->resource.resource_buf != NULL )
		{
			GlobalFree( socket_context->resource.resource_buf );
			socket_context->resource.resource_buf = NULL;
		}

		BeginClose( socket_context, ClientIoShutdown );
	}
}

void SendRedirect( SOCKET_CONTEXT *socket_context )
{
	char *resource = NULL;
	if ( socket_context->ssl->pbIoBuffer != NULL )
	{
		socket_context->ssl->pbIoBuffer[ socket_context->ssl->cbIoBuffer ] = 0;

		char *resource_end = _StrStrA( ( PSTR )socket_context->ssl->pbIoBuffer, "\r\n" );
		if ( resource_end != NULL )
		{
			char *resource_start = _StrChrA( ( PSTR )socket_context->ssl->pbIoBuffer, '/' );
			if ( resource_start != NULL )
			{
				if ( *( resource_start + 1 ) == '/' )
				{
					resource_start++;
					resource_start = _StrChrA( resource_start + 1, '/' );
				}

				if ( resource_start + 1 != NULL )
				{
					resource_end = _StrChrA( resource_start + 1, ' ' );
					if ( resource_end != NULL )
					{
						int resource_length = ( ( resource_end - resource_start ) + 1 );	// Include the NULL character.
						resource = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * resource_length );
						_memcpy_s( resource, resource_length, resource_start, resource_length - 1 );
						resource[ resource_length - 1 ] = 0;	// Sanity.
					}
				}
			}
		}
	}

	int reply_buf_length = __snprintf( socket_context->io_context->wsabuf.buf, MAX_BUFF_SIZE,
				"HTTP/1.1 301 Moved Permanently\r\n" \
				"Location: https://%s:%hu%s\r\n"
				"Content-Type: text/html\r\n" \
				"Content-Length: 120\r\n" \
				"Connection: close\r\n\r\n" \
				"<!DOCTYPE html><html><head><title>301 Moved Permanently</title></head><body><h1>301 Moved Permanently</h1></body></html>", g_domain, g_port, SAFESTRA( resource ) );

	if ( resource != NULL )
	{
		GlobalFree( resource );
		resource = NULL;
	}

	DWORD dwFlags = 0;

	socket_context->io_context->LastIOOperation = ClientIoHandshakeReply;
	socket_context->io_context->IOOperation = ClientIoClose;	// This is closed because the SSL connection was never established. An SSL shutdown would just fail.

	socket_context->ref_count++;

	socket_context->io_context->nTotalBytes = reply_buf_length;
	socket_context->io_context->nSentBytes = 0;
	socket_context->io_context->wsabuf.len = reply_buf_length;

	int nRet = _WSASend( socket_context->Socket, &( socket_context->io_context->wsabuf ), 1, NULL, dwFlags, &( socket_context->io_context->overlapped ), NULL );

	if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
	{
		socket_context->ref_count--;
		BeginClose( socket_context );
	}
}

int WriteIgnoreList( SOCKET_CONTEXT *socket_context )
{
	char *json_buf = socket_context->io_context->wsabuf.buf + 10;	// 10 byte offset so that we can encode the websocket frame header.
	int json_buf_length = 0;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "{\"IGNORE_LIST\":[", 16 );
	json_buf_length += 16;

	node_type *node = NULL;

	if ( socket_context->ignore_node == NULL )
	{
		node = dllrbt_get_head( ignore_list );
	}
	else
	{
		node = socket_context->ignore_node;
	}

	while ( node != NULL )
	{
		ignoreinfo *ii = ( ignoreinfo * )node->val;

		int cfg_val_length = lstrlenA( SAFESTRA( ii->c_phone_number ) );

		if ( json_buf_length + cfg_val_length + 13 + 10 + 2 + 1 >= ( MAX_BUFF_SIZE - 10 ) )		// 13 for the json, 10 for the int count, 2 for "]}".
		{
			--json_buf_length;

			break;
		}

		json_buf_length += __snprintf( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "{\"N\":\"%s\",\"C\":%lu}", SAFESTRA( ii->c_phone_number ), ii->count );
		node = node->next;

		if ( node != NULL )
		{
			json_buf[ json_buf_length++ ] = ',';
		}
	}

	socket_context->ignore_node = node;


	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "]}", 2 );
	json_buf_length += 2;

	// Websocket payload size. Adjust the start of our buffer if the payload is less than the given amounts.
	if ( json_buf_length <= 125 )
	{
		socket_context->io_context->wsabuf.buf += 8;
	}
	else if ( json_buf_length <= 65535 )
	{
		socket_context->io_context->wsabuf.buf += 6;
	}

	return ConstructFrameHeader( socket_context->io_context->wsabuf.buf, MAX_BUFF_SIZE, WS_BINARY_FRAME, /*json_buf,*/ json_buf_length ) + json_buf_length;
}

int WriteForwardList( SOCKET_CONTEXT *socket_context )
{
	char *json_buf = socket_context->io_context->wsabuf.buf + 10;	// 10 byte offset so that we can encode the websocket frame header.
	int json_buf_length = 0;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "{\"FORWARD_LIST\":[", 17 );
	json_buf_length += 17;

	node_type *node = NULL;

	if ( socket_context->forward_node == NULL )
	{
		node = dllrbt_get_head( forward_list );
	}
	else
	{
		node = socket_context->forward_node;
	}

	while ( node != NULL )
	{
		forwardinfo *fi = ( forwardinfo * )node->val;

		int cfg_val_length = lstrlenA( SAFESTRA( fi->c_call_from ) );
		cfg_val_length += lstrlenA( SAFESTRA( fi->c_forward_to ) );

		if ( json_buf_length + cfg_val_length + 20 + 10 + 2 + 1 >= ( MAX_BUFF_SIZE - 10 ) )	// 20 for the json, 10 for the int count, 2 for "]}".
		{
			--json_buf_length;

			break;
		}

		json_buf_length += __snprintf( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "{\"F\":\"%s\",\"T\":\"%s\",\"C\":%lu}", SAFESTRA( fi->c_call_from ), SAFESTRA( fi->c_forward_to ), fi->count );
		node = node->next;

		if ( node != NULL )
		{
			json_buf[ json_buf_length++ ] = ',';
		}
	}

	socket_context->forward_node = node;


	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "]}", 2 );
	json_buf_length += 2;

	// Websocket payload size. Adjust the start of our buffer if the payload is less than the given amounts.
	if ( json_buf_length <= 125 )
	{
		socket_context->io_context->wsabuf.buf += 8;
	}
	else if ( json_buf_length <= 65535 )
	{
		socket_context->io_context->wsabuf.buf += 6;
	}

	return ConstructFrameHeader( socket_context->io_context->wsabuf.buf, MAX_BUFF_SIZE, WS_BINARY_FRAME, /*json_buf,*/ json_buf_length ) + json_buf_length;
}

int WriteContactList( SOCKET_CONTEXT *socket_context )
{
	char *json_buf = socket_context->io_context->wsabuf.buf + 10;	// 10 byte offset so that we can encode the websocket frame header.
	int json_buf_length = 0;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "{\"CONTACT_LIST\":[", 17 );
	json_buf_length += 17;

	node_type *node = NULL;

	if ( socket_context->contact_node == NULL )
	{
		node = dllrbt_get_head( contact_list );
	}
	else
	{
		node = socket_context->contact_node;
	}

	while ( node != NULL )
	{
		contactinfo *ci = ( contactinfo * )node->val;

		int cfg_val_length = lstrlenA( SAFESTRA( ci->contact.home_phone_number ) );
		cfg_val_length += lstrlenA( SAFESTRA( ci->contact.cell_phone_number ) );

		cfg_val_length += lstrlenA( SAFESTRA( ci->contact.first_name ) );
		cfg_val_length += lstrlenA( SAFESTRA( ci->contact.last_name ) );

		if ( json_buf_length + cfg_val_length + 29 + 2 + 1 >= ( MAX_BUFF_SIZE - 10 ) )	// 29 for the json, 2 for "]}".
		{
			--json_buf_length;

			break;
		}

		json_buf_length += __snprintf( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "{\"F\":\"%s\",\"L\":\"%s\",\"H\":\"%s\",\"C\":\"%s\"}", SAFESTRA( ci->contact.first_name ), SAFESTRA( ci->contact.last_name ), SAFESTRA( ci->contact.home_phone_number ), SAFESTRA( ci->contact.cell_phone_number ) );
		node = node->next;

		if ( node != NULL )
		{
			json_buf[ json_buf_length++ ] = ',';
		}
	}

	socket_context->contact_node = node;


	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "]}", 2 );
	json_buf_length += 2;

	// Websocket payload size. Adjust the start of our buffer if the payload is less than the given amounts.
	if ( json_buf_length <= 125 )
	{
		socket_context->io_context->wsabuf.buf += 8;
	}
	else if ( json_buf_length <= 65535 )
	{
		socket_context->io_context->wsabuf.buf += 6;
	}

	return ConstructFrameHeader( socket_context->io_context->wsabuf.buf, MAX_BUFF_SIZE, WS_BINARY_FRAME, /*json_buf,*/ json_buf_length ) + json_buf_length;
}

int WriteCallLog( SOCKET_CONTEXT *socket_context )
{
	char *json_buf = socket_context->io_context->wsabuf.buf + 10;	// 10 byte offset so that we can encode the websocket frame header.
	int json_buf_length = 0;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "{\"CALL_LOG\":[", 13 );
	json_buf_length += 13;

	node_type *node = NULL;

	if ( socket_context->call_node == NULL )
	{
		node = dllrbt_get_head( call_log );
	}
	else
	{
		node = socket_context->call_node;	// Start from the last node if we broke the packet up.
	}

	DoublyLinkedList *dll = NULL;

	bool exit_loop = false;
	while ( node != NULL )
	{
		if ( socket_context->call_list != NULL )
		{
			dll = socket_context->call_list;
			socket_context->call_list = NULL;	// Reset the last list node.
		}
		else
		{
			dll = ( DoublyLinkedList * )node->val;
		}

		while ( dll != NULL )
		{
			displayinfo *di = ( displayinfo * )dll->data;

			int cfg_val_length = lstrlenA( SAFESTRA( di->ci.caller_id ) );

			unsigned int ec_length = 0;
			char *escaped_caller_id = json_escape( SAFESTRA( di->ci.caller_id ), cfg_val_length, &ec_length );

			if ( escaped_caller_id != NULL )
			{
				cfg_val_length = ec_length;
			}

			cfg_val_length += lstrlenA( SAFESTRA( di->ci.call_from ) );


			LARGE_INTEGER date;
			date.HighPart = di->time.HighPart;
			date.LowPart = di->time.LowPart;

			date.QuadPart -= ( 11644473600000 * 10000 );

			// Divide the 64bit value.
			__asm
			{
				xor edx, edx;				//; Zero out the register so we don't divide a full 64bit value.
				mov eax, date.HighPart;		//; We'll divide the high order bits first.
				mov ecx, FILETIME_TICKS_PER_SECOND;
				div ecx;
				mov date.HighPart, eax;		//; Store the high order quotient.
				mov eax, date.LowPart;		//; Now we'll divide the low order bits.
				div ecx;
				mov date.LowPart, eax;		//; Store the low order quotient.
				//; Any remainder will be stored in edx. We're not interested in it though.
			}

			if ( json_buf_length + cfg_val_length + 20 + 2 + 30 + 2 + 1 >= ( MAX_BUFF_SIZE - 10 ) )	// 20 for timestamp, 2 for Ignore and Forward, 30 for the json, 2 for "]}".
			{
				--json_buf_length;

				if ( escaped_caller_id != NULL )
				{
					GlobalFree( escaped_caller_id );
					escaped_caller_id = NULL;
				}

				exit_loop = true;
				break;
			}

			json_buf_length += __snprintf( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "{\"C\":\"%s\",\"N\":\"%s\",\"T\":%lld,\"I\":%c,\"F\":%c}", ( escaped_caller_id != NULL ? escaped_caller_id : SAFESTRA( di->ci.caller_id ) ), SAFESTRA( di->ci.call_from ), date.QuadPart, ( di->ci.ignore == true ? '1' : '0' ), ( di->ci.forward == true ? '1' : '0' ) );

			if ( escaped_caller_id != NULL )
			{
				GlobalFree( escaped_caller_id );
				escaped_caller_id = NULL;
			}

			dll = dll->next;

			if ( dll != NULL || node->next != NULL )
			{
				json_buf[ json_buf_length++ ] = ',';
			}
		}

		if ( exit_loop == true )
		{
			break;
		}

		node = node->next;
	}

	// Remember our last nodes so we can continue from the last position if we broke the data up.
	socket_context->call_list = dll;
	socket_context->call_node = node;


	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "]}", 2 );
	json_buf_length += 2;

	// Websocket payload size. Adjust the start of our buffer if the payload is less than the given amounts.
	if ( json_buf_length <= 125 )
	{
		socket_context->io_context->wsabuf.buf += 8;
	}
	else if ( json_buf_length <= 65535 )
	{
		socket_context->io_context->wsabuf.buf += 6;
	}

	return ConstructFrameHeader( socket_context->io_context->wsabuf.buf, MAX_BUFF_SIZE, WS_BINARY_FRAME, /*json_buf,*/ json_buf_length ) + json_buf_length;
}

THREAD_RETURN SendCallLogUpdate( LPVOID pArg )
{
	// Make sure this assignment is done before we enter the critical section.
	displayinfo *di = ( displayinfo * )pArg;

	char *buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * MAX_BUFF_SIZE );
	char *pBuf = buffer;

	char *json_buf = pBuf + 10;	// 10 byte offset so that we can encode the websocket frame header.
	int json_buf_length = 0;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "{\"CALL_LOG_UPDATE\":[", 20 );
	json_buf_length += 20;

	int cfg_val_length = lstrlenA( SAFESTRA( di->ci.caller_id ) );

	unsigned int ec_length = 0;
	char *escaped_caller_id = json_escape( SAFESTRA( di->ci.caller_id ), cfg_val_length, &ec_length );

	if ( escaped_caller_id != NULL )
	{
		cfg_val_length = ec_length;
	}

	cfg_val_length += lstrlenA( SAFESTRA( di->ci.call_from ) );
	cfg_val_length += lstrlenA( SAFESTRA( di->ci.call_reference_id ) );


	LARGE_INTEGER date;
	date.HighPart = di->time.HighPart;
	date.LowPart = di->time.LowPart;

	date.QuadPart -= ( 11644473600000 * 10000 );

	// Divide the 64bit value.
	__asm
	{
		xor edx, edx;				//; Zero out the register so we don't divide a full 64bit value.
		mov eax, date.HighPart;		//; We'll divide the high order bits first.
		mov ecx, FILETIME_TICKS_PER_SECOND;
		div ecx;
		mov date.HighPart, eax;		//; Store the high order quotient.
		mov eax, date.LowPart;		//; Now we'll divide the low order bits.
		div ecx;
		mov date.LowPart, eax;		//; Store the low order quotient.
		//; Any remainder will be stored in edx. We're not interested in it though.
	}

	if ( json_buf_length + cfg_val_length + 20 + 27 + 2 + 1 >= ( MAX_BUFF_SIZE - 10 ) )	// 20 for timestamp, 27 for the json, 2 for "]}".
	{
		if ( escaped_caller_id != NULL )
		{
			GlobalFree( escaped_caller_id );
			escaped_caller_id = NULL;
		}

		GlobalFree( buffer );

		ExitThread( 0 );
		return 0;
	}

	json_buf_length += __snprintf( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "{\"C\":\"%s\",\"N\":\"%s\",\"T\":%lld,\"R\":\"%s\"}", ( escaped_caller_id != NULL ? escaped_caller_id : SAFESTRA( di->ci.caller_id ) ), SAFESTRA( di->ci.call_from ), date.QuadPart, SAFESTRA( di->ci.call_reference_id ) );

	if ( escaped_caller_id != NULL )
	{
		GlobalFree( escaped_caller_id );
		escaped_caller_id = NULL;
	}

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFF_SIZE - 10 ) - json_buf_length, "]}", 2 );
	json_buf_length += 2;

	// Websocket payload size. Adjust the start of our buffer if the payload is less than the given amounts.
	if ( json_buf_length <= 125 )
	{
		pBuf += 8;
	}
	else if ( json_buf_length <= 65535 )
	{
		pBuf += 6;
	}

	int reply_buf_length = ConstructFrameHeader( pBuf, MAX_BUFF_SIZE, WS_BINARY_FRAME, /*json_buf,*/ json_buf_length ) + json_buf_length;

	EnterCriticalSection( &g_CriticalSection );

	DoublyLinkedList *dll = client_context_list;

	SOCKET_CONTEXT *socket_context = NULL;

	WSABUF wsa_update;

	while ( dll != NULL )
	{
		if ( g_bEndServer == true )
		{
			break;
		}

		socket_context = ( SOCKET_CONTEXT * )dll->data;

		if ( socket_context->connection_type == CON_TYPE_WEBSOCKET )
		{
			socket_context->ping_sent = 0;	// Reset the ping count. We'll consider this a ping.

			int nRet = 0;
			DWORD dwFlags = 0;

			bool sent = true;
			socket_context->ref_count++;

			wsa_update.buf = pBuf;
			wsa_update.len = reply_buf_length;

			socket_context->io_context->nTotalBytes = reply_buf_length;
			socket_context->io_context->nSentBytes = 0;

			socket_context->io_context->IOOperation = ClientIoWebSocket;
			socket_context->io_context->LastIOOperation = ClientIoWebSocket;

			if ( use_ssl == true )
			{
				nRet = SSL_WSASend( socket_context, &wsa_update, &( socket_context->io_context->update_overlapped ), sent );

				if ( nRet != SEC_E_OK )
				{
					sent = false;
				}
			}
			else
			{
				nRet = _WSASend( socket_context->Socket, &wsa_update, 1, NULL, dwFlags, &( socket_context->io_context->update_overlapped ), NULL );
			
				if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
				{
					sent = false;
				}
			}

			if ( sent == false )
			{
				socket_context->ref_count--;
				BeginClose( socket_context );
			}
			else	// This must be done so that we don't free buffer before all contexts' IO operations have been sent. Only call this when WSAGetLastError() == ERROR_IO_PENDING
			{
				// Wait for the overlap IO to complete. Spares us from having to copy to the context buffer.
				while ( HasOverlappedIoCompleted( &( socket_context->io_context->update_overlapped ) ) == FALSE )
				{
					if ( g_bEndServer == true )
					{
						break;
					}

					Sleep( 0 );
				}
			}
		}

		dll = dll->next;
	}

	LeaveCriticalSection( &g_CriticalSection );

	GlobalFree( buffer );

	ExitThread( 0 );
	return 0;
}


void SendPing( SOCKET_CONTEXT *socket_context )
{
	EnterCriticalSection( &g_CriticalSection );

	int nRet = 0;
	DWORD dwFlags = 0;

	bool sent = true;
	socket_context->ref_count++;

	static WSABUF wsa_ping;
	static char ping[ 2 ];
	ping[ 0 ] = -119;	// 0x89; Ping byte
	ping[ 1 ] = 0;
	wsa_ping.len = 2;	// ConstructFrameHeader( ping, 2, WS_PING, /*NULL,*/ 0 );
	wsa_ping.buf = ping;

	socket_context->io_context->nTotalBytes = 2;
	socket_context->io_context->nSentBytes = 0;

	socket_context->io_context->IOOperation = ClientIoWebSocket;

	if ( use_ssl == true )
	{
		nRet = SSL_WSASend( socket_context, &wsa_ping, &( socket_context->io_context->ping_overlapped ), sent );

		if ( nRet != SEC_E_OK )
		{
			sent = false;
		}
	}
	else
	{
		nRet = _WSASend( socket_context->Socket, &wsa_ping, 1, NULL, dwFlags, &( socket_context->io_context->ping_overlapped ), NULL );
	
		if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
		{
			sent = false;
		}
	}

	if ( sent == false )
	{
		socket_context->ref_count--;
		BeginClose( socket_context );
	}

	LeaveCriticalSection( &g_CriticalSection );
}

bool WriteListData( SOCKET_CONTEXT *socket_context, bool do_read )
{
	node_type *node = NULL;

	EnterCriticalSection( &g_CriticalSection );

	socket_context->io_context->wsabuf.buf = socket_context->io_context->buffer;
	socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE;

	int reply_buf_length = 0;

	EnterCriticalSection( list_cs );

	switch ( socket_context->list_type )
	{
		case OVERLAP_CALL_LOG:
		{
			reply_buf_length = WriteCallLog( socket_context );

			node = socket_context->call_node;
		}
		break;

		case OVERLAP_CONTACT_LIST:
		{
			reply_buf_length = WriteContactList( socket_context );

			node = socket_context->contact_node;
		}
		break;

		case OVERLAP_FORWARD_LIST:
		{
			reply_buf_length = WriteForwardList( socket_context );

			node = socket_context->forward_node;
		}
		break;

		case OVERLAP_IGNORE_LIST:
		{
			reply_buf_length = WriteIgnoreList( socket_context );

			node = socket_context->ignore_node;
		}
		break;
	}

	LeaveCriticalSection( list_cs );

	socket_context->io_context->IOOperation = ClientIoWrite;

	// Determine whether we need to read more data, or not.
	if ( node == NULL )
	{
		if ( do_read == true )
		{
			socket_context->io_context->IOOperation = ClientIoReadMore;
		}
	}

	socket_context->io_context->nTotalBytes = reply_buf_length;
	socket_context->io_context->nSentBytes = 0;
	socket_context->io_context->wsabuf.len = reply_buf_length;

	bool ret = TrySend( socket_context, &( socket_context->io_context->overlapped ), ClientIoWrite, socket_context->io_context->IOOperation );

	if ( ret == false )
	{
		BeginClose( socket_context );
	}

	LeaveCriticalSection( &g_CriticalSection );

	return ( node != NULL ? false : true );
}



void SendListData( SOCKET_CONTEXT *socket_context )
{
	EnterCriticalSection( &g_CriticalSection );

	DoublyLinkedList *dll = socket_context->list_data;

	WS_OPCODE opcode = WS_CONTINUATION_FRAME;	// Placeholder for now.

	OVERLAP_TYPE ot = OVERLAP_PING;

	bool do_write = true;
	bool do_read = false;

	if ( dll != NULL )
	{
		PAYLOAD_INFO *pi = ( PAYLOAD_INFO * )dll->data;

		if ( pi->opcode == WS_PONG )
		{
			socket_context->ping_sent = 0;	// Reset the ping counter.

			do_read = true;
			do_write = false;
		}
		else if ( pi->opcode == WS_TEXT_FRAME )
		{
			char *payload_value = pi->payload_value;
			int payload_value_length = lstrlenA( payload_value );

			if ( ignore_list != NULL && payload_value_length >= 15 && _memcmp( payload_value, "GET_IGNORE_LIST", 15 ) == 0 )
			{
				ot = OVERLAP_IGNORE_LIST;
			}
			else if ( forward_list != NULL && payload_value_length >= 16 && _memcmp( payload_value, "GET_FORWARD_LIST", 16 ) == 0 )
			{
				ot = OVERLAP_FORWARD_LIST;
			}
			else if ( contact_list != NULL && payload_value_length >= 16 && _memcmp( payload_value, "GET_CONTACT_LIST", 16 ) == 0 )
			{
				ot = OVERLAP_CONTACT_LIST;
			}
			else if ( call_log != NULL && payload_value_length >= 12 && _memcmp( payload_value, "GET_CALL_LOG", 12 ) == 0 )
			{
				ot = OVERLAP_CALL_LOG;
			}
			else if ( payload_value_length >= 16 && _memcmp( payload_value, "IGNORE_INCOMING:", 16 ) == 0 )
			{
				do_read = true;
				do_write = false;

				char *from = _StrChrA( payload_value + 16, ':' );
				if ( from != NULL )
				{
					char *reference = from + 1;
					int reference_length = ( payload_value + payload_value_length ) - reference;

					*from = 0;
					from = payload_value + 16;

					EnterCriticalSection( list_cs );

					DoublyLinkedList *dll_w = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )from, true );
					if ( dll_w != NULL )
					{
						if ( dll_w->prev != NULL && dll_w->prev->data != NULL )
						{
							displayinfo *di = ( displayinfo * )dll_w->prev->data;

							// Verify that the reference value is the same.
							if ( _memcmp( di->ci.call_reference_id, reference, reference_length ) == 0 )
							{
								( ( pWebIgnoreIncomingCall )WebIgnoreIncomingCall )( di );
							}
						}
						else if ( dll_w->data != NULL )	// The incoming call is the only item in the list.
						{
							displayinfo *di = ( displayinfo * )dll_w->data;

							// Verify that the reference value is the same.
							if ( _memcmp( di->ci.call_reference_id, reference, reference_length ) == 0 )
							{
								( ( pWebIgnoreIncomingCall )WebIgnoreIncomingCall )( di );
							}
						}
					}

					LeaveCriticalSection( list_cs );
				}
			}
			else if ( payload_value_length >= 17 && _memcmp( payload_value, "FORWARD_INCOMING:", 17 ) == 0 )
			{
				do_read = true;
				do_write = false;

				char *from = _StrChrA( payload_value + 17, ':' );
				if ( from != NULL )
				{
					char *to = from + 1;

					char *reference = _StrChrA( to, ':' );
					if ( reference != NULL )
					{
						int to_length = ( reference - to );

						*reference = 0;
						reference++;

						int reference_length = ( payload_value + payload_value_length ) - reference;

						*from = 0;
						from = payload_value + 17;

						EnterCriticalSection( list_cs );

						DoublyLinkedList *dll_w = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )from, true );
						if ( dll_w != NULL )
						{
							if ( dll_w->prev != NULL && dll_w->prev->data != NULL )
							{
								displayinfo *di = ( displayinfo * )dll_w->prev->data;

								// Verify that the reference value is the same.
								if ( _memcmp( di->ci.call_reference_id, reference, reference_length ) == 0 )
								{
									char *forward_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( to_length + 1 ) );
									_memcpy_s( forward_to, to_length + 1, to, to_length + 1 );
									forward_to[ to_length ] = 0;	// Sanity.

									if ( ( ( pWebForwardIncomingCall )WebForwardIncomingCall )( di, forward_to ) == false )
									{
										GlobalFree( forward_to );
									}
								}
							}
							else if ( dll_w->data != NULL )	// The incoming call is the only item in the list.
							{
								displayinfo *di = ( displayinfo * )dll_w->data;

								// Verify that the reference value is the same.
								if ( _memcmp( di->ci.call_reference_id, reference, reference_length ) == 0 )
								{
									char *forward_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( to_length + 1 ) );
									_memcpy_s( forward_to, to_length + 1, to, to_length + 1 );
									forward_to[ to_length ] = 0;	// Sanity.

									if ( ( ( pWebForwardIncomingCall )WebForwardIncomingCall )( di, forward_to ) == false )
									{
										GlobalFree( forward_to );
									}
								}
							}
						}

						LeaveCriticalSection( list_cs );
					}
				}
			}
			else	// Post another read for any unknown text code.
			{
				do_read = true;
				do_write = false;
			}
		}
		else if ( pi->opcode == WS_CONNECTION_CLOSE )
		{
			do_read = false;
			do_write = false;

			socket_context->io_context->wsabuf.buf = socket_context->io_context->buffer;
			//socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE;

			//int reply_buf_length = ConstructFrameHeader( socket_context->io_context->wsabuf.buf, MAX_BUFF_SIZE, WS_CONNECTION_CLOSE, /*NULL,*/ 0 );

			socket_context->io_context->wsabuf.buf[ 0 ] = -120;	//0x88;	Close byte
			socket_context->io_context->wsabuf.buf[ 1 ] = 0;

			socket_context->io_context->nTotalBytes = 2;//reply_buf_length;
			socket_context->io_context->nSentBytes  = 0;
			socket_context->io_context->wsabuf.len  = 2;//reply_buf_length;

			socket_context->connection_type = CON_TYPE_HTTP;

			bool ret = TrySend( socket_context, &( socket_context->io_context->overlapped ), socket_context->io_context->LastIOOperation, ClientIoShutdown );

			if ( ret == false )
			{
				BeginClose( socket_context );
			}
		}
		else	// Post another read for any unknown opcode.
		{
			do_read = true;
			do_write = false;
		}

		bool remove_node = true;
		if ( do_write == true )
		{
			socket_context->list_type = ot;

			remove_node = WriteListData( socket_context, ( dll->next == NULL ? true : false ) );
		}
		else if ( do_read == true )
		{
			socket_context->io_context->LastIOOperation = ClientIoWrite;
			socket_context->io_context->IOOperation = ( dll->next == NULL ? ClientIoReadMore : ClientIoWrite );

			socket_context->io_context->nTotalBytes = 0;
			socket_context->ref_count++;
			PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )socket_context->dll, &( socket_context->io_context->overlapped ) );
		}

		if ( remove_node == true )
		{
			DoublyLinkedList *del_node = dll;
			dll = dll->next;

			GlobalFree( ( ( PAYLOAD_INFO * )del_node->data )->payload_value );
			GlobalFree( ( PAYLOAD_INFO * )del_node->data );
			GlobalFree( del_node );


			socket_context->list_data = dll;
		}
	}

	LeaveCriticalSection( &g_CriticalSection );
}

void ParseWebsocketInfo( SOCKET_CONTEXT *socket_context )
{
	char *payload_buf = socket_context->io_context->wsabuf.buf;
	DWORD decoded_size = socket_context->io_context->wsabuf.len;

	char *payload = NULL;
	unsigned __int64 payload_length = 0;
	WS_OPCODE opcode = WS_CONTINUATION_FRAME;	// Placeholder.

	DoublyLinkedList *q_dll = NULL;

	// It's possible for these requests to get queued in the buffer.
	// We should process them all, and when done, post a single read operation (if required).
	do
	{
		payload_buf = DeconstructFrame( payload_buf, ( DWORD * )&decoded_size, opcode, &payload, payload_length );

		PAYLOAD_INFO *pi = ( PAYLOAD_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( PAYLOAD_INFO ) );

		pi->payload_value = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( ( SIZE_T )payload_length + 1 ) );
		_memcpy_s( pi->payload_value, ( size_t )payload_length, payload, ( size_t )payload_length );
		pi->payload_value[ payload_length ] = 0;	// Sanity.

		pi->opcode = opcode;

		DoublyLinkedList *node = DLL_CreateNode( pi );
		DLL_AddNode( &q_dll, node, -1 );
	}
	while ( payload_buf != NULL );

	EnterCriticalSection( &g_CriticalSection );

	// Add the requests to the end of our list.
	DLL_AddNode( &( socket_context->list_data ), q_dll, -1 );
	
	LeaveCriticalSection( &g_CriticalSection );

	SendListData( socket_context );
}

// Stale connections will be closed in 1 minute.
// WebSocket connections will be closed if a ping request fails.
DWORD WINAPI Poll( LPVOID WorkThreadContext )
{
	while ( true )
	{
		WaitForSingleObject( ping_semaphore, 30000 );

		if ( g_bEndServer == true )
		{
			ExitThread( 0 );
			return 0;
		}

		EnterCriticalSection( &g_CriticalSection );

		DoublyLinkedList *dll = client_context_list;

		while ( dll != NULL )
		{
			if ( g_bEndServer == true )
			{
				break;
			}

			SOCKET_CONTEXT *socket_context = ( SOCKET_CONTEXT * )dll->data;

			if ( socket_context->connection_type == CON_TYPE_WEBSOCKET )
			{
				socket_context->ping_sent++;

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
				socket_context->timeout++;

				if ( socket_context->timeout >= 2 )
				{
					BeginClose( socket_context );
				}
			}

			dll = dll->next;
		}

		LeaveCriticalSection( &g_CriticalSection );
	}

	// We should not get here.
	ExitThread( 0 );
	return 0;

}

bool DecodeRecv( SOCKET_CONTEXT *socket_context, DWORD &dwIoSize, HANDLE hIOCP, DoublyLinkedList *dll )
{
	WSABUF wsa_decode;

	DWORD bytes_decoded = 0;
	DWORD dwRecvNumBytes = 0;
	DWORD dwFlags = 0;

	bool skip_send = false;

	if ( socket_context->ssl->rd.scRet == SEC_E_INCOMPLETE_MESSAGE )
	{
		socket_context->ssl->cbIoBuffer += dwIoSize;
	}
	else
	{
		socket_context->ssl->cbIoBuffer = dwIoSize;
	}

	dwIoSize = 0;

	socket_context->io_context->wsabuf.buf[ socket_context->ssl->cbIoBuffer ] = 0;	// Sanity.
	socket_context->io_context->wsabuf.len = socket_context->ssl->cbIoBuffer;	

	wsa_decode.buf = socket_context->io_context->wsabuf.buf + socket_context->io_context->nBufferOffset;
	wsa_decode.len = socket_context->ssl->cbIoBuffer;

	// Decode our message.
	while ( socket_context->ssl->pbIoBuffer[ 0 ] != 0 && socket_context->ssl->cbIoBuffer > 0 )
	{
		SECURITY_STATUS sRet = SSL_WSARecv_Decode( socket_context->ssl, &wsa_decode, bytes_decoded );

		switch ( sRet )
		{
			// We've successfully decoded a portion of the message.
			case SEC_E_OK:
			case SEC_I_CONTINUE_NEEDED:
			{
				if ( bytes_decoded <= 0 )
				{
					bytes_decoded = 0;
					socket_context->ssl->cbIoBuffer = 0;
				}

				dwIoSize += bytes_decoded;

				wsa_decode.buf += bytes_decoded;

				if ( sRet == SEC_E_OK )
				{
					wsa_decode.len = socket_context->ssl->cbIoBuffer;
				}
				else	// Set the length to the remaining data.
				{
					wsa_decode.len = socket_context->ssl->cbRecDataBuf;
					socket_context->ssl->cbIoBuffer = socket_context->ssl->cbRecDataBuf;
				}
			}
			break;

			// The message was incomplete. Request more data from the client.
			case SEC_E_INCOMPLETE_MESSAGE:
			{
				skip_send = true;

				bool ret = TryReceive( socket_context, &( socket_context->io_context->overlapped ), ClientIoRead, socket_context->io_context->IOOperation );
				if ( ret == false )
				{
					BeginClose( socket_context );
				}
			}
			break;

			// Client wants us to perform another handshake.
			case SEC_I_RENEGOTIATE:
			{
				skip_send = true;

				bool sent = false;
				socket_context->io_context->IOOperation = ClientIoHandshakeReply;
				socket_context->ref_count++;
				SSL_WSAAccept( socket_context, &( socket_context->io_context->overlapped ), sent );

				if ( sent == false )
				{
					socket_context->ref_count--;
				}
			}
			break;

			// Client has signaled a shutdown, or some other error occurred.
			//case SEC_I_CONTEXT_EXPIRED:
			default:
			{
				skip_send = true;

				BeginClose( socket_context );
			}
			break;
		}

		if ( skip_send == true )
		{
			socket_context->ssl->cbIoBuffer = 0;
			return false;
		}
	}

	socket_context->ssl->cbIoBuffer = 0;
	socket_context->ssl->pbIoBuffer[ 0 ] = 0;

	return true;
}

THREAD_RETURN Server( LPVOID pArguments )
{
	DWORD dwThreadCount = 0;
	int nRet = 0;

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

	while ( g_bRestart == true )
	{
		g_bRestart = false;
		g_bEndServer = false;

		use_ssl = cfg_enable_ssl;
		ssl_version = cfg_ssl_version;
		use_authentication = cfg_use_authentication;
		verify_origin = cfg_verify_origin;

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

		PreloadIndexFile();

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
			ping_semaphore = NULL;
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

	EnableDisableMenus( false );

	if ( server_semaphore != NULL )
	{
		ReleaseSemaphore( server_semaphore, 1, NULL );
	}

	in_server_thread = false;

	_ExitThread( 0 );
	return 0;
}

SOCKET CreateSocket()
{
	int nRet = 0;
	int nZero = 0;
	SOCKET socket = INVALID_SOCKET;

	socket = _WSASocketW( AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED ); 
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

	struct addrinfoW hints;
	struct addrinfoW *addrlocal = NULL;

	// Resolve the interface
	_memzero( &hints, sizeof( addrinfoW ) );
    hints.ai_flags  = AI_PASSIVE;
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

	// Use Hostname.
	if ( cfg_address_type == 0 )
	{
		g_domain = GetUTF8Domain( cfg_hostname );

		if ( _GetAddrInfoW( cfg_hostname, cport, &hints, &addrlocal ) != 0 )
		{
			ret = false;
			goto CLEANUP;
		}
	}
	else	// Use IP Address.
	{
		struct sockaddr_in src_addr;
		_memzero( &src_addr, sizeof( sockaddr_in ) );

		src_addr.sin_family = AF_INET;
		src_addr.sin_addr.s_addr = _htonl( cfg_ip_address );

		wchar_t wcs_ip[ 16 ];
		DWORD wcs_ip_length = 16;
		_WSAAddressToStringW( ( SOCKADDR * )&src_addr, sizeof( struct sockaddr_in ), NULL, wcs_ip, &wcs_ip_length );

		g_domain = GetUTF8Domain( wcs_ip );

		if ( _GetAddrInfoW( wcs_ip, cport, &hints, &addrlocal) != 0 )
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

	g_sdListen = CreateSocket();
	if ( g_sdListen == INVALID_SOCKET)
	{
		ret = false;
		goto CLEANUP;
	}

    int nRet = _bind( g_sdListen, addrlocal->ai_addr, ( int )addrlocal->ai_addrlen );
	if ( nRet == SOCKET_ERROR)
	{
		ret = false;
		goto CLEANUP;
	}

	nRet = _listen( g_sdListen, 5 );
	if ( nRet == SOCKET_ERROR )
	{
		ret = false;
		goto CLEANUP;
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
	DWORD bytes = 0;

	// GUID to Microsoft specific extensions
	GUID acceptex_guid = WSAID_ACCEPTEX;

	SOCKET_CONTEXT *listen_socket_context = NULL;

	// The listening socket context uses the SocketAccept member to store the socket for client connection. 
	if ( listen_context == NULL )
	{
		listen_context = UpdateCompletionPort( g_sdListen, ClientIoAccept, true );
		if ( listen_context == NULL /*|| listen_context->data == NULL*/ )
		{
			return false;
		}

		listen_socket_context = ( SOCKET_CONTEXT * )listen_context->data;

        // Load the AcceptEx extension function from the provider for this socket.
        nRet = _WSAIoctl( g_sdListen, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof( acceptex_guid ), &listen_socket_context->fnAcceptEx, sizeof( listen_socket_context->fnAcceptEx ), &bytes, NULL, NULL );
        if ( nRet == SOCKET_ERROR )
        {
            return false;
        }
	}
	else
	{
		/*if ( listen_context->data == NULL )
		{
			return false;
		}*/

		listen_socket_context = ( SOCKET_CONTEXT * )listen_context->data;
	}

	listen_socket_context->io_context->SocketAccept = CreateSocket();
	if ( listen_socket_context->io_context->SocketAccept == INVALID_SOCKET )
	{
		return false;
	}

	// Accept a connection without waiting for any data. (dwReceiveDataLength = 0)
	nRet = listen_socket_context->fnAcceptEx( g_sdListen, listen_socket_context->io_context->SocketAccept, ( LPVOID )( listen_socket_context->io_context->buffer ), 0, sizeof( SOCKADDR_STORAGE ) + 16, sizeof( SOCKADDR_STORAGE ) + 16, &dwRecvNumBytes, &( listen_socket_context->io_context->overlapped ) );
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
	SOCKET_CONTEXT *accept_socket_context = NULL;
	DWORD dwFlags = 0;
	DWORD dwIoSize = 0;

	DoublyLinkedList *dll = NULL;

	while ( true )
	{
		// Service io completion packets
		bSuccess = GetQueuedCompletionStatus( hIOCP, &dwIoSize, ( PDWORD_PTR )&dll, ( LPOVERLAPPED * )&overlapped, INFINITE );

		if ( g_bEndServer == true )
		{
			ExitThread( 0 );
			return 0;
		}

		if ( dll == NULL )
		{
			ExitThread( 0 );
			return 0;
		}

		socket_context = ( SOCKET_CONTEXT * )dll->data;

		if ( socket_context == NULL )
		{
			ExitThread( 0 );
			return 0;
		}

		socket_context->timeout = 0;	// Reset timeout counter.

		if ( socket_context->io_context->IOOperation != ClientIoAccept )
		{
			socket_context->io_context->nSentBytes += dwIoSize;

			socket_context->ref_count--;

			// If the completion failed, and we have pending operations, then continue until we have no more in the queue.
			if ( bSuccess == FALSE && socket_context->ref_count > 0 )
			{
				continue;
			}

			// Continue to write any data that hasn't been sent.
			if ( socket_context->io_context->nSentBytes < socket_context->io_context->nTotalBytes )
			{
				bool ret = TrySend( socket_context, overlapped, socket_context->io_context->LastIOOperation, socket_context->io_context->IOOperation );
				if ( ret == false )
				{
					BeginClose( socket_context );
				}

				continue;
			}

			// Process WebSocket data.
			if ( socket_context->connection_type == CON_TYPE_WEBSOCKET )
			{
				if ( overlapped == &( socket_context->io_context->overlapped ) )
				{
					if ( socket_context->io_context->LastIOOperation == ClientIoWrite )	// Set when writing lists.
					{
						socket_context->ci->tx_bytes += dwIoSize;

						// We sent data and need to continue processing more before we read.
						if ( socket_context->io_context->IOOperation == ClientIoWrite )
						{
							SendListData( socket_context );

							continue;
						}
						else if ( socket_context->io_context->IOOperation == ClientIoReadMore )	// We processed all the data we needed to send and now we need to read.
						{
							bool ret = TryReceive( socket_context, &( socket_context->io_context->overlapped ), ClientIoRead, ClientIoWebSocket );
							if ( ret == false )
							{
								BeginClose( socket_context );
							}

							continue;
						}
					}
				}
				else	// Handle ping and call log update completions.
				{
					socket_context->ci->tx_bytes += dwIoSize;

					continue;
				}
			}
			else if ( socket_context->io_context->IOOperation == ClientIoWrite )	// Non-WebSocket resource data.
			{
				socket_context->ci->tx_bytes += dwIoSize;

				SendResource( socket_context );

				continue;
			}
/*
			if ( socket_context->ref_count > 1 )
			{
				int debug_break = 0;
			}
*/
			if ( bSuccess == FALSE || ( bSuccess == TRUE && dwIoSize == 0 ) )
			{
				// The connection was either shutdown by the client, or closed abruptly.

				socket_context->io_context->IOOperation = ClientIoClose;

				if ( socket_context->ref_count > 0 )
				{
					SOCKET s = socket_context->Socket;
					socket_context->Socket = INVALID_SOCKET;
					_closesocket( s );
					continue;
				}
/*
				if ( socket_context->ref_count < 0 )
				{
					int debug_break = 0;
				}
*/
				// Attempt a nice shutdown for our persistent (SSL) connections. connection_type is set to CON_TYPE_HTTP in the shutdown case statement.
				if ( socket_context->connection_type == CON_TYPE_WEBSOCKET && use_ssl == true )
				{
					socket_context->io_context->IOOperation = ClientIoShutdown;
				}
				else	// Everything else will get shutdown, closed, and cleaned up.
				{
					CloseClient( &dll, false );

					continue;
				}
			}
		}

		switch ( socket_context->io_context->IOOperation )
		{
			case ClientIoAccept:
			{
				// Allow the accept socket to inherit the properties of the listen socket.
				int nRet = _setsockopt( socket_context->io_context->SocketAccept, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, ( char * )&g_sdListen, sizeof( g_sdListen ) );
				if ( nRet == SOCKET_ERROR )
				{
					_WSASetEvent( g_hCleanupEvent[ 0 ] );
					ExitThread( 0 );
					return 0;
				}

				DoublyLinkedList *accept_dll = UpdateCompletionPort( socket_context->io_context->SocketAccept, ClientIoAccept, false );

				if ( accept_dll != NULL && accept_dll->data != NULL )
				{
					accept_socket_context = ( SOCKET_CONTEXT * )accept_dll->data;
				}
				else
				{
					_WSASetEvent( g_hCleanupEvent[ 0 ] );
					ExitThread( 0 );
					return 0;
				}

				bool sent = true;

				accept_socket_context->ref_count++;
				accept_socket_context->io_context->LastIOOperation = ClientIoAccept;
				
				if ( use_ssl == true )
				{
					accept_socket_context->io_context->IOOperation = ClientIoHandshakeReply;
					SSL_WSAAccept( accept_socket_context, &( accept_socket_context->io_context->overlapped ), sent );
				}
				else
				{
					accept_socket_context->io_context->IOOperation = ClientIoRead;
					nRet = _WSARecv( accept_socket_context->Socket, &accept_socket_context->io_context->wsabuf, 1, NULL, &dwFlags, &( accept_socket_context->io_context->overlapped ), NULL );

					if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
					{
						sent = false;
					}
				}

				if ( sent == false )
				{
					accept_socket_context->ref_count--;
					BeginClose( accept_socket_context );
				}

				// Check to see if we need to enter the critical section to create the polling thread.
				if ( ping_semaphore == NULL )
				{
					EnterCriticalSection( &g_CriticalSection );

					// Check again if we had threads queue up.
					if ( ping_semaphore == NULL )
					{
						ping_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

						CloseHandle( _CreateThread( NULL, 0, Poll, NULL, 0, NULL ) );
					}

					LeaveCriticalSection( &g_CriticalSection );
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
				socket_context->ci->tx_bytes += dwIoSize;

				socket_context->io_context->wsabuf.buf[ dwIoSize ] = 0;	// Sanity.
				socket_context->io_context->wsabuf.len = dwIoSize;

				socket_context->ssl->cbIoBuffer = dwIoSize;

				socket_context->io_context->nSentBytes = 0;
				socket_context->io_context->nTotalBytes = 0;

				bool sent = false;
				socket_context->ref_count++;
				socket_context->io_context->LastIOOperation = ClientIoHandshakeReply;
				socket_context->io_context->IOOperation = ClientIoHandshakeResponse;
				SECURITY_STATUS sRet = SSL_WSAAccept_Reply( socket_context, &( socket_context->io_context->overlapped ), sent );

				if ( sent == false )
				{
					socket_context->ref_count--;
				}

				if ( sRet == SEC_E_OK )	// If true, then no send or recv was made.
				{
					socket_context->io_context->IOOperation = ClientIoRead;

					if ( socket_context->io_context->LastIOOperation == ClientIoReadMore )
					{
						socket_context->io_context->nTotalBytes = 0;
						socket_context->ref_count++;
						PostQueuedCompletionStatus( g_hIOCP, socket_context->ssl->cbIoBuffer, ( ULONG_PTR )socket_context->dll, &( socket_context->io_context->overlapped ) );
					}
					else
					{
						socket_context->ssl->cbIoBuffer = 0;

						bool ret = TryReceive( socket_context, &( socket_context->io_context->overlapped ), ClientIoRead, socket_context->io_context->IOOperation );
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
				else if ( sRet != SEC_I_CONTINUE_NEEDED && sRet != SEC_I_COMPLETE_AND_CONTINUE && /*sRet != SEC_E_INCOMPLETE_MESSAGE &&*/ sRet != SEC_I_INCOMPLETE_CREDENTIALS )	// Stop handshake and close the connection.
				{
					BeginClose( socket_context );
				}
			}
			break;

			case ClientIoHandshakeResponse:
			{
				socket_context->ci->rx_bytes += dwIoSize;

				socket_context->io_context->wsabuf.buf[ dwIoSize ] = 0;	// Sanity.
				socket_context->io_context->wsabuf.len = dwIoSize;

				socket_context->ssl->cbIoBuffer = dwIoSize;

				socket_context->io_context->nSentBytes = 0;
				socket_context->io_context->nTotalBytes = 0;

				bool sent = false;
				socket_context->ref_count++;
				socket_context->io_context->IOOperation = ClientIoHandshakeReply;
				socket_context->io_context->LastIOOperation = ClientIoHandshakeResponse;

				SECURITY_STATUS sRet = SSL_WSAAccept_Response( socket_context, &( socket_context->io_context->overlapped ), sent );

				if ( sent == false )
				{
					socket_context->ref_count--;
				}

				if ( sRet == SEC_E_OK )	// If true, then no recv was made.
				{
					socket_context->io_context->IOOperation = ClientIoRead;

					if ( socket_context->io_context->LastIOOperation == ClientIoReadMore )
					{
						socket_context->io_context->nTotalBytes = 0;
						socket_context->ref_count++;
						PostQueuedCompletionStatus( g_hIOCP, socket_context->ssl->cbIoBuffer, ( ULONG_PTR )socket_context->dll, &( socket_context->io_context->overlapped ) );
					}
					else
					{
						socket_context->ssl->cbIoBuffer = 0;

						bool ret = TryReceive( socket_context, &( socket_context->io_context->overlapped ), ClientIoRead, socket_context->io_context->IOOperation );
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

			case ClientIoRead:
			{
				socket_context->ci->rx_bytes += dwIoSize;

				bool spawn_thread = false;

				DWORD decoded_size = dwIoSize;

				if ( use_ssl == true )
				{
					if ( DecodeRecv( socket_context, decoded_size, hIOCP, dll ) == false )
					{
						break;
					}
				}

				socket_context->io_context->wsabuf.buf[ ( decoded_size < MAX_BUFF_SIZE ) ? decoded_size : ( MAX_BUFF_SIZE - 1 ) ] = 0;	// Sanity.

				SendResource( socket_context );
			}
			break;

			case ClientIoWebSocket:
			{
				socket_context->ci->rx_bytes += dwIoSize;

				// Reset the pointer if we sent lists that modified it.
				socket_context->io_context->wsabuf.buf = socket_context->io_context->buffer;
				socket_context->io_context->wsabuf.len = MAX_BUFF_SIZE;

				DWORD decoded_size = dwIoSize;

				if ( use_ssl == true )
				{
					//decoded_size = socket_context->ssl->sbIoBuffer;
					if ( DecodeRecv( socket_context, decoded_size, hIOCP, dll ) == false )
					{
						socket_context->io_context->nBufferOffset = 0;
						break;
					}

					socket_context->io_context->nBufferOffset += decoded_size;

					// We need to read more data. The websocket frame is incomplete.
					// Occurs on TSL 1.0 and below for browsers that handle the BEAST attack.
					if ( decoded_size < 2 )
					{
						socket_context->io_context->nTotalBytes = 0;
						socket_context->io_context->nSentBytes = 0;

						socket_context->io_context->LastIOOperation = ClientIoReadMore;
						bool sent = false;
						
						socket_context->ref_count++;
						SSL_WSARecv( socket_context, &( socket_context->io_context->overlapped ), sent );

						if ( sent == false )
						{
							socket_context->ref_count--;

							socket_context->io_context->nBufferOffset = 0;

							BeginClose( socket_context );
						}

						break;
					}

					decoded_size = socket_context->io_context->nBufferOffset;

					socket_context->io_context->nBufferOffset = 0;
				}

				socket_context->io_context->wsabuf.buf[ ( decoded_size < MAX_BUFF_SIZE ) ? decoded_size : ( MAX_BUFF_SIZE - 1 ) ] = 0;	// Sanity.
				socket_context->io_context->wsabuf.len = decoded_size;

				ParseWebsocketInfo( socket_context );
			}
			break;

			case ClientIoShutdown:
			{
				socket_context->ci->tx_bytes += dwIoSize;

				socket_context->io_context->LastIOOperation = ClientIoShutdown;
				socket_context->io_context->IOOperation = ClientIoClose;

				socket_context->connection_type = CON_TYPE_HTTP;

				if ( use_ssl == true )
				{
					bool sent = false;

					socket_context->io_context->nTotalBytes = 0;
					socket_context->io_context->nSentBytes = 0;

					socket_context->ref_count++;
					SECURITY_STATUS sRet = SSL_WSAShutdown( socket_context, &( socket_context->io_context->overlapped ), sent );

					if ( sent == false )
					{
						socket_context->ref_count--;
					}

					if ( sRet != SEC_E_OK )
					{
						socket_context->ref_count++;
						PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )dll, &( socket_context->io_context->overlapped ) );
					}
				}
				else
				{
					socket_context->ref_count++;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )dll, &( socket_context->io_context->overlapped ) );
				}
			}
			break;

			case ClientIoClose:
			{
				socket_context->connection_type = CON_TYPE_HTTP;

				if ( socket_context->ref_count > 0 )
				{
					SOCKET s = socket_context->Socket;
					socket_context->Socket = INVALID_SOCKET;
					_closesocket( s );	// Saves us from having to post if there's already a pending IO operation. Should force the operation to complete.
				}
				else	// Done when no other IO operation is pending.
				{
					socket_context->ref_count++;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )dll, &( socket_context->io_context->overlapped ) );
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
DoublyLinkedList *UpdateCompletionPort( SOCKET sd, IO_OPERATION ClientIo, bool bIsListen )
{
	DoublyLinkedList *dll = NULL;

	EnterCriticalSection( &g_CriticalSection );

	SOCKET_CONTEXT *socket_context = ( SOCKET_CONTEXT * )GlobalAlloc( GPTR, sizeof( SOCKET_CONTEXT ) );
	if ( socket_context )
	{
		socket_context->io_context = ( IO_CONTEXT * )GlobalAlloc( GPTR, sizeof( IO_CONTEXT ) );
		if ( socket_context->io_context )
		{
			socket_context->Socket = sd;

			socket_context->io_context->IOOperation = ClientIo;
			socket_context->io_context->wsabuf.buf  = socket_context->io_context->buffer;
			socket_context->io_context->wsabuf.len  = MAX_BUFF_SIZE;//sizeof( socket_context->io_context->buffer );

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
					GlobalFree( socket_context->io_context );
					socket_context->io_context = NULL;

					GlobalFree( socket_context );
					socket_context = NULL;

					return NULL;
				}

				ssl->s = sd;

				socket_context->ssl = ssl;

			}

			dll = DLL_CreateNode( ( void * )socket_context );

			g_hIOCP = CreateIoCompletionPort( ( HANDLE )sd, g_hIOCP, ( DWORD_PTR )dll, 0 );
			if ( g_hIOCP == NULL )
			{
				if ( use_ssl == true )
				{
					SSL_free( socket_context->ssl );
					socket_context->ssl = NULL;
				}

				GlobalFree( socket_context->io_context );
				socket_context->io_context = NULL;

				GlobalFree( socket_context );
				socket_context = NULL;

				GlobalFree( dll );
				dll = NULL;
			}
			else
			{
				// All all socket contexts (except the listening one) to our linked list.
				if ( bIsListen == false )
				{
					CONNECTION_INFO *ci = ( CONNECTION_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CONNECTION_INFO ) );
					ci->rx_bytes = 0;
					ci->tx_bytes = 0;
					ci->psc = socket_context;

					// Remote
					struct sockaddr_in addr;
					_memzero( &addr, sizeof( sockaddr_in ) );

					socklen_t len = sizeof( sockaddr_in );
					_getpeername( socket_context->Socket, ( struct sockaddr * )&addr, &len );

					addr.sin_family = AF_INET;

					USHORT port = _ntohs( addr.sin_port );
					addr.sin_port = 0;

					DWORD ip_length = sizeof( ci->r_ip );
					_WSAAddressToStringW( ( SOCKADDR * )&addr, sizeof( struct sockaddr_in ), NULL, ci->r_ip, &ip_length );

					__snwprintf( ci->r_port, sizeof( ci->r_port ), L"%hu", port );

					_GetNameInfoW( ( SOCKADDR * )&addr, sizeof( struct sockaddr_in ), ( PWCHAR )&ci->r_host_name, NI_MAXHOST, NULL, 0, 0 );

					// Local
					_memzero( &addr, sizeof( sockaddr_in ) );

					len = sizeof( sockaddr_in );
					_getsockname( socket_context->Socket, ( struct sockaddr * )&addr, &len );

					port = _ntohs( addr.sin_port );
					addr.sin_port = 0;

					ip_length = sizeof( ci->l_ip );
					_WSAAddressToStringW( ( SOCKADDR * )&addr, sizeof( struct sockaddr_in ), NULL, ci->l_ip, &ip_length );

					__snwprintf( ci->l_port, sizeof( ci->l_port ), L"%hu", port );

					_GetNameInfoW( ( SOCKADDR * )&addr, sizeof( struct sockaddr_in ), ( PWCHAR )&ci->l_host_name, NI_MAXHOST, NULL, 0, 0 );

					socket_context->ci = ci;

					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
					lvi.iSubItem = 0;
					lvi.iItem = _SendMessageW( g_hWnd_connections, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )ci;	// lParam = our contactinfo structure from the connection thread.
					_SendMessageW( g_hWnd_connections, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

					socket_context->dll = dll;
					DLL_AddNode( &client_context_list, dll, 0 );
				}
			}
		}
		else
		{
			GlobalFree( socket_context );
			socket_context = NULL;
		}
	}

	LeaveCriticalSection( &g_CriticalSection );

	return dll;
}

void CloseClient( DoublyLinkedList **node, bool bGraceful )
{
	EnterCriticalSection( &g_CriticalSection );

	if ( *node != NULL )
	{
		SOCKET_CONTEXT *socket_context = ( SOCKET_CONTEXT * )( *node )->data;

		if ( socket_context != NULL )
		{
			if ( bGraceful == false )
			{
				// Force the subsequent closesocket to be abortive.
				LINGER  lingerStruct;

				lingerStruct.l_onoff = 1;
				lingerStruct.l_linger = 0;
				_setsockopt( socket_context->Socket, SOL_SOCKET, SO_LINGER, ( char * )&lingerStruct, sizeof( lingerStruct ) );
			}

			_shutdown( socket_context->Socket, SD_BOTH );
			_closesocket( socket_context->Socket );
			socket_context->Socket = INVALID_SOCKET;

			if ( socket_context->io_context != NULL )
			{
				if ( socket_context->io_context->SocketAccept != INVALID_SOCKET )
				{
					_closesocket( socket_context->io_context->SocketAccept );
					socket_context->io_context->SocketAccept = INVALID_SOCKET;
				}

				GlobalFree( socket_context->io_context );
				socket_context->io_context = NULL;
			}

			if ( use_ssl == true )
			{
				SSL_free( socket_context->ssl );
				socket_context->ssl = NULL;
			}

			RemoveConnectionInfo( socket_context );
			socket_context->ci = NULL;

			if ( socket_context->resource.resource_buf != NULL )
			{
				GlobalFree( socket_context->resource.resource_buf );
				socket_context->resource.resource_buf = NULL;
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

			GlobalFree( socket_context );
			socket_context = NULL;
		}

		DLL_RemoveNode( &client_context_list, *node );

		GlobalFree( *node );
		*node = NULL;
	}

	LeaveCriticalSection( &g_CriticalSection );

	return;    
}

// Free all context structure in the global list of context structures.
void FreeClientContexts()
{
	DoublyLinkedList *dll = client_context_list;
	DoublyLinkedList *del_dll = NULL;

	while ( dll != NULL )
	{
		del_dll = dll;
		dll = dll->next;
		CloseClient( &del_dll, false );
	}

	client_context_list = NULL;

	return;
}

void FreeListenContext()
{
	if ( listen_context != NULL )
	{
		SOCKET_CONTEXT *socket_context = ( SOCKET_CONTEXT * )listen_context->data;

		if ( socket_context != NULL )
		{
			if ( socket_context->io_context != NULL )
			{
				if ( socket_context->io_context->SocketAccept != INVALID_SOCKET )
				{
					_shutdown( socket_context->io_context->SocketAccept, SD_BOTH );
					_closesocket( socket_context->io_context->SocketAccept );
					socket_context->io_context->SocketAccept = INVALID_SOCKET;
				}

				GlobalFree( socket_context->io_context );
				socket_context->io_context = NULL;
			}

			GlobalFree( socket_context );
			socket_context = NULL;
		}

		GlobalFree( listen_context );
		listen_context = NULL;
	}

	return;
}
