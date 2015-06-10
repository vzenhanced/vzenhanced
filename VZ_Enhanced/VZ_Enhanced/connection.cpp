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
#include "connection.h"
#include "parsing.h"
#include "menus.h"
#include "utilities.h"
#include "message_log_utilities.h"
#include "list_operations.h"

#include "lite_advapi32.h"
#include "lite_comdlg32.h"
#include "string_tables.h"

// This header value is for the non-standard "App-Name" header field, and is required by the VPNS server.
// Seems it's only needed when registering and requesting account information.
#define APPLICATION_NAME	"VZ-Enhanced-1.0.1.8"
//#define APPLICATION_NAME	"VoiceZone-Air-1.5.0.16"

#define REFERER				"app:/voicezone.html"

#define USER_AGENT			"VZ-Enhanced/1.0.1.8"
//#define USER_AGENT		"Mozilla/5.0 (Windows; U; en-US) AppleWebKit/533.19.4 (KHTML, like Gecko) AdobeAIR/4.0"

#define ORIGIN				"app://"

#define DEFAULT_BUFLEN		8192

#define DEFAULT_PORT		80
#define DEFAULT_PORT_SECURE	443

#define CURRENT_VERSION		1018
#define VERSION_URL			"https://sites.google.com/site/vzenhanced/version.txt"

CRITICAL_SECTION ct_cs;				// Queues additional connection threads.
CRITICAL_SECTION cwt_cs;			// Queues additional connection worker threads.
CRITICAL_SECTION cit_cs;			// Queues additional connection incoming threads.
CRITICAL_SECTION cut_cs;			// Queues additional update check threads.

HANDLE connection_semaphore = NULL;			// Blocks shutdown while the connection thread is active.
HANDLE connection_worker_semaphore = NULL;
HANDLE connection_incoming_semaphore = NULL;
HANDLE update_check_semaphore = NULL;
HANDLE reconnect_semaphore = NULL;

HANDLE select_line_semaphore = NULL;

bool in_connection_thread = false;
bool in_connection_worker_thread = false;
bool in_connection_incoming_thread = false;
bool in_update_check_thread = false;

CONNECTION main_con = { NULL, INVALID_SOCKET, true, LOGGED_OUT };			// Our polling connection. Receives incoming notifications.
CONNECTION worker_con = { NULL, INVALID_SOCKET, true, LOGGED_OUT };			// User initiated server request connection. (Add/Edit/Remove/Import/Export contacts, etc.)
CONNECTION incoming_con = { NULL, INVALID_SOCKET, true, LOGGED_OUT };		// Automatic server request connection. (Ignore/Forward incoming call, Make call)
CONNECTION update_con = { NULL, INVALID_SOCKET, true, CONNECTION_KILL };	// Connection for update checks.

unsigned char contact_update_in_progress = UPDATE_END;	// Idle state
unsigned char contact_import_in_progress = UPDATE_END;	// Idle state

// Shared thread variables.
char *encoded_username = NULL;
char *encoded_password = NULL;

// Cookies.
dllrbt_tree *saml_cookie_tree = NULL;
dllrbt_tree *wayfarer_cookie_tree = NULL;
dllrbt_tree *vpns_cookie_tree = NULL;

char *saml_cookies = NULL;
char *wayfarer_cookies = NULL;
char *vpns_cookies = NULL;

char *client_id = NULL;

// Account information
char *account_id = NULL;
char *account_status = NULL;
char *account_type = NULL;
char *principal_id = NULL;
char *service_type = NULL;
char *service_status = NULL;
char *service_context = NULL;
char **service_phone_number = NULL;
char *service_privacy_value = NULL;
char **service_features = NULL;

int phone_lines = 0;
int current_phone_line = 0;

char *worker_send_buffer = NULL;
char *connection_send_buffer = NULL;
char *incoming_send_buffer = NULL;

char device_id[ 13 ];

#define CHECK_CONNECTION_STATE( con, statement ) if ( con.state != LOGGED_OUT && con.state != LOGGING_OUT ) { statement }

void free_shared_variables()
{
	node_type *node = dllrbt_get_head( saml_cookie_tree );
	while ( node != NULL )
	{
		COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )node->val;

		if ( cc != NULL )
		{
			GlobalFree( cc->cookie_name );
			GlobalFree( cc->cookie_value );
			GlobalFree( cc );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( saml_cookie_tree );
	saml_cookie_tree = NULL;

	node = dllrbt_get_head( wayfarer_cookie_tree );
	while ( node != NULL )
	{
		COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )node->val;

		if ( cc != NULL )
		{
			GlobalFree( cc->cookie_name );
			GlobalFree( cc->cookie_value );
			GlobalFree( cc );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( wayfarer_cookie_tree );
	wayfarer_cookie_tree = NULL;

	node = dllrbt_get_head( vpns_cookie_tree );
	while ( node != NULL )
	{
		COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )node->val;

		if ( cc != NULL )
		{
			GlobalFree( cc->cookie_name );
			GlobalFree( cc->cookie_value );
			GlobalFree( cc );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( vpns_cookie_tree );
	vpns_cookie_tree = NULL;

	GlobalFree( encoded_username );
	encoded_username = NULL;
	GlobalFree( encoded_password );
	encoded_password = NULL;

	GlobalFree( saml_cookies );
	saml_cookies = NULL;
	GlobalFree( wayfarer_cookies );
	wayfarer_cookies = NULL;
	GlobalFree( vpns_cookies );
	vpns_cookies = NULL;

	GlobalFree( client_id );
	client_id = NULL;

	// Free account information.
	GlobalFree( account_id );
	account_id = NULL;
	GlobalFree( account_status );
	account_status = NULL;
	GlobalFree( account_type );
	account_type = NULL;
	GlobalFree( principal_id );
	principal_id = NULL;
	GlobalFree( service_type );
	service_type = NULL;
	GlobalFree( service_status );
	service_status = NULL;
	GlobalFree( service_context );
	service_context = NULL;
	GlobalFree( service_privacy_value );
	service_privacy_value = NULL;

	if ( service_phone_number != NULL )
	{
		for ( int i = 0; i < phone_lines; ++i )
		{
			GlobalFree( service_phone_number[ i ] );
			service_phone_number[ i ] = NULL;
		}
		GlobalFree( service_phone_number );
		service_phone_number = NULL;
	}

	if ( service_features != NULL )
	{
		for ( int i = 0; i < phone_lines; ++i )
		{
			GlobalFree( service_features[ i ] );
			service_features[ i ] = NULL;
		}
		GlobalFree( service_features );
		service_features = NULL;
	}

	phone_lines = 0;
	current_phone_line = 0;

	GlobalFree( worker_send_buffer );
	worker_send_buffer = NULL;
	GlobalFree( connection_send_buffer );
	connection_send_buffer = NULL;
	GlobalFree( incoming_send_buffer );
	incoming_send_buffer = NULL;
}

void CreateMACAddress( char *buffer )
{
	BYTE rbuffer[ 6 ];
	_memzero( rbuffer, 6 );

	bool generate = true;

	#ifndef ADVAPI32_USE_STATIC_LIB
		if ( advapi32_state == ADVAPI32_STATE_SHUTDOWN )
		{
			generate = InitializeAdvApi32();
		}
	#endif

	if ( generate == true )
	{
		HCRYPTPROV hProvider = NULL;
		_CryptAcquireContextW( &hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT );
		_CryptGenRandom( hProvider, 6, ( BYTE * )&rbuffer );
		_CryptReleaseContext( hProvider, 0 );
	}

	__snprintf( buffer, 13, "%.2X%.2X%.2X%.2X%.2X%.2X", rbuffer[ 0 ], rbuffer[ 1 ], rbuffer[ 2 ], rbuffer[ 3 ], rbuffer[ 4 ], rbuffer[ 5 ] );
}

void kill_connection_thread()
{
	if ( in_connection_thread == true )
	{
		// This semaphore will be released when the thread gets killed.
		connection_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		main_con.state = CONNECTION_KILL;		// Causes the connection thread to cease processing and release the semaphore.

		//main_con.state = LOGGED_OUT;			// Set login state to logged off.
		if ( main_con.ssl_socket != NULL )
		{
			_shutdown( main_con.ssl_socket->s, SD_BOTH );	// Force any blocked calls to exit.
		}

		if ( main_con.socket != INVALID_SOCKET )
		{
			_shutdown( main_con.socket, SD_BOTH );
		}

		// If we're in the reconnect loop, then exit.
		if ( reconnect_semaphore != NULL )
		{
			ReleaseSemaphore( reconnect_semaphore, 1, NULL );
		}

		// This will trigger the select_line_semaphore.
		if ( g_hWnd_phone_lines != NULL )
		{
			_SendMessageW( g_hWnd_phone_lines, WM_CLOSE, 0, 0 );
		}

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( connection_semaphore, 5000 );
		CloseHandle( connection_semaphore );
		connection_semaphore = NULL;
	}
}

void kill_connection_worker_thread()
{
	if ( in_connection_worker_thread == true )
	{
		// This semaphore will be released when the thread gets killed.
		connection_worker_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		worker_con.state = CONNECTION_KILL;			// Causes the connection thread to cease processing and release the semaphore.

		//worker_con.state = LOGGED_OUT;			// Set login state to logged off.
		if ( worker_con.ssl_socket != NULL )
		{
			_shutdown( worker_con.ssl_socket->s, SD_BOTH );	// Force any blocked calls to exit.
		}

		if ( worker_con.socket != INVALID_SOCKET )
		{
			_shutdown( worker_con.socket, SD_BOTH );
		}

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( connection_worker_semaphore, 5000 );
		CloseHandle( connection_worker_semaphore );
		connection_worker_semaphore = NULL;
	}
}

void kill_connection_incoming_thread()
{
	if ( in_connection_incoming_thread == true )
	{
		// This semaphore will be released when the thread gets killed.
		connection_incoming_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		incoming_con.state = CONNECTION_KILL;		// Causes the connection thread to cease processing and release the semaphore.

		//incoming_con.state = LOGGED_OUT;			// Set login state to logged off.
		if ( incoming_con.ssl_socket != NULL )
		{
			_shutdown( incoming_con.ssl_socket->s, SD_BOTH );	// Force any blocked calls to exit.
		}

		if ( incoming_con.socket != INVALID_SOCKET )
		{
			_shutdown( incoming_con.socket, SD_BOTH );
		}

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( connection_incoming_semaphore, 5000 );
		CloseHandle( connection_incoming_semaphore );
		connection_incoming_semaphore = NULL;
	}
}

void kill_update_check_thread()
{
	if ( in_update_check_thread == true )
	{
		// This semaphore will be released when the thread gets killed.
		update_check_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		update_con.state = CONNECTION_KILL;			// Causes the connection thread to cease processing and release the semaphore.

		if ( update_con.ssl_socket != NULL )
		{
			_shutdown( update_con.ssl_socket->s, SD_BOTH );	// Force any blocked calls to exit.
		}

		if ( update_con.socket != INVALID_SOCKET )
		{
			_shutdown( update_con.socket, SD_BOTH );
		}

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( update_check_semaphore, 5000 );
		CloseHandle( update_check_semaphore );
		update_check_semaphore = NULL;
	}
}

SOCKET Client_Connection( const char *server, unsigned short port, int timeout )
{
	SOCKET client_socket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;

	char cport[ 6 ];

	if ( ws2_32_state == WS2_32_STATE_SHUTDOWN )
	{
		#ifndef WS2_32_USE_STATIC_LIB
			if ( InitializeWS2_32() == false ){ return INVALID_SOCKET; }
		#else
			StartWS2_32();
		#endif
	}

	_memzero( &hints, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	__snprintf( cport, 6, "%hu", port );

	// Resolve the server address and port
	if ( _getaddrinfo( server, cport, &hints, &result ) != 0 )
	{
		if ( result != NULL )
		{
			_freeaddrinfo( result );
		}

		return INVALID_SOCKET;
	}

	// Attempt to connect to an address until one succeeds
	for ( ptr = result; ptr != NULL; ptr = ptr->ai_next )
	{
		// Create a SOCKET for connecting to server
		client_socket = _socket( ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol );
		if ( client_socket == INVALID_SOCKET )
		{
			_closesocket( client_socket );
			client_socket = INVALID_SOCKET;
			break;
		}

		// Set a timeout for the connection.
		if ( timeout != 0 )
		{
			timeout *= 1000;
			if ( _setsockopt( client_socket, SOL_SOCKET, SO_RCVTIMEO, ( char * )&timeout, sizeof( int ) ) == SOCKET_ERROR )
			{
				_closesocket( client_socket );
				client_socket = INVALID_SOCKET;
				break;
			}
		}

		// Connect to server.
		if ( _connect( client_socket, ptr->ai_addr, ( int )ptr->ai_addrlen ) == SOCKET_ERROR )
		{
			_closesocket( client_socket );
			client_socket = INVALID_SOCKET;
			continue;
		}

		break;
	}

	if ( result != NULL )
	{
		_freeaddrinfo( result );
	}

	return client_socket;
}

SSL *Client_Connection_Secure( const char *server, unsigned short port, int timeout )
{
	// Create a normal socket connection.
	SOCKET client_socket = Client_Connection( server, port, timeout );

	if ( client_socket == INVALID_SOCKET )
	{
		return NULL;
	}

	// Wrap the socket as an SSL connection.

	if ( ssl_state == SSL_STATE_SHUTDOWN )
	{
		if ( SSL_library_init() == 0 )
		{
			_shutdown( client_socket, SD_BOTH );
			_closesocket( client_socket );
			return NULL;
		}
	}

	DWORD protocol = 0;
	switch ( cfg_connection_ssl_version )
	{
		case 4:	protocol |= SP_PROT_TLS1_2_CLIENT;
		case 3:	protocol |= SP_PROT_TLS1_1_CLIENT;
		case 2:	protocol |= SP_PROT_TLS1_CLIENT;
		case 1:	protocol |= SP_PROT_SSL3_CLIENT;
		case 0:	{ if ( cfg_connection_ssl_version < 4 ) { protocol |= SP_PROT_SSL2_CLIENT; } }
	}

	SSL *ssl = SSL_new( protocol );
	if ( ssl == NULL )
	{
		_shutdown( client_socket, SD_BOTH );
		_closesocket( client_socket );
		return NULL;
	}

	ssl->s = client_socket;

	if ( SSL_connect( ssl ) != 1 )
	{
		SSL_shutdown( ssl );
		SSL_free( ssl );

		_shutdown( client_socket, SD_BOTH );
		_closesocket( client_socket );

		return NULL;
	}

	return ssl;
}

void CleanupConnection( CONNECTION *con, char *host = NULL )
{
	char message_log[ 320 ];

	if ( con->ssl_socket != NULL )
	{
		SOCKET s = con->ssl_socket->s;
		SSL_shutdown( con->ssl_socket );
		SSL_free( con->ssl_socket );
		con->ssl_socket = NULL;
		_shutdown( s, SD_BOTH );
		_closesocket( s );

		if ( cfg_enable_message_log == true )
		{
			if ( host != NULL )
			{
				__snprintf( message_log, 320, "Closed connection to: %.253s:%d", host, ( con->secure == true ? DEFAULT_PORT_SECURE : DEFAULT_PORT ) );
				_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_NOTICE, ( LPARAM )message_log );
			}
			else
			{
				_SendNotifyMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_W | ML_NOTICE, ( LPARAM )ST_Closed_connection );
			}
		}
	}

	if ( con->socket != INVALID_SOCKET )
	{
		_shutdown( con->socket, SD_BOTH );
		_closesocket( con->socket );
		con->socket = INVALID_SOCKET;

		if ( cfg_enable_message_log == true )
		{
			if ( host != NULL )
			{
				__snprintf( message_log, 320, "Closed connection: to %.253s:%d", host, ( con->secure == true ? DEFAULT_PORT_SECURE : DEFAULT_PORT ) );
				_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_NOTICE, ( LPARAM )message_log );
			}
			else
			{
				_SendNotifyMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_W | ML_NOTICE, ( LPARAM )ST_Closed_connection );
			}
		}
	}
}

int SendHTTPRequest( CONNECTION *con, char *request_buffer, int request_buffer_length )
{
	if ( con == NULL || ( con->secure == true && con->ssl_socket == NULL ) || ( con->secure == false && con->socket == INVALID_SOCKET ) )
	{
		return SOCKET_ERROR;
	}

	if ( request_buffer == NULL || request_buffer_length <= 0 )
	{
		return 0;
	}

	if ( con->secure == true )
	{
		return SSL_write( con->ssl_socket, request_buffer, request_buffer_length );
	}
	else
	{
		return _send( con->socket, request_buffer, request_buffer_length, 0 );
	}
}

int GetHTTPResponse( CONNECTION *con, char **response_buffer, unsigned int &response_buffer_length, unsigned short &http_status, unsigned int &content_length, unsigned int &last_buffer_size )
{
	response_buffer_length = 0;
	http_status = 0;
	content_length = 0;

	if ( con == NULL || ( con->secure == true && con->ssl_socket == NULL ) || ( con->secure == false && con->socket == INVALID_SOCKET ) )
	{
		if ( *response_buffer != NULL )
		{
			GlobalFree( *response_buffer );
			*response_buffer = NULL;
			last_buffer_size = 0;
		}

		return SOCKET_ERROR;
	}

	unsigned int buffer_size = 0;

	// Set the buffer size.
	if ( last_buffer_size > 0 )
	{
		buffer_size = last_buffer_size;	// Set it to the last size if we've supplied the last size.
	}
	else	// Assume the buffer is NULL if no last size was supplied.
	{
		// Free it if it's not.
		if ( *response_buffer != NULL )
		{
			GlobalFree( *response_buffer );
			*response_buffer = NULL;
		}

		buffer_size = DEFAULT_BUFLEN * 2;
		last_buffer_size = buffer_size;
	}

	// If the response buffer is not NULL, then we are reusing it. If not, then create it.
	if ( *response_buffer == NULL )
	{
		*response_buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( buffer_size + 1 ) );	// Add one for the NULL character.
	}

	int read = 0;	// Must be signed.
	unsigned int header_offset = 0;
	bool chunked_encoding_set = false;
	bool content_length_set = false;

	while ( true )
	{
		if ( con->secure == true )
		{
			read = SSL_read( con->ssl_socket, *response_buffer + response_buffer_length, DEFAULT_BUFLEN );
		}
		else
		{
			read = _recv( con->socket, *response_buffer + response_buffer_length, DEFAULT_BUFLEN, 0 );
		}

		if ( read == SOCKET_ERROR )
		{
			GlobalFree( *response_buffer );
			*response_buffer = NULL;
			last_buffer_size = 0;
			response_buffer_length = 0;
			http_status = 0;
			content_length = 0;

			return SOCKET_ERROR;
		}
		else if ( read <= 0 )
		{
			break;
		}

		*( *response_buffer + response_buffer_length + read ) = 0;	// Sanity.

		if ( header_offset == 0 )
		{
			// Find the end of the header.
			char *end_of_header = fields_tolower( *response_buffer );
			if ( end_of_header != NULL )
			{
				end_of_header += 4;

				header_offset = ( end_of_header - *response_buffer );

				char *find_status = _StrChrA( *response_buffer, ' ' );
				if ( find_status != NULL )
				{
					++find_status;
					char *end_status = _StrChrA( find_status, ' ' );
					if ( end_status != NULL )
					{
						// Status response.
						char cstatus[ 4 ];
						_memzero( cstatus, 4 );
						_memcpy_s( cstatus, 4, find_status, ( ( end_status - find_status ) > 3 ? 3 : ( end_status - find_status ) ) );
						cstatus[ 3 ] = 0; // Sanity
						http_status = ( unsigned short )_strtoul( cstatus, NULL, 10 );
					}
				}

				if ( http_status == 0 )
				{
					GlobalFree( *response_buffer );
					*response_buffer = NULL;
					last_buffer_size = 0;
					response_buffer_length = 0;
					http_status = 0;
					content_length = 0;

					return -1;
				}

				char *transfer_encoding_header = _StrStrA( *response_buffer, "transfer-encoding:" );
				if ( transfer_encoding_header != NULL )
				{
					transfer_encoding_header += 18;
					char *end_of_chunk_header = _StrStrA( transfer_encoding_header, "\r\n" );
					if ( end_of_chunk_header != NULL )
					{
						// Skip whitespace.
						while ( transfer_encoding_header < end_of_chunk_header )
						{
							if ( transfer_encoding_header[ 0 ] != ' ' && transfer_encoding_header[ 0 ] != '\t' && transfer_encoding_header[ 0 ] != '\f' )
							{
								break;
							}

							++transfer_encoding_header;
						}

						// Skip whitespace that could appear before the "\r\n", but after the field value.
						while ( ( end_of_chunk_header - 1 ) >= transfer_encoding_header )
						{
							if ( *( end_of_chunk_header - 1 ) != ' ' && *( end_of_chunk_header - 1 ) != '\t' && *( end_of_chunk_header - 1 ) != '\f' )
							{
								break;
							}

							--end_of_chunk_header;
						}

						if ( _StrCmpNIA( transfer_encoding_header, "chunked", end_of_chunk_header - transfer_encoding_header ) == 0 )
						{
							chunked_encoding_set = true;
						}
					}
				}

				char *content_length_header = _StrStrA( *response_buffer, "content-length:" );
				if ( content_length_header != NULL )
				{
					content_length_header += 15;
					char *end_of_length_header = _StrStrA( content_length_header, "\r\n" );
					if ( end_of_length_header != NULL )
					{
						// Skip whitespace.
						while ( content_length_header < end_of_length_header )
						{
							if ( content_length_header[ 0 ] != ' ' && content_length_header[ 0 ] != '\t' && content_length_header[ 0 ] != '\f' )
							{
								break;
							}

							++content_length_header;
						}

						// Skip whitespace that could appear before the "\r\n", but after the field value.
						while ( ( end_of_length_header - 1 ) >= content_length_header )
						{
							if ( *( end_of_length_header - 1 ) != ' ' && *( end_of_length_header - 1 ) != '\t' && *( end_of_length_header - 1 ) != '\f' )
							{
								break;
							}

							--end_of_length_header;
						}

						char clength[ 11 ];
						_memzero( clength, 11 );
						_memcpy_s( clength, 11, content_length_header, ( ( end_of_length_header - content_length_header ) > 10 ? 10 : ( end_of_length_header - content_length_header ) ) );
						clength[ 10 ] = 0;	// Sanity
						content_length = _strtoul( clength, NULL, 10 );

						content_length_set = true;
					}
				}
			}
		}

		response_buffer_length += read;

		if ( chunked_encoding_set == true )
		{
			// If we've receive the end of a chunked transfer, then exit the loop.
			if ( _StrCmpNA( *response_buffer + response_buffer_length - 4, "\r\n\r\n", 4 ) == 0 )
			{
				break;
			}
		}
			
		if ( content_length_set == true )
		{
			// If there is content, see how much we've received and compare it to the content length.
			if ( content_length + header_offset == response_buffer_length )
			{
				break;
			}
		}

		// Resize the response_buffer if we need more room.
		if ( response_buffer_length + DEFAULT_BUFLEN >= buffer_size )
		{
			buffer_size += DEFAULT_BUFLEN;
			last_buffer_size = buffer_size;

			char *realloc_buffer = ( char * )GlobalReAlloc( *response_buffer, sizeof( char ) * buffer_size, GMEM_MOVEABLE );
			if ( realloc_buffer == NULL )
			{
				GlobalFree( *response_buffer );
				*response_buffer = NULL;
				last_buffer_size = 0;
				response_buffer_length = 0;
				http_status = 0;
				content_length = 0;
				
				return -1;
			}

			*response_buffer = realloc_buffer;
		}
	}

	*( *response_buffer + response_buffer_length ) = 0;	// Sanity

	// Ensure that the content length was accurately represented.
	if ( content_length + header_offset != response_buffer_length )
	{
		content_length = response_buffer_length - header_offset;
	}



	// Now we need to decode the buffer in case it was a chunked transfer.



	if ( chunked_encoding_set == true )
	{
		// Beginning of the first chunk. (After the header)
		char *find_chunk = *response_buffer + header_offset;

		unsigned int chunk_offset = header_offset;

		// Find the end of the first chunk.
		char *end_chunk = _StrStrA( find_chunk, "\r\n" );
		if ( end_chunk == NULL )
		{
			return -1;	// If it doesn't exist, then we exit.
		}

		unsigned int data_length = 0;
		char chunk[ 32 ];

		do
		{
			// Get the chunk value (including any extensions).
			_memzero( chunk, 32 );
			_memcpy_s( chunk, 32, find_chunk, ( ( end_chunk - find_chunk ) > 32 ? 32 : ( end_chunk - find_chunk ) ) );
			chunk[ 31 ] = 0; // Sanity

			end_chunk += 2;
			
			// Find any chunked extension and ignore it.
			char *find_extension = _StrChrA( chunk, ';' );
			if ( find_extension != NULL )
			{
				*find_extension = NULL;
			}

			// Convert the hexidecimal chunk value into an integer. Stupid...
			data_length = _strtoul( chunk, NULL, 16 );
			
			// If the chunk value is less than or 0, or it's too large, then we're done.
			if ( data_length == 0 || ( data_length > ( response_buffer_length - chunk_offset ) ) )
			{
				break;
			}

			// Copy over our reply_buffer.
			_memcpy_s( *response_buffer + chunk_offset, response_buffer_length - chunk_offset, end_chunk, data_length );
			chunk_offset += data_length;				// data_length doesn't include any NULL characters.
			*( *response_buffer + chunk_offset ) = 0;	// Sanity

			find_chunk = _StrStrA( end_chunk + data_length, "\r\n" ); // This should be the token after the data.
			if ( find_chunk == NULL )
			{
				break;
			}
			find_chunk += 2;

			end_chunk = _StrStrA( find_chunk, "\r\n" );
			if ( end_chunk == NULL )
			{
				break;
			}
		} 
		while ( data_length > 0 );

		response_buffer_length = chunk_offset;
		content_length = response_buffer_length - header_offset;
	}

	return 0;
}

bool Try_Connect( CONNECTION *con, char *host, int timeout = 0, bool retry = true )
{
	int total_retries = 0;

	if ( cfg_connection_reconnect == true && retry == true )
	{
		total_retries = cfg_connection_retries;
	}

	char message_log[ 320 ];
	if ( cfg_enable_message_log == true )
	{
		__snprintf( message_log, 320, "Connecting to: %.253s:%d", host, ( con->secure == true ? DEFAULT_PORT_SECURE : DEFAULT_PORT ) );
		_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_NOTICE, ( LPARAM )message_log );
	}

	// We consider retry == 0 to be our normal connection.
	for ( int retry = 0; retry <= total_retries; ++retry )
	{
		// Stop processing and exit the thread.
		if ( con->state == LOGGED_OUT || con->state == LOGGING_OUT )
		{
			break;
		}

		// Connect to server.
		if ( con->secure == true )
		{
			con->ssl_socket = Client_Connection_Secure( host, DEFAULT_PORT_SECURE, timeout );
		}
		else
		{
			con->socket = Client_Connection( host, DEFAULT_PORT, timeout );
		}

		// If the connection failed, try again.
		if ( ( con->secure == true && con->ssl_socket == NULL ) || ( con->secure == false && con->socket == INVALID_SOCKET ) )
		{
			// If the connection failed our first attempt, and then the subsequent number of retries, then we exit.
			if ( retry == total_retries )
			{
				break;
			}

			if ( cfg_enable_message_log == true )
			{
				__snprintf( message_log, 320, "Retrying connection to: %.253s:%d", host, ( con->secure == true ? DEFAULT_PORT_SECURE : DEFAULT_PORT ) );
				_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_WARNING, ( LPARAM )message_log );
			}

			// Retry from the beginning.
			continue;
		}

		if ( cfg_enable_message_log == true )
		{
			__snprintf( message_log, 320, "Connected to: %.253s:%d", host, ( con->secure == true ? DEFAULT_PORT_SECURE : DEFAULT_PORT ) );
			_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_NOTICE, ( LPARAM )message_log );
		}

		// We've made a successful connection.
		return true;
	}

	if ( cfg_enable_message_log == true )
	{
		__snprintf( message_log, 320, "Failed to connect to %.253s:%d", host, ( con->secure == true ? DEFAULT_PORT_SECURE : DEFAULT_PORT ) );
		_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_ERROR, ( LPARAM )message_log );
	}

	// If we couldn't establish a connection, or we've abuptly shutdown, then free any ssl_socket resources.
	CleanupConnection( con, host );

	return false;
}

int Try_Send_Receive( CONNECTION *con, char *host, char *resource /*for logging only*/,
					  char *request_buffer, unsigned int request_buffer_length,
					  char **response_buffer, unsigned int &response_buffer_length, unsigned short &http_status, unsigned int &content_length, unsigned int &last_buffer_size,
					  int timeout = 0, bool retry = true )
{
	int response_code = 0;
	int total_retries = 0;
	http_status = 0;

	if ( cfg_connection_reconnect == true && retry == true )
	{
		total_retries = cfg_connection_retries;
	}

	// We consider retry == 0 to be our normal connection.
	for ( int retry = 0; retry <= total_retries; ++retry )
	{
		// Stop processing and exit the thread.
		if ( con->state == LOGGED_OUT || con->state == LOGGING_OUT )
		{
			break;
		}

		response_code = 0;

		char message_log[ 2430 ];
		if ( cfg_enable_message_log == true )
		{
			__snprintf( message_log, 2430, "Sending %lu byte request for: http%s://%.253s:%d%.2048s", request_buffer_length, ( con->secure == true ? "s" : "" ), host, ( con->secure == true ? DEFAULT_PORT_SECURE : DEFAULT_PORT ), resource );
			_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_NOTICE, ( LPARAM )message_log );
		}

		if ( SendHTTPRequest( con, request_buffer, request_buffer_length ) > 0 )
		{
			response_code = GetHTTPResponse( con, response_buffer, response_buffer_length, http_status, content_length, last_buffer_size );
		}

		if ( cfg_enable_message_log == true )
		{
			__snprintf( message_log, 2430, "Received %lu byte response with status code %lu from: http%s://%.253s:%d%.2048s", response_buffer_length, http_status, ( con->secure == true ? "s" : "" ), host, ( con->secure == true ? DEFAULT_PORT_SECURE : DEFAULT_PORT ), resource );
			_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_NOTICE, ( LPARAM )message_log );
		}

		// HTTP Status codes: 200 OK, 302 Found (redirect)
		if ( http_status == 200 || http_status == 302 )
		{
			return 0;
		}
		else if ( http_status == 403 )	// Bad cookies.
		{
			return 0;
		}
		else if ( response_code <= 0 )	// No response was received: 0, or recv failed: -1
		{
			// Stop processing and exit the thread.
			if ( con->state == LOGGED_OUT || con->state == LOGGING_OUT )
			{
				break;
			}

			if ( retry == total_retries )
			{
				break;
			}

			if ( cfg_enable_message_log == true )
			{
				__snprintf( message_log, 2430, "Retrying %lu byte request for: http%s://%.253s:%d%.2048s", request_buffer_length, ( con->secure == true ? "s" : "" ), host, ( con->secure == true ? DEFAULT_PORT_SECURE : DEFAULT_PORT ), resource );
				_SendMessageW( g_hWnd_message_log, WM_ALERT, ML_WRITE_OUTPUT_A | ML_WARNING, ( LPARAM )message_log );
			}

			// Close the connection since no more data will be sent or received.
			CleanupConnection( con, host );

			// Establish the connection again.
			if ( Try_Connect( con, host, timeout ) == false )
			{
				break;
			}
		}
		else	// Some other HTTP Status code.
		{
			break;
		}
	}

	// If we couldn't establish a connection, or we've abuptly shutdown, then free any ssl_socket resources.
	CleanupConnection( con, host );

	return -1;
}

void GetVersionNumber( char *version_url, unsigned int &version, char **download_url )
{
	char *resource = NULL;
	char *host = NULL;

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	int send_buffer_length = 0;

	char *version_send_buffer = NULL;

	_SetWindowTextW( g_hWnd_main, L"VZ Enhanced - Checking for updates..." );

	if ( ParseURL( version_url, &host, &resource ) == false )
	{
		goto CLEANUP;
	}

	version_send_buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( DEFAULT_BUFLEN * 2 ) );

	// First determine the server type (http or https)
	if ( _memcmp( version_url, "https://", 8 ) == 0 )
	{
		update_con.secure = true;
	}
	else
	{
		update_con.secure = false;
	}

	// Request the file and go through any redirects that exist.
	for ( unsigned char redirects = 0; redirects < 5; ++redirects )
	{
		send_buffer_length = __snprintf( version_send_buffer, DEFAULT_BUFLEN * 2,
		"GET %s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Connection: keep-alive\r\n\r\n", resource, host );

		if ( Try_Connect( &update_con, host, cfg_connection_timeout ) == false )
		{
			goto CLEANUP;
		}

		if ( Try_Send_Receive( &update_con, host, resource, version_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
		{
			goto CLEANUP;
		}

		CleanupConnection( &update_con, host );

		GlobalFree( resource );
		resource = NULL;

		GlobalFree( host );
		host = NULL;
		
		if ( http_status == 302 )	// Redirect
		{
			if ( ParseRedirect( response, &host, &resource ) == false )
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	if ( http_status == 200 )
	{
		char *file = _StrStrA( response, "\r\n\r\n" );
		if ( file != NULL )
		{
			file += 4;

			char *version_end = _StrStrA( file, "\r\n" );
			if ( version_end != NULL )
			{
				*version_end = 0;
				version_end += 2;

				version = _strtoul( file, NULL, 10 );

				if ( content_length >= ( unsigned int )( version_end - file ) )
				{
					unsigned int download_url_length = ( content_length - ( version_end - file ) );

					*download_url = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( download_url_length + 1 ) );
					_memcpy_s( *download_url, download_url_length + 1, version_end, download_url_length );

					*( *download_url + download_url_length ) = 0;	// Sanity.
				}
			}
		}
	}

CLEANUP:

	CleanupConnection( &update_con, host );

	GlobalFree( response );
	GlobalFree( resource );
	GlobalFree( host );
	GlobalFree( version_send_buffer );

	_SetWindowTextW( g_hWnd_main, PROGRAM_CAPTION );
}

bool DownloadUpdate( char *download_url )
{
	char *host = NULL;
	char *resource = NULL;

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	int send_buffer_length = 0;

	char *download_buffer = NULL;

	wchar_t *file_path = NULL;

	bool update_status = false;

	_SetWindowTextW( g_hWnd_main, L"VZ Enhanced - Downloading update..." );

	if ( ParseURL( download_url, &host, &resource ) == false )
	{
		goto CLEANUP;
	}

	download_buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( DEFAULT_BUFLEN * 32 ) );

	// First determine the server type (http or https)
	if ( _memcmp( download_url, "https://", 8 ) == 0 )
	{
		update_con.secure = true;
	}
	else
	{
		update_con.secure = false;
	}

	// Get the file name from the url.
	char *file_name = resource;
	int offset = lstrlenA( file_name );

	while ( offset != 0 && file_name[ --offset ] != '/' );

	if ( file_name[ offset ] == '/' )
	{
		++offset;
	}
	
	file_name += offset;

	int val_length = MultiByteToWideChar( CP_UTF8, 0, file_name, -1, NULL, 0 );	// Include the NULL terminator.
	wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
	MultiByteToWideChar( CP_UTF8, 0, file_name, -1, val, val_length );

	file_path = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof ( wchar_t ) * MAX_PATH );
	_memzero( file_path, sizeof ( wchar_t ) * MAX_PATH );
	_wmemcpy_s( file_path, MAX_PATH, val, val_length );

	GlobalFree( val );

	// Request the file and go through any redirects that exist.
	for ( unsigned char redirects = 0; redirects < 5; ++redirects )
	{
		send_buffer_length = __snprintf( download_buffer, DEFAULT_BUFLEN * 32,
		"GET %s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Connection: keep-alive\r\n\r\n", resource, host );

		if ( Try_Connect( &update_con, host, cfg_connection_timeout ) == false )
		{
			goto CLEANUP;
		}

		if ( Try_Send_Receive( &update_con, host, resource, download_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
		{
			goto CLEANUP;
		}

		CleanupConnection( &update_con, host );

		GlobalFree( resource );
		resource = NULL;

		GlobalFree( host );
		host = NULL;
		
		if ( http_status == 302 )	// Redirect
		{
			if ( ParseRedirect( response, &host, &resource ) == false )
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	if ( http_status == 200 )
	{
		char *file = _StrStrA( response, "\r\n\r\n" );
		if ( file != NULL )
		{
			file += 4;

			update_status = true;

			_SetWindowTextW( g_hWnd_main, PROGRAM_CAPTION );

			OPENFILENAME ofn;
			_memzero( &ofn, sizeof( OPENFILENAME ) );
			ofn.lStructSize = sizeof( OPENFILENAME );
			ofn.hwndOwner = g_hWnd_main;
			ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
			ofn.lpstrTitle = L"Save As";
			ofn.lpstrFile = file_path;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_READONLY;

			if ( _GetSaveFileNameW( &ofn ) )
			{
				// Save file.
				DWORD write = 0;
				HANDLE hFile = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
				if ( hFile != INVALID_HANDLE_VALUE )
				{
					WriteFile( hFile, file, content_length, &write, NULL );

					CloseHandle( hFile );
				}
				else
				{
					if ( update_con.state != CONNECTION_KILL && update_con.state != CONNECTION_CANCEL ){ MESSAGE_LOG_OUTPUT_MB( ML_ERROR, ST_File_could_not_be_saved ) }
				}
			}
		}
	}

CLEANUP:

	CleanupConnection( &update_con, host );

	GlobalFree( response );
	GlobalFree( resource );
	GlobalFree( host );
	GlobalFree( download_buffer );
	GlobalFree( file_path );

	_SetWindowTextW( g_hWnd_main, PROGRAM_CAPTION );

	return update_status;
}

THREAD_RETURN CheckForUpdates( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &cut_cs );

	in_update_check_thread = true;

	UPDATE_CHECK_INFO *update_info = ( UPDATE_CHECK_INFO * )pArguments;

	if ( update_info == NULL )
	{
		goto CLEANUP;
	}

	update_con.state = CONNECTION_ACTIVE;

	// Check the version.txt file.
	if ( update_info->download_url == NULL )
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Checking_for_updates )

		unsigned int new_version = 0;

		_EnableMenuItem( g_hMenu, MENU_CHECK_FOR_UPDATES, MF_DISABLED );

		GetVersionNumber( VERSION_URL, new_version, &( update_info->download_url ) );

		_EnableMenuItem( g_hMenu, MENU_CHECK_FOR_UPDATES, MF_ENABLED );

		if ( new_version == CURRENT_VERSION )
		{
			if ( update_info->notify == true )
			{
				// Up to date
				if ( update_con.state != CONNECTION_KILL && update_con.state != CONNECTION_CANCEL ){ _SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, NULL ); }
			}

			MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_The_program_is_up_to_date )
		}
		else if ( new_version > CURRENT_VERSION )
		{
			// update_info is freed in WM_ALERT, or passed back to this function to download and then freed.
			if ( update_con.state != CONNECTION_KILL && update_con.state != CONNECTION_CANCEL )
			{
				UPDATE_CHECK_INFO *t_update_info = update_info;

				update_info = NULL;

				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 1, ( LPARAM )t_update_info );
			}

			MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_A_new_version_of_the_program_is_available )
		}
		else
		{
			if ( update_con.state != CONNECTION_KILL && update_con.state != CONNECTION_CANCEL ){ _SendNotifyMessageW( g_hWnd_main, WM_ALERT, 2, NULL ); }

			MESSAGE_LOG_OUTPUT( ML_WARNING, ST_The_update_check_could_not_be_completed )
		}
	}
	else	// If we have a download url, then get the file.
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Downloading_update )

		MENUITEMINFO mii;
		_memzero( &mii, sizeof( MENUITEMINFO ) );
		mii.cbSize = sizeof( MENUITEMINFO );
		mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		mii.fType = MFT_STRING;
		mii.dwTypeData = ST_Cancel_Update_Download;
		mii.cch = 13;
		mii.wID = MENU_CHECK_FOR_UPDATES;
		mii.fState = MFS_ENABLED;
		_SetMenuItemInfoW( g_hMenu, MENU_CHECK_FOR_UPDATES, FALSE, &mii );

		if ( DownloadUpdate( update_info->download_url ) == false )
		{
			if ( update_con.state != CONNECTION_KILL && update_con.state != CONNECTION_CANCEL ){ _SendNotifyMessageW( g_hWnd_main, WM_ALERT, 3, NULL ); }

			MESSAGE_LOG_OUTPUT( ML_WARNING, ST_The_download_could_not_be_completed )
		}
		else
		{
			MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Downloaded_update )
		}

		mii.dwTypeData = ST_Check_for__Updates;
		mii.cch = 18;
		_SetMenuItemInfoW( g_hMenu, MENU_CHECK_FOR_UPDATES, FALSE, &mii );
	}

CLEANUP:

	CleanupConnection( &update_con );
	update_con.state = CONNECTION_KILL;

	if ( update_info != NULL )
	{
		GlobalFree( update_info->download_url );
		GlobalFree( update_info );
	}

	// Release the semaphore if we're killing the thread.
	if ( update_check_semaphore != NULL )
	{
		ReleaseSemaphore( update_check_semaphore, 1, NULL );
	}

	in_update_check_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &cut_cs );

	_ExitThread( 0 );
	return 0;
}

int ConstructVPNSPOST( char *send_buffer, char *resource, char *data, int data_length )
{
	char *host = "vpns.timewarnercable.com";

	return __snprintf( send_buffer, DEFAULT_BUFLEN * 2,
	"POST %s " \
	"HTTP/1.1\r\n" \
	"Host: %s\r\n" \
	"Referer: %s\r\n" \
	"App-Name: %s\r\n" \
	"User-Agent: %s\r\n" \
	"Origin: %s\r\n" \
	"Cookie: %s; %s\r\n" \
	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n" \
	"Content-Length: %d\r\n" \
	"Connection: close\r\n\r\n" \
	"%s", resource, host, REFERER, APPLICATION_NAME, USER_AGENT, ORIGIN, vpns_cookies, wayfarer_cookies, data_length, data );
}

bool CheckAuthorization( char *response )
{
	bool authorization_status = false;
	char *error_code = NULL;

	if ( ParseXApplicationErrorCode( response, &error_code ) == true )
	{
		// Set the error code string to lower case.
		_CharLowerA( ( LPSTR )error_code );

		if ( _StrStrA( error_code, "auth cookie" ) != NULL )
		{
			authorization_status = true;
		}

		GlobalFree( error_code );
	}

	return authorization_status;
}

// Calling function is responsible for establishing a connection.
bool UploadMedia( wchar_t *file_path, HWND hWnd_update, unsigned char media_type )
{
	bool upload_status = false;

	char *upload_buffer = NULL;

	// Open the file for reading.
	HANDLE hFile = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		goto CLEANUP;
	}

	// Get the picture file's size.
	DWORD size = GetFileSize( hFile, 0 );

	// If it's larger than 5 MB, then exit the thread. The server doesn't want files larger than 5 MB.
	if ( size > 5242880 )
	{
		MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_File_size_must_be_less_than_5_megabytes )
		goto CLEANUP;
	}

	char *host = "vpns.timewarnercable.com";
	char *resource = NULL;
	if ( media_type == 1 )	// Picture types
	{
		resource = "/vpnscontent/v1_5/media/sizedphoto/";
	}
	else	// Other types (contact import lists)
	{
		resource = "/vpnscontent/v1_5/media/";
	}

	DWORD read = 0;			// The chunk size we've read from the picture file.
	DWORD total_read = 0;	// The total amount of data we've read from the picture file. Compare this to the file size.

	int upload_buffer_offset = 0;
	int upload_buffer_length = 524288 + 1024;
	upload_buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * upload_buffer_length );

	// Set our header information.

	upload_buffer_length = __snprintf( upload_buffer, upload_buffer_length,
	"POST %s " \
	"HTTP/1.1\r\n" \
	"Host: %s\r\n" \
	"Referer: %s\r\n" \
	"App-Name: %s\r\n" \
	"User-Agent: %s\r\n" \
	"Origin: %s\r\n" \
	"Cookie: %s; %s\r\n" \
	"Content-Type: multipart/form-data\r\n" \
	"Content-Length: %d\r\n" \
	"Connection: close\r\n\r\n", resource, host, REFERER, APPLICATION_NAME, USER_AGENT, ORIGIN, vpns_cookies, wayfarer_cookies, size );

	bool read_file = false;		// Skips adding the header size to upload_info.
	UPLOAD_INFO upload_info;
	upload_info.size = size;
	upload_info.sent = 0;

	// 1. Send the header we constructed above.
	// 2. Read a 512 KB chunk of the picture file into the upload buffer.
	// 3. Send 32 KB chunks of that 512 KB chunk.
	// 4. When all of the 512 KB chunk has been sent, repeat steps 2 and 3.
	while ( true )
	{
		if ( ( media_type == 0 && contact_import_in_progress == UPDATE_FAIL ) || ( media_type == 1 && contact_update_in_progress == UPDATE_FAIL ) )
		{
			goto CLEANUP;
		}

		if ( SendHTTPRequest( &worker_con, upload_buffer + upload_buffer_offset, upload_buffer_length ) > 0 )
		{
			upload_buffer_offset += upload_buffer_length;	// Update our offset for each chunk sent.

			// Update our contact window with the upload progress.
			if ( hWnd_update != NULL && read_file == true )
			{
				upload_info.sent += upload_buffer_length;
				_SendMessageW( hWnd_update, WM_PROPAGATE, MAKEWPARAM( CW_UPDATE, UPDATE_PROGRESS ), ( LPARAM )&upload_info );
			}

			// Read data from the picture file if we haven't already, or if we've sent all of the 512 KB picture file buffer.
			if ( read == 0 )
			{
				// Exit the loop if we've read all of the data.
				if ( total_read >= size )
				{
					break;
				}

				// Read a 512 KB chunk of the picture file into the upload buffer.
				ReadFile( hFile, upload_buffer, 524288, &read, NULL );

				read_file = true;

				total_read += read;			// Update the total size we've read.

				upload_buffer_offset = 0;	// Reset our offset for each 512 KB chunk read.

				// If there's no more data to read, then exit the loop.
				if ( read == 0 )
				{
					break;
				}
			}

			// Set our buffer length to the read size if read is less than 32 KB.
			if ( read < 32768 )
			{
				upload_buffer_length = read;
			}
			else	// Otherwise, set our buffer length to the 32 KB chunk.
			{
				upload_buffer_length = 32768;
			}

			// Decrement our total read size by the amount we're going to send.
			read -= upload_buffer_length;

			// Read should become 0, but if it gets decremented beyond that, then exit the thread.
			if ( read > size )
			{
				goto CLEANUP;
			}
		}
		else	// Send failed, exit the thread.
		{
			goto CLEANUP;
		}
	}

	upload_status = true;

CLEANUP:

	// We're done reading. Close the file.
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		CloseHandle( hFile );
	}

	GlobalFree( upload_buffer );

	return upload_status;
}

THREAD_RETURN ImportContactList( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &cwt_cs );

	in_connection_worker_thread = true;

	if ( g_hWnd_main != NULL )
	{
		_SendMessageW( g_hWnd_main, WM_PROPAGATE, MAKEWPARAM( CW_UPDATE, UPDATE_BEGIN ), 0 );
	}

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	bool import_status = false;

	importexportinfo *iei = ( importexportinfo * )pArguments;

	if ( iei == NULL )
	{
		goto CLEANUP;
	}

	if ( iei->file_path == NULL )
	{
		goto CLEANUP;
	}

	char *host = "vpns.timewarnercable.com";

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Importing_contact_list )

	// Establish a connection to the server and keep it active until shutdown.
	// Use a 60 second timeout in case the file has a lot of entries. The server seems to take a while to process them.
	if ( worker_con.ssl_socket == NULL && Try_Connect( &worker_con, host, 60/*cfg_connection_timeout*/ ) == false )
	{
		//CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_VPNS_server ) )
		goto CLEANUP;
	}

	// Media type = 0 for contact list.
	if ( UploadMedia( iei->file_path, g_hWnd_main, 0 ) == false )
	{
		goto CLEANUP;
	}

	int response_code = GetHTTPResponse( &worker_con, &response, response_length, http_status, content_length, last_buffer_size );
	if ( response_code == -1 || ( http_status != 200 && http_status != 302 ) )
	{
		if ( worker_con.state != LOGGED_OUT && worker_con.state != LOGGING_OUT )
		{
			char *error_code = NULL;	// POST requests only.
			if ( ParseXApplicationErrorCode( response, &error_code ) == true )
			{
				MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
			}
		}

		goto CLEANUP;
	}

	// If successful, get the import list location.
	char *xml = _StrStrA( response, "\r\n\r\n" );
	if ( xml != NULL && *( xml + 4 ) != NULL )
	{
		xml += 4;

		// If we got an import list location, then request it be imported to the server.
		char *import_list_location = NULL;
		if ( GetMediaLocation( xml, &import_list_location ) == true )
		{
			int send_buffer_length = 0;

			// Get the file name from the import file.
			int cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, iei->file_path, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
			char *utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the NULL character.
			WideCharToMultiByte( CP_UTF8, 0, iei->file_path, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			char *filename = GetFileName( utf8_cfg_val );

			char update_location_reply[ 1024 ];

			//char *host = "vpns.timewarnercable.com";
			char *resource = "/vpnsservice/v1_5/contacts";

			int update_location_reply_length = __snprintf( update_location_reply, 1024,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
			"<UABContactList operation=\"contactImport\">" \
				"<User id=\"%s\"/>" \
				"<File type=\"%s\">" \
					"<Language>en</Language>" \
					"<FormatType>%s</FormatType>" \
					"<Disposition>filename=%s</Disposition>" \
					"<Location>%s</Location>" \
				"</File>" \
			"</UABContactList>", SAFESTRA( account_id ), ( iei->file_type == 1 ? "vcard" : "outlook" ), ( iei->file_type == 1 ? "text/vcard" : "text/csv" ), SAFESTRA( filename ), SAFESTRA( import_list_location ) );

			GlobalFree( utf8_cfg_val );
			GlobalFree( import_list_location );

			send_buffer_length = ConstructVPNSPOST( worker_send_buffer, resource, update_location_reply, update_location_reply_length );

			// Get the import information.
			if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, 60/*cfg_connection_timeout*/ ) == -1 )
			{
				if ( worker_con.state != LOGGED_OUT && worker_con.state != LOGGING_OUT )
				{
					char *error_code = NULL;	// POST requests only.
					if ( ParseXApplicationErrorCode( response, &error_code ) == true )
					{
						MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
					}
				}

				goto CLEANUP;
			}

			xml = _StrStrA( response, "\r\n\r\n" );
			if ( xml != NULL && *( xml + 4 ) != NULL )
			{
				xml += 4;

				int success_count = 0;
				int failure_count = 0;
				ParseImportResponse( xml, success_count, failure_count );

				char *response_message = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 128 );
				__snprintf( response_message, 128, "%d out of %d contacts were imported successfully.", success_count, success_count + failure_count );

				_SendNotifyMessageW( g_hWnd_message_log, WM_ALERT, ( cfg_enable_message_log == true ? ML_WRITE_OUTPUT_A : 0 ) | ML_FREE_INPUT | ML_MESSAGE_BOX_A | ML_NOTICE, ( LPARAM )response_message );	// response_message will be freed.

				// We imported at least 1 contact. Add them to our contact list.
				if ( success_count > 0 )
				{
					manageinfo *mi = ( manageinfo * )GlobalAlloc( GMEM_FIXED, sizeof( manageinfo ) );
					mi->ci = NULL;
					mi->manage_type = 0;	// Get and add all contacts.

					// mi is freed in the ManageContactInformation thread.
					CloseHandle( ( HANDLE )_CreateThread( NULL, 0, ManageContactInformation, ( void * )mi, 0, NULL ) );
				}

				import_status = true;
			}
		}
	}

CLEANUP:

	CleanupConnection( &worker_con, host );

	GlobalFree( response );

	if ( iei != NULL )
	{
		GlobalFree( iei->file_path );
		GlobalFree( iei );
	}

	// Release the semaphore if we're killing the thread.
	if ( connection_worker_semaphore != NULL )
	{
		ReleaseSemaphore( connection_worker_semaphore, 1, NULL );
	}

	if ( g_hWnd_main != NULL )
	{

		_SendMessageW( g_hWnd_main, WM_PROPAGATE, MAKEWPARAM( CW_UPDATE, ( ( import_status == false && contact_update_in_progress != UPDATE_FAIL ) ? UPDATE_FAIL : UPDATE_END ) ), 0 );
	}

	in_connection_worker_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &cwt_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN ExportContactList( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &cwt_cs );

	in_connection_worker_thread = true;

	importexportinfo *iei = ( importexportinfo * )pArguments;

	if ( iei == NULL )
	{
		goto CLEANUP;
	}

	if ( iei->file_path == NULL )
	{
		goto CLEANUP;
	}

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	int send_buffer_length = 0;

	char *resource = "/vpnsservice/v1_5/contacts";
	char *host = "vpns.timewarnercable.com";

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Exporting_contact_list )

	// Establish a connection to the server and keep it active until shutdown.
	if ( worker_con.ssl_socket == NULL && Try_Connect( &worker_con, host, cfg_connection_timeout ) == false )
	{
		//CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_VPNS_server ) )
		goto CLEANUP;
	}

	char export_reply[ 1024 ];

	int export_reply_length = __snprintf( export_reply, 1024,
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
	"<UABContactList operation=\"contactExport\">" \
		"<User id=\"%s\"/>" \
		"<File type=\"%s\" />" \
	"</UABContactList>", SAFESTRA( account_id ), ( iei->file_type == 1 ? "vcard" : "outlook" ) );

	send_buffer_length = ConstructVPNSPOST( worker_send_buffer, resource, export_reply, export_reply_length );

	if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		if ( worker_con.state != LOGGED_OUT && worker_con.state != LOGGING_OUT )
		{
			char *error_code = NULL;	// POST requests only.
			if ( ParseXApplicationErrorCode( response, &error_code ) == true )
			{
				MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
			}
		}

		goto CLEANUP;
	}

	char *xml = _StrStrA( response, "\r\n\r\n" );
	if ( xml != NULL && *( xml + 4 ) != NULL )
	{
		xml += 4;

		char *contact_list_location = NULL;
		if ( GetContactListLocation( xml, &contact_list_location ) == true )
		{
			// Download the file.

			char *list_host = NULL;
			char *list_resource = NULL;

			if ( ParseURL( contact_list_location, &list_host, &list_resource ) == true )
			{
				// If ci is not NULL, then request that specific contact's information. Otherwise, request all the contacts information.
				send_buffer_length = __snprintf( worker_send_buffer, DEFAULT_BUFLEN * 2,
				"GET %s " \
				"HTTP/1.1\r\n" \
				"Host: %s\r\n" \
				"Referer: %s\r\n" \
				"App-Name: %s\r\n" \
				"User-Agent: %s\r\n" \
				"Cookie: %s\r\n" \
				"Connection: close\r\n\r\n", list_resource, list_host, REFERER, APPLICATION_NAME, USER_AGENT, wayfarer_cookies );

				if ( Try_Send_Receive( &worker_con, list_host, list_resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
				{
					GlobalFree( list_host );
					GlobalFree( list_resource );

					CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT_MB( ML_ERROR, ST_Failed_to_export_contact_list ) )
					goto CLEANUP;
				}

				char *xml = _StrStrA( response, "\r\n\r\n" );
				if ( xml != NULL && *( xml + 4 ) != NULL )
				{
					xml += 4;

					// Save file.
					DWORD write = 0;
					HANDLE hFile = CreateFile( iei->file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
					if ( hFile != INVALID_HANDLE_VALUE )
					{
						WriteFile( hFile, xml, content_length, &write, NULL );

						CloseHandle( hFile );

						MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Exported_contact_list )
					}
					else
					{
						CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT_MB( ML_ERROR, ST_File_could_not_be_saved ) )
					}
				}
			}

			GlobalFree( list_host );
			GlobalFree( list_resource );
		}
	}

CLEANUP:

	CleanupConnection( &worker_con, host );

	GlobalFree( response );

	if ( iei != NULL )
	{
		GlobalFree( iei->file_path );
		GlobalFree( iei );
	}

	// Release the semaphore if we're killing the thread.
	if ( connection_worker_semaphore != NULL )
	{
		ReleaseSemaphore( connection_worker_semaphore, 1, NULL );
	}

	in_connection_worker_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &cwt_cs );

	_ExitThread( 0 );
	return 0;
}

void DownloadContactPictures( updateinfo *ui )
{
	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	int send_buffer_length = 0;

	char *host = NULL;
	char *resource = NULL;

	bool downloaded_picture = false;	// For logging purposes.

	contactinfo *old_ci = NULL;	// Do not free this.

	EnterCriticalSection( &pe_cs );

	// old_ci is the contact we want to download a picture for. Make sure it exists if ui is not NULL.
	if ( ui != NULL && ui->old_ci == NULL )
	{
		goto CLEANUP;
	}
	else if ( ui != NULL )
	{
		old_ci = ( contactinfo * )ui->old_ci;
	}

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Downloading_contact_picture_s_ )

	// If ui == NULL, then download all contact pictures.

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\pictures\0", 10 );
	base_directory[ base_directory_length + 9 ] = 0;	// Sanity

	int folder_length = base_directory_length + 9;

	// Create the pictures folder if the folder does not exits.
	if ( GetFileAttributesW( base_directory ) == INVALID_FILE_ATTRIBUTES )
	{
		CreateDirectoryW( base_directory, NULL );
	}

	// Go through the contact_list tree and download each contact's picture.
	node_type *node = NULL;
	contactinfo *ci = NULL;

	if ( old_ci == NULL )
	{
		node = dllrbt_get_head( contact_list );
	}

	while ( true )
	{
		if ( old_ci == NULL && node == NULL )
		{
			break;
		}

		// Go through the contact_list tree for each contact.
		if ( old_ci == NULL )
		{
			ci = ( contactinfo * )node->val;

			// Make sure the contact/s we're processing aren't already displayed.
			if ( ci != NULL && ci->displayed == true )
			{
				node = node->next;
				continue;
			}
		}
		else	// Use the single contact we supplied.
		{
			ci = old_ci;
		}

		// Make sure the contact exits and has a picture location set.
		if ( ci != NULL && ci->contact.picture_location != NULL && ci->contact.contact_entry_id != NULL )
		{
			if ( ci->picture_path != NULL )
			{
				GlobalFree( ci->picture_path );
				ci->picture_path = NULL;
			}

			int picture_path_length = folder_length + lstrlenA( ci->contact.contact_entry_id ) + 6;

			ci->picture_path = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * picture_path_length );

			// Construct the filename based on the contact entry id. Append .jpg at the end.
			__snwprintf( ci->picture_path, picture_path_length, L"%s\\%S.jpg", base_directory, ci->contact.contact_entry_id );

			// See if the file already exits. Don't download if it does.
			/*if ( GetFileAttributes( ci->picture_path ) != INVALID_FILE_ATTRIBUTES )
			{
				if ( old_ci == NULL ){ node = node->next; continue; } else { break; }
			}*/

			// Free these values if we're looping.
			if ( host != NULL )
			{
				GlobalFree( host );
				host = NULL;
			}

			if ( resource != NULL )
			{
				GlobalFree( resource );
				resource = NULL;
			}

			if ( ParseURL( ci->contact.picture_location, &host, &resource ) == true )
			{
				// Establish a connection to the server and keep it active until shutdown.
				if ( worker_con.ssl_socket == NULL && Try_Connect( &worker_con, host, cfg_connection_timeout ) == false )
				{
					// Don't keep the path if connecting to the server failed.
					GlobalFree( ci->picture_path );
					ci->picture_path = NULL;

					if ( worker_con.state == LOGGED_OUT || worker_con.state == LOGGING_OUT )
					{
						goto CLEANUP;
					}

					if ( old_ci == NULL ){ node = node->next; continue; } else { break; }
				}

				// Get the picture from the URL.
				send_buffer_length = __snprintf( worker_send_buffer, DEFAULT_BUFLEN * 2,
				"GET %s " \
				"HTTP/1.1\r\n" \
				"Host: %s\r\n" \
				"Referer: %s\r\n" \
				"App-Name: %s\r\n" \
				"User-Agent: %s\r\n" \
				"Cookie: %s\r\n" \
				"Connection: close\r\n\r\n", resource, host, REFERER, APPLICATION_NAME, USER_AGENT, wayfarer_cookies );

				if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
				{
					// Don't keep the path if downloading the file failed.
					GlobalFree( ci->picture_path );
					ci->picture_path = NULL;

					if ( worker_con.state == LOGGED_OUT || worker_con.state == LOGGING_OUT )
					{
						goto CLEANUP;
					}

					if ( old_ci == NULL ){ node = node->next; continue; } else { break; }
				}

				char *xml = _StrStrA( response, "\r\n\r\n" );
				if ( xml != NULL && *( xml + 4 ) != NULL )
				{
					xml += 4;

					// Save the image.
					DWORD write = 0;
					HANDLE hFile = CreateFile( ci->picture_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
					if ( hFile != INVALID_HANDLE_VALUE )
					{
						WriteFile( hFile, xml, content_length, &write, NULL );

						CloseHandle( hFile );

						if ( downloaded_picture == false )
						{
							downloaded_picture = true;
						}
					}
					else	// Don't keep the path if saving the file failed.
					{
						GlobalFree( ci->picture_path );
						ci->picture_path = NULL;
					}
				}
				else	// Don't keep the path if no file was downloaded.
				{
					GlobalFree( ci->picture_path );
					ci->picture_path = NULL;
				}
			}
		}

		if ( old_ci == NULL )
		{
			node = node->next;
		}
		else
		{
			break;
		}
	}

	if ( downloaded_picture == true )
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Downloaded_contact_picture_s_ )
	}

CLEANUP:

	LeaveCriticalSection( &pe_cs );

	CleanupConnection( &worker_con, host );

	GlobalFree( response );
	GlobalFree( host );
	GlobalFree( resource );
}






// old_ci and new_ci in the updateinfo struct must not be NULL.
// old_ci picture_path will be updated.
// new_ci picture_path will be freed and set to NULL.
// Returns the upload_status.
bool UploadContactPicture( updateinfo *ui )
{
	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	contactinfo *old_ci = NULL;	// Do not free this.
	contactinfo *new_ci = NULL;	// Do not free this.

	bool upload_status = false;

	if ( ui == NULL )
	{
		goto CLEANUP;
	}

	old_ci = ( contactinfo * )ui->old_ci;
	new_ci = ( contactinfo * )ui->new_ci;

	// old_ci and new_ci must not be NULL.
	if ( old_ci == NULL || new_ci == NULL )
	{
		goto CLEANUP;
	}

	char *host = "vpns.timewarnercable.com";

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Uploading_contact_picture )

	// Establish a connection to the server and keep it active until shutdown.
	if ( worker_con.ssl_socket == NULL && Try_Connect( &worker_con, host, cfg_connection_timeout ) == false )
	{
		//CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_VPNS_server ) )
		goto CLEANUP;
	}

	// Media type = 1 for picture.
	if ( UploadMedia( new_ci->picture_path, g_hWnd_contact, 1 ) == false )
	{
		goto CLEANUP;
	}

	int response_code = GetHTTPResponse( &worker_con, &response, response_length, http_status, content_length, last_buffer_size );
	if ( response_code == -1 || ( http_status != 200 && http_status != 302 ) )
	{
		if ( worker_con.state != LOGGED_OUT && worker_con.state != LOGGING_OUT )
		{
			char *error_code = NULL;	// POST requests only.
			if ( ParseXApplicationErrorCode( response, &error_code ) == true )
			{
				MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
			}
		}

		goto CLEANUP;
	}

	// If successful, get the contact's new picture location.
	char *xml = _StrStrA( response, "\r\n\r\n" );
	if ( xml != NULL && *( xml + 4 ) != NULL )
	{
		xml += 4;

		// If we got a picture location, then update the old contact's picture location.
		char *picture_location = NULL;
		if ( GetMediaLocation( xml, &picture_location ) == true )
		{
			if ( old_ci->contact.picture_location != NULL )
			{
				GlobalFree( old_ci->contact.picture_location );
			}

			old_ci->contact.picture_location = picture_location;
		}

		int send_buffer_length = 0;

		// Get the file name from the new picture path. The old picture path will be updated when/if we download the picture from the server.
		int cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, new_ci->picture_path, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
		char *utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the NULL character.
		WideCharToMultiByte( CP_UTF8, 0, new_ci->picture_path, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

		char *filename = GetFileName( utf8_cfg_val );

		char update_location_reply[ 1024 ];

		char *resource = "/vpnsservice/v1_5/contactsPics";

		int update_location_reply_length = __snprintf( update_location_reply, 1024,
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
		"<UABContactList operation=\"update\">" \
			"<User id=\"%s\" />" \
			"<ContactEntry operation=\"update\" id=\"%s\">" \
				"<Content operation=\"create\" type=\"picture\">" \
					"<Language>en</Language>" \
					"<FormatType>%s</FormatType>" \
					"<Disposition>filename=%s</Disposition>" \
					"<Location>%s</Location>" \
				"</Content>" \
			"</ContactEntry>" \
		"</UABContactList>", SAFESTRA( account_id ), SAFESTRA( old_ci->contact.contact_entry_id ), GetMIMEByFileName( filename ), SAFESTRA( filename ), SAFESTRA( old_ci->contact.picture_location ) );

		GlobalFree( utf8_cfg_val );

		send_buffer_length = ConstructVPNSPOST( worker_send_buffer, resource, update_location_reply, update_location_reply_length );

		// Update the contact's picture location on the server.
		if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
		{
			if ( worker_con.state != LOGGED_OUT && worker_con.state != LOGGING_OUT )
			{
				char *error_code = NULL;	// POST requests only.
				if ( ParseXApplicationErrorCode( response, &error_code ) == true )
				{
					MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
				}
			}

			goto CLEANUP;
		}


		// Now, download the new image from the server. This will also update the file_path for the old contact information.
		if ( cfg_download_pictures == true )
		{
			// old_ci picture_path will be updated in here.
			DownloadContactPictures( ui );

			// The new_ci picture_path is no longer needed.
			if ( new_ci->picture_path != NULL )
			{
				GlobalFree( new_ci->picture_path );

				new_ci->picture_path = NULL;
			}
		}
		else	// If we're not downloading the image, then at least set the old contact's picture_path to the new value. This will get reverted anyway during startup, but it keeps it for the session.
		{
			// Set the old_ci picture_path to the new_ci picture_path.
			if ( old_ci->picture_path != NULL )
			{
				GlobalFree( old_ci->picture_path );
			}

			old_ci->picture_path = new_ci->picture_path;

			new_ci->picture_path = NULL;	// Set to NULL so we don't accidentally free it later.
		}

		// Upload was a success.
		upload_status = true;

		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Uploaded_contact_picture )
	}

CLEANUP:

	CleanupConnection( &worker_con, host );

	GlobalFree( response );

	// If we never updated the old_ci picture_path to the new_ci picture_path, then free the new_ci picture_path.
	if ( new_ci != NULL && new_ci->picture_path != NULL && upload_status == false )
	{
		GlobalFree( new_ci->picture_path );

		new_ci->picture_path = NULL;
	}

	return upload_status;
}

THREAD_RETURN Authorization( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &cwt_cs );

	in_connection_worker_thread = true;

	char *resource = NULL;
	char *host = NULL;
	char *orignialResourceUri = "aHR0cHM6Ly92cG5zLnRpbWV3YXJuZXJjYWJsZS5jb20vdnBuc3NlcnZpY2UvdjEv";

	char ml_resource[ 256 ];
	ml_resource[ 0 ] = 0;	// Sanity.

	int saml_parameter_length = 0;
	char *saml_parameters = NULL;

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	int send_buffer_length = 0;

	host = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 29 );
	_memcpy_s( host, 29, "wayfarer.timewarnercable.com\0", 29 );
	host[ 28 ] = 0;	// Sanity.

	if ( wayfarer_cookies != NULL )
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Reauthorizing_login_information )

		send_buffer_length = __snprintf( worker_send_buffer, DEFAULT_BUFLEN * 2,
		"GET /wayfarer?originalResourceUri=%s&Ecom%%5FUser%%5FID=%s&Ecom%%5FPassword=%s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Referer: %s\r\n" \
		"User-Agent: %s\r\n" \
		"Cookie: %s\r\n" \
		"Connection: close\r\n\r\n", orignialResourceUri, encoded_username, encoded_password, host, REFERER, USER_AGENT, wayfarer_cookies );
	}
	else
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Logging_in )

		send_buffer_length = __snprintf( worker_send_buffer, DEFAULT_BUFLEN * 2,
		"GET /wayfarer?originalResourceUri=%s&Ecom%%5FUser%%5FID=%s&Ecom%%5FPassword=%s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Referer: %s\r\n" \
		"User-Agent: %s\r\n" \
		"Connection: close\r\n\r\n", orignialResourceUri, encoded_username, encoded_password, host, REFERER, USER_AGENT );
	}

	if ( Try_Connect( &worker_con, host, cfg_connection_timeout ) == false )
	{
		// Alert the user when logging in (not reauthorizing) if we failed to connect.
		if ( wayfarer_cookies == NULL )
		{
			CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT_MB( ML_ERROR, ST_A_connection_could_not_be_established ) )
		}
		//CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_wayfarer_server ) )
		goto CLEANUP;
	}

	if ( cfg_enable_message_log == true )
	{
		__snprintf( ml_resource, 256, "/wayfarer?originalResourceUri=%s&Ecom%%5FUser%%5FID=[REDACTED]&Ecom%%5FPassword=[REDACTED]", orignialResourceUri );
	}

	if ( Try_Send_Receive( &worker_con, host, ml_resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_receive_proper_status_code_from_wayfarer_server ) )
		goto CLEANUP;
	}

	CleanupConnection( &worker_con, host );

	GlobalFree( host );
	host = NULL;

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Parsing_SAML_request )

	if ( ParseSAMLForm( response, &host, &resource, &saml_parameters, saml_parameter_length ) == false )	// This will parse the SAML request.
	{
		CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_parse_SAML_request ) )
		goto CLEANUP;
	}

	if ( saml_cookies != NULL )
	{
		send_buffer_length = __snprintf( worker_send_buffer, DEFAULT_BUFLEN * 2,
		"POST %s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Referer: %s\r\n" \
		"User-Agent: %s\r\n" \
		"Cookie: %s\r\n" \
		"Content-Type: application/x-www-form-urlencoded\r\n" \
		"Content-Length: %d\r\n" \
		"Connection: keep-alive\r\n\r\n" \
		"%s", resource, host, REFERER, USER_AGENT, saml_cookies, saml_parameter_length, saml_parameters );
	}
	else
	{
		send_buffer_length = __snprintf( worker_send_buffer, DEFAULT_BUFLEN * 2,
		"POST %s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Referer: %s\r\n" \
		"User-Agent: %s\r\n" \
		"Content-Type: application/x-www-form-urlencoded\r\n" \
		"Content-Length: %d\r\n" \
		"Connection: keep-alive\r\n\r\n" \
		"%s", resource, host, REFERER, USER_AGENT, saml_parameter_length, saml_parameters );
	}

	GlobalFree( saml_parameters );
	saml_parameters = NULL;

	if ( Try_Connect( &worker_con, host, cfg_connection_timeout ) == false )
	{
		//CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_SAML_server ) )
		goto CLEANUP;
	}

	if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_receive_proper_status_code_from_SAML_server ) )
		goto CLEANUP;
	}

	char *new_saml_cookies = NULL;

	if ( ParseCookies( response, &saml_cookie_tree, &new_saml_cookies ) == false )
	{
		GlobalFree( new_saml_cookies );
		new_saml_cookies = NULL;

		CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_parse_server_cookies ) )
		goto CLEANUP;
	}

	// If we got a new cookie.
	if ( new_saml_cookies != NULL )
	{
		// Then see if the new cookie is not blank.
		if ( new_saml_cookies[ 0 ] != NULL )
		{
			// If it's not, then free the old cookie and update it to the new one.
			GlobalFree( saml_cookies );
			saml_cookies = new_saml_cookies;
		}
		else	// Otherwise, if the cookie is blank, then free it.
		{
			GlobalFree( new_saml_cookies );
		}
	}

	// HTTP redirect. Follow it.
	if ( http_status == 302 )
	{
		GlobalFree( resource );
		resource = NULL;
		GlobalFree( host );
		host = NULL;

		if ( ParseRedirect( response, &host, &resource ) == false )
		{
			CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_Incorrect_username_or_password ) )
			goto CLEANUP;
		}

		send_buffer_length = __snprintf( worker_send_buffer, DEFAULT_BUFLEN * 2,
		"GET %s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Referer: %s\r\n" \
		"User-Agent: %s\r\n" \
		"Cookie: %s\r\n" \
		"Connection: close\r\n\r\n", resource, host, REFERER, USER_AGENT, saml_cookies );

		if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
		{
			CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_receive_proper_status_code_from_SAML_server ) )
			goto CLEANUP;
		}
	}

	CleanupConnection( &worker_con, host );

	GlobalFree( resource );
	resource = NULL;
	GlobalFree( host );
	host = NULL;

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Parsing_SAML_response )

	if ( ParseSAMLForm( response, &host, &resource, &saml_parameters, saml_parameter_length ) == false )	// This will parse the SAML response.
	{
		CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_parse_SAML_response ) )
		goto CLEANUP;
	}

	if ( wayfarer_cookies != NULL )
	{
		send_buffer_length = __snprintf( worker_send_buffer, DEFAULT_BUFLEN * 2,
		"POST %s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Referer: %s\r\n" \
		"User-Agent: %s\r\n" \
		"Cookie: %s\r\n" \
		"Content-Type: application/x-www-form-urlencoded\r\n" \
		"Content-Length: %d\r\n" \
		"Connection: close\r\n\r\n" \
		"%s", resource, host, REFERER, USER_AGENT, wayfarer_cookies, saml_parameter_length, saml_parameters );
	}
	else
	{
		send_buffer_length = __snprintf( worker_send_buffer, DEFAULT_BUFLEN * 2,
		"POST %s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Referer: %s\r\n" \
		"User-Agent: %s\r\n" \
		"Content-Type: application/x-www-form-urlencoded\r\n" \
		"Content-Length: %d\r\n" \
		"Connection: close\r\n\r\n" \
		"%s", resource, host, REFERER, USER_AGENT, saml_parameter_length, saml_parameters );
	}

	GlobalFree( saml_parameters );
	saml_parameters = NULL;

	if ( Try_Connect( &worker_con, host, cfg_connection_timeout ) == false )
	{
		//CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_VPNS_server ) )
		goto CLEANUP;
	}

	if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_receive_proper_status_code_from_VPNS_server ) )
		goto CLEANUP;
	}

	CleanupConnection( &worker_con, host );

	GlobalFree( resource );
	resource = NULL;
	GlobalFree( host );
	host = NULL;

	char *new_wayfarer_cookies = NULL;

	// This value will be saved
	if ( ParseCookies( response, &wayfarer_cookie_tree, &new_wayfarer_cookies ) == false )
	{
		GlobalFree( new_wayfarer_cookies );
		new_wayfarer_cookies = NULL;

		CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_parse_server_cookies ) )
		goto CLEANUP;
	}

	// If we got a new cookie.
	if ( new_wayfarer_cookies != NULL )
	{
		// Then see if the new cookie is not blank.
		if ( new_wayfarer_cookies[ 0 ] != NULL )
		{
			// If it's not, then free the old cookie and update it to the new one.
			GlobalFree( wayfarer_cookies );
			wayfarer_cookies = new_wayfarer_cookies;
		}
		else	// Otherwise, if the cookie is blank, then free it.
		{
			GlobalFree( new_wayfarer_cookies );
		}
	}

	// The response header will also give a Location field value of: https://vpns.timewarnercable.com/vpnsservice/v1/. It's outdated though.
	// We'll use https://vpns.timewarnercable.com/vpnsservice/v1_5/ instead.

CLEANUP:

	CleanupConnection( &worker_con, host );

	GlobalFree( response );
	GlobalFree( saml_parameters );
	GlobalFree( resource );
	GlobalFree( host );

	// Release the semaphore if we're killing the thread.
	if ( connection_worker_semaphore != NULL )
	{
		ReleaseSemaphore( connection_worker_semaphore, 1, NULL );
	}

	in_connection_worker_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &cwt_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN CallPhoneNumber( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &cit_cs );

	in_connection_incoming_thread = true;

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	// ci and its values will be freed at the end of this thread.
	callerinfo *ci = ( callerinfo * )pArguments;

	if ( ci == NULL )
	{
		goto CLEANUP;
	}

	int send_buffer_length = 0;

	char *resource = "/vpnsservice/v1_5/clicktocall";
	char *host = "vpns.timewarnercable.com";

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Initiating_outbound_call )

	// Establish a connection to the server and keep it active until shutdown.
	if ( incoming_con.ssl_socket == NULL && Try_Connect( &incoming_con, host, cfg_connection_timeout ) == false )
	{
		//CHECK_CONNECTION_STATE( incoming_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_VPNS_server ) )
		goto CLEANUP;
	}

	char phone_call_reply[ 1024 ];

	int phone_call_reply_length = __snprintf( phone_call_reply, 1024,
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
	"<CallControl>" \
		"<Registration>" \
			"<Client id=\"%s\"></Client>" \
			"<Service id=\"%s\" idType=\"telephone\" />" \
		"</Registration>" \
		"<Call operation=\"create\">" \
			"<To id=\"%s\" name=\"\" />" \
			"<From id=\"%s\" name=\"\" />" \
		"</Call>" \
	"</CallControl>", SAFESTRA( client_id ), ( service_phone_number != NULL ? SAFESTRA( service_phone_number[ current_phone_line ] ) : "" ), SAFESTRA( ci->call_to ), SAFESTRA( ci->call_from ) );

	send_buffer_length = ConstructVPNSPOST( incoming_send_buffer, resource, phone_call_reply, phone_call_reply_length );

	// HTTP status error 400 indicates a bad request. Here it means that the area code was restricted, or bad.
	if ( Try_Send_Receive( &incoming_con, host, resource, incoming_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		if ( incoming_con.state != LOGGED_OUT && incoming_con.state != LOGGING_OUT )
		{
			char *error_code = NULL;	// POST requests only.
			if ( ParseXApplicationErrorCode( response, &error_code ) == true )
			{
				MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
			}
		}
	}
	else
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Initiated_outbound_call )
	}

CLEANUP:

	// Close the connection since no more data will be sent or received.
	CleanupConnection( &incoming_con, host );

	GlobalFree( response );

	GlobalFree( ci->call_from );
	GlobalFree( ci->call_to );
	GlobalFree( ci->call_reference_id );
	GlobalFree( ci->caller_id );
	GlobalFree( ci->forward_to );
	GlobalFree( ci );

	// Release the semaphore if we're killing the thread.
	if ( connection_incoming_semaphore != NULL )
	{
		ReleaseSemaphore( connection_incoming_semaphore, 1, NULL );
	}

	in_connection_incoming_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &cit_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN ForwardIncomingCall( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &cit_cs );

	in_connection_incoming_thread = true;

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	callerinfo *ci = ( callerinfo * )pArguments;

	if ( ci == NULL )
	{
		goto CLEANUP;
	}

	int send_buffer_length = 0;

	char *resource = "/vpnsservice/v1_5/incomingcall/";
	char *host = "vpns.timewarnercable.com";

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Forwarding_incoming_call )

	// Establish a connection to the server and keep it active until shutdown.
	if ( incoming_con.ssl_socket == NULL && Try_Connect( &incoming_con, host, cfg_connection_timeout ) == false )
	{
		//CHECK_CONNECTION_STATE( incoming_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_VPNS_server ) )
		goto CLEANUP;
	}

	char forward_reply[ 1024 ];

	int forward_reply_length = __snprintf( forward_reply, 1024,
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
	"<CallControl>" \
		"<Registration>" \
			"<Client id=\"%s\" />" \
			"<Service id=\"%s\" idType=\"telephone\" />" \
		"</Registration>" \
		"<Call operation=\"forward\">" \
			"<To id=\"%s\" name=\"\" />" \
			"<From id=\"%s\" name=\"\" />" \
			"<ForwardNumber>%s</ForwardNumber>" \
			"<CallReferenceId>%s</CallReferenceId>" \
		"</Call>" \
	"</CallControl>", SAFESTRA( client_id ), ( service_phone_number != NULL ? SAFESTRA( service_phone_number[ current_phone_line ] ) : "" ), SAFESTRA( ci->call_to ), SAFESTRA( ci->call_from ), SAFESTRA( ci->forward_to ), SAFESTRA( ci->call_reference_id ) );

	send_buffer_length = ConstructVPNSPOST( incoming_send_buffer, resource, forward_reply, forward_reply_length );

	if ( Try_Send_Receive( &incoming_con, host, resource, incoming_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		if ( incoming_con.state != LOGGED_OUT && incoming_con.state != LOGGING_OUT )
		{
			char *error_code = NULL;	// POST requests only.
			if ( ParseXApplicationErrorCode( response, &error_code ) == true )
			{
				MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
			}
		}
	}
	else
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Forwarded_incoming_call )
	}

CLEANUP:

	CleanupConnection( &incoming_con, host );

	GlobalFree( response );

	// Release the semaphore if we're killing the thread.
	if ( connection_incoming_semaphore != NULL )
	{
		ReleaseSemaphore( connection_incoming_semaphore, 1, NULL );
	}

	in_connection_incoming_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &cit_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN IgnoreIncomingCall( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &cit_cs );

	in_connection_incoming_thread = true;

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	callerinfo *ci = ( callerinfo * )pArguments;

	if ( ci == NULL )
	{
		goto CLEANUP;
	}

	int send_buffer_length = 0;

	// Note the / at the end.
	char *resource = "/vpnsservice/v1_5/incomingcall/";
	char *host = "vpns.timewarnercable.com";

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Ignoring_incoming_call )

	// Establish a connection to the server and keep it active until shutdown.
	if ( incoming_con.ssl_socket == NULL && Try_Connect( &incoming_con, host, cfg_connection_timeout ) == false )
	{
		//CHECK_CONNECTION_STATE( incoming_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_VPNS_server ) )
		goto CLEANUP;
	}

	char ignore_reply[ 1024 ];

	int ignore_reply_length = __snprintf( ignore_reply, 1024,
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
	"<CallControl>" \
		"<Registration>" \
			"<Client id=\"%s\" />" \
			"<Service id=\"%s\" idType=\"telephone\" />" \
		"</Registration>" \
		"<Call operation=\"ignore\">" \
			"<To id=\"%s\" />" \
			"<From id=\"%s\" />" \
			"<CallReferenceId>%s</CallReferenceId>" \
		"</Call>" \
	"</CallControl>", SAFESTRA( client_id ), ( service_phone_number != NULL ? SAFESTRA( service_phone_number[ current_phone_line ] ) : "" ), SAFESTRA( ci->call_to ), SAFESTRA( ci->call_from ), SAFESTRA( ci->call_reference_id ) );

	send_buffer_length = ConstructVPNSPOST( incoming_send_buffer, resource, ignore_reply, ignore_reply_length );

	if ( Try_Send_Receive( &incoming_con, host, resource, incoming_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		if ( incoming_con.state != LOGGED_OUT && incoming_con.state != LOGGING_OUT )
		{
			char *error_code = NULL;	// POST requests only.
			if ( ParseXApplicationErrorCode( response, &error_code ) == true )
			{
				MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
			}
		}
	}
	else
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Ignored_incoming_call )
	}

CLEANUP:

	CleanupConnection( &incoming_con, host );

	GlobalFree( response );

	// Release the semaphore if we're killing the thread.
	if ( connection_incoming_semaphore != NULL )
	{
		ReleaseSemaphore( connection_incoming_semaphore, 1, NULL );
	}

	in_connection_incoming_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &cit_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN UpdateRegistration( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &cwt_cs );

	in_connection_worker_thread = true;

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	int send_buffer_length = 0;

	char *resource = "/vpnsservice/v1_5/registration";
	char *host = "vpns.timewarnercable.com";

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Updating_registration )

	// Establish a connection to the server and keep it active until shutdown.
	if ( worker_con.ssl_socket == NULL && Try_Connect( &worker_con, host, cfg_connection_timeout ) == false )
	{
		//CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_VPNS_server ) )
		goto CLEANUP;
	}

	char update_registration[ 1024 ];

	int update_registration_reply_length = __snprintf( update_registration, 1024,
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
	"<Registration operation=\"create\" id=\"%s\">" \
		"<Principal id=\"%s\" />" \
		"<Device id=\"%s\" />" \
		"<Client id=\"%s\" />" \
	"</Registration>", SAFESTRA( principal_id ), SAFESTRA( principal_id ), SAFESTRA( device_id ), SAFESTRA( client_id ) );

	send_buffer_length = ConstructVPNSPOST( worker_send_buffer, resource, update_registration, update_registration_reply_length );

	// Update registration
	if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		if ( worker_con.state != LOGGED_OUT && worker_con.state != LOGGING_OUT )
		{ 
			char *error_code = NULL;	// POST requests only.
			if ( ParseXApplicationErrorCode( response, &error_code ) == true )
			{
				MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
			}
		}
	}
	else
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Updated_registration )
	}

CLEANUP:

	CleanupConnection( &worker_con, host );

	GlobalFree( response );

	// Release the semaphore if we're killing the thread.
	if ( connection_worker_semaphore != NULL )
	{
		ReleaseSemaphore( connection_worker_semaphore, 1, NULL );
	}

	in_connection_worker_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &cwt_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN DeleteContact( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &cwt_cs );

	in_connection_worker_thread = true;

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	// The contact we want to delete.
	contactinfo *ci = ( contactinfo * )pArguments;

	// This should stop the user from modifying the contact selection if our connection takes a while to be established.
	Processing_Window( g_hWnd_contact_list, false );

	if ( ci == NULL )
	{
		goto CLEANUP;
	}

	int send_buffer_length = 0;

	char *resource = "/vpnsservice/v1_5/contacts";
	char *host = "vpns.timewarnercable.com";

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Deleting_contact )

	// Establish a connection to the server and keep it active until shutdown.
	if ( worker_con.ssl_socket == NULL && Try_Connect( &worker_con, host, cfg_connection_timeout ) == false )
	{
		//CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_VPNS_server ) )
		goto CLEANUP;
	}

	char delete_contact[ 1024 ];

	int delete_reply_length = __snprintf( delete_contact, 1024,
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
	"<UABContactList operation=\"update\">" \
		"<User id=\"%s\" />" \
		"<ContactEntry operation=\"delete\" id=\"%s\"></ContactEntry>" \
	"</UABContactList>", SAFESTRA( account_id ), SAFESTRA( ci->contact.contact_entry_id ) );

	send_buffer_length = ConstructVPNSPOST( worker_send_buffer, resource, delete_contact, delete_reply_length );

	if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		bool delete_status = false;

		if ( worker_con.state != LOGGED_OUT && worker_con.state != LOGGING_OUT )
		{
			char *error_code = NULL;	// POST requests only.
			if ( ParseXApplicationErrorCode( response, &error_code ) == true )
			{
				// "Error - UAB 14004:[AB-CONTACT] Contact not found."
				char *error_number = _StrStrA( error_code, "UAB 14004:" );
				
				// If the contact was not found, then delete it locally anyway.
				if ( error_number != NULL )
				{
					delete_status = true;
				}

				MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
			}
		}

		if ( delete_status == false )
		{
			goto CLEANUP;
		}
	}

	// Should probably check for: x-application-error-code == "Success"

	removeinfo *ri = ( removeinfo * )GlobalAlloc( GMEM_FIXED, sizeof( removeinfo ) );
	ri->disable_critical_section = false;
	ri->hWnd = g_hWnd_contact_list;

	// ri will be freed in the remove_items thread.
	HANDLE hThread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL );

	// Make sure the removal completes before we call Processing_Window below. Shouldn't take long.
	if ( hThread != NULL )
	{
		WaitForSingleObject( hThread, INFINITE );
		CloseHandle( hThread );
	}

CLEANUP:

	CleanupConnection( &worker_con, host );

	GlobalFree( response );

	Processing_Window( g_hWnd_contact_list, true );

	// Release the semaphore if we're killing the thread.
	if ( connection_worker_semaphore != NULL )
	{
		ReleaseSemaphore( connection_worker_semaphore, 1, NULL );
	}

	in_connection_worker_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &cwt_cs );

	_ExitThread( 0 );
	return 0;
}

// old_ci and new_ci in the updateinfo struct must not be NULL.
// 1. Update a contact if new information is supplied.
// 2. Parse the response information.
// 3. Upload a picture if it was supplied.
// 4. Update the contact in the contact listview.
THREAD_RETURN UpdateContactInformation( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &cwt_cs );

	in_connection_worker_thread = true;

	if ( g_hWnd_contact != NULL )
	{
		_SendMessageW( g_hWnd_contact, WM_PROPAGATE, MAKEWPARAM( CW_UPDATE, UPDATE_BEGIN ), 0 );
	}

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	updateinfo *ui = NULL;
	contactinfo *old_ci = NULL;	// Do not free this.
	contactinfo *new_ci = NULL;

	bool picture_only = false;	// If we only update the picture.
	bool update_status = false;
	bool picture_status = false;

	ui = ( updateinfo * )pArguments;

	if ( ui == NULL )
	{
		goto CLEANUP;
	}

	old_ci = ( contactinfo * )ui->old_ci;
	new_ci = ( contactinfo * )ui->new_ci;

	// old_ci and new_ci must not be NULL.
	if ( old_ci == NULL || new_ci == NULL )
	{
		goto CLEANUP;
	}

	picture_only = ui->picture_only;

	int send_buffer_length = 0;

	char *resource = "/vpnsservice/v1_5/contacts";
	char *host = "vpns.timewarnercable.com";

	char ml_resource[ 256 ];
	ml_resource[ 0 ] = 0;	// Sanity.

	int update_contact_reply_length = 0;
	char update_contact[ 2048 ];

	if ( picture_only == false )
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Updating_contact_information )
	}

	// Establish a connection to the server and keep it active until shutdown.
	if ( worker_con.ssl_socket == NULL && Try_Connect( &worker_con, host, cfg_connection_timeout ) == false )
	{
		//CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_VPNS_server ) )
		goto CLEANUP;
	}

	// Update the old contact with its new information. Skip this if we're only updating the picture.
	if ( picture_only == false )
	{
		char *o_title = encode_xml_entities( old_ci->contact.title );
		char *o_first_name = encode_xml_entities( old_ci->contact.first_name );
		char *o_last_name = encode_xml_entities( old_ci->contact.last_name );
		char *o_nickname = encode_xml_entities( old_ci->contact.nickname );

		char *o_business_name = encode_xml_entities( old_ci->contact.business_name );
		char *o_designation = encode_xml_entities( old_ci->contact.designation );
		char *o_department = encode_xml_entities( old_ci->contact.department );
		char *o_category = encode_xml_entities( old_ci->contact.category );
		
		char *o_email_address = encode_xml_entities( old_ci->contact.email_address );
		char *o_web_page = encode_xml_entities( old_ci->contact.web_page );

		char *n_title = encode_xml_entities( new_ci->contact.title );
		char *n_first_name = encode_xml_entities( new_ci->contact.first_name );
		char *n_last_name = encode_xml_entities( new_ci->contact.last_name );
		char *n_nickname = encode_xml_entities( new_ci->contact.nickname );

		char *n_business_name = encode_xml_entities( new_ci->contact.business_name );
		char *n_designation = encode_xml_entities( new_ci->contact.designation );
		char *n_department = encode_xml_entities( new_ci->contact.department );
		char *n_category = encode_xml_entities( new_ci->contact.category );
		
		char *n_email_address = encode_xml_entities( new_ci->contact.email_address );
		char *n_web_page = encode_xml_entities( new_ci->contact.web_page );

		// Update the contact's information.
		// type="fax" is a dummy value.
		update_contact_reply_length = __snprintf( update_contact, 2048,
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
		"<UABContactList operation=\"update\">" \
			"<User id=\"%s\" />" \
			"<ContactEntry operation=\"update\" id=\"%s\">" \
				"<Title>%s</Title>" \
				"<FirstName>%s</FirstName>" \
				"<LastName>%s</LastName>" \
				"<NickName>%s</NickName>" \
				"<BusinessName>%s</BusinessName>" \
				"<Designation>%s</Designation>" \
				"<Department>%s</Department>" \
				"<Category>%s</Category>" \
				"<PhoneNumber id=\"%s\" type=\"home\">" \
					"<Number>%s</Number>" \
				"</PhoneNumber>" \
				"<PhoneNumber id=\"%s\" type=\"work\">" \
					"<Number>%s</Number>" \
				"</PhoneNumber>" \
				"<PhoneNumber id=\"%s\" type=\"office\">" \
					"<Number>%s</Number>" \
				"</PhoneNumber>" \
				"<PhoneNumber id=\"%s\" type=\"cell\">" \
					"<Number>%s</Number>" \
				"</PhoneNumber>" \
				"<PhoneNumber id=\"%s\" type=\"fax\">" \
					"<Number>%s</Number>" \
				"</PhoneNumber>" \
				"<PhoneNumber id=\"%s\" type=\"other\">" \
					"<Number>%s</Number>" \
				"</PhoneNumber>" \
				"<Email id=\"%s\" type=\"home\">" \
					"<Address>%s</Address>" \
				"</Email>" \
				"<Web id=\"%s\" type=\"home\">" \
					"<Url>%s</Url>" \
				"</Web>" \
				"<RingTone>%s</RingTone>" \
			"</ContactEntry>" \
		"</UABContactList>",
		SAFESTRA( account_id ), SAFESTRA( old_ci->contact.contact_entry_id ),
		SAFESTR2A( ( n_title != NULL ? n_title : new_ci->contact.title ), ( o_title != NULL ? o_title : old_ci->contact.title ) ),
		SAFESTR2A( ( n_first_name != NULL ? n_first_name : new_ci->contact.first_name ), ( o_first_name != NULL ? o_first_name : old_ci->contact.first_name ) ),
		SAFESTR2A( ( n_last_name != NULL ? n_last_name : new_ci->contact.last_name ), ( o_last_name != NULL ? o_last_name : old_ci->contact.last_name ) ),
		SAFESTR2A( ( n_nickname != NULL ? n_nickname : new_ci->contact.nickname ), ( o_nickname != NULL ? o_nickname : old_ci->contact.nickname ) ),
		SAFESTR2A( ( n_business_name != NULL ? n_business_name : new_ci->contact.business_name ), ( o_business_name != NULL ? o_business_name : old_ci->contact.business_name ) ),
		SAFESTR2A( ( n_designation != NULL ? n_designation : new_ci->contact.designation ), ( o_designation != NULL ? o_designation : old_ci->contact.designation ) ),
		SAFESTR2A( ( n_department != NULL ? n_department : new_ci->contact.department ), ( o_department != NULL ? o_department : old_ci->contact.department ) ),
		SAFESTR2A( ( n_category != NULL ? n_category : new_ci->contact.category ), ( o_category != NULL ? o_category : old_ci->contact.category ) ),
		SAFESTRA( old_ci->contact.home_phone_number_id ),
		SAFESTR2A( new_ci->contact.home_phone_number, old_ci->contact.home_phone_number ),
		SAFESTRA( old_ci->contact.work_phone_number_id ),
		SAFESTR2A( new_ci->contact.work_phone_number, old_ci->contact.work_phone_number ),
		SAFESTRA( old_ci->contact.office_phone_number_id ),
		SAFESTR2A( new_ci->contact.office_phone_number, old_ci->contact.office_phone_number ),
		SAFESTRA( old_ci->contact.cell_phone_number_id ),
		SAFESTR2A( new_ci->contact.cell_phone_number, old_ci->contact.cell_phone_number ),
		SAFESTRA( old_ci->contact.fax_number_id ),
		SAFESTR2A( new_ci->contact.fax_number, old_ci->contact.fax_number ),
		SAFESTRA( old_ci->contact.other_phone_number_id ),
		SAFESTR2A( new_ci->contact.other_phone_number, old_ci->contact.other_phone_number ),
		SAFESTRA( old_ci->contact.email_address_id ),
		SAFESTR2A( ( n_email_address != NULL ? n_email_address : new_ci->contact.email_address ), ( o_email_address != NULL ? o_email_address : old_ci->contact.email_address ) ),
		SAFESTRA( old_ci->contact.web_page_id ),
		SAFESTR2A( ( n_web_page != NULL ? n_web_page : new_ci->contact.web_page ), ( o_web_page != NULL ? o_web_page : old_ci->contact.web_page ) ),
		SAFESTRA( old_ci->contact.ringtone ) );

		GlobalFree( o_title );
		GlobalFree( o_first_name );
		GlobalFree( o_last_name );
		GlobalFree( o_nickname );

		GlobalFree( o_business_name );
		GlobalFree( o_designation );
		GlobalFree( o_department );
		GlobalFree( o_category );

		GlobalFree( o_email_address );
		GlobalFree( o_web_page );

		GlobalFree( n_title );
		GlobalFree( n_first_name );
		GlobalFree( n_last_name );
		GlobalFree( n_nickname );

		GlobalFree( n_business_name );
		GlobalFree( n_designation );
		GlobalFree( n_department );
		GlobalFree( n_category );

		GlobalFree( n_email_address );
		GlobalFree( n_web_page );

		send_buffer_length = ConstructVPNSPOST( worker_send_buffer, resource, update_contact, update_contact_reply_length );

		if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
		{
			if ( worker_con.state != LOGGED_OUT && worker_con.state != LOGGING_OUT )
			{
				char *error_code = NULL;	// POST requests only.
				if ( ParseXApplicationErrorCode( response, &error_code ) == true )
				{
					MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
				}
			}

			goto CLEANUP;
		}

		// Now parse our contact's information if we added any values.
		char *xml = _StrStrA( response, "\r\n\r\n" );
		if ( xml != NULL && *( xml + 4 ) != NULL )
		{
			// We can't reliably parse the contact information IDs from this first response.
			// The elements don't include their "type" attribute and can technically be in any order.
			// So we'll request the full information instead.

			// Request a single contact's information.
			send_buffer_length = __snprintf( worker_send_buffer, DEFAULT_BUFLEN * 2,
			"GET %s?UserId=%s&ContactId=%s " \
			"HTTP/1.1\r\n" \
			"Host: %s\r\n" \
			"Referer: %s\r\n" \
			"App-Name: %s\r\n" \
			"User-Agent: %s\r\n" \
			"Cookie: %s; %s\r\n" \
			"Connection: close\r\n\r\n", resource, account_id, old_ci->contact.contact_entry_id, host, REFERER, APPLICATION_NAME, USER_AGENT, vpns_cookies, wayfarer_cookies );

			if ( cfg_enable_message_log == true )
			{
				__snprintf( ml_resource, 256, "%s?UserId=[REDACTED]&ContactId=%s", resource, old_ci->contact.contact_entry_id );
			}

			if ( Try_Send_Receive( &worker_con, host, ml_resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
			{
				update_status = false;	// Signals the cleanup to free ci.

				CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_receive_proper_status_code_when_requesting_contact_information ) )
				goto CLEANUP;
			}

			// Now that we have the information we need. We can parse the IDs correctly.
			xml = _StrStrA( response, "\r\n\r\n" );
			if ( xml != NULL && *( xml + 4 ) != NULL )
			{
				xml += 4;

				// Updates the element IDs.
				update_status = UpdateConactInformationIDs( xml, new_ci );
			}
			else
			{
				update_status = false;
			}
		}
		else	// If there's no response, then either a value was removed or no additional values were added and the update was successful.
		{
			// If a value was removed and it had an ID, then we'll need to free the ID. (Done in update_contact_list)
			update_status = true;
		}
	}

	// Here we're either uploading a picture, or removing one.

	// Upload the picture if we didn't remove it instead.
	if ( ui->remove_picture == false )
	{
		// Upload the new picture, and then download it from the server.
		// old_ci picture_path will be updated here.
		// new_ci picture_path will be freed and set to NULL here.
		if ( new_ci->picture_path != NULL )
		{
			picture_status = UploadContactPicture( ui );
		}
		else	// If new_ci picture_path is NULL, then we don't want to update it.
		{
			picture_status = true;	// Assume it succeeded.
		}
	}
	else	// Remove the contact's picture.
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Removing_contact_picture )

		update_contact_reply_length = __snprintf( update_contact, 1024,
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
		"<UABContactList operation=\"update\">" \
			"<User id=\"%s\" />" \
			"<ContactEntry operation=\"update\" id=\"%s\">" \
				"<Content operation=\"delete\" type=\"picture\"></Content>" \
			"</ContactEntry>" \
		"</UABContactList>", SAFESTRA( account_id ), SAFESTRA( old_ci->contact.contact_entry_id ) );

		send_buffer_length = ConstructVPNSPOST( worker_send_buffer, resource, update_contact, update_contact_reply_length );

		if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
		{
			if ( worker_con.state != LOGGED_OUT && worker_con.state != LOGGING_OUT )
			{
				char *error_code = NULL;	// POST requests only.
				if ( ParseXApplicationErrorCode( response, &error_code ) == true )
				{
					MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
				}
			}
		}
		else	// Remove the old_ci picture path/location from the contact.
		{
			MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Removed_contact_picture )

			GlobalFree( old_ci->picture_path );
			old_ci->picture_path = NULL;

			GlobalFree( old_ci->contact.picture_location );
			old_ci->contact.picture_location = NULL;

			picture_status = true;
		}
	}

	// If our update succeeded, then update our contact list.
	if ( update_status == true )
	{
		// ui and/or new_ci will be freed in the update_contact_list thread.
		CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_contact_list, ( void * )ui, 0, NULL ) );
	}

CLEANUP:

	CleanupConnection( &worker_con, host );

	GlobalFree( response );

	// If something went wrong and we never received a reply from the server, then free the new contactinfo structure.
	if ( new_ci != NULL && update_status == false )
	{
		free_contactinfo( &new_ci );
	}

	// If something went wrong and we never received a reply from the server, then free the new updateinfo structure.
	if ( ui != NULL && update_status == false )
	{
		GlobalFree( ui );
	}

	// Release the semaphore if we're killing the thread.
	if ( connection_worker_semaphore != NULL )
	{
		ReleaseSemaphore( connection_worker_semaphore, 1, NULL );
	}

	if ( g_hWnd_contact != NULL )
	{
		_SendMessageW( g_hWnd_contact, WM_PROPAGATE, MAKEWPARAM( CW_UPDATE, ( ( ( ( picture_only == false && update_status == false ) || picture_status == false ) && contact_update_in_progress != UPDATE_FAIL ) ? UPDATE_FAIL : UPDATE_END ) ), 0 );
	}

	in_connection_worker_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &cwt_cs );

	_ExitThread( 0 );
	return 0;
}

// 1. Get a contact's/all contacts information.
// 2. Parse the response information.
// 3. Download the contact/contacts pictures.
// 4. Add the contact/contacts to the contact listview.
THREAD_RETURN ManageContactInformation( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &cwt_cs );

	in_connection_worker_thread = true;

	int send_buffer_length = 0;

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	manageinfo *mi = NULL;
	contactinfo *ci = NULL;

	bool add_status = false;	// Signals the cleanup to free ci.

	mi = ( manageinfo * )pArguments;

	if ( mi == NULL )
	{
		goto CLEANUP;
	}

	ci = mi->ci;

	// If we want to add a single contact, ci must not be null.
	// If we want to add all contacts, ci must be null.
	if ( ( mi->manage_type == 1 && ci == NULL ) || ( mi->manage_type == 0 && ci != NULL ) )
	{
		goto CLEANUP;
	}

	char *resource = "/vpnsservice/v1_5/contacts";
	char *host = "vpns.timewarnercable.com";

	char ml_resource[ 256 ];
	ml_resource[ 0 ] = 0;	// Sanity.

	if ( mi->manage_type == 0 )
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Requesting_contact_list )
	}
	else if ( mi->manage_type == 1 )
	{
		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Adding_contact )
	}

	// Establish a connection to the server and keep it active until shutdown.
	if ( worker_con.ssl_socket == NULL && Try_Connect( &worker_con, host, cfg_connection_timeout ) == false )
	{
		//CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_VPNS_server ) )
		goto CLEANUP;
	}

	
	// Request all the contacts information.
	if ( mi->manage_type == 0 )
	{
		send_buffer_length = __snprintf( worker_send_buffer, DEFAULT_BUFLEN * 2,
		"GET %s?UserId=%s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Referer: %s\r\n" \
		"App-Name: %s\r\n" \
		"User-Agent: %s\r\n" \
		"Cookie: %s; %s\r\n" \
		"Connection: close\r\n\r\n", resource, account_id, host, REFERER, APPLICATION_NAME, USER_AGENT, vpns_cookies, wayfarer_cookies );

		if ( cfg_enable_message_log == true )
		{
			__snprintf( ml_resource, 256, "%s?UserId=[REDACTED]", resource );
		}
	}
	else if ( mi->manage_type == 1 )	// Add a single contact to our contact list.
	{
		char add_contact[ 2048 ];

		char *t_title = encode_xml_entities( ci->contact.title );
		char *t_first_name = encode_xml_entities( ci->contact.first_name );
		char *t_last_name = encode_xml_entities( ci->contact.last_name );
		char *t_nickname = encode_xml_entities( ci->contact.nickname );

		char *t_business_name = encode_xml_entities( ci->contact.business_name );
		char *t_designation = encode_xml_entities( ci->contact.designation );
		char *t_department = encode_xml_entities( ci->contact.department );
		char *t_category = encode_xml_entities( ci->contact.category );
		
		char *t_email_address = encode_xml_entities( ci->contact.email_address );
		char *t_web_page = encode_xml_entities( ci->contact.web_page );

		// type="fax" is a dummy value.
		int add_contact_length = __snprintf( add_contact, 2048,
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
		"<UABContactList operation=\"create\">" \
			"<User id=\"%s\" />" \
			"<ContactEntry>" \
				"<Title>%s</Title>" \
				"<FirstName>%s</FirstName>" \
				"<LastName>%s</LastName>" \
				"<NickName>%s</NickName>" \
				"<BusinessName>%s</BusinessName>" \
				"<Designation>%s</Designation>" \
				"<Department>%s</Department>" \
				"<Category>%s</Category>" \
				"<PhoneNumber type=\"home\">" \
					"<Number>%s</Number>" \
				"</PhoneNumber>" \
				"<PhoneNumber type=\"work\">" \
					"<Number>%s</Number>" \
				"</PhoneNumber>" \
				"<PhoneNumber type=\"office\">" \
					"<Number>%s</Number>" \
				"</PhoneNumber>" \
				"<PhoneNumber type=\"cell\">" \
					"<Number>%s</Number>" \
				"</PhoneNumber>" \
				"<PhoneNumber type=\"fax\">" \
					"<Number>%s</Number>" \
				"</PhoneNumber>" \
				"<PhoneNumber type=\"other\">" \
					"<Number>%s</Number>" \
				"</PhoneNumber>" \
				"<Email type=\"home\">" \
					"<Address>%s</Address>" \
				"</Email>" \
				"<Web type=\"home\">" \
					"<Url>%s</Url>" \
				"</Web>" \
				"<RingTone>%s</RingTone>" \
			"</ContactEntry>" \
			"</UABContactList>",
			SAFESTRA( account_id ), SAFESTR2A( t_title, ci->contact.title ), SAFESTR2A( t_first_name, ci->contact.first_name ), SAFESTR2A( t_last_name, ci->contact.last_name ), SAFESTR2A( t_nickname, ci->contact.nickname ),
			SAFESTR2A( t_business_name, ci->contact.business_name ), SAFESTR2A( t_designation, ci->contact.designation ), SAFESTR2A( t_department, ci->contact.department ), SAFESTR2A( t_category, ci->contact.category ),
			SAFESTRA( ci->contact.home_phone_number ), SAFESTRA( ci->contact.work_phone_number ), SAFESTRA( ci->contact.office_phone_number ), SAFESTRA( ci->contact.cell_phone_number ), SAFESTRA( ci->contact.fax_number ), SAFESTRA( ci->contact.other_phone_number ),
			SAFESTR2A( t_email_address, ci->contact.email_address ), SAFESTR2A( t_web_page, ci->contact.web_page ), SAFESTRA( ci->contact.ringtone ) );

		GlobalFree( t_title );
		GlobalFree( t_first_name );
		GlobalFree( t_last_name );
		GlobalFree( t_nickname );

		GlobalFree( t_business_name );
		GlobalFree( t_designation );
		GlobalFree( t_department );
		GlobalFree( t_category );

		GlobalFree( t_email_address );
		GlobalFree( t_web_page );

		send_buffer_length = ConstructVPNSPOST( worker_send_buffer, resource, add_contact, add_contact_length );

		if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
		{
			if ( worker_con.state != LOGGED_OUT && worker_con.state != LOGGING_OUT )
			{
				// "Error - UAB 752: Duplicated contact"
				char *error_code = NULL;	// POST requests only.
				if ( ParseXApplicationErrorCode( response, &error_code ) == true )
				{
					MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
				}
			}

			goto CLEANUP;
		}

		// If successful, get the contact's entry ID.
		char *xml = _StrStrA( response, "\r\n\r\n" );
		if ( xml != NULL && *( xml + 4 ) != NULL )
		{
			xml += 4;

			add_status = GetContactEntryId( xml, &( ci->contact.contact_entry_id ) );
		}

		// If we get here and haven't added the contact, then exit.
		if ( add_status == false )
		{
			CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_receive_proper_reply_when_adding_contact ) )
			goto CLEANUP;
		}

		// Request a single contact's information.
		send_buffer_length = __snprintf( worker_send_buffer, DEFAULT_BUFLEN * 2,
		"GET %s?UserId=%s&ContactId=%s " \
		"HTTP/1.1\r\n" \
		"Host: %s\r\n" \
		"Referer: %s\r\n" \
		"App-Name: %s\r\n" \
		"User-Agent: %s\r\n" \
		"Cookie: %s; %s\r\n" \
		"Connection: close\r\n\r\n", resource, account_id, ci->contact.contact_entry_id, host, REFERER, APPLICATION_NAME, USER_AGENT, vpns_cookies, wayfarer_cookies );

		if ( cfg_enable_message_log == true )
		{
			__snprintf( ml_resource, 256, "%s?UserId=[REDACTED]&ContactId=%s", resource, ci->contact.contact_entry_id );
		}
	}

	if ( Try_Send_Receive( &worker_con, host, ml_resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		add_status = false;	// Signals the cleanup to free ci.

		CHECK_CONNECTION_STATE( worker_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_receive_proper_status_code_when_requesting_contact_list ) )
		goto CLEANUP;
	}

	bool list_status = false;	// Lets us download a contact/s pictures and add the contact/s to the listview.

	// Now parse our contact/s information.
	char *xml = _StrStrA( response, "\r\n\r\n" );
	if ( xml != NULL && *( xml + 4 ) != NULL )
	{
		xml += 4;

		if ( mi->manage_type == 0 )
		{
			// Build a list of contacts and request their pictures.
			list_status = BuildContactList( xml );
		}
		else if ( mi->manage_type == 1 )
		{
			// ci is not freed if it can't be added to the contact_list tree. We'll free it in this thread if it fails.
			// Updates the element IDs and adds the contact to the contact_list.
			add_status = list_status = UpdateContactList( xml, ci );
		}
	}
	else	// No reply, assume it wasn't added.
	{
		add_status = false;	// Signals the cleanup to free ci.
	}

	// If we built or updated our contact list, then download the contact/s pictures and add them to the contact listview.
	if ( list_status == true )
	{
		// Now download any pictures the contact/s may have.
		if ( cfg_download_pictures == true )
		{
			resource = "/vpnsservice/v1_5/contactsPics";

			MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Requesting_contact_picture_location_s_ )

			int download_reply_length = 0;

			char request[ 1024 ];
			char *contact_pictures_request = NULL;

			if ( mi->manage_type == 0 )	// Get all IDs.
			{
				int num_pictures = 0;
				int total_id_length = 0;


				EnterCriticalSection( &pe_cs );
				// Go through the contact_list tree that BuildContactList created and get the total length of each ID.
				node_type *node = dllrbt_get_head( contact_list );
				while ( node != NULL )
				{
					contactinfo *ci1 = ( contactinfo * )node->val;

					if ( ci1 != NULL && ci1->contact.contact_entry_id != NULL && ci1->displayed == false )
					{
						total_id_length += lstrlenA( ci1->contact.contact_entry_id );
						++num_pictures;
					}

					node = node->next;
				}
				LeaveCriticalSection( &pe_cs );

				int contact_pictures_request_length = total_id_length + ( num_pictures * 69 ) + 1024;

				contact_pictures_request = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * contact_pictures_request_length );

				// Build the XML request for each contact in the tree that has a valid ID.

				download_reply_length = __snprintf( contact_pictures_request, contact_pictures_request_length,
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
				"<UABContactList operation=\"query\">" \
					"<User id=\"%s\" />", SAFESTRA( account_id ) );

				EnterCriticalSection( &pe_cs );
				node = dllrbt_get_head( contact_list );
				while ( node != NULL )
				{
					contactinfo *ci1 = ( contactinfo * )node->val;

					if ( ci1 != NULL && ci1->contact.contact_entry_id != NULL && ci1->displayed == false )
					{
						download_reply_length += __snprintf( contact_pictures_request + download_reply_length, contact_pictures_request_length - download_reply_length,
						"<ContactEntry id=\"%s\">" \
							"<Content type=\"picture\"></Content>" \
						"</ContactEntry>", SAFESTRA( ci1->contact.contact_entry_id ) );
					}
					node = node->next;
				}
				LeaveCriticalSection( &pe_cs );

				_memcpy_s( contact_pictures_request + download_reply_length, contact_pictures_request_length - download_reply_length, "</UABContactList>", 17 );
				download_reply_length += 17;
				contact_pictures_request[ download_reply_length ] = 0;	// Sanity.
			}
			else if ( mi->manage_type == 1 )	// Request a single contact's picture.
			{
				download_reply_length = __snprintf( request, 1024,
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
				"<UABContactList operation=\"query\">" \
					"<User id=\"%s\" />" \
					"<ContactEntry id=\"%s\">" \
						"<Content type=\"picture\"></Content>" \
					"</ContactEntry>" \
				"</UABContactList>", SAFESTRA( account_id ), SAFESTRA( ci->contact.contact_entry_id ) );

				contact_pictures_request = request;
			}

			send_buffer_length = ConstructVPNSPOST( worker_send_buffer, resource, contact_pictures_request, download_reply_length );

			// Free the request if we built it for all contacts.
			if ( mi->manage_type == 0 )
			{
				GlobalFree( contact_pictures_request );
			}

			// Doesn't really matter if this fails.
			if ( Try_Send_Receive( &worker_con, host, resource, worker_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
			{
				if ( worker_con.state != LOGGED_OUT && worker_con.state != LOGGING_OUT )
				{
					char *error_code = NULL;	// POST requests only.
					if ( ParseXApplicationErrorCode( response, &error_code ) == true )
					{
						MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
					}
				}
			}
			else
			{
				// If we got a reply, process the picture locations.
				xml = _StrStrA( response, "\r\n\r\n" );
				if ( xml != NULL && *( xml + 4 ) != NULL )
				{
					xml += 4;

					// Update each contact's picture location.
					if ( UpdatePictureLocations( xml ) == true )
					{
						// Download the pictures if any contact has one.
						DownloadContactPictures( NULL );
					}
					else
					{
						MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_No_contact_picture_location_s__were_found )
					}
				}
			}
		}

		// Finally, update our contact list.
		if ( mi->manage_type == 0 )
		{
			// Freed in the update_contact_list thread.
			updateinfo *ui = ( updateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( updateinfo ) );
			ui->new_ci = NULL;
			ui->old_ci = NULL;
			ui->picture_only = false;
			ui->remove_picture = false;
			// Add them all to the listview. This will create all the wide character strings needed for display.
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_contact_list, ( void * )ui, 0, NULL ) );
		}
		else if ( mi->manage_type == 1 )
		{
			// Freed in the update_contact_list thread.
			updateinfo *ui = ( updateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( updateinfo ) );
			ui->new_ci = NULL;
			ui->old_ci = ci;
			ui->picture_only = false;
			ui->remove_picture = false;
			// Add the contact to the listview. The only values this creates are the wide character phone values. Everything else will have been set.
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_contact_list, ( void * )ui, 0, NULL ) );
		}
	}


CLEANUP:

	CleanupConnection( &worker_con, host );

	GlobalFree( response );

	// Free the manageinfo structure.
	if ( mi != NULL )
	{
		// If we supplied a contactinfo structure and it wasn't added for some reason, then free it and all its values.
		if ( mi->ci != NULL && add_status == false )
		{
			free_contactinfo( &( mi->ci ) );
		}

		GlobalFree( mi );
	}

	// Release the semaphore if we're killing the thread.
	if ( connection_worker_semaphore != NULL )
	{
		ReleaseSemaphore( connection_worker_semaphore, 1, NULL );
	}

	in_connection_worker_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &cwt_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN Connection( void *pArguments )
{
	// This will block every other connection thread from entering until the first thread is complete.
	EnterCriticalSection( &ct_cs );

	in_connection_thread = true;

	// Stop any connection worker threads.
	kill_connection_worker_thread();
	kill_connection_incoming_thread();

	// Free shared variables among connection threads.
	free_shared_variables();

	main_con.state = LOGGING_IN;
	worker_con.state = CONNECTION_ACTIVE;
	incoming_con.state = CONNECTION_ACTIVE;

	_SendMessageW( g_hWnd_login, WM_PROPAGATE, LOGGING_IN, 0 );	// Logging in

	int send_buffer_length = 0;
	connection_send_buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( DEFAULT_BUFLEN * 2 ) );
	worker_send_buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( DEFAULT_BUFLEN * 2 ) );
	incoming_send_buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( DEFAULT_BUFLEN * 2 ) );

	char registration_buffer[ 1024 ];

	char *response = NULL;
	unsigned int response_length = 0;
	unsigned short http_status = 0;
	unsigned int content_length = 0;
	unsigned int last_buffer_size = 0;

	char *xml = NULL;

	char *host = "vpns.timewarnercable.com";

	char ml_resource[ 256 ];
	ml_resource[ 0 ] = 0;	// Sanity.

	dllrbt_tree *session_cookie_tree = NULL;
	char *session_cookies = NULL;

	char **service_notifications = NULL;

	bool reauthorize = false;

	LARGE_INTEGER li_current, li_update, li_register;
	SYSTEMTIME SystemTime, LastSystemTime;
	FILETIME FileTime, LastFileTime;

	LOGIN_SETTINGS *ls = ( LOGIN_SETTINGS * )pArguments;

	if ( ls == NULL )
	{
		goto CLEANUP;
	}

	encoded_username = url_encode( ls->username, lstrlenA( ls->username ) );
	encoded_password = url_encode( ls->password, lstrlenA( ls->password ) );

	GlobalFree( ls->password );
	ls->password = NULL;

	HANDLE hThread = ( HANDLE )_CreateThread( NULL, 0, Authorization, ( void * )NULL, 0, NULL );

	if ( hThread != NULL )
	{
		WaitForSingleObject( hThread, INFINITE );	// Wait until Authorization is complete, or failure.
		CloseHandle( hThread );
	}

	// This is the only value we need from the Authorization thread. If it's still NULL, then we can't continue.
	if ( wayfarer_cookies == NULL )
	{
		goto CLEANUP;
	}

	if ( Try_Connect( &main_con, host, cfg_connection_timeout ) == false )
	{
		//CHECK_CONNECTION_STATE( main_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_connect_to_VPNS_server ) )
		goto CLEANUP;
	}

	// Create a random MAC address instead of using the real one. 1. It increases privacy. 2. It allows multiple login instances.
	CreateMACAddress( device_id );

	// Create registration.
	content_length = __snprintf( registration_buffer, 1024,
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
	"<Registration operation=\"create\" id=\"%s\">" \
		"<Principal id=\"%s\" />" \
		"<Device id=\"%s\" />" \
	"</Registration>", SAFESTRA( ls->username ), SAFESTRA( ls->username ), SAFESTRA( device_id ) );

	GlobalFree( ls->username );
	ls->username = NULL;

	// App-Name header is required.	// Must use "Content-Type: application/x-www-form-urlencoded; charset=UTF-8" with "Origin" header.
	send_buffer_length = __snprintf( connection_send_buffer, DEFAULT_BUFLEN * 2,
	"POST /vpnsservice/v1_5/registration " \
	"HTTP/1.1\r\n" \
	"Host: %s\r\n" \
	"Referer: %s\r\n" \
	"App-Name: %s\r\n" \
	"User-Agent: %s\r\n" \
	"Origin: %s\r\n" \
	"Cookie: %s\r\n" \
	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n" \
	"Content-Length: %d\r\n" \
	"Connection: keep-alive\r\n\r\n" \
	"%s", host, REFERER, APPLICATION_NAME, USER_AGENT, ORIGIN, wayfarer_cookies, content_length, registration_buffer );

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Requesting_available_services )

	// Request available services.
	if ( Try_Send_Receive( &main_con, host, "/vpnsservice/v1_5/registration", connection_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		if ( main_con.state != LOGGED_OUT && main_con.state != LOGGING_OUT )
		{ 
			char *error_code = NULL;	// POST requests only.
			if ( ParseXApplicationErrorCode( response, &error_code ) == true )
			{
				MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
			}
			else
			{
				CHECK_CONNECTION_STATE( main_con, MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_Failed_to_request_available_services ) )
			}
		}

		goto CLEANUP;
	}

	xml = _StrStrA( response, "\r\n\r\n" );
	if ( xml != NULL && *( xml + 4 ) != NULL )
	{
		xml += 4;

		MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Parsing_account_information )

		if ( GetAccountInformation( xml, &client_id, &account_id, &account_status, &account_type, &principal_id, &service_type, &service_status, &service_context, &service_phone_number, &service_privacy_value, &service_notifications, &service_features, phone_lines ) == false )
		{
			CHECK_CONNECTION_STATE( main_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_parse_account_information ) )
			goto CLEANUP;
		}
	}
	else
	{
		CHECK_CONNECTION_STATE( main_con, MESSAGE_LOG_OUTPUT_MB( ML_WARNING, ST_The_login_was_unsuccessful ) )
		goto CLEANUP;
	}

	if ( ParseCookies( response, &vpns_cookie_tree, &vpns_cookies ) == false )
	{
		CHECK_CONNECTION_STATE( main_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_parse_server_cookies ) )
		goto CLEANUP;
	}

	// Prompt the user to select the phone line they want to receive calls on.
	// This will set current_phone_line.
	if ( phone_lines > 1 )
	{
		// Don't block if we've canceled the login.
		if ( main_con.state == LOGGED_OUT || main_con.state == LOGGING_OUT )
		{
			goto CLEANUP;
		}

		// This semaphore will be released in g_hWnd_phone_lines' WM_DESTROY.
		select_line_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		// Have the login window create the phone line prompt.
		_SendMessageW( g_hWnd_login, WM_PROPAGATE, SELECT_LINE, 0 );

		// Wait for the user selection.
		WaitForSingleObject( select_line_semaphore, INFINITE );
		CloseHandle( select_line_semaphore );
		select_line_semaphore = NULL;
	}

	// v1_5 format.
	// Update registration.
	content_length = __snprintf( registration_buffer, 1024,
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
	"<Registration operation=\"update\">" \
		"<Client id=\"%s\" />" \
		"<Service id=\"%s\" idType=\"telephone\">%s</Service>" \
	"</Registration>", SAFESTRA( client_id ), ( service_phone_number != NULL ? SAFESTRA( service_phone_number[ current_phone_line ] ) : "" ), ( service_notifications != NULL ? SAFESTRA( service_notifications[ current_phone_line ] ) : "" ) );

	if ( service_notifications != NULL )
	{
		for ( int i = 0; i < phone_lines; ++i )
		{
			GlobalFree( service_notifications[ i ] );
			service_notifications[ i ] = NULL;
		}
		GlobalFree( service_notifications );
		service_notifications = NULL;
	}

	send_buffer_length = __snprintf( connection_send_buffer, DEFAULT_BUFLEN * 2,
	"POST /vpnsservice/v1_5/registration " \
	"HTTP/1.1\r\n" \
	"Host: %s\r\n" \
	"Referer: %s\r\n" \
	"App-Name: %s\r\n" \
	"User-Agent: %s\r\n" \
	"Origin: %s\r\n" \
	"Cookie: %s; %s\r\n" \
	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n" \
	"Content-Length: %d\r\n" \
	"Connection: keep-alive\r\n\r\n" \
	"%s", host, REFERER, APPLICATION_NAME, USER_AGENT, ORIGIN, vpns_cookies, wayfarer_cookies, content_length, registration_buffer );

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Subscribing_to_available_services )

	// Subscribe to the available services above.
	if ( Try_Send_Receive( &main_con, host, "/vpnsservice/v1_5/registration", connection_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		if ( main_con.state != LOGGED_OUT && main_con.state != LOGGING_OUT )
		{ 
			char *error_code = NULL;	// POST requests only.
			if ( ParseXApplicationErrorCode( response, &error_code ) == true )
			{
				MESSAGE_LOG_OUTPUT_FREE_A( ML_ERROR, error_code )	// Frees error_code
			}
			else
			{
				CHECK_CONNECTION_STATE( main_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_subscribe_to_available_services ) )
			}
		}

		goto CLEANUP;
	}

	manageinfo *mi = ( manageinfo * )GlobalAlloc( GMEM_FIXED, sizeof( manageinfo ) );
	mi->ci = NULL;
	mi->manage_type = 0;	// Get and add all contacts.

	// mi is freed in the ManageContactInformation thread.
	CloseHandle( ( HANDLE )_CreateThread( NULL, 0, ManageContactInformation, ( void * )mi, 0, NULL ) );

	// v1_5 format.
	send_buffer_length = __snprintf( connection_send_buffer, DEFAULT_BUFLEN * 2,
	"GET /vpnspush/v1_5/join?id=%s&clientId=%s " \
	"HTTP/1.1\r\n" \
	"Host: %s\r\n" \
	"Referer: %s\r\n" \
	"App-Name: %s\r\n" \
	"User-Agent: %s\r\n" \
	"Cookie: %s\r\n" \
	"Connection: keep-alive\r\n\r\n", ( service_phone_number != NULL ? SAFESTRA( service_phone_number[ current_phone_line ] ) : "" ), SAFESTRA( client_id ), host, REFERER, APPLICATION_NAME, USER_AGENT, wayfarer_cookies );

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Joining_notification_server )

	if ( cfg_enable_message_log == true )
	{
		__snprintf( ml_resource, 256, "/vpnspush/v1_5/join?id=[REDACTED]&clientId=[REDACTED]" );
	}

	// Notification setup.
	if ( Try_Send_Receive( &main_con, host, ml_resource, connection_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
	{
		CHECK_CONNECTION_STATE( main_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_receive_proper_status_code_when_attempting_notification_setup ) )
		goto CLEANUP;
	}

	if ( ParseCookies( response, &session_cookie_tree, &session_cookies ) == false )
	{
		CHECK_CONNECTION_STATE( main_con, MESSAGE_LOG_OUTPUT( ML_ERROR, ST_Failed_to_parse_notification_setup_cookies ) )
		goto CLEANUP;
	}

	if ( response != NULL )
	{
		response[ 0 ] = NULL;
	}

	send_buffer_length = __snprintf( connection_send_buffer, DEFAULT_BUFLEN * 2,
	"GET /vpnspush/v1_5/check " \
	"HTTP/1.1\r\n" \
	"Host: %s\r\n" \
	"Referer: %s\r\n" \
	"User-Agent: %s\r\n" \
	"Cookie: %s; %s\r\n" \
	"Connection: keep-alive\r\n\r\n", host, REFERER, USER_AGENT, session_cookies, wayfarer_cookies );

	// At this point we should be logged in. We can send our ping to the server.

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Logged_in )

	if ( main_con.state != LOGGED_OUT && main_con.state != LOGGING_OUT )
	{
		main_con.state = LOGGED_IN;
		_SendMessageW( g_hWnd_login, WM_PROPAGATE, LOGGED_IN, 0 );	// Logged in
	}

	GetLocalTime( &LastSystemTime );
	SystemTimeToFileTime( &LastSystemTime, &LastFileTime );

	li_update.LowPart = LastFileTime.dwLowDateTime;
	li_update.HighPart = LastFileTime.dwHighDateTime;

	li_register.LowPart = LastFileTime.dwLowDateTime;
	li_register.HighPart = LastFileTime.dwHighDateTime;

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Polling_for_incoming_data )

	while ( true )
	{
		// HTTP status error 400 indicates a bad request. Here it means that the session cookie has expired, or is bad.
		if ( Try_Send_Receive( &main_con, host, "/vpnspush/v1_5/check", connection_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout ) == -1 )
		{
			// Reconnect loop.
			while ( true )
			{
				if ( main_con.state == LOGGED_OUT || main_con.state == LOGGING_OUT )
				{
					goto CLEANUP;
				}

				CleanupConnection( &main_con, host );

				MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Reestablishing_connection )

				if ( Try_Connect( &main_con, host, cfg_connection_timeout, false ) == true )
				{
					// Rejoin the notification server. v1_5 format.
					send_buffer_length = __snprintf( connection_send_buffer, DEFAULT_BUFLEN * 2,
					"GET /vpnspush/v1_5/join?id=%s&clientId=%s " \
					"HTTP/1.1\r\n" \
					"Host: %s\r\n" \
					"Referer: %s\r\n" \
					"App-Name: %s\r\n" \
					"User-Agent: %s\r\n" \
					"Cookie: %s; %s\r\n" \
					"Connection: keep-alive\r\n\r\n", ( service_phone_number != NULL ? SAFESTRA( service_phone_number[ current_phone_line ] ) : "" ), SAFESTRA( client_id ), host, REFERER, APPLICATION_NAME, USER_AGENT, session_cookies, wayfarer_cookies );

					if ( cfg_enable_message_log == true )
					{
						__snprintf( ml_resource, 256, "/vpnspush/v1_5/join?id=[REDACTED]&clientId=[REDACTED]" );
					}

					// Notification setup.
					if ( Try_Send_Receive( &main_con, host, ml_resource, connection_send_buffer, send_buffer_length, &response, response_length, http_status, content_length, last_buffer_size, cfg_connection_timeout, false ) != -1 )
					{
						// If the wayfarer cookie is bad, then http_status will probably be 403.
						if ( http_status != 200 && CheckAuthorization( response ) == true )
						{
							reauthorize = true;

							if ( response != NULL )
							{
								response[ 0 ] = NULL;
							}

							break;	// Break out of reconnect loop and attempt to get a new wayfarer cookie.
						}

						char *new_session_cookies = NULL;

						if ( ParseCookies( response, &session_cookie_tree, &new_session_cookies ) == true )
						{
							// If we got a new cookie.
							if ( new_session_cookies != NULL )
							{
								// Then see if the new cookie is not blank.
								if ( new_session_cookies[ 0 ] != NULL )
								{
									// If it's not, then free the old cookie and update it to the new one.
									GlobalFree( session_cookies );
									session_cookies = new_session_cookies;
								}
								else	// Otherwise, if the cookie is blank, then free it.
								{
									GlobalFree( new_session_cookies );
								}
							}

							if ( response != NULL )
							{
								response[ 0 ] = NULL;
							}

							send_buffer_length = __snprintf( connection_send_buffer, DEFAULT_BUFLEN * 2,
							"GET /vpnspush/v1_5/check " \
							"HTTP/1.1\r\n" \
							"Host: %s\r\n" \
							"Referer: %s\r\n" \
							"User-Agent: %s\r\n" \
							"Cookie: %s; %s\r\n" \
							"Connection: keep-alive\r\n\r\n", host, REFERER, USER_AGENT, session_cookies, wayfarer_cookies );

							break;	// Break out of reconnect loop since we have a new session cookie.
						}
						else
						{
							GlobalFree( new_session_cookies );
							new_session_cookies = NULL;
						}
					}
				}

				if ( main_con.state != LOGGED_OUT && main_con.state != LOGGING_OUT )
				{
					if ( reconnect_semaphore == NULL )
					{
						// This semaphore will be released when we log out or shut down the program.
						reconnect_semaphore = CreateSemaphore( NULL, 0, 1, NULL );
					}

					// Wait 30 seconds to reconnect.
					WaitForSingleObject( reconnect_semaphore, 30000 );
				}
			}
		}

		GetLocalTime( &SystemTime );
		SystemTimeToFileTime( &SystemTime, &FileTime );

		// If the wayfarer cookie is bad, then http_status will probably be 403.
		if ( http_status != 200 && reauthorize == false && CheckAuthorization( response ) == true )
		{
			reauthorize = true;

			if ( response != NULL )
			{
				response[ 0 ] = NULL;
			}
		}

		xml = ( response != NULL ? _StrStrA( response, "\r\n\r\n" ) : NULL );
		if ( xml != NULL && *( xml + 4 ) != NULL )
		{
			xml += 4;

			char *call_to = NULL;
			char *call_from = NULL;
			char *caller_id = NULL;
			char *call_reference_id = NULL;

			// True only if all of the values were found.
			if ( GetCallerIDInformation( xml, &call_to, &call_from, &caller_id, &call_reference_id ) == true )
			{
				displayinfo *di = ( displayinfo * )GlobalAlloc( GMEM_FIXED, sizeof( displayinfo ) );

				di->ci.call_to = call_to;
				di->ci.call_from = call_from;
				di->ci.call_reference_id = call_reference_id;
				di->ci.caller_id = caller_id;
				di->ci.forward_to = NULL;
				di->ci.ignored = false;
				di->ci.forwarded = false;
				di->caller_id = NULL;
				di->phone_number = NULL;
				di->reference = NULL;
				di->forward_to = NULL;
				di->sent_to = NULL;
				di->w_forward_caller_id = NULL;
				di->w_forward_phone_number = NULL;
				di->w_ignore_caller_id = NULL;
				di->w_ignore_phone_number = NULL;
				di->w_time = NULL;
				di->time.LowPart = FileTime.dwLowDateTime;
				di->time.HighPart = FileTime.dwHighDateTime;
				di->process_incoming = true;
				di->ignore_phone_number = false;
				di->forward_phone_number = false;
				di->ignore_cid_match_count = 0;
				di->forward_cid_match_count = 0;

				// This will also ignore or forward the call if it's in our lists.
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_call_log, ( void * )di, 0, NULL ) );
			}
		}

		li_current.LowPart = FileTime.dwLowDateTime;
		li_current.HighPart = FileTime.dwHighDateTime;

		// More than 25 minutes have elapsed. Log in again to update the wayfarer cookie.
		// What we're doing here is preemptively updating the wayfarer cookie before it expires.
		// This allows us to quickly process the incoming call without the need to handle any possible authentication errors.
		if ( ( li_current.QuadPart - li_update.QuadPart ) >= ( 25 * 60 * FILETIME_TICKS_PER_SECOND ) || reauthorize == true )
		{
			// Reauthorize to get a new wayfarer cookie.
			HANDLE hThread = ( HANDLE )_CreateThread( NULL, 0, Authorization, ( void * )NULL, 0, NULL );

			if ( hThread != NULL )
			{
				WaitForSingleObject( hThread, INFINITE );	// Wait until Authorization is complete, or failure.
				CloseHandle( hThread );
			}

			// Update the wayfarer cookie for the server ping.
			send_buffer_length = __snprintf( connection_send_buffer, DEFAULT_BUFLEN * 2,
			"GET /vpnspush/v1_5/check " \
			"HTTP/1.1\r\n" \
			"Host: %s\r\n" \
			"Referer: %s\r\n" \
			"User-Agent: %s\r\n" \
			"Cookie: %s; %s\r\n" \
			"Connection: keep-alive\r\n\r\n", host, REFERER, USER_AGENT, session_cookies, wayfarer_cookies );

			// Reset the last wayfarer cookie update time.
			GetLocalTime( &LastSystemTime );
			SystemTimeToFileTime( &LastSystemTime, &LastFileTime );

			li_update.LowPart = LastFileTime.dwLowDateTime;
			li_update.HighPart = LastFileTime.dwHighDateTime;

			reauthorize = false;
		}

		// Update (refresh) registration after 20 hours.
		if ( ( li_current.QuadPart - li_register.QuadPart ) >= ( 20 * 60 * 60 * FILETIME_TICKS_PER_SECOND ) )
		{
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, UpdateRegistration, ( void * )NULL, 0, NULL ) );

			// Reset the last registration time.
			GetLocalTime( &LastSystemTime );
			SystemTimeToFileTime( &LastSystemTime, &LastFileTime );

			li_register.LowPart = LastFileTime.dwLowDateTime;
			li_register.HighPart = LastFileTime.dwHighDateTime;
		}
	}

CLEANUP:

	CleanupConnection( &main_con, host );

	// Stop any connection worker threads.
	kill_connection_worker_thread();

	CleanupConnection( &worker_con );

	// Stop any connection worker threads.
	kill_connection_incoming_thread();

	CleanupConnection( &incoming_con );

	if ( reconnect_semaphore != NULL )
	{
		CloseHandle( reconnect_semaphore );
		reconnect_semaphore = NULL;
	}

	MESSAGE_LOG_OUTPUT( ML_NOTICE, ST_Logged_out )

	// Show the default login window if we cleanly logged off, canceled the login procedure, or if we experienced an unexpected log off.
	if ( main_con.state != LOGGED_OUT )
	{
		main_con.state = LOGGED_OUT;
		_SendMessageW( g_hWnd_login, WM_PROPAGATE, LOGGED_OUT, 0 );
	}

	// Reset so we can log back in from a clean log off.
	main_con.state = LOGGED_OUT;
	worker_con.state = LOGGED_OUT;
	incoming_con.state = LOGGED_OUT;

	// Free shared variables among connection threads.
	free_shared_variables();

	node_type *node = dllrbt_get_head( session_cookie_tree );
	while ( node != NULL )
	{
		COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )node->val;

		if ( cc != NULL )
		{
			GlobalFree( cc->cookie_name );
			GlobalFree( cc->cookie_value );
			GlobalFree( cc );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( session_cookie_tree );

	GlobalFree( session_cookies );

	GlobalFree( response );

	if ( service_notifications != NULL )
	{
		for ( int i = 0; i < phone_lines; ++i )
		{
			GlobalFree( service_notifications[ i ] );
		}
		GlobalFree( service_notifications );
	}

	if ( ls != NULL )
	{
		// Free login information.
		GlobalFree( ls->username );
		GlobalFree( ls->password );
		GlobalFree( ls );
	}

	// Release the semaphore if we're killing the thread.
	if ( connection_semaphore != NULL )
	{
		ReleaseSemaphore( connection_semaphore, 1, NULL );
	}

	in_connection_thread = false;

	// We're done. Let other connection threads continue.
	LeaveCriticalSection( &ct_cs );

	_ExitThread( 0 );
	return 0;
}
