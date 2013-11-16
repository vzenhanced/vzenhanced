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
#include "menus.h"
#include "connection.h"
#include "utilities.h"
#include "string_tables.h"
#include "file_operations.h"
#include "lite_gdi32.h"

#include "web_server.h"

#define BTN_OK			1000
#define BTN_REMEMBER	1001

HWND g_hWnd_username = NULL;
HWND g_hWnd_password = NULL;
HWND g_hWnd_remember = NULL;
HWND g_hWnd_btn_login = NULL;
HWND g_hWnd_static1 = NULL;
HWND g_hWnd_static2 = NULL;

HCURSOR wait_cursor = NULL;			// Temporary cursor while processing entries.

bool silent_startup = false;

LRESULT CALLBACK LoginWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HWND hWnd_static = _CreateWindowW( WC_STATIC, NULL, SS_GRAYFRAME | WS_CHILD | WS_VISIBLE, 10, 10, rc.right - 20, 115, hWnd, NULL, NULL, NULL );

			g_hWnd_static1 = _CreateWindowW( WC_STATIC, ST_Username_, WS_CHILD | WS_VISIBLE, 20, 20, ( rc.right - rc.left ) - 40, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_username, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 35, ( rc.right - rc.left ) - 40, 20, hWnd, NULL, NULL, NULL );

			g_hWnd_static2 = _CreateWindowW( WC_STATIC, ST_Password_, WS_CHILD | WS_VISIBLE, 20, 60, ( rc.right - rc.left ) - 40, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_password, ES_AUTOHSCROLL | ES_PASSWORD | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 75, ( rc.right - rc.left ) - 40, 20, hWnd, NULL, NULL, NULL );

			g_hWnd_remember = _CreateWindowW( WC_BUTTON, ST_Remember_login, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 100, ( rc.right - rc.left ) - 40, 20, hWnd, ( HMENU )BTN_REMEMBER, NULL, NULL );

			g_hWnd_btn_login = _CreateWindowW( WC_BUTTON, ST_Log_In, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, ( rc.right - rc.left - ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) ) / 2, rc.bottom - 32, _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ), 23, hWnd, ( HMENU )BTN_OK, NULL, NULL );

			_SendMessageW( g_hWnd_username, EM_LIMITTEXT, 254, 0 );
			_SendMessageW( g_hWnd_password, EM_LIMITTEXT, 254, 0 );
			_SendMessageW( g_hWnd_remember, BM_SETCHECK, ( cfg_remember_login == true ? BST_CHECKED : BST_UNCHECKED ), 0 );

			// Make pretty font.
			_SendMessageW( g_hWnd_static1, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static2, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_username, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_password, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_remember, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_login, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SetFocus( g_hWnd_username );

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetStockObject( WHITE_BRUSH ) );
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case IDOK:
				case BTN_OK:
				{
					// Cancel the login procedure.
					if ( login_state != LOGGED_OUT )
					{
						_EnableWindow( g_hWnd_btn_login, FALSE );

						login_state = LOGGED_OUT;
						if ( main_con.ssl_socket != NULL )
						{
							_shutdown( main_con.ssl_socket->s, SD_BOTH );
						}

						break;
					}

					unsigned int username_length = _SendMessageW( g_hWnd_username, WM_GETTEXTLENGTH, 0, 0 );
					unsigned int password_length = _SendMessageW( g_hWnd_password, WM_GETTEXTLENGTH, 0, 0 );

					if ( username_length == 0 && password_length == 0 )
					{
						_MessageBoxW( hWnd, ST_must_enter_un_and_pw, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
						break;
					}
					else if ( username_length == 0 )
					{
						_MessageBoxW( hWnd, ST_must_enter_un, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
						break;
					}
					else if ( password_length == 0 )
					{
						_MessageBoxW( hWnd, ST_must_enter_pw, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
						break;
					}

					bool changed = false;	// Note any input that's changed.
					bool save_info = ( _SendMessageW( g_hWnd_remember, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					if ( cfg_remember_login != save_info )
					{
						if ( _MessageBoxW( hWnd, ST_PROMPT_auto_log_in, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
						{
							cfg_connection_auto_login = true;

							if ( g_hWnd_options != NULL )
							{
								_SendMessageW( g_hWnd_options, WM_PROPAGATE, 2, 0 );	// Put a check in the options box.
							}
						}

						if ( g_hWnd_options != NULL )
						{
							_SendMessageW( g_hWnd_options, WM_PROPAGATE, 1, 0 );	// Enable the options box.
						}

						changed = true;
					}

					wchar_t *username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( username_length + 1 ) );
					_SendMessageW( g_hWnd_username, WM_GETTEXT, username_length + 1, ( LPARAM )username );

					wchar_t *password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( password_length + 1 ) );
					_SendMessageW( g_hWnd_password, WM_GETTEXT, password_length + 1, ( LPARAM )password );

					int cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, username, -1, NULL, 0, NULL, NULL );
					char *utf8_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
					cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, username, -1, utf8_username, cfg_val_length, NULL, NULL );

					cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, password, -1, NULL, 0, NULL, NULL );
					char *utf8_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
					cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, password, -1, utf8_password, cfg_val_length, NULL, NULL );

					if ( cfg_username == NULL )
					{
						cfg_username = username;	// Temporary username. This will be freed when the program exits.
					}
					else if ( cfg_username != NULL && lstrcmpW( cfg_username, username ) != 0 )	// If the saved or temporary username does not match the input
					{
						// Then free the saved or temporary username and set it to the input.
						GlobalFree( cfg_username );
						cfg_username = username;

						// Set changed only if we want to save the input.
						if ( save_info == true )
						{
							changed = true;
						}
					}
					else	// If the saved or temporary username matches the input.
					{
						// Then free the input. We'll continue using the saved or temporary username.
						GlobalFree( username );
					}

					if ( cfg_password == NULL )
					{
						cfg_password = password;	// Temporary password. This will be freed when the program exits, or if the user logs off.
					}
					else if ( cfg_password != NULL && lstrcmpW( cfg_password, password ) != 0 )	// If the saved or temporary password does not match the input.
					{
						// Then free the saved or temporary password and set it to the input.
						GlobalFree( cfg_password );
						cfg_password = password;

						// Set changed only if we want to save the input.
						if ( save_info == true )
						{
							changed = true;
						}
					}
					else	// If the saved or temporary password matches the input.
					{
						// Then free the input. We'll continue using the saved or temporary password.
						GlobalFree( password );
					}

					// Only save if something has changed, and the check box is checked.
					if ( changed == true && save_info == true )
					{
						cfg_remember_login = true;
						save_config();	// Save our settings.
					}

					// Freed in the connection thread;
					LOGIN_SETTINGS *ls = ( LOGIN_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( LOGIN_SETTINGS ) );
					ls->username = utf8_username;
					ls->password = utf8_password;

					CloseHandle( ( HANDLE )_CreateThread( NULL, 0, Connection, ( void * )ls, 0, NULL ) );
				}
				break;

				case BTN_REMEMBER:
				{
					if ( HIWORD( wParam ) == BN_CLICKED )
					{
						// If we're now unchecked and our settings were set to remember, then prompt to remove settings.
						if ( _SendMessageW( g_hWnd_remember, BM_GETCHECK, 0, 0 ) == BST_UNCHECKED && cfg_remember_login == true )
						{
							if ( _MessageBoxW( hWnd, ST_PROMPT_delete_login_info, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
							{
								GlobalFree( cfg_username );
								cfg_username = NULL;

								GlobalFree( cfg_password );
								cfg_password = NULL;

								cfg_remember_login = false;
								cfg_connection_auto_login = false;

								// Clear the login information.
								_SendMessageW( g_hWnd_username, WM_SETTEXT, 0, 0 );
								_SendMessageW( g_hWnd_password, WM_SETTEXT, 0, 0 );

								if ( g_hWnd_options != NULL )
								{
									_SendMessageW( g_hWnd_options, WM_PROPAGATE, 0, 0 );	// Disable and uncheck the options box.
								}

								save_config();
							}
							else
							{
								_SendMessageW( g_hWnd_remember, BM_SETCHECK, BST_CHECKED, 0 );
							}
						}
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_ACTIVATE:
		{
			// 0 = inactive, > 0 = active
			g_hWnd_active = ( wParam == 0 ? NULL : hWnd );

            return FALSE;
		}
		break;

		case WM_CHANGE_CURSOR:
		{
			// SetCursor must be called from the window thread.
			if ( wParam == TRUE )
			{
				wait_cursor = _LoadCursorW( NULL, IDC_APPSTARTING );	// Arrow + hourglass.
				_SetCursor( wait_cursor );
			}
			else
			{
				_SetCursor( _LoadCursorW( NULL, IDC_ARROW ) );	// Default arrow.
				wait_cursor = NULL;
			}
		}
		break;

		case WM_SETCURSOR:
		{
			if ( wait_cursor != NULL )
			{
				_SetCursor( wait_cursor );	// Keep setting our cursor if it reverts back to the default.
				return TRUE;
			}

			_DefWindowProcW( hWnd, msg, wParam, lParam );
			return FALSE;
		}
		break;

		case WM_ALERT:
		{
			if ( wParam == 0 )	// Const wchar_t string.
			{
				_MessageBoxW( hWnd, ( LPCWSTR )lParam, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
			}
			else if ( wParam == 1 )	// Pointer to char string. Must be freed.
			{
				_MessageBoxA( hWnd, ( LPCSTR )lParam, PROGRAM_CAPTION_A, MB_APPLMODAL | MB_ICONWARNING );

				GlobalFree( ( char * )lParam );
			}
		}
		break;

		case WM_PROPAGATE:
		{
			if ( wParam == AUTO_LOGIN )
			{
				// It should be assumed that we have a saved username and password in order to auto log in.
				if ( cfg_username != NULL && cfg_password != NULL )
				{
					if ( lParam == TRUE )
					{
						silent_startup = true;
					}

					int cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_username, -1, NULL, 0, NULL, NULL );
					char *utf8_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
					cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_username, -1, utf8_username, cfg_val_length, NULL, NULL );

					cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_password, -1, NULL, 0, NULL, NULL );
					char *utf8_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
					cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, cfg_password, -1, utf8_password, cfg_val_length, NULL, NULL );

					// Freed in the connection thread;
					LOGIN_SETTINGS *ls = ( LOGIN_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( LOGIN_SETTINGS ) );
					ls->username = utf8_username;
					ls->password = utf8_password;

					CloseHandle( ( HANDLE )_CreateThread( NULL, 0, Connection, ( void * )ls, 0, NULL ) );
				}

				if ( lParam == FALSE )	// Set to true if we want a silent startup.
				{
					_ShowWindow( hWnd, SW_SHOW );
					_SetForegroundWindow( hWnd );
				}
			}
			else if ( wParam == LOGGING_IN )	// Logging in
			{
				_SendMessageW( hWnd, WM_CHANGE_CURSOR, TRUE, 0 );			// SetCursor only works from the main thread. Set it to an arrow with hourglass.

				UpdateMenus( UM_DISABLE );

				_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Loggin_In___ );
				_SendMessageW( g_hWnd_btn_login, WM_SETTEXT, 0, ( LPARAM )ST_Cancel );
				_EnableWindow( g_hWnd_username, FALSE );
				_EnableWindow( g_hWnd_password, FALSE );
				_EnableWindow( g_hWnd_remember, FALSE );
				_EnableWindow( g_hWnd_static1, FALSE );
				_EnableWindow( g_hWnd_static2, FALSE );

				if ( cfg_tray_icon == true )
				{
					g_nid.uFlags = NIF_TIP;
					_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ), L"VZ Enhanced - Logging In...\0", 28 );
					g_nid.szTip[ 27 ] = 0;	// Sanity
					_Shell_NotifyIconW( NIM_MODIFY, &g_nid );
				}
			}
			else if ( wParam == LOGGED_IN )	// Logged in
			{
				_SendMessageW( hWnd, WM_CHANGE_CURSOR, FALSE, 0 );	// Reset the cursor.

				UpdateMenus( UM_ENABLE );

				// This will never be seen, except maybe by Spy++.
				_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Logged_In );
				_SendMessageW( g_hWnd_btn_login, WM_SETTEXT, 0, ( LPARAM )ST_Log_Off );
				_SendMessageW( g_hWnd_username, WM_SETTEXT, 0, 0 );
				_SendMessageW( g_hWnd_password, WM_SETTEXT, 0, 0 );
				_EnableWindow( g_hWnd_username, FALSE );
				_EnableWindow( g_hWnd_password, FALSE );
				_EnableWindow( g_hWnd_remember, FALSE );
				_EnableWindow( g_hWnd_static1, FALSE );
				_EnableWindow( g_hWnd_static2, FALSE );

				if ( cfg_tray_icon == true )
				{
					g_nid.uFlags = NIF_TIP;
					_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ), L"VZ Enhanced - Logged In\0", 24 );
					g_nid.szTip[ 23 ] = 0;	// Sanity
					_Shell_NotifyIconW( NIM_MODIFY, &g_nid );
				}

				if ( silent_startup == false )
				{
					_ShowWindow( g_hWnd_main, SW_SHOW );
				}
				else
				{
					silent_startup = false;
				}

				_ShowWindow( hWnd, SW_HIDE );
			}
			else	// Default - Logged out/Disconnected
			{
				_SendMessageW( hWnd, WM_CHANGE_CURSOR, FALSE, 0 );	// Reset the cursor.

				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_contact_list, ( void * )NULL, 0, NULL ) );

				UpdateMenus( UM_DISABLE );

				_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Login );
				_EnableWindow( g_hWnd_btn_login, TRUE );
				_EnableWindow( g_hWnd_username, TRUE );
				_EnableWindow( g_hWnd_password, TRUE );
				_EnableWindow( g_hWnd_remember, TRUE );
				_EnableWindow( g_hWnd_static1, TRUE );
				_EnableWindow( g_hWnd_static2, TRUE );
				_SendMessageW( g_hWnd_btn_login, WM_SETTEXT, 0, ( LPARAM )ST_Log_In );
				_SendMessageW( g_hWnd_username, WM_SETTEXT, 0, ( LPARAM )cfg_username );

				if ( cfg_remember_login == true )
				{
					_SendMessageW( g_hWnd_password, WM_SETTEXT, 0, ( LPARAM )cfg_password );
				}
				else	// If we aren't set to save the password, then we can deleted our temporary one (created when we logged in).
				{
					if ( cfg_password != NULL )
					{
						GlobalFree( cfg_password );
						cfg_password = NULL;
					}
				}

				if ( cfg_tray_icon == true )
				{
					g_nid.uFlags = NIF_TIP;
					_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ), L"VZ Enhanced - Logged Out\0", 25 );
					g_nid.szTip[ 24 ] = 0;	// Sanity
					_Shell_NotifyIconW( NIM_MODIFY, &g_nid );
				}

				_ShowWindow( hWnd, SW_SHOW );
				_SetForegroundWindow( hWnd );

				if ( silent_startup == true )
				{
					silent_startup = false;
				}
			}
		}
		break;

		case WM_CLOSE:
		{
			if ( silent_startup == true )
			{
				silent_startup = false;
			}

			_ShowWindow( hWnd, SW_HIDE );
		}
		break;

		default:
		{
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}
	return TRUE;
}
