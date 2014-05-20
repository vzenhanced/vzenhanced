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

RANGE *ignore_range_list[ 16 ];
RANGE *forward_range_list[ 16 ];

dllrbt_tree *ignore_list = NULL;
bool ignore_list_changed = false;

dllrbt_tree *forward_list = NULL;
bool forward_list_changed = false;

void read_config()
{
	//DWORD length = GetCurrentDirectory( MAX_PATH, settings_path );	// Get the full path
	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\settings.bin\0", 14 );
	base_directory[ base_directory_length + 13 ] = 0;	// Sanity.

	// Open our config file if it exists.
	HANDLE hFile_cfg = CreateFile( base_directory, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, pos = 0;
		DWORD fz = GetFileSize( hFile_cfg, NULL );

		// Our config file is going to be small. If it's something else, we're not going to read it.
		if ( fz >= 298 && fz < 10240 )
		{
			char *cfg_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * fz + 1 );

			ReadFile( hFile_cfg, cfg_buf, sizeof( char ) * fz, &read, NULL );

			cfg_buf[ fz ] = 0;	// Guarantee a NULL terminated buffer.

			// Read the config. It must be in the order specified below.
			if ( read == fz )
			{
				char *next = cfg_buf;// + 3;

				cfg_remember_login = ( *next == 1 ? true : false );
				next += sizeof( bool );

				_memcpy_s( &cfg_connection_auto_login, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_connection_reconnect, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_download_pictures, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_connection_retries, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_connection_timeout, sizeof( unsigned short ), next, sizeof( unsigned short ) );
				next += sizeof( unsigned short );
				_memcpy_s( &cfg_connection_ssl_version, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_check_for_updates, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_pos_x, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_pos_y, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_width, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_height, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_tab_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_tab_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_tab_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_tab_order4, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_column_width1, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width2, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width3, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width4, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width5, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width6, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width7, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width8, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width9, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_column2_width1, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width2, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width3, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width4, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width5, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width6, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width7, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width8, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_column2_width9, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width10, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width11, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width12, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width13, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width14, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width15, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width16, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column2_width17, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_column3_width1, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column3_width2, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column3_width3, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column3_width4, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_column4_width1, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column4_width2, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column4_width3, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_column_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order4, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order5, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order6, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order7, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order8, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order9, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_column2_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order4, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order5, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order6, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order7, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order8, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_column2_order9, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order10, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order11, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order12, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order13, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order14, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order15, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order16, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column2_order17, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_column3_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column3_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column3_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column3_order4, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_column4_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column4_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column4_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_tray_icon, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_close_to_tray, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_minimize_to_tray, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_silent_startup, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_always_on_top, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_enable_call_log_history, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_enable_popups, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

				_memcpy_s( &cfg_popup_hide_border, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_popup_width, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_popup_height, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_popup_position, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );

				_memcpy_s( &cfg_popup_time, sizeof( unsigned short ), next, sizeof( unsigned short ) );
				next += sizeof( unsigned short );
				_memcpy_s( &cfg_popup_transparency, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );

				_memcpy_s( &cfg_popup_gradient, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_popup_gradient_direction, sizeof( bool ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_background_color1, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_background_color2, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );

				_memcpy_s( &cfg_popup_font_color1, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_font_color2, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_font_color3, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_font_height1, sizeof( LONG ), next, sizeof( LONG ) );
				next += sizeof( LONG );
				_memcpy_s( &cfg_popup_font_height2, sizeof( LONG ), next, sizeof( LONG ) );
				next += sizeof( LONG );
				_memcpy_s( &cfg_popup_font_height3, sizeof( LONG ), next, sizeof( LONG ) );
				next += sizeof( LONG );
				_memcpy_s( &cfg_popup_font_weight1, sizeof( LONG ), next, sizeof( LONG ) );
				next += sizeof( LONG );
				_memcpy_s( &cfg_popup_font_weight2, sizeof( LONG ), next, sizeof( LONG ) );
				next += sizeof( LONG );
				_memcpy_s( &cfg_popup_font_weight3, sizeof( LONG ), next, sizeof( LONG ) );
				next += sizeof( LONG );
				_memcpy_s( &cfg_popup_font_italic1, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_italic2, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_italic3, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_underline1, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_underline2, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_underline3, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_strikeout1, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_strikeout2, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_strikeout3, sizeof( BYTE ), next, sizeof( BYTE ) );
				next += sizeof( BYTE );
				_memcpy_s( &cfg_popup_font_shadow1, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_popup_font_shadow2, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_popup_font_shadow3, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );
				_memcpy_s( &cfg_popup_font_shadow_color1, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_font_shadow_color2, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_font_shadow_color3, sizeof( COLORREF ), next, sizeof( COLORREF ) );
				next += sizeof( COLORREF );
				_memcpy_s( &cfg_popup_justify_line1, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_justify_line2, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_justify_line3, sizeof( unsigned char ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_line_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_popup_line_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_popup_line_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_popup_time_format, sizeof( bool ), next, sizeof( unsigned char ) );
				next += sizeof( unsigned char );
				_memcpy_s( &cfg_popup_play_sound, sizeof( bool ), next, sizeof( bool ) );
				next += sizeof( bool );

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
							char *username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
							_memcpy_s( username, string_length, next, string_length );
							username[ string_length ] = 0; // Sanity;

							decode_cipher( username, string_length );

							// Read username.
							cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, username, string_length + 1, NULL, 0 );	// Include the NULL character.
							cfg_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
							MultiByteToWideChar( CP_UTF8, 0, username, string_length + 1, cfg_username, cfg_val_length );

							GlobalFree( username );

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
							char *password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
							_memcpy_s( password, string_length, next, string_length );
							password[ string_length ] = 0; // Sanity;

							decode_cipher( password, string_length );

							// Read password.
							cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, password, string_length + 1, NULL, 0 );	// Include the NULL character.
							cfg_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
							MultiByteToWideChar( CP_UTF8, 0, password, string_length + 1, cfg_password, cfg_val_length );

							GlobalFree( password );

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

					// Read font face.
					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_popup_font_face1 = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_popup_font_face1, cfg_val_length );

					next += string_length;
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					// Read font face.
					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_popup_font_face2 = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_popup_font_face2, cfg_val_length );

					next += string_length;
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					// Read font face.
					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_popup_font_face3 = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_popup_font_face3, cfg_val_length );

					next += string_length;
				}

				if ( ( DWORD )( next - cfg_buf ) < read )
				{
					string_length = lstrlenA( next ) + 1;

					// Read sound.
					cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
					cfg_popup_sound = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
					MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_popup_sound, cfg_val_length );

					next += string_length;
				}

				// Set the default values for bad configuration values.

				if ( cfg_tray_icon == false )
				{
					cfg_silent_startup = false;
				}

				// These should never be negative.
				if ( cfg_popup_time > 300 || cfg_popup_time < 0 ) cfg_popup_time = 10;
				if ( cfg_connection_retries > 10 || cfg_connection_retries < 0 ) cfg_connection_retries = 3;
				if ( cfg_connection_timeout > 300 || cfg_connection_timeout < 0 ) cfg_connection_timeout = 15;

				CheckColumnOrders( 0, list_columns, NUM_COLUMNS );
				CheckColumnOrders( 1, contact_list_columns, NUM_COLUMNS2 );
				CheckColumnOrders( 2, forward_list_columns, NUM_COLUMNS3 );
				CheckColumnOrders( 3, ignore_list_columns, NUM_COLUMNS4 );

				// Revert column widths if they exceed our limits.
				CheckColumnWidths();
			}

			GlobalFree( cfg_buf );
		}

		CloseHandle( hFile_cfg );
	}
}

void save_config()
{
	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\settings.bin\0", 14 );
	base_directory[ base_directory_length + 13 ] = 0;	// Sanity.

	// Open our config file if it exists.
	HANDLE hFile_cfg = CreateFile( base_directory, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		int size = ( sizeof( int ) * 53 ) + ( sizeof( bool ) * 18 ) + ( sizeof( char ) * 58 ) + ( sizeof( unsigned short ) * 2 );// + ( sizeof( wchar_t ) * 96 );
		int pos = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );
		
		_memcpy_s( write_buf + pos, size - pos, &cfg_remember_login, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_connection_auto_login, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_connection_reconnect, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_download_pictures, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_connection_retries, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_connection_timeout, sizeof( unsigned short ) );
		pos += sizeof( unsigned short );
		_memcpy_s( write_buf + pos, size - pos, &cfg_connection_ssl_version, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_check_for_updates, sizeof( bool ) );
		pos += sizeof( bool );
		
		_memcpy_s( write_buf + pos, size - pos, &cfg_pos_x, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_pos_y, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_width, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_height, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_tab_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_tab_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_tab_order3, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_tab_order4, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width1, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width2, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width3, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width4, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width5, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width6, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width7, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width8, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width9, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width1, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width2, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width3, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width4, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width5, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width6, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width7, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width8, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width9, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width10, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width11, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width12, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width13, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width14, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width15, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width16, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_width17, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_width1, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_width2, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_width3, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_width4, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_width1, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_width2, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_width3, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order3, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order4, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order5, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order6, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order7, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order8, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order9, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order3, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order4, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order5, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order6, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order7, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order8, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order9, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order10, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order11, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order12, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order13, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order14, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order15, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order16, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column2_order17, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_order3, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column3_order4, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column4_order3, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_tray_icon, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_close_to_tray, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_minimize_to_tray, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_silent_startup, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_always_on_top, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_call_log_history, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_popups, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_hide_border, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_width, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_height, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_position, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_time, sizeof( unsigned short ) );
		pos += sizeof( unsigned short );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_transparency, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_gradient, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_gradient_direction, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_background_color1, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_background_color2, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );

		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_color1, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_color2, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_color3, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_height1, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_height2, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_height3, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_weight1, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_weight2, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_weight3, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_italic1, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_italic2, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_italic3, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_underline1, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_underline2, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_underline3, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_strikeout1, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_strikeout2, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_strikeout3, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_shadow1, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_shadow2, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_shadow3, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_shadow_color1, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_shadow_color2, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_font_shadow_color3, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_justify_line1, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_justify_line2, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_justify_line3, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_line_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_line_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_line_order3, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_time_format, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_popup_play_sound, sizeof( bool ) );
		//pos += sizeof( bool );

		DWORD write = 0;
		WriteFile( hFile_cfg, write_buf, size, &write, NULL );


		GlobalFree( write_buf );

		int cfg_val_length = 0;
		char *utf8_cfg_val = NULL;

		// Only save the username and password if we are set to remember them.
		if ( cfg_remember_login == true )
		{
			if ( cfg_username != NULL )
			{
				cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_username, -1, NULL, 0, NULL, NULL ) + sizeof( char );	// Add 1 byte for our encoded length.
				utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
				cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_username, -1, utf8_cfg_val + sizeof( char ), cfg_val_length - sizeof( char ), NULL, NULL );

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

			if ( cfg_password != NULL )
			{
				cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_password, -1, NULL, 0, NULL, NULL ) + sizeof( char );	// Add 1 byte for our encoded length.
				utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
				cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_password, -1, utf8_cfg_val + sizeof( char ), cfg_val_length - sizeof( char ), NULL, NULL );

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
		}
		else	// Save two NULL characters instead.
		{
			WriteFile( hFile_cfg, "\0\0", 2, &write, NULL );
		}

		if ( cfg_popup_font_face1 != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_font_face1, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_font_face1, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_popup_font_face2 != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_font_face2, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_font_face2, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_popup_font_face3 != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_font_face3, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_font_face3, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );

			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}

		if ( cfg_popup_sound != NULL )
		{
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_sound, -1, NULL, 0, NULL, NULL );
			utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
			cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_popup_sound, -1, utf8_cfg_val, cfg_val_length, NULL, NULL );

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


void read_ignore_list()
{
	ignore_list = dllrbt_create( dllrbt_compare );

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\ignore.txt\0", 12 );
	base_directory[ base_directory_length + 11 ] = 0;	// Sanity.

	// Open our config file if it exists.
	HANDLE hFile_ignore = CreateFile( base_directory, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_ignore != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0;

		DWORD fz = GetFileSize( hFile_ignore, NULL );

		char *ignore_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 32768 + 1 ) );	// This buffer must be greater than, or equal to 29 bytes.

		bool skip_next_newline = false;

		while ( total_read < fz )
		{
			ReadFile( hFile_ignore, ignore_buf, sizeof( char ) * 32768, &read, NULL );
			ignore_buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

			total_read += read;

			char *p = ignore_buf;
			char *s = ignore_buf;

			while ( p != NULL && *s != NULL )
			{
				// If we read a partial number and it was too long.
				if ( skip_next_newline == true )
				{
					skip_next_newline = false;

					// Then we skip the remainder of the number.
					p = _StrStrA( s, "\r\n" );

					// If it was the last number, then we're done.
					if ( p == NULL )
					{
						continue;
					}

					// Move to the next value.
					p += 2;
					s = p;
				}

				// Find the newline token.
				p = _StrStrA( s, "\r\n" );

				// If we found one, copy the number.
				if ( p != NULL )
				{
					*p = 0;

					char *t = _StrChrA( s, ':' );	// Find the ignore count.

					DWORD length1 = 0;
					DWORD length2 = 0;

					if ( t == NULL )
					{
						length1 = ( p - s );
					}
					else
					{
						length1 = ( t - s );
						++t;
						length2 = ( ( p - t ) > 10 ? 10 : ( p - t ) );
					}

					// Make sure the number is at most 15 + 1 digits.
					if ( ( length1 <= 16 && length1 > 0 ) && ( length2 <= 10 && length2 >= 0 ) )
					{
						ignoreinfo *ii = ( ignoreinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreinfo ) );

						ii->c_phone_number = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length1 + 1 ) );
						_memcpy_s( ii->c_phone_number, length1 + 1, s, length1 );
						*( ii->c_phone_number + length1 ) = 0;	// Sanity

						ii->phone_number = FormatPhoneNumber( ii->c_phone_number );

						if ( length2 == 0 )
						{
							ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
							*ii->c_total_calls = '0';
							*( ii->c_total_calls + 1 ) = 0;	// Sanity

							ii->count = 0;
						}
						else
						{
							ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length2 + 1 ) );
							_memcpy_s( ii->c_total_calls, length2 + 1, t, length2 );
							*( ii->c_total_calls + length2 ) = 0;	// Sanity

							ii->count = _strtoul( ii->c_total_calls, NULL, 10 );
						}

						int val_length = MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
						wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
						MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, val, val_length );

						ii->total_calls = val;

						ii->state = 0;

						if ( dllrbt_insert( ignore_list, ( void * )ii->c_phone_number, ( void * )ii ) != DLLRBT_STATUS_OK )
						{
							free_ignoreinfo( &ii );
						}
						else if ( is_num( ii->c_phone_number ) == 1 )	// See if the value we're adding is a range (has wildcard values in it).
						{
							RangeAdd( &ignore_range_list[ length1 - 1 ], ii->c_phone_number, length1 );
						}
					}

					// Move to the next value.
					p += 2;
					s = p;
				}
				else if ( total_read < fz )	// Reached the end of what we've read.
				{
					// If we didn't find a token and haven't finished reading the entire file, then offset the file pointer back to the end of the last valid number.
					DWORD offset = ( ( ignore_buf + read ) - s );

					char *t = _StrChrA( s, ':' );	// Find the ignore count.

					DWORD length1 = 0;
					DWORD length2 = 0;

					if ( t == NULL )
					{
						length1 = offset;
					}
					else
					{
						length1 = ( t - s );
						++t;
						length2 = ( ( ignore_buf + read ) - t );
					}

					// Max recommended number length is 15 + 1.
					// We can only offset back less than what we've read, and no more than 27 digits (bytes). 15 + 1 + 1 + 10
					// This means read, and our buffer size should be greater than 27.
					// Based on the token we're searching for: "\r\n", the buffer NEEDS to be greater or equal to 29.
					if ( offset < read && offset <= 16 && length2 <= 10 )
					{
						total_read -= offset;
						SetFilePointer( hFile_ignore, total_read, NULL, FILE_BEGIN );
					}
					else	// The length of the next number is too long.
					{
						skip_next_newline = true;
					}
				}
				else	// We reached the EOF and there was no newline token.
				{
					char *t = _StrChrA( s, ':' );	// Find the ignore count.

					DWORD length1 = 0;
					DWORD length2 = 0;

					if ( t == NULL )
					{
						length1 = ( ( ignore_buf + read ) - s );
					}
					else
					{
						length1 = ( t - s );
						++t;
						length2 = ( ( ( ignore_buf + read ) - t ) > 10 ? 10 : ( ( ignore_buf + read ) - t ) );
					}

					// Make sure the number is at most 15 + 1 digits.
					if ( ( length1 <= 16 && length1 > 0 ) && ( length2 <= 10 && length2 >= 0 ) )
					{
						ignoreinfo *ii = ( ignoreinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreinfo ) );

						ii->c_phone_number = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length1 + 1 ) );
						_memcpy_s( ii->c_phone_number, length1 + 1, s, length1 );
						*( ii->c_phone_number + length1 ) = 0;	// Sanity

						ii->phone_number = FormatPhoneNumber( ii->c_phone_number );

						if ( length2 == 0 )
						{
							ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
							*ii->c_total_calls = '0';
							*( ii->c_total_calls + 1 ) = 0;	// Sanity

							ii->count = 0;
						}
						else
						{
							ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length2 + 1 ) );
							_memcpy_s( ii->c_total_calls, length2 + 1, t, length2 );
							*( ii->c_total_calls + length2 ) = 0;	// Sanity

							ii->count = _strtoul( ii->c_total_calls, NULL, 10 );
						}

						int val_length = MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
						wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
						MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, val, val_length );

						ii->total_calls = val;

						ii->state = 0;

						if ( dllrbt_insert( ignore_list, ( void * )ii->c_phone_number, ( void * )ii ) != DLLRBT_STATUS_OK )
						{
							free_ignoreinfo( &ii );
						}
						else if ( is_num( ii->c_phone_number ) == 1 )	// See if the value we're adding is a range (has wildcard values in it).
						{
							RangeAdd( &ignore_range_list[ length1 - 1 ], ii->c_phone_number, length1 );
						}
					}
				}
			}
		}

		GlobalFree( ignore_buf );

		CloseHandle( hFile_ignore );
	}
}

void save_ignore_list()
{
	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\ignore.txt\0", 12 );
	base_directory[ base_directory_length + 11 ] = 0;	// Sanity

	// Open our config file if it exists.
	HANDLE hFile_ignore = CreateFile( base_directory, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_ignore != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );	// This buffer must be greater than, or equal to 29 bytes.

		node_type *node = dllrbt_get_head( ignore_list );
		while ( node != NULL )
		{
			ignoreinfo *ii = ( ignoreinfo * )node->val;

			if ( ii != NULL && ii->c_phone_number != NULL && ii->c_total_calls != NULL )
			{
				int phone_number_length = lstrlenA( ii->c_phone_number );
				int total_calls_length = lstrlenA( ii->c_total_calls );

				// See if the next number can fit in the buffer. If it can't, then we dump the buffer.
				if ( pos + phone_number_length + total_calls_length + 3 > size )
				{
					// Dump the buffer.
					WriteFile( hFile_ignore, write_buf, pos, &write, NULL );
					pos = 0;
				}

				// Ignore numbers that are greater than 15 + 1 digits (bytes).
				if ( phone_number_length + total_calls_length + 3 <= 29 )
				{
					// Add to the buffer.
					_memcpy_s( write_buf + pos, size - pos, ii->c_phone_number, phone_number_length );
					pos += phone_number_length;
					write_buf[ pos++ ] = ':';
					_memcpy_s( write_buf + pos, size - pos, ii->c_total_calls, total_calls_length );
					pos += total_calls_length;
					write_buf[ pos++ ] = '\r';
					write_buf[ pos++ ] = '\n';
				}
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile_ignore, write_buf, pos, &write, NULL );
		}

		GlobalFree( write_buf );

		CloseHandle( hFile_ignore );
	}
}

void read_forward_list()
{
	forward_list = dllrbt_create( dllrbt_compare );

	//DWORD length = GetCurrentDirectory( MAX_PATH, forward_list_path );	// Get the full path
	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\forward.txt\0", 13 );
	base_directory[ base_directory_length + 12 ] = 0;	// Sanity.

	// Open our config file if it exists.
	HANDLE hFile_forward = CreateFile( base_directory, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_forward != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0;

		DWORD fz = GetFileSize( hFile_forward, NULL );

		char *forward_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 32768 + 1 ) );	// This buffer must be greater than, or equal to 46 bytes.

		bool skip_next_newline = false;

		while ( total_read < fz )
		{
			ReadFile( hFile_forward, forward_buf, sizeof( char ) * 32768, &read, NULL );
			forward_buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

			total_read += read;

			char *p = forward_buf;
			char *s = forward_buf;

			while ( p != NULL && *s != NULL )
			{
				// If we read a partial number and it was too long.
				if ( skip_next_newline == true )
				{
					skip_next_newline = false;

					// Then we skip the remainder of the number.
					p = _StrStrA( s, "\r\n" );

					// If it was the last number, then we're done.
					if ( p == NULL )
					{
						continue;
					}

					// Move to the next value.
					p += 2;
					s = p;
				}

				// Find the newline token.
				p = _StrStrA( s, "\r\n" );

				// If we found one, copy the number.
				if ( p != NULL )
				{
					*p = 0;

					char *t = _StrChrA( s, ':' );

					// We must have at least a second phone number.
					if ( t != NULL )
					{
						DWORD length1 = ( t - s );
						++t;

						char *t2 = _StrChrA( t, ':' );	// Find the forward count.

						DWORD length2 = 0;
						DWORD length3 = 0;

						if ( t2 == NULL )
						{
							length2 = ( p - t );
						}
						else
						{
							length2 = ( t2 - t );
							++t2;
							length3 = ( ( p - t2 ) > 10 ? 10 : ( p - t2 ) );
						}

						// Make sure the number is at most 15 + 1 digits.
						if ( ( length1 <= 16 && length1 > 0 ) && ( length2 <= 16 && length2 > 0 ) && ( length3 <= 10 && length3 >= 0 ) )
						{
							forwardinfo *fi = ( forwardinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardinfo ) );

							fi->c_call_from = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length1 + 1 ) );
							_memcpy_s( fi->c_call_from, length1 + 1, s, length1 );
							*( fi->c_call_from + length1 ) = 0;	// Sanity

							fi->call_from = FormatPhoneNumber( fi->c_call_from );

							fi->c_forward_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length2 + 1 ) );
							_memcpy_s( fi->c_forward_to, length2 + 1, t, length2 );
							*( fi->c_forward_to + length2 ) = 0;	// Sanity

							fi->forward_to = FormatPhoneNumber( fi->c_forward_to );

							if ( length3 == 0 )
							{
								fi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
								*fi->c_total_calls = '0';
								*( fi->c_total_calls + 1 ) = 0;	// Sanity

								fi->count = 0;
							}
							else
							{
								fi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length3 + 1 ) );
								_memcpy_s( fi->c_total_calls, length3 + 1, t2, length3 );
								*( fi->c_total_calls + length3 ) = 0;	// Sanity

								fi->count = _strtoul( fi->c_total_calls, NULL, 10 );
							}

							int val_length = MultiByteToWideChar( CP_UTF8, 0, fi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
							wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
							MultiByteToWideChar( CP_UTF8, 0, fi->c_total_calls, -1, val, val_length );

							fi->total_calls = val;

							fi->state = 0;

							if ( dllrbt_insert( forward_list, ( void * )fi->c_call_from, ( void * )fi ) != DLLRBT_STATUS_OK )
							{
								free_forwardinfo( &fi );
							}
							else if ( is_num( fi->c_call_from ) == 1 )	// See if the value we're adding is a range (has wildcard values in it).
							{
								RangeAdd( &forward_range_list[ length1 - 1 ], fi->c_call_from, length1 );
							}
						}
					}

					// Move to the next value.
					p += 2;
					s = p;
				}
				else if ( total_read < fz )	// Reached the end of what we've read.
				{
					// If we didn't find a token and haven't finished reading the entire file, then offset the file pointer back to the end of the last valid number.
					DWORD offset = ( ( forward_buf + read ) - s );

					char *t = _StrChrA( s, ':' );

					DWORD length1 = 0;
					DWORD length2 = 0;
					DWORD length3 = 0;

					if ( t == NULL )
					{
						length1 = offset;
					}
					else
					{
						length1 = ( t - s );
						++t;

						s = t;

						t = _StrChrA( s, ':' );	// Find the forward count.

						if ( t == NULL )
						{
							length2 = ( ( forward_buf + read ) - s );
						}
						else
						{
							length2 = ( t - s );
							++t;
							length3 = ( ( ( forward_buf + read ) - t ) > 10 ? 10 : ( ( forward_buf + read ) - t ) );
						}
					}

					// Max recommended number length is 15 + 1.
					// We can only offset back less than what we've read, and no more than 44 bytes. 15 + 1 + 1 + 15 + 1 + 1 + 10.
					// This means read, and our buffer size should be greater than 44.
					// Based on the token we're searching for: "\r\n", the buffer NEEDS to be greater or equal to 46.
					if ( offset < read && length1 <= 16 && length2 <= 16 && length3 <= 10 )
					{
						total_read -= offset;
						SetFilePointer( hFile_forward, total_read, NULL, FILE_BEGIN );
					}
					else	// The length of the next number is too long.
					{
						skip_next_newline = true;
					}
				}
				else	// We reached the EOF and there was no newline token.
				{
					char *t = _StrChrA( s, ':' );

					// We must have at least a second phone number.
					if ( t != NULL )
					{
						DWORD length1 = ( t - s );
						++t;

						char *t2 = _StrChrA( t, ':' );	// Find the forward count.

						DWORD length2 = 0;
						DWORD length3 = 0;

						if ( t2 == NULL )
						{
							length2 = ( ( forward_buf + read ) - t );
						}
						else
						{
							length2 = ( t2 - t );
							++t2;
							length3 = ( ( ( forward_buf + read ) - t2 ) > 10 ? 10 : ( ( forward_buf + read ) - t2 ) );
						}

						// Make sure the number is at most 15 + 1 digits.
						if ( ( length1 <= 16 && length1 > 0 ) && ( length2 <= 16 && length2 > 0 ) && ( length3 <= 10 && length3 >= 0 ) )
						{
							forwardinfo *fi = ( forwardinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardinfo ) );

							fi->c_call_from = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length1 + 1 ) );
							_memcpy_s( fi->c_call_from, length1 + 1, s, length1 );
							*( fi->c_call_from + length1 ) = 0;	// Sanity

							fi->call_from = FormatPhoneNumber( fi->c_call_from );

							fi->c_forward_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length2 + 1 ) );
							_memcpy_s( fi->c_forward_to, length2 + 1, t, length2 );
							*( fi->c_forward_to + length2 ) = 0;	// Sanity

							fi->forward_to = FormatPhoneNumber( fi->c_forward_to );

							if ( length3 == 0 )
							{
								fi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
								*fi->c_total_calls = '0';
								*( fi->c_total_calls + 1 ) = 0;	// Sanity

								fi->count = 0;
							}
							else
							{
								fi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length3 + 1 ) );
								_memcpy_s( fi->c_total_calls, length3 + 1, t2, length3 );
								*( fi->c_total_calls + length3 ) = 0;	// Sanity

								fi->count = _strtoul( fi->c_total_calls, NULL, 10 );
							}

							int val_length = MultiByteToWideChar( CP_UTF8, 0, fi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
							wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
							MultiByteToWideChar( CP_UTF8, 0, fi->c_total_calls, -1, val, val_length );

							fi->total_calls = val;

							fi->state = 0;

							if ( dllrbt_insert( forward_list, ( void * )fi->c_call_from, ( void * )fi ) != DLLRBT_STATUS_OK )
							{
								free_forwardinfo( &fi );
							}
							else if ( is_num( fi->c_call_from ) == 1 )	// See if the value we're adding is a range (has wildcard values in it).
							{
								RangeAdd( &forward_range_list[ length1 - 1 ], fi->c_call_from, length1 );
							}
						}
					}
				}
			}
		}

		GlobalFree( forward_buf );

		CloseHandle( hFile_forward );
	}
}

void save_forward_list()
{
	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\forward.txt\0", 13 );
	base_directory[ base_directory_length + 12 ] = 0;	// Sanity.

	// Open our config file if it exists.
	HANDLE hFile_forward = CreateFile( base_directory, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_forward != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );	// This buffer must be greater than, or equal to 46 bytes.

		node_type *node = dllrbt_get_head( forward_list );
		while ( node != NULL )
		{
			forwardinfo *fi = ( forwardinfo * )node->val;

			if ( fi != NULL && fi->c_call_from != NULL && fi->c_forward_to != NULL && fi->c_total_calls != NULL )
			{
				int phone_number_length1 = lstrlenA( fi->c_call_from );
				int phone_number_length2 = lstrlenA( fi->c_forward_to );
				int total_calls_length = lstrlenA( fi->c_total_calls );

				// See if the next number can fit in the buffer. If it can't, then we dump the buffer.
				if ( pos + phone_number_length1 + phone_number_length2 + total_calls_length + 4 > size )
				{
					// Dump the buffer.
					WriteFile( hFile_forward, write_buf, pos, &write, NULL );
					pos = 0;
				}

				// Ignore numbers that are greater than 15 + 1 digits (bytes).
				if ( phone_number_length1 + phone_number_length2 + total_calls_length + 4 <= 46 )
				{
					// Add to the buffer.
					_memcpy_s( write_buf + pos, size - pos, fi->c_call_from, phone_number_length1 );
					pos += phone_number_length1;
					write_buf[ pos++ ] = ':';
					_memcpy_s( write_buf + pos, size - pos, fi->c_forward_to, phone_number_length2 );
					pos += phone_number_length2;
					write_buf[ pos++ ] = ':';
					_memcpy_s( write_buf + pos, size - pos, fi->c_total_calls, total_calls_length );
					pos += total_calls_length;
					write_buf[ pos++ ] = '\r';
					write_buf[ pos++ ] = '\n';
				}
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile_forward, write_buf, pos, &write, NULL );
		}

		GlobalFree( write_buf );

		CloseHandle( hFile_forward );
	}
}

void save_call_log_history()
{
	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\call_log_history.bin\0", 22 );
	base_directory[ base_directory_length + 21 ] = 0;	// Sanity.

	// Open our config file if it exists.
	HANDLE hFile_call_log = CreateFile( base_directory, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_call_log != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		int item_count = _SendMessageW( g_hWnd_list, LVM_GETITEMCOUNT, 0, 0 );

		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;

		for ( int i = 0; i < item_count; ++i )
		{
			lvi.iItem = i;
			_SendMessageW( g_hWnd_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

			displayinfo *di = ( displayinfo * )lvi.lParam;

			// lstrlen is safe for NULL values.
			int caller_id_length = lstrlenA( di->ci.caller_id ) + 1;
			int phone_number_length1 = lstrlenA( di->ci.call_from ) + 1;
			int phone_number_length2 = lstrlenA( di->ci.call_to ) + 1;
			int phone_number_length3 = lstrlenA( di->ci.forward_to ) + 1;
			int reference_id_length = lstrlenA( di->ci.call_reference_id ) + 1;

			// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
			if ( ( signed )( pos + phone_number_length1 + phone_number_length2 + phone_number_length3 + caller_id_length + reference_id_length + sizeof( LONGLONG ) + ( sizeof( bool ) * 2 ) ) > size )
			{
				// Dump the buffer.
				WriteFile( hFile_call_log, write_buf, pos, &write, NULL );
				pos = 0;
			}

			_memcpy_s( write_buf + pos, size - pos, &di->ci.ignored, sizeof( bool ) );
			pos += sizeof( bool );

			_memcpy_s( write_buf + pos, size - pos, &di->ci.forwarded, sizeof( bool ) );
			pos += sizeof( bool );

			_memcpy_s( write_buf + pos, size - pos, &di->time.QuadPart, sizeof( LONGLONG ) );
			pos += sizeof( LONGLONG );

			_memcpy_s( write_buf + pos, size - pos, di->ci.caller_id, caller_id_length );
			pos += caller_id_length;

			_memcpy_s( write_buf + pos, size - pos, di->ci.call_from, phone_number_length1 );
			pos += phone_number_length1;

			_memcpy_s( write_buf + pos, size - pos, di->ci.call_to, phone_number_length2 );
			pos += phone_number_length2;

			_memcpy_s( write_buf + pos, size - pos, di->ci.forward_to, phone_number_length3 );
			pos += phone_number_length3;

			_memcpy_s( write_buf + pos, size - pos, di->ci.call_reference_id, reference_id_length );
			pos += reference_id_length;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile_call_log, write_buf, pos, &write, NULL );
		}

		GlobalFree( write_buf );

		CloseHandle( hFile_call_log );
	}
}
