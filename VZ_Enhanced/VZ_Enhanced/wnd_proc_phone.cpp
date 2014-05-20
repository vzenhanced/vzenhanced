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
#include "utilities.h"
#include "string_tables.h"
#include "lite_gdi32.h"
#include "connection.h"

#define BTN_0			1000
#define BTN_1			1001
#define BTN_2			1002
#define BTN_3			1003
#define BTN_4			1004
#define BTN_5			1005
#define BTN_6			1006
#define BTN_7			1007
#define BTN_8			1008
#define BTN_9			1009

#define BTN_ACTION		1012
#define EDIT_NUMBER		1013
#define EDIT_NUMBER2	1014

static unsigned short bad_area_codes[ 29 ] = { 211, 242, 246, 264, 268, 284, 311, 345, 411, 441, 473, 511, 611, 649, 664, 711, 758, 767, 784, 809, 811, 829, 849, 868, 869, 876, 900, 911, 976 };

bool forward_incoming = false;	// If the call is incoming and not in the forward list, then let us forward the call manually.
displayinfo *edit_di = NULL;	// Display the number we want to forward (when forwarding a number in the call log listview).
forwardinfo *edit_fi = NULL;	// Display the numbers we want to edit (when editing a number in the forward list listview).

bool forward_focus = false;		// Allows number buttons to insert a value into the correct edit box.

WNDPROC EditProc = NULL;

LRESULT CALLBACK EditSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_CHAR:
		{
			char current_phone_number[ 17 ];
			_memzero( current_phone_number, 17 );
			int current_phone_number_length = _SendMessageA( hWnd, WM_GETTEXT, 17, ( LPARAM )current_phone_number );

			DWORD offset_begin = 0, offset_end = 0;
			_SendMessageA( hWnd, EM_GETSEL, ( WPARAM )&offset_begin, ( WPARAM )&offset_end );

			// If a '*' or '+' was entered, set the text and return. Don't perform the default window procedure.
			if ( wParam == '*' )
			{
				if ( ( current_phone_number[ 0 ] == '+' && offset_begin == 0 && ( offset_end == 0 || ( offset_end == 1 && current_phone_number_length >= 16 ) ) ) ||
					 ( current_phone_number[ 0 ] != '+' && offset_begin == offset_end && current_phone_number_length >= 15 ) )
				{
					return 0;
				}

				_SendMessageW( hWnd, EM_REPLACESEL, FALSE, ( LPARAM )L"*" );
				return 0;
			}
			else if ( wParam == '+' )
			{
				if ( ( current_phone_number[ 0 ] != '+' && offset_begin == 0 && offset_end == 0 ) || ( offset_begin == 0 && offset_end > 0 ) )
				{
					_SendMessageW( hWnd, EM_REPLACESEL, FALSE, ( LPARAM )L"+" );
					return 0;
				}
			}
			else if ( wParam >= 0x30 && wParam <= 0x39 )
			{
				if ( ( current_phone_number[ 0 ] == '+' && offset_begin == 0 && ( offset_end == 0 || ( offset_end == 1 && current_phone_number_length >= 16 ) ) ) ||
					 ( current_phone_number[ 0 ] != '+' && offset_begin == offset_end && current_phone_number_length >= 15 ) )
				{
					return 0;
				}
			}
		}
		break;

		case WM_PASTE:
		{
			if ( _OpenClipboard( hWnd ) )
			{
				bool ret = true;
				char phone_number[ 17 ];
				_memzero( phone_number, 17 );
				char paste_length = 0;

				// Get ASCII text from the clipboard.
				HANDLE hglbPaste = _GetClipboardData( CF_TEXT );
				if ( hglbPaste != NULL )
				{
					// Get whatever text data is in the clipboard.
					char *lpstrPaste = ( char * )GlobalLock( hglbPaste );
					if ( lpstrPaste != NULL )
					{
						// Find the length of the data and limit it to 15 + 1 ("+") characters.
						paste_length = ( char )min( 16, lstrlenA( lpstrPaste ) );

						// Copy the data to our phone number buffer.
						_memcpy_s( phone_number, 17, lpstrPaste, paste_length );
						phone_number[ paste_length ] = 0;	// Sanity.
					}

					GlobalUnlock( hglbPaste );
				}

				_CloseClipboard();

				if ( paste_length > 0 )
				{
					char i = 0;

					if ( phone_number[ 0 ] == '+' )
					{
						++i;
					}
					else if ( paste_length == 16 )
					{
						phone_number[ --paste_length ] = 0;
					}

					// Make sure the phone number contains only numbers or wildcard values.
					for ( ; i < paste_length; ++i )
					{
						if ( phone_number[ i ] != '*' && !is_digit( phone_number[ i ] ) )
						{
							ret = false;
							break;
						}
					}

					// If we have a value and all characters are acceptable, then set the text in the text box and return. Don't perform the default window procedure.
					if ( ret == true )
					{
						char current_phone_number[ 17 ];
						_memzero( current_phone_number, 17 );
						int current_phone_number_length = _SendMessageA( hWnd, WM_GETTEXT, 17, ( LPARAM )current_phone_number );

						DWORD offset_begin = 0, offset_end = 0;
						_SendMessageA( hWnd, EM_GETSEL, ( WPARAM )&offset_begin, ( LPARAM )&offset_end );

						if ( current_phone_number[ 0 ] == '+' && phone_number[ 0 ] != '+' && offset_begin > 0 && offset_end > 0 ||
							 current_phone_number[ 0 ] != '+' && ( ( phone_number[ 0 ] != '+' && ( ( current_phone_number_length + paste_length ) <= 15 ) ) ||
																   ( phone_number[ 0 ] == '+' && offset_begin == 0 ) ) )
						{
							_SendMessageA( hWnd, EM_REPLACESEL, FALSE, ( LPARAM )phone_number );
							
						}

						return 0;
					}
				}
			}
		}
		break;
	}

	
	// Everything that we don't handle gets passed back to the parent to process.
	return _CallWindowProcW( EditProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK PhoneWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// 0 = default, 1 = allow wildcards, 2 = show forward edit control and allow wildcards
			CHAR enable_items = ( CHAR )( ( CREATESTRUCT * )lParam )->lpCreateParams;

			HWND hWnd_static = _CreateWindowW( WC_STATIC, NULL, SS_GRAYFRAME | WS_CHILD | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_phone_static1 = _CreateWindowW( WC_STATIC, ST_Phone_Number_, WS_CHILD | WS_VISIBLE, 20, 20, ( rc.right - rc.left ) - 40, 15, hWnd, NULL, NULL, NULL );
			HWND g_hWnd_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NOHIDESEL | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 35, ( rc.right - rc.left ) - 40, 20, hWnd, ( HMENU )EDIT_NUMBER, NULL, NULL );

			if ( enable_items == 2 )
			{
				HWND g_hWnd_phone_static2 = _CreateWindowW( WC_STATIC, ST_Forward_to_, WS_CHILD | WS_VISIBLE, 20, 60, ( rc.right - rc.left ) - 40, 15, hWnd, NULL, NULL, NULL );
				HWND g_hWnd_number2 = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NOHIDESEL | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 75, ( rc.right - rc.left ) - 40, 20, hWnd, ( HMENU )EDIT_NUMBER2, NULL, NULL );
				_SendMessageW( g_hWnd_number2, EM_LIMITTEXT, 15, 0 );
				_SendMessageW( g_hWnd_phone_static2, WM_SETFONT, ( WPARAM )hFont, 0 );
				_SendMessageW( g_hWnd_number2, WM_SETFONT, ( WPARAM )hFont, 0 );
			}

			HWND g_hWnd_1 = _CreateWindowW( WC_BUTTON, L"1", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 65 + ( enable_items == 2 ? 40 : 0 ), 50, 25, hWnd, ( HMENU )BTN_1, NULL, NULL );
			HWND g_hWnd_2 = _CreateWindowW( WC_BUTTON, L"2 ABC", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 75, 65 + ( enable_items == 2 ? 40 : 0 ), 50, 25, hWnd, ( HMENU )BTN_2, NULL, NULL );
			HWND g_hWnd_3 = _CreateWindowW( WC_BUTTON, L"3 DEF", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 130, 65 + ( enable_items == 2 ? 40 : 0 ), 50, 25, hWnd, ( HMENU )BTN_3, NULL, NULL );

			HWND g_hWnd_4 = _CreateWindowW( WC_BUTTON, L"4 GHI", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 95 + ( enable_items == 2 ? 40 : 0 ), 50, 25, hWnd, ( HMENU )BTN_4, NULL, NULL );
			HWND g_hWnd_5 = _CreateWindowW( WC_BUTTON, L"5 JKL", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 75, 95 + ( enable_items == 2 ? 40 : 0 ), 50, 25, hWnd, ( HMENU )BTN_5, NULL, NULL );
			HWND g_hWnd_6 = _CreateWindowW( WC_BUTTON, L"6 MNO", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 130, 95 + ( enable_items == 2 ? 40 : 0 ), 50, 25, hWnd, ( HMENU )BTN_6, NULL, NULL );

			HWND g_hWnd_7 = _CreateWindowW( WC_BUTTON, L"7 PQRS", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 125 + ( enable_items == 2 ? 40 : 0 ), 50, 25, hWnd, ( HMENU )BTN_7, NULL, NULL );
			HWND g_hWnd_8 = _CreateWindowW( WC_BUTTON, L"8 TUV", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 75, 125 + ( enable_items == 2 ? 40 : 0 ), 50, 25, hWnd, ( HMENU )BTN_8, NULL, NULL );
			HWND g_hWnd_9 = _CreateWindowW( WC_BUTTON, L"9 WXYZ", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 130, 125 + ( enable_items == 2 ? 40 : 0 ), 50, 25, hWnd, ( HMENU )BTN_9, NULL, NULL );

			HWND g_hWnd_0 = _CreateWindowW( WC_BUTTON, L"0", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 75, 155 + ( enable_items == 2 ? 40 : 0 ), 50, 25, hWnd, ( HMENU )BTN_0, NULL, NULL );

			HWND g_hWnd_dial_action = _CreateWindowW( WC_BUTTON, ST_Dial_Phone_Number, WS_CHILD | WS_TABSTOP | WS_VISIBLE, ( rc.right - rc.left - ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) - 40 ) / 2, rc.bottom - 32, ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) + 40, 23, hWnd, ( HMENU )BTN_ACTION, NULL, NULL );

			_SendMessageW( g_hWnd_number, EM_LIMITTEXT, ( enable_items != 0 ? 16 : 15 ), 0 );

			_SendMessageW( g_hWnd_phone_static1, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_number, WM_SETFONT, ( WPARAM )hFont, 0 );

			

			_SendMessageW( g_hWnd_dial_action, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_1, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_2, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_3, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_4, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_5, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_6, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_7, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_8, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_9, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_0, WM_SETFONT, ( WPARAM )hFont, 0 );

			if ( enable_items != 0 )
			{
				EditProc = ( WNDPROC )_GetWindowLongW( g_hWnd_number, GWL_WNDPROC );
				_SetWindowLongW( g_hWnd_number, GWL_WNDPROC, ( LONG )EditSubProc );
			}

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetStockObject( WHITE_BRUSH ) );
		}
		break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			_SetFocus( hWnd );
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case BTN_ACTION:
				{
					char number[ 17 ];
					int length = _SendMessageA( _GetDlgItem( hWnd, EDIT_NUMBER ), WM_GETTEXT, 17, ( LPARAM )number );

					if ( length > 0 )
					{
						if ( length == 10 || ( length == 11 && number[ 0 ] == '1' ) )
						{
							char value[ 3 ];
							if ( length == 10 )
							{
								value[ 0 ] = number[ 0 ];
								value[ 1 ] = number[ 1 ];
								value[ 2 ] = number[ 2 ];
							}
							else
							{
								value[ 0 ] = number[ 1 ];
								value[ 1 ] = number[ 2 ];
								value[ 2 ] = number[ 3 ];
							}

							unsigned short area_code = ( unsigned short )_strtoul( value, NULL, 10 );

							for ( int i = 0; i < 29; ++i )
							{
								if ( area_code == bad_area_codes[ i ] )
								{
									_MessageBoxW( hWnd, ST_restricted_area_code, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
									_SetFocus( _GetDlgItem( hWnd, EDIT_NUMBER ) );
									return 0;
								}
							}
						}
					}
					else
					{
						_MessageBoxW( hWnd, ST_enter_valid_phone_number, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
						_SetFocus( _GetDlgItem( hWnd, EDIT_NUMBER ) );
						break;
					}

					if ( hWnd == g_hWnd_dial )
					{
						char *call_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * length + 1 );
						_memcpy_s( call_to, length + 1, number, length );
						call_to[ length ] = 0;	// Sanity

						callerinfo *ci = ( callerinfo * )GlobalAlloc( GMEM_FIXED, sizeof( callerinfo ) );
						ci->call_from = NULL;	// Our service number.
						ci->call_to = call_to;
						ci->call_reference_id = NULL;
						ci->caller_id = NULL;
						ci->forward_to = NULL;
						ci->ignored = false;
						ci->forwarded = false;

						// ci will be freed in CallPhoneNumber.
						CloseHandle( ( HANDLE )_CreateThread( NULL, 0, CallPhoneNumber, ( void * )ci, 0, NULL ) );
					}
					else if ( hWnd == g_hWnd_forward )
					{
						char number2[ 17 ];
						int length2 = _SendMessageA( _GetDlgItem( hWnd, EDIT_NUMBER2 ), WM_GETTEXT, 17, ( LPARAM )number2 );

						if ( length2 > 0 )
						{
							if ( length2 == 10 || ( length2 == 11 && number2[ 0 ] == '1' ) )
							{
								char value[ 3 ];
								if ( length2 == 10 )
								{
									value[ 0 ] = number2[ 0 ];
									value[ 1 ] = number2[ 1 ];
									value[ 2 ] = number2[ 2 ];
								}
								else
								{
									value[ 0 ] = number2[ 1 ];
									value[ 1 ] = number2[ 2 ];
									value[ 2 ] = number2[ 3 ];
								}

								unsigned short area_code = ( unsigned short )_strtoul( value, NULL, 10 );

								for ( int i = 0; i < 29; ++i )
								{
									if ( area_code == bad_area_codes[ i ] )
									{
										_MessageBoxW( hWnd, ST_restricted_area_code, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
										_SetFocus( _GetDlgItem( hWnd, EDIT_NUMBER2 ) );
										return 0;
									}
								}
							}
						}
						else
						{
							_MessageBoxW( hWnd, ST_enter_valid_phone_number, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
							_SetFocus( _GetDlgItem( hWnd, EDIT_NUMBER2 ) );
							break;
						}

						if ( forward_incoming == true )
						{
							if ( edit_di != NULL )
							{
								char *forward_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * length2 + 1 );
								_memcpy_s( forward_to, length2 + 1, number2, length2 );
								forward_to[ length2 ] = 0;	// Sanity

								edit_di->process_incoming = false;
								edit_di->ci.forward_to = forward_to;
								edit_di->ci.forwarded = true;

								// Forward to phone number
								edit_di->forward_to = FormatPhoneNumber( edit_di->ci.forward_to );

								_InvalidateRect( g_hWnd_list, NULL, TRUE );

								CloseHandle( ( HANDLE )_CreateThread( NULL, 0, ForwardIncomingCall, ( void * )&( edit_di->ci ), 0, NULL ) );
							}

							forward_incoming = false;
						}
						else
						{
							forwardupdateinfo *fui = ( forwardupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardupdateinfo ) );
							fui->fi = NULL;
							fui->action = 0;	// Add = 0, 1 = Remove, 2 = Add all tree items, 3 = Update
							fui->call_from = NULL;
							fui->forward_to = NULL;

							if ( edit_di != NULL )	// Add item from call log listview to forward_list and forward list listview.
							{
								fui->hWnd = g_hWnd_list;	// Update call log items.
							}
							else if ( edit_fi != NULL )	// Update all the forward information.
							{
								char *call_from = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * length + 1 );
								_memcpy_s( call_from, length + 1, number, length );
								call_from[ length ] = 0;	// Sanity

								fui->call_from = call_from;

								fui->hWnd = g_hWnd_forward_list;	// Update forward list items.
								fui->action = 3;	// Update
							}
							else	// Add item to forward list listview and update items in call log listview.
							{
								char *call_from = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * length + 1 );
								_memcpy_s( call_from, length + 1, number, length );
								call_from[ length ] = 0;	// Sanity

								fui->call_from = call_from;

								fui->hWnd = g_hWnd_forward_list;	// Update forward list items.
							}

							char *forward_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * length2 + 1 );
							_memcpy_s( forward_to, length2 + 1, number2, length2 );
							forward_to[ length2 ] = 0;	// Sanity

							fui->forward_to = forward_to;

							// Add items. fui is freed in the update_forward_list thread.
							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_forward_list, ( void * )fui, 0, NULL ) );
						}
					}
					else if ( hWnd == g_hWnd_ignore_phone_number )
					{
						ignoreupdateinfo *iui = ( ignoreupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreupdateinfo ) );
						iui->ii = NULL;
						iui->action = 0;	// Add = 0, 1 = Remove
						iui->hWnd = g_hWnd_ignore_list;

						char *ignore_number = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * length + 1 );
						_memcpy_s( ignore_number, length + 1, number, length );
						ignore_number[ length ] = 0;	// Sanity

						iui->phone_number = ignore_number;

						// Add items. iui is freed in the update_ignore_list thread.
						CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_list, ( void * )iui, 0, NULL ) );
					}

					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case EDIT_NUMBER:
				{
					if ( HIWORD( wParam ) == EN_SETFOCUS )
					{
						forward_focus = false;
						_PostMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), EM_SETSEL, 0, -1 );
					}
				}
				break;

				case EDIT_NUMBER2:
				{
					if ( HIWORD( wParam ) == EN_SETFOCUS )
					{
						forward_focus = true;
						_PostMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), EM_SETSEL, 0, -1 );
					}
				}
				break;

				case BTN_1:
				{
					if ( forward_focus == false )
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), EM_REPLACESEL, FALSE, ( LPARAM )L"1" );
					}
					else
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), EM_REPLACESEL, FALSE, ( LPARAM )L"1" );
					}
				}
				break;

				case BTN_2:
				{
					if ( forward_focus == false )
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), EM_REPLACESEL, FALSE, ( LPARAM )L"2" );
					}
					else
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), EM_REPLACESEL, FALSE, ( LPARAM )L"2" );
					}
				}
				break;

				case BTN_3:
				{
					if ( forward_focus == false )
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), EM_REPLACESEL, FALSE, ( LPARAM )L"3" );
					}
					else
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), EM_REPLACESEL, FALSE, ( LPARAM )L"3" );
					}
				}
				break;

				case BTN_4:
				{
					if ( forward_focus == false )
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), EM_REPLACESEL, FALSE, ( LPARAM )L"4" );
					}
					else
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), EM_REPLACESEL, FALSE, ( LPARAM )L"4" );
					}
				}
				break;

				case BTN_5:
				{
					if ( forward_focus == false )
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), EM_REPLACESEL, FALSE, ( LPARAM )L"5" );
					}
					else
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), EM_REPLACESEL, FALSE, ( LPARAM )L"5" );
					}
				}
				break;

				case BTN_6:
				{
					if ( forward_focus == false )
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), EM_REPLACESEL, FALSE, ( LPARAM )L"6" );
					}
					else
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), EM_REPLACESEL, FALSE, ( LPARAM )L"6" );
					}
				}
				break;

				case BTN_7:
				{
					if ( forward_focus == false )
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), EM_REPLACESEL, FALSE, ( LPARAM )L"7" );
					}
					else
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), EM_REPLACESEL, FALSE, ( LPARAM )L"7" );
					}
				}
				break;

				case BTN_8:
				{
					if ( forward_focus == false )
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), EM_REPLACESEL, FALSE, ( LPARAM )L"8" );
					}
					else
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), EM_REPLACESEL, FALSE, ( LPARAM )L"8" );
					}
				}
				break;

				case BTN_9:
				{
					if ( forward_focus == false )
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), EM_REPLACESEL, FALSE, ( LPARAM )L"9" );
					}
					else
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), EM_REPLACESEL, FALSE, ( LPARAM )L"9" );
					}
				}
				break;

				case BTN_0:
				{
					if ( forward_focus == false )
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), EM_REPLACESEL, FALSE, ( LPARAM )L"0" );
					}
					else
					{
						_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), EM_REPLACESEL, FALSE, ( LPARAM )L"0" );
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_PROPAGATE:
		{
			if ( wParam == 1 )	// Forward phone number.
			{
				forward_focus = false;
				forward_incoming = false;
				edit_fi = NULL;

				edit_di = ( displayinfo * )lParam;

				_SendMessageW( _GetDlgItem( hWnd, BTN_ACTION ), WM_SETTEXT, 0, ( LPARAM )ST_Forward_Phone_Number );

				_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), WM_SETTEXT, 0, 0 );

				if ( edit_di != NULL )	// If this is not NULL, then we're adding a forwarding number from the call log listview.
				{
					_SendMessageA( _GetDlgItem( hWnd, EDIT_NUMBER ), WM_SETTEXT, 0, ( LPARAM )edit_di->ci.call_from );

					_EnableWindow( _GetDlgItem( hWnd, EDIT_NUMBER ), FALSE );		// We don't want to edit this number.
					_SetFocus( _GetDlgItem( hWnd, EDIT_NUMBER2 ) );
				}
				else	// If edit_di is NULL, then we're adding new numbers from the forward list listview.
				{
					_EnableWindow( _GetDlgItem( hWnd, EDIT_NUMBER ), TRUE );		// Allow us to edit this number.

					_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), WM_SETTEXT, 0, 0 );

					_SetFocus( _GetDlgItem( hWnd, EDIT_NUMBER ) );
				}
			}
			else if ( wParam == 0 )	// Dial phone number.
			{
				char *phone_number = ( char * )lParam;

				if ( phone_number != NULL && is_num( phone_number ) == 0 )	// Don't allow range numbers or anything with weird symbols.
				{
					_SendMessageA( _GetDlgItem( hWnd, EDIT_NUMBER ), WM_SETTEXT, 0, ( LPARAM )( phone_number[ 0 ] == '+' ? phone_number + 1 : phone_number ) );
				}
				else
				{
					_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), WM_SETTEXT, 0, 0 );
				}

				_SendMessageW( _GetDlgItem( hWnd, BTN_ACTION ), WM_SETTEXT, 0, ( LPARAM )ST_Dial_Phone_Number );

				_SetFocus( _GetDlgItem( hWnd, EDIT_NUMBER ) );
			}
			else if ( wParam == 2 )	// Ignore phone number.
			{
				_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), WM_SETTEXT, 0, 0 );
				_SendMessageW( _GetDlgItem( hWnd, BTN_ACTION ), WM_SETTEXT, 0, ( LPARAM )ST_Ignore_Phone_Number );

				_SetFocus( _GetDlgItem( hWnd, EDIT_NUMBER ) );
			}
			else if ( wParam == 3 )	// Update forward information.
			{
				forward_focus = false;
				forward_incoming = false;
				edit_di = NULL;

				edit_fi = ( forwardinfo * )lParam;

				if ( edit_fi != NULL )	// If not NULL, then allow update.
				{
					_SendMessageW( _GetDlgItem( hWnd, BTN_ACTION ), WM_SETTEXT, 0, ( LPARAM )ST_Update_Phone_Number );

					_SendMessageA( _GetDlgItem( hWnd, EDIT_NUMBER ), WM_SETTEXT, 0, ( LPARAM )edit_fi->c_call_from );
					_SendMessageA( _GetDlgItem( hWnd, EDIT_NUMBER2 ), WM_SETTEXT, 0, ( LPARAM )edit_fi->c_forward_to );

					_EnableWindow( _GetDlgItem( hWnd, EDIT_NUMBER ), FALSE );	// We don't want to edit this number.

					_SetFocus( _GetDlgItem( hWnd, EDIT_NUMBER2 ) );
				}
				else	// If NULL, then I suppose we'll just add new numbers.
				{
					_SendMessageW( _GetDlgItem( hWnd, BTN_ACTION ), WM_SETTEXT, 0, ( LPARAM )ST_Forward_Phone_Number );

					_EnableWindow( _GetDlgItem( hWnd, EDIT_NUMBER ), TRUE );	// Allow us to edit this number.

					_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), WM_SETTEXT, 0, 0 );
					_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), WM_SETTEXT, 0, 0 );

					_SetFocus( _GetDlgItem( hWnd, EDIT_NUMBER ) );
				}
			}
			else if ( wParam == 4 )	// An incoming number to forward.
			{
				edit_di = ( displayinfo * )lParam;

				if ( edit_di != NULL )
				{
					forward_incoming = true;

					_SendMessageW( _GetDlgItem( hWnd, BTN_ACTION ), WM_SETTEXT, 0, ( LPARAM )ST_Forward_Phone_Number );

					_SendMessageA( _GetDlgItem( hWnd, EDIT_NUMBER ), WM_SETTEXT, 0, ( LPARAM )edit_di->ci.call_to );
					_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), WM_SETTEXT, 0, 0 );

					_EnableWindow( _GetDlgItem( hWnd, EDIT_NUMBER ), FALSE );	// We don't want to edit this number.

					_SetFocus( _GetDlgItem( hWnd, EDIT_NUMBER2 ) );
				}
				else	// If NULL, then I suppose we'll just add new numbers.
				{
					forward_incoming = false;

					_SendMessageW( _GetDlgItem( hWnd, BTN_ACTION ), WM_SETTEXT, 0, ( LPARAM )ST_Forward_Phone_Number );

					_EnableWindow( _GetDlgItem( hWnd, EDIT_NUMBER ), TRUE );	// Allow us to edit this number.

					_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER ), WM_SETTEXT, 0, 0 );
					_SendMessageW( _GetDlgItem( hWnd, EDIT_NUMBER2 ), WM_SETTEXT, 0, 0 );

					_SetFocus( _GetDlgItem( hWnd, EDIT_NUMBER ) );
				}
			}

			_ShowWindow( hWnd, SW_SHOWNORMAL );
		}
		break;

		case WM_ACTIVATE:
		{
			// 0 = inactive, > 0 = active
			g_hWnd_active = ( wParam == 0 ? NULL : hWnd );

            return FALSE;
		}
		break;

		case WM_CLOSE:
		{
			_DestroyWindow( hWnd );
		}
		break;

		case WM_DESTROY:
		{
			if ( hWnd == g_hWnd_dial )
			{
				g_hWnd_dial = NULL;
			}
			else if ( hWnd == g_hWnd_forward )
			{
				edit_di = NULL;
				edit_fi = NULL;
				forward_focus = false;
				forward_incoming = false;

				g_hWnd_forward = NULL;
			}
			else if ( hWnd == g_hWnd_ignore_phone_number )
			{
				g_hWnd_ignore_phone_number = NULL;
			}
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
