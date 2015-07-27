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
#include "connection_resource.h"
#include "utilities.h"

#include "lite_shell32.h"
#include "lite_advapi32.h"

dllrbt_tree *g_resource_cache = NULL;
unsigned long long g_resource_cache_size = 0;
unsigned long long maximum_resource_cache_size = 0;

READER_WRITER_LOCK g_reader_writer_lock;
CRITICAL_SECTION g_fairness_mutex;
SRWLOCK_L g_srwlock;

void ReadRequest( SOCKET_CONTEXT *socket_context, DWORD request_size )
{
	socket_context->connection_info.rx_bytes += request_size;

	socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer + socket_context->io_context.nBufferOffset;
	socket_context->io_context.wsabuf.len = MAX_BUFFER_SIZE - socket_context->io_context.nBufferOffset;

	if ( use_ssl == true && DecodeRecv( socket_context, request_size ) == false )
	{
		socket_context->io_context.nBufferOffset = 0;
		return;
	}

	socket_context->io_context.nBufferOffset += request_size;

	// Make sure the header is complete (ends with "\r\n\r\n").
	if ( socket_context->io_context.nBufferOffset >= 4 &&
		 socket_context->io_context.wsabuf.buf[ request_size - 4 ] == '\r' &&
		 socket_context->io_context.wsabuf.buf[ request_size - 3 ] == '\n' &&
		 socket_context->io_context.wsabuf.buf[ request_size - 2 ] == '\r' &&
		 socket_context->io_context.wsabuf.buf[ request_size - 1 ] == '\n' )
	{
		// Set to the size of the request.
		socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer;
		socket_context->io_context.wsabuf.len = ( socket_context->io_context.nBufferOffset < MAX_BUFFER_SIZE ) ? socket_context->io_context.nBufferOffset : ( MAX_BUFFER_SIZE - 1 );

		socket_context->io_context.wsabuf.buf[ socket_context->io_context.wsabuf.len ] = 0;	// Sanity.

		socket_context->io_context.nBufferOffset = 0;

		SendResource( socket_context );
	}
	else	// We need to read more data. The header is incomplete.
	{
		// We can't hold any more of the request in the buffer.
		if ( MAX_BUFFER_SIZE - socket_context->io_context.nBufferOffset == 0 )
		{
			socket_context->io_context.nBufferOffset = 0;

			// Reset.
			socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer;
			socket_context->io_context.wsabuf.len = MAX_BUFFER_SIZE;

			int reply_buf_length = __snprintf( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE,
									"HTTP/1.1 413 Request Entity Too Large\r\n" \
									"Content-Type: text/html\r\n" \
									"Content-Length: 134\r\n" \
									"Connection: close\r\n\r\n" \
									"<!DOCTYPE html><html><head><title>413 Request Entity Too Large</title></head><body><h1>413 Request Entity Too Large</h1></body></html>" );

			bool ret = TrySend( socket_context, &( socket_context->io_context.overlapped ), ( use_ssl == true ? ClientIoShutdown : ClientIoClose ) );
			if ( ret == false )
			{
				BeginClose( socket_context );
			}
		}
		else	// If more data can be stored in the buffer, then request it.
		{
			bool ret = TryReceive( socket_context, &( socket_context->io_context.overlapped ), ClientIoReadRequest );
			if ( ret == false )
			{
				socket_context->io_context.nBufferOffset = 0;

				BeginClose( socket_context );
			}
		}
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

	int reply_buf_length = __snprintf( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE,
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

	socket_context->io_context.IOOperation = ClientIoWrite;
	socket_context->io_context.NextIOOperation = ClientIoClose;	// This is closed because the SSL connection was never established. An SSL shutdown would just fail.

	InterlockedIncrement( &socket_context->io_context.ref_count );

	socket_context->io_context.wsabuf.len = reply_buf_length;

	int nRet = _WSASend( socket_context->Socket, &( socket_context->io_context.wsabuf ), 1, NULL, dwFlags, &( socket_context->io_context.overlapped ), NULL );
	if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
	{
		InterlockedDecrement( &socket_context->io_context.ref_count );
		BeginClose( socket_context );
	}
}

void SendResource( SOCKET_CONTEXT *socket_context )
{
	IO_OPERATION io_operation = ( use_ssl == true ? ClientIoShutdown : ClientIoClose );

	unsigned int reply_buf_length = 0;

	socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer;
	socket_context->io_context.wsabuf.len = MAX_BUFFER_SIZE;

	// If we haven't loaded a resource file, or our cached resource file, then do so below.
	if ( socket_context->resource.hFile_resource == INVALID_HANDLE_VALUE && socket_context->resource.use_cache == false )
	{
		// Get all the necessary values from our header.
		GetHeaderValues( socket_context );

		// See if we're authorized.
		if ( !socket_context->resource.is_authorized )
		{
			socket_context->resource.status_code = STATUS_CODE_401;	// Unauthorized.

			goto SEND_RESPONSE;
		}

		// See if our connection is a websocket upgrade.
		if ( socket_context->resource.connection_type == CONNECTION_HEADER_UPGRADE )
		{
			// See if we have a websocket upgrade key.
			// If set, then our origin (if we checked it) will be valid.
			// And if we didn't check the origin, then we'll assume it's valid.
			if ( socket_context->resource.websocket_upgrade_key != NULL )
			{
				// websocket_upgrade_key is freed in the switch below.
				socket_context->resource.status_code = STATUS_CODE_101;	// Switching Protocols.	
			}
			else	// If we don't have an upgrade key, then either our origin is invalid, or the key was never found/bad upgrade type.
			{
				socket_context->resource.status_code = STATUS_CODE_403;	// Forbidden.
			}

			goto SEND_RESPONSE;
		}

		// If our checks above were good, then handle the resource request.

		// Find the first line of the HTTP header. Method Resource Protocol (GET /index.html HTTP/1.1)
		char *line_end = _StrStrA( socket_context->io_context.wsabuf.buf, "\r\n" );
		if ( line_end != NULL )
		{
			*line_end = 0;

			// Find the beginning of the resource.
			char *resource_begin = _StrChrA( socket_context->io_context.wsabuf.buf, '/' );
			if ( resource_begin != NULL )
			{
				// Make sure our method type is GET. Anything else is unsupported.
				if ( ( resource_begin - socket_context->io_context.wsabuf.buf >= 3 ) && _memcmp( socket_context->io_context.wsabuf.buf, "GET", 3 ) == 0 )
				{
					++resource_begin;

					// Find the end of the resource.
					char *resource_end = _StrChrA( resource_begin, ' ' );
					if ( resource_end != NULL )
					{
						*resource_end = 0;

						unsigned int resource_path_length = 0;
						char *resource_path = url_decode( resource_begin, resource_end - resource_begin, &resource_path_length );

						// Reset.
						*resource_end = ' ';
						*line_end = '\r';

						// This will set our resource struct and any necessary status code values.
						GetResource( socket_context, resource_path, resource_path_length );

						if ( socket_context->resource.status_code == STATUS_CODE_200 )
						{
							if ( socket_context->resource.use_chunked_transfer == true )
							{
								reply_buf_length = __snprintf( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE,
									"HTTP/1.1 200 OK\r\n" \
									"Content-Type: %s\r\n" \
									"Cache-Control: max-age=31536000\r\n" \
									"Transfer-Encoding: chunked\r\n" \
									"Connection: %s\r\n\r\n", GetMIMEByFileName( resource_path ), ( allow_keep_alive_requests == true ? "keep-alive" : "close" ) );
							}
							else
							{
								reply_buf_length = __snprintf( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE,
									"HTTP/1.1 200 OK\r\n" \
									"Content-Type: %s\r\n" \
									"Content-Length: %d\r\n" \
									"Cache-Control: max-age=31536000\r\n" \
									"Connection: %s\r\n\r\n", GetMIMEByFileName( resource_path ), socket_context->resource.file_size, ( allow_keep_alive_requests == true ? "keep-alive" : "close" ) );
							}
						}

						GlobalFree( resource_path );
					}
					else
					{
						socket_context->resource.status_code = STATUS_CODE_400;	// Bad Request.
					}
				}
				else
				{
					socket_context->resource.status_code = STATUS_CODE_501;	// Bad Method / Not Implemented.
				}
			}
			else
			{
				socket_context->resource.status_code = STATUS_CODE_400;	// Bad Request.
			}
		}
		else
		{
			socket_context->resource.status_code = STATUS_CODE_400;	// Bad Request.
		}
	}

SEND_RESPONSE:

	switch ( socket_context->resource.status_code )
	{
		case STATUS_CODE_101:	// Switch Protocols (WebSocket)
		{
			socket_context->connection_type = CON_TYPE_WEBSOCKET;	// Websocket connection type.

			reply_buf_length = __snprintf( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE,
									"HTTP/1.1 101 Switching Protocols\r\n" \
									"Upgrade: websocket\r\n" \
									"Connection: Upgrade\r\n" \
									"Sec-WebSocket-Accept: %s\r\n\r\n", socket_context->resource.websocket_upgrade_key );

			GlobalFree( socket_context->resource.websocket_upgrade_key );
			socket_context->resource.websocket_upgrade_key = NULL;

			io_operation = ClientIoReadMoreWebSocketRequest;
		}
		break;

		case STATUS_CODE_200:	// Normal file.
		{
			// Continue to write while our total bytes read is less than the file size.
			if ( socket_context->resource.total_read < socket_context->resource.file_size )
			{
				// If we read everything from our resource buffer, then read more data. Should only happen when reading from a file and not our resource cache.
				if ( socket_context->resource.resource_buf_offset == socket_context->resource.resource_buf_size )
				{
					if ( socket_context->resource.use_cache == false )
					{
						ReadFile( socket_context->resource.hFile_resource, socket_context->resource.resource_buf, MAX_RESOURCE_BUFFER_SIZE, &( socket_context->resource.resource_buf_size ), NULL );
						socket_context->resource.resource_buf_offset = 0;
					}
				}

				unsigned int rem = 0;

				// Add our chunked encoding length.
				if ( socket_context->resource.use_chunked_transfer == true )
				{
					// Subtract 17 for the chunked encodings. (example: FFFFFFFF\r\n[DATA]\r\n0\r\n\r\n)
					// rem is the amount of file data we'll be writing to the buffer.
					rem = min( MAX_BUFFER_SIZE - reply_buf_length - 17, socket_context->resource.resource_buf_size - socket_context->resource.resource_buf_offset );

					reply_buf_length += __snprintf( socket_context->io_context.wsabuf.buf + reply_buf_length, MAX_BUFFER_SIZE - reply_buf_length, "%x\r\n", rem );
				}
				else
				{
					rem = min( MAX_BUFFER_SIZE - reply_buf_length, socket_context->resource.resource_buf_size - socket_context->resource.resource_buf_offset );
				}

				// Write our data.
				//_memcpy_s( socket_context->io_context.wsabuf.buf + reply_buf_length, MAX_BUFFER_SIZE - reply_buf_length, ( socket_context->resource.use_cache == true ? index_file_buf : socket_context->resource.resource_buf ) + socket_context->resource.resource_buf_offset, rem );
				_memcpy_s( socket_context->io_context.wsabuf.buf + reply_buf_length, MAX_BUFFER_SIZE - reply_buf_length, socket_context->resource.resource_buf + socket_context->resource.resource_buf_offset, rem );

				reply_buf_length += rem;

				socket_context->resource.resource_buf_offset += rem;
				socket_context->resource.total_read += rem;

				// Add the end of the chunked encoding.
				if ( socket_context->resource.use_chunked_transfer == true )
				{
					_memcpy_s( socket_context->io_context.wsabuf.buf + reply_buf_length, MAX_BUFFER_SIZE - reply_buf_length, "\r\n", 2 );
					reply_buf_length += 2;
				}

				// If we've read and copied the entire file, then clean up our resource information.
				if ( socket_context->resource.total_read >= socket_context->resource.file_size )
				{
					if ( socket_context->resource.use_chunked_transfer == true )
					{
						// Add the terminating chunk value.
						_memcpy_s( socket_context->io_context.wsabuf.buf + reply_buf_length, MAX_BUFFER_SIZE - reply_buf_length, "0\r\n\r\n", 5 );
						reply_buf_length += 5;
					}

					if ( allow_keep_alive_requests == true )
					{
						io_operation = ClientIoReadMoreRequest;	// Keep reading if our connection is to be kept alive.
					}
					else
					{
						io_operation = ( use_ssl == true ? ClientIoShutdown : ClientIoClose );
					}

					// We've ended up writing the entire file to our buffer. Clean up our resources and shutdown/close the connection.
					if ( socket_context->resource.hFile_resource != INVALID_HANDLE_VALUE )
					{
						CloseHandle( socket_context->resource.hFile_resource );
						socket_context->resource.hFile_resource = INVALID_HANDLE_VALUE;
					}

					socket_context->resource.resource_buf_size = 0;
					socket_context->resource.total_read = 0;
					socket_context->resource.file_size = 0;
					socket_context->resource.resource_buf_offset = 0;

					if ( socket_context->resource.resource_buf != NULL )
					{
						if ( socket_context->resource.use_cache == false )
						{
							GlobalFree( socket_context->resource.resource_buf );
						}
						socket_context->resource.resource_buf = NULL;
					}
				}
				else	// There's still more data to be read.
				{
					io_operation = ClientIoWriteRequestResource;	// Continue to write more of the file.
				}
			}
		}
		break;

		case STATUS_CODE_400:	// Bad Request.
		{
			reply_buf_length = __snprintf( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE,
									"HTTP/1.1 400 Bad Request\r\n" \
									"Content-Type: text/html\r\n" \
									"Content-Length: 108\r\n" \
									"Connection: close\r\n\r\n" \
									"<!DOCTYPE html><html><head><title>400 Bad Request</title></head><body><h1>400 Bad Request</h1></body></html>" );

			io_operation = ( use_ssl == true ? ClientIoShutdown : ClientIoClose );
		}
		break;

		case STATUS_CODE_401:	// Unauthorized.
		{
			reply_buf_length = __snprintf( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE,
									"HTTP/1.1 401 Unauthorized\r\n" \
									"Content-Type: text/html; charset=UTF-8\r\n" \
									"WWW-Authenticate: Basic realm=\"Authorization Required\"\r\n" \
									"Content-Length: 110\r\n" \
									"Connection: close\r\n\r\n" \
									"<!DOCTYPE html><html><head><title>401 Unauthorized</title></head><body><h1>401 Unauthorized</h1></body></html>" );

			io_operation = ( use_ssl == true ? ClientIoShutdown : ClientIoClose );
		}
		break;

		case STATUS_CODE_403:	// Forbidden.
		{
			reply_buf_length = __snprintf( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE,
									"HTTP/1.1 403 Forbidden\r\n" \
									"Content-Type: text/html\r\n" \
									"Content-Length: 104\r\n" \
									"Connection: close\r\n\r\n" \
									"<!DOCTYPE html><html><head><title>403 Forbidden</title></head><body><h1>403 Forbidden</h1></body></html>" );

			io_operation = ( use_ssl == true ? ClientIoShutdown : ClientIoClose );
		}
		break;

		case STATUS_CODE_404:	// File Not Found.
		{
			reply_buf_length = __snprintf( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE,
									"HTTP/1.1 404 Not Found\r\n" \
									"Content-Type: text/html\r\n" \
									"Content-Length: 104\r\n" \
									"Connection: close\r\n\r\n" \
									"<!DOCTYPE html><html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>" );

			io_operation = ( use_ssl == true ? ClientIoShutdown : ClientIoClose );
		}
		break;

		case STATUS_CODE_500:	// Internal Server Error.
		{
			reply_buf_length = __snprintf( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE,
									"HTTP/1.1 500 Internal Server Error\r\n" \
									"Content-Type: text/html\r\n" \
									"Content-Length: 128\r\n" \
									"Connection: close\r\n\r\n" \
									"<!DOCTYPE html><html><head><title>500 Internal Server Error</title></head><body><h1>500 Internal Server Error</h1></body></html>" );

			io_operation = ( use_ssl == true ? ClientIoShutdown : ClientIoClose );
		}
		break;

		case STATUS_CODE_501:	// Not Implemented.
		{
			reply_buf_length = __snprintf( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE,
									"HTTP/1.1 501 Not Implemented\r\n" \
									"Content-Type: text/html\r\n" \
									"Content-Length: 116\r\n" \
									"Connection: close\r\n\r\n" \
									"<!DOCTYPE html><html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1></body></html>" );

			io_operation = ( use_ssl == true ? ClientIoShutdown : ClientIoClose );
		}
		break;
	}

	socket_context->io_context.wsabuf.len = reply_buf_length;

	// If we have something to write.
	if ( reply_buf_length > 0 )
	{
		bool ret = TrySend( socket_context, &( socket_context->io_context.overlapped ), io_operation );
		if ( ret == false )
		{
			BeginClose( socket_context );
		}
	}
	else	// Something went wrong, close the connection.
	{
		BeginClose( socket_context, ( use_ssl == true ? ClientIoShutdown : ClientIoClose ) );
	}
}

void GetResource( SOCKET_CONTEXT *socket_context, char *resource_path, unsigned int resource_path_length )
{
	RESOURCE_CACHE_ITEM *rci = NULL;

	// No reason to lock if the resource cache is disabled.
	if ( maximum_resource_cache_size > 0 )
	{
		if ( use_rwl_library == true )
		{
			_AcquireSRWLockSharedFairly( &g_fairness_mutex, &g_srwlock );
		}
		else
		{
			EnterReaderLock( &g_reader_writer_lock );
		}

		// See if our tree has the phone number to add the node to.
		rci = ( RESOURCE_CACHE_ITEM * )dllrbt_find( g_resource_cache, ( void * )resource_path, true );

		if ( use_rwl_library == true )
		{
			_ReleaseSRWLockShared( &g_srwlock );
		}
		else
		{
			LeaveReaderLock( &g_reader_writer_lock );
		}
	}

	// The cache resource exists. Set our context's resource information.
	if ( rci != NULL )
	{
		socket_context->resource.status_code = STATUS_CODE_200;	// OK.

		socket_context->resource.use_cache = true;

		socket_context->resource.resource_buf = rci->resource_data;
		socket_context->resource.file_size = rci->resource_data_size;
		socket_context->resource.resource_buf_size = rci->resource_data_size;
	}
	else if ( g_resource_cache_size <= maximum_resource_cache_size && maximum_resource_cache_size > 0 )	// Write the resource to our cache.
	{
		if ( use_rwl_library == true )
		{
			_AcquireSRWLockExclusiveFairly( &g_fairness_mutex, &g_srwlock );
		}
		else
		{
			EnterWriterLock( &g_reader_writer_lock );
		}

		// Search again while we're in the WriterLock in case we added it in another thread after our ReaderLock.
		rci = ( RESOURCE_CACHE_ITEM * )dllrbt_find( g_resource_cache, ( void * )resource_path, true );

		if ( rci == NULL )
		{
			// This will set socket_context->resource.hFile_resource and any necessary status code values (not found, forbidden, internal error, etc.).
			OpenResourceFile( socket_context, resource_path, resource_path_length );

			if ( socket_context->resource.hFile_resource != INVALID_HANDLE_VALUE )
			{
				socket_context->resource.file_size = GetFileSize( socket_context->resource.hFile_resource, NULL );

				// If the current resource cache size + the opened file size is greater than our maximum resource cache size...
				// ...and the opened file size is greater than 2 megabytes, then don't add it to the resource cache.
				// This gives us a bit of leeway, but prevents huge files from filling the resource cache.
				if ( g_resource_cache_size + socket_context->resource.file_size > maximum_resource_cache_size && socket_context->resource.file_size > 2097152 )
				{
					socket_context->resource.use_cache = false;

					socket_context->resource.resource_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * MAX_RESOURCE_BUFFER_SIZE );

					ReadFile( socket_context->resource.hFile_resource, socket_context->resource.resource_buf, MAX_RESOURCE_BUFFER_SIZE, &( socket_context->resource.resource_buf_size ), NULL );
				}
				else	// Add any file that can fit into our resource cache.
				{
					socket_context->resource.use_cache = true;

					// Make sure we have data to allocate and read.
					if ( socket_context->resource.file_size > 0 )
					{
						socket_context->resource.resource_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * socket_context->resource.file_size );

						ReadFile( socket_context->resource.hFile_resource, socket_context->resource.resource_buf, socket_context->resource.file_size, &( socket_context->resource.resource_buf_size ), NULL );
					}
					else	// If there's nothing to send, then keep our buffer NULL.
					{
						socket_context->resource.resource_buf = NULL;
						socket_context->resource.resource_buf_size = 0;
					}

					// No need to keep the file handle open since we're using our cache buffer.
					CloseHandle( socket_context->resource.hFile_resource );
					socket_context->resource.hFile_resource = INVALID_HANDLE_VALUE;

					// Create our cache item to add to the tree.
					rci = ( RESOURCE_CACHE_ITEM * )GlobalAlloc( GMEM_FIXED, sizeof( RESOURCE_CACHE_ITEM ) );
					rci->resource_path = GlobalStrDupA( resource_path );
					rci->resource_data = socket_context->resource.resource_buf;
					rci->resource_data_size = socket_context->resource.file_size;

					// Insert the resource into the tree.
					if ( dllrbt_insert( g_resource_cache, ( void * )rci->resource_path, ( void * )rci ) != DLLRBT_STATUS_OK )
					{
						// This shouldn't happen.
						socket_context->resource.status_code = STATUS_CODE_500;	// Internal Error.

						socket_context->resource.resource_buf = NULL;
						socket_context->resource.file_size = 0;
						socket_context->resource.resource_buf_size = 0;

						GlobalFree( rci->resource_data );
						GlobalFree( rci->resource_path );
						GlobalFree( rci );
					}
					else
					{
						g_resource_cache_size += socket_context->resource.file_size;
					}
				}
			}
		}
		else
		{
			socket_context->resource.status_code = STATUS_CODE_200;	// OK.

			socket_context->resource.use_cache = true;

			socket_context->resource.resource_buf = rci->resource_data;
			socket_context->resource.file_size = rci->resource_data_size;
			socket_context->resource.resource_buf_size = rci->resource_data_size;
		}

		if ( use_rwl_library == true )
		{
			_ReleaseSRWLockExclusive( &g_srwlock );
		}
		else
		{
			LeaveWriterLock( &g_reader_writer_lock );
		}
	}
	else	// Read exclusively from the disk.
	{
		// This will set socket_context->resource.hFile_resource and any necessary status code values (not found, forbidden, internal error, etc.).
		OpenResourceFile( socket_context, resource_path, resource_path_length );

		if ( socket_context->resource.hFile_resource != INVALID_HANDLE_VALUE )
		{
			socket_context->resource.use_cache = false;

			socket_context->resource.file_size = GetFileSize( socket_context->resource.hFile_resource, NULL );

			// Make sure we have data to allocate and read.
			if ( socket_context->resource.file_size > 0 )
			{
				socket_context->resource.resource_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * MAX_RESOURCE_BUFFER_SIZE );

				ReadFile( socket_context->resource.hFile_resource, socket_context->resource.resource_buf, MAX_RESOURCE_BUFFER_SIZE, &( socket_context->resource.resource_buf_size ), NULL );
			}
			else	// If there's nothing to send, then keep our buffer NULL.
			{
				socket_context->resource.resource_buf = NULL;
				socket_context->resource.resource_buf_size = 0;
			}
		}
	}
}

void OpenResourceFile( SOCKET_CONTEXT *socket_context, char *resource_path, unsigned int resource_path_length )
{
	// Make sure the client doesn't access parent directories beyond the root directory.
	if ( _StrStrA( resource_path, "./" ) == NULL && _StrStrA( resource_path, ".\\" ) == NULL )
	{
		// Convert the UTF-8 path into a Wide Char string.
		int path_length = MultiByteToWideChar( CP_UTF8, 0, resource_path, -1, NULL, 0 ) + ( document_root_directory_length + 1 + 10 + 1 );	// Add an extra space for a trailing '\'
		wchar_t *w_resource = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * path_length );

		// Add the root directory and a trailing '\'.
		_wmemcpy_s( w_resource, path_length, document_root_directory, document_root_directory_length );
		_wmemcpy_s( w_resource + document_root_directory_length, path_length - document_root_directory_length, L"\\", 1 );

		// Append the resource path.
		MultiByteToWideChar( CP_UTF8, 0, resource_path, -1, w_resource + ( document_root_directory_length + 1 ), path_length - ( document_root_directory_length + 1 ) - 10 );

		// See if the full path is a file or folder on our system.
		DWORD fa = GetFileAttributesW( w_resource );
		if ( fa != INVALID_FILE_ATTRIBUTES )
		{
			// If the full path is a folder, then send an index file if it exists.
			if ( ( fa & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
			{
				// See if the directory ends with a slash.
				if ( resource_path_length > 0 && ( *( resource_path + ( resource_path_length - 1 ) ) != '/' && *( resource_path + ( resource_path_length - 1 ) ) != '\\' ) )
				{
					*( w_resource + ( path_length - 12 ) ) = L'\\';	// Add the slash if there isn't one.
				}
				else	// Decrease the path length if there's already a slash.
				{
					--path_length;
				}

				// First try to open an index that ends with ".html".
				_wmemcpy_s( w_resource + ( path_length - 11 ), 11, L"index.html\0", 11 );
				socket_context->resource.hFile_resource = CreateFile( w_resource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
				
				// If the index file does not end with ".html", then see if it ends with ".htm".
				if ( socket_context->resource.hFile_resource == INVALID_HANDLE_VALUE )
				{
					*( w_resource + ( path_length - 2 ) ) = 0;	// Set the 'l' to NULL.
					//_memcpy_s( w_resource + ( path_length - 11 ), sizeof( wchar_t ) * 11, L"index.htm\0", sizeof( wchar_t ) * 10 );
					socket_context->resource.hFile_resource = CreateFile( w_resource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
				}
			}
			else	// Otherwise, the resource is a file.
			{
				socket_context->resource.hFile_resource = CreateFile( w_resource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
			}

			if ( socket_context->resource.hFile_resource != INVALID_HANDLE_VALUE )
			{
				socket_context->resource.status_code = STATUS_CODE_200;	// OK.
			}
			else	// File didn't open.
			{
				socket_context->resource.status_code = STATUS_CODE_500;	// Internal error.
			}
		}
		else
		{
			socket_context->resource.status_code = STATUS_CODE_404;	// Not Found.
		}

		GlobalFree( w_resource );
	}
	else
	{
		socket_context->resource.status_code = STATUS_CODE_403;	// Forbidden.
	}
}

void GetHeaderValues( SOCKET_CONTEXT *socket_context )
{
	socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer;
	socket_context->io_context.wsabuf.len = MAX_BUFFER_SIZE;

	char *request_buffer = socket_context->io_context.wsabuf.buf;

	char *end_of_header = fields_tolower( request_buffer );
	if ( end_of_header != NULL )
	{
		// Assume we're authorized if we're not checking for it.
		socket_context->resource.is_authorized = true;

		// This has priority over the other fields. If the authorization fails, then we exit immediately.
		if ( use_authentication == true )
		{
			// This must be reset every time so that we can verify any possible changes.
			socket_context->resource.is_authorized = false;

			char *authorization_header = _StrStrA( request_buffer, "authorization:" );
			if ( authorization_header != NULL )
			{
				authorization_header = _StrStrIA( authorization_header + 14, "Basic " );	// The protocol doesn't specify whether "Basic" is case-sensitive or not. Note that the protocol requires a single space (SP) after "Basic".
				if ( authorization_header != NULL )
				{
					authorization_header += 6;

					char *authorization_header_end = _StrStrA( authorization_header, "\r\n" );
					if ( authorization_header_end != NULL )
					{
						int key_length = ( authorization_header_end - authorization_header );

						if ( key_length == encoded_authentication_length && encoded_authentication != NULL && _memcmp( authorization_header, encoded_authentication, encoded_authentication_length ) == 0 )
						{
							socket_context->resource.is_authorized = true;
						}

						/*char *key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( key_length + 1 ) );
						_memcpy_s( key, ( key_length + 1 ), authorization_header, key_length );
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
							socket_context->resource.is_authorized = true;
						}*/
					}
				}
			}

			if ( socket_context->resource.is_authorized == false )
			{
				return;
			}
		}

		// Determine the Connection type (closed, keep-alive, upgrade).
		if ( socket_context->resource.connection_type == CONNECTION_HEADER_NOT_SET )
		{
			char *connection_header = _StrStrA( request_buffer, "connection:" );
			if ( connection_header != NULL )
			{
				connection_header += 11;

				char *connection_header_end = _StrStrA( connection_header, "\r\n" );
				if ( connection_header_end != NULL )
				{
					// Skip whitespace.
					while ( connection_header < connection_header_end )
					{
						if ( connection_header[ 0 ] != ' ' && connection_header[ 0 ] != '\t' && connection_header[ 0 ] != '\f' )
						{
							break;
						}

						++connection_header;
					}

					// Skip whitespace that could appear before the "\r\n", but after the field value.
					while ( ( connection_header_end - 1 ) >= connection_header )
					{
						if ( *( connection_header_end - 1 ) != ' ' && *( connection_header_end - 1 ) != '\t' && *( connection_header_end - 1 ) != '\f' )
						{
							break;
						}

						--connection_header_end;
					}

					// Apparently the Connection header can have multiple values. Ex: "Connection: keep-alive, Upgrade\r\n"

					char tmp_end = *connection_header_end;
					*connection_header_end = 0;	// Sanity

					char *connection_type = _StrStrIA( connection_header, "keep-alive" );
					if ( connection_type != NULL )
					{
						socket_context->resource.connection_type = CONNECTION_HEADER_KEEP_ALIVE;
					}
					else // If we found the keep-alive value, then there shouldn't be a close. Wouldn't make sense.
					{
						connection_type = _StrStrIA( connection_header, "close" );
						if ( connection_type != NULL )
						{
							socket_context->resource.connection_type = CONNECTION_HEADER_CLOSE;
						}
					}

					// The upgrade value can be paired with keep-alive and close. So we need to search for it as well.
					// We'll have it take priority over the others if we find it.
					connection_type = _StrStrIA( connection_header, "upgrade" );
					if ( connection_type != NULL )
					{
						socket_context->resource.connection_type = CONNECTION_HEADER_UPGRADE;
					}

					*connection_header_end = tmp_end;	// Restore.
				}
			}
			else	// If the Connection header field does not exist, then by default, it'll be keep-alive. At least for HTTP/1.1 - which is all that we're going to support.
			{
				socket_context->resource.connection_type = CONNECTION_HEADER_KEEP_ALIVE;
			}
		}

		// If our Connection was an upgrade, then see if it's a websocket upgrade.
		if ( socket_context->resource.connection_type == CONNECTION_HEADER_UPGRADE )
		{
			// Our only concern here is if it's a websocket upgrade.
			bool websocket_upgrade = false;

			char *upgrade_header = _StrStrA( request_buffer, "upgrade:" );
			if ( upgrade_header != NULL )
			{
				upgrade_header += 8;

				char *upgrade_header_end = _StrStrA( upgrade_header, "\r\n" );
				if ( upgrade_header_end != NULL )
				{
					// Skip whitespace.
					while ( upgrade_header < upgrade_header_end )
					{
						if ( upgrade_header[ 0 ] != ' ' && upgrade_header[ 0 ] != '\t' && upgrade_header[ 0 ] != '\f' )
						{
							break;
						}

						++upgrade_header;
					}

					// Skip whitespace that could appear before the "\r\n", but after the field value.
					while ( ( upgrade_header_end - 1 ) >= upgrade_header )
					{
						if ( *( upgrade_header_end - 1 ) != ' ' && *( upgrade_header_end - 1 ) != '\t' && *( upgrade_header_end - 1 ) != '\f' )
						{
							break;
						}

						--upgrade_header_end;
					}

					if ( _StrCmpNIA( upgrade_header, "websocket", upgrade_header_end - upgrade_header ) == 0 )
					{
						websocket_upgrade = true;
					}
				}
			}

			if ( websocket_upgrade == true )
			{
				// Optionally check to see if our orgin matches the server host.
				if ( verify_origin == true )
				{
					char *origin_header = _StrStrA( request_buffer, "origin:" );
					if ( origin_header != NULL )
					{
						origin_header += 7;

						// Skip whitespace that could appear after the ":", but before the field value.
						while ( origin_header < end_of_header )
						{
							if ( origin_header[ 0 ] != ' ' && origin_header[ 0 ] != '\t' && origin_header[ 0 ] != '\f' )
							{
								break;
							}

							++origin_header;
						}

						char *origin_header_end = _StrStrA( origin_header, "\r\n" );

						// Skip whitespace that could appear before the "\r\n", but after the field value.
						while ( ( origin_header_end - 1 ) >= origin_header )
						{
							if ( *( origin_header_end - 1 ) != ' ' && *( origin_header_end - 1 ) != '\t' && *( origin_header_end - 1 ) != '\f' )
							{
								break;
							}

							--origin_header_end;
						}

						// Verify the origin.
						char tmp_end = *origin_header_end;
						*origin_header_end = 0;	// Sanity

						socket_context->resource.has_valid_origin = VerifyOrigin( origin_header, ( origin_header_end - origin_header ) );

						*origin_header_end = tmp_end;	// Restore.
					}
				}
				else	// Assume our origin is valid if we're not checking it.
				{
					socket_context->resource.has_valid_origin = true;
				}

				// If the origin is valid, whether we accepted one or not, the get our websocket key.
				if ( socket_context->resource.has_valid_origin == true )
				{
					// Get our websocket request key and create our response key.
					char *websocket_key = _StrStrA( request_buffer, "sec-websocket-key:" );
					if ( websocket_key != NULL )
					{
						websocket_key += 18;

						// Skip whitespace that could appear after the ":", but before the field value.
						while ( websocket_key < end_of_header )
						{
							if ( websocket_key[ 0 ] != ' ' && websocket_key[ 0 ] != '\t' && websocket_key[ 0 ] != '\f' )
							{
								break;
							}

							++websocket_key;
						}

						char *websocket_key_end = _StrStrA( websocket_key, "\r\n" );

						// Skip whitespace that could appear before the "\r\n", but after the field value.
						while ( ( websocket_key_end - 1 ) >= websocket_key )
						{
							if ( *( websocket_key_end - 1 ) != ' ' && *( websocket_key_end - 1 ) != '\t' && *( websocket_key_end - 1 ) != '\f' )
							{
								break;
							}

							--websocket_key_end;
						}

						int websocket_key_length = ( websocket_key_end - websocket_key );
						int total_websocket_key_length = websocket_key_length + 36;
						char *key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( total_websocket_key_length + 1 ) );
						_memcpy_s( key, ( total_websocket_key_length + 1 ), websocket_key, websocket_key_length );
						_memcpy_s( key + websocket_key_length, ( total_websocket_key_length + 1 ) - websocket_key_length, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36 );
						key[ total_websocket_key_length ] = 0;	// Sanity.
						
						HCRYPTHASH hHash = NULL;
						HCRYPTPROV hProvider = NULL;
						_CryptAcquireContextW( &hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT );
						_CryptCreateHash( hProvider, CALG_SHA1, NULL, 0, &hHash );
						_CryptHashData( hHash, ( BYTE * )key, total_websocket_key_length, 0 );

						DWORD sha1_value_length, dwByteCount = sizeof( DWORD );
						_CryptGetHashParam( hHash, HP_HASHSIZE, ( BYTE * )&sha1_value_length, &dwByteCount, 0 );

						char *sha1_value = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( sha1_value_length + 1 ) );

						_CryptGetHashParam( hHash, HP_HASHVAL, ( BYTE * )sha1_value, &sha1_value_length, 0 );
						sha1_value[ sha1_value_length ] = 0; // Sanity.

						DWORD dwLength = 0;
						_CryptBinaryToStringA( ( BYTE * )sha1_value, sha1_value_length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &dwLength );

						socket_context->resource.websocket_upgrade_key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * dwLength );
						_CryptBinaryToStringA( ( BYTE * )sha1_value, sha1_value_length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, ( LPSTR )socket_context->resource.websocket_upgrade_key, &dwLength );
						socket_context->resource.websocket_upgrade_key[ dwLength ] = 0; // Sanity.

						GlobalFree( sha1_value );
						_CryptDestroyHash( hHash );
						_CryptReleaseContext( hProvider, 0 );
						GlobalFree( key );
					}
				}
			}
		}

		// Determine the transfer encoding.
		if ( socket_context->resource.use_chunked_transfer == false )
		{
			char *transfer_encoding_header = _StrStrA( request_buffer, "transfer-encoding:" );
			if ( transfer_encoding_header != NULL )
			{
				transfer_encoding_header += 18;

				char *transfer_encoding_header_end = _StrStrA( transfer_encoding_header, "\r\n" );
				if ( transfer_encoding_header_end != NULL )
				{
					// Skip whitespace.
					while ( transfer_encoding_header < transfer_encoding_header_end )
					{
						if ( transfer_encoding_header[ 0 ] != ' ' && transfer_encoding_header[ 0 ] != '\t' && transfer_encoding_header[ 0 ] != '\f' )
						{
							break;
						}

						++transfer_encoding_header;
					}

					// Skip whitespace that could appear before the "\r\n", but after the field value.
					while ( ( transfer_encoding_header_end - 1 ) >= transfer_encoding_header )
					{
						if ( *( transfer_encoding_header_end - 1 ) != ' ' && *( transfer_encoding_header_end - 1 ) != '\t' && *( transfer_encoding_header_end - 1 ) != '\f' )
						{
							break;
						}

						--transfer_encoding_header_end;
					}

					if ( _StrCmpNIA( transfer_encoding_header, "chunked", transfer_encoding_header_end - transfer_encoding_header ) == 0 )
					{
						socket_context->resource.use_chunked_transfer = true;
					}
				}
			}
		}
	}
}

bool VerifyOrigin( char *origin, int origin_length )
{
	char *origin_position = origin;

	// Check the protocol.
	if ( use_ssl == true )
	{
		if ( origin_length >= 8 && _memcmp( origin_position, "https://", 8 ) != 0 )
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
		if ( origin_length >= 7 && _memcmp( origin_position, "http://", 7 ) != 0 )
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

void CleanupResourceCache( dllrbt_tree **resource_cache )
{
	// Free the values of the resource cache.
	node_type *node = dllrbt_get_head( *resource_cache );
	while ( node != NULL )
	{
		RESOURCE_CACHE_ITEM *rci = ( RESOURCE_CACHE_ITEM * )node->val;

		if ( rci != NULL )
		{
			GlobalFree( rci->resource_data );
			GlobalFree( rci->resource_path );
			GlobalFree( rci );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( *resource_cache );
	*resource_cache = NULL;
}
