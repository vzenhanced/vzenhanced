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
#include "file_operations.h"

#define BTN_SAVE_COLUMNS	1000
#define BTN_CANCEL_COLUMNS	1001
#define BTN_APPLY_COLUMNS	1002

#define BTN_CHANGE_STATE	1003

HWND g_hWnd_column_tab = NULL;

HWND g_hWnd_chk_call_log_num = NULL;
HWND g_hWnd_chk_caller_id = NULL;
HWND g_hWnd_chk_date_and_time = NULL;
HWND g_hWnd_chk_forwarded = NULL;
HWND g_hWnd_chk_forwarding_phone_number = NULL;
HWND g_hWnd_chk_ignored = NULL;
HWND g_hWnd_chk_phone_number = NULL;
HWND g_hWnd_chk_reference = NULL;
HWND g_hWnd_chk_sent_to_phone_number = NULL;

HWND g_hWnd_chk_contact_list_num = NULL;
HWND g_hWnd_chk_cell_phone = NULL;
HWND g_hWnd_chk_company = NULL;
HWND g_hWnd_chk_department = NULL;
HWND g_hWnd_chk_email_address = NULL;
HWND g_hWnd_chk_fax_number = NULL;
HWND g_hWnd_chk_first_name = NULL;
HWND g_hWnd_chk_home_phone = NULL;
HWND g_hWnd_chk_job_title = NULL;
HWND g_hWnd_chk_last_name = NULL;
HWND g_hWnd_chk_nickname = NULL;
HWND g_hWnd_chk_office_phone = NULL;
HWND g_hWnd_chk_other_phone = NULL;
HWND g_hWnd_chk_profession = NULL;
HWND g_hWnd_chk_title = NULL;
HWND g_hWnd_chk_web_page = NULL;
HWND g_hWnd_chk_work_phone_number = NULL;

HWND g_hWnd_chk_ignored_num = NULL;
HWND g_hWnd_chk_ignored_phone = NULL;
HWND g_hWnd_chk_ignored_total_calls = NULL;

HWND g_hWnd_chk_forwardlist_num = NULL;
HWND g_hWnd_chk_forwardlist_from_phone = NULL;
HWND g_hWnd_chk_forwardlist_to_phone = NULL;
HWND g_hWnd_chk_forwardlist_total_calls = NULL;

HWND g_hWnd_apply_columns = NULL;

HWND g_hWnd_calllog_columns = NULL;
HWND g_hWnd_contactlist_columns = NULL;
HWND g_hWnd_forwardlist_columns = NULL;
HWND g_hWnd_ignorelist_columns = NULL;

bool columns_state_changed = false;

LRESULT CALLBACK CallLogColumnsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			HWND hWnd_static = _CreateWindowW( WC_STATIC, ST_Show_columns_, WS_CHILD | WS_VISIBLE, 0, 0, 120, 20, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_call_log_num = _CreateWindowW( WC_BUTTON, ST_Entry_NUM, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 20, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_caller_id = _CreateWindowW( WC_BUTTON, ST_Caller_ID, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 40, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_date_and_time = _CreateWindowW( WC_BUTTON, ST_CLL_Date_and_Time, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 60, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_forwarded = _CreateWindowW( WC_BUTTON, ST_CLL_Forward, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 80, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_forwarding_phone_number = _CreateWindowW( WC_BUTTON, ST_CLL_Forwarded_to, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 100, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_ignored = _CreateWindowW( WC_BUTTON, ST_CLL_Ignore, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 120, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_phone_number = _CreateWindowW( WC_BUTTON, ST_Phone_Number, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 125, 20, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_reference = _CreateWindowW( WC_BUTTON, ST_CLL_Reference, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 125, 40, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_sent_to_phone_number = _CreateWindowW( WC_BUTTON, ST_CLL_Sent_to, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 125, 60, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );

			_SendMessageW( hWnd_static, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_call_log_num, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_caller_id, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_date_and_time, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_forwarded, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_forwarding_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_ignored, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_reference, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_sent_to_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_call_log_num, BM_SETCHECK, ( cfg_column_order1 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_caller_id, BM_SETCHECK, ( cfg_column_order2 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_date_and_time, BM_SETCHECK, ( cfg_column_order3 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_forwarded, BM_SETCHECK, ( cfg_column_order4 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_forwarding_phone_number, BM_SETCHECK, ( cfg_column_order5 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_ignored, BM_SETCHECK, ( cfg_column_order6 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_phone_number, BM_SETCHECK, ( cfg_column_order7 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_reference, BM_SETCHECK, ( cfg_column_order8 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_sent_to_phone_number, BM_SETCHECK, ( cfg_column_order9 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );

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
				case BTN_CHANGE_STATE:
				{
					columns_state_changed = true;
					_EnableWindow( g_hWnd_apply_columns, TRUE );
				}
				break;
			}

			return 0;
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

LRESULT CALLBACK ContactListColumnsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			HWND hWnd_static = _CreateWindowW( WC_STATIC, ST_Show_columns_, WS_CHILD | WS_VISIBLE, 0, 0, 120, 20, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_contact_list_num = _CreateWindowW( WC_BUTTON, ST_Entry_NUM, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 20, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_cell_phone = _CreateWindowW( WC_BUTTON, ST_CL_Cell_Phone_Number, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 40, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_company = _CreateWindowW( WC_BUTTON, ST_CL_Company, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 60, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_department = _CreateWindowW( WC_BUTTON, ST_CL_Department, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 80, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_email_address = _CreateWindowW( WC_BUTTON, ST_CL_Email_Address, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 100, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_fax_number = _CreateWindowW( WC_BUTTON, ST_CL_Fax_Number, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 120, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_first_name = _CreateWindowW( WC_BUTTON, ST_CL_First_Name, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 125, 20, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_home_phone = _CreateWindowW( WC_BUTTON, ST_CL_Home_Phone_Number, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 125, 40, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			
			g_hWnd_chk_job_title = _CreateWindowW( WC_BUTTON, ST_CL_Job_Title, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 125, 60, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_last_name = _CreateWindowW( WC_BUTTON, ST_CL_Last_Name, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 125, 80, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_nickname = _CreateWindowW( WC_BUTTON, ST_CL_Nickname, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 125, 100, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_office_phone = _CreateWindowW( WC_BUTTON, ST_CL_Office_Phone_Number, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 125, 120, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_other_phone = _CreateWindowW( WC_BUTTON, ST_CL_Other_Phone_Number, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 250, 20, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_profession = _CreateWindowW( WC_BUTTON, ST_CL_Profession, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 250, 40, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_title = _CreateWindowW( WC_BUTTON, ST_CL_Title, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 250, 60, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_web_page = _CreateWindowW( WC_BUTTON, ST_CL_Web_Page, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 250, 80, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_work_phone_number = _CreateWindowW( WC_BUTTON, ST_CL_Work_Phone_Number, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 250, 100, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );

			_SendMessageW( hWnd_static, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_contact_list_num, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_first_name, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_last_name, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_nickname, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_cell_phone, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_home_phone, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_office_phone, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_other_phone, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_title, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_company, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_job_title, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_department, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_profession, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_work_phone_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_fax_number, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_email_address, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_web_page, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_contact_list_num, BM_SETCHECK, ( cfg_column2_order1 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_cell_phone, BM_SETCHECK, ( cfg_column2_order2 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_company, BM_SETCHECK, ( cfg_column2_order3 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_department, BM_SETCHECK, ( cfg_column2_order4 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_email_address, BM_SETCHECK, ( cfg_column2_order5 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_fax_number, BM_SETCHECK, ( cfg_column2_order6 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_first_name, BM_SETCHECK, ( cfg_column2_order7 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_home_phone, BM_SETCHECK, ( cfg_column2_order8 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_job_title, BM_SETCHECK, ( cfg_column2_order9 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_last_name, BM_SETCHECK, ( cfg_column2_order10 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_nickname, BM_SETCHECK, ( cfg_column2_order11 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_office_phone, BM_SETCHECK, ( cfg_column2_order12 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_other_phone, BM_SETCHECK, ( cfg_column2_order13 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_profession, BM_SETCHECK, ( cfg_column2_order14 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_title, BM_SETCHECK, ( cfg_column2_order15 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_web_page, BM_SETCHECK, ( cfg_column2_order16 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_work_phone_number, BM_SETCHECK, ( cfg_column2_order17 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );

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
				case BTN_CHANGE_STATE:
				{
					columns_state_changed = true;
					_EnableWindow( g_hWnd_apply_columns, TRUE );
				}
				break;
			}

			return 0;
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

LRESULT CALLBACK ForwardListColumnsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			HWND hWnd_static = _CreateWindowW( WC_STATIC, ST_Show_columns_, WS_CHILD | WS_VISIBLE, 0, 0, 120, 20, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_forwardlist_num = _CreateWindowW( WC_BUTTON, ST_Entry_NUM, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 20, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_forwardlist_to_phone = _CreateWindowW( WC_BUTTON, ST_FL_Forward_to, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 40, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_forwardlist_from_phone = _CreateWindowW( WC_BUTTON, ST_Phone_Number, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 60, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_forwardlist_total_calls = _CreateWindowW( WC_BUTTON, ST_Total_Calls, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 80, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );

			_SendMessageW( hWnd_static, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_forwardlist_num, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_forwardlist_to_phone, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_forwardlist_from_phone, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_forwardlist_total_calls, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_forwardlist_num, BM_SETCHECK, ( cfg_column3_order1 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_forwardlist_to_phone, BM_SETCHECK, ( cfg_column3_order2 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_forwardlist_from_phone, BM_SETCHECK, ( cfg_column3_order3 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_forwardlist_total_calls, BM_SETCHECK, ( cfg_column3_order4 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );

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
				case BTN_CHANGE_STATE:
				{
					columns_state_changed = true;
					_EnableWindow( g_hWnd_apply_columns, TRUE );
				}
				break;
			}

			return 0;
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

LRESULT CALLBACK IgnoreListColumnsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			HWND hWnd_static = _CreateWindowW( WC_STATIC, ST_Show_columns_, WS_CHILD | WS_VISIBLE, 0, 0, 120, 20, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_ignored_num = _CreateWindowW( WC_BUTTON, ST_Entry_NUM, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 20, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_ignored_phone = _CreateWindowW( WC_BUTTON, ST_Phone_Number, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 40, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );
			g_hWnd_chk_ignored_total_calls = _CreateWindowW( WC_BUTTON, ST_Total_Calls, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 60, 120, 20, hWnd, ( HMENU )BTN_CHANGE_STATE, NULL, NULL );

			_SendMessageW( hWnd_static, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_ignored_num, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_ignored_phone, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_ignored_total_calls, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_ignored_num, BM_SETCHECK, ( cfg_column4_order1 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_ignored_phone, BM_SETCHECK, ( cfg_column4_order2 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_ignored_total_calls, BM_SETCHECK, ( cfg_column4_order3 != -1 ? BST_CHECKED : BST_UNCHECKED ), 0 );

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
				case BTN_CHANGE_STATE:
				{
					columns_state_changed = true;
					_EnableWindow( g_hWnd_apply_columns, TRUE );
				}
				break;
			}

			return 0;
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


LRESULT CALLBACK ColumnsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_column_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, WC_TABCONTROL, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			g_hWnd_calllog_columns = _CreateWindowExW( WS_EX_CONTROLPARENT, L"calllog_columns", NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 35, rc.right - 50, rc.bottom - 95, g_hWnd_column_tab, NULL, NULL, NULL );
			g_hWnd_contactlist_columns = _CreateWindowExW( WS_EX_CONTROLPARENT, L"contactlist_columns", NULL, WS_CHILD | WS_TABSTOP, 15, 35, rc.right - 50, rc.bottom - 95, g_hWnd_column_tab, NULL, NULL, NULL );
			g_hWnd_forwardlist_columns = _CreateWindowExW( WS_EX_CONTROLPARENT, L"forwardlist_columns", NULL, WS_CHILD | WS_TABSTOP, 15, 35, rc.right - 50, rc.bottom - 95, g_hWnd_column_tab, NULL, NULL, NULL );
			g_hWnd_ignorelist_columns = _CreateWindowExW( WS_EX_CONTROLPARENT, L"ignorelist_columns", NULL, WS_CHILD | WS_TABSTOP, 15, 35, rc.right - 50, rc.bottom - 95, g_hWnd_column_tab, NULL, NULL, NULL );

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_PARAM | TCIF_TEXT;	// The tab will have text and an lParam value.
			ti.pszText = ( LPWSTR )ST_Call_Log;
			ti.lParam = ( LPARAM )g_hWnd_calllog_columns;
			_SendMessageW( g_hWnd_column_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Contact_List;
			ti.lParam = ( LPARAM )g_hWnd_contactlist_columns;
			_SendMessageW( g_hWnd_column_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Forward_List;
			ti.lParam = ( LPARAM )g_hWnd_forwardlist_columns;
			_SendMessageW( g_hWnd_column_tab, TCM_INSERTITEM, 2, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Ignore_List;
			ti.lParam = ( LPARAM )g_hWnd_ignorelist_columns;
			_SendMessageW( g_hWnd_column_tab, TCM_INSERTITEM, 3, ( LPARAM )&ti );	// Insert a new tab at the end.
			
			HWND g_hWnd_ok_columns = _CreateWindowW( WC_BUTTON, ST_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 260, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_SAVE_COLUMNS, NULL, NULL );
			HWND g_hWnd_cancel_columns = _CreateWindowW( WC_BUTTON, ST_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 175, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_CANCEL_COLUMNS, NULL, NULL );
			g_hWnd_apply_columns = _CreateWindowW( WC_BUTTON, ST_Apply, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, rc.right - 90, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_APPLY_COLUMNS, NULL, NULL );

			_SendMessageW( g_hWnd_column_tab, WM_SETFONT, ( WPARAM )hFont, 0 );		

			_SendMessageW( g_hWnd_ok_columns, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_cancel_columns, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_apply_columns, WM_SETFONT, ( WPARAM )hFont, 0 );

			return 0;
		}

		/*case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;*/

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case IDOK:
				case BTN_SAVE_COLUMNS:
				case BTN_APPLY_COLUMNS:
				{
					if ( columns_state_changed == false )
					{
						_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
						break;
					}

					bool proceed = false;

					bool column1_array[ NUM_COLUMNS ];
					column1_array[ 0 ] = ( _SendMessageW( g_hWnd_chk_call_log_num, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column1_array[ 1 ] = ( _SendMessageW( g_hWnd_chk_caller_id, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column1_array[ 2 ] = ( _SendMessageW( g_hWnd_chk_date_and_time, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column1_array[ 3 ] = ( _SendMessageW( g_hWnd_chk_forwarded, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column1_array[ 4 ] = ( _SendMessageW( g_hWnd_chk_forwarding_phone_number, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column1_array[ 5 ] = ( _SendMessageW( g_hWnd_chk_ignored, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column1_array[ 6 ] = ( _SendMessageW( g_hWnd_chk_phone_number, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column1_array[ 7 ] = ( _SendMessageW( g_hWnd_chk_reference, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column1_array[ 8 ] = ( _SendMessageW( g_hWnd_chk_sent_to_phone_number, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					for ( char i = 0; i < NUM_COLUMNS; ++i )
					{
						if ( column1_array[ i ] == true )
						{
							proceed = true;
							break;
						}
					}

					if ( proceed == false )
					{
						_SendMessageW( g_hWnd_column_tab, TCM_SETCURFOCUS, 0, 0 );
						_MessageBoxW( hWnd, ST_must_have_column, PROGRAM_CAPTION, MB_ICONWARNING );
						break;
					}

					bool column2_array[ NUM_COLUMNS2 ];
					column2_array[ 0 ] = ( _SendMessageW( g_hWnd_chk_contact_list_num, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 1 ] = ( _SendMessageW( g_hWnd_chk_cell_phone, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 2 ] = ( _SendMessageW( g_hWnd_chk_company, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 3 ] = ( _SendMessageW( g_hWnd_chk_department, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 4 ] = ( _SendMessageW( g_hWnd_chk_email_address, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 5 ] = ( _SendMessageW( g_hWnd_chk_fax_number, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 6 ] = ( _SendMessageW( g_hWnd_chk_first_name, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 7 ] = ( _SendMessageW( g_hWnd_chk_home_phone, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 8 ] = ( _SendMessageW( g_hWnd_chk_job_title, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 9 ] = ( _SendMessageW( g_hWnd_chk_last_name, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 10 ] = ( _SendMessageW( g_hWnd_chk_nickname, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 11 ] = ( _SendMessageW( g_hWnd_chk_office_phone, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 12 ] = ( _SendMessageW( g_hWnd_chk_other_phone, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 13 ] = ( _SendMessageW( g_hWnd_chk_profession, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 14 ] = ( _SendMessageW( g_hWnd_chk_title, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 15 ] = ( _SendMessageW( g_hWnd_chk_web_page, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column2_array[ 16 ] = ( _SendMessageW( g_hWnd_chk_work_phone_number, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					proceed = false;
					for ( char i = 0; i < NUM_COLUMNS2; ++i )
					{
						if ( column2_array[ i ] == true )
						{
							proceed = true;
							break;
						}
					}

					if ( proceed == false )
					{
						_SendMessageW( g_hWnd_column_tab, TCM_SETCURFOCUS, 1, 0 );
						_MessageBoxW( hWnd, ST_must_have_column, PROGRAM_CAPTION, MB_ICONWARNING );
						break;
					}

					bool column3_array[ NUM_COLUMNS3 ];
					column3_array[ 0 ] = ( _SendMessageW( g_hWnd_chk_forwardlist_num, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column3_array[ 1 ] = ( _SendMessageW( g_hWnd_chk_forwardlist_to_phone, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column3_array[ 2 ] = ( _SendMessageW( g_hWnd_chk_forwardlist_from_phone, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column3_array[ 3 ] = ( _SendMessageW( g_hWnd_chk_forwardlist_total_calls, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					proceed = false;
					for ( char i = 0; i < NUM_COLUMNS3; ++i )
					{
						if ( column3_array[ i ] == true )
						{
							proceed = true;
							break;
						}
					}

					if ( proceed == false )
					{
						_SendMessageW( g_hWnd_column_tab, TCM_SETCURFOCUS, 2, 0 );
						_MessageBoxW( hWnd, ST_must_have_column, PROGRAM_CAPTION, MB_ICONWARNING );
						break;
					}

					bool column4_array[ NUM_COLUMNS4 ];
					column4_array[ 0 ] = ( _SendMessageW( g_hWnd_chk_ignored_num, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column4_array[ 1 ]= ( _SendMessageW( g_hWnd_chk_ignored_phone, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					column4_array[ 2 ]= ( _SendMessageW( g_hWnd_chk_ignored_total_calls, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					proceed = false;
					for ( char i = 0; i < NUM_COLUMNS4; ++i )
					{
						if ( column4_array[ i ] == true )
						{
							proceed = true;
							break;
						}
					}

					if ( proceed == false )
					{
						_SendMessageW( g_hWnd_column_tab, TCM_SETCURFOCUS, 3, 0 );
						_MessageBoxW( hWnd, ST_must_have_column, PROGRAM_CAPTION, MB_ICONWARNING );
						break;
					}

					// Make sure our columns are up to date.
					UpdateColumnOrders();

					int index = 0;	// Column index from virtual index.

					// Add new columns first.

					LVCOLUMN lvc;
					_memzero( &lvc, sizeof( LVCOLUMN ) );
					lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_ORDER; 

					for ( char i = 0; i < NUM_COLUMNS; ++i )
					{
						if ( column1_array[ i ] == true && *list_columns[ i ] == -1 )
						{
							switch ( i )
							{
								case 0:
								{
									*list_columns[ i ] = 0;
									index = 0;

									// Update the virtual indices.
									for ( int j = 1; j < NUM_COLUMNS; ++j )
									{
										if ( *list_columns[ j ] != -1 )
										{
											( *( list_columns[ j ] ) )++;
										}
									}
								}
								break;

								case 1:
								case 2:
								case 3:
								case 4:
								case 5:
								case 6:
								case 7:
								{
									*list_columns[ i ] = total_columns;
									index = GetColumnIndexFromVirtualIndex( *list_columns[ i ], list_columns, NUM_COLUMNS );
								}
								break;

								case 8:
								{
									*list_columns[ i ] = total_columns;
									index = total_columns;
								}
								break;
							}

							lvc.iOrder = *list_columns[ i ];
							lvc.pszText = call_log_string_table[ i ];
							lvc.cx = *list_columns_width[ i ];
							_SendMessageW( g_hWnd_list, LVM_INSERTCOLUMN, index, ( LPARAM )&lvc );

							++total_columns;
						}
					}

					for ( char i = 0; i < NUM_COLUMNS2; ++i )
					{
						if ( column2_array[ i ] == true && *contact_list_columns[ i ] == -1 )
						{
							switch ( i )
							{
								case 0:
								{
									*contact_list_columns[ i ] = 0;
									index = 0;

									// Update the virtual indices.
									for ( int j = 1; j < NUM_COLUMNS2; ++j )
									{
										if ( *contact_list_columns[ j ] != -1 )
										{
											( *( contact_list_columns[ j ] ) )++;
										}
									}
								}
								break;

								case 1:
								case 2:
								case 3:
								case 4:
								case 5:
								case 6:
								case 7:
								case 8:
								case 9:
								case 10:
								case 11:
								case 12:
								case 13:
								case 14:
								case 15:
								{
									*contact_list_columns[ i ] = total_columns2;
									index = GetColumnIndexFromVirtualIndex( *contact_list_columns[ i ], contact_list_columns, NUM_COLUMNS2 );
								}
								break;

								case 16:
								{
									*contact_list_columns[ i ] = total_columns2;
									index = total_columns2;
								}
								break;
							}

							lvc.iOrder = *contact_list_columns[ i ];
							lvc.pszText = contact_list_string_table[ i ];
							lvc.cx = *contact_list_columns_width[ i ];
							_SendMessageW( g_hWnd_contact_list, LVM_INSERTCOLUMN, index, ( LPARAM )&lvc );

							++total_columns2;
						}
					}

					for ( char i = 0; i < NUM_COLUMNS3; ++i )
					{
						if ( column3_array[ i ] == true && *forward_list_columns[ i ] == -1 )
						{
							switch ( i )
							{
								case 0:
								{
									*forward_list_columns[ i ] = 0;
									index = 0;

									// Update the virtual indices.
									for ( int j = 1; j < NUM_COLUMNS3; ++j )
									{
										if ( *forward_list_columns[ j ] != -1 )
										{
											( *( forward_list_columns[ j ] ) )++;
										}
									}
								}
								break;

								case 1:
								case 2:
								{
									*forward_list_columns[ i ] = total_columns3;
									index = GetColumnIndexFromVirtualIndex( *forward_list_columns[ i ], forward_list_columns, NUM_COLUMNS3 );
								}
								break;

								case 3:
								{
									*forward_list_columns[ i ] = total_columns3;
									index = total_columns3;
								}
								break;
							}

							lvc.iOrder = *forward_list_columns[ i ];
							lvc.pszText = forward_list_string_table[ i ];
							lvc.cx = *forward_list_columns_width[ i ];
							_SendMessageW( g_hWnd_forward_list, LVM_INSERTCOLUMN, index, ( LPARAM )&lvc );

							++total_columns3;
						}
					}

					for ( char i = 0; i < NUM_COLUMNS4; ++i )
					{
						if ( column4_array[ i ] == true && *ignore_list_columns[ i ] == -1 )
						{
							switch ( i )
							{
								case 0:
								{
									*ignore_list_columns[ i ] = 0;
									index = 0;

									// Update the virtual indices.
									for ( int j = 1; j < NUM_COLUMNS4; ++j )
									{
										if ( *ignore_list_columns[ j ] != -1 )
										{
											( *( ignore_list_columns[ j ] ) )++;
										}
									}
								}
								break;

								case 1:
								{
									*ignore_list_columns[ i ] = total_columns4;
									index = GetColumnIndexFromVirtualIndex( *ignore_list_columns[ i ], ignore_list_columns, NUM_COLUMNS4 );
								}
								break;

								case 2:
								{
									*ignore_list_columns[ i ] = total_columns4;
									index = total_columns4;
								}
								break;
							}

							lvc.iOrder = *ignore_list_columns[ i ];
							lvc.pszText = ignore_list_string_table[ i ];
							lvc.cx = *ignore_list_columns_width[ i ];
							_SendMessageW( g_hWnd_ignore_list, LVM_INSERTCOLUMN, index, ( LPARAM )&lvc );

							++total_columns4;
						}
					}

					// Remove Columns.

					for ( char i = NUM_COLUMNS - 1; i >= 0; --i )
					{
						if ( column1_array[ i ] == false && *list_columns[ i ] != -1 )
						{
							--total_columns;

							switch ( i )
							{
								case 8:
								{
									index = total_columns;
								}
								break;

								case 7:
								case 6:
								case 5:
								case 4:
								case 3:
								case 2:
								case 1:
								{
									index = GetColumnIndexFromVirtualIndex( *list_columns[ i ], list_columns, NUM_COLUMNS );
								}
								break;

								case 0:
								{
									index = 0;	
								}
								break;
							}

							*list_columns[ i ] = -1;
							_SendMessageW( g_hWnd_list, LVM_DELETECOLUMN, index, 0 );
						}
					}

					for ( char i = NUM_COLUMNS2 - 1; i >= 0; --i )
					{
						if ( column2_array[ i ] == false && *contact_list_columns[ i ] != -1 )
						{
							--total_columns2;

							switch ( i )
							{
								case 16:
								{
									index = total_columns2;
								}
								break;

								case 15:
								case 14:
								case 13:
								case 12:
								case 11:
								case 10:
								case 9:
								case 8:
								case 7:
								case 6:
								case 5:
								case 4:
								case 3:
								case 2:
								case 1:
								{
									index = GetColumnIndexFromVirtualIndex( *contact_list_columns[ i ], contact_list_columns, NUM_COLUMNS2 );
								}
								break;

								case 0:
								{
									index = 0;	
								}
								break;
							}

							*contact_list_columns[ i ] = -1;
							_SendMessageW( g_hWnd_contact_list, LVM_DELETECOLUMN, index, 0 );
						}
					}

					for ( char i = NUM_COLUMNS3 - 1; i >= 0; --i )
					{
						if ( column3_array[ i ] == false && *forward_list_columns[ i ] != -1 )
						{
							--total_columns3;

							switch ( i )
							{
								case 3:
								{
									index = total_columns3;
								}
								break;

								case 2:
								case 1:
								{
									index = GetColumnIndexFromVirtualIndex( *forward_list_columns[ i ], forward_list_columns, NUM_COLUMNS3 );
									
								}
								break;

								case 0:
								{
									index = 0;	
								}
								break;
							}

							*forward_list_columns[ i ] = -1;
							_SendMessageW( g_hWnd_forward_list, LVM_DELETECOLUMN, index, 0 );
						}
					}

					for ( char i = NUM_COLUMNS4 - 1; i >= 0; --i )
					{
						if ( column4_array[ i ] == false && *ignore_list_columns[ i ] != -1 )
						{
							--total_columns4;

							switch ( i )
							{
								case 2:
								{
									index = total_columns4;
								}
								break;

								case 1:
								{
									index = GetColumnIndexFromVirtualIndex( *ignore_list_columns[ i ], ignore_list_columns, NUM_COLUMNS4 );
									
								}
								break;

								case 0:
								{
									index = 0;	
								}
								break;
							}

							*ignore_list_columns[ i ] = -1;
							_SendMessageW( g_hWnd_ignore_list, LVM_DELETECOLUMN, index, 0 );
						}
					}

					// Set our columns to their final order.
					UpdateColumnOrders();

					// Save our new settings.
					save_config();

					columns_state_changed = false;

					if ( LOWORD( wParam ) == BTN_APPLY_COLUMNS )
					{
						_EnableWindow( g_hWnd_apply_columns, FALSE );
					}
					else
					{
						_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
					}
				}
				break;

				case BTN_CANCEL_COLUMNS:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;
			}

			return 0;
		}
		break;

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case TCN_SELCHANGING:		// The tab that's about to lose focus
				{
					NMHDR *nmhdr = ( NMHDR * )lParam;

					TCITEM tie;
					_memzero( &tie, sizeof( TCITEM ) );
					tie.mask = TCIF_PARAM; // Get the lparam value
					int index = _SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( ( HWND )( tie.lParam ), SW_HIDE );
					}

					return FALSE;
				}
				break;

				case TCN_SELCHANGE:			// The tab that gains focus
                {
					NMHDR *nmhdr = ( NMHDR * )lParam;

					HWND hWnd_focused = GetFocus();
					if ( hWnd_focused != hWnd && hWnd_focused != nmhdr->hwndFrom )
					{
						SetFocus( GetWindow( nmhdr->hwndFrom, GW_CHILD ) );
					}

					TCITEM tie;
					_memzero( &tie, sizeof( TCITEM ) );
					tie.mask = TCIF_PARAM; // Get the lparam value
					int index = _SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( ( HWND )( tie.lParam ), SW_SHOW );
					}

					return FALSE;
                }
				break;
			}

			return FALSE;
		}
		break;

		case WM_PROPAGATE:
		{
			_SendMessageW( g_hWnd_column_tab, TCM_SETCURFOCUS, wParam, 0 );

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
			columns_state_changed = false;	// Reset the state if it changed, but the columns were never saved.

			g_hWnd_columns = NULL;
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
