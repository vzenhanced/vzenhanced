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
#include "wnd_subproc.h"
#include "connection.h"
#include "menus.h"
#include "utilities.h"
#include "file_operations.h"
#include "string_tables.h"
#include <commdlg.h>

#include "lite_gdi32.h"
#include "lite_comdlg32.h"

#include "web_server.h"

#define _wcsicmp_s( a, b ) ( ( a == NULL && b == NULL ) ? 0 : ( a != NULL && b == NULL ) ? 1 : ( a == NULL && b != NULL ) ? -1 : lstrcmpiW( a, b ) )
#define _stricmp_s( a, b ) ( ( a == NULL && b == NULL ) ? 0 : ( a != NULL && b == NULL ) ? 1 : ( a == NULL && b != NULL ) ? -1 : lstrcmpiA( a, b ) )

// Object variables
HWND g_hWnd_columns = NULL;
HWND g_hWnd_options = NULL;
HWND g_hWnd_account = NULL;
HWND g_hWnd_contact = NULL;
HWND g_hWnd_dial = NULL;
HWND g_hWnd_forward = NULL;
HWND g_hWnd_ignore_phone_number = NULL;
HWND g_hWnd_list = NULL;				// Handle to the listview control.
HWND g_hWnd_contact_list = NULL;
HWND g_hWnd_ignore_list = NULL;
HWND g_hWnd_forward_list = NULL;
HWND g_hWnd_edit = NULL;				// Handle to the listview edit control.
HWND g_hWnd_tab = NULL;
HWND g_hWnd_connection_manager = NULL;


NOTIFYICONDATA g_nid;					// Tray icon information.

// Window variables
int cx = 0;								// Current x (left) position of the main window based on the mouse.
int cy = 0;								// Current y (top) position of the main window based on the mouse.

unsigned char total_tabs = 0;

unsigned char total_columns = 0;
unsigned char total_columns2 = 0;
unsigned char total_columns3 = 0;
unsigned char total_columns4 = 0;

bool last_menu = false;	// true if context menu was last open, false if main menu was last open. See: WM_ENTERMENULOOP

struct sortinfo
{
	HWND hWnd;
	int column;
	unsigned char direction;
};



// Sort function for columns.
int CALLBACK CompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	sortinfo *si = ( sortinfo * )lParamSort;

	int arr[ 17 ];

	if ( si->hWnd == g_hWnd_list )
	{
		displayinfo *fi1 = ( displayinfo * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		displayinfo *fi2 = ( displayinfo * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_list, LVM_GETCOLUMNORDERARRAY, total_columns, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, list_columns, NUM_COLUMNS, total_columns );

		switch ( arr[ si->column ] )
		{
			case 5:
			{
				if ( fi1->ignore == fi2->ignore )
				{
					return 0;
				}
				else if ( fi1->ignore == true && fi2->ignore == false )
				{
					return 1;
				}
				else
				{
					return -1;
				}
			}
			break;

			case 3:
			{
				if ( fi1->forward == fi2->forward )
				{
					return 0;
				}
				else if ( fi1->forward == true && fi2->forward == false )
				{
					return 1;
				}
				else
				{
					return -1;
				}
			}
			break;

			case 1: { return _stricmp_s( fi1->ci.caller_id, fi2->ci.caller_id ); } break;
			case 7: { return _stricmp_s( fi1->ci.call_reference_id, fi2->ci.call_reference_id ); } break;

			case 4:
			case 6:
			case 8: { return _wcsicmp_s( fi1->display_values[ arr[ si->column ] - 1 ], fi2->display_values[ arr[ si->column ] - 1 ] ); } break;

			case 2: { return ( fi1->time.QuadPart > fi2->time.QuadPart ); } break;

			default:
			{
				return 0;
			}
			break;
		}	
	}
	else if ( si->hWnd == g_hWnd_contact_list )
	{
		contactinfo *fi1 = ( contactinfo * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		contactinfo *fi2 = ( contactinfo * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_contact_list, LVM_GETCOLUMNORDERARRAY, total_columns2, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, contact_list_columns, NUM_COLUMNS2, total_columns2 );

		switch ( arr[ si->column ] )
		{
			case 1:
			case 5:
			case 7:
			case 11:
			case 12:
			case 16: { return _wcsicmp_s( fi1->contactinfo_values[ arr[ si->column ] - 1 ], fi2->contactinfo_values[ arr[ si->column ] - 1 ] ); } break;
			
			case 2:
			case 3:
			case 4:
			case 6:
			case 8:
			case 9:
			case 10:
			case 13:
			case 14:
			case 15: { return _stricmp_s( fi1->contact.contact_values[ arr[ si->column ] - 1 ], fi2->contact.contact_values[ arr[ si->column ] - 1 ] ); } break;

			default:
			{
				return 0;
			}
			break;
		}	
	}
	else if ( si->hWnd == g_hWnd_forward_list )
	{
		forwardinfo *fi1 = ( forwardinfo * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		forwardinfo *fi2 = ( forwardinfo * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_forward_list, LVM_GETCOLUMNORDERARRAY, total_columns3, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, forward_list_columns, NUM_COLUMNS3, total_columns3 );

		switch ( arr[ si->column ] )
		{
			case 1:
			case 2: { return _wcsicmp_s( fi1->forwardinfo_values[ arr[ si->column ] - 1 ], fi2->forwardinfo_values[ arr[ si->column ] - 1 ] ); } break;
			case 3:	{ return ( fi1->count > fi2->count ); } break;

			default:
			{
				return 0;
			}
			break;
		}
	}
	else if ( si->hWnd == g_hWnd_ignore_list )
	{
		ignoreinfo *fi1 = ( ignoreinfo * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		ignoreinfo *fi2 = ( ignoreinfo * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_ignore_list, LVM_GETCOLUMNORDERARRAY, total_columns4, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, ignore_list_columns, NUM_COLUMNS4, total_columns4 );

		switch ( arr[ si->column ] )
		{
			case 1: { return _wcsicmp_s( fi1->ignoreinfo_values[ arr[ si->column ] - 1 ], fi2->ignoreinfo_values[ arr[ si->column ] - 1 ] ); } break;
			case 2:	{ return ( fi1->count > fi2->count ); } break;

			default:
			{
				return 0;
			}
			break;
		}
	}

	return 0;
}

LRESULT CALLBACK MainWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			CreateMenus();

			// Set our menu bar.
			_SetMenu( hWnd, g_hMenu );

			// Create our listview windows.
			g_hWnd_list = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW, 0, 0, MIN_WIDTH, MIN_HEIGHT, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP );

			g_hWnd_contact_list = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW, 0, 0, MIN_WIDTH, MIN_HEIGHT, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_contact_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP );

			g_hWnd_ignore_list = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW, 0, 0, MIN_WIDTH, MIN_HEIGHT, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_ignore_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP );

			g_hWnd_forward_list = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW, 0, 0, MIN_WIDTH, MIN_HEIGHT, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_forward_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP );



			g_hWnd_tab = _CreateWindowW( WC_TABCONTROL, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			TabProc = ( WNDPROC )_GetWindowLongW( g_hWnd_tab, GWL_WNDPROC );
			_SetWindowLongW( g_hWnd_tab, GWL_WNDPROC, ( LONG )TabSubProc );

			_SendMessageW( g_hWnd_tab, WM_PROPAGATE, 1, 0 );	// Open theme data. Must be done after we subclass the control. Theme object will be closed when the tab control is destroyed.

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_PARAM | TCIF_TEXT;			// The tab will have text and an lParam value.

			for ( int i = 0; i < 4; ++i )
			{
				if ( i == cfg_tab_order1 )
				{
					if ( i == 0 )
					{
						_ShowWindow( g_hWnd_list, SW_SHOW );
					}

					ti.pszText = ( LPWSTR )ST_Call_Log;		// This will simply set the width of each tab item. We're not going to use it.
					ti.lParam = ( LPARAM )g_hWnd_list;
					_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, total_tabs++, ( LPARAM )&ti );	// Insert a new tab at the end.
				}
				else if ( i == cfg_tab_order2 )
				{
					if ( i == 0 )
					{
						_ShowWindow( g_hWnd_contact_list, SW_SHOW );
					}

					ti.pszText = ( LPWSTR )ST_Contact_List;	// This will simply set the width of each tab item. We're not going to use it.
					ti.lParam = ( LPARAM )g_hWnd_contact_list;
					_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, total_tabs++, ( LPARAM )&ti );	// Insert a new tab at the end.
				}
				else if ( i == cfg_tab_order3 )
				{
					if ( i == 0 )
					{
						_ShowWindow( g_hWnd_forward_list, SW_SHOW );
					}

					ti.pszText = ( LPWSTR )ST_Forward_List;	// This will simply set the width of each tab item. We're not going to use it.
					ti.lParam = ( LPARAM )g_hWnd_forward_list;
					_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, total_tabs++, ( LPARAM )&ti );	// Insert a new tab at the end.
				}
				else if ( i == cfg_tab_order4 )
				{
					if ( i == 0 )
					{
						_ShowWindow( g_hWnd_ignore_list, SW_SHOW );
					}

					ti.pszText = ( LPWSTR )ST_Ignore_List;	// This will simply set the width of each tab item. We're not going to use it.
					ti.lParam = ( LPARAM )g_hWnd_ignore_list;
					_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, total_tabs++, ( LPARAM )&ti );	// Insert a new tab at the end.
				}
			}

			if ( total_tabs == 0 )
			{
				_ShowWindow( g_hWnd_tab, SW_HIDE );
			}

			int arr[ 17 ];

			// Initliaze our listview columns
			LVCOLUMN lvc;
			_memzero( &lvc, sizeof( LVCOLUMN ) );
			lvc.mask = LVCF_WIDTH | LVCF_TEXT;

			for ( char i = 0; i < NUM_COLUMNS; ++i )
			{
				if ( *list_columns[ i ] != -1 )
				{
					lvc.pszText = call_log_string_table[ i ];
					lvc.cx = *list_columns_width[ i ];
					_SendMessageW( g_hWnd_list, LVM_INSERTCOLUMN, total_columns, ( LPARAM )&lvc );

					arr[ total_columns++ ] = *list_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_list, LVM_SETCOLUMNORDERARRAY, total_columns, ( LPARAM )arr );

			for ( char i = 0; i < NUM_COLUMNS2; ++i )
			{
				if ( *contact_list_columns[ i ] != -1 )
				{
					lvc.pszText = contact_list_string_table[ i ];
					lvc.cx = *contact_list_columns_width[ i ];
					_SendMessageW( g_hWnd_contact_list, LVM_INSERTCOLUMN, total_columns2, ( LPARAM )&lvc );

					arr[ total_columns2++ ] = *contact_list_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_contact_list, LVM_SETCOLUMNORDERARRAY, total_columns2, ( LPARAM )arr );

			for ( char i = 0; i < NUM_COLUMNS3; ++i )
			{
				if ( *forward_list_columns[ i ] != -1 )
				{
					lvc.pszText = forward_list_string_table[ i ];
					lvc.cx = *forward_list_columns_width[ i ];
					_SendMessageW( g_hWnd_forward_list, LVM_INSERTCOLUMN, total_columns3, ( LPARAM )&lvc );

					arr[ total_columns3++ ] = *forward_list_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_forward_list, LVM_SETCOLUMNORDERARRAY, total_columns3, ( LPARAM )arr );

			for ( char i = 0; i < NUM_COLUMNS4; ++i )
			{
				if ( *ignore_list_columns[ i ] != -1 )
				{
					lvc.pszText = ignore_list_string_table[ i ];
					lvc.cx = *ignore_list_columns_width[ i ];
					_SendMessageW( g_hWnd_ignore_list, LVM_INSERTCOLUMN, total_columns4, ( LPARAM )&lvc );

					arr[ total_columns4++ ] = *ignore_list_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_ignore_list, LVM_SETCOLUMNORDERARRAY, total_columns4, ( LPARAM )arr );

			if ( cfg_tray_icon == true )
			{
				_memzero( &g_nid, sizeof( NOTIFYICONDATA ) );
				g_nid.cbSize = sizeof( g_nid );
				g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
				g_nid.hWnd = hWnd;
				g_nid.uCallbackMessage = WM_TRAY_NOTIFY;
				g_nid.uID = 1000;
				g_nid.hIcon = ( HICON )_LoadImageW( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ICON ), IMAGE_ICON, 16, 16, LR_SHARED );
				_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ), L"VZ Enhanced - Logged Out\0", 25 );
				g_nid.szTip[ 24 ] = 0;	// Sanity.
				_Shell_NotifyIconW( NIM_ADD, &g_nid );
			}

			if ( cfg_enable_call_log_history == true )
			{
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, read_call_log_history, ( void * )NULL, 0, NULL ) );
			}

			forwardupdateinfo *fui = ( forwardupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardupdateinfo ) );
			fui->fi = NULL;
			fui->call_from = NULL;
			fui->forward_to = NULL;
			fui->action = 2;	// Add all forward_list items.
			fui->hWnd = g_hWnd_forward_list;

			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_forward_list, ( void * )fui, 0, NULL ) );


			ignoreupdateinfo *iui = ( ignoreupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreupdateinfo ) );
			iui->ii = NULL;
			iui->phone_number = NULL;
			iui->action = 2;	// Add all ignore_list items.
			iui->hWnd = g_hWnd_ignore_list;

			// iui is freed in the update_ignore_list thread.
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_list, ( void * )iui, 0, NULL ) );


			return 0;
		}
		break;

		case WM_KEYDOWN:
		{
			// We'll just give the listview control focus since it's handling our keypress events.
			_SetFocus( g_hWnd_list );

			return 0;
		}
		break;

		case WM_MOVING:
		{
			POINT cur_pos;
			RECT wa;
			RECT *rc = ( RECT * )lParam;

			_GetCursorPos( &cur_pos );
			_OffsetRect( rc, cur_pos.x - ( rc->left + cx ), cur_pos.y - ( rc->top + cy ) );

			// Allow our main window to attach to the desktop edge.
			_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );			
			if( is_close( rc->left, wa.left ) )				// Attach to left side of the desktop.
			{
				_OffsetRect( rc, wa.left - rc->left, 0 );
			}
			else if ( is_close( wa.right, rc->right ) )		// Attach to right side of the desktop.
			{
				_OffsetRect( rc, wa.right - rc->right, 0 );
			}

			if( is_close( rc->top, wa.top ) )				// Attach to top of the desktop.
			{
				_OffsetRect( rc, 0, wa.top - rc->top );
			}
			else if ( is_close( wa.bottom, rc->bottom ) )	// Attach to bottom of the desktop.
			{
				_OffsetRect( rc, 0, wa.bottom - rc->bottom );
			}

			// Save our settings for the position/dimensions of the window.
			cfg_pos_x = rc->left;
			cfg_pos_y = rc->top;
			cfg_width = rc->right - rc->left;
			cfg_height = rc->bottom - rc->top;

			return TRUE;
		}
		break;

		case WM_ENTERMENULOOP:
		{
			// If we've clicked the menu bar.
			if ( ( BOOL )wParam == FALSE )
			{
				// And a context menu was open, then revert the context menu additions.
				if ( last_menu == true )
				{
					UpdateMenus( UM_ENABLE );

					last_menu = false;	// Prevent us from calling UpdateMenus again.
				}

				// Allow us to save the call log if there are any entries in the call log listview.
				_EnableMenuItem( g_hMenu, MENU_SAVE_CALL_LOG, ( _SendMessageW( g_hWnd_list, LVM_GETITEMCOUNT, 0, 0 ) > 0 ? MF_ENABLED : MF_DISABLED ) );
			}
			else if ( ( BOOL )wParam == TRUE )
			{
				last_menu = true;	// The last menu to be open was a context menu.
			}

			return 0;
		}
		break;

		case WM_ENTERSIZEMOVE:
		{
			//Get the current position of our window before it gets moved.
			POINT cur_pos;
			RECT rc;
			_GetWindowRect( hWnd, &rc );
			_GetCursorPos( &cur_pos );
			cx = cur_pos.x - rc.left;
			cy = cur_pos.y - rc.top;

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc, rc_tab;
			_GetClientRect( hWnd, &rc );

			// Calculate the display rectangle, assuming the tab control is the size of the client area.
			_SetRect( &rc_tab, 0, 0, rc.right, rc.bottom );
			_SendMessageW( g_hWnd_tab, TCM_ADJUSTRECT, FALSE, ( LPARAM )&rc_tab );
			rc_tab.top -= 2;	// Cut the last 2 lines off the tab control.

			// Allow our listview to resize in proportion to the main window.
			HDWP hdwp = _BeginDeferWindowPos( 4 );
			_DeferWindowPos( hdwp, g_hWnd_tab, HWND_TOP, 1, 0, rc.right, rc.bottom, 0 );
			_DeferWindowPos( hdwp, g_hWnd_list, HWND_TOP, 2, rc_tab.top + 2, rc.right - rc.left - 4, rc.bottom - rc_tab.top - 4, 0 );
			_DeferWindowPos( hdwp, g_hWnd_contact_list, HWND_TOP, 2, rc_tab.top + 2, rc.right - rc.left - 4, rc.bottom - rc_tab.top - 4, 0 );
			_DeferWindowPos( hdwp, g_hWnd_ignore_list, HWND_TOP, 2, rc_tab.top + 2, rc.right - rc.left - 4, rc.bottom - rc_tab.top - 4, 0 );
			_DeferWindowPos( hdwp, g_hWnd_forward_list, HWND_TOP, 2, rc_tab.top + 2, rc.right - rc.left - 4, rc.bottom - rc_tab.top - 4, 0 );
			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_SIZING:
		{
			RECT *rc = ( RECT * )lParam;

			// Save our settings for the position/dimensions of the window.
			cfg_pos_x = rc->left;
			cfg_pos_y = rc->top;
			cfg_width = rc->right - rc->left;
			cfg_height = rc->bottom - rc->top;
		}
		break;

		case WM_MEASUREITEM:
		{
			// Set the row height of the list view.
			if ( ( ( LPMEASUREITEMSTRUCT )lParam )->CtlType = ODT_LISTVIEW )
			{
				( ( LPMEASUREITEMSTRUCT )lParam )->itemHeight = _GetSystemMetrics( SM_CYSMICON ) + 2;
			}
			return TRUE;
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

		case WM_COMMAND:
		{
			// Check to see if our command is a menu item.
			if ( HIWORD( wParam ) == 0 )
			{
				// Get the id of the menu item.
				switch ( LOWORD( wParam ) )
				{
					case MENU_LOGIN:
					{
						if ( login_state == LOGGED_IN )
						{
							// Forceful shutdown.
							login_state = LOGGED_OUT;
							if ( main_con.ssl_socket != NULL )
							{
								_shutdown( main_con.ssl_socket->s, SD_BOTH );
							}
						}
						else
						{
							if ( _IsWindowVisible( g_hWnd_login ) == TRUE )
							{
								_SetForegroundWindow( g_hWnd_login );
							}
							else
							{
								_SendMessageW( g_hWnd_login, WM_PROPAGATE, 0, 0 );
							}
						}
					}
					break;

					case MENU_CLOSE_TAB:
					{
						switch ( context_tab_index )
						{
							case 0:
							{
								_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( MENU_VIEW_IGNORE_LIST, 0 ), 0 );
							}
							break;

							case 1:
							{
								_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( MENU_VIEW_CALL_LOG, 0 ), 0 );
							}
							break;

							case 2:
							{
								_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( MENU_VIEW_CONTACT_LIST, 0 ), 0 );
							}
							break;

							case 3:
							{
								_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( MENU_VIEW_FORWARD_LIST, 0 ), 0 );
							}
							break;
						}
					}
					break;

					case MENU_VIEW_IGNORE_LIST:
					case MENU_VIEW_CALL_LOG:
					case MENU_VIEW_CONTACT_LIST:
					case MENU_VIEW_FORWARD_LIST:
					{
						char *tab_index = NULL;
						HWND t_hWnd = NULL;

						TCITEM ti;
						_memzero( &ti, sizeof( TCITEM ) );
						ti.mask = TCIF_PARAM | TCIF_TEXT;			// The tab will have text and an lParam value.

						if ( LOWORD( wParam ) == MENU_VIEW_CALL_LOG )
						{
							tab_index = &cfg_tab_order1;
							t_hWnd = g_hWnd_list;
							ti.pszText = ( LPWSTR )ST_Call_Log;		// This will simply set the width of each tab item. We're not going to use it.
						}
						else if ( LOWORD( wParam ) == MENU_VIEW_CONTACT_LIST )
						{
							tab_index = &cfg_tab_order2;
							t_hWnd = g_hWnd_contact_list;
							ti.pszText = ( LPWSTR )ST_Contact_List;		// This will simply set the width of each tab item. We're not going to use it.
						}
						else if ( LOWORD( wParam ) == MENU_VIEW_FORWARD_LIST )
						{
							tab_index = &cfg_tab_order3;
							t_hWnd = g_hWnd_forward_list;
							ti.pszText = ( LPWSTR )ST_Forward_List;		// This will simply set the width of each tab item. We're not going to use it.
						}
						else if ( LOWORD( wParam ) == MENU_VIEW_IGNORE_LIST )
						{
							tab_index = &cfg_tab_order4;
							t_hWnd = g_hWnd_ignore_list;
							ti.pszText = ( LPWSTR )ST_Ignore_List;		// This will simply set the width of each tab item. We're not going to use it.
						}

						if ( *tab_index != -1 )	// Remove the tab.
						{
							// Go through all the tabs and decrease the order values that are greater than the index we removed.
							for ( unsigned char i = 0; i < NUM_TABS; ++i )
							{
								if ( *tab_order[ i ] != -1 && *tab_order[ i ] > *tab_index )
								{
									( *( tab_order[ i ] ) )--;
								}
							}

							*tab_index = -1;

							for ( int i = 0; i < total_tabs; ++i )
							{
								_SendMessageW( g_hWnd_tab, TCM_GETITEM, i, ( LPARAM )&ti );

								if ( ( HWND )ti.lParam == t_hWnd )
								{
									int index = _SendMessageW( g_hWnd_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab

									// If the tab we remove is the last tab and it is focused, then set focus to the tab on the left.
									// If the tab we remove is focused and there is a tab to the right, then set focus to the tab on the right.
									if ( total_tabs > 1 && i == index )
									{
										_SendMessageW( g_hWnd_tab, TCM_SETCURFOCUS, ( i < total_tabs - 1 ? i + 1 : i - 1 ), 0 );	// Give the new tab focus.
									}

									_SendMessageW( g_hWnd_tab, TCM_DELETEITEM, i, 0 );
									--total_tabs;

									// Hide the tab control if no more tabs are visible.
									if ( total_tabs == 0 )
									{
										_ShowWindow( ( HWND )ti.lParam, SW_HIDE );
										_ShowWindow( g_hWnd_tab, SW_HIDE );
									}
								}
							}
						}
						else	// Add the tab.
						{
							*tab_index = total_tabs;	// The new tab will be added to the end.

							ti.lParam = ( LPARAM )t_hWnd;
							_SendMessageW( g_hWnd_tab, TCM_INSERTITEM, total_tabs, ( LPARAM )&ti );	// Insert a new tab at the end.

							// If no tabs were previously visible, then show the tab control.
							if ( total_tabs == 0 )
							{
								_ShowWindow( g_hWnd_tab, SW_SHOW );
								_ShowWindow( t_hWnd, SW_SHOW );

								_SendMessageW( hWnd, WM_SIZE, 0, 0 );	// Forces the window to resize the listview.
							}

							++total_tabs;
						}

						_CheckMenuItem( g_hMenu, LOWORD( wParam ), ( *tab_index != -1 ? MF_CHECKED : MF_UNCHECKED ) );
					}
					break;

					case MENU_COPY_SEL_COL1:
					case MENU_COPY_SEL_COL2:
					case MENU_COPY_SEL_COL3:
					case MENU_COPY_SEL_COL4:
					case MENU_COPY_SEL_COL5:
					case MENU_COPY_SEL_COL6:
					case MENU_COPY_SEL_COL7:
					case MENU_COPY_SEL_COL8:
					case MENU_COPY_SEL_COL21:
					case MENU_COPY_SEL_COL22:
					case MENU_COPY_SEL_COL23:
					case MENU_COPY_SEL_COL24:
					case MENU_COPY_SEL_COL25:
					case MENU_COPY_SEL_COL26:
					case MENU_COPY_SEL_COL27:
					case MENU_COPY_SEL_COL28:
					case MENU_COPY_SEL_COL29:
					case MENU_COPY_SEL_COL210:
					case MENU_COPY_SEL_COL211:
					case MENU_COPY_SEL_COL212:
					case MENU_COPY_SEL_COL213:
					case MENU_COPY_SEL_COL214:
					case MENU_COPY_SEL_COL215:
					case MENU_COPY_SEL_COL216:
					case MENU_COPY_SEL_COL31:
					case MENU_COPY_SEL_COL32:
					case MENU_COPY_SEL_COL33:
					case MENU_COPY_SEL_COL41:
					case MENU_COPY_SEL_COL42:
					case MENU_COPY_SEL:
					{
						int index = _SendMessageW( g_hWnd_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
						if ( index != -1 )
						{
							TCITEM tie;
							_memzero( &tie, sizeof( TCITEM ) );
							tie.mask = TCIF_PARAM; // Get the lparam value
							_SendMessageW( g_hWnd_tab, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
							
							copyinfo *ci = ( copyinfo * )GlobalAlloc( GMEM_FIXED, sizeof( copyinfo ) );
							ci->column = LOWORD( wParam );
							ci->hWnd = ( HWND )tie.lParam;

							// ci is freed in the copy items thread.
							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, copy_items, ( void * )ci, 0, NULL ) );
						}
					}
					break;

					case MENU_REMOVE_SEL:
					{
						int index = _SendMessageW( g_hWnd_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
						if ( index != -1 )
						{
							TCITEM tie;
							_memzero( &tie, sizeof( TCITEM ) );
							tie.mask = TCIF_PARAM; // Get the lparam value
							_SendMessageW( g_hWnd_tab, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
							
							if ( ( HWND )tie.lParam == g_hWnd_list )
							{
								if ( _MessageBoxW( hWnd, ST_PROMPT_remove_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
								{
									removeinfo *ri = ( removeinfo * )GlobalAlloc( GMEM_FIXED, sizeof( removeinfo ) );
									ri->disable_critical_section = false;
									ri->hWnd = g_hWnd_list;

									// ri will be freed in the remove_items thread.
									CloseHandle( ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL ) );
								}
							}
							else if ( ( HWND )tie.lParam == g_hWnd_contact_list )
							{
								// Remove the first selected (not the focused & selected).
								LVITEM lvi;
								_memzero( &lvi, sizeof( LVITEM ) );
								lvi.mask = LVIF_PARAM;
								lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_SELECTED );//_SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

								if ( lvi.iItem != -1 )
								{
									_SendMessageW( g_hWnd_contact_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

									if ( _MessageBoxW( hWnd, ST_PROMPT_delete_contact, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
									{
										CloseHandle( ( HANDLE )_CreateThread( NULL, 0, DeleteContact, ( void * )( ( contactinfo * )lvi.lParam ), 0, NULL ) );
									}
								}
							}
							else if ( ( HWND )tie.lParam == g_hWnd_ignore_list )
							{
								// Retrieve the lParam value from the selected listview item.
								LVITEM lvi;
								_memzero( &lvi, sizeof( LVITEM ) );
								lvi.mask = LVIF_PARAM;
								lvi.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

								if ( lvi.iItem != -1 )
								{
									if ( _MessageBoxW( hWnd, ST_PROMPT_remove_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
									{
										ignoreupdateinfo *iui = ( ignoreupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreupdateinfo ) );
										iui->ii = NULL;
										iui->phone_number = NULL;
										iui->action = 1;	// 1 = Remove, 0 = Add
										iui->hWnd = g_hWnd_ignore_list;

										// iui is freed in the update_ignore_list thread.
										CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_list, ( void * )iui, 0, NULL ) );
									}
								}
							}
							else if ( ( HWND )tie.lParam == g_hWnd_forward_list )
							{
								// Retrieve the lParam value from the selected listview item.
								LVITEM lvi;
								_memzero( &lvi, sizeof( LVITEM ) );
								lvi.mask = LVIF_PARAM;
								lvi.iItem = _SendMessageW( g_hWnd_forward_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

								if ( lvi.iItem != -1 )
								{
									if ( _MessageBoxW( hWnd, ST_PROMPT_remove_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
									{
										forwardupdateinfo *fui = ( forwardupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardupdateinfo ) );
										fui->fi = NULL;
										fui->call_from = NULL;
										fui->forward_to = NULL;
										fui->action = 1;	// 1 = Remove, 0 = Add
										fui->hWnd = g_hWnd_forward_list;

										// fui is freed in the update_forward_list thread.
										CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_forward_list, ( void * )fui, 0, NULL ) );
									}
								}
							}
						}
					}
					break;

					case MENU_SELECT_ALL:
					{
						int index = _SendMessageW( g_hWnd_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
						if ( index != -1 )
						{
							TCITEM tie;
							_memzero( &tie, sizeof( TCITEM ) );
							tie.mask = TCIF_PARAM; // Get the lparam value
							_SendMessageW( g_hWnd_tab, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
							
							// Set the state of all items to selected.
							LVITEM lvi;
							_memzero( &lvi, sizeof( LVITEM ) );
							lvi.mask = LVIF_STATE;
							lvi.state = LVIS_SELECTED;
							lvi.stateMask = LVIS_SELECTED;
							_SendMessageW( ( HWND )tie.lParam, LVM_SETITEMSTATE, -1, ( LPARAM )&lvi );

							UpdateMenus( UM_ENABLE );
						}
					}
					break;

					case MENU_INCOMING_IGNORE:
					{
						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );
						lvi.mask = LVIF_PARAM;
						lvi.iItem = _SendMessageW( g_hWnd_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

						if ( lvi.iItem != -1 )
						{
							_SendMessageW( g_hWnd_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

							displayinfo *di = ( displayinfo * )lvi.lParam;

							di->process_incoming = false;
							di->ci.ignored = true;

							_InvalidateRect( g_hWnd_list, NULL, TRUE );

							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, IgnoreIncomingCall, ( void * )&( di->ci ), 0, NULL ) );
						}
					}
					break;

					case MENU_OPEN_WEB_PAGE:
					{
						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );
						lvi.mask = LVIF_PARAM;
						lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

						if ( lvi.iItem != -1 )
						{
							_SendMessageW( g_hWnd_contact_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

							contactinfo *ci = ( contactinfo * )lvi.lParam;
							if ( ci != NULL && ci->web_page != NULL )
							{
								_ShellExecuteW( NULL, L"open", ci->web_page, NULL, NULL, SW_SHOWNORMAL );
							}
						}
					}
					break;

					case MENU_OPEN_EMAIL_ADDRESS:
					{
						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );
						lvi.mask = LVIF_PARAM;
						lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

						if ( lvi.iItem != -1 )
						{
							_SendMessageW( g_hWnd_contact_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

							contactinfo *ci = ( contactinfo * )lvi.lParam;
							if ( ci != NULL && ci->email_address != NULL )
							{
								int email_address_length = lstrlenW( ci->email_address );
								wchar_t *mailto = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( email_address_length + 8 ) );
								_wmemcpy_s( mailto, email_address_length + 8, L"mailto:", 7 );
								_wmemcpy_s( mailto + 7, email_address_length + 1, ci->email_address, email_address_length );
								mailto[ email_address_length + 7 ] = 0;	// Sanity.

								_ShellExecuteW( NULL, L"open", mailto, NULL, NULL, SW_SHOWNORMAL );

								GlobalFree( mailto );
							}
						}
					}
					break;

					case MENU_CALL_PHONE_COL14:
					case MENU_CALL_PHONE_COL16:
					case MENU_CALL_PHONE_COL18:
					case MENU_CALL_PHONE_COL21:
					case MENU_CALL_PHONE_COL25:
					case MENU_CALL_PHONE_COL27:
					case MENU_CALL_PHONE_COL211:
					case MENU_CALL_PHONE_COL212:
					case MENU_CALL_PHONE_COL216:
					case MENU_CALL_PHONE_COL31:
					case MENU_CALL_PHONE_COL32:
					case MENU_CALL_PHONE_COL41:
					case MENU_DIAL_PHONE_NUM:
					{
						char *call_to = NULL;

						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );
						lvi.mask = LVIF_PARAM;

						if ( LOWORD( wParam ) == MENU_CALL_PHONE_COL14 ||
							 LOWORD( wParam ) == MENU_CALL_PHONE_COL16 ||
							 LOWORD( wParam ) == MENU_CALL_PHONE_COL18 )
						{
							lvi.iItem = _SendMessageW( g_hWnd_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

							if ( lvi.iItem != -1 )
							{
								_SendMessageW( g_hWnd_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

								switch ( LOWORD( wParam ) )
								{
									case MENU_CALL_PHONE_COL14: { call_to = ( ( displayinfo * )lvi.lParam )->ci.forward_to; } break;
									case MENU_CALL_PHONE_COL16: { call_to = ( ( displayinfo * )lvi.lParam )->ci.call_from; } break;
									case MENU_CALL_PHONE_COL18: { call_to = ( ( displayinfo * )lvi.lParam )->ci.call_to; } break;
								}
							}
						}
						else if ( LOWORD( wParam ) == MENU_CALL_PHONE_COL21 ||
								  LOWORD( wParam ) == MENU_CALL_PHONE_COL25 ||
								  LOWORD( wParam ) == MENU_CALL_PHONE_COL27 ||
								  LOWORD( wParam ) == MENU_CALL_PHONE_COL211 ||
								  LOWORD( wParam ) == MENU_CALL_PHONE_COL212 || 
								  LOWORD( wParam ) == MENU_CALL_PHONE_COL216 )
						{
							lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

							if ( lvi.iItem != -1 )
							{
								_SendMessageW( g_hWnd_contact_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

								switch ( LOWORD( wParam ) )
								{
									case MENU_CALL_PHONE_COL21: { call_to = ( ( contactinfo * )lvi.lParam )->contact.cell_phone_number; } break;
									case MENU_CALL_PHONE_COL25: { call_to = ( ( contactinfo * )lvi.lParam )->contact.fax_number; } break;
									case MENU_CALL_PHONE_COL27: { call_to = ( ( contactinfo * )lvi.lParam )->contact.home_phone_number; } break;
									case MENU_CALL_PHONE_COL211: { call_to = ( ( contactinfo * )lvi.lParam )->contact.office_phone_number; } break;
									case MENU_CALL_PHONE_COL212: { call_to = ( ( contactinfo * )lvi.lParam )->contact.other_phone_number; } break;
									case MENU_CALL_PHONE_COL216: { call_to = ( ( contactinfo * )lvi.lParam )->contact.work_phone_number; } break;
								}
							}
						}
						else if ( LOWORD( wParam ) == MENU_CALL_PHONE_COL31 ||
								  LOWORD( wParam ) == MENU_CALL_PHONE_COL32 )
						{
							lvi.iItem = _SendMessageW( g_hWnd_forward_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

							if ( lvi.iItem != -1 )
							{
								_SendMessageW( g_hWnd_forward_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

								switch ( LOWORD( wParam ) )
								{
									case MENU_CALL_PHONE_COL31: { call_to = ( ( forwardinfo * )lvi.lParam )->c_forward_to; } break;
									case MENU_CALL_PHONE_COL32: { call_to = ( ( forwardinfo * )lvi.lParam )->c_call_from; } break;
								}
							}
						}
						else if ( LOWORD( wParam ) == MENU_CALL_PHONE_COL41 )
						{
							lvi.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

							if ( lvi.iItem != -1 )
							{
								_SendMessageW( g_hWnd_ignore_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

								call_to = ( ( ignoreinfo * )lvi.lParam )->c_phone_number;
							}
						}

						if ( g_hWnd_dial == NULL )
						{
							g_hWnd_dial = _CreateWindowExW( ( cfg_always_on_top == true ? WS_EX_TOPMOST : 0 ), L"phone", ST_Dial_Phone_Number, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 205 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 255 ) / 2 ), 205, 255, NULL, NULL, NULL, ( LPVOID )0 );
						}

						_SendMessageW( g_hWnd_dial, WM_PROPAGATE, 0, ( LPARAM )call_to );

						_SetForegroundWindow( g_hWnd_dial );
					}
					break;

					case MENU_ADD_IGNORE_LIST:	// ignore list listview right click.
					{
						if ( g_hWnd_ignore_phone_number == NULL )
						{
							// Allow wildcard input. (Last parameter of CreateWindow is 1)
							g_hWnd_ignore_phone_number = _CreateWindowExW( ( cfg_always_on_top == true ? WS_EX_TOPMOST : 0 ), L"phone", ST_Ignore_Phone_Number, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 205 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 255 ) / 2 ), 205, 255, NULL, NULL, NULL, ( LPVOID )1 );
						}

						_SendMessageW( g_hWnd_ignore_phone_number, WM_PROPAGATE, 2, 0 );

						_SetForegroundWindow( g_hWnd_ignore_phone_number );
					}
					break;

					case MENU_IGNORE_LIST:	// call log listview right click.
					{
						// Retrieve the lParam value from the selected listview item.
						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );
						lvi.mask = LVIF_PARAM;
						lvi.iItem = _SendMessageW( g_hWnd_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

						if ( lvi.iItem != -1 )
						{
							_SendMessageW( g_hWnd_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

							ignoreupdateinfo *iui = ( ignoreupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreupdateinfo ) );
							iui->ii = NULL;
							iui->phone_number = NULL;
							iui->action = ( ( ( displayinfo * )lvi.lParam )->ignore == true ? 1 : 0 );	// 1 = Remove, 0 = Add
							iui->hWnd = g_hWnd_list;

							// iui is freed in the update_ignore_list thread.
							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_list, ( void * )iui, 0, NULL ) );
						}
					}
					break;

					case MENU_INCOMING_FORWARD:
					case MENU_EDIT_FORWARD_LIST:
					case MENU_ADD_FORWARD_LIST:	// forward list listview right click.
					{
						if ( g_hWnd_forward == NULL )
						{
							// Show forward window with the forward edit box enabled and allow wildcard input. (Last parameter of CreateWindow is 2)
							g_hWnd_forward = _CreateWindowExW( ( cfg_always_on_top == true ? WS_EX_TOPMOST : 0 ), L"phone", ST_Forward_Phone_Number, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 205 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 295 ) / 2 ), 205, 295, NULL, NULL, NULL, ( LPVOID )2 );
						}

						if ( LOWORD( wParam ) == MENU_ADD_FORWARD_LIST )
						{
							_SendMessageW( g_hWnd_forward, WM_PROPAGATE, 1, 0 );
						}
						else
						{
							LVITEM lvi;
							_memzero( &lvi, sizeof( LVITEM ) );
							lvi.mask = LVIF_PARAM;

							if ( LOWORD( wParam ) == MENU_EDIT_FORWARD_LIST )
							{
								lvi.iItem = _SendMessageW( g_hWnd_forward_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

								if ( lvi.iItem != -1 )
								{
									_SendMessageW( g_hWnd_forward_list, LVM_GETITEM, 0, ( LPARAM )&lvi );
									_SendMessageW( g_hWnd_forward, WM_PROPAGATE, 3, lvi.lParam );
								}
							}
							else if ( LOWORD( wParam ) == MENU_INCOMING_FORWARD )
							{
								lvi.iItem = _SendMessageW( g_hWnd_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

								if ( lvi.iItem != -1 )
								{
									_SendMessageW( g_hWnd_list, LVM_GETITEM, 0, ( LPARAM )&lvi );
									_SendMessageW( g_hWnd_forward, WM_PROPAGATE, 4, lvi.lParam );
								}
							}
						}

						_SetForegroundWindow( g_hWnd_forward );
					}
					break;

					case MENU_FORWARD_LIST:	// call log listview right click.
					{
						// Retrieve the lParam value from the selected listview item.
						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );
						lvi.mask = LVIF_PARAM;
						lvi.iItem = _SendMessageW( g_hWnd_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

						if ( lvi.iItem != -1 )
						{
							_SendMessageW( g_hWnd_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

							// This item we've selected is in the forwardlist tree.
							if ( ( ( displayinfo * )lvi.lParam )->forward == true )
							{
								forwardupdateinfo *fui = ( forwardupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardupdateinfo ) );
								fui->fi = NULL;
								fui->call_from = NULL;
								fui->forward_to = NULL;
								fui->action = 1;	// 1 = Remove, 0 = Add
								fui->hWnd = g_hWnd_list;

								// Remove items.
								CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_forward_list, ( void * )fui, 0, NULL ) );
							}
							else	// The item we've selected is not in the forwardlist tree. Prompt the user for a number to forward.
							{
								if ( g_hWnd_forward == NULL )
								{
									// Show forward window with the forward edit box enabled and allow wildcard input. (Last parameter of CreateWindow is 2)
									g_hWnd_forward = _CreateWindowExW( ( cfg_always_on_top == true ? WS_EX_TOPMOST : 0 ), L"phone", ST_Forward_Phone_Number, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 205 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 295 ) / 2 ), 205, 295, NULL, NULL, NULL, ( LPVOID )2 );
								}

								_SendMessageW( g_hWnd_forward, WM_PROPAGATE, 1, lvi.lParam );

								_SetForegroundWindow( g_hWnd_forward );
							}
						}
					}
					break;

					case MENU_SELECT_COLUMNS:
					{
						if ( g_hWnd_columns == NULL )
						{
							// Show columns window.
							g_hWnd_columns = _CreateWindowExW( ( cfg_always_on_top == true ? WS_EX_TOPMOST : 0 ), L"columns", ST_Select_Columns, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 425 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 260 ) / 2 ), 425, 260, NULL, NULL, NULL, NULL );
						}

						int index = _SendMessageW( g_hWnd_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
						if ( index != -1 )
						{
							TCITEM tie;
							_memzero( &tie, sizeof( TCITEM ) );
							tie.mask = TCIF_PARAM; // Get the lparam value
							_SendMessageW( g_hWnd_tab, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
							
							if ( ( HWND )tie.lParam == g_hWnd_list )
							{
								index = 0;
							}
							else if ( ( HWND )tie.lParam == g_hWnd_contact_list )
							{
								index = 1;
							}
							else if ( ( HWND )tie.lParam == g_hWnd_forward_list )
							{
								index = 2;
							}
							else if ( ( HWND )tie.lParam == g_hWnd_ignore_list )
							{
								index = 3;
							}
						}
						
						_SendMessageW( g_hWnd_columns, WM_PROPAGATE, index, 0 );
						
						_SetForegroundWindow( g_hWnd_columns );
					}
					break;

					case MENU_EDIT_CONTACT:
					case MENU_ADD_CONTACT:
					{
						if ( g_hWnd_contact == NULL )
						{
							g_hWnd_contact = _CreateWindowExW( ( cfg_always_on_top == true ? WS_EX_TOPMOST : 0 ), L"contact", ST_Contact_Information, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 550 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 290 ) / 2 ), 550, 290, NULL, NULL, NULL, NULL );
						}

						if ( LOWORD( wParam ) == MENU_EDIT_CONTACT )
						{
							LVITEM lvi;
							_memzero( &lvi, sizeof( LVITEM ) );
							lvi.mask = LVIF_PARAM;
							lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

							if ( lvi.iItem != -1 )
							{
								_SendMessageW( g_hWnd_contact_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

								_SendMessageW( g_hWnd_contact, WM_PROPAGATE, MAKEWPARAM( CW_MODIFY, 0 ), lvi.lParam );	// Edit contact.
							}
						}
						else
						{
							_SendMessageW( g_hWnd_contact, WM_PROPAGATE, MAKEWPARAM( CW_MODIFY, 0 ), 0 );	// Add contact.
						}

						_SetForegroundWindow( g_hWnd_contact );
					}
					break;

					case MENU_ACCOUNT:
					{
						if ( g_hWnd_account == NULL )
						{
							g_hWnd_account = _CreateWindowExW( ( cfg_always_on_top == true ? WS_EX_TOPMOST : 0 ), L"account", ST_Account_Information, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 290 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 290 ) / 2 ), 290, 290, NULL, NULL, NULL, NULL );
							_ShowWindow( g_hWnd_account, SW_SHOWNORMAL );
						}
						_SetForegroundWindow( g_hWnd_account );
					}
					break;

					case MENU_EXPORT_CONTACTS:
					{
						wchar_t *file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof ( wchar_t ) * MAX_PATH );
						_memzero( file_name, sizeof ( wchar_t ) * MAX_PATH );

						OPENFILENAME ofn;
						_memzero( &ofn, sizeof( OPENFILENAME ) );
						ofn.lStructSize = sizeof( OPENFILENAME );
						ofn.hwndOwner = hWnd;
						ofn.lpstrFilter = L"CSV (Comma delimited) (*.csv)\0*.csv\0" \
										  L"vCard Files (*.vcf)\0*.vcf\0" \
										  L"All Files (*.*)\0*.*\0";
						ofn.lpstrTitle = ST_Export_Contacts;
						ofn.lpstrFile = file_name;
						ofn.nMaxFile = MAX_PATH;
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_READONLY;

						if ( _GetSaveFileNameW( &ofn ) )
						{
							importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof ( importexportinfo ) );

							iei->file_path = file_name;

							iei->file_type = ( unsigned char )( ofn.nFilterIndex - 1 );

							// Add the file extension.
							if ( iei->file_type <= 1 )
							{
								bool append = true;

								// See if a file extension was typed.
								if ( ofn.nFileExtension > 0 )
								{
									// See if it's the same as our selected file type.
									if ( lstrcmpW( iei->file_path + ofn.nFileExtension, ( iei->file_type == 1 ? L"vcf" :  L"csv" ) ) == 0 )
									{
										append = false;
									}
								}

								if ( append == true )
								{
									int file_name_length = lstrlenW( iei->file_path + ofn.nFileOffset );

									// Append the file extension to the end of the file name.
									if ( ofn.nFileOffset + file_name_length + 4 < MAX_PATH )
									{
										_wmemcpy_s( iei->file_path + ofn.nFileOffset + file_name_length, MAX_PATH, ( iei->file_type == 1 ? L".vcf" :  L".csv" ), 4 );
										*( iei->file_path + ofn.nFileOffset + file_name_length + 4 ) = 0;	// Sanity.
									}
								}
							}

							// iei will be freed in the ExportContactList thread.
							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, ExportContactList, ( void * )iei, 0, NULL ) );
						}
						else
						{
							GlobalFree( file_name );
						}
					}
					break;

					case MENU_IMPORT_CONTACTS:
					{
						if ( contact_import_in_progress != UPDATE_END )
						{
							_EnableMenuItem( g_hMenu, MENU_IMPORT_CONTACTS, MF_DISABLED );

							contact_import_in_progress = UPDATE_FAIL;	// Stop the import.

							break;
						}

						wchar_t *file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof ( wchar_t ) * MAX_PATH );
						_memzero( file_name, sizeof ( wchar_t ) * MAX_PATH );

						OPENFILENAME ofn;
						_memzero( &ofn, sizeof( OPENFILENAME ) );
						ofn.lStructSize = sizeof( OPENFILENAME );
						ofn.hwndOwner = hWnd;
						ofn.lpstrFilter = L"CSV (Comma delimited) (*.csv)\0*.csv\0" \
										  L"vCard Files (*.vcf)\0*.vcf\0" \
										  L"All Files (*.*)\0*.*\0";
						ofn.lpstrTitle = ST_Import_Contacts;
						ofn.lpstrFile = file_name;
						ofn.nMaxFile = MAX_PATH;
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_READONLY;

						if ( _GetOpenFileNameW( &ofn ) )
						{
							importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof ( importexportinfo ) );

							iei->file_path = file_name;

							iei->file_type = ( unsigned char )( ofn.nFilterIndex - 1 );

							// iei will be freed in the ExportContactList thread.
							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, ImportContactList, ( void * )iei, 0, NULL ) );
						}
						else
						{
							GlobalFree( file_name );
						}
					}
					break;

					case MENU_SAVE_CALL_LOG:
					{
						wchar_t *file_path = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof ( wchar_t ) * MAX_PATH );
						_memzero( file_path, sizeof ( wchar_t ) * MAX_PATH );

						OPENFILENAME ofn;
						_memzero( &ofn, sizeof( OPENFILENAME ) );
						ofn.lStructSize = sizeof( OPENFILENAME );
						ofn.hwndOwner = hWnd;
						ofn.lpstrFilter = L"CSV (Comma delimited) (*.csv)\0*.csv\0";
						ofn.lpstrTitle = ST_Save_Call_Log;
						ofn.lpstrFile = file_path;
						ofn.nMaxFile = MAX_PATH;
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_READONLY;

						if ( _GetSaveFileNameW( &ofn ) )
						{
							bool append = true;

							// See if a file extension was typed.
							if ( ofn.nFileExtension > 0 )
							{
								// See if it's the same as our selected file type.
								if ( lstrcmpW( file_path + ofn.nFileExtension, L"csv" ) == 0 )
								{
									append = false;
								}
							}

							if ( append == true )
							{
								int file_name_length = lstrlenW( file_path + ofn.nFileOffset );

								// Append the file extension to the end of the file name.
								if ( ofn.nFileOffset + file_name_length + 4 < MAX_PATH )
								{
									_wmemcpy_s( file_path + ofn.nFileOffset + file_name_length, MAX_PATH, L".csv", 4 );
									*( file_path + ofn.nFileOffset + file_name_length + 4 ) = 0;	// Sanity.
								}
							}

							// file_path will be freed in the save_call_log thread.
							CloseHandle( ( HANDLE )_CreateThread( NULL, 0, save_call_log, ( void * )file_path, 0, NULL ) );
						}
						else
						{
							GlobalFree( file_path );
						}
					}
					break;

					case MENU_OPTIONS:
					{
						if ( g_hWnd_options == NULL )
						{
							g_hWnd_options = _CreateWindowExW( ( cfg_always_on_top == true ? WS_EX_TOPMOST : 0 ), L"options", ST_Options, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 580 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 385 ) / 2 ), 580, 385, NULL, NULL, NULL, NULL );
							_ShowWindow( g_hWnd_options, SW_SHOWNORMAL );
						}
						_SetForegroundWindow( g_hWnd_options );
					}
					break;

					case MENU_ABOUT:
					{
						_MessageBoxW( hWnd, L"VZ Enhanced is made free under the GPLv3 license.\n\nCopyright \xA9 2013-2014 Eric Kutcher", PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
					}
					break;

					case MENU_RESTORE:
					{
						if ( _IsIconic( hWnd ) )	// If minimized, then restore the window.
						{
							_ShowWindow( hWnd, SW_RESTORE );
						}
						else if ( _IsWindowVisible( hWnd ) == TRUE )	// If already visible, then flash the window.
						{
							_FlashWindow( hWnd, TRUE );
						}
						else	// If hidden, then show the window.
						{
							_ShowWindow( hWnd, SW_SHOW );
						}
					}
					break;

					case MENU_EXIT:
					{
						_SendMessageW( hWnd, WM_EXIT, 0, 0 );
					}
					break;

					case MENU_START_WEB_SERVER:
					{
						if ( web_server_state == WEB_SERVER_STATE_RUNNING )
						{
							StartWebServer();
						}
					}
					break;

					case MENU_RESTART_WEB_SERVER:
					{
						if ( web_server_state == WEB_SERVER_STATE_RUNNING )
						{
							RestartWebServer();
						}
					}
					break;

					case MENU_STOP_WEB_SERVER:
					{
						if ( web_server_state == WEB_SERVER_STATE_RUNNING )
						{
							StopWebServer();
						}
					}
					break;

					case MENU_CONNECTION_MANAGER:
					{
						if ( web_server_state == WEB_SERVER_STATE_RUNNING )
						{
							_ShowWindow( g_hWnd_connection_manager, SW_SHOWNORMAL );
							_SetForegroundWindow( g_hWnd_connection_manager );
						}
					}
					break;
				}
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

					TCITEM tie;
					_memzero( &tie, sizeof( TCITEM ) );
					tie.mask = TCIF_PARAM; // Get the lparam value
					int index = _SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( ( HWND )( tie.lParam ), SW_SHOW );

						_InvalidateRect( nmhdr->hwndFrom, NULL, TRUE );	// Repaint the control
					}

					ChangeMenuByHWND( ( HWND )( tie.lParam ) );

					UpdateMenus( UM_ENABLE );

					return FALSE;
                }
				break;

				case HDN_ENDDRAG:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;
					HWND hWnd_parent = _GetParent( nmh->hdr.hwndFrom );

					// Prevent the # columns from moving and the other columns from becoming the first column.
					if ( nmh->iItem == 0 || nmh->pitem->iOrder == 0 )
					{
						// Make sure the # columns are visible.
						if ( hWnd_parent == g_hWnd_list && *list_columns[ 0 ] != -1 )
						{
							return TRUE;
						}
						else if ( hWnd_parent == g_hWnd_contact_list && *contact_list_columns[ 0 ] != -1 )
						{
							return TRUE;
						}
						else if ( hWnd_parent == g_hWnd_ignore_list && *ignore_list_columns[ 0 ] != -1 )
						{
							return TRUE;
						}
						else if ( hWnd_parent == g_hWnd_forward_list && *forward_list_columns[ 0 ] != -1 )
						{
							return TRUE;
						}
					}

					return FALSE;
				}
				break;

				case HDN_ENDTRACK:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;

					int index = nmh->iItem;

					HWND parent = _GetParent( nmh->hdr.hwndFrom );
					if ( parent == g_hWnd_list )
					{
						index = GetVirtualIndexFromColumnIndex( index, list_columns, NUM_COLUMNS );

						if ( index != -1 )
						{
							*list_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
					else if ( parent == g_hWnd_contact_list )
					{
						index = GetVirtualIndexFromColumnIndex( index, contact_list_columns, NUM_COLUMNS2 );

						if ( index != -1 )
						{
							*contact_list_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
					else if ( parent == g_hWnd_forward_list )
					{
						index = GetVirtualIndexFromColumnIndex( index, forward_list_columns, NUM_COLUMNS3 );

						if ( index != -1 )
						{
							*forward_list_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
					else if ( parent == g_hWnd_ignore_list )
					{
						index = GetVirtualIndexFromColumnIndex( index, ignore_list_columns, NUM_COLUMNS4 );

						if ( index != -1 )
						{
							*ignore_list_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
				}
				break;

				case LVN_KEYDOWN:
				{
					// Make sure the control key is down and that we're not already in a worker thread. Prevents threads from queuing in case the user falls asleep on their keyboard.
					if ( _GetKeyState( VK_CONTROL ) & 0x8000 && in_worker_thread == false )
					{
						NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

						// Determine which key was pressed.
						switch ( ( ( LPNMLVKEYDOWN )lParam )->wVKey )
						{
							case 'A':	// Select all items if Ctrl + A is down and there are items in the list.
							{
								if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 ) > 0 )
								{
									_SendMessageW( hWnd, WM_COMMAND, MENU_SELECT_ALL, 0 );
								}
							}
							break;

							case 'C':	// Copy selected items if Ctrl + C is down and there are selected items in the list.
							{
								if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 )
								{
									_SendMessageW( hWnd, WM_COMMAND, MENU_COPY_SEL, 0 );
								}
							}
							break;

							case 'S':	// Save Call Log items if Ctrl + S is down and there are items in the list.
							{
								if ( _SendMessageW( g_hWnd_list, LVM_GETITEMCOUNT, 0, 0 ) > 0 )
								{
									_SendMessageW( hWnd, WM_COMMAND, MENU_SAVE_CALL_LOG, 0 );
								}
							}
							break;
						}
					}
				}
				break;

				case LVN_COLUMNCLICK:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					LVCOLUMN lvc;
					_memzero( &lvc, sizeof( LVCOLUMN ) );
					lvc.mask = LVCF_FMT | LVCF_ORDER;
					_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

					sortinfo si;
					si.column = lvc.iOrder;
					si.hWnd = nmlv->hdr.hwndFrom;

					if ( HDF_SORTUP & lvc.fmt )	// Column is sorted upward.
					{
						si.direction = 0;	// Now sort down.

						// Sort down
						lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) | HDF_SORTDOWN;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, ( WPARAM )nmlv->iSubItem, ( LPARAM )&lvc );

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )CompareFunc );
					}
					else if ( HDF_SORTDOWN & lvc.fmt )	// Column is sorted downward.
					{
						si.direction = 1;	// Now sort up.

						// Sort up
						lvc.fmt = lvc.fmt & ( ~HDF_SORTDOWN ) | HDF_SORTUP;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )CompareFunc );
					}
					else	// Column has no sorting set.
					{
						// Remove the sort format for all columns.
						for ( int i = 0; i < ( nmlv->hdr.hwndFrom == g_hWnd_list ? total_columns : NUM_COLUMNS2 ); i++ )
						{
							// Get the current format
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, i, ( LPARAM )&lvc );
							// Remove sort up and sort down
							lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) & ( ~HDF_SORTDOWN );
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, i, ( LPARAM )&lvc );
						}

						// Read current the format from the clicked column
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

						si.direction = 0;	// Start the sort going down.

						// Sort down to start.
						lvc.fmt = lvc.fmt | HDF_SORTDOWN;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )CompareFunc );
					}
				}
				break;

				case NM_RCLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					HandleRightClick( nmitem->hdr.hwndFrom );
				}
				break;

				case LVN_DELETEITEM:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					// Item count will be 1 since the item hasn't yet been deleted.
					if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 ) == 1 )
					{
						// Disable the menus that require at least one item in the list.
						UpdateMenus( UM_DISABLE_OVERRIDE );
					}
				}
				break;

				case LVN_ITEMCHANGED:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					if ( in_worker_thread == false )
					{
						UpdateMenus( ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 ? UM_ENABLE : UM_DISABLE ) );
					}
				}
				break;
			}
			return FALSE;
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The item we want to draw is our listview.
			if ( dis->CtlType == ODT_LISTVIEW )
			{
				// Alternate item color's background.
				if ( dis->itemID % 2 )	// Even rows will have a light grey background.
				{
					HBRUSH color = _CreateSolidBrush( ( COLORREF )RGB( 0xF7, 0xF7, 0xF7 ) );
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
				}

				// Set the selected item's color.
				bool selected = false;
				if ( dis->itemState & ( ODS_FOCUS || ODS_SELECTED ) )
				{
					if ( ( skip_log_draw == true && dis->hwndItem == g_hWnd_list ) ||
						 ( skip_contact_draw == true && dis->hwndItem == g_hWnd_contact_list ) ||
						 ( skip_ignore_draw == true && dis->hwndItem == g_hWnd_ignore_list ) ||
						 ( skip_forward_draw == true && dis->hwndItem == g_hWnd_forward_list ) )
					{
						return TRUE;	// Don't draw selected items because their lParam values are being deleted.
					}

					HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
					selected = true;
				}

				// Get the item's text.
				wchar_t tbuf[ MAX_PATH ];
				wchar_t *buf = tbuf;
				LVITEM lvi;
				_memzero( &lvi, sizeof( LVITEM ) );
				lvi.mask = LVIF_PARAM;
				lvi.iItem = dis->itemID;
				_SendMessageW( dis->hwndItem, LVM_GETITEM, 0, ( LPARAM )&lvi );	// Get the lParam value from our item.

				// This is the full size of the row.
				RECT last_rc;
				last_rc.bottom = 0;
				last_rc.left = 0;
				last_rc.right = 0;
				last_rc.top = 0;

				// This will keep track of the current colunn's left position.
				int last_left = 0;

				int arr[ 17 ];
				int arr2[ 17 ];

				int column_count = 0;
				if ( dis->hwndItem == g_hWnd_list )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, total_columns, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, list_columns, NUM_COLUMNS, total_columns );

					column_count = total_columns;
				}
				else if ( dis->hwndItem == g_hWnd_contact_list )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, total_columns2, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS2 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, contact_list_columns, NUM_COLUMNS2, total_columns2 );

					column_count = total_columns2;
				}
				else if ( dis->hwndItem == g_hWnd_forward_list )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, total_columns3, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS3 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, forward_list_columns, NUM_COLUMNS3, total_columns3 );

					column_count = total_columns3;
				}
				else if ( dis->hwndItem == g_hWnd_ignore_list )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, total_columns4, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * 17, arr, sizeof( int ) * NUM_COLUMNS4 );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, ignore_list_columns, NUM_COLUMNS4, total_columns4 );

					column_count = total_columns4;
				}

				LVCOLUMN lvc;
				_memzero( &lvc, sizeof( LVCOLUMN ) );
				lvc.mask = LVCF_WIDTH;

				// Loop through all the columns
				for ( int i = 0; i < column_count; ++i )
				{
					if ( dis->hwndItem == g_hWnd_list )
					{
						// Save the appropriate text in our buffer for the current column.
						switch ( arr2[ i ] )
						{
							case 0:
							{
								buf = tbuf;	// Reset the buffer pointer.

								__snwprintf( buf, MAX_PATH, L"%lu", dis->itemID + 1 );
							}
							break;
							case 1:
							case 2:
							case 3:
							case 4:
							case 5:
							case 6:
							case 7:
							case 8: { buf = ( ( displayinfo * )lvi.lParam )->display_values[ arr2[ i ] - 1 ]; } break;
						}
					}
					else if ( dis->hwndItem == g_hWnd_contact_list )
					{
						switch ( arr2[ i ] )
						{
							case 0:
							{
								buf = tbuf;	// Reset the buffer pointer.

								__snwprintf( buf, MAX_PATH, L"%lu", dis->itemID + 1 );
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
							case 16:
							{
								buf = ( ( contactinfo * )lvi.lParam )->contactinfo_values[ arr2[ i ] - 1 ];
							}
							break;
						}
					}
					else if ( dis->hwndItem == g_hWnd_forward_list )
					{
						switch ( arr2[ i ] )
						{
							case 0:
							{
								buf = tbuf;	// Reset the buffer pointer.

								__snwprintf( buf, MAX_PATH, L"%lu", dis->itemID + 1 );
							}
							break;
							case 1:
							case 2:
							case 3: { buf = ( ( forwardinfo * )lvi.lParam )->forwardinfo_values[ arr2[ i ] - 1 ]; } break;
						}
					}
					else if ( dis->hwndItem == g_hWnd_ignore_list )
					{
						switch ( arr2[ i ] )
						{
							case 0:
							{
								buf = tbuf;	// Reset the buffer pointer.

								__snwprintf( buf, MAX_PATH, L"%lu", dis->itemID + 1 );
							}
							break;
							case 1:
							case 2: { buf = ( ( ignoreinfo * )lvi.lParam )->ignoreinfo_values[ arr2[ i ] - 1 ]; } break;
						}
					}

					if ( buf == NULL )
					{
						tbuf[ 0 ] = L'\0';
						buf = tbuf;
					}

					// Get the dimensions of the listview column
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMN, arr[ i ], ( LPARAM )&lvc );

					last_rc = dis->rcItem;

					// This will adjust the text to fit nicely into the rectangle.
					last_rc.left = 5 + last_left;
					last_rc.right = lvc.cx + last_left - 5;
					last_rc.top += 2;

					// Save the height and width of this region.
					int width = last_rc.right - last_rc.left;
					int height = last_rc.bottom - last_rc.top;

					// Normal text position.
					RECT rc;
					rc.left = 0;
					rc.top = 0;
					rc.right = width;
					rc.bottom = height;

					// Create and save a bitmap in memory to paint to.
					HDC hdcMem = _CreateCompatibleDC( dis->hDC );
					HBITMAP hbm = _CreateCompatibleBitmap( dis->hDC, width, height );
					HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
					_DeleteObject( ohbm );
					_DeleteObject( hbm );
					HFONT ohf = ( HFONT )_SelectObject( hdcMem, hFont );
					_DeleteObject( ohf );

					// Transparent background for text.
					_SetBkMode( hdcMem, TRANSPARENT );

					// Draw selected text
					if ( selected == true )
					{
						// Fill the background.
						HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
						_FillRect( hdcMem, &rc, color );
						_DeleteObject( color );

						// White text.
						_SetTextColor( hdcMem, RGB( 0xFF, 0xFF, 0xFF ) );
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS );
						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );
					}
					else	// Draw normal text.
					{
						// Fill the background.
						HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
						_FillRect( hdcMem, &rc, color );
						_DeleteObject( color );

						// Black text for normal entries, red for ignored, and orange for forwarded.
						if ( dis->hwndItem != g_hWnd_list )
						{
							_SetTextColor( hdcMem, RGB( 0x00, 0x00, 0x00 ) );
						}
						else
						{
							_SetTextColor( hdcMem, ( ( ( displayinfo * )lvi.lParam )->ci.ignored == true ? RGB( 0xFF, 0x00, 0x00 ) : ( ( displayinfo * )lvi.lParam )->ci.forwarded == true ? RGB( 0xFF, 0x80, 0x00 ) : RGB( 0x00, 0x00, 0x00 ) ) );
						}
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS );
						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCAND );
					}

					// Delete our back buffer.
					_DeleteDC( hdcMem );

					// Save the last left position of our column.
					last_left += lvc.cx;
				}
			}
			return TRUE;
		}
		break;

		case WM_SYSCOMMAND:
		{
			if ( wParam == SC_MINIMIZE && cfg_tray_icon == true && cfg_minimize_to_tray == true )
			{
				_ShowWindow( hWnd, SW_HIDE );

				return 0;
			}

			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;

		case WM_PROPAGATE:
		{
			if ( LOWORD( wParam ) == CW_MODIFY )
			{
				displayinfo *di = ( displayinfo * )lParam;	// lParam = our displayinfo structure from the connection thread.

				FormatDisplayInfo( di );

				// Insert a row into our listview.
				LVITEM lvi;
				_memzero( &lvi, sizeof( LVITEM ) );
				lvi.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
				lvi.iItem = _SendMessageW( g_hWnd_list, LVM_GETITEMCOUNT, 0, 0 );
				lvi.iSubItem = 0;
				lvi.lParam = lParam;
				int index = _SendMessageW( g_hWnd_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

				// 0 = show popups, etc. 1 = don't show.
				if ( HIWORD( wParam ) == 0 )
				{
					// Scroll to the newest entry.
					_SendMessageW( g_hWnd_list, LVM_ENSUREVISIBLE, index, FALSE );

					if ( cfg_enable_popups == true )
					{
						CreatePopup( di );
					}

					if ( web_server_state == WEB_SERVER_STATE_RUNNING )
					{
						UpdateCallLog( di );
					}
				}
			}
			else if ( LOWORD( wParam ) == CW_UPDATE )
			{
				// TRUE if we've started to update the contact.
				if ( HIWORD( wParam ) == UPDATE_BEGIN )
				{
					_SetWindowTextW( hWnd, ST_Importing_Contacts );

					MENUITEMINFO mii;
					_memzero( &mii, sizeof( MENUITEMINFO ) );
					mii.cbSize = sizeof( MENUITEMINFO );
					mii.fMask = MIIM_TYPE | MIIM_STATE;
					mii.fType = MFT_STRING;
					mii.dwTypeData = ST_Cancel_Import;
					mii.cch = 13;
					mii.fState = MFS_ENABLED;
					_SetMenuItemInfoW( g_hMenu, MENU_IMPORT_CONTACTS, FALSE, &mii );

					// Allow us to cancel.
					contact_import_in_progress = UPDATE_BEGIN;
				}
				else if ( HIWORD( wParam ) == UPDATE_END || HIWORD( wParam ) == UPDATE_FAIL )	// Sent from the ImportContactList thread if we've finished updating, or canceled the update.
				{
					_SetWindowTextW( hWnd, PROGRAM_CAPTION );

					MENUITEMINFO mii;
					_memzero( &mii, sizeof( MENUITEMINFO ) );
					mii.cbSize = sizeof( MENUITEMINFO );
					mii.fMask = MIIM_TYPE | MIIM_STATE;
					mii.dwTypeData = ST_Import_Contacts___;
					mii.cch = 18;
					mii.fState = MFS_ENABLED;
					_SetMenuItemInfoW( g_hMenu, MENU_IMPORT_CONTACTS, FALSE, &mii );

					// Reset the state to allow us to update again.
					contact_import_in_progress = UPDATE_END;
				}
				else if ( HIWORD( wParam ) == UPDATE_PROGRESS )	// Sent from the UploadMedia function in the ImportContactList thread with upload information.
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
					__snwprintf( title, 256, L"Importing Contacts - (%d.%1d%%) %lu / %lu bytes", i_percentage, remainder, upload_info->sent, upload_info->size );

					_SetWindowTextW( hWnd, title );
				}
			}
		}
		break;

		case WM_CLOSE:
		{
			if ( cfg_tray_icon == true && cfg_close_to_tray == true )
			{
				_ShowWindow( hWnd, SW_HIDE );
			}
			else
			{
				_SendMessageW( hWnd, WM_EXIT, 0, 0 );
			}
			return 0;
		}
		break;

		case WM_EXIT:
		{
			// Prevent the possibility of running additional processes.
			if ( g_hWnd_columns != NULL )
			{
				_EnableWindow( g_hWnd_columns, FALSE );
				_ShowWindow( g_hWnd_columns, SW_HIDE );
			}
			if ( g_hWnd_options != NULL )
			{
				_EnableWindow( g_hWnd_options, FALSE );
				_ShowWindow( g_hWnd_options, SW_HIDE );
			}
			if ( g_hWnd_contact != NULL )
			{
				_EnableWindow( g_hWnd_contact, FALSE );
				_ShowWindow( g_hWnd_contact, SW_HIDE );
			}
			if ( g_hWnd_account != NULL )
			{
				_EnableWindow( g_hWnd_account, FALSE );
				_ShowWindow( g_hWnd_account, SW_HIDE );
			}
			if ( g_hWnd_dial != NULL )
			{
				_EnableWindow( g_hWnd_dial, FALSE );
				_ShowWindow( g_hWnd_dial, SW_HIDE );
			}
			if ( g_hWnd_forward != NULL )
			{
				_EnableWindow( g_hWnd_forward, FALSE );
				_ShowWindow( g_hWnd_forward, SW_HIDE );
			}
			if ( g_hWnd_ignore_phone_number != NULL )
			{
				_EnableWindow( g_hWnd_ignore_phone_number, FALSE );
				_ShowWindow( g_hWnd_ignore_phone_number, SW_HIDE );
			}
			if ( web_server_state == WEB_SERVER_STATE_RUNNING && g_hWnd_connection_manager != NULL )
			{
				_EnableWindow( g_hWnd_connection_manager, FALSE );
				_ShowWindow( g_hWnd_connection_manager, SW_HIDE );
			}

			_EnableWindow( g_hWnd_login, FALSE );
			_ShowWindow( g_hWnd_login, SW_HIDE );
			_EnableWindow( hWnd, FALSE );
			_ShowWindow( hWnd, SW_HIDE );


			// If we're in a secondary thread, then kill it (cleanly) and wait for it to exit.
			if ( in_worker_thread == true || in_connection_thread == true || in_connection_worker_thread == true || in_connection_incoming_thread == true )
			{
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, cleanup, ( void * )NULL, 0, NULL ) );
			}
			else	// Otherwise, destroy the window normally.
			{
				worker_con.state = true;
				incoming_con.state = true;
				main_con.state = true;
				kill_worker_thead = true;
				_SendMessageW( hWnd, WM_DESTROY_ALT, 0, 0 );
			}
		}
		break;

		case WM_DESTROY_ALT:
		{
			if ( g_hWnd_columns != NULL )
			{
				_DestroyWindow( g_hWnd_columns );
			}

			if ( g_hWnd_options != NULL )
			{
				_DestroyWindow( g_hWnd_options );
			}

			if ( g_hWnd_contact != NULL )
			{
				_DestroyWindow( g_hWnd_contact );
			}

			if ( g_hWnd_account != NULL )
			{
				_DestroyWindow( g_hWnd_account );
			}

			if ( g_hWnd_dial != NULL )
			{
				_DestroyWindow( g_hWnd_dial );
			}

			if ( g_hWnd_forward != NULL )
			{
				_DestroyWindow( g_hWnd_forward );
			}

			if ( g_hWnd_ignore_phone_number != NULL )
			{
				_DestroyWindow( g_hWnd_ignore_phone_number );
			}

			if ( web_server_state == WEB_SERVER_STATE_RUNNING && g_hWnd_connection_manager != NULL )
			{
				_DestroyWindow( g_hWnd_connection_manager );
			}

			if ( cfg_enable_call_log_history == true && call_log_changed == true )
			{
				save_call_log_history();
			}

			// Get the number of items in the listview.
			int num_items = _SendMessageW( g_hWnd_list, LVM_GETITEMCOUNT, 0, 0 );

			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;

			// Go through each item, and free their lParam values.
			for ( int i = 0; i < num_items; ++i )
			{
				lvi.iItem = i;
				_SendMessageW( g_hWnd_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

				displayinfo *di = ( displayinfo * )lvi.lParam;
				free_displayinfo( &di );
			}

			UpdateColumnOrders();

			DestroyMenus();

			if ( cfg_tray_icon == true )
			{
				// Remove the icon from the notification area.
				_Shell_NotifyIconW( NIM_DELETE, &g_nid );
			}

			_DestroyWindow( hWnd );
		}
		break;

		case WM_DESTROY:
		{
			_PostQuitMessage( 0 );
			return 0;
		}
		break;

		case WM_TRAY_NOTIFY:
		{
			switch ( lParam )
			{
				case WM_LBUTTONDOWN:
				{
					if ( _IsWindowVisible( hWnd ) == FALSE )
					{
						_ShowWindow( hWnd, SW_SHOW );

						// Show the login window if we're logging in. We hide it if we're doing a silent startup...so it should be made visible.
						if ( login_state == LOGGING_IN )
						{
							_ShowWindow( g_hWnd_login, SW_SHOW );
							_SetForegroundWindow( g_hWnd_login );
						}
						else
						{
							_SetForegroundWindow( hWnd );
						}
					}
					else
					{
						_ShowWindow( hWnd, SW_HIDE );
					}
				}
				break;

				case WM_RBUTTONDOWN:
				case WM_CONTEXTMENU:
				{
					_SetForegroundWindow( hWnd );	// Must set so that the menu can close.

					// Show our edit context menu as a popup.
					POINT p;
					_GetCursorPos( &p ) ;
					_TrackPopupMenu( g_hMenuSub_tray, 0, p.x, p.y, 0, hWnd, NULL );
				}
				break;
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
