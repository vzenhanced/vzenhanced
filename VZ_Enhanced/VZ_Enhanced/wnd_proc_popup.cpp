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
#include "lite_gdi32.h"
#include "lite_winmm.h"

#define BTN_OK			1000

#define IDT_TIMER		2001

VOID CALLBACK TimerProc( HWND hWnd, UINT msg, UINT idTimer, DWORD dwTime )
{ 
	_KillTimer( hWnd, IDT_TIMER );

	_DestroyWindow( hWnd );
}

LRESULT CALLBACK PopupWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_PROPAGATE:
		{
			_SetWindowPos( hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

			DoublyLinkedList *ll = ( DoublyLinkedList * )_GetWindowLongW( hWnd, 0 );
			if ( ll != NULL && ll->data != NULL && ll->shared != NULL )
			{
				SHARED_SETTINGS *ss = ( SHARED_SETTINGS * )ll->shared;

				if ( ss->popup_play_sound == true )
				{
					bool play = true;
					#ifndef WINMM_USE_STATIC_LIB
						if ( winmm_state == WINMM_STATE_SHUTDOWN )
						{
							play = InitializeWinMM();
						}
					#endif

					if ( play == true )
					{
						_PlaySoundW( ss->popup_sound, NULL, SND_ASYNC | SND_FILENAME );
					}
				}

				_SetTimer( hWnd, IDT_TIMER, ss->popup_time * 1000, ( TIMERPROC )TimerProc );
			}
			else
			{
				_DestroyWindow( hWnd );
			}
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			DoublyLinkedList *ll = ( DoublyLinkedList * )_GetWindowLongW( hWnd, 0 );

			if ( ll != NULL && ll->shared != NULL )
			{
				SHARED_SETTINGS *ss = ( SHARED_SETTINGS * )ll->shared;

				if ( ss->popup_gradient == false )
				{
					HBRUSH background = _CreateSolidBrush( ss->popup_background_color1 );
					_FillRect( hdcMem, &client_rc, background );
					_DeleteObject( background );
				}
				else
				{
					// Stupid thing wants big-endian.
					TRIVERTEX vertex[ 2 ];
					vertex[ 0 ].x = 0;
					vertex[ 0 ].y = 0;
					vertex[ 0 ].Red = ( COLOR16 )GetRValue( ss->popup_background_color1 ) << 8;
					vertex[ 0 ].Green = ( COLOR16 )GetGValue( ss->popup_background_color1 ) << 8;
					vertex[ 0 ].Blue  = ( COLOR16 )GetBValue( ss->popup_background_color1 ) << 8;
					vertex[ 0 ].Alpha = 0x0000;

					vertex[ 1 ].x = client_rc.right - client_rc.left;
					vertex[ 1 ].y = client_rc.bottom - client_rc.top; 
					vertex[ 1 ].Red = ( COLOR16 )GetRValue( ss->popup_background_color2 ) << 8;
					vertex[ 1 ].Green = ( COLOR16 )GetGValue( ss->popup_background_color2 ) << 8;
					vertex[ 1 ].Blue  = ( COLOR16 )GetBValue( ss->popup_background_color2 ) << 8;
					vertex[ 1 ].Alpha = 0x0000;

					// Create a GRADIENT_RECT structure that references the TRIVERTEX vertices.
					GRADIENT_RECT gRc;
					gRc.UpperLeft = 0;
					gRc.LowerRight = 1;

					// Draw a shaded rectangle. 
					_GdiGradientFill( hdcMem, vertex, 2, &gRc, 1, ( ss->popup_gradient_direction == 1 ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V ) );
				}

				// Transparent background for text.
				_SetBkMode( hdcMem, TRANSPARENT );

				for ( int i = 0; i < 3; ++i )
				{
					if ( ll == NULL )
					{
						break;
					}

					POPUP_SETTINGS *p_s = ( POPUP_SETTINGS * )ll->data;
					ll = ll->next;

					if ( p_s->popup_line_order <= 0 )
					{
						continue;
					}

					// Reset ohf after drawing text.
					HFONT ohf = ( HFONT )_SelectObject( hdcMem, p_s->font );

					RECT rc_line;
					rc_line.bottom = 5;
					rc_line.left = 5;
					rc_line.right = 5;
					rc_line.top = 5;

					if ( i == 0 )
					{
						rc_line.right = client_rc.right - 5;
						rc_line.bottom = client_rc.bottom - 5;
						_DrawTextW( hdcMem, p_s->line_text, -1, &rc_line, DT_NOPREFIX | DT_CALCRECT );
						rc_line.right = client_rc.right - 5;
						rc_line.left = 5;
					}
					else if ( i == 1 )
					{
						rc_line.right = client_rc.right - 5;
						rc_line.bottom = client_rc.bottom - 5;
						_DrawTextW( hdcMem, p_s->line_text, -1, &rc_line, DT_NOPREFIX | DT_CALCRECT );
						rc_line.right = client_rc.right - 5;
						rc_line.left = 5;
						LONG tmp_height = rc_line.bottom - rc_line.top;
						rc_line.top = ( client_rc.bottom - ( rc_line.bottom - rc_line.top ) - 5 ) / 2;
						rc_line.bottom = rc_line.top + tmp_height;
					}
					else if ( i == 2 )
					{
						rc_line.right = client_rc.right - 5;
						rc_line.bottom = client_rc.bottom - 5;
						_DrawTextW( hdcMem, p_s->line_text, -1, &rc_line, DT_NOPREFIX | DT_CALCRECT );
						rc_line.top = client_rc.bottom - ( rc_line.bottom - rc_line.top ) - 5;
						rc_line.bottom = client_rc.bottom - 5;
						rc_line.left = 5;
						rc_line.right = client_rc.right - 5;
					}

					if ( p_s->font_shadow == true )
					{
						RECT rc_shadow_line = rc_line;
						rc_shadow_line.left += 2;
						rc_shadow_line.top += 2;
						rc_shadow_line.right += 2;
						rc_shadow_line.bottom += 2;

						_SetTextColor( hdcMem, p_s->font_shadow_color );

						_DrawTextW( hdcMem, p_s->line_text, -1, &rc_shadow_line, DT_NOPREFIX | ( p_s->popup_justify == 0 ? DT_LEFT : ( p_s->popup_justify == 1 ? DT_CENTER : DT_RIGHT ) ) );
					}

					_SelectObject( hdcMem, p_s->font );

					_SetTextColor( hdcMem, p_s->font_color );

					_DrawTextW( hdcMem, p_s->line_text, -1, &rc_line, DT_NOPREFIX | ( p_s->popup_justify == 0 ? DT_LEFT : ( p_s->popup_justify == 1 ? DT_CENTER : DT_RIGHT ) ) );

					_SelectObject( hdcMem, ohf );	// Reset the old font.
				}
			}

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_CAPTURECHANGED:
		{
			DoublyLinkedList *ll = ( DoublyLinkedList * )_GetWindowLongW( hWnd, 0 );
			if ( ll != NULL && ll->data != NULL )
			{
				POPUP_SETTINGS *ps = ( POPUP_SETTINGS * )ll->data;

				ps->window_settings.is_dragging = ( ( HWND )lParam == hWnd ) ? true : false;
			}
			return 0;
		}
		break;

		case WM_LBUTTONDOWN:
		{
			_SetCapture( hWnd );

			DoublyLinkedList *ll = ( DoublyLinkedList * )_GetWindowLongW( hWnd, 0 );
			if ( ll != NULL && ll->data != NULL )
			{
				POPUP_SETTINGS *ps = ( POPUP_SETTINGS * )ll->data;

				_GetCursorPos( &( ps->window_settings.drag_position ) );
				RECT rc;
				_GetWindowRect( hWnd, &rc );
				ps->window_settings.window_position.x = rc.left;
				ps->window_settings.window_position.y = rc.top;

				ps->window_settings.is_dragging = true;
			}
			return 0;
		}
		break;

		case WM_LBUTTONUP:
		{
			_ReleaseCapture();
			return 0;
		}
	    break;

		case WM_MOUSEMOVE:
		{
			DoublyLinkedList *ll = ( DoublyLinkedList * )_GetWindowLongW( hWnd, 0 );
			if ( ll != NULL && ll->data != NULL )
			{
				POPUP_SETTINGS *ps = ( POPUP_SETTINGS * )ll->data;

				// See if we have the mouse captured.
				if ( ps->window_settings.is_dragging == true )
				{
					POINT cur_pos;
					RECT wa;
					RECT rc;
					_GetWindowRect( hWnd, &rc );

					_GetCursorPos( &cur_pos );
					_OffsetRect( &rc, cur_pos.x - ( rc.left + ps->window_settings.drag_position.x - ps->window_settings.window_position.x ), cur_pos.y - ( rc.top + ps->window_settings.drag_position.y - ps->window_settings.window_position.y ) );

					// Allow our main window to attach to the desktop edge.
					_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );			
					if( is_close( rc.left, wa.left ) )				// Attach to left side of the desktop.
					{
						_OffsetRect( &rc, wa.left - rc.left, 0 );
					}
					else if ( is_close( wa.right, rc.right ) )		// Attach to right side of the desktop.
					{
						_OffsetRect( &rc, wa.right - rc.right, 0 );
					}

					if( is_close( rc.top, wa.top ) )				// Attach to top of the desktop.
					{
						_OffsetRect( &rc, 0, wa.top - rc.top );
					}
					else if ( is_close( wa.bottom, rc.bottom ) )	// Attach to bottom of the desktop.
					{
						_OffsetRect( &rc, 0, wa.bottom - rc.bottom );
					}

					_SetWindowPos( hWnd, NULL, rc.left, rc.top, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOSIZE );
				}
			}
			return 0;
		}
		break;

		case WM_NCHITTEST:
		{
			// Allow us to work in the client area, but nothing else (resize, title bar, etc.).
			return ( _DefWindowProcW( hWnd, msg, wParam, lParam ) == HTCLIENT ? HTCLIENT : NULL );
		}
		break;

		case WM_RBUTTONUP:
		case WM_CLOSE:
		{
			_DestroyWindow( hWnd );
		}
		break;

		case WM_DESTROY:
		{
			DoublyLinkedList *current_node = ( DoublyLinkedList * )_GetWindowLongW( hWnd, 0 );
			if ( current_node != NULL && current_node->data != NULL )
			{
				( ( POPUP_SETTINGS * )current_node->data )->window_settings.is_dragging = false;
			}

			// We can free the shared memory from the first node.
			if ( current_node != NULL && current_node->shared != NULL )
			{
				GlobalFree( ( ( SHARED_SETTINGS * )current_node->shared )->popup_sound );
				GlobalFree( ( ( SHARED_SETTINGS * )current_node->shared ) );
			}

			while ( current_node != NULL )
			{
				DoublyLinkedList *del_node = current_node;
				current_node = current_node->next;

				_DeleteObject( ( ( POPUP_SETTINGS * )del_node->data )->font );
				GlobalFree( ( ( POPUP_SETTINGS * )del_node->data )->font_face );
				GlobalFree( ( ( POPUP_SETTINGS * )del_node->data )->line_text );
				GlobalFree( ( ( POPUP_SETTINGS * )del_node->data ) );
				GlobalFree( del_node );
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
