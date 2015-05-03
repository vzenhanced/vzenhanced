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
#include "string_tables.h"
#include "utilities.h"

#define BTN_EXIT_LINES	1000

HWND g_hWnd_selected_line = NULL;

LRESULT CALLBACK PhoneLinesWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HWND hWnd_static = _CreateWindowW( WC_STATIC, NULL, SS_GRAYFRAME | WS_CHILD | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			HWND hWnd_static1 = _CreateWindowW( WC_STATIC, ST_Select_line, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 20, rc.right - 40, 20, hWnd, NULL, NULL, NULL );
			g_hWnd_selected_line = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTBOX, NULL, WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 20, 40, rc.right - 40, 75, hWnd, NULL, NULL, NULL );

			if ( service_phone_number != NULL )
			{
				for ( int i = 0; i < phone_lines; ++i )
				{
					wchar_t *phone_number = FormatPhoneNumber( service_phone_number[ i ] );
					_SendMessageW( g_hWnd_selected_line, LB_ADDSTRING, 0, ( LPARAM )phone_number );
					GlobalFree( phone_number );
				}
			}

			_SendMessageA( g_hWnd_selected_line, LB_SETCURSEL, 0, 0 );

			HWND ok = _CreateWindowW( WC_BUTTON, ST_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, ( rc.right - rc.left - ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) ) / 2, rc.bottom - 32, _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ), 23, hWnd, ( HMENU )BTN_EXIT_LINES, NULL, NULL );

			_SendMessageW( g_hWnd_selected_line, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_static1, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( ok, WM_SETFONT, ( WPARAM )hFont, 0 );

			_EnableWindow( g_hWnd_login, FALSE );
			_SendMessageW( g_hWnd_login, WM_CHANGE_CURSOR, FALSE, 0 );

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case IDOK:
				case BTN_EXIT_LINES:
				{
					int selected_line = _SendMessageA( g_hWnd_selected_line, LB_GETCURSEL, 0, 0 );

					// Make sure that the current phone line is within range of the service phone number array.
					if ( selected_line < 0 || selected_line >= phone_lines )
					{
						selected_line = 0;
					}

					current_phone_line = selected_line;

					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
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

		case WM_CLOSE:
		{
			_DestroyWindow( hWnd );
		}
		break;

		case WM_DESTROY:
		{
			_SendMessageW( g_hWnd_login, WM_CHANGE_CURSOR, TRUE, 0 );
			_EnableWindow( g_hWnd_login, TRUE );
			_SetForegroundWindow( g_hWnd_login );

			if ( select_line_semaphore != NULL )
			{
				ReleaseSemaphore( select_line_semaphore, 1, NULL );
			}

			g_hWnd_phone_lines = NULL;
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
