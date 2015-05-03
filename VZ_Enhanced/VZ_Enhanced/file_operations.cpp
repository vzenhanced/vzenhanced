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
#include "string_tables.h"

RANGE *ignore_range_list[ 16 ];
RANGE *forward_range_list[ 16 ];

dllrbt_tree *ignore_list = NULL;
bool ignore_list_changed = false;

dllrbt_tree *forward_list = NULL;
bool forward_list_changed = false;

dllrbt_tree *ignore_cid_list = NULL;
bool ignore_cid_list_changed = false;

dllrbt_tree *forward_cid_list = NULL;
bool forward_cid_list_changed = false;

char read_config()
{
	char status = 0;

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\vz_enhanced_settings\0", 22 );
	base_directory[ base_directory_length + 21 ] = 0;	// Sanity.

	HANDLE hFile_cfg = CreateFile( base_directory, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, pos = 0;
		DWORD fz = GetFileSize( hFile_cfg, NULL );

		// Our config file is going to be small. If it's something else, we're not going to read it.
		if ( fz >= 368 && fz < 10240 )
		{
			char *cfg_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * fz + 1 );

			ReadFile( hFile_cfg, cfg_buf, sizeof( char ) * fz, &read, NULL );

			cfg_buf[ fz ] = 0;	// Guarantee a NULL terminated buffer.

			// Read the config. It must be in the order specified below.
			if ( read == fz && _memcmp( cfg_buf, MAGIC_ID_SETTINGS, 4 ) == 0 )
			{
				char *next = cfg_buf + 4;

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
				_memcpy_s( &cfg_column_width10, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column_width11, sizeof( int ), next, sizeof( int ) );
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

				_memcpy_s( &cfg_column5_width1, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column5_width2, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column5_width3, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column5_width4, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column5_width5, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column5_width6, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );

				_memcpy_s( &cfg_column6_width1, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column6_width2, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column6_width3, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column6_width4, sizeof( int ), next, sizeof( int ) );
				next += sizeof( int );
				_memcpy_s( &cfg_column6_width5, sizeof( int ), next, sizeof( int ) );
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
				_memcpy_s( &cfg_column_order10, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column_order11, sizeof( char ), next, sizeof( char ) );
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

				_memcpy_s( &cfg_column5_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column5_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column5_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column5_order4, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column5_order5, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column5_order6, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );

				_memcpy_s( &cfg_column6_order1, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column6_order2, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column6_order3, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column6_order4, sizeof( char ), next, sizeof( char ) );
				next += sizeof( char );
				_memcpy_s( &cfg_column6_order5, sizeof( char ), next, sizeof( char ) );
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

				_memcpy_s( &cfg_enable_message_log, sizeof( bool ), next, sizeof( bool ) );
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
				if ( cfg_popup_time > 300 ) cfg_popup_time = 10;
				if ( cfg_connection_retries > 10 ) cfg_connection_retries = 3;
				if ( cfg_connection_timeout > 300 || ( cfg_connection_timeout > 0 && cfg_connection_timeout < 60 ) ) cfg_connection_timeout = 60;

				CheckColumnOrders( 0, call_log_columns, NUM_COLUMNS1 );
				CheckColumnOrders( 1, contact_list_columns, NUM_COLUMNS2 );
				CheckColumnOrders( 2, forward_list_columns, NUM_COLUMNS3 );
				CheckColumnOrders( 3, ignore_list_columns, NUM_COLUMNS4 );
				CheckColumnOrders( 4, forward_cid_list_columns, NUM_COLUMNS5 );
				CheckColumnOrders( 5, ignore_cid_list_columns, NUM_COLUMNS6 );

				// Revert column widths if they exceed our limits.
				CheckColumnWidths();
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

	return status;
}

char save_config()
{
	char status = 0;

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\vz_enhanced_settings\0", 22 );
	base_directory[ base_directory_length + 21 ] = 0;	// Sanity.

	HANDLE hFile_cfg = CreateFile( base_directory, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		int size = ( sizeof( int ) * 66 ) + ( sizeof( bool ) * 19 ) + ( sizeof( char ) * 75 ) + ( sizeof( unsigned short ) * 2 );// + ( sizeof( wchar_t ) * 96 );
		int pos = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_SETTINGS, sizeof( char ) * 4 );	// Magic identifier for the main program's settings.
		pos += ( sizeof( char ) * 4 );
		
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
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width10, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_width11, sizeof( int ) );
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

		_memcpy_s( write_buf + pos, size - pos, &cfg_column5_width1, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column5_width2, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column5_width3, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column5_width4, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column5_width5, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column5_width6, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column6_width1, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column6_width2, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column6_width3, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column6_width4, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column6_width5, sizeof( int ) );
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
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order10, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column_order11, sizeof( char ) );
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

		_memcpy_s( write_buf + pos, size - pos, &cfg_column5_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column5_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column5_order3, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column5_order4, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column5_order5, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column5_order6, sizeof( char ) );
		pos += sizeof( char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_column6_order1, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column6_order2, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column6_order3, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column6_order4, sizeof( char ) );
		pos += sizeof( char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_column6_order5, sizeof( char ) );
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

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_message_log, sizeof( bool ) );
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
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}


char read_ignore_list( wchar_t *file_path, dllrbt_tree *list )
{
	char status = 0;

	HANDLE hFile_ignore = hFile_ignore = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_ignore != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0;
		bool skip_next_newline = false;

		char magic_identifier[ 4 ];
		ReadFile( hFile_ignore, magic_identifier, sizeof( char ) * 4, &read, NULL );
		if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_IGNORE_PN, 4 ) == 0 )
		{
			DWORD fz = GetFileSize( hFile_ignore, NULL ) - 4;

			char *ignore_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 32768 + 1 ) );	// This buffer must be greater than, or equal to 28 bytes.

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
						p = _StrChrA( s, '\x1e' );

						// If it was the last number, then we're done.
						if ( p == NULL )
						{
							continue;
						}

						// Move to the next value.
						++p;
						s = p;
					}

					// Find the newline token.
					p = _StrChrA( s, '\x1e' );

					// If we found one, copy the number.
					if ( p != NULL || ( total_read >= fz ) )
					{
						char *tp = p;

						if ( p != NULL )
						{
							*p = 0;
							++p;
						}
						else	// Handle an EOF.
						{
							tp = ( ignore_buf + read );
						}

						char *t = _StrChrA( s, '\x1f' );	// Find the ignore count.

						DWORD length1 = 0;
						DWORD length2 = 0;

						if ( t == NULL )
						{
							length1 = ( tp - s );
						}
						else
						{
							length1 = ( t - s );
							++t;
							length2 = ( ( tp - t ) > 10 ? 10 : ( tp - t ) );
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

							if ( dllrbt_insert( list, ( void * )ii->c_phone_number, ( void * )ii ) != DLLRBT_STATUS_OK )
							{
								free_ignoreinfo( &ii );
							}
							else if ( is_num( ii->c_phone_number ) == 1 )	// See if the value we're adding is a range (has wildcard values in it).
							{
								RangeAdd( &ignore_range_list[ length1 - 1 ], ii->c_phone_number, length1 );
							}
						}

						// Move to the next value.
						s = p;
					}
					else if ( total_read < fz )	// Reached the end of what we've read.
					{
						// If we didn't find a token and haven't finished reading the entire file, then offset the file pointer back to the end of the last valid number.
						DWORD offset = ( ( ignore_buf + read ) - s );

						char *t = _StrChrA( s, '\x1f' );	// Find the ignore count.

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
							length2 = ( ( ( ignore_buf + read ) - t ) > 10 ? 10 : ( ( ignore_buf + read ) - t ) );
						}

						// Max recommended number length is 15 + 1.
						// We can only offset back less than what we've read, and no more than 27 digits (bytes). 15 + 1 + 1 + 10
						// This means read, and our buffer size should be greater than 27.
						// Based on the token we're searching for: "\x1e", the buffer NEEDS to be greater or equal to 28.
						if ( offset < read && length1 <= 16 && length2 <= 10 )
						{
							total_read -= offset;
							SetFilePointer( hFile_ignore, total_read, NULL, FILE_BEGIN );
						}
						else	// The length of the next number is too long.
						{
							skip_next_newline = true;
						}
					}
				}
			}

			GlobalFree( ignore_buf );
		}
		else
		{
			status = -2;	// Bad file format.
		}

		CloseHandle( hFile_ignore );
	}
	else
	{
		status = -1;	// Can't open file for reading.
	}

	return status;
}

char read_ignore_cid_list( wchar_t *file_path, dllrbt_tree *list )
{
	char status = 0;

	HANDLE hFile_ignore = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_ignore != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0;
		bool skip_next_newline = false;

		char magic_identifier[ 4 ];
		ReadFile( hFile_ignore, magic_identifier, sizeof( char ) * 4, &read, NULL );
		if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_IGNORE_CNAM, 4 ) == 0 )
		{
			DWORD fz = GetFileSize( hFile_ignore, NULL ) - 4;

			char *ignore_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 32768 + 1 ) );	// This buffer must be greater than, or equal to 29 bytes.

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
						p = _StrChrA( s, '\x1e' );

						// If it was the last number, then we're done.
						if ( p == NULL )
						{
							continue;
						}

						// Move to the next value.
						++p;
						s = p;
					}

					// Find the newline token.
					p = _StrChrA( s, '\x1e' );

					// If we found one, copy the number.
					if ( p != NULL || ( total_read >= fz ) )
					{
						char *tp = p;

						if ( p != NULL )
						{
							*p = 0;
							++p;
						}
						else	// Handle an EOF.
						{
							tp = ( ignore_buf + read );
						}

						char *t = _StrChrA( s, '\x1f' );	// Find the match values.

						if ( t != NULL && ( ( tp - t ) >= 2 ) )
						{
							DWORD length1 = ( t - s );
							DWORD length2 = 0;

							bool match_case = ( t[ 1 ] & 0x02 ? true : false );
							bool match_whole_word = ( t[ 1 ] & 0x01 ? true : false );

							t = _StrChrA( t + 2, '\x1f' );

							if ( t != NULL )
							{
								++t;
								length2 = ( ( tp - t ) > 10 ? 10 : ( tp - t ) );
							}

							// Make sure the number is at most 15 digits.
							if ( ( length1 <= 15 && length1 > 0 ) && ( length2 <= 10 && length2 >= 0 ) )
							{
								ignorecidinfo *icidi = ( ignorecidinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignorecidinfo ) );

								icidi->c_caller_id = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length1 + 1 ) );
								_memcpy_s( icidi->c_caller_id, length1 + 1, s, length1 );
								*( icidi->c_caller_id + length1 ) = 0;	// Sanity

								int val_length = MultiByteToWideChar( CP_UTF8, 0, icidi->c_caller_id, -1, NULL, 0 );	// Include the NULL terminator.
								wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
								MultiByteToWideChar( CP_UTF8, 0, icidi->c_caller_id, -1, val, val_length );

								icidi->caller_id = val;

								if ( length2 == 0 )
								{
									icidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
									*icidi->c_total_calls = '0';
									*( icidi->c_total_calls + 1 ) = 0;	// Sanity

									icidi->count = 0;
								}
								else
								{
									icidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length2 + 1 ) );
									_memcpy_s( icidi->c_total_calls, length2 + 1, t, length2 );
									*( icidi->c_total_calls + length2 ) = 0;	// Sanity

									icidi->count = _strtoul( icidi->c_total_calls, NULL, 10 );
								}

								val_length = MultiByteToWideChar( CP_UTF8, 0, icidi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
								val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
								MultiByteToWideChar( CP_UTF8, 0, icidi->c_total_calls, -1, val, val_length );

								icidi->total_calls = val;

								icidi->match_case = match_case;
								icidi->match_whole_word = match_whole_word;

								if ( icidi->match_case == false )
								{
									icidi->c_match_case = GlobalStrDupA( "No" );
									icidi->w_match_case = GlobalStrDupW( ST_No );
								}
								else
								{
									icidi->c_match_case = GlobalStrDupA( "Yes" );
									icidi->w_match_case = GlobalStrDupW( ST_Yes );
								}

								if ( icidi->match_whole_word == false )
								{
									icidi->c_match_whole_word = GlobalStrDupA( "No" );
									icidi->w_match_whole_word = GlobalStrDupW( ST_No );
								}
								else
								{
									icidi->c_match_whole_word = GlobalStrDupA( "Yes" );
									icidi->w_match_whole_word = GlobalStrDupW( ST_Yes );
								}

								icidi->state = 0;
								icidi->active = false;

								if ( dllrbt_insert( list, ( void * )icidi, ( void * )icidi ) != DLLRBT_STATUS_OK )
								{
									free_ignorecidinfo( &icidi );
								}
							}
						}

						// Move to the next value.
						s = p;
					}
					else if ( total_read < fz )	// Reached the end of what we've read.
					{
						// If we didn't find a token and haven't finished reading the entire file, then offset the file pointer back to the end of the last valid caller ID.
						DWORD offset = ( ( ignore_buf + read ) - s );

						char *t = _StrChrA( s, '\x1f' );	// Find the match values.

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

							s = t + 1;

							t = _StrChrA( s, '\x1f' );	// Find the total count.

							if ( t == NULL )
							{
								length2 = ( ( ignore_buf + read ) - s );
							}
							else
							{
								length2 = ( t - s );
								++t;
								length3 = ( ( ( ignore_buf + read ) - t ) > 10 ? 10 : ( ( ignore_buf + read ) - t ) );
							}
						}

						// Max recommended caller ID length is 15.
						// We can only offset back less than what we've read, and no more than 28 bytes. 15 + 1 + 1 + 1 + 10.
						// This means read, and our buffer size should be greater than 28.
						// Based on the token we're searching for: "\x1e", the buffer NEEDS to be greater or equal to 29.
						if ( offset < read && length1 <= 15 && length2 <= 2 && length3 <= 10 )
						{
							total_read -= offset;
							SetFilePointer( hFile_ignore, total_read, NULL, FILE_BEGIN );
						}
						else	// The length of the next caller ID is too long.
						{
							skip_next_newline = true;
						}
					}
				}
			}

			GlobalFree( ignore_buf );
		}
		else
		{
			status = -2;	// Bad file format.
		}

		CloseHandle( hFile_ignore );
	}
	else
	{
		status = -1;	// Can't open file for reading.
	}

	return status;
}

char save_ignore_list( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_ignore = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_ignore != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );	// This buffer must be greater than, or equal to 28 bytes.

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_IGNORE_PN, sizeof( char ) * 4 );	// Magic identifier for the ignore phone number list.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( ignore_list );
		while ( node != NULL )
		{
			ignoreinfo *ii = ( ignoreinfo * )node->val;

			if ( ii != NULL && ii->c_phone_number != NULL && ii->c_total_calls != NULL )
			{
				int phone_number_length = lstrlenA( ii->c_phone_number );
				int total_calls_length = lstrlenA( ii->c_total_calls );

				// See if the next number can fit in the buffer. If it can't, then we dump the buffer.
				if ( pos + phone_number_length + total_calls_length + 2 > size )
				{
					// Dump the buffer.
					WriteFile( hFile_ignore, write_buf, pos, &write, NULL );
					pos = 0;
				}

				// Ignore numbers that are greater than 15 + 1 digits (bytes).
				if ( phone_number_length + total_calls_length + 2 <= 28 )
				{
					// Add to the buffer.
					_memcpy_s( write_buf + pos, size - pos, ii->c_phone_number, phone_number_length );
					pos += phone_number_length;
					write_buf[ pos++ ] = '\x1f';
					_memcpy_s( write_buf + pos, size - pos, ii->c_total_calls, total_calls_length );
					pos += total_calls_length;
					write_buf[ pos++ ] = '\x1e';
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
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}

char save_ignore_cid_list( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_ignore = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_ignore != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );	// This buffer must be greater than, or equal to 29 bytes.

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_IGNORE_CNAM, sizeof( char ) * 4 );	// Magic identifier for the ignore caller ID name list.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( ignore_cid_list );
		while ( node != NULL )
		{
			ignorecidinfo *icidi = ( ignorecidinfo * )node->val;

			if ( icidi != NULL && icidi->c_caller_id != NULL && icidi->c_total_calls != NULL )
			{
				int caller_id_length = lstrlenA( icidi->c_caller_id );
				int total_calls_length = lstrlenA( icidi->c_total_calls );

				// See if the next number can fit in the buffer. If it can't, then we dump the buffer.
				if ( pos + caller_id_length + total_calls_length + 1 + 3 > size )
				{
					// Dump the buffer.
					WriteFile( hFile_ignore, write_buf, pos, &write, NULL );
					pos = 0;
				}

				// Ignore caller ID names that are greater than 15 digits (bytes).
				if ( caller_id_length + total_calls_length + 1 + 3 <= 29 )
				{
					// Add to the buffer.
					_memcpy_s( write_buf + pos, size - pos, icidi->c_caller_id, caller_id_length );
					pos += caller_id_length;
					write_buf[ pos++ ] = '\x1f';
					write_buf[ pos++ ] = 0x80 | ( icidi->match_case == true ? 0x02 : 0x00 ) | ( icidi->match_whole_word == true ? 0x01 : 0x00 );
					write_buf[ pos++ ] = '\x1f';
					_memcpy_s( write_buf + pos, size - pos, icidi->c_total_calls, total_calls_length );
					pos += total_calls_length;
					write_buf[ pos++ ] = '\x1e';
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
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}

char read_forward_list( wchar_t *file_path, dllrbt_tree *list )
{
	char status = 0;

	HANDLE hFile_forward = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_forward != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0;
		bool skip_next_newline = false;

		char magic_identifier[ 4 ];
		ReadFile( hFile_forward, magic_identifier, sizeof( char ) * 4, &read, NULL );
		if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_FORWARD_PN, 4 ) == 0 )
		{
			DWORD fz = GetFileSize( hFile_forward, NULL ) - 4;

			char *forward_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 32768 + 1 ) );	// This buffer must be greater than, or equal to 45 bytes.

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
						p = _StrChrA( s, '\x1e' );

						// If it was the last number, then we're done.
						if ( p == NULL )
						{
							continue;
						}

						// Move to the next value.
						++p;
						s = p;
					}

					// Find the newline token.
					p = _StrChrA( s, '\x1e' );

					// If we found one, copy the number.
					if ( p != NULL || ( total_read >= fz ) )
					{
						char *tp = p;

						if ( p != NULL )
						{
							*p = 0;
							++p;
						}
						else	// Handle an EOF.
						{
							tp = ( forward_buf + read );
						}

						char *t = _StrChrA( s, '\x1f' );

						// We must have at least a second phone number.
						if ( t != NULL )
						{
							DWORD length1 = ( t - s );
							++t;

							char *t2 = _StrChrA( t, '\x1f' );	// Find the forward count.

							DWORD length2 = 0;
							DWORD length3 = 0;

							if ( t2 == NULL )
							{
								length2 = ( tp - t );
							}
							else
							{
								length2 = ( t2 - t );
								++t2;
								length3 = ( ( tp - t2 ) > 10 ? 10 : ( tp - t2 ) );
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

								if ( dllrbt_insert( list, ( void * )fi->c_call_from, ( void * )fi ) != DLLRBT_STATUS_OK )
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
						s = p;
					}
					else if ( total_read < fz )	// Reached the end of what we've read.
					{
						// If we didn't find a token and haven't finished reading the entire file, then offset the file pointer back to the end of the last valid number.
						DWORD offset = ( ( forward_buf + read ) - s );

						char *t = _StrChrA( s, '\x1f' );

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

							s = t + 1;

							t = _StrChrA( s, '\x1f' );	// Find the forward count.

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
						// Based on the token we're searching for: "\x1e", the buffer NEEDS to be greater or equal to 45.
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
				}
			}

			GlobalFree( forward_buf );
		}
		else
		{
			status = -2;	// Bad file format.
		}

		CloseHandle( hFile_forward );
	}
	else
	{
		status = -1;	// Can't open file for reading.
	}

	return status;
}

char read_forward_cid_list( wchar_t *file_path, dllrbt_tree *list )
{
	char status = 0;

	HANDLE hFile_forward = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_forward != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0;
		bool skip_next_newline = false;

		char magic_identifier[ 4 ];
		ReadFile( hFile_forward, magic_identifier, sizeof( char ) * 4, &read, NULL );
		if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_FORWARD_CNAM, 4 ) == 0 )
		{
			DWORD fz = GetFileSize( hFile_forward, NULL ) - 4;

			char *forward_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 32768 + 1 ) );	// This buffer must be greater than, or equal to 46 bytes.

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
						p = _StrChrA( s, '\x1e' );

						// If it was the last number, then we're done.
						if ( p == NULL )
						{
							continue;
						}

						// Move to the next value.
						++p;
						s = p;
					}

					// Find the newline token.
					p = _StrChrA( s, '\x1e' );

					// If we found one, copy the number.
					if ( p != NULL || ( total_read >= fz ) )
					{
						char *tp = p;

						if ( p != NULL )
						{
							*p = 0;
							++p;
						}
						else	// Handle an EOF.
						{
							tp = ( forward_buf + read );
						}

						char *t = _StrChrA( s, '\x1f' );	// Find the forward to phone number.

						// We must have a forward phone number.
						if ( t != NULL )
						{
							DWORD length1 = ( t - s );
							++t;

							char *t2 = _StrChrA( t, '\x1f' );	// Find the match values.

							if ( t2 != NULL && ( ( tp - t2 ) >= 2 ) )
							{
								DWORD length2 = ( t2 - t );
								DWORD length3 = 0;

								bool match_case = ( t2[ 1 ] & 0x02 ? true : false );
								bool match_whole_word = ( t2[ 1 ] & 0x01 ? true : false );

								t2 = _StrChrA( t2 + 2, '\x1f' );

								if ( t2 != NULL )
								{
									++t2;
									length3 = ( ( tp - t2 ) > 10 ? 10 : ( tp - t2 ) );
								}

								// Make sure the caller ID is at most 15 digits and the number is at most 16 digits.
								if ( ( length1 <= 15 && length1 > 0 ) && ( length2 <= 16 && length2 > 0 ) && ( length3 <= 10 && length3 >= 0 ) )
								{
									forwardcidinfo *fcidi = ( forwardcidinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardcidinfo ) );

									fcidi->c_caller_id = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length1 + 1 ) );
									_memcpy_s( fcidi->c_caller_id, length1 + 1, s, length1 );
									*( fcidi->c_caller_id + length1 ) = 0;	// Sanity

									int val_length = MultiByteToWideChar( CP_UTF8, 0, fcidi->c_caller_id, -1, NULL, 0 );	// Include the NULL terminator.
									wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
									MultiByteToWideChar( CP_UTF8, 0, fcidi->c_caller_id, -1, val, val_length );

									fcidi->caller_id = val;

									fcidi->c_forward_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length2 + 1 ) );
									_memcpy_s( fcidi->c_forward_to, length2 + 1, t, length2 );
									*( fcidi->c_forward_to + length2 ) = 0;	// Sanity

									fcidi->forward_to = FormatPhoneNumber( fcidi->c_forward_to );

									if ( length3 == 0 )
									{
										fcidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
										*fcidi->c_total_calls = '0';
										*( fcidi->c_total_calls + 1 ) = 0;	// Sanity

										fcidi->count = 0;
									}
									else
									{
										fcidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( length3 + 1 ) );
										_memcpy_s( fcidi->c_total_calls, length3 + 1, t2, length3 );
										*( fcidi->c_total_calls + length3 ) = 0;	// Sanity

										fcidi->count = _strtoul( fcidi->c_total_calls, NULL, 10 );
									}

									val_length = MultiByteToWideChar( CP_UTF8, 0, fcidi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
									val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
									MultiByteToWideChar( CP_UTF8, 0, fcidi->c_total_calls, -1, val, val_length );

									fcidi->total_calls = val;

									fcidi->match_case = match_case;
									fcidi->match_whole_word = match_whole_word;

									if ( fcidi->match_case == false )
									{
										fcidi->c_match_case = GlobalStrDupA( "No" );
										fcidi->w_match_case = GlobalStrDupW( ST_No );
									}
									else
									{
										fcidi->c_match_case = GlobalStrDupA( "Yes" );
										fcidi->w_match_case = GlobalStrDupW( ST_Yes );
									}

									if ( fcidi->match_whole_word == false )
									{
										fcidi->c_match_whole_word = GlobalStrDupA( "No" );
										fcidi->w_match_whole_word = GlobalStrDupW( ST_No );
									}
									else
									{
										fcidi->c_match_whole_word = GlobalStrDupA( "Yes" );
										fcidi->w_match_whole_word = GlobalStrDupW( ST_Yes );
									}

									fcidi->state = 0;
									fcidi->active = false;

									if ( dllrbt_insert( list, ( void * )fcidi, ( void * )fcidi ) != DLLRBT_STATUS_OK )
									{
										free_forwardcidinfo( &fcidi );
									}
								}
							}
						}

						// Move to the next value.
						s = p;
					}
					else if ( total_read < fz )	// Reached the end of what we've read.
					{
						// If we didn't find a token and haven't finished reading the entire file, then offset the file pointer back to the end of the last valid number.
						DWORD offset = ( ( forward_buf + read ) - s );

						char *t = _StrChrA( s, '\x1f' );	// Find the forward to phone number.

						DWORD length1 = 0;
						DWORD length2 = 0;
						DWORD length3 = 0;
						DWORD length4 = 0;

						if ( t == NULL )
						{
							length1 = offset;
						}
						else
						{
							length1 = ( t - s );

							s = t + 1;

							t = _StrChrA( s, '\x1f' );	// Find the match values.

							if ( t == NULL )
							{
								length2 = ( ( forward_buf + read ) - s );
							}
							else
							{
								length2 = ( t - s );

								s = t + 1;

								t = _StrChrA( s, '\x1f' );	// Find the total count.

								if ( t == NULL )
								{
									length3 = ( ( forward_buf + read ) - s );
								}
								else
								{
									length3 = ( t - s );
									++t;
									length4 = ( ( ( forward_buf + read ) - t ) > 10 ? 10 : ( ( forward_buf + read ) - t ) );
								}
							}
						}

						// Max recommended number length is 15 + 1.
						// We can only offset back less than what we've read, and no more than 45 bytes. 15 + 1 + 15 + 1 + 1 + 1 + 1 + 10.
						// This means read, and our buffer size should be greater than 45.
						// Based on the token we're searching for: "\x1e", the buffer NEEDS to be greater or equal to 46.
						if ( offset < read && length1 <= 15 && length2 <= 16 && length3 <= 2 && length4 <= 10 )
						{
							total_read -= offset;
							SetFilePointer( hFile_forward, total_read, NULL, FILE_BEGIN );
						}
						else	// The length of the next number is too long.
						{
							skip_next_newline = true;
						}
					}
				}
			}

			GlobalFree( forward_buf );
		}
		else
		{
			status = -2;	// Bad file format.
		}

		CloseHandle( hFile_forward );
	}
	else
	{
		status = -1;	// Can't open file for reading.
	}

	return status;
}

char save_forward_list( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_forward = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_forward != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );	// This buffer must be greater than, or equal to 45 bytes.

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_FORWARD_PN, sizeof( char ) * 4 );	// Magic identifier for the forward phone number list.
		pos += ( sizeof( char ) * 4 );

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
				if ( pos + phone_number_length1 + phone_number_length2 + total_calls_length + 3 > size )
				{
					// Dump the buffer.
					WriteFile( hFile_forward, write_buf, pos, &write, NULL );
					pos = 0;
				}

				// Ignore numbers that are greater than 15 + 1 digits (bytes).
				if ( phone_number_length1 + phone_number_length2 + total_calls_length + 3 <= 45 )
				{
					// Add to the buffer.
					_memcpy_s( write_buf + pos, size - pos, fi->c_call_from, phone_number_length1 );
					pos += phone_number_length1;
					write_buf[ pos++ ] = '\x1f';
					_memcpy_s( write_buf + pos, size - pos, fi->c_forward_to, phone_number_length2 );
					pos += phone_number_length2;
					write_buf[ pos++ ] = '\x1f';
					_memcpy_s( write_buf + pos, size - pos, fi->c_total_calls, total_calls_length );
					pos += total_calls_length;
					write_buf[ pos++ ] = '\x1e';
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
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}

char save_forward_cid_list( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_forward = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_forward != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );	// This buffer must be greater than, or equal to 46 bytes.

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_FORWARD_CNAM, sizeof( char ) * 4 );	// Magic identifier for the forward caller ID name list.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( forward_cid_list );
		while ( node != NULL )
		{
			forwardcidinfo *fcidi = ( forwardcidinfo * )node->val;

			if ( fcidi != NULL && fcidi->c_caller_id != NULL && fcidi->c_forward_to != NULL && fcidi->c_total_calls != NULL )
			{
				int caller_id_length = lstrlenA( fcidi->c_caller_id );
				int phone_number_length = lstrlenA( fcidi->c_forward_to );
				int total_calls_length = lstrlenA( fcidi->c_total_calls );

				// See if the next number can fit in the buffer. If it can't, then we dump the buffer.
				if ( pos + caller_id_length + phone_number_length + total_calls_length + 1 + 4 > size )
				{
					// Dump the buffer.
					WriteFile( hFile_forward, write_buf, pos, &write, NULL );
					pos = 0;
				}

				// Ignore caller ID names that are greater than 15 and numbers that are greater than 15 + 1 digits (bytes).
				if ( caller_id_length + phone_number_length + total_calls_length + 1 + 4 <= 46 )
				{
					// Add to the buffer.
					_memcpy_s( write_buf + pos, size - pos, fcidi->c_caller_id, caller_id_length );
					pos += caller_id_length;
					write_buf[ pos++ ] = '\x1f';
					_memcpy_s( write_buf + pos, size - pos, fcidi->c_forward_to, phone_number_length );
					pos += phone_number_length;
					write_buf[ pos++ ] = '\x1f';
					write_buf[ pos++ ] = 0x80 | ( fcidi->match_case == true ? 0x02 : 0x00 ) | ( fcidi->match_whole_word == true ? 0x01 : 0x00 );
					write_buf[ pos++ ] = '\x1f';
					_memcpy_s( write_buf + pos, size - pos, fcidi->c_total_calls, total_calls_length );
					pos += total_calls_length;
					write_buf[ pos++ ] = '\x1e';
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
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}

char read_call_log_history( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_read = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_read != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		bool ignored = false;
		bool forwarded = false;
		LONGLONG time = 0;
		char *caller_id = NULL;
		char *call_from = NULL;
		char *call_to = NULL;
		char *forward_to = NULL;
		char *call_reference_id = NULL;

		char range_number[ 32 ];

		char magic_identifier[ 4 ];
		ReadFile( hFile_read, magic_identifier, sizeof( char ) * 4, &read, NULL );
		if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_CALL_LOG, 4 ) == 0 )
		{
			DWORD fz = GetFileSize( hFile_read, NULL ) - 4;

			char *history_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 1 ) );	// 512 KB buffer.

			while ( total_read < fz )
			{
				// Stop processing and exit the function.
				/*if ( kill_worker_thread_flag == true )
				{
					break;
				}*/

				ReadFile( hFile_read, history_buf, sizeof( char ) * 524288, &read, NULL );

				history_buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

				// Make sure that we have at least part of the entry. 5 = 5 NULL strings. This is the minimum size an entry could be.
				if ( read < ( ( sizeof( bool ) * 2 ) + sizeof( LONGLONG ) + 5 ) )
				{
					break;
				}

				total_read += read;

				// Prevent an infinite loop if a really really long entry causes us to jump back to the same point in the file.
				// If it's larger than our buffer, then the file is probably invalid/corrupt.
				if ( total_read == last_total )
				{
					break;
				}

				last_total = total_read;

				p = history_buf;
				offset = last_entry = 0;

				while ( offset < read )
				{
					// Stop processing and exit the function.
					/*if ( kill_worker_thread_flag == true )
					{
						break;
					}*/

					caller_id = NULL;
					call_from = NULL;
					call_to = NULL;
					forward_to = NULL;
					call_reference_id = NULL;

					// Ignored state
					offset += sizeof( bool );
					if ( offset >= read ) { goto CLEANUP; }
					_memcpy_s( &ignored, sizeof( bool ), p, sizeof( bool ) );
					p += sizeof( bool );

					// Forwarded state
					offset += sizeof( bool );
					if ( offset >= read ) { goto CLEANUP; }
					_memcpy_s( &forwarded, sizeof( bool ), p, sizeof( bool ) );
					p += sizeof( bool );

					// Call time
					offset += sizeof( LONGLONG );
					if ( offset >= read ) { goto CLEANUP; }
					_memcpy_s( &time, sizeof( LONGLONG ), p, sizeof( LONGLONG ) );
					p += sizeof( LONGLONG );

					// Caller ID
					int string_length = lstrlenA( p ) + 1;

					offset += string_length;
					if ( offset >= read ) { goto CLEANUP; }

					caller_id = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
					_memcpy_s( caller_id, string_length, p, string_length );
					*( caller_id + ( string_length - 1 ) ) = 0;	// Sanity

					p += string_length;

					// Incoming number
					string_length = lstrlenA( p );

					offset += ( string_length + 1 );
					if ( offset >= read ) { goto CLEANUP; }

					int call_from_length = min( string_length, 16 );
					int adjusted_size = call_from_length + 1;

					call_from = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * adjusted_size );
					_memcpy_s( call_from, adjusted_size, p, adjusted_size );
					*( call_from + ( adjusted_size - 1 ) ) = 0;	// Sanity

					p += ( string_length + 1 );

					// The number that was called (our number)
					string_length = lstrlenA( p );

					offset += ( string_length + 1 );
					if ( offset >= read ) { goto CLEANUP; }

					adjusted_size = min( string_length, 16 ) + 1;

					call_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * adjusted_size );
					_memcpy_s( call_to, adjusted_size, p, adjusted_size );
					*( call_to + ( adjusted_size - 1 ) ) = 0;	// Sanity

					p += ( string_length + 1 );

					// The number that the incoming call was forwarded to
					string_length = lstrlenA( p );

					offset += ( string_length + 1 );
					if ( offset >= read ) { goto CLEANUP; }

					adjusted_size = min( string_length, 16 ) + 1;

					forward_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * adjusted_size );
					_memcpy_s( forward_to, adjusted_size, p, adjusted_size );
					*( forward_to + ( adjusted_size - 1 ) ) = 0;	// Sanity

					p += ( string_length + 1 );

					// Server reference ID
					string_length = lstrlenA( p ) + 1;

					offset += string_length;
					if ( offset > read ) { goto CLEANUP; }	// Offset must equal read for the last string in the file. It's truncated/corrupt if it doesn't.

					call_reference_id = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
					_memcpy_s( call_reference_id, string_length, p, string_length );
					*( call_reference_id + ( string_length - 1 ) ) = 0;	// Sanity

					p += string_length;

					last_entry = offset;	// This value is the ending offset of the last valid entry.

					displayinfo *di = ( displayinfo * )GlobalAlloc( GMEM_FIXED, sizeof( displayinfo ) );

					di->ci.call_to = call_to;
					di->ci.call_from = call_from;
					di->ci.call_reference_id = call_reference_id;
					di->ci.caller_id = caller_id;
					di->ci.forward_to = forward_to;
					di->ci.ignored = ignored;
					di->ci.forwarded = forwarded;
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
					di->time.QuadPart = time;
					di->process_incoming = false;
					di->ignore_phone_number = false;
					di->forward_phone_number = false;
					di->ignore_cid_match_count = 0;
					di->forward_cid_match_count = 0;

					// Create the node to insert into a linked list.
					DoublyLinkedList *di_node = DLL_CreateNode( ( void * )di );

					// See if our tree has the phone number to add the node to.
					DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
					if ( dll == NULL )
					{
						// If no phone number exits, insert the node into the tree.
						if ( dllrbt_insert( call_log, ( void * )di->ci.call_from, ( void * )di_node ) != DLLRBT_STATUS_OK )
						{
							GlobalFree( di_node );	// This shouldn't happen.
						}
					}
					else	// If a phone number exits, insert the node into the linked list.
					{
						DLL_AddNode( &dll, di_node, -1 );	// Insert at the end of the doubly linked list.
					}

					// Search the ignore_list for a match.
					ignoreinfo *ii = ( ignoreinfo * )dllrbt_find( ignore_list, ( void * )di->ci.call_from, true );

					// Try searching the range list.
					if ( ii == NULL )
					{
						_memzero( range_number, 32 );

						int range_index = call_from_length;
						range_index = ( range_index > 0 ? range_index - 1 : 0 );

						if ( RangeSearch( &ignore_range_list[ range_index ], di->ci.call_from, range_number ) == true )
						{
							ii = ( ignoreinfo * )dllrbt_find( ignore_list, ( void * )range_number, true );
						}
					}

					if ( ii != NULL )
					{
						di->ignore_phone_number = true;
					}

					// Search for the first ignore caller ID list match. di->ignore_cid_match_count will be updated here for all keyword matches.
					find_ignore_caller_id_name_match( di );

					// Search the forward list for a match.
					forwardinfo *fi = ( forwardinfo * )dllrbt_find( forward_list, ( void * )di->ci.call_from, true );

					// Try searching the range list.
					if ( fi == NULL )
					{
						_memzero( range_number, 32 );

						int range_index = call_from_length;
						range_index = ( range_index > 0 ? range_index - 1 : 0 );

						if ( RangeSearch( &forward_range_list[ range_index ], di->ci.call_from, range_number ) == true )
						{
							fi = ( forwardinfo * )dllrbt_find( forward_list, ( void * )range_number, true );
						}
					}

					if ( fi != NULL )
					{
						di->forward_phone_number = true;
					}

					// Search for the first forward caller ID list match. di->forward_cid_match_count will be updated here for all keyword matches.
					find_forward_caller_id_name_match( di );

					_SendNotifyMessageW( g_hWnd_main, WM_PROPAGATE, MAKEWPARAM( CW_MODIFY, 1 ), ( LPARAM )di );	// Add entry to listview and don't show popup.

					continue;

	CLEANUP:
					GlobalFree( caller_id );
					GlobalFree( call_from );
					GlobalFree( call_to );
					GlobalFree( forward_to );
					GlobalFree( call_reference_id );

					// Go back to the last valid entry.
					if ( total_read < fz )
					{
						total_read -= ( read - last_entry );
						SetFilePointer( hFile_read, total_read, NULL, FILE_BEGIN );
					}

					break;
				}
			}

			GlobalFree( history_buf );
		}
		else
		{
			status = -2;	// Bad file format.
		}

		CloseHandle( hFile_read );	
	}
	else
	{
		status = -1;	// Can't open file for reading.
	}

	return status;
}


char save_call_log_history( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_call_log = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_call_log != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_CALL_LOG, sizeof( char ) * 4 );	// Magic identifier for the call log history.
		pos += ( sizeof( char ) * 4 );

		int item_count = _SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );

		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;

		for ( int i = 0; i < item_count; ++i )
		{
			lvi.iItem = i;
			_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

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
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}

char save_call_log_csv_file( wchar_t *file_path )
{
	char status = 0;

	HANDLE hFile_call_log = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_call_log != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;
		char unix_timestamp[ 21 ];
		_memzero( unix_timestamp, 21 );

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		// Write the UTF-8 BOM and CSV column titles.
		WriteFile( hFile_call_log, "\xEF\xBB\xBF\"Caller ID Name\",\"Phone Number\",\"Date and Time\",\"Unix Timestamp\",\"Ignore Phone Number\",\"Forward Phone Number\",\"Ignore Caller ID Name\",\"Forward Caller ID Name\",\"Forwarded to\",\"Sent to\"", 186, &write, NULL );

		node_type *node = dllrbt_get_head( call_log );
		while ( node != NULL )
		{
			// Stop processing and exit the function.
			/*if ( kill_worker_thread_flag == true )
			{
				break;
			}*/

			DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
			while ( di_node != NULL )
			{
				// Stop processing and exit the function.
				/*if ( kill_worker_thread_flag == true )
				{
					break;
				}*/

				displayinfo *di = ( displayinfo * )di_node->data;

				if ( di != NULL )
				{
					// lstrlen is safe for NULL values.
					int phone_number_length1 = lstrlenA( di->ci.call_from );
					int phone_number_length2 = lstrlenA( di->ci.forward_to );
					int phone_number_length3 = lstrlenA( di->ci.call_to );

					wchar_t *w_ignore = ( di->ignore_phone_number == true ? ST_Yes : ST_No );
					wchar_t *w_forward = ( di->forward_phone_number == true ? ST_Yes : ST_No );
					wchar_t *w_ignore_cid = ( di->ignore_cid_match_count > 0 ? ST_Yes : ST_No );
					wchar_t *w_forward_cid = ( di->forward_cid_match_count > 0 ? ST_Yes : ST_No );

					int ignore_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore, -1, NULL, 0, NULL, NULL );
					char *utf8_ignore = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ignore_length ); // Size includes the null character.
					ignore_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore, -1, utf8_ignore, ignore_length, NULL, NULL ) - 1;

					int forward_length = WideCharToMultiByte( CP_UTF8, 0, w_forward, -1, NULL, 0, NULL, NULL );
					char *utf8_forward = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * forward_length ); // Size includes the null character.
					forward_length = WideCharToMultiByte( CP_UTF8, 0, w_forward, -1, utf8_forward, forward_length, NULL, NULL ) - 1;

					int ignore_cid_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore_cid, -1, NULL, 0, NULL, NULL );
					char *utf8_ignore_cid = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ignore_cid_length ); // Size includes the null character.
					ignore_cid_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore_cid, -1, utf8_ignore_cid, ignore_cid_length, NULL, NULL ) - 1;

					int forward_cid_length = WideCharToMultiByte( CP_UTF8, 0, w_forward_cid, -1, NULL, 0, NULL, NULL );
					char *utf8_forward_cid = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * forward_cid_length ); // Size includes the null character.
					forward_cid_length = WideCharToMultiByte( CP_UTF8, 0, w_forward_cid, -1, utf8_forward_cid, forward_cid_length, NULL, NULL ) - 1;

					char *escaped_caller_id = escape_csv( di->ci.caller_id );

					int caller_id_length = lstrlenA( ( escaped_caller_id != NULL ? escaped_caller_id : di->ci.caller_id ) );

					int time_length = WideCharToMultiByte( CP_UTF8, 0, di->w_time, -1, NULL, 0, NULL, NULL );
					char *utf8_time = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * time_length ); // Size includes the null character.
					time_length = WideCharToMultiByte( CP_UTF8, 0, di->w_time, -1, utf8_time, time_length, NULL, NULL ) - 1;

					// Convert the time into a 32bit Unix timestamp.
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

					int timestamp_length = __snprintf( unix_timestamp, 21, "%llu", date.QuadPart );

					// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
					if ( pos + phone_number_length1 + phone_number_length2 + phone_number_length3 + caller_id_length + time_length + timestamp_length + ignore_length + forward_length + ignore_cid_length + forward_cid_length + 15 > size )
					{
						// Dump the buffer.
						WriteFile( hFile_call_log, write_buf, pos, &write, NULL );
						pos = 0;
					}

					// Add to the buffer.
					write_buf[ pos++ ] = '\r';
					write_buf[ pos++ ] = '\n';

					write_buf[ pos++ ] = '\"';
					_memcpy_s( write_buf + pos, size - pos, ( escaped_caller_id != NULL ? escaped_caller_id : di->ci.caller_id ), caller_id_length );
					pos += caller_id_length;
					write_buf[ pos++ ] = '\"';
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, di->ci.call_from, phone_number_length1 );
					pos += phone_number_length1;
					write_buf[ pos++ ] = ',';

					write_buf[ pos++ ] = '\"';
					_memcpy_s( write_buf + pos, size - pos, utf8_time, time_length );
					pos += time_length;
					write_buf[ pos++ ] = '\"';
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, unix_timestamp, timestamp_length );
					pos += timestamp_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_ignore, ignore_length );
					pos += ignore_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_forward, forward_length );
					pos += forward_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_ignore_cid, ignore_cid_length );
					pos += ignore_cid_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_forward_cid, forward_cid_length );
					pos += forward_cid_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, di->ci.forward_to, phone_number_length2 );
					pos += phone_number_length2;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, di->ci.call_to, phone_number_length3 );
					pos += phone_number_length3;

					GlobalFree( utf8_forward );
					GlobalFree( utf8_ignore );
					GlobalFree( utf8_forward_cid );
					GlobalFree( utf8_ignore_cid );
					GlobalFree( utf8_time );
					GlobalFree( escaped_caller_id );
				}

				di_node = di_node->next;
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile_call_log, write_buf, pos, &write, NULL );
		}

		GlobalFree( write_buf );

		CloseHandle( hFile_call_log );
	}
	else
	{
		status = -1;	// Can't open file for writing.
	}

	return status;
}
