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
#include "string_tables.h"
#include "lite_gdi32.h"

#define BTN_EXIT_INFO	1000

LRESULT CALLBACK AccountWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HWND hWnd_static = _CreateWindowW( WC_STATIC, NULL, SS_GRAYFRAME | WS_CHILD | WS_VISIBLE, 10, 10, rc.right - 20, 215, hWnd, NULL, NULL, NULL );

			HWND hWnd_static1 = _CreateWindowW( WC_STATIC, ST_Account_ID_, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 20, 95, 20, hWnd, NULL, NULL, NULL );
			HWND hWnd_static2 = _CreateWindowW( WC_STATIC, ST_Account_Type_, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 40, 95, 20, hWnd, NULL, NULL, NULL );
			HWND hWnd_static3 = _CreateWindowW( WC_STATIC, ST_Account_Status_, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 60, 95, 20, hWnd, NULL, NULL, NULL );
			HWND hWnd_static4 = _CreateWindowW( WC_STATIC, ST_Principal_ID_, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 80, 95, 20, hWnd, NULL, NULL, NULL );
			HWND hWnd_static5 = _CreateWindowW( WC_STATIC, ST_Service_Type_, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 100, 95, 20, hWnd, NULL, NULL, NULL );
			HWND hWnd_static6 = _CreateWindowW( WC_STATIC, ST_Service_Status_, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 120, 95, 20, hWnd, NULL, NULL, NULL );
			HWND hWnd_static7 = _CreateWindowW( WC_STATIC, ST_Service_Context_, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 140, 95, 20, hWnd, NULL, NULL, NULL );
			HWND hWnd_static8 = _CreateWindowW( WC_STATIC, ST_Phone_Number_, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 160, 95, 20, hWnd, NULL, NULL, NULL );
			HWND hWnd_static9 = _CreateWindowW( WC_STATIC, ST_Privacy_Value_, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 180, 95, 20, hWnd, NULL, NULL, NULL );
			HWND hWnd_static10 = _CreateWindowW( WC_STATIC, ST_Features_, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 200, 95, 20, hWnd, NULL, NULL, NULL );

			wchar_t *val = NULL;
			int val_length = 0;

			if ( account_id != NULL )
			{
				val_length = MultiByteToWideChar( CP_UTF8, 0, account_id, -1, NULL, 0 );	// Include the NULL terminator.
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
				MultiByteToWideChar( CP_UTF8, 0, account_id, -1, val, val_length );
			}

			HWND hWnd_edit1 = _CreateWindowW( WC_EDIT, val, ES_READONLY | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE, 115, 20, 150, 20, hWnd, NULL, NULL, NULL );

			GlobalFree( val );
			val = NULL;

			if ( account_type != NULL )
			{
				val_length = MultiByteToWideChar( CP_UTF8, 0, account_type, -1, NULL, 0 );	// Include the NULL terminator.
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
				MultiByteToWideChar( CP_UTF8, 0, account_type, -1, val, val_length );
			}

			HWND hWnd_edit2 = _CreateWindowW( WC_EDIT, val, ES_READONLY | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE, 115, 40, 150, 20, hWnd, NULL, NULL, NULL );

			GlobalFree( val );
			val = NULL;

			if ( account_status != NULL )
			{
				val_length = MultiByteToWideChar( CP_UTF8, 0, account_status, -1, NULL, 0 );	// Include the NULL terminator.
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
				MultiByteToWideChar( CP_UTF8, 0, account_status, -1, val, val_length );
			}

			HWND hWnd_edit3 = _CreateWindowW( WC_EDIT, val, ES_READONLY | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE, 115, 60, 150, 20, hWnd, NULL, NULL, NULL );

			GlobalFree( val );
			val = NULL;

			if ( principal_id != NULL )
			{
				val_length = MultiByteToWideChar( CP_UTF8, 0, principal_id, -1, NULL, 0 );	// Include the NULL terminator.
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
				MultiByteToWideChar( CP_UTF8, 0, principal_id, -1, val, val_length );
			}

			HWND hWnd_edit4 = _CreateWindowW( WC_EDIT, val, ES_READONLY | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE, 115, 80, 150, 20, hWnd, NULL, NULL, NULL );

			GlobalFree( val );
			val = NULL;

			if ( service_type != NULL )
			{
				val_length = MultiByteToWideChar( CP_UTF8, 0, service_type, -1, NULL, 0 );	// Include the NULL terminator.
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
				MultiByteToWideChar( CP_UTF8, 0, service_type, -1, val, val_length );
			}

			HWND hWnd_edit5 = _CreateWindowW( WC_EDIT, val, ES_READONLY | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE, 115, 100, 150, 20, hWnd, NULL, NULL, NULL );

			GlobalFree( val );
			val = NULL;

			if ( service_status != NULL )
			{
				val_length = MultiByteToWideChar( CP_UTF8, 0, service_status, -1, NULL, 0 );	// Include the NULL terminator.
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
				MultiByteToWideChar( CP_UTF8, 0, service_status, -1, val, val_length );
			}

			HWND hWnd_edit6 = _CreateWindowW( WC_EDIT, val, ES_READONLY | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE, 115, 120, 150, 20, hWnd, NULL, NULL, NULL );

			GlobalFree( val );
			val = NULL;

			if ( service_context != NULL )
			{
				val_length = MultiByteToWideChar( CP_UTF8, 0, service_context, -1, NULL, 0 );	// Include the NULL terminator.
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
				MultiByteToWideChar( CP_UTF8, 0, service_context, -1, val, val_length );
			}

			HWND hWnd_edit7 = _CreateWindowW( WC_EDIT, val, ES_READONLY | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE, 115, 140, 150, 20, hWnd, NULL, NULL, NULL );

			GlobalFree( val );
			val = NULL;

			if ( service_phone_number != NULL )
			{
				val_length = MultiByteToWideChar( CP_UTF8, 0, service_phone_number, -1, NULL, 0 );	// Include the NULL terminator.
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
				MultiByteToWideChar( CP_UTF8, 0, service_phone_number, -1, val, val_length );
			}

			HWND hWnd_edit8 = _CreateWindowW( WC_EDIT, val, ES_READONLY | WS_CHILD | ES_AUTOHSCROLL | WS_VISIBLE, 115, 160, 150, 20, hWnd, NULL, NULL, NULL );

			GlobalFree( val );
			val = NULL;

			if ( service_privacy_value != NULL )
			{
				val_length = MultiByteToWideChar( CP_UTF8, 0, service_privacy_value, -1, NULL, 0 );	// Include the NULL terminator.
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
				MultiByteToWideChar( CP_UTF8, 0, service_privacy_value, -1, val, val_length );
			}

			HWND hWnd_edit9 = _CreateWindowW( WC_EDIT, val, ES_READONLY | WS_CHILD | ES_AUTOHSCROLL | WS_VISIBLE, 115, 180, 150, 20, hWnd, NULL, NULL, NULL );

			GlobalFree( val );
			val = NULL;

			if ( service_features != NULL )
			{
				val_length = MultiByteToWideChar( CP_UTF8, 0, service_features, -1, NULL, 0 );	// Include the NULL terminator.
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
				MultiByteToWideChar( CP_UTF8, 0, service_features, -1, val, val_length );
			}

			HWND hWnd_edit10 = _CreateWindowW( WC_EDIT, val, ES_READONLY | WS_CHILD | ES_AUTOHSCROLL | WS_VISIBLE, 115, 200, 150, 20, hWnd, NULL, NULL, NULL );

			GlobalFree( val );
			val = NULL;

			HWND ok = _CreateWindowW( WC_BUTTON, ST_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, ( rc.right - rc.left - ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) ) / 2, rc.bottom - 32, _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ), 23, hWnd, ( HMENU )BTN_EXIT_INFO, NULL, NULL );

			_SendMessageW( hWnd_static1, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_static2, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_static3, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_static4, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_static5, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_static6, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_static7, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_static8, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_static9, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_static10, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_edit1, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_edit2, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_edit3, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_edit4, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_edit5, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_edit6, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_edit7, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_edit8, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_edit9, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_edit10, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( ok, WM_SETFONT, ( WPARAM )hFont, 0 );

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
				case BTN_EXIT_INFO:
				{
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
			g_hWnd_account = NULL;
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
