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

#include "globals.h"

#include "file_operations.h"
#include "utilities.h"
#include "connection.h"
#include "lite_ntdll.h"
#include "lite_crypt32.h"

#define ROTATE_LEFT( x, n ) ( ( ( x ) << ( n ) ) | ( ( x ) >> ( 8 - ( n ) ) ) )
#define ROTATE_RIGHT( x, n ) ( ( ( x ) >> ( n ) ) | ( ( x ) << ( 8 - ( n ) ) ) )

int g_document_root_directory_length = 0;

void encode_cipher( char *buffer, int buffer_length )
{
	int offset = buffer_length + 128;
	for ( int i = 0; i < buffer_length; ++i )
	{
		*buffer ^= ( unsigned char )buffer_length;
		*buffer = ( *buffer + offset ) % 256;
		*buffer = ROTATE_LEFT( ( unsigned char )*buffer, offset % 8 );

		buffer++;
		--offset;
	}
}

void decode_cipher( char *buffer, int buffer_length )
{
	int offset = buffer_length + 128;
	for ( int i = buffer_length; i > 0; --i )
	{
		*buffer = ROTATE_RIGHT( ( unsigned char )*buffer, offset % 8 );
		*buffer = ( *buffer - offset ) % 256;
		*buffer ^= ( unsigned char )buffer_length;

		buffer++;
		--offset;
	}
}

char read_config()
{
	char status = 0;

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\web_server_settings\0", 21 );
	base_directory[ base_directory_length + 20 ] = 0;	// Sanity.

	// Open our config file if it exists.
	HANDLE hFile_cfg = CreateFile( base_directory, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, pos = 0;
		DWORD fz = GetFileSize( hFile_cfg, NULL );

		// Our config file is going to be small. If it's something else, we're not going to read it.
		if ( fz >= 31 && fz < 1024 )
		{
			char *cfg_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * fz + 1 );

			ReadFile( hFile_cfg, cfg_buf, sizeof( char ) * fz, &read, NULL );

			cfg_buf[ fz ] = 0;	// Guarantee a NULL terminated buffer.

			// Read the config. It must be in the order specified below.
			if ( read == fz && _memcmp( cfg_buf, MAGIC_ID_WS_SETTINGS, 4 ) == 0 )
			{
				char *next = cfg_buf + 4;

				cfg_enable_web_server = ( *next == 1 ? true : false );
				next += sizeof( bool );

				_memcpy_s( &cfg_address_type, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );

				_memcpy_s( &cfg_ip_address, sizeof( unsigned long ), next, sizeof( unsigned long ) );
				next += sizeof( unsigned long );

				_memcpy_s( &cfg_port, sizeof( unsigned short ), next, sizeof( unsigned short ) );
				next += sizeof( unsigned short );

				_memcpy_s( &cfg_use_authentication, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_auto_start, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_verify_origin, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_allow_keep_alive_requests, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_resource_cache_size, sizeof( unsigned long long ), next, sizeof( unsigned long long ) );
				next += sizeof( unsigned long long );

				_memcpy_s( &cfg_thread_count, sizeof( unsigned long ), next, sizeof( unsigned long ) );
				next += sizeof( unsigned long );

				_memcpy_s( &cfg_enable_ssl, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_certificate_type, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );

				_memcpy_s( &cfg_ssl_version, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );

				int string_length = 0;
				int cfg_val_length = 0;

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					// Length of the string - not including the NULL character.
					string_length = ( unsigned char )*next;
					++next;

					if ( string_length > 0 )
					{
						if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
						{
							// string_length does not contain the NULL character of the string.
							char *authentication_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
							_memcpy_s( authentication_username, string_length, next, string_length );
							authentication_username[ string_length ] = 0; // Sanity;

							decode_cipher( authentication_username, string_length );

							// Read password.
							cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, authentication_username, string_length + 1, NULL, 0 );	// Include the NULL character.
							cfg_authentication_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
							MultiByteToWideChar( CP_UTF8, 0, authentication_username, string_length + 1, cfg_authentication_username, cfg_val_length );

							GlobalFree( authentication_username );

							next += string_length;
						}
						else
						{
							read = 0;
						}
					}
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					// Length of the string - not including the NULL character.
					string_length = ( unsigned char )*next;
					++next;

					if ( string_length > 0 )
					{
						if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
						{
							// string_length does not contain the NULL character of the string.
							char *authentication_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
							_memcpy_s( authentication_password, string_length, next, string_length );
							authentication_password[ string_length ] = 0; // Sanity;

							decode_cipher( authentication_password, string_length );

							// Read password.
							cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, authentication_password, string_length + 1, NULL, 0 );	// Include the NULL character.
							cfg_authentication_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
							MultiByteToWideChar( CP_UTF8, 0, authentication_password, string_length + 1, cfg_authentication_password, cfg_val_length );

							GlobalFree( authentication_password );

							next += string_length;
						}
						else
						{
							read = 0;
						}
					}
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					// Length of the string - not including the NULL character.
					_memcpy_s( &string_length, sizeof( unsigned short ), next, sizeof( unsigned short ) );
					next += sizeof( unsigned short );

					if ( string_length > 0 )
					{
						if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
						{
							// string_length does not contain the NULL character of the string.
							char *certificate_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
							_memcpy_s( certificate_password, string_length, next, string_length );
							certificate_password[ string_length ] = 0; // Sanity;

							decode_cipher( certificate_password, string_length );

							// Read password.
							cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, certificate_password, string_length + 1, NULL, 0 );	// Include the NULL character.
							cfg_certificate_pkcs_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
							MultiByteToWideChar( CP_UTF8, 0, certificate_password, string_length + 1, cfg_certificate_pkcs_password, cfg_val_length );

							GlobalFree( certificate_password );

							next += string_length;
						}
						else
						{
							read = 0;
						}
					}
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_hostname, cfg_val_length );

					next += string_length;
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					if ( string_length > 1 )
					{
						cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
						cfg_document_root_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
						MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_document_root_directory, cfg_val_length );

						g_document_root_directory_length = cfg_val_length - 1;	// Store the base directory length.
					}

					next += string_length;
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_certificate_pkcs_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_certificate_pkcs_file_name, cfg_val_length );

					next += string_length;
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_certificate_cer_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_certificate_cer_file_name, cfg_val_length );

					next += string_length;
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_certificate_key_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_certificate_key_file_name, cfg_val_length );

					next += string_length;
				}

				// Set the default values for bad configuration values.

				if ( cfg_port == 0 )
				{
					cfg_port = 1;
				}

				if ( cfg_thread_count > max_threads )
				{
					cfg_thread_count = max( ( max_threads / 2 ), 1 );
				}
				else if ( cfg_thread_count == 0 )
				{
					cfg_thread_count = 1;
				}
				
				if ( cfg_ssl_version > 4 )
				{
					cfg_ssl_version = 4;	// TLS 1.2.
				}
			}
			else
			{
				status = -2;	// Bad file format.
			}

			GlobalFree( cfg_buf );
		}
		else
		{
			status = -3;	// Incorrect file size.
		}

		CloseHandle( hFile_cfg );
	}
	else
	{
		status = -1;	// Can't open file for reading.
	}

	if ( cfg_hostname == NULL )
	{
		cfg_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 10 );
		_wmemcpy_s( cfg_hostname, 10, L"localhost\0", 10 );
		cfg_hostname[ 9 ] = 0;	// Sanity.
	}

	if ( cfg_document_root_directory == NULL )
	{
		cfg_document_root_directory = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );
		g_document_root_directory_length = GetCurrentDirectoryW( MAX_PATH, cfg_document_root_directory );
		if ( g_document_root_directory_length + 8 <= MAX_PATH )
		{
			_wmemcpy_s( cfg_document_root_directory + g_document_root_directory_length, MAX_PATH - g_document_root_directory_length, L"\\htdocs\0", 8 );
			cfg_document_root_directory[ g_document_root_directory_length + 7 ] = 0;	// Sanity.

			g_document_root_directory_length += 7;
		}

		// Create the document root folder if the folder does not exits.
		if ( GetFileAttributesW( cfg_document_root_directory ) == INVALID_FILE_ATTRIBUTES )
		{
			CreateDirectoryW( cfg_document_root_directory, NULL );
		}
	}

	return status;
}

char save_config()
{
	char status = 0;

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\web_server_settings\0", 21 );
	base_directory[ base_directory_length + 20 ] = 0;	// Sanity.

	// Open our config file if it exists.
	HANDLE hFile_cfg = CreateFile( base_directory, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		int size = ( sizeof( bool ) * 6 ) + ( sizeof( char ) * 7 ) + ( sizeof( unsigned short ) * 1 ) + ( sizeof( unsigned int ) * 2 ) + ( sizeof( unsigned long long ) * 1 );
		int pos = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_WS_SETTINGS, sizeof( char ) * 4 );	// Magic identifier for the web server's settings.
		pos += ( sizeof( char ) * 4 );
		
		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_web_server, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_address_type, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_ip_address, sizeof( unsigned long ) );
		pos += sizeof( unsigned long );

		_memcpy_s( write_buf + pos, size - pos, &cfg_port, sizeof( unsigned short ) );
		pos += sizeof( unsigned short );

		_memcpy_s( write_buf + pos, size - pos, &cfg_use_authentication, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_auto_start, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_verify_origin, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_allow_keep_alive_requests, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_resource_cache_size, sizeof( unsigned long long ) );
		pos += sizeof( unsigned long long );

		_memcpy_s( write_buf + pos, size - pos, &cfg_thread_count, sizeof( unsigned long ) );
		pos += sizeof( unsigned long );

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_ssl, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_certificate_type, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_ssl_version, sizeof( unsigned char ) );
		//pos += sizeof( unsigned char );

		DWORD write = 0;
		WriteFile( hFile_cfg, write_buf, size, &write, NULL );

		GlobalFree( write_buf );

		int cfg_val_length = 0;
		char *utf8_cfg_val = NULL;

		if ( cfg_authentication_username != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_username, -1, NULL, 0, NULL, NULL ) + sizeof( char );	// Add 1 byte for our encoded length.
			utf8_cfg_val = ( char * )GlobalAlloc( GPTR, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_username, -1, utf8_cfg_val + sizeof( char ), cfg_val_length - sizeof( char ), NULL, NULL );

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( char ) );

			encode_cipher( utf8_cfg_val + sizeof( char ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( char ), &write, NULL );	// Do not write the NULL terminator.

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_authentication_password != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_password, -1, NULL, 0, NULL, NULL ) + sizeof( char );	// Add 1 byte for our encoded length.
			utf8_cfg_val = ( char * )GlobalAlloc( GPTR, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_password, -1, utf8_cfg_val + sizeof( char ), cfg_val_length - sizeof( char ), NULL, NULL );

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( char ) );

			encode_cipher( utf8_cfg_val + sizeof( char ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( char ), &write, NULL );	// Do not write the NULL terminator.

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_certificate_pkcs_password != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_certificate_pkcs_password, -1, NULL, 0, NULL, NULL ) + sizeof( unsigned short );	// Add 2 bytes for our encoded length.
			utf8_cfg_val = ( char * )GlobalAlloc( GPTR, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_certificate_pkcs_password, -1, utf8_cfg_val + sizeof( unsigned short ), cfg_val_length - sizeof( unsigned short ), NULL, NULL );

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( unsigned short ) );

			encode_cipher( utf8_cfg_val + sizeof( unsigned short ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( unsigned short ), &write, NULL );	// Do not write the NULL terminator.

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0\0", 2, &write, NULL );
		}

		if ( cfg_hostname != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_hostname, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_hostname, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_document_root_directory != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_document_root_directory, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_document_root_directory, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_certificate_pkcs_file_name != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_certificate_pkcs_file_name, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_certificate_pkcs_file_name, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_certificate_cer_file_name != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_certificate_cer_file_name, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_certificate_cer_file_name, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_certificate_key_file_name != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_certificate_key_file_name, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_certificate_key_file_name, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		CloseHandle( hFile_cfg );
	}
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}

void PreloadIndexFile( dllrbt_tree *resource_cache, unsigned long long *resource_cache_size, unsigned long long maximum_resource_cache_size )
{
	if ( resource_cache != NULL && resource_cache_size != NULL && maximum_resource_cache_size > 0 )
	{
		wchar_t file_path[ MAX_PATH ];
		_memzero( file_path, MAX_PATH );

		// First try to open an index that ends with ".html".
		_wmemcpy_s( file_path, MAX_PATH, cfg_document_root_directory, g_document_root_directory_length );
		_wmemcpy_s( file_path + g_document_root_directory_length, MAX_PATH - g_document_root_directory_length, L"\\index.html\0", 12 );
		file_path[ g_document_root_directory_length + 11 ] = 0;	// Sanity.

		HANDLE hFile_index = CreateFile( file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		
		// If the index file does not end with ".html", then see if it ends with ".htm".
		if ( hFile_index == INVALID_HANDLE_VALUE )
		{
			file_path[ g_document_root_directory_length + 10 ] = 0;	// Set the 'l' to NULL.
			hFile_index = CreateFile( file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		}

		if ( hFile_index != INVALID_HANDLE_VALUE )
		{
			char *resource_buf = NULL;
			DWORD bytes_read = 0;

			DWORD file_size = GetFileSize( hFile_index, NULL );

			// Make sure our file can fit into the resource cache.
			if ( file_size <= maximum_resource_cache_size )
			{
				// Make sure we have data to allocate and read.
				if ( file_size > 0 )
				{
					resource_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * file_size );

					ReadFile( hFile_index, resource_buf, file_size, &bytes_read, NULL );
				}

				// Create our cache item to add to the tree.
				RESOURCE_CACHE_ITEM *rci = ( RESOURCE_CACHE_ITEM * )GlobalAlloc( GMEM_FIXED, sizeof( RESOURCE_CACHE_ITEM ) );
				rci->resource_path = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) );
				rci->resource_path[ 0 ] = 0;	// Sanity.
				rci->resource_data = resource_buf;
				rci->resource_data_size = file_size;

				// Insert the resource into the tree.
				if ( dllrbt_insert( resource_cache, ( void * )rci->resource_path, ( void * )rci ) != DLLRBT_STATUS_OK )
				{
					// This shouldn't happen.
					GlobalFree( rci->resource_data );
					GlobalFree( rci->resource_path );
					GlobalFree( rci );
				}
				else
				{
					*resource_cache_size += file_size;
				}
			}

			CloseHandle( hFile_index );
		}
	}
}
