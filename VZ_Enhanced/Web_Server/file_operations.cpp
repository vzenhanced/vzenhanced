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

#include "globals.h"

#include "file_operations.h"
#include "utilities.h"
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

void read_config()
{
	//DWORD length = GetCurrentDirectory( MAX_PATH, settings_path );	// Get the full path
	_memcpy_s( base_directory + base_directory_length, sizeof( wchar_t ) * ( MAX_PATH - base_directory_length ), L"\\web_server_settings.bin\0", sizeof( wchar_t ) * ( 25 ) );
	base_directory[ base_directory_length + 24 ] = 0;	// Sanity.

	// Open our config file if it exists.
	HANDLE hFile_cfg = CreateFile( base_directory, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, pos = 0;
		DWORD fz = GetFileSize( hFile_cfg, NULL );

		// Our config file is going to be small. If it's something else, we're not going to read it.
		if ( fz >= 18 && fz < 1024 )
		{
			char *cfg_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * fz + 1 );

			ReadFile( hFile_cfg, cfg_buf, sizeof( char ) * fz, &read, NULL );

			cfg_buf[ fz ] = 0;	// Guarantee a NULL terminated buffer.

			// Read the config. It must be in the order specified below.
			if ( read == fz )
			{
				char *next = cfg_buf;// + 3;

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
					string_length = ( unsigned char )*next;
					++next;

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
			}

			GlobalFree( cfg_buf );
		}

		CloseHandle( hFile_cfg );
	}

	if ( cfg_hostname == NULL )
	{
		cfg_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof ( wchar_t ) * 10 );
		_wmemcpy_s( cfg_hostname, 10, L"localhost\0", 10 );
		cfg_hostname[ 9 ] = 0;	// Sanity.
	}

	if ( cfg_document_root_directory == NULL )
	{
		cfg_document_root_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof ( wchar_t ) * MAX_PATH );
		_memzero( cfg_document_root_directory, sizeof ( wchar_t ) * MAX_PATH );
		g_document_root_directory_length = GetCurrentDirectoryW( MAX_PATH, cfg_document_root_directory );
		if ( g_document_root_directory_length + 8 <= MAX_PATH )
		{
			_memcpy_s( cfg_document_root_directory + g_document_root_directory_length, sizeof( wchar_t ) * ( MAX_PATH - g_document_root_directory_length ), L"\\htdocs\0", sizeof( wchar_t ) * 8 );
			cfg_document_root_directory[ g_document_root_directory_length + 7 ] = 0;	// Sanity.

			g_document_root_directory_length += 7;
		}

		// Create the document root folder if the folder does not exits.
		if ( GetFileAttributesW( cfg_document_root_directory ) == INVALID_FILE_ATTRIBUTES )
		{
			CreateDirectoryW( cfg_document_root_directory, NULL );
		}
	}

	if ( cfg_thread_count > max_threads )
	{
		cfg_thread_count = max_threads;
	}
	else if ( cfg_thread_count == 0 )
	{
		cfg_thread_count = 2;
	}
}

void save_config()
{
	_memcpy_s( base_directory + base_directory_length, sizeof( wchar_t ) * ( MAX_PATH - base_directory_length ), L"\\web_server_settings.bin\0", sizeof( wchar_t ) * ( 25 ) );
	base_directory[ base_directory_length + 24 ] = 0;	// Sanity.

	// Open our config file if it exists.
	HANDLE hFile_cfg = CreateFile( base_directory, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		int size = ( sizeof( bool ) * 5 ) + ( sizeof( char ) * 3 ) + ( sizeof( unsigned short ) * 1 ) + ( sizeof( unsigned int ) * 2 );
		int pos = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );
		
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
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_username, -1, utf8_cfg_val + sizeof( char ), cfg_val_length - sizeof( char ), NULL, NULL );

			int length = cfg_val_length - 1;
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( char ) );

			encode_cipher( utf8_cfg_val + sizeof( char ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );	// Do not write the NULL terminator.

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_authentication_password != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_password, -1, NULL, 0, NULL, NULL ) + sizeof( char );	// Add 1 byte for our encoded length.
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_password, -1, utf8_cfg_val + sizeof( char ), cfg_val_length - sizeof( char ), NULL, NULL );

			int length = cfg_val_length - 1;
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( char ) );

			encode_cipher( utf8_cfg_val + sizeof( char ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );	// Do not write the NULL terminator.

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_certificate_pkcs_password != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_certificate_pkcs_password, -1, NULL, 0, NULL, NULL ) + sizeof( char );	// Add 1 byte for our encoded length.
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_certificate_pkcs_password, -1, utf8_cfg_val + sizeof( char ), cfg_val_length - sizeof( char ), NULL, NULL );

			int length = cfg_val_length - 1;
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( char ) );

			encode_cipher( utf8_cfg_val + sizeof( char ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );	// Do not write the NULL terminator.

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
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
}

void PreloadIndexFile()
{
	if ( index_file_buf != NULL )
	{
		GlobalFree( index_file_buf );
		index_file_buf = NULL;
	}
	index_file_buf_length = 0;

	wchar_t file_path[ MAX_PATH ];
	_memzero( file_path, MAX_PATH );

	_wmemcpy_s( file_path, MAX_PATH, cfg_document_root_directory, g_document_root_directory_length );
	_wmemcpy_s( file_path + g_document_root_directory_length, MAX_PATH - g_document_root_directory_length, L"\\index.html\0", 12 );

	HANDLE hFile_index = CreateFile( file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_index == INVALID_HANDLE_VALUE )
	{
		file_path[ g_document_root_directory_length + 10 ] = 0;
		hFile_index = CreateFile( file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	}

	if ( hFile_index != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0;

		index_file_buf_length = GetFileSize( hFile_index, NULL );

		index_file_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * index_file_buf_length );

		ReadFile( hFile_index, index_file_buf, sizeof( char ) * index_file_buf_length, &read, NULL );

		CloseHandle( hFile_index );
	}
}