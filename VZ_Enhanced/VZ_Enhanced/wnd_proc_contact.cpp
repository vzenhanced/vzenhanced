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
#include "connection.h"
#include "utilities.h"
#include "string_tables.h"

#include "lite_comdlg32.h"
#include "lite_gdi32.h"
#include "lite_gdiplus.h"

#define BTN_EXIT_INFO		1000
#define BTN_UPDATE_PICTURE	1001
#define BTN_REMOVE_PICTURE	1002

HWND g_hWnd_title = NULL;
HWND g_hWnd_first_name = NULL;
HWND g_hWnd_last_name = NULL;
HWND g_hWnd_nickname = NULL;

HWND g_hWnd_company = NULL;
HWND g_hWnd_job_title = NULL;
HWND g_hWnd_department = NULL;
HWND g_hWnd_profession = NULL;

HWND g_hWnd_home_phone_number = NULL;
HWND g_hWnd_cell_phone_number = NULL;
HWND g_hWnd_office_phone_number = NULL;
HWND g_hWnd_other_phone_number = NULL;

HWND g_hWnd_work_phone_number = NULL;
HWND g_hWnd_fax_number = NULL;

HWND g_hWnd_email_address = NULL;
HWND g_hWnd_web_page = NULL;


HWND g_hWnd_remove_picture = NULL;

HWND g_hWnd_contact_action = NULL;

HWND g_hWnd_static_picture = NULL;

contactinfo *edit_ci = NULL;

unsigned int g_picture_height = 0;
unsigned int g_picture_width = 0;
HBITMAP g_hbm_picture = NULL;

wchar_t *g_picture_file_path = NULL;
bool remove_picture = false;

LRESULT CALLBACK ContactWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HWND hWnd_static = _CreateWindowW( WC_STATIC, NULL, SS_GRAYFRAME | WS_CHILD | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static1 = _CreateWindowW( WC_STATIC, ST_Title_, WS_CHILD | WS_VISIBLE, 20, 22, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_title = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 20, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static2 = _CreateWindowW( WC_STATIC, ST_First_Name_, WS_CHILD | WS_VISIBLE, 20, 47, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_first_name = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 45, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static3 = _CreateWindowW( WC_STATIC, ST_Last_Name_, WS_CHILD | WS_VISIBLE, 20, 72, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_last_name = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 70, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static4 = _CreateWindowW( WC_STATIC, ST_Nickname_, WS_CHILD | WS_VISIBLE, 20, 97, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_nickname = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 95, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static5 = _CreateWindowW( WC_STATIC, ST_Company_, WS_CHILD | WS_VISIBLE, 20, 122, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_company = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 120, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static6 = _CreateWindowW( WC_STATIC, ST_Job_Title_, WS_CHILD | WS_VISIBLE, 20, 147, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_job_title = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 145, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static7 = _CreateWindowW( WC_STATIC, ST_Department_, WS_CHILD | WS_VISIBLE, 20, 172, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_department = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 170, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static8 = _CreateWindowW( WC_STATIC, ST_Profession_, WS_CHILD | WS_VISIBLE, 20, 197, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_profession = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 100, 195, 100, 20, hWnd, NULL, NULL, NULL );



			HWND g_hWnd_static9 = _CreateWindowW( WC_STATIC, ST_Home_Phone_, WS_CHILD | WS_VISIBLE, 220, 22, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_home_phone_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 20, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static10 = _CreateWindowW( WC_STATIC, ST_Cell_Phone_, WS_CHILD | WS_VISIBLE, 220, 47, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_cell_phone_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 45, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static11 = _CreateWindowW( WC_STATIC, ST_Office_Phone_, WS_CHILD | WS_VISIBLE, 220, 72, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_office_phone_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 70, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static12 = _CreateWindowW( WC_STATIC, ST_Other_Phone_, WS_CHILD | WS_VISIBLE, 220, 97, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_other_phone_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 95, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static13 = _CreateWindowW( WC_STATIC, ST_Work_Phone_, WS_CHILD | WS_VISIBLE, 220, 122, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_work_phone_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 120, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static14 = _CreateWindowW( WC_STATIC, ST_Fax_, WS_CHILD | WS_VISIBLE, 220, 147, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_fax_number = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 145, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static15 = _CreateWindowW( WC_STATIC, ST_Email_Address_, WS_CHILD | WS_VISIBLE, 220, 172, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_email_address = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 170, 100, 20, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_static16 = _CreateWindowW( WC_STATIC, ST_Web_Page_, WS_CHILD | WS_VISIBLE, 220, 197, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_web_page = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 300, 195, 100, 20, hWnd, NULL, NULL, NULL );



			HWND g_hWnd_static17 = _CreateWindowW( WC_STATIC, ST_Contact_Picture_, WS_CHILD | WS_VISIBLE, 420, 20, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_static_picture = _CreateWindowW( WC_STATIC, NULL, SS_OWNERDRAW | WS_BORDER | WS_CHILD | WS_VISIBLE, 420, 35, 100, 100, hWnd, NULL, NULL, NULL );

			HWND g_hWnd_update_picture = _CreateWindowW( WC_BUTTON, ST_Choose_File___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 420, 145, 100, 20, hWnd, ( HMENU )BTN_UPDATE_PICTURE, NULL, NULL );
			g_hWnd_remove_picture = _CreateWindowW( WC_BUTTON, ST_Remove_Picture, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 420, 170, 100, 20, hWnd, ( HMENU )BTN_REMOVE_PICTURE, NULL, NULL );

			g_hWnd_contact_action = _CreateWindowW( WC_BUTTON, ST_Add_Contact, WS_CHILD | WS_TABSTOP | WS_VISIBLE, ( rc.right - rc.left - ( _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ) ) ) / 2, rc.bottom - 32, _GetSystemMetrics( SM_CXMINTRACK ) - ( 2 * _GetSystemMetrics( SM_CXSIZEFRAME ) ), 23, hWnd, ( HMENU )BTN_EXIT_INFO, NULL, NULL );

			_SendMessageW( g_hWnd_home_phone_number, EM_LIMITTEXT, 15, 0 );
			_SendMessageW( g_hWnd_cell_phone_number, EM_LIMITTEXT, 15, 0 );
			_SendMessageW( g_hWnd_office_phone_number, EM_LIMITTEXT, 15, 0 );
			_SendMessageW( g_hWnd_other_phone_number, EM_LIMITTEXT, 15, 0 );
			_SendMessageW( g_hWnd_work_phone_number, EM_LIMITTEXT, 15, 0 );
			_SendMessageW( g_hWnd_fax_number, EM_LIMITTEXT, 15, 0 );


			_SendMessageW( g_hWnd_static1, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_title, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static2, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_first_name, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static3, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_last_name, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static4, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_nickname, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static5, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_company, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static6, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_job_title, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static7, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_department, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static8, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_profession, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static9, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_home_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static10, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_cell_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static11, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_office_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static12, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_other_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static13, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_work_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static14, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_fax_number, WM_SETFONT, ( WPARAM )hFont, 0 );


			_SendMessageW( g_hWnd_static15, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_email_address, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static16, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_web_page, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static17, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_update_picture, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_remove_picture, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_contact_action, WM_SETFONT, ( WPARAM )hFont, 0 );

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The item we want to draw is our listview.
			if ( dis->CtlType == ODT_STATIC && dis->hwndItem == g_hWnd_static_picture )
			{
				// Create and save a bitmap in memory to paint to.
				HDC hdcMem = _CreateCompatibleDC( dis->hDC );
				HBITMAP hbm = _CreateCompatibleBitmap( dis->hDC, dis->rcItem.right - dis->rcItem.left, dis->rcItem.bottom - dis->rcItem.top );
				HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
				_DeleteObject( ohbm );
				_DeleteObject( hbm );

				HBRUSH hBrush = _GetSysColorBrush( COLOR_WINDOW );//( HBRUSH )_GetStockObject( WHITE_BRUSH );//GetClassLong( _GetParent( hWnd ), GCL_HBRBACKGROUND );
				_FillRect( hdcMem, &dis->rcItem, hBrush );

				// Load the image and center it in the frame bitmap.
				HDC hdcMem2 = _CreateCompatibleDC( dis->hDC );
				ohbm = ( HBITMAP )_SelectObject( hdcMem2, g_hbm_picture );
				_DeleteObject( ohbm );
				_BitBlt( hdcMem, ( ( dis->rcItem.right - dis->rcItem.left ) - g_picture_width ) / 2, ( ( dis->rcItem.bottom - dis->rcItem.top ) - g_picture_height ) / 2, dis->rcItem.right - dis->rcItem.left, dis->rcItem.bottom - dis->rcItem.top, hdcMem2, 0, 0, SRCCOPY );
				_DeleteDC( hdcMem2 );

				// Draw everything to the device.
				_BitBlt( dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right - dis->rcItem.left, dis->rcItem.bottom - dis->rcItem.top, hdcMem, 0, 0, SRCCOPY );
	
				// Delete our back buffer.
				_DeleteDC( hdcMem );
			}
			return TRUE;
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case BTN_EXIT_INFO:
				{
					if ( contact_update_in_progress != UPDATE_END )
					{
						_EnableWindow( g_hWnd_contact_action, FALSE );	// Disable the action button.

						contact_update_in_progress = UPDATE_FAIL;	// Stop the update.

						break;
					}

					bool info_changed = false;	// If this is true, then we'll update the contact.

					contactinfo *ci = ( contactinfo * )GlobalAlloc( GMEM_FIXED, sizeof( contactinfo ) );
					initialize_contactinfo( &ci );

					int cfg_val_length = 0;
					char *utf8_val = NULL;



					unsigned int text_length = _SendMessageW( g_hWnd_title, WM_GETTEXTLENGTH, 0, 0 );

					wchar_t *text_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( text_length + 1 ) );
					_SendMessageW( g_hWnd_title, WM_GETTEXT, text_length + 1, ( LPARAM )text_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->title == NULL || ( edit_ci->title != NULL && lstrcmpW( text_val, edit_ci->title ) ) ) != 0 ) )
					{
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, NULL, 0, NULL, NULL );
						utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, utf8_val, cfg_val_length, NULL, NULL );

						ci->contact.title = utf8_val;
						ci->title = text_val;

						info_changed = true;
					}
					else
					{
						GlobalFree( text_val );
					}



					text_length = _SendMessageW( g_hWnd_first_name, WM_GETTEXTLENGTH, 0, 0 );

					text_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( text_length + 1 ) );
					_SendMessageW( g_hWnd_first_name, WM_GETTEXT, text_length + 1, ( LPARAM )text_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->first_name == NULL || ( edit_ci->first_name != NULL && lstrcmpW( text_val, edit_ci->first_name ) ) ) != 0 ) )
					{
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, NULL, 0, NULL, NULL );
						utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, utf8_val, cfg_val_length, NULL, NULL );

						ci->contact.first_name = utf8_val;
						ci->first_name = text_val;

						info_changed = true;
					}
					else
					{
						GlobalFree( text_val );
					}
				



					text_length = _SendMessageW( g_hWnd_last_name, WM_GETTEXTLENGTH, 0, 0 );

					text_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( text_length + 1 ) );
					_SendMessageW( g_hWnd_last_name, WM_GETTEXT, text_length + 1, ( LPARAM )text_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->last_name == NULL || ( edit_ci->last_name != NULL && lstrcmpW( text_val, edit_ci->last_name ) ) ) != 0 ) )
					{
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, NULL, 0, NULL, NULL );
						utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, utf8_val, cfg_val_length, NULL, NULL );

						ci->contact.last_name = utf8_val;
						ci->last_name = text_val;

						info_changed = true;
					}
					else
					{
						GlobalFree( text_val );
					}




					text_length = _SendMessageW( g_hWnd_nickname, WM_GETTEXTLENGTH, 0, 0 );

					text_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( text_length + 1 ) );
					_SendMessageW( g_hWnd_nickname, WM_GETTEXT, text_length + 1, ( LPARAM )text_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->nickname == NULL || ( edit_ci->nickname != NULL && lstrcmpW( text_val, edit_ci->nickname ) ) ) != 0 ) )
					{
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, NULL, 0, NULL, NULL );
						utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, utf8_val, cfg_val_length, NULL, NULL );

						ci->contact.nickname = utf8_val;
						ci->nickname = text_val;

						info_changed = true;
					}
					else
					{
						GlobalFree( text_val );
					}




					text_length = _SendMessageW( g_hWnd_company, WM_GETTEXTLENGTH, 0, 0 );

					text_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( text_length + 1 ) );
					_SendMessageW( g_hWnd_company, WM_GETTEXT, text_length + 1, ( LPARAM )text_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->business_name == NULL || ( edit_ci->business_name != NULL && lstrcmpW( text_val, edit_ci->business_name ) ) ) != 0 ) )
					{
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, NULL, 0, NULL, NULL );
						utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, utf8_val, cfg_val_length, NULL, NULL );

						ci->contact.business_name = utf8_val;
						ci->business_name = text_val;

						info_changed = true;
					}
					else
					{
						GlobalFree( text_val );
					}



					text_length = _SendMessageW( g_hWnd_job_title, WM_GETTEXTLENGTH, 0, 0 );

					text_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( text_length + 1 ) );
					_SendMessageW( g_hWnd_job_title, WM_GETTEXT, text_length + 1, ( LPARAM )text_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->designation == NULL || ( edit_ci->designation != NULL && lstrcmpW( text_val, edit_ci->designation ) ) ) != 0 ) )
					{
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, NULL, 0, NULL, NULL );
						utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, utf8_val, cfg_val_length, NULL, NULL );

						ci->contact.designation = utf8_val;
						ci->designation = text_val;

						info_changed = true;
					}
					else
					{
						GlobalFree( text_val );
					}



					text_length = _SendMessageW( g_hWnd_department, WM_GETTEXTLENGTH, 0, 0 );

					text_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( text_length + 1 ) );
					_SendMessageW( g_hWnd_department, WM_GETTEXT, text_length + 1, ( LPARAM )text_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->department == NULL || ( edit_ci->department != NULL && lstrcmpW( text_val, edit_ci->department ) ) ) != 0 ) )
					{
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, NULL, 0, NULL, NULL );
						utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, utf8_val, cfg_val_length, NULL, NULL );

						ci->contact.department = utf8_val;
						ci->department = text_val;

						info_changed = true;
					}
					else
					{
						GlobalFree( text_val );
					}



					text_length = _SendMessageW( g_hWnd_profession, WM_GETTEXTLENGTH, 0, 0 );

					text_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( text_length + 1 ) );
					_SendMessageW( g_hWnd_profession, WM_GETTEXT, text_length + 1, ( LPARAM )text_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->category == NULL || ( edit_ci->category != NULL && lstrcmpW( text_val, edit_ci->category ) ) ) != 0 ) )
					{
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, NULL, 0, NULL, NULL );
						utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, utf8_val, cfg_val_length, NULL, NULL );

						ci->contact.category = utf8_val;
						ci->category = text_val;

						info_changed = true;
					}
					else
					{
						GlobalFree( text_val );
					}



					text_length = _SendMessageW( g_hWnd_email_address, WM_GETTEXTLENGTH, 0, 0 );

					text_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( text_length + 1 ) );
					_SendMessageW( g_hWnd_email_address, WM_GETTEXT, text_length + 1, ( LPARAM )text_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->email_address == NULL || ( edit_ci->email_address != NULL && lstrcmpW( text_val, edit_ci->email_address ) ) ) != 0 ) )
					{
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, NULL, 0, NULL, NULL );
						utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, utf8_val, cfg_val_length, NULL, NULL );

						ci->contact.email_address = utf8_val;
						ci->email_address = text_val;

						info_changed = true;
					}
					else
					{
						GlobalFree( text_val );
					}



					text_length = _SendMessageW( g_hWnd_web_page, WM_GETTEXTLENGTH, 0, 0 );

					text_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( text_length + 1 ) );
					_SendMessageW( g_hWnd_web_page, WM_GETTEXT, text_length + 1, ( LPARAM )text_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->web_page == NULL || ( edit_ci->web_page != NULL && lstrcmpW( text_val, edit_ci->web_page ) ) ) != 0 ) )
					{
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, NULL, 0, NULL, NULL );
						utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
						cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, text_val, -1, utf8_val, cfg_val_length, NULL, NULL );

						ci->contact.web_page = utf8_val;
						ci->web_page = text_val;

						info_changed = true;
					}
					else
					{
						GlobalFree( text_val );
					}



					text_length = _SendMessageA( g_hWnd_home_phone_number, WM_GETTEXTLENGTH, 0, 0 );

					utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( text_length + 1 ) );
					_SendMessageA( g_hWnd_home_phone_number, WM_GETTEXT, text_length + 1, ( LPARAM )utf8_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->home_phone_number == NULL || ( edit_ci->contact.home_phone_number != NULL && lstrcmpA( utf8_val, edit_ci->contact.home_phone_number ) ) ) != 0 ) )
					{
						ci->contact.home_phone_number = utf8_val;
						//ci->home_phone_number;	// Set if added/updated

						info_changed = true;
					}
					else
					{
						GlobalFree( utf8_val );
					}



					text_length = _SendMessageA( g_hWnd_cell_phone_number, WM_GETTEXTLENGTH, 0, 0 );

					utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( text_length + 1 ) );
					_SendMessageA( g_hWnd_cell_phone_number, WM_GETTEXT, text_length + 1, ( LPARAM )utf8_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->cell_phone_number == NULL || ( edit_ci->contact.cell_phone_number != NULL && lstrcmpA( utf8_val, edit_ci->contact.cell_phone_number ) ) ) != 0 ) )
					{
						ci->contact.cell_phone_number = utf8_val;
						//ci->cell_phone_number;	// Set if added/updated

						info_changed = true;
					}
					else
					{
						GlobalFree( utf8_val );
					}



					text_length = _SendMessageA( g_hWnd_office_phone_number, WM_GETTEXTLENGTH, 0, 0 );

					utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( text_length + 1 ) );
					_SendMessageA( g_hWnd_office_phone_number, WM_GETTEXT, text_length + 1, ( LPARAM )utf8_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->office_phone_number == NULL || ( edit_ci->contact.office_phone_number != NULL && lstrcmpA( utf8_val, edit_ci->contact.office_phone_number ) ) ) != 0 ) )
					{
						ci->contact.office_phone_number = utf8_val;
						//ci->office_phone_number;	// Set if added/updated

						info_changed = true;
					}
					else
					{
						GlobalFree( utf8_val );
					}



					text_length = _SendMessageA( g_hWnd_other_phone_number, WM_GETTEXTLENGTH, 0, 0 );

					utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( text_length + 1 ) );
					_SendMessageA( g_hWnd_other_phone_number, WM_GETTEXT, text_length + 1, ( LPARAM )utf8_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->other_phone_number == NULL || ( edit_ci->contact.other_phone_number != NULL && lstrcmpA( utf8_val, edit_ci->contact.other_phone_number ) ) ) != 0 ) )
					{
						ci->contact.other_phone_number = utf8_val;
						//ci->other_phone_number;	// Set if added/updated

						info_changed = true;
					}
					else
					{
						GlobalFree( utf8_val );
					}



					text_length = _SendMessageA( g_hWnd_work_phone_number, WM_GETTEXTLENGTH, 0, 0 );

					utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( text_length + 1 ) );
					_SendMessageA( g_hWnd_work_phone_number, WM_GETTEXT, text_length + 1, ( LPARAM )utf8_val );

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->work_phone_number == NULL || ( edit_ci->contact.work_phone_number != NULL && lstrcmpA( utf8_val, edit_ci->contact.work_phone_number ) ) ) != 0 ) )
					{
						ci->contact.work_phone_number = utf8_val;
						//ci->work_phone_number;	// Set if added/updated

						info_changed = true;
					}
					else
					{
						GlobalFree( utf8_val );
					}



					text_length = _SendMessageA( g_hWnd_fax_number, WM_GETTEXTLENGTH, 0, 0 );

					utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( text_length + 1 ) );
					_SendMessageA( g_hWnd_fax_number, WM_GETTEXT, text_length + 1, ( LPARAM )utf8_val );;

					if ( edit_ci == NULL || ( edit_ci != NULL && ( edit_ci->fax_number == NULL || ( edit_ci->contact.fax_number != NULL && lstrcmpA( utf8_val, edit_ci->contact.fax_number ) ) ) != 0 ) )
					{
						ci->contact.fax_number = utf8_val;
						//ci->fax_number;	// Set if added/updated

						info_changed = true;
					}
					else
					{
						GlobalFree( utf8_val );
					}




					bool picture_changed = false;

					// First see if we selected a picture.
					if ( g_picture_file_path != NULL )
					{
						// Then see if the contact has a picture set, and if it does, see if it's the same as the one we've selected.
						if ( edit_ci != NULL && ( edit_ci->picture_path != NULL && lstrcmpW( g_picture_file_path, edit_ci->picture_path ) == 0 ) )
						{
							// If the picture paths are the same, then free the selected one.
							GlobalFree( g_picture_file_path );

							g_picture_file_path = NULL;
						}
						else	// If we're updating (and the paths differ), or we're adding a new contact.
						{
							// Set the contact's picture path.
							ci->picture_path = GlobalStrDupW( g_picture_file_path );

							picture_changed = true;
						}
					}

					if ( edit_ci != NULL )	// Update contact.
					{
						if ( info_changed == true || picture_changed == true || remove_picture == true )
						{
							updateinfo *ui = ( updateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( updateinfo ) );
							ui->old_ci = NULL;
							ui->new_ci = NULL;
							ui->remove_picture = remove_picture;

							ui->picture_only = !info_changed;	// If info_changed is false, then only update the picture, else, update the info and/or the picture.

							ui->old_ci = edit_ci;
							ui->new_ci = ci;

							// Attempt to update this contact.
							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, UpdateContactInformation, ( void * )ui, 0, NULL ) );
						}
						else
						{
							free_contactinfo( &ci );
						}
					}
					else	// Add contact.
					{
						manageinfo *mi = ( manageinfo * )GlobalAlloc( GMEM_FIXED, sizeof( manageinfo ) );
						mi->ci = ci;			// This will be freed if it can't be added.
						mi->manage_type = 1;	// Attempt to add this contact.

						// mi is freed in the ManageContactInformation thread.
						CloseHandle( ( HANDLE )_CreateThread( NULL, 0, ManageContactInformation, ( void * )mi, 0, NULL ) );
					}
				}
				break;

				case BTN_REMOVE_PICTURE:
				{
					if ( edit_ci != NULL )	// Update contact.
					{
						// Only set it to remove if the contact has a picture set.
						if ( edit_ci->picture_path != NULL )
						{
							remove_picture = true;
						}

						if ( g_hbm_picture != NULL )
						{
							_DeleteObject( g_hbm_picture );
							g_hbm_picture = NULL;

							_InvalidateRect( g_hWnd_static_picture, NULL, TRUE );
						}

						// Disable the remove button.
						_EnableWindow( g_hWnd_remove_picture, FALSE );
					}
				}
				break;

				case BTN_UPDATE_PICTURE:
				{
					wchar_t *picture_file_path = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof ( wchar_t ) * MAX_PATH );
					if ( edit_ci != NULL && edit_ci->picture_path != NULL )
					{
						_wcsncpy_s( picture_file_path, MAX_PATH, edit_ci->picture_path, MAX_PATH );
						picture_file_path[ MAX_PATH - 1 ] = 0;	// Sanity.
					}
					else
					{
						_memzero( picture_file_path, sizeof ( wchar_t ) * MAX_PATH );
					}

					OPENFILENAME ofn;
					_memzero( &ofn, sizeof( OPENFILENAME ) );
					ofn.lStructSize = sizeof( OPENFILENAME );
					ofn.hwndOwner = hWnd;
					ofn.lpstrFilter = L"Bitmap Files (*.bmp;*.dib)\0*.bmp;*.dib\0" \
									  L"JPEG (*.jpg;*.jpeg;*.jpe;*.jfif)\0*.jpg;*.jpeg;*.jpe;*.jfif\0" \
									  L"GIF (*.gif)\0*.gif\0" \
									  L"PNG (*.png)\0*.png\0" \
									  L"All Files (*.*)\0*.*\0";
					ofn.nFilterIndex = 2;	// Default filter is for JPEGs.
					ofn.lpstrTitle = ST_Contact_Picture;
					ofn.lpstrFile = picture_file_path;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_READONLY;

					if ( _GetOpenFileNameW( &ofn ) )
					{
						// Update the selected picture path if it's already set.
						if ( g_picture_file_path != NULL )
						{
							GlobalFree( g_picture_file_path );
						}

						g_picture_file_path = picture_file_path;

						remove_picture = false;	// We have a new picture, so don't remove it.

						if ( g_hbm_picture != NULL )
						{
							_DeleteObject( g_hbm_picture );
							g_hbm_picture = NULL;
						}

						g_picture_height = 0;
						g_picture_width = 0;

						bool convert = true;
						if ( gdiplus_state == GDIPLUS_STATE_SHUTDOWN )
						{
							#ifndef GDIPLUS_USE_STATIC_LIB
								convert = InitializeGDIPlus();
							#else
								StartGDIPlus();
							#endif
						}

						if ( convert == true )
						{
							g_hbm_picture = ImageToBitmap( g_picture_file_path, g_picture_height, g_picture_width );
						}

						_InvalidateRect( g_hWnd_static_picture, NULL, TRUE );

						_EnableWindow( g_hWnd_remove_picture, TRUE );
					}
					else
					{
						GlobalFree( picture_file_path );
						picture_file_path = NULL;
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_PROPAGATE:
		{
			if ( LOWORD( wParam ) == CW_MODIFY )	// Add/Update contact.
			{
				// See if there are no updates in progress.
				if ( contact_update_in_progress == UPDATE_END )
				{
					// If we have a window open already and want to switch (add/update)
					// Then clean up and reset any variables that have been set.

					if ( g_hbm_picture != NULL )
					{
						_DeleteObject( g_hbm_picture );
						g_hbm_picture = NULL;
					}

					g_picture_height = 0;
					g_picture_width = 0;

					if ( g_picture_file_path != NULL )
					{
						GlobalFree( g_picture_file_path );
						g_picture_file_path = NULL;
					}

					remove_picture = false;

					edit_ci = ( contactinfo * )lParam;

					if ( edit_ci != NULL )
					{
						if ( edit_ci->picture_path != NULL )
						{
							bool convert = true;
							if ( gdiplus_state == GDIPLUS_STATE_SHUTDOWN )
							{
								#ifndef GDIPLUS_USE_STATIC_LIB
									convert = InitializeGDIPlus();
								#else
									StartGDIPlus();
								#endif
							}

							if ( convert == true )
							{
								g_hbm_picture = ImageToBitmap( edit_ci->picture_path, g_picture_height, g_picture_width );
							}

							_EnableWindow( g_hWnd_remove_picture, TRUE );
						}
						else
						{
							_EnableWindow( g_hWnd_remove_picture, FALSE );
						}

						_SendMessageW( g_hWnd_first_name, WM_SETTEXT, 0, ( LPARAM )edit_ci->first_name );
						_SendMessageW( g_hWnd_last_name, WM_SETTEXT, 0, ( LPARAM )edit_ci->last_name );
						_SendMessageW( g_hWnd_nickname, WM_SETTEXT, 0, ( LPARAM )edit_ci->nickname );
						_SendMessageA( g_hWnd_home_phone_number, WM_SETTEXT, 0, ( LPARAM )edit_ci->contact.home_phone_number );
						_SendMessageA( g_hWnd_cell_phone_number, WM_SETTEXT, 0, ( LPARAM )edit_ci->contact.cell_phone_number );
						_SendMessageA( g_hWnd_office_phone_number, WM_SETTEXT, 0, ( LPARAM )edit_ci->contact.office_phone_number );
						_SendMessageA( g_hWnd_other_phone_number, WM_SETTEXT, 0, ( LPARAM )edit_ci->contact.other_phone_number );

						_SendMessageW( g_hWnd_title, WM_SETTEXT, 0, ( LPARAM )edit_ci->title );
						_SendMessageW( g_hWnd_company, WM_SETTEXT, 0, ( LPARAM )edit_ci->business_name );
						_SendMessageW( g_hWnd_job_title, WM_SETTEXT, 0, ( LPARAM )edit_ci->designation );
						_SendMessageW( g_hWnd_department, WM_SETTEXT, 0, ( LPARAM )edit_ci->department );
						_SendMessageW( g_hWnd_profession, WM_SETTEXT, 0, ( LPARAM )edit_ci->category );
						_SendMessageA( g_hWnd_work_phone_number, WM_SETTEXT, 0, ( LPARAM )edit_ci->contact.work_phone_number );
						_SendMessageA( g_hWnd_fax_number, WM_SETTEXT, 0, ( LPARAM )edit_ci->contact.fax_number );
						_SendMessageW( g_hWnd_email_address, WM_SETTEXT, 0, ( LPARAM )edit_ci->email_address );
						_SendMessageW( g_hWnd_web_page, WM_SETTEXT, 0, ( LPARAM )edit_ci->web_page );

						_SendMessageW( g_hWnd_contact_action, WM_SETTEXT, 0, ( LPARAM )ST_Update_Contact );

						_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Update_Contact );
					}
					else	// Add contact.
					{
						edit_ci = NULL;

						_EnableWindow( g_hWnd_remove_picture, FALSE );

						_SendMessageW( g_hWnd_first_name, WM_SETTEXT, 0, 0 );
						_SendMessageW( g_hWnd_last_name, WM_SETTEXT, 0, 0 );
						_SendMessageW( g_hWnd_nickname, WM_SETTEXT, 0, 0 );
						_SendMessageA( g_hWnd_home_phone_number, WM_SETTEXT, 0, 0 );
						_SendMessageA( g_hWnd_cell_phone_number, WM_SETTEXT, 0, 0 );
						_SendMessageA( g_hWnd_office_phone_number, WM_SETTEXT, 0, 0 );
						_SendMessageA( g_hWnd_other_phone_number, WM_SETTEXT, 0, 0 );

						_SendMessageW( g_hWnd_title, WM_SETTEXT, 0, 0 );
						_SendMessageW( g_hWnd_company, WM_SETTEXT, 0, 0 );
						_SendMessageW( g_hWnd_job_title, WM_SETTEXT, 0, 0 );
						_SendMessageW( g_hWnd_department, WM_SETTEXT, 0, 0 );
						_SendMessageW( g_hWnd_profession, WM_SETTEXT, 0, 0 );
						_SendMessageW( g_hWnd_work_phone_number, WM_SETTEXT, 0, 0 );
						_SendMessageW( g_hWnd_fax_number, WM_SETTEXT, 0, 0 );
						_SendMessageW( g_hWnd_email_address, WM_SETTEXT, 0, 0 );
						_SendMessageW( g_hWnd_web_page, WM_SETTEXT, 0, 0 );

						_SendMessageW( g_hWnd_contact_action, WM_SETTEXT, 0, ( LPARAM )ST_Add_Contact );

						_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Add_Contact );
					}

					_InvalidateRect( g_hWnd_static_picture, NULL, TRUE );
				}
				else	// If there is, ask to cancel it.
				{
					if ( _MessageBoxW( hWnd, ST_PROMPT_contact_update_in_progress, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
					{
						_EnableWindow( g_hWnd_contact_action, FALSE );	// Disable the action button.

						contact_update_in_progress = UPDATE_FAIL;	// Stop the update.
					}
				}

				_ShowWindow( hWnd, SW_SHOWNORMAL );
			}
			else if ( LOWORD( wParam ) == CW_UPDATE )	// Sent from the UpdateContactInformation thread when we start updating.
			{
				// TRUE if we've started to update the contact.
				if ( HIWORD( wParam ) == UPDATE_BEGIN )
				{
					_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Updating_Contact );

					_SendMessageW( g_hWnd_contact_action, WM_SETTEXT, 0, ( LPARAM )ST_Cancel_Update );

					// Allow us to cancel.
					contact_update_in_progress = UPDATE_BEGIN;
				}
				else if ( HIWORD( wParam ) == UPDATE_END )	// Sent from the UpdateContactInformation thread if we've finished updating, or canceled the update.
				{
					// If the UpdateContactInformation thread exited with contact_update_in_progress set to UPDATE_FAIL, then we canceled the update.
					if ( contact_update_in_progress == UPDATE_FAIL )
					{
						_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Update_Contact___Canceled );
					}
					else	// If it exited with contact_update_in_progress set to UPDATE_BEGIN, then we finished updating.
					{
						_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Update_Contact___Complete );

						// If the update was successful, then free the local picture path (since it's now been set).
						if ( g_picture_file_path != NULL )
						{
							GlobalFree( g_picture_file_path );
							g_picture_file_path = NULL;
						}
					}

					remove_picture = false;
					
					_EnableWindow( g_hWnd_contact_action, TRUE );	// Enable the action button if it had been disabled because of a cancel request.
					_SendMessageW( g_hWnd_contact_action, WM_SETTEXT, 0, ( LPARAM )ST_Update_Contact );

					// Reset the state to allow us to update again.
					contact_update_in_progress = UPDATE_END;
				}
				else if ( HIWORD( wParam ) == UPDATE_PROGRESS )	// Sent from the UploadMedia function in the UpdateContactInformation thread with upload information.
				{
					// The upload info contains the file size and amount transferred.
					UPLOAD_INFO *upload_info = ( UPLOAD_INFO * )lParam;

					// the ntdll.dll printf functions don't appear to support floating point values. This is a way around it.

					// Multiply the floating point division by 1000%.
					// This leaves us with an integer in which the last digit will represent the decimal value.
					float f_percentage = 1000.0f * ( float )upload_info->sent / ( float )upload_info->size;
					int i_percentage = 0;
					_asm
					{
						fld f_percentage;	//; Load the floating point value onto the FPU stack.
						fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
					}

					// Get the last digit (decimal value).
					int remainder = i_percentage % 10;
					// Divide the integer by (10%) to get it back in range of 0% to 100%.
					i_percentage /= 10;

					wchar_t title[ 256 ];
					__snwprintf( title, 256, L"Updating Contact - (%d.%1d%%) %lu / %lu bytes", i_percentage, remainder, upload_info->sent, upload_info->size );

					_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )title );
				}
				else if ( HIWORD( wParam ) == UPDATE_FAIL )	// Sent from the UpdateContactInformation thread if either the update or the picture upload failed.
				{
					_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_Update_Contact___Failed );

					_EnableWindow( g_hWnd_contact_action, TRUE );	// Enable the action button if it had been disabled because of a cancel request.
					_SendMessageW( g_hWnd_contact_action, WM_SETTEXT, 0, ( LPARAM )ST_Update_Contact );

					// Reset the state to allow us to update again.
					contact_update_in_progress = UPDATE_END;
				}
			}
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
			// If an update is in progress, ask if the user wants to cancel before closing the window.
			if ( contact_update_in_progress == UPDATE_BEGIN )
			{
				if ( _MessageBoxW( hWnd, ST_PROMPT_contact_update_in_progress, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
				{
					_EnableWindow( g_hWnd_contact_action, FALSE );	// Disable the action button.

					contact_update_in_progress = UPDATE_FAIL;	// Stop the update.
				}

				break;
			}

			_DestroyWindow( hWnd );
		}
		break;

		case WM_DESTROY:
		{
			contact_update_in_progress = UPDATE_END;
			remove_picture = false;

			if ( g_hbm_picture != NULL )
			{
				_DeleteObject( g_hbm_picture );
				g_hbm_picture = NULL;
			}

			g_picture_height = 0;
			g_picture_width = 0;

			if ( g_picture_file_path != NULL )
			{
				GlobalFree( g_picture_file_path );
				g_picture_file_path = NULL;
			}

			g_hWnd_contact = NULL;
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
