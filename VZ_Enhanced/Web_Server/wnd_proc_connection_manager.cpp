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

#include "lite_ntdll.h"
#include "lite_gdi32.h"
#include "lite_user32.h"

#include "menus.h"
#include "connection.h"
#include "string_tables.h"

#define IDT_TIMER		1000

HWND g_hWnd_connections = NULL;

bool skip_connections_draw = false;

bool show_host_name = false;

VOID CALLBACK TimerProc( HWND hWnd, UINT msg, UINT idTimer, DWORD dwTime )
{
	// Redraw our image.
	_InvalidateRect( g_hWnd_connections, NULL, TRUE );
}

THREAD_RETURN close_connections( void *pArguments )
{
	EnterCriticalSection( &close_connection_cs );

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;
	lvi.iItem = -1;

	int sel_count = _SendMessageW( g_hWnd_connections, LVM_GETSELECTEDCOUNT, 0, 0 );

	int *index_array = NULL;

	_SendMessageW( g_hWnd_connections, LVM_ENSUREVISIBLE, 0, FALSE );

	index_array = ( int * )GlobalAlloc( GMEM_FIXED, sizeof( int ) * sel_count );

	lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

	// Create an index list of selected items (in reverse order).
	for ( int i = 0; i < sel_count; i++ )
	{
		lvi.iItem = index_array[ sel_count - 1 - i ] = _SendMessageW( g_hWnd_connections, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
	}

	for ( int i = 0; i < sel_count; ++i )
	{
		lvi.iItem = index_array[ i ];

		_SendMessageW( g_hWnd_connections, LVM_GETITEM, 0, ( LPARAM )&lvi );

		CONNECTION_INFO *ci = ( CONNECTION_INFO * )lvi.lParam;

		// Make sure we're not already in the process of shutting down or closing the connection.
		if ( ci->psc != NULL &&
		   ( ci->psc->io_context.IOOperation != ClientIoShutdown && ci->psc->io_context.IOOperation != ClientIoClose ) &&
		   ( ci->psc->io_context.NextIOOperation != ClientIoShutdown && ci->psc->io_context.NextIOOperation != ClientIoClose ) )
		{
			// Attempts a shutdown and then explicitly closes the connection (to force any stale connections to return from the completion port).
			BeginClose( ci->psc, ( use_ssl == true ? ClientIoShutdown : ClientIoClose ) );
		}
	}

	GlobalFree( index_array );

	LeaveCriticalSection( &close_connection_cs );

	_ExitThread( 0 );
	return 0;
}

// Sort function for columns.
int CALLBACK CMCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	sortinfo *si = ( sortinfo * )lParamSort;

	if ( si->hWnd == g_hWnd_connections )
	{
		CONNECTION_INFO *ci1 = ( CONNECTION_INFO * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		CONNECTION_INFO *ci2 = ( CONNECTION_INFO * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		switch ( si->column )
		{
			case 1: { return ( show_host_name == true ? _wcsicmp_s( ci1->l_host_name, ci2->l_host_name ) : _wcsicmp_s( ci1->l_ip, ci2->l_ip ) ); } break;
			case 2: { return _wcsicmp_s( ci1->l_port, ci2->l_port ); } break;

			case 3: { return ( show_host_name == true ? _wcsicmp_s( ci1->r_host_name, ci2->r_host_name ) : _wcsicmp_s( ci1->r_ip, ci2->r_ip ) ); } break;
			case 4: { return _wcsicmp_s( ci1->r_port, ci2->r_port ); } break;

			case 5: { return ( ci1->tx_bytes > ci2->tx_bytes ); } break;
			case 6: { return ( ci1->rx_bytes > ci2->rx_bytes ); } break;

			default:
			{
				return 0;
			}
			break;
		}	
	}

	return 0;
}

LRESULT CALLBACK ConnectionManagerWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_connections = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_connections, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES );

			// Initialize our listview columns
			LVCOLUMN lvc;
			_memzero( &lvc, sizeof( LVCOLUMN ) );
			lvc.mask = LVCF_WIDTH | LVCF_TEXT;

			lvc.pszText = ST_NUM;
			lvc.cx = 35;
			_SendMessageW( g_hWnd_connections, LVM_INSERTCOLUMN, 0, ( LPARAM )&lvc );

			lvc.pszText = ST_Remote_Address;
			lvc.cx = 150;
			_SendMessageW( g_hWnd_connections, LVM_INSERTCOLUMN, 1, ( LPARAM )&lvc );

			lvc.pszText = ST_Remote_Port;
			lvc.cx = 75;
			_SendMessageW( g_hWnd_connections, LVM_INSERTCOLUMN, 2, ( LPARAM )&lvc );

			lvc.pszText = ST_Local_Address;
			lvc.cx = 150;
			_SendMessageW( g_hWnd_connections, LVM_INSERTCOLUMN, 3, ( LPARAM )&lvc );

			lvc.pszText = ST_Local_Port;
			lvc.cx = 75;
			_SendMessageW( g_hWnd_connections, LVM_INSERTCOLUMN, 4, ( LPARAM )&lvc );

			lvc.pszText = ST_Sent_Bytes;
			lvc.cx = 100;
			_SendMessageW( g_hWnd_connections, LVM_INSERTCOLUMN, 5, ( LPARAM )&lvc );

			lvc.pszText = ST_Received_Bytes;
			lvc.cx = 100;
			_SendMessageW( g_hWnd_connections, LVM_INSERTCOLUMN, 6, ( LPARAM )&lvc );

			_SendMessageW( g_hWnd_connections, WM_SETFONT, ( WPARAM )hFont, 0 );

			CreateContextMenu();

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// Allow our listview to resize in proportion to the main window.
			HDWP hdwp = _BeginDeferWindowPos( 1 );
			_DeferWindowPos( hdwp, g_hWnd_connections, HWND_TOP, 0, 0, rc.right, rc.bottom, 0 );
			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_MEASUREITEM:
		{
			// Set the row height of the list view.
			if ( ( ( LPMEASUREITEMSTRUCT )lParam )->CtlType = ODT_LISTVIEW )
			{
				( ( LPMEASUREITEMSTRUCT )lParam )->itemHeight = row_height;
			}
			return TRUE;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
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
					case MENU_CLOSE_CONNECTION:
					{
						CloseHandle( ( HANDLE )_CreateThread( NULL, 0, close_connections, ( void * )NULL, 0, NULL ) );
					}
					break;

					case MENU_RESOLVE_ADDRESSES:
					{
						show_host_name = !show_host_name;
						_CheckMenuItem( g_hMenuSub_connection_manager, MENU_RESOLVE_ADDRESSES, ( show_host_name == true ? MF_CHECKED : MF_UNCHECKED ) );
						_InvalidateRect( g_hWnd_connections, NULL, TRUE );
					}
					break;

					case MENU_SELECT_ALL:
					{
						// Set the state of all items to selected.
						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );
						lvi.mask = LVIF_STATE;
						lvi.state = LVIS_SELECTED;
						lvi.stateMask = LVIS_SELECTED;
						_SendMessageW( g_hWnd_connections, LVM_SETITEMSTATE, -1, ( LPARAM )&lvi );

						UpdateMenus();
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

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )CMCompareFunc );
					}
					else if ( HDF_SORTDOWN & lvc.fmt )	// Column is sorted downward.
					{
						si.direction = 1;	// Now sort up.

						// Sort up
						lvc.fmt = lvc.fmt & ( ~HDF_SORTDOWN ) | HDF_SORTUP;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )CMCompareFunc );
					}
					else	// Column has no sorting set.
					{
						// Remove the sort format for all columns.
						for ( unsigned char i = 0; i < 7; ++i )
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

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )CMCompareFunc );
					}
				}
				break;

				case NM_RCLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_connections )
					{
						UpdateMenus();

						_ClientToScreen( nmitem->hdr.hwndFrom, &nmitem->ptAction );

						_TrackPopupMenu( g_hMenuSub_connection_manager, 0, nmitem->ptAction.x, nmitem->ptAction.y, 0, hWnd, NULL );
					}
				}
				break;

				case LVN_KEYDOWN:
				{
					// Make sure the control key is down and that we're not already in a worker thread. Prevents threads from queuing in case the user falls asleep on their keyboard.
					if ( _GetKeyState( VK_CONTROL ) & 0x8000 )
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
						}
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
			if ( dis->CtlType == ODT_LISTVIEW && dis->itemData != NULL )
			{
				CONNECTION_INFO *ci = ( CONNECTION_INFO * )dis->itemData;

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
					if ( skip_connections_draw == true && dis->hwndItem == g_hWnd_connections )
					{
						return TRUE;	// Don't draw selected items because their lParam values are being deleted.
					}

					HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
					selected = true;
				}

				wchar_t tbuf[ 21 ];
				wchar_t *buf = tbuf;

				// This is the full size of the row.
				RECT last_rc;

				// This will keep track of the current colunn's left position.
				int last_left = 0;

				LVCOLUMN lvc;
				_memzero( &lvc, sizeof( LVCOLUMN ) );
				lvc.mask = LVCF_WIDTH;

				// Loop through all the columns
				for ( int i = 0; i < 7; ++i )
				{
					// Save the appropriate text in our buffer for the current column.
					switch ( i )
					{
						case 0:
						{
							buf = tbuf;	// Reset the buffer pointer.

							__snwprintf( buf, 21, L"%lu", dis->itemID + 1 );
						}
						break;

						case 1:
						{
							if ( show_host_name == false )
							{
								buf = ci->r_ip;
							}
							else
							{
								buf = ci->r_host_name;
							}
						}
						break;

						case 2:
						{
							buf = ci->r_port;
						}
						break;

						case 3:
						{
							if ( show_host_name == false )
							{
								buf = ci->l_ip;
							}
							else
							{
								buf = ci->l_host_name;
							}
						}
						break;

						case 4:
						{
							buf = ci->l_port;
						}
						break;

						case 5:
						{
							buf = tbuf;	// Reset the buffer pointer.

							__snwprintf( buf, 21, L"%llu", ci->tx_bytes );
						}
						break;

						case 6:
						{
							buf = tbuf;	// Reset the buffer pointer.

							__snwprintf( buf, 21, L"%llu", ci->rx_bytes );
						}
						break;
					}

					if ( buf == NULL )
					{
						tbuf[ 0 ] = L'\0';
						buf = tbuf;
					}

					// Get the dimensions of the listview column
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMN, i, ( LPARAM )&lvc );

					last_rc = dis->rcItem;

					// This will adjust the text to fit nicely into the rectangle.
					last_rc.left = 5 + last_left;
					last_rc.right = lvc.cx + last_left - 5;

					// Save the last left position of our column.
					last_left += lvc.cx;

					// Save the height and width of this region.
					int width = last_rc.right - last_rc.left;
					if ( width <= 0 )
					{
						continue;
					}

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
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS );
						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );
					}
					else	// Draw normal text.
					{
						// Fill the background.
						HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
						_FillRect( hdcMem, &rc, color );
						_DeleteObject( color );

						// Black text.
						_SetTextColor( hdcMem, RGB( 0x00, 0x00, 0x00 ) );
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS );
						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCAND );
					}

					// Delete our back buffer.
					_DeleteDC( hdcMem );
				}
			}
			return TRUE;
		}
		break;

		case WM_ACTIVATE:
		{
			if ( LOWORD( wParam ) != WA_INACTIVE )
			{
				_SetTimer( hWnd, IDT_TIMER, 5000, ( TIMERPROC )TimerProc );
			}
		}
		break;

		case WM_CLOSE:
		{
			_KillTimer( hWnd, IDT_TIMER );

			_ShowWindow( hWnd, SW_HIDE );
		}
		break;

		case WM_DESTROY:
		{
			_KillTimer( hWnd, IDT_TIMER );

			*g_hWnd_connection_manager = NULL;
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
