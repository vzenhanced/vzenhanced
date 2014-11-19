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
#include "list_operations.h"
#include "utilities.h"
#include "menus.h"
#include "string_tables.h"

#include "connection.h"
#include "web_server.h"

bool skip_log_draw = false;				// Prevents WM_DRAWITEM from accessing listview items while we're removing them.
bool skip_contact_draw = false;
bool skip_ignore_draw = false;
bool skip_forward_draw = false;
bool skip_ignore_cid_draw = false;
bool skip_forward_cid_draw = false;

// If the caller ID name matches a value in our info structure, then set the state if we're adding or removing it.
void update_caller_id_name_matches( void *cidi, unsigned char info_type, bool add_value )
{
	if ( cidi == NULL )
	{
		return;
	}

	char *caller_id = NULL;
	bool match_case = true;
	bool match_whole_word = true;
	bool *active = NULL;

	if ( info_type == 0 )	// Handle ignore cid info.
	{
		caller_id = ( ( ignorecidinfo * )cidi )->c_caller_id;
		match_case = ( ( ignorecidinfo * )cidi )->match_case;
		match_whole_word = ( ( ignorecidinfo * )cidi )->match_whole_word;
		active = &( ( ( ignorecidinfo * )cidi )->active );
	}
	else if ( info_type == 1 )	// Handle forward cid info.
	{
		caller_id = ( ( forwardcidinfo * )cidi )->c_caller_id;
		match_case = ( ( forwardcidinfo * )cidi )->match_case;
		match_whole_word = ( ( forwardcidinfo * )cidi )->match_whole_word;
		active = &( ( ( forwardcidinfo * )cidi )->active );
	}
	else
	{
		return;
	}

	bool update_cid_state = false;

	// Update each displayinfo item to indicate that it is now ignored.
	node_type *node = dllrbt_get_head( call_log );
	while ( node != NULL )
	{
		DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
		while ( di_node != NULL )
		{
			displayinfo *mdi = ( displayinfo * )di_node->data;

			if ( match_case == true && match_whole_word == true )
			{
				if ( lstrcmpA( mdi->ci.caller_id, caller_id ) == 0 )
				{
					update_cid_state = true;
				}
			}
			else if ( match_case == false && match_whole_word == true )
			{
				if ( lstrcmpiA( mdi->ci.caller_id, caller_id ) == 0 )
				{
					update_cid_state = true;
				}
			}
			else if ( match_case == true && match_whole_word == false )
			{
				if ( _StrStrA( mdi->ci.caller_id, caller_id ) != NULL )
				{
					update_cid_state = true;
				}
			}
			else if ( match_case == false && match_whole_word == false )
			{
				if ( _StrStrIA( mdi->ci.caller_id, caller_id ) != NULL )
				{
					update_cid_state = true;
				}
			}

			if ( update_cid_state == true )
			{
				if ( add_value == true )
				{
					*active = true;

					if ( info_type == 0 )
					{
						++( mdi->ignore_cid_match_count );
						GlobalFree( mdi->w_ignore_caller_id );
						mdi->w_ignore_caller_id = GlobalStrDupW( ST_Yes );
					}
					else if ( info_type == 1 )
					{
						++( mdi->forward_cid_match_count );
						GlobalFree( mdi->w_forward_caller_id );
						mdi->w_forward_caller_id = GlobalStrDupW( ST_Yes );
					}
				}
				else
				{
					if ( info_type == 0 )
					{
						--( mdi->ignore_cid_match_count );

						if ( mdi->ignore_cid_match_count == 0 )
						{
							*active = false;

							GlobalFree( mdi->w_ignore_caller_id );
							mdi->w_ignore_caller_id = GlobalStrDupW( ST_No );
						}
					}
					else
					{
						--( mdi->forward_cid_match_count );

						if ( mdi->forward_cid_match_count == 0 )
						{
							*active = false;

							GlobalFree( mdi->w_forward_caller_id );
							mdi->w_forward_caller_id = GlobalStrDupW( ST_No );
						}
					}
				}

				update_cid_state = false;
			}

			di_node = di_node->next;
		}

		node = node->next;
	}
}

THREAD_RETURN remove_items( void *pArguments )
{
	removeinfo *ri = ( removeinfo * )pArguments;
	
	HWND hWnd = NULL;
	bool disable_critical_section = false;

	if ( ri != NULL )
	{
		disable_critical_section = ri->disable_critical_section;
		hWnd = ri->hWnd;

		GlobalFree( ri );
	}

	// This will block every other thread from entering until the first thread is complete.
	if ( disable_critical_section == false )
	{
		EnterCriticalSection( &pe_cs );
	}

	in_worker_thread = true;
	
	bool *skip_draw = NULL;

	// Prevent the listviews from drawing while freeing lParam values.
	if ( hWnd == g_hWnd_call_log )
	{
		skip_draw = &skip_log_draw;
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		skip_draw = &skip_contact_draw;
	}
	else if ( hWnd == g_hWnd_ignore_list )
	{
		skip_draw = &skip_ignore_draw;
	}
	else if ( hWnd == g_hWnd_forward_list )
	{
		skip_draw = &skip_forward_draw;
	}
	else if ( hWnd == g_hWnd_ignore_cid_list )
	{
		skip_draw = &skip_ignore_cid_draw;
	}
	else if ( hWnd == g_hWnd_forward_cid_list )
	{
		skip_draw = &skip_forward_cid_draw;
	}
	
	if ( skip_draw != NULL )
	{
		*skip_draw = true;
	}

	if ( disable_critical_section == false )
	{
		Processing_Window( hWnd, false );
	}

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;

	int item_count = _SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
	int sel_count = 1;

	// If we're removing contacts, then we can only do one at a time. Otherwise, handle all selected.
	if ( hWnd != g_hWnd_contact_list )
	{
		sel_count = _SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );
	}

	int *index_array = NULL;

	bool handle_all = false;
	if ( item_count == sel_count )
	{
		handle_all = true;
	}
	else
	{
		_SendMessageW( hWnd, LVM_ENSUREVISIBLE, 0, FALSE );

		index_array = ( int * )GlobalAlloc( GMEM_FIXED, sizeof( int ) * sel_count );

		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

		// Create an index list of selected items (in reverse order).
		for ( int i = 0; i < sel_count; i++ )
		{
			lvi.iItem = index_array[ sel_count - 1 - i ] = _SendMessageW( hWnd, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
		}

		item_count = sel_count;
	}

	// Go through each item, and free their lParam values.
	for ( int i = 0; i < item_count; ++i )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thead == true )
		{
			break;
		}

		if ( handle_all == true )
		{
			lvi.iItem = i;
		}
		else
		{
			lvi.iItem = index_array[ i ];
		}

		_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

		if ( hWnd == g_hWnd_call_log )
		{
			displayinfo *di = ( displayinfo * )lvi.lParam;

			if ( di != NULL )
			{
				// Update each displayinfo item.
				dllrbt_iterator *itr = dllrbt_find( call_log, ( void * )di->ci.call_from, false );
				if ( itr != NULL )
				{
					// Head of the linked list.
					DoublyLinkedList *ll = ( DoublyLinkedList * )( ( node_type * )itr )->val;

					// Go through each linked list node and remove the one with di.

					DoublyLinkedList *current_node = ll;
					while ( current_node != NULL )
					{
						if ( ( displayinfo * )current_node->data == di )
						{
							DLL_RemoveNode( &ll, current_node );
							GlobalFree( current_node );

							if ( ll != NULL && ll->data != NULL )
							{
								// Reset the head in the tree.
								( ( node_type * )itr )->val = ( void * )ll;
								( ( node_type * )itr )->key = ( void * )( ( displayinfo * )ll->data )->ci.call_from;

								call_log_changed = true;
							}

							break;
						}

						current_node = current_node->next;
					}

					// If the head of the linked list is NULL, then we can remove the linked list from the call log tree.
					if ( ll == NULL )
					{
						dllrbt_remove( call_log, itr );	// Remove the node from the tree. The tree will rebalance itself.

						call_log_changed = true;
					}
				}

				free_displayinfo( &di );
			}
		}
		else if ( hWnd == g_hWnd_contact_list )
		{
			contactinfo *ci = ( contactinfo * )lvi.lParam;

			if ( ci != NULL )
			{
				dllrbt_iterator *itr = dllrbt_find( contact_list, ( void * )ci->contact.contact_entry_id, false );
				if ( itr != NULL )
				{
					dllrbt_remove( contact_list, itr );	// Remove the node from the tree. The tree will rebalance itself.
				}

				remove_custom_caller_id( ci );

				free_contactinfo( &ci );
			}
		}
		else if ( hWnd == g_hWnd_ignore_list )
		{
			ignoreinfo *ii = ( ignoreinfo * )lvi.lParam;

			if ( ii != NULL )
			{
				char range_number[ 32 ];
				_memzero( range_number, 32 );

				int range_index = lstrlenA( ii->c_phone_number );
				range_index = ( range_index > 0 ? range_index - 1 : 0 );

				// If the number we've removed is a range, then remove it from the range list.
				if ( is_num( ii->c_phone_number ) == 1 )
				{
					RangeRemove( &ignore_range_list[ range_index ], ii->c_phone_number );

					// Update each displayinfo item to indicate that it is no longer ignored.
					node_type *node = dllrbt_get_head( call_log );
					while ( node != NULL )
					{
						DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
						while ( di_node != NULL )
						{
							displayinfo *mdi = ( displayinfo * )di_node->data;

							// Process display values that are set to be ignored.
							if ( mdi->ignore_phone_number == true )
							{
								// First, see if the display value falls within another range.
								if ( RangeSearch( &ignore_range_list[ range_index ], mdi->ci.call_from, range_number ) == true )
								{
									// If it does, then we'll skip the display value.
									break;
								}

								// Next, see if the display value exists as a non-range value.
								if ( dllrbt_find( ignore_list, ( void * )mdi->ci.call_from, false ) != NULL )
								{
									// If it does, then we'll skip the display value.
									break;
								}

								// Finally, see if the display item falls within the range we've removed.
								if ( RangeCompare( ii->c_phone_number, mdi->ci.call_from ) == true )
								{
									mdi->ignore_phone_number = false;
									GlobalFree( mdi->w_ignore_phone_number );
									mdi->w_ignore_phone_number = GlobalStrDupW( ST_No );
								}
							}

							di_node = di_node->next;
						}

						node = node->next;
					}
				}
				else
				{
					// See if the value we remove falls within a range. If it doesn't, then set its new display values.
					if ( RangeSearch( &ignore_range_list[ range_index ], ii->c_phone_number, range_number ) == false )
					{
						// Update each displayinfo item to indicate that it is no longer ignored.
						DoublyLinkedList *ll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )ii->c_phone_number, true );
						if ( ll != NULL )
						{
							DoublyLinkedList *di_node = ll;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								if ( mdi->ignore_phone_number == true )
								{
									mdi->ignore_phone_number = false;
									GlobalFree( mdi->w_ignore_phone_number );
									mdi->w_ignore_phone_number = GlobalStrDupW( ST_No );
								}

								di_node = di_node->next;
							}
						}
					}
				}

				// See if the ignore_list phone number exits. It should.
				dllrbt_iterator *itr = dllrbt_find( ignore_list, ( void * )ii->c_phone_number, false );
				if ( itr != NULL )
				{
					dllrbt_remove( ignore_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					ignore_list_changed = true;	// Causes us to save the ignore_list on shutdown.
				}

				free_ignoreinfo( &ii );
			}
		}
		else if ( hWnd == g_hWnd_forward_list )
		{
			forwardinfo *fi = ( forwardinfo * )lvi.lParam;

			if ( fi != NULL )
			{
				char range_number[ 32 ];
				_memzero( range_number, 32 );

				int range_index = lstrlenA( fi->c_call_from );
				range_index = ( range_index > 0 ? range_index - 1 : 0 );

				// If the number we've removed is a range, then remove it from the range list.
				if ( is_num( fi->c_call_from ) == 1 )
				{
					RangeRemove( &forward_range_list[ range_index ], fi->c_call_from );

					// Update each displayinfo item to indicate that it is no longer forwarded.
					node_type *node = dllrbt_get_head( call_log );
					while ( node != NULL )
					{
						DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
						while ( di_node != NULL )
						{
							displayinfo *mdi = ( displayinfo * )di_node->data;

							// Process display values that are set to be forwarded.
							if ( mdi->forward_phone_number == true )
							{
								// First, see if the display value falls within another range.
								if ( RangeSearch( &forward_range_list[ range_index ], mdi->ci.call_from, range_number ) == true )
								{
									// If it does, then we'll skip the display value.
									break;
								}

								// Next, see if the display value exists as a non-range value.
								if ( dllrbt_find( forward_list, ( void * )mdi->ci.call_from, false ) != NULL )
								{
									// If it does, then we'll skip the display value.
									break;
								}

								// Finally, see if the display item falls within the range we've removed.
								if ( RangeCompare( fi->c_call_from, mdi->ci.call_from ) == true )
								{
									mdi->forward_phone_number = false;
									GlobalFree( mdi->w_forward_phone_number );
									mdi->w_forward_phone_number = GlobalStrDupW( ST_No );
								}
							}

							di_node = di_node->next;
						}

						node = node->next;
					}
				}
				else
				{
					// See if the value we remove falls within a range. If it doesn't, then set its new display values.
					if ( RangeSearch( &forward_range_list[ range_index ], fi->c_call_from, range_number ) == false )
					{
						// Update each displayinfo item to indicate that it is no longer forwarded.
						DoublyLinkedList *ll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )fi->c_call_from, true );
						if ( ll != NULL )
						{
							DoublyLinkedList *di_node = ll;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								if ( mdi->forward_phone_number == true )
								{
									mdi->forward_phone_number = false;
									GlobalFree( mdi->w_forward_phone_number );
									mdi->w_forward_phone_number = GlobalStrDupW( ST_No );
								}

								di_node = di_node->next;
							}
						}
					}
				}

				// See if the forward_list phone number exits. It should.
				dllrbt_iterator *itr = dllrbt_find( forward_list, ( void * )fi->c_call_from, false );
				if ( itr != NULL )
				{
					dllrbt_remove( forward_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					forward_list_changed = true;	// Causes us to save the forward_list on shutdown.
				}

				free_forwardinfo( &fi );
			}
		}
		else if ( hWnd == g_hWnd_ignore_cid_list )
		{
			ignorecidinfo *icidi = ( ignorecidinfo * )lvi.lParam;

			if ( icidi != NULL )
			{
				update_caller_id_name_matches( ( void * )icidi, 0, false );

				// See if the ignore_cid_list value exits. It should.
				dllrbt_iterator *itr = dllrbt_find( ignore_cid_list, ( void * )icidi, false );
				if ( itr != NULL )
				{
					dllrbt_remove( ignore_cid_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					ignore_cid_list_changed = true;	// Causes us to save the ignore_cid_list on shutdown.
				}

				free_ignorecidinfo( &icidi );
			}
		}
		else if ( hWnd == g_hWnd_forward_cid_list )
		{
			forwardcidinfo *fcidi = ( forwardcidinfo * )lvi.lParam;

			if ( fcidi != NULL )
			{
				update_caller_id_name_matches( ( void * )fcidi, 1, false );

				// See if the forward_cid_list value exits. It should.
				dllrbt_iterator *itr = dllrbt_find( forward_cid_list, ( void * )fcidi, false );
				if ( itr != NULL )
				{
					dllrbt_remove( forward_cid_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					forward_cid_list_changed = true;	// Causes us to save the forward_cid_list on shutdown.
				}

				free_forwardcidinfo( &fcidi );
			}
		}

		if ( handle_all == false )
		{
			_SendMessageW( hWnd, LVM_DELETEITEM, index_array[ i ], 0 );
		}
	}

	if ( handle_all == true )
	{
		_SendMessageW( hWnd, LVM_DELETEALLITEMS, 0, 0 );
	}
	else
	{
		GlobalFree( index_array );
	}

	if ( skip_draw != NULL )
	{
		*skip_draw = false;
	}

	if ( disable_critical_section == false )
	{
		Processing_Window( hWnd, true );

		// Release the semaphore if we're killing the thread.
		if ( worker_semaphore != NULL )
		{
			ReleaseSemaphore( worker_semaphore, 1, NULL );
		}
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	if ( disable_critical_section == false )
	{
		LeaveCriticalSection( &pe_cs );
	}

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN update_ignore_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	HWND hWnd = NULL;
	ignoreupdateinfo *iui = ( ignoreupdateinfo * )pArguments;
	if ( iui != NULL )
	{
		hWnd = iui->hWnd;
	}

	Processing_Window( hWnd, false );

	if ( iui != NULL )
	{
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.
		lvi.iSubItem = 0;

		if ( iui->hWnd == g_hWnd_ignore_list )
		{
			if ( iui->action == 0 )	// Add a single item to ignore_list and ignore list listview.
			{
				// Zero init.
				ignoreinfo *ii = ( ignoreinfo * )GlobalAlloc( GPTR, sizeof( ignoreinfo ) );

				ii->c_phone_number = iui->phone_number;

				// Try to insert the ignore_list info in the tree.
				if ( dllrbt_insert( ignore_list, ( void * )ii->c_phone_number, ( void * )ii ) != DLLRBT_STATUS_OK )
				{
					free_ignoreinfo( &ii );

					_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_already_in_ignore_list );
				}
				else	// If it was able to be inserted into the tree, then update our displayinfo items and the ignore list listview.
				{
					ii->phone_number = FormatPhoneNumber( iui->phone_number );

					ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
					*ii->c_total_calls = '0';
					*( ii->c_total_calls + 1 ) = 0;	// Sanity

					int val_length = MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, val, val_length );

					ii->total_calls = val;

					ii->count = 0;
					ii->state = 0;

					// See if the value we're adding is a range (has wildcard values in it).
					if ( ii->c_phone_number != NULL && is_num( ii->c_phone_number ) == 1 )
					{
						int phone_number_length = lstrlenA( ii->c_phone_number );

						RangeAdd( &ignore_range_list[ ( phone_number_length > 0 ? phone_number_length - 1 : 0 ) ], ii->c_phone_number, phone_number_length );

						// Update each displayinfo item to indicate that it is now ignored.
						node_type *node = dllrbt_get_head( call_log );
						while ( node != NULL )
						{
							DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								// Process values that are not set to be ignored. See if the value falls within our range.
								if ( mdi->ignore_phone_number == false && RangeCompare( ii->c_phone_number, mdi->ci.call_from ) == true )
								{
									mdi->ignore_phone_number = true;
									GlobalFree( mdi->w_ignore_phone_number );
									mdi->w_ignore_phone_number = GlobalStrDupW( ST_Yes );
								}

								di_node = di_node->next;
							}

							node = node->next;
						}
					}
					else
					{
						// Update each displayinfo item to indicate that it is now ignored.
						DoublyLinkedList *ll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )ii->c_phone_number, true );
						if ( ll != NULL )
						{
							DoublyLinkedList *di_node = ll;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								if ( mdi->ignore_phone_number == false )
								{
									mdi->ignore_phone_number = true;
									GlobalFree( mdi->w_ignore_phone_number );
									mdi->w_ignore_phone_number = GlobalStrDupW( ST_Yes );
								}

								di_node = di_node->next;
							}
						}
					}

					ignore_list_changed = true;

					lvi.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )ii;	// lParam = our contactinfo structure from the connection thread.
					_SendMessageW( g_hWnd_ignore_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
				}
			}
			else if ( iui->action == 1 )	// Remove from ignore_list and ignore list listview.
			{
				removeinfo *ri = ( removeinfo * )GlobalAlloc( GMEM_FIXED, sizeof( removeinfo ) );
				ri->disable_critical_section = true;
				ri->hWnd = g_hWnd_ignore_list;

				// ri will be freed in the remove_items thread.
				HANDLE hThread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL );

				if ( hThread != NULL )
				{
					WaitForSingleObject( hThread, INFINITE );	// Wait at most 10 seconds for the remove_items thread to finish.
					CloseHandle( hThread );
				}
			}
			else if ( iui->action == 2 )	// Add all items in the ignore_list to the ignore list listview.
			{
				// Insert a row into our listview.
				node_type *node = dllrbt_get_head( ignore_list );
				while ( node != NULL )
				{
					ignoreinfo *ii = ( ignoreinfo * )node->val;

					if ( ii != NULL )
					{
						lvi.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )ii;	// lParam = our contactinfo structure from the connection thread.
						_SendMessageW( g_hWnd_ignore_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
					}

					node = node->next;
				}
			}
			else if ( iui->action == 3 )	// Update the recently called number's call count.
			{
				if ( iui->ii != NULL )
				{
					++( iui->ii->count );

					GlobalFree( iui->ii->c_total_calls );

					iui->ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 11 );
					__snprintf( iui->ii->c_total_calls, 11, "%lu", iui->ii->count );

					GlobalFree( iui->ii->total_calls );

					int val_length = MultiByteToWideChar( CP_UTF8, 0, iui->ii->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, iui->ii->c_total_calls, -1, val, val_length );

					iui->ii->total_calls = val;

					ignore_list_changed = true;
				}
			}

			_InvalidateRect( g_hWnd_call_log, NULL, TRUE );
		}
		else if ( iui->hWnd == g_hWnd_call_log )	// call log listview
		{
			int item_count = _SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = _SendMessageW( g_hWnd_call_log, LVM_GETSELECTEDCOUNT, 0, 0 );
			
			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				item_count = sel_count;
			}

			bool remove_state_changed = false;	// If true, then go through the ignore list listview and remove the items that have changed state.
			bool skip_range_warning = false;	// Skip the range warning if we've already displayed it.

			int last_selected = lvi.iItem;

			// Go through each item in the listview.
			for ( int i = 0; i < item_count; ++i )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thead == true )
				{
					break;
				}

				if ( handle_all == true )
				{
					lvi.iItem = i;
				}
				else
				{
					last_selected = lvi.iItem = _SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, last_selected, LVNI_SELECTED );
				}

				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				displayinfo *di = ( displayinfo * )lvi.lParam;

				if ( di->ignore_phone_number == true && iui->action == 1 )		// Remove from ignore_list.
				{
					char range_number[ 32 ];
					_memzero( range_number, 32 );

					int range_index = lstrlenA( di->ci.call_from );
					range_index = ( range_index > 0 ? range_index - 1 : 0 );

					// First, see if the phone number is in our range list.
					if ( RangeSearch( &ignore_range_list[ range_index ], di->ci.call_from, range_number ) == false )
					{
						// If not, then see if the phone number is in our ignore_list.
						dllrbt_iterator *itr = dllrbt_find( ignore_list, ( void * )di->ci.call_from, false );
						if ( itr != NULL )
						{
							// Update each displayinfo item to indicate that it is no longer ignored.
							DoublyLinkedList *ll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
							if ( ll != NULL )
							{
								DoublyLinkedList *di_node = ll;
								while ( di_node != NULL )
								{
									displayinfo *mdi = ( displayinfo * )di_node->data;

									if ( mdi->ignore_phone_number == true )
									{
										mdi->ignore_phone_number = false;
										GlobalFree( mdi->w_ignore_phone_number );
										mdi->w_ignore_phone_number = GlobalStrDupW( ST_No );
									}

									di_node = di_node->next;
								}
							}

							// If an ignore list listview item also shows up in the call log listview, then we'll remove it after this loop.
							ignoreinfo *ii = ( ignoreinfo * )( ( node_type * )itr )->val;
							ii->state = 1;
							remove_state_changed = true;	// This will let us go through the ignore list listview and free items with a state == 1.

							dllrbt_remove( ignore_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

							ignore_list_changed = true;	// Causes us to save the ignore_list on shutdown.
						}

						di->ignore_phone_number = false;
						GlobalFree( di->w_ignore_phone_number );
						di->w_ignore_phone_number = GlobalStrDupW( ST_No );
					}
					else
					{
						if ( skip_range_warning == false )
						{
							_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_remove_from_ignore_phone_number_list );
							skip_range_warning = true;
						}
					}
				}
				else if ( di->ignore_phone_number == false && iui->action == 0 )	// Add to ignore_list.
				{
					// Zero init.
					ignoreinfo *ii = ( ignoreinfo * )GlobalAlloc( GPTR, sizeof( ignoreinfo ) );

					ii->c_phone_number = GlobalStrDupA( di->ci.call_from );

					if ( dllrbt_insert( ignore_list, ( void * )ii->c_phone_number, ( void * )ii ) != DLLRBT_STATUS_OK )
					{
						free_ignoreinfo( &ii );

						_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_already_in_ignore_list );
					}
					else	// Add to ignore list listview as well.
					{
						ii->phone_number = FormatPhoneNumber( di->ci.call_from );

						ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
						*ii->c_total_calls = '0';
						*( ii->c_total_calls + 1 ) = 0;	// Sanity

						int val_length = MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
						wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
						MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, val, val_length );

						ii->total_calls = val;

						ii->count = 0;

						ii->state = 0;

						// Update all nodes if it already exits.
						DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
						if ( dll != NULL )
						{
							DoublyLinkedList *node = dll;
							while ( node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )node->data;

								if ( mdi->ignore_phone_number == false )
								{
									mdi->ignore_phone_number = true;
									GlobalFree( mdi->w_ignore_phone_number );
									mdi->w_ignore_phone_number = GlobalStrDupW( ST_Yes );
								}

								node = node->next;
							}
						}

						di->ignore_phone_number = true;
						GlobalFree( di->w_ignore_phone_number );
						di->w_ignore_phone_number = GlobalStrDupW( ST_Yes );

						/*	// Wildcard values shouldn't appear in the call log listview.
						// See if the value we're adding is a range (has wildcard values in it). Only allow 10 digit numbers.
						if ( ii->c_phone_number != NULL && lstrlenA( ii->c_phone_number ) == 10 && is_num( ii->c_phone_number ) == 1 )
						{
							AddRange( &range_list, ii->c_phone_number );
						}*/

						ignore_list_changed = true;

						lvi.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )ii;
						_SendMessageW( g_hWnd_ignore_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
					}
				}
			}

			// If we removed items in the call log listview from the ignore_list, then remove them from the ignore list listview.
			if ( remove_state_changed == true )
			{
				int item_count = _SendMessageW( g_hWnd_ignore_list, LVM_GETITEMCOUNT, 0, 0 );

				skip_ignore_draw = true;
				_EnableWindow( g_hWnd_ignore_list, FALSE );

				// Start from the end and work backwards.
				for ( int i = item_count - 1; i >= 0; --i )
				{
					lvi.iItem = i;
					_SendMessageW( g_hWnd_ignore_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					ignoreinfo *ii = ( ignoreinfo * )lvi.lParam;

					if ( ii != NULL && ii->state == 1 )
					{
						free_ignoreinfo( &ii );

						_SendMessageW( g_hWnd_ignore_list, LVM_DELETEITEM, i, 0 );
					}
				}

				skip_ignore_draw = false;
				_EnableWindow( g_hWnd_ignore_list, TRUE );
			}
		}

		GlobalFree( iui );
	}

	Processing_Window( hWnd, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN update_ignore_cid_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	HWND hWnd = NULL;
	ignorecidupdateinfo *icidui = ( ignorecidupdateinfo * )pArguments;
	if ( icidui != NULL )
	{
		hWnd = icidui->hWnd;
	}

	Processing_Window( hWnd, false );

	if ( icidui != NULL )
	{
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.
		lvi.iSubItem = 0;

		if ( icidui->hWnd == g_hWnd_ignore_cid_list )
		{
			if ( icidui->action == 0 )	// Add a single item to ignore_cid_list and ignore list listview.
			{
				// Zero init.
				ignorecidinfo *icidi = ( ignorecidinfo * )GlobalAlloc( GPTR, sizeof( ignorecidinfo ) );

				icidi->c_caller_id = icidui->caller_id;

				icidi->match_case = icidui->match_case;
				icidi->match_whole_word = icidui->match_whole_word;

				// Try to insert the ignore_cid_list info in the tree.
				if ( dllrbt_insert( ignore_cid_list, ( void * )icidi, ( void * )icidi ) != DLLRBT_STATUS_OK )
				{
					free_ignorecidinfo( &icidi );

					_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_cid_already_in_ignore_cid_list );
				}
				else	// If it was able to be inserted into the tree, then update our displayinfo items and the ignore cid list listview.
				{
					int val_length = MultiByteToWideChar( CP_UTF8, 0, icidi->c_caller_id, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, icidi->c_caller_id, -1, val, val_length );

					icidi->caller_id = val;

					icidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
					*icidi->c_total_calls = '0';
					*( icidi->c_total_calls + 1 ) = 0;	// Sanity

					val_length = MultiByteToWideChar( CP_UTF8, 0, icidi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, icidi->c_total_calls, -1, val, val_length );

					icidi->total_calls = val;

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

					icidi->count = 0;
					icidi->state = 0;
					icidi->active = false;

					update_caller_id_name_matches( ( void * )icidi, 0, true );

					ignore_cid_list_changed = true;

					lvi.iItem = _SendMessageW( g_hWnd_ignore_cid_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )icidi;
					_SendMessageW( g_hWnd_ignore_cid_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
				}
			}
			else if ( icidui->action == 1 )	// Remove from ignore_cid_list and ignore cid list listview.
			{
				removeinfo *ri = ( removeinfo * )GlobalAlloc( GMEM_FIXED, sizeof( removeinfo ) );
				ri->disable_critical_section = true;
				ri->hWnd = g_hWnd_ignore_cid_list;

				// ri will be freed in the remove_items thread.
				HANDLE hThread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL );

				if ( hThread != NULL )
				{
					WaitForSingleObject( hThread, INFINITE );	// Wait at most 10 seconds for the remove_items thread to finish.
					CloseHandle( hThread );
				}
			}
			else if ( icidui->action == 2 )	// Add all items in the ignore_cid_list to the ignore cid list listview.
			{
				// Insert a row into our listview.
				node_type *node = dllrbt_get_head( ignore_cid_list );
				while ( node != NULL )
				{
					ignorecidinfo *icidi = ( ignorecidinfo * )node->val;

					if ( icidi != NULL )
					{
						lvi.iItem = _SendMessageW( g_hWnd_ignore_cid_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )icidi;	// lParam = our contactinfo structure from the connection thread.
						_SendMessageW( g_hWnd_ignore_cid_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
					}

					node = node->next;
				}
			}
			else if ( icidui->action == 3 )	// Update entry
			{
				ignorecidinfo *old_icidi = icidui->icidi;
				if ( old_icidi != NULL )
				{
					// This temporary structure is used to search our ignore_cid_list tree.
					ignorecidinfo *new_icidi = ( ignorecidinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignorecidinfo ) );

					new_icidi->c_caller_id = old_icidi->c_caller_id;	// DO NOT FREE THIS VALUE.
					new_icidi->match_case = icidui->match_case;
					new_icidi->match_whole_word = icidui->match_whole_word;

					// See if our new entry already exists.
					dllrbt_iterator *itr = dllrbt_find( ignore_cid_list, ( void * )new_icidi, false );
					if ( itr == NULL )
					{
						bool keep_active = false;
						bool update_cid_state1 = false, update_cid_state2 = false;

						// Update each displayinfo item to indicate that it is ignored or no longer ignored.
						node_type *node = dllrbt_get_head( call_log );
						while ( node != NULL )
						{
							DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								// Find our old matches.
								if ( old_icidi->match_case == true && old_icidi->match_whole_word == true )
								{
									if ( lstrcmpA( mdi->ci.caller_id, old_icidi->c_caller_id ) == 0 )
									{
										update_cid_state1 = true;
									}
								}
								else if ( old_icidi->match_case == false && old_icidi->match_whole_word == true )
								{
									if ( lstrcmpiA( mdi->ci.caller_id, old_icidi->c_caller_id ) == 0 )
									{
										update_cid_state1 = true;
									}
								}
								else if ( old_icidi->match_case == true && old_icidi->match_whole_word == false )
								{
									if ( _StrStrA( mdi->ci.caller_id, old_icidi->c_caller_id ) != NULL )
									{
										update_cid_state1 = true;
									}
								}
								else if ( old_icidi->match_case == false && old_icidi->match_whole_word == false )
								{
									if ( _StrStrIA( mdi->ci.caller_id, old_icidi->c_caller_id ) != NULL )
									{
										update_cid_state1 = true;
									}
								}

								// Find our new matches.
								if ( new_icidi->match_case == true && new_icidi->match_whole_word == true )
								{
									if ( lstrcmpA( mdi->ci.caller_id, old_icidi->c_caller_id ) == 0 )
									{
										update_cid_state2 = true;
									}
								}
								else if ( new_icidi->match_case == false && new_icidi->match_whole_word == true )
								{
									if ( lstrcmpiA( mdi->ci.caller_id, old_icidi->c_caller_id ) == 0 )
									{
										update_cid_state2 = true;
									}
								}
								else if ( new_icidi->match_case == true && new_icidi->match_whole_word == false )
								{
									if ( _StrStrA( mdi->ci.caller_id, old_icidi->c_caller_id ) != NULL )
									{
										update_cid_state2 = true;
									}
								}
								else if ( new_icidi->match_case == false && new_icidi->match_whole_word == false )
								{
									if ( _StrStrIA( mdi->ci.caller_id, old_icidi->c_caller_id ) != NULL )
									{
										update_cid_state2 = true;
									}
								}

								if ( update_cid_state1 == true && update_cid_state2 == true )
								{
									keep_active = true;	// Skip updating values that haven't changed.
								}
								else if ( update_cid_state1 == true )
								{
									--( mdi->ignore_cid_match_count );

									if ( mdi->ignore_cid_match_count == 0 )
									{
										old_icidi->active = false;

										GlobalFree( mdi->w_ignore_caller_id );
										mdi->w_ignore_caller_id = GlobalStrDupW( ST_No );
									}
								}
								else if ( update_cid_state2 == true )
								{
									++( mdi->ignore_cid_match_count );

									if ( mdi->ignore_cid_match_count > 0 )
									{
										old_icidi->active = true;
									}

									GlobalFree( mdi->w_ignore_caller_id );
									mdi->w_ignore_caller_id = GlobalStrDupW( ST_Yes );
								}

								update_cid_state1 = update_cid_state2 = false;

								di_node = di_node->next;
							}

							node = node->next;
						}

						if ( keep_active == true )
						{
							old_icidi->active = true;
						}

						// Find the old icidi and remove it.
						itr = dllrbt_find( ignore_cid_list, ( void * )old_icidi, false );
						dllrbt_remove( ignore_cid_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

						old_icidi->match_case = new_icidi->match_case;
						old_icidi->match_whole_word = new_icidi->match_whole_word;

						// Re-add the old icidi with updated values.
						dllrbt_insert( ignore_cid_list, ( void * )old_icidi, ( void * )old_icidi );

						GlobalFree( old_icidi->c_match_case );
						GlobalFree( old_icidi->w_match_case );
						GlobalFree( old_icidi->c_match_whole_word );
						GlobalFree( old_icidi->w_match_whole_word );

						if ( old_icidi->match_case == false )
						{
							old_icidi->c_match_case = GlobalStrDupA( "No" );
							old_icidi->w_match_case = GlobalStrDupW( ST_No );
						}
						else
						{
							old_icidi->c_match_case = GlobalStrDupA( "Yes" );
							old_icidi->w_match_case = GlobalStrDupW( ST_Yes );
						}

						if ( old_icidi->match_whole_word == false )
						{
							old_icidi->c_match_whole_word = GlobalStrDupA( "No" );
							old_icidi->w_match_whole_word = GlobalStrDupW( ST_No );
						}
						else
						{
							old_icidi->c_match_whole_word = GlobalStrDupA( "Yes" );
							old_icidi->w_match_whole_word = GlobalStrDupW( ST_Yes );
						}

						ignore_cid_list_changed = true;	// Makes us save the new ignore_cid_list.
					}
					else
					{
						// See if the value we're updating is different from the match above.
						if ( old_icidi != ( ignorecidinfo * )( ( node_type * )itr )->val )
						{
							_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_cid_already_in_ignore_cid_list );
						}
					}

					GlobalFree( new_icidi );	// We don't need this anymore.
				}
			}
			else if ( icidui->action == 4 )	// Update the recently called number's call count.
			{
				if ( icidui->icidi != NULL )
				{
					++( icidui->icidi->count );

					GlobalFree( icidui->icidi->c_total_calls );

					icidui->icidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 11 );
					__snprintf( icidui->icidi->c_total_calls, 11, "%lu", icidui->icidi->count );

					GlobalFree( icidui->icidi->total_calls );

					int val_length = MultiByteToWideChar( CP_UTF8, 0, icidui->icidi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, icidui->icidi->c_total_calls, -1, val, val_length );

					icidui->icidi->total_calls = val;

					ignore_cid_list_changed = true;
				}
			}

			_InvalidateRect( g_hWnd_call_log, NULL, TRUE );
		}
		else if ( icidui->hWnd == g_hWnd_call_log )	// call log listview
		{
			int item_count = _SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = _SendMessageW( g_hWnd_call_log, LVM_GETSELECTEDCOUNT, 0, 0 );
			
			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				item_count = sel_count;
			}

			bool remove_state_changed = false;	// If true, then go through the ignore list listview and remove the items that have changed state.
			bool skip_multi_cid_warning = false;	// Skip the multiple caller IDs warning if we've already displayed it.

			int last_selected = lvi.iItem;

			// Go through each item in the listview.
			for ( int i = 0; i < item_count; ++i )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thead == true )
				{
					break;
				}

				if ( handle_all == true )
				{
					lvi.iItem = i;
				}
				else
				{
					last_selected = lvi.iItem = _SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, last_selected, LVNI_SELECTED );
				}

				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				displayinfo *di = ( displayinfo * )lvi.lParam;

				if ( di->ignore_cid_match_count > 0 && icidui->action == 1 )		// Remove from ignore_cid_list.
				{
					if ( di->ignore_cid_match_count <= 1 )
					{
						bool update_cid_state = false;

						// Find a keyword that matches the value we want to remove. Make sure that keyword only matches one set of values.
						node_type *node = dllrbt_get_head( ignore_cid_list );
						while ( node != NULL )
						{
							// The keyword to remove.
							ignorecidinfo *icidi = ( ignorecidinfo * )node->val;

							// Only remove active keyword matches.
							if ( icidi->active == true )
							{
								if ( icidi->match_case == true && icidi->match_whole_word == true )
								{
									if ( lstrcmpA( di->ci.caller_id, icidi->c_caller_id ) == 0 )
									{
										update_cid_state = true;
									}
								}
								else if ( icidi->match_case == false && icidi->match_whole_word == true )
								{
									if ( lstrcmpiA( di->ci.caller_id, icidi->c_caller_id ) == 0 )
									{
										update_cid_state = true;
									}
								}
								else if ( icidi->match_case == true && icidi->match_whole_word == false )
								{
									if ( _StrStrA( di->ci.caller_id, icidi->c_caller_id ) != NULL )
									{
										update_cid_state = true;
									}
								}
								else if ( icidi->match_case == false && icidi->match_whole_word == false )
								{
									if ( _StrStrIA( di->ci.caller_id, icidi->c_caller_id ) != NULL )
									{
										update_cid_state = true;
									}
								}

								if ( update_cid_state == true )
								{
									update_caller_id_name_matches( ( void * )icidi, 0, false );

									icidi->state = 1;
									remove_state_changed = true;	// This will let us go through the ignore cid list listview and free items with a state == 1.

									dllrbt_iterator *itr = dllrbt_find( ignore_cid_list, ( void * )icidi, false );
									dllrbt_remove( ignore_cid_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

									ignore_cid_list_changed = true;	// Causes us to save the ignore_cid_list on shutdown.

									break;	// Break out of the keyword search.
								}
							}

							node = node->next;
						}
					}
					else
					{
						if ( skip_multi_cid_warning == false )
						{
							_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_remove_from_ignore_caller_id_list );
							skip_multi_cid_warning = true;
						}
					}
				}
				else if ( di->ignore_cid_match_count == 0 && icidui->action == 0 )	// Add to ignore_cid_list.
				{
					// Zero init.
					ignorecidinfo *icidi = ( ignorecidinfo * )GlobalAlloc( GPTR, sizeof( ignorecidinfo ) );

					icidi->c_caller_id = GlobalStrDupA( di->ci.caller_id );

					icidi->match_case = icidui->match_case;
					icidi->match_whole_word = icidui->match_whole_word;

					// Try to insert the ignore_cid_list info in the tree.
					if ( dllrbt_insert( ignore_cid_list, ( void * )icidi, ( void * )icidi ) != DLLRBT_STATUS_OK )
					{
						free_ignorecidinfo( &icidi );

						_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_cid_already_in_ignore_cid_list );
					}
					else	// If it was able to be inserted into the tree, then update our displayinfo items and the ignore cid list listview.
					{
						int val_length = MultiByteToWideChar( CP_UTF8, 0, icidi->c_caller_id, -1, NULL, 0 );	// Include the NULL terminator.
						wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
						MultiByteToWideChar( CP_UTF8, 0, icidi->c_caller_id, -1, val, val_length );

						icidi->caller_id = val;

						icidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
						*icidi->c_total_calls = '0';
						*( icidi->c_total_calls + 1 ) = 0;	// Sanity

						val_length = MultiByteToWideChar( CP_UTF8, 0, icidi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
						val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
						MultiByteToWideChar( CP_UTF8, 0, icidi->c_total_calls, -1, val, val_length );

						icidi->total_calls = val;

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

						icidi->count = 0;
						icidi->state = 0;
						icidi->active = false;

						update_caller_id_name_matches( ( void * )icidi, 0, true );

						ignore_cid_list_changed = true;

						lvi.iItem = _SendMessageW( g_hWnd_ignore_cid_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )icidi;
						_SendMessageW( g_hWnd_ignore_cid_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
					}
				}
			}

			// If we removed items in the call log listview from the ignore_cid_list, then remove them from the ignore cid list listview.
			if ( remove_state_changed == true )
			{
				int item_count = _SendMessageW( g_hWnd_ignore_cid_list, LVM_GETITEMCOUNT, 0, 0 );

				skip_ignore_cid_draw = true;
				_EnableWindow( g_hWnd_ignore_cid_list, FALSE );

				// Start from the end and work backwards.
				for ( int i = item_count - 1; i >= 0; --i )
				{
					lvi.iItem = i;
					_SendMessageW( g_hWnd_ignore_cid_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					ignorecidinfo *icidi = ( ignorecidinfo * )lvi.lParam;

					if ( icidi != NULL && icidi->state == 1 )
					{
						free_ignorecidinfo( &icidi );

						_SendMessageW( g_hWnd_ignore_cid_list, LVM_DELETEITEM, i, 0 );
					}
				}

				skip_ignore_cid_draw = false;
				_EnableWindow( g_hWnd_ignore_cid_list, TRUE );
			}
		}

		GlobalFree( icidui );
	}

	Processing_Window( hWnd, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN update_forward_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	HWND hWnd = NULL;
	forwardupdateinfo *fui = ( forwardupdateinfo * )pArguments;
	if ( fui != NULL )
	{
		hWnd = fui->hWnd;
	}

	Processing_Window( hWnd, false );

	if ( fui != NULL )
	{
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.
		lvi.iSubItem = 0;

		if ( fui->hWnd == g_hWnd_forward_list )
		{
			if ( fui->action == 0 )	// Add a single item to the forward_list and forward list listview.
			{
				// Zero init.
				forwardinfo *fi = ( forwardinfo * )GlobalAlloc( GPTR, sizeof( forwardinfo ) );

				fi->c_call_from = fui->call_from;
				fi->c_forward_to = fui->forward_to;

				// Try to insert the forward_list info in the tree.
				if ( dllrbt_insert( forward_list, ( void * )fi->c_call_from, ( void * )fi ) != DLLRBT_STATUS_OK )
				{
					free_forwardinfo( &fi );

					_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_already_in_forward_list );
				}
				else	// If it was able to be inserted into the tree, then update our displayinfo items and the forward list listview.
				{
					fi->call_from = FormatPhoneNumber( fui->call_from );
					fi->forward_to = FormatPhoneNumber( fui->forward_to );

					fi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
					*fi->c_total_calls = '0';
					*( fi->c_total_calls + 1 ) = 0;	// Sanity

					int val_length = MultiByteToWideChar( CP_UTF8, 0, fi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, fi->c_total_calls, -1, val, val_length );

					fi->total_calls = val;

					fi->count = 0;
					fi->state = 0;

					// See if the value we're adding is a range (has wildcard values in it).
					if ( fi->c_call_from != NULL && is_num( fi->c_call_from ) == 1 )
					{
						int phone_number_length = lstrlenA( fi->c_call_from );

						RangeAdd( &forward_range_list[ ( phone_number_length > 0 ? phone_number_length - 1 : 0 ) ], fi->c_call_from, phone_number_length );

						// Update each displayinfo item to indicate that it is now forwarded.
						node_type *node = dllrbt_get_head( call_log );
						while ( node != NULL )
						{
							DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								// Process values that are not set to be forwarded. See if the value falls within our range.
								if ( mdi->forward_phone_number == false && RangeCompare( fi->c_call_from, mdi->ci.call_from ) == true )
								{
									mdi->forward_phone_number = true;
									GlobalFree( mdi->w_forward_phone_number );
									mdi->w_forward_phone_number = GlobalStrDupW( ST_Yes );
								}

								di_node = di_node->next;
							}

							node = node->next;
						}
					}
					else
					{
						// Update each displayinfo item to indicate that it is now forwarded.
						DoublyLinkedList *ll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )fi->c_call_from, true );
						if ( ll != NULL )
						{
							DoublyLinkedList *di_node = ll;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								if ( mdi->forward_phone_number == false )
								{
									mdi->forward_phone_number = true;
									GlobalFree( mdi->w_forward_phone_number );
									mdi->w_forward_phone_number = GlobalStrDupW( ST_Yes );

									// This doesn't need to be updated.
									/*if ( mdi->ci.forward_to != NULL )
									{
										GlobalFree( mdi->ci.forward_to );
										mdi->ci.forward_to = GlobalStrDupA( fi->c_forward_to );
									}*/
								}

								di_node = di_node->next;
							}
						}
					}

					forward_list_changed = true;

					lvi.iItem = _SendMessageW( g_hWnd_forward_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )fi;	// lParam = our contactinfo structure from the connection thread.
					_SendMessageW( g_hWnd_forward_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
				}
			}
			else if ( fui->action == 1 )	// Remove from forward_list and forward list listview.
			{
				removeinfo *ri = ( removeinfo * )GlobalAlloc( GMEM_FIXED, sizeof( removeinfo ) );
				ri->disable_critical_section = true;
				ri->hWnd = g_hWnd_forward_list;

				// ri will be freed in the remove_items thread.
				HANDLE hThread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL );

				if ( hThread != NULL )
				{
					WaitForSingleObject( hThread, INFINITE );	// Wait at most 10 seconds for the remove_items thread to finish.
					CloseHandle( hThread );
				}
			}
			else if ( fui->action == 2 )	// Add all items in the foward_list to the forward list listview.
			{
				// Insert a row into our listview.
				node_type *node = dllrbt_get_head( forward_list );
				while ( node != NULL )
				{
					forwardinfo *fi = ( forwardinfo * )node->val;

					if ( fi != NULL )
					{
						lvi.iItem = _SendMessageW( g_hWnd_forward_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )fi;	// lParam = our contactinfo structure from the connection thread.
						_SendMessageW( g_hWnd_forward_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
					}

					node = node->next;
				}
			}
			else if ( fui->action == 3 )	// Update entry.
			{
				if ( fui->fi != NULL )
				{
					GlobalFree( fui->fi->c_forward_to );
					GlobalFree( fui->fi->forward_to );

					fui->fi->c_forward_to = fui->forward_to;
					fui->fi->forward_to = FormatPhoneNumber( fui->forward_to );

					forward_list_changed = true;	// Makes us save the new forward_list.
				}
				else
				{
					GlobalFree( fui->forward_to );
				}
			}
			else if ( fui->action == 4 )	// Update the recently called number's call count.
			{
				if ( fui->fi != NULL )
				{
					++( fui->fi->count );

					GlobalFree( fui->fi->c_total_calls );

					fui->fi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 11 );
					__snprintf( fui->fi->c_total_calls, 11, "%lu", fui->fi->count );

					GlobalFree( fui->fi->total_calls );

					int val_length = MultiByteToWideChar( CP_UTF8, 0, fui->fi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, fui->fi->c_total_calls, -1, val, val_length );

					fui->fi->total_calls = val;

					forward_list_changed = true;
				}
			}

			_InvalidateRect( g_hWnd_call_log, NULL, TRUE );
		}
		else if ( fui->hWnd == g_hWnd_call_log )	// call log listview
		{
			int item_count = _SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = _SendMessageW( g_hWnd_call_log, LVM_GETSELECTEDCOUNT, 0, 0 );
			
			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				item_count = sel_count;
			}

			bool remove_state_changed = false;	// If true, then go through the forward list listview and remove the items that have changed state.
			bool skip_range_warning = false;	// Skip the range warning if we've already displayed it.

			int last_selected = lvi.iItem;

			// Go through each item in the listview.
			for ( int i = 0; i < item_count; ++i )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thead == true )
				{
					break;
				}

				if ( handle_all == true )
				{
					lvi.iItem = i;
				}
				else
				{
					last_selected = lvi.iItem = _SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, last_selected, LVNI_SELECTED );
				}

				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				displayinfo *di = ( displayinfo * )lvi.lParam;

				if ( di->forward_phone_number == true && fui->action == 1 )		// Remove from forward_list.
				{
					char range_number[ 32 ];
					_memzero( range_number, 32 );

					int range_index = lstrlenA( di->ci.call_from );
					range_index = ( range_index > 0 ? range_index - 1 : 0 );

					// First, see if the phone number is in our range list.
					if ( RangeSearch( &forward_range_list[ range_index ], di->ci.call_from, range_number ) == false )
					{
						// If not, then see if the phone number is in our forward_list.
						dllrbt_iterator *itr = dllrbt_find( forward_list, ( void * )di->ci.call_from, false );
						if ( itr != NULL )
						{
							// Update each displayinfo item to indicate that it is no longer forwarded.
							DoublyLinkedList *ll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
							if ( ll != NULL )
							{
								DoublyLinkedList *di_node = ll;
								while ( di_node != NULL )
								{
									displayinfo *mdi = ( displayinfo * )di_node->data;

									if ( mdi->forward_phone_number == true )
									{
										mdi->forward_phone_number = false;
										GlobalFree( mdi->w_forward_phone_number );
										mdi->w_forward_phone_number = GlobalStrDupW( ST_No );

										// This doesn't need to be updated.
										/*if ( mdi->ci.forward_to != NULL )
										{
											GlobalFree( mdi->ci.forward_to );
											mdi->ci.forward_to = NULL;
										}*/
									}

									di_node = di_node->next;
								}
							}

							// If a forward list listview item also shows up in the call log listview, then we'll remove it after this loop.
							forwardinfo *fi = ( forwardinfo * )( ( node_type * )itr )->val;
							fi->state = 1;	// Remove item.
							remove_state_changed = true;	// This will let us go through the forward list listview and free items with a state == 1.

							dllrbt_remove( forward_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

							forward_list_changed = true;	// Causes us to save the forward_list on shutdown.
						}

						di->forward_phone_number = false;
						GlobalFree( di->w_forward_phone_number );
						di->w_forward_phone_number = GlobalStrDupW( ST_No );
					}
					else
					{
						if ( skip_range_warning == false )
						{
							_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_remove_from_forward_phone_number_list );
							skip_range_warning = true;
						}
					}
				}
				else if ( di->forward_phone_number == false && fui->action == 0 )	// Add to forward_list.
				{
					// Zero init.
					forwardinfo *fi = ( forwardinfo * )GlobalAlloc( GPTR, sizeof( forwardinfo ) );

					fi->c_call_from = GlobalStrDupA( di->ci.call_from );
					fi->c_forward_to = GlobalStrDupA( fui->forward_to );

					if ( dllrbt_insert( forward_list, ( void * )fi->c_call_from, ( void * )fi ) != DLLRBT_STATUS_OK )
					{
						free_forwardinfo( &fi );

						_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_already_in_forward_list );
					}
					else	// Add to forward list listview as well.
					{
						fi->call_from = FormatPhoneNumber( di->ci.call_from );
						fi->forward_to = FormatPhoneNumber( fui->forward_to );

						fi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
						*fi->c_total_calls = '0';
						*( fi->c_total_calls + 1 ) = 0;	// Sanity

						int val_length = MultiByteToWideChar( CP_UTF8, 0, fi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
						wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
						MultiByteToWideChar( CP_UTF8, 0, fi->c_total_calls, -1, val, val_length );

						fi->total_calls = val;

						fi->count = 0;

						fi->state = 0;

						// Update all nodes if it already exits.
						DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
						if ( dll != NULL )
						{
							DoublyLinkedList *node = dll;
							while ( node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )node->data;

								if ( mdi->forward_phone_number == false )
								{
									mdi->forward_phone_number = true;
									GlobalFree( mdi->w_forward_phone_number );
									mdi->w_forward_phone_number = GlobalStrDupW( ST_Yes );

									// This doesn't need to be updated.
									/*if ( mdi->ci.forward_to != NULL )
									{
										GlobalFree( mdi->ci.forward_to );
										mdi->ci.forward_to = GlobalStrDupA( fi->c_forward_to );
									}*/
								}

								node = node->next;
							}
						}

						di->forward_phone_number = true;
						GlobalFree( di->w_forward_phone_number );
						di->w_forward_phone_number = GlobalStrDupW( ST_Yes );

						/*	// Wildcard values shouldn't appear in the call log listview.
						// See if the value we're adding is a range (has wildcard values in it). Only allow 10 digit numbers.
						if ( fi->c_call_from != NULL && lstrlenA( fi->c_call_from ) == 10 && is_num( fi->c_call_from ) == 1 )
						{
							AddRange( &forward_range_list, fi->c_call_from );
						}*/

						forward_list_changed = true;

						lvi.iItem = _SendMessageW( g_hWnd_forward_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )fi;
						_SendMessageW( g_hWnd_forward_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
					}
				}
			}

			// If we removed items in the call log listview from the forward_list, then remove them from the forward list listview.
			if ( remove_state_changed == true )
			{
				int item_count = _SendMessageW( g_hWnd_forward_list, LVM_GETITEMCOUNT, 0, 0 );

				skip_forward_draw = true;
				_EnableWindow( g_hWnd_forward_list, FALSE );

				// Start from the end and work backwards.
				for ( int i = item_count - 1; i >= 0; --i )
				{
					lvi.iItem = i;
					_SendMessageW( g_hWnd_forward_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					forwardinfo *fi = ( forwardinfo * )lvi.lParam;

					if ( fi != NULL && fi->state == 1 )
					{
						free_forwardinfo( &fi );

						_SendMessageW( g_hWnd_forward_list, LVM_DELETEITEM, i, 0 );
					}
				}

				skip_forward_draw = false;
				_EnableWindow( g_hWnd_forward_list, TRUE );
			}

			GlobalFree( fui->forward_to );	// If this was used, then it was copied.
		}

		GlobalFree( fui );
	}

	Processing_Window( hWnd, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN update_forward_cid_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	HWND hWnd = NULL;
	forwardcidupdateinfo *fcidui = ( forwardcidupdateinfo * )pArguments;
	if ( fcidui != NULL )
	{
		hWnd = fcidui->hWnd;
	}

	Processing_Window( hWnd, false );

	if ( fcidui != NULL )
	{
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.
		lvi.iSubItem = 0;

		if ( fcidui->hWnd == g_hWnd_forward_cid_list )
		{
			if ( fcidui->action == 0 )	// Add a single item to the forward_list and forward list listview.
			{
				// Zero init.
				forwardcidinfo *fcidi = ( forwardcidinfo * )GlobalAlloc( GPTR, sizeof( forwardcidinfo ) );

				fcidi->c_caller_id = fcidui->caller_id;
				fcidi->c_forward_to = fcidui->forward_to;

				fcidi->match_case = fcidui->match_case;
				fcidi->match_whole_word = fcidui->match_whole_word;

				// Try to insert the forward_cid_list info in the tree.
				if ( dllrbt_insert( forward_cid_list, ( void * )fcidi, ( void * )fcidi ) != DLLRBT_STATUS_OK )
				{
					free_forwardcidinfo( &fcidi );

					_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_cid_already_in_forward_cid_list );
				}
				else	// If it was able to be inserted into the tree, then update our displayinfo items and the forward cid list listview.
				{
					int val_length = MultiByteToWideChar( CP_UTF8, 0, fcidi->c_caller_id, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, fcidi->c_caller_id, -1, val, val_length );

					fcidi->caller_id = val;
					fcidi->forward_to = FormatPhoneNumber( fcidi->c_forward_to );

					fcidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
					*fcidi->c_total_calls = '0';
					*( fcidi->c_total_calls + 1 ) = 0;	// Sanity

					val_length = MultiByteToWideChar( CP_UTF8, 0, fcidi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, fcidi->c_total_calls, -1, val, val_length );

					fcidi->total_calls = val;

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

					fcidi->count = 0;
					fcidi->state = 0;
					fcidi->active = false;

					update_caller_id_name_matches( ( void * )fcidi, 1, true );

					forward_cid_list_changed = true;

					lvi.iItem = _SendMessageW( g_hWnd_forward_cid_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )fcidi;
					_SendMessageW( g_hWnd_forward_cid_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
				}
			}
			else if ( fcidui->action == 1 )	// Remove from forward_list and forward list listview.
			{
				removeinfo *ri = ( removeinfo * )GlobalAlloc( GMEM_FIXED, sizeof( removeinfo ) );
				ri->disable_critical_section = true;
				ri->hWnd = g_hWnd_forward_cid_list;

				// ri will be freed in the remove_items thread.
				HANDLE hThread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL );

				if ( hThread != NULL )
				{
					WaitForSingleObject( hThread, INFINITE );	// Wait at most 10 seconds for the remove_items thread to finish.
					CloseHandle( hThread );
				}
			}
			else if ( fcidui->action == 2 )	// Add all items in the foward_list to the forward list listview.
			{
				// Insert a row into our listview.
				node_type *node = dllrbt_get_head( forward_cid_list );
				while ( node != NULL )
				{
					forwardcidinfo *fcidi = ( forwardcidinfo * )node->val;

					if ( fcidi != NULL )
					{
						lvi.iItem = _SendMessageW( g_hWnd_forward_cid_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )fcidi;	// lParam = our contactinfo structure from the connection thread.
						_SendMessageW( g_hWnd_forward_cid_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
					}

					node = node->next;
				}
			}
			else if ( fcidui->action == 3 )	// Update entry.
			{
				forwardcidinfo *old_fcidi = fcidui->fcidi;
				if ( old_fcidi != NULL )
				{
					bool update_cid_state = true;

					// This temporary structure is used to search our ignore_cid_list tree.
					forwardcidinfo *new_fcidi = ( forwardcidinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardcidinfo ) );

					new_fcidi->c_caller_id = old_fcidi->c_caller_id;	// DO NOT FREE THIS VALUE.
					new_fcidi->match_case = fcidui->match_case;
					new_fcidi->match_whole_word = fcidui->match_whole_word;

					// See if our new entry already exists.
					dllrbt_iterator *itr = dllrbt_find( forward_cid_list, ( void * )new_fcidi, false );

					// See if the forward_to numbers match.
					if ( itr != NULL )
					{
						forwardcidinfo *tmp_fcidi = ( forwardcidinfo * )( ( node_type * )itr )->val;

						if ( lstrcmpA( tmp_fcidi->c_forward_to, fcidui->forward_to ) == 0 )
						{
							update_cid_state = false;
						}
					}
					
					if ( update_cid_state == true )
					{
						bool keep_active = false;
						bool update_cid_state1 = false, update_cid_state2 = false;

						// Update each displayinfo item to indicate that it is forwarded or no longer forwarded.
						node_type *node = dllrbt_get_head( call_log );
						while ( node != NULL )
						{
							DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								// Find our old matches.
								if ( old_fcidi->match_case == true && old_fcidi->match_whole_word == true )
								{
									if ( lstrcmpA( mdi->ci.caller_id, old_fcidi->c_caller_id ) == 0 )
									{
										update_cid_state1 = true;
									}
								}
								else if ( old_fcidi->match_case == false && old_fcidi->match_whole_word == true )
								{
									if ( lstrcmpiA( mdi->ci.caller_id, old_fcidi->c_caller_id ) == 0 )
									{
										update_cid_state1 = true;
									}
								}
								else if ( old_fcidi->match_case == true && old_fcidi->match_whole_word == false )
								{
									if ( _StrStrA( mdi->ci.caller_id, old_fcidi->c_caller_id ) != NULL )
									{
										update_cid_state1 = true;
									}
								}
								else if ( old_fcidi->match_case == false && old_fcidi->match_whole_word == false )
								{
									if ( _StrStrIA( mdi->ci.caller_id, old_fcidi->c_caller_id ) != NULL )
									{
										update_cid_state1 = true;
									}
								}

								// Find our new matches.
								if ( new_fcidi->match_case == true && new_fcidi->match_whole_word == true )
								{
									if ( lstrcmpA( mdi->ci.caller_id, old_fcidi->c_caller_id ) == 0 )
									{
										update_cid_state2 = true;
									}
								}
								else if ( new_fcidi->match_case == false && new_fcidi->match_whole_word == true )
								{
									if ( lstrcmpiA( mdi->ci.caller_id, old_fcidi->c_caller_id ) == 0 )
									{
										update_cid_state2 = true;
									}
								}
								else if ( new_fcidi->match_case == true && new_fcidi->match_whole_word == false )
								{
									if ( _StrStrA( mdi->ci.caller_id, old_fcidi->c_caller_id ) != NULL )
									{
										update_cid_state2 = true;
									}
								}
								else if ( new_fcidi->match_case == false && new_fcidi->match_whole_word == false )
								{
									if ( _StrStrIA( mdi->ci.caller_id, old_fcidi->c_caller_id ) != NULL )
									{
										update_cid_state2 = true;
									}
								}

								if ( update_cid_state1 == true && update_cid_state2 == true )
								{
									keep_active = true;	// Skip updating values that haven't changed.
								}
								else if ( update_cid_state1 == true )
								{
									--( mdi->forward_cid_match_count );

									if ( mdi->forward_cid_match_count == 0 )
									{
										old_fcidi->active = false;

										GlobalFree( mdi->w_forward_caller_id );
										mdi->w_forward_caller_id = GlobalStrDupW( ST_No );
									}
								}
								else if ( update_cid_state2 == true )
								{
									++( mdi->forward_cid_match_count );

									if ( mdi->forward_cid_match_count > 0 )
									{
										old_fcidi->active = true;
									}

									GlobalFree( mdi->w_forward_caller_id );
									mdi->w_forward_caller_id = GlobalStrDupW( ST_Yes );
								}

								update_cid_state1 = update_cid_state2 = false;

								di_node = di_node->next;
							}

							node = node->next;
						}

						if ( keep_active == true )
						{
							old_fcidi->active = true;
						}

						GlobalFree( old_fcidi->c_forward_to );
						GlobalFree( old_fcidi->forward_to );

						old_fcidi->c_forward_to = fcidui->forward_to;
						old_fcidi->forward_to = FormatPhoneNumber( fcidui->forward_to );

						// Find the old fcidi and remove it.
						itr = dllrbt_find( forward_cid_list, ( void * )old_fcidi, false );
						dllrbt_remove( forward_cid_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

						old_fcidi->match_case = new_fcidi->match_case;
						old_fcidi->match_whole_word = new_fcidi->match_whole_word;

						// Re-add the old fcidi with updated values.
						dllrbt_insert( forward_cid_list, ( void * )old_fcidi, ( void * )old_fcidi );

						GlobalFree( old_fcidi->c_match_case );
						GlobalFree( old_fcidi->w_match_case );
						GlobalFree( old_fcidi->c_match_whole_word );
						GlobalFree( old_fcidi->w_match_whole_word );

						if ( old_fcidi->match_case == false )
						{
							old_fcidi->c_match_case = GlobalStrDupA( "No" );
							old_fcidi->w_match_case = GlobalStrDupW( ST_No );
						}
						else
						{
							old_fcidi->c_match_case = GlobalStrDupA( "Yes" );
							old_fcidi->w_match_case = GlobalStrDupW( ST_Yes );
						}

						if ( old_fcidi->match_whole_word == false )
						{
							old_fcidi->c_match_whole_word = GlobalStrDupA( "No" );
							old_fcidi->w_match_whole_word = GlobalStrDupW( ST_No );
						}
						else
						{
							old_fcidi->c_match_whole_word = GlobalStrDupA( "Yes" );
							old_fcidi->w_match_whole_word = GlobalStrDupW( ST_Yes );
						}

						forward_cid_list_changed = true;	// Makes us save the new forward_cid_list.
					}
					else
					{
						GlobalFree( fcidui->forward_to );

						// See if the value we're updating is different from the match above.
						if ( old_fcidi != ( forwardcidinfo * )( ( node_type * )itr )->val )
						{
							_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_cid_already_in_forward_cid_list );
						}
					}

					GlobalFree( new_fcidi );	// We don't need this anymore.
				}
				else
				{
					GlobalFree( fcidui->forward_to );
				}
			}
			else if ( fcidui->action == 4 )	// Update the recently called number's call count.
			{
				if ( fcidui->fcidi != NULL )
				{
					++( fcidui->fcidi->count );

					GlobalFree( fcidui->fcidi->c_total_calls );

					fcidui->fcidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 11 );
					__snprintf( fcidui->fcidi->c_total_calls, 11, "%lu", fcidui->fcidi->count );

					GlobalFree( fcidui->fcidi->total_calls );

					int val_length = MultiByteToWideChar( CP_UTF8, 0, fcidui->fcidi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, fcidui->fcidi->c_total_calls, -1, val, val_length );

					fcidui->fcidi->total_calls = val;

					forward_cid_list_changed = true;
				}
			}

			_InvalidateRect( g_hWnd_call_log, NULL, TRUE );
		}
		else if ( fcidui->hWnd == g_hWnd_call_log )	// call log listview
		{
			int item_count = _SendMessageW( g_hWnd_call_log, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = _SendMessageW( g_hWnd_call_log, LVM_GETSELECTEDCOUNT, 0, 0 );
			
			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				item_count = sel_count;
			}

			bool remove_state_changed = false;	// If true, then go through the forward list listview and remove the items that have changed state.
			bool skip_multi_cid_warning = false;	// Skip the multiple caller IDs warning if we've already displayed it.

			int last_selected = lvi.iItem;

			// Go through each item in the listview.
			for ( int i = 0; i < item_count; ++i )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thead == true )
				{
					break;
				}

				if ( handle_all == true )
				{
					lvi.iItem = i;
				}
				else
				{
					last_selected = lvi.iItem = _SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, last_selected, LVNI_SELECTED );
				}

				_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

				displayinfo *di = ( displayinfo * )lvi.lParam;

				if ( di->forward_cid_match_count > 0 && fcidui->action == 1 )		// Remove from forward_cid_list.
				{
					if ( di->forward_cid_match_count <= 1 )
					{
						bool update_cid_state = false;

						// Find a keyword that matches the value we want to remove. Make sure that keyword only matches one set of values.
						node_type *node = dllrbt_get_head( forward_cid_list );
						while ( node != NULL )
						{
							// The keyword to remove.
							forwardcidinfo *fcidi = ( forwardcidinfo * )node->val;

							// Only remove active keyword matches.
							if ( fcidi->active == true )
							{
								if ( fcidi->match_case == true && fcidi->match_whole_word == true )
								{
									if ( lstrcmpA( di->ci.caller_id, fcidi->c_caller_id ) == 0 )
									{
										update_cid_state = true;
									}
								}
								else if ( fcidi->match_case == false && fcidi->match_whole_word == true )
								{
									if ( lstrcmpiA( di->ci.caller_id, fcidi->c_caller_id ) == 0 )
									{
										update_cid_state = true;
									}
								}
								else if ( fcidi->match_case == true && fcidi->match_whole_word == false )
								{
									if ( _StrStrA( di->ci.caller_id, fcidi->c_caller_id ) != NULL )
									{
										update_cid_state = true;
									}
								}
								else if ( fcidi->match_case == false && fcidi->match_whole_word == false )
								{
									if ( _StrStrIA( di->ci.caller_id, fcidi->c_caller_id ) != NULL )
									{
										update_cid_state = true;
									}
								}

								if ( update_cid_state == true )
								{
									update_caller_id_name_matches( ( void * )fcidi, 1, false );

									fcidi->state = 1;
									remove_state_changed = true;	// This will let us go through the forward cid list listview and free items with a state == 1.

									dllrbt_iterator *itr = dllrbt_find( forward_cid_list, ( void * )fcidi, false );
									dllrbt_remove( forward_cid_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

									forward_cid_list_changed = true;	// Causes us to save the forward_cid_list on shutdown.

									break;	// Break out of the keyword search.
								}
							}

							node = node->next;
						}
					}
					else
					{
						if ( skip_multi_cid_warning == false )
						{
							_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_remove_from_forward_caller_id_list );
							skip_multi_cid_warning = true;
						}
					}
				}
				else if ( di->forward_cid_match_count == 0 && fcidui->action == 0 )	// Add to forward_list.
				{
					// Zero init.
					forwardcidinfo *fcidi = ( forwardcidinfo * )GlobalAlloc( GPTR, sizeof( forwardcidinfo ) );

					fcidi->c_caller_id = GlobalStrDupA( di->ci.caller_id );
					fcidi->c_forward_to = GlobalStrDupA( fcidui->forward_to );

					fcidi->match_case = fcidui->match_case;
					fcidi->match_whole_word = fcidui->match_whole_word;

					// Try to insert the forward_cid_list info in the tree.
					if ( dllrbt_insert( forward_cid_list, ( void * )fcidi, ( void * )fcidi ) != DLLRBT_STATUS_OK )
					{
						free_forwardcidinfo( &fcidi );

						_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_cid_already_in_forward_cid_list );
					}
					else	// If it was able to be inserted into the tree, then update our displayinfo items and the forward cid list listview.
					{
						int val_length = MultiByteToWideChar( CP_UTF8, 0, fcidi->c_caller_id, -1, NULL, 0 );	// Include the NULL terminator.
						wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
						MultiByteToWideChar( CP_UTF8, 0, fcidi->c_caller_id, -1, val, val_length );

						fcidi->caller_id = val;
						fcidi->forward_to = FormatPhoneNumber( fcidui->forward_to );

						fcidi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
						*fcidi->c_total_calls = '0';
						*( fcidi->c_total_calls + 1 ) = 0;	// Sanity

						val_length = MultiByteToWideChar( CP_UTF8, 0, fcidi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
						val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
						MultiByteToWideChar( CP_UTF8, 0, fcidi->c_total_calls, -1, val, val_length );

						fcidi->total_calls = val;

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

						fcidi->count = 0;
						fcidi->state = 0;
						fcidi->active = false;

						update_caller_id_name_matches( ( void * )fcidi, 1, true );

						forward_cid_list_changed = true;

						lvi.iItem = _SendMessageW( g_hWnd_forward_cid_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )fcidi;
						_SendMessageW( g_hWnd_forward_cid_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
					}
				}
			}

			// If we removed items in the call log listview from the forward_list, then remove them from the forward list listview.
			if ( remove_state_changed == true )
			{
				int item_count = _SendMessageW( g_hWnd_forward_cid_list, LVM_GETITEMCOUNT, 0, 0 );

				skip_forward_cid_draw = true;
				_EnableWindow( g_hWnd_forward_cid_list, FALSE );

				// Start from the end and work backwards.
				for ( int i = item_count - 1; i >= 0; --i )
				{
					lvi.iItem = i;
					_SendMessageW( g_hWnd_forward_cid_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					forwardcidinfo *fcidi = ( forwardcidinfo * )lvi.lParam;

					if ( fcidi != NULL && fcidi->state == 1 )
					{
						free_forwardcidinfo( &fcidi );

						_SendMessageW( g_hWnd_forward_cid_list, LVM_DELETEITEM, i, 0 );
					}
				}

				skip_forward_cid_draw = false;
				_EnableWindow( g_hWnd_forward_cid_list, TRUE );
			}

			GlobalFree( fcidui->forward_to );	// If this was used, then it was copied.
		}

		GlobalFree( fcidui );
	}

	Processing_Window( hWnd, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN update_contact_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	Processing_Window( g_hWnd_contact_list, false );

	wchar_t *val = NULL;
	int val_length = 0;

	// Insert a row into our listview.
	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
	lvi.iSubItem = 0;

	updateinfo *ui = ( updateinfo * )pArguments;	// Freed at the end of this thread.
	contactinfo *ci = NULL;
	contactinfo *uci = NULL;
	if ( ui != NULL )
	{
		ci = ( contactinfo * )ui->old_ci;
		uci = ( contactinfo * )ui->new_ci;
	}

	if ( ci != NULL && uci != NULL )	// Update a contact. Free uci when done.
	{
		if ( ui->remove_picture == true )	// We want to remove the contact's picture.
		{
			if ( ci->picture_path != NULL )
			{
				GlobalFree( ci->picture_path );
				ci->picture_path = NULL;
			}

			if ( ci->contact.picture_location != NULL )
			{
				GlobalFree( ci->contact.picture_location );
				ci->contact.picture_location = NULL;
			}
		}

		if ( uci->contact.first_name != NULL )
		{
			GlobalFree( ci->contact.first_name );
			ci->contact.first_name = uci->contact.first_name;
			GlobalFree( ci->first_name );
			ci->first_name = uci->first_name;
		}

		if ( uci->contact.last_name != NULL )
		{
			GlobalFree( ci->contact.last_name );
			ci->contact.last_name = uci->contact.last_name;
			GlobalFree( ci->last_name );
			ci->last_name = uci->last_name;
		}

		if ( uci->contact.nickname != NULL )
		{
			GlobalFree( ci->contact.nickname );
			ci->contact.nickname = uci->contact.nickname;
			GlobalFree( ci->nickname );
			ci->nickname = uci->nickname;
		}

		if ( uci->contact.title != NULL )
		{
			GlobalFree( ci->contact.title );
			ci->contact.title = uci->contact.title;
			GlobalFree( ci->title );
			ci->title = uci->title;
		}

		if ( uci->contact.business_name != NULL )
		{
			GlobalFree( ci->contact.business_name );
			ci->contact.business_name = uci->contact.business_name;
			GlobalFree( ci->business_name );
			ci->business_name = uci->business_name;
		}

		if ( uci->contact.designation != NULL )
		{
			GlobalFree( ci->contact.designation );
			ci->contact.designation = uci->contact.designation;
			GlobalFree( ci->designation );
			ci->designation = uci->designation;
		}

		if ( uci->contact.department != NULL )
		{
			GlobalFree( ci->contact.department );
			ci->contact.department = uci->contact.department;
			GlobalFree( ci->department );
			ci->department = uci->department;
		}

		if ( uci->contact.category != NULL )
		{
			GlobalFree( ci->contact.category );
			ci->contact.category = uci->contact.category;
			GlobalFree( ci->category );
			ci->category = uci->category;
		}

		if ( uci->contact.email_address != NULL )
		{
			// If we have a blank value, then there should be no ID for that value.
			if ( ( ci->contact.email_address == NULL || ( ci->contact.email_address != NULL && ci->contact.email_address[ 0 ] != NULL ) ) && uci->contact.email_address[ 0 ] == NULL )
			{
				GlobalFree( ci->contact.email_address_id );
				ci->contact.email_address_id = NULL;

				GlobalFree( uci->contact.email_address_id );
				uci->contact.email_address_id = NULL;
			}

			GlobalFree( ci->contact.email_address );
			ci->contact.email_address = uci->contact.email_address;
			GlobalFree( ci->email_address );
			ci->email_address = uci->email_address;
		}

		if ( uci->contact.email_address_id != NULL )
		{
			GlobalFree( ci->contact.email_address_id );
			ci->contact.email_address_id = uci->contact.email_address_id;
		}

		if ( uci->contact.web_page != NULL )
		{
			// If we have a blank value, then there should be no ID for that value.
			if ( ( ci->contact.web_page == NULL || ( ci->contact.web_page != NULL && ci->contact.web_page[ 0 ] != NULL ) ) && uci->contact.web_page[ 0 ] == NULL )
			{
				GlobalFree( ci->contact.web_page_id );
				ci->contact.web_page_id = NULL;

				GlobalFree( uci->contact.web_page_id );
				uci->contact.web_page_id = NULL;
			}

			GlobalFree( ci->contact.web_page );
			ci->contact.web_page = uci->contact.web_page;
			GlobalFree( ci->web_page );
			ci->web_page = uci->web_page;
		}

		if ( uci->contact.web_page_id != NULL )
		{
			GlobalFree( ci->contact.web_page_id );
			ci->contact.web_page_id = uci->contact.web_page_id;
		}

		if ( uci->contact.home_phone_number != NULL )
		{
			// If we have a blank value, then there should be no ID for that value.
			if ( ( ci->contact.home_phone_number == NULL || ( ci->contact.home_phone_number != NULL && ci->contact.home_phone_number[ 0 ] != NULL ) ) && uci->contact.home_phone_number[ 0 ] == NULL )
			{
				GlobalFree( ci->contact.home_phone_number_id );
				ci->contact.home_phone_number_id = NULL;

				GlobalFree( uci->contact.home_phone_number_id );
				uci->contact.home_phone_number_id = NULL;
			}

			remove_custom_caller_id( ci );

			GlobalFree( ci->contact.home_phone_number );
			ci->contact.home_phone_number = uci->contact.home_phone_number;
			GlobalFree( ci->home_phone_number );
			ci->home_phone_number = FormatPhoneNumber( ci->contact.home_phone_number );

			add_custom_caller_id( ci );
		}

		if ( uci->contact.home_phone_number_id != NULL )
		{
			GlobalFree( ci->contact.home_phone_number_id );
			ci->contact.home_phone_number_id = uci->contact.home_phone_number_id;
		}

		if ( uci->contact.cell_phone_number != NULL )
		{
			// If we have a blank value, then there should be no ID for that value.
			if ( ( ci->contact.cell_phone_number == NULL || ( ci->contact.cell_phone_number != NULL && ci->contact.cell_phone_number[ 0 ] != NULL ) ) && uci->contact.cell_phone_number[ 0 ] == NULL )
			{
				GlobalFree( ci->contact.cell_phone_number_id );
				ci->contact.cell_phone_number_id = NULL;

				GlobalFree( uci->contact.cell_phone_number_id );
				uci->contact.cell_phone_number_id = NULL;
			}

			remove_custom_caller_id( ci );

			GlobalFree( ci->contact.cell_phone_number );
			ci->contact.cell_phone_number = uci->contact.cell_phone_number;
			GlobalFree( ci->cell_phone_number );
			ci->cell_phone_number = FormatPhoneNumber( ci->contact.cell_phone_number );

			add_custom_caller_id( ci );
		}

		if ( uci->contact.cell_phone_number_id != NULL )
		{
			GlobalFree( ci->contact.cell_phone_number_id );
			ci->contact.cell_phone_number_id = uci->contact.cell_phone_number_id;
		}

		if ( uci->contact.office_phone_number != NULL )
		{
			// If we have a blank value, then there should be no ID for that value.
			if ( ( ci->contact.office_phone_number == NULL || ( ci->contact.office_phone_number != NULL && ci->contact.office_phone_number[ 0 ] != NULL ) ) && uci->contact.office_phone_number[ 0 ] == NULL )
			{
				GlobalFree( ci->contact.office_phone_number_id );
				ci->contact.office_phone_number_id = NULL;

				GlobalFree( uci->contact.office_phone_number_id );
				uci->contact.office_phone_number_id = NULL;
			}

			remove_custom_caller_id( ci );

			GlobalFree( ci->contact.office_phone_number );
			ci->contact.office_phone_number = uci->contact.office_phone_number;
			GlobalFree( ci->office_phone_number );
			ci->office_phone_number = FormatPhoneNumber( ci->contact.office_phone_number );

			add_custom_caller_id( ci );
		}

		if ( uci->contact.office_phone_number_id != NULL )
		{
			GlobalFree( ci->contact.office_phone_number_id );
			ci->contact.office_phone_number_id = uci->contact.office_phone_number_id;
		}

		if ( uci->contact.other_phone_number != NULL )
		{
			// If we have a blank value, then there should be no ID for that value.
			if ( ( ci->contact.other_phone_number == NULL || ( ci->contact.other_phone_number != NULL && ci->contact.other_phone_number[ 0 ] != NULL ) ) && uci->contact.other_phone_number[ 0 ] == NULL )
			{
				GlobalFree( ci->contact.other_phone_number_id );
				ci->contact.other_phone_number_id = NULL;

				GlobalFree( uci->contact.other_phone_number_id );
				uci->contact.other_phone_number_id = NULL;
			}

			remove_custom_caller_id( ci );

			GlobalFree( ci->contact.other_phone_number );
			ci->contact.other_phone_number = uci->contact.other_phone_number;
			GlobalFree( ci->other_phone_number );
			ci->other_phone_number = FormatPhoneNumber( ci->contact.other_phone_number );

			add_custom_caller_id( ci );
		}

		if ( uci->contact.other_phone_number_id != NULL )
		{
			GlobalFree( ci->contact.other_phone_number_id );
			ci->contact.other_phone_number_id = uci->contact.other_phone_number_id;
		}

		if ( uci->contact.work_phone_number != NULL )
		{
			// If we have a blank value, then there should be no ID for that value.
			if ( ( ci->contact.work_phone_number == NULL || ( ci->contact.work_phone_number != NULL && ci->contact.work_phone_number[ 0 ] != NULL ) ) && uci->contact.work_phone_number[ 0 ] == NULL )
			{
				GlobalFree( ci->contact.work_phone_number_id );
				ci->contact.work_phone_number_id = NULL;

				GlobalFree( uci->contact.work_phone_number_id );
				uci->contact.work_phone_number_id = NULL;
			}

			remove_custom_caller_id( ci );

			GlobalFree( ci->contact.work_phone_number );
			ci->contact.work_phone_number = uci->contact.work_phone_number;
			GlobalFree( ci->work_phone_number );
			ci->work_phone_number = FormatPhoneNumber( ci->contact.work_phone_number );

			add_custom_caller_id( ci );
		}

		if ( uci->contact.work_phone_number_id != NULL )
		{
			GlobalFree( ci->contact.work_phone_number_id );
			ci->contact.work_phone_number_id = uci->contact.work_phone_number_id;
		}

		if ( uci->contact.fax_number != NULL )
		{
			// If we have a blank value, then there should be no ID for that value.
			if ( ( ci->contact.fax_number == NULL || ( ci->contact.fax_number != NULL && ci->contact.fax_number[ 0 ] != NULL ) ) && uci->contact.fax_number[ 0 ] == NULL )
			{
				GlobalFree( ci->contact.fax_number_id );
				ci->contact.fax_number_id = NULL;

				GlobalFree( uci->contact.fax_number_id );
				uci->contact.fax_number_id = NULL;
			}

			remove_custom_caller_id( ci );

			GlobalFree( ci->contact.fax_number );
			ci->contact.fax_number = uci->contact.fax_number;
			GlobalFree( ci->fax_number );
			ci->fax_number = FormatPhoneNumber( ci->contact.fax_number );

			add_custom_caller_id( ci );
		}

		if ( uci->contact.fax_number_id != NULL )
		{
			GlobalFree( ci->contact.fax_number_id );
			ci->contact.fax_number_id = uci->contact.fax_number_id;
		}

		if ( uci->picture_path != NULL )
		{
			GlobalFree( uci->picture_path );
		}

		GlobalFree( uci );
	}
	else if ( ci != NULL && uci == NULL )	// Add a single contact to the contact list listview.
	{
		ci->home_phone_number = FormatPhoneNumber( ci->contact.home_phone_number );
		ci->cell_phone_number = FormatPhoneNumber( ci->contact.cell_phone_number );
		ci->office_phone_number = FormatPhoneNumber( ci->contact.office_phone_number );
		ci->other_phone_number = FormatPhoneNumber( ci->contact.other_phone_number );

		ci->work_phone_number = FormatPhoneNumber( ci->contact.work_phone_number );
		ci->fax_number = FormatPhoneNumber( ci->contact.fax_number );

		ci->displayed = true;

		add_custom_caller_id( ci );

		lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETITEMCOUNT, 0, 0 );
		lvi.lParam = ( LPARAM )ci;	// lParam = our contactinfo structure from the connection thread.
		_SendMessageW( g_hWnd_contact_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
	}
	else if ( ui != NULL && ci == NULL && uci == NULL )	// Add all contacts in the contact_list to the contact list listview.
	{
		node_type *node = dllrbt_get_head( contact_list );
		while ( node != NULL )
		{
			ci = ( contactinfo * )node->val;

			// Add contacts that haven't been displayed yet.
			if ( ci != NULL && ci->displayed == false )
			{
				// First Name
				if ( ci->contact.first_name != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.first_name, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.first_name, -1, val, val_length );

					ci->first_name = val;
					val = NULL;
				}

				// Last Name
				if ( ci->contact.last_name != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.last_name, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.last_name, -1, val, val_length );

					ci->last_name = val;
					val = NULL;
				}

				// Nickname
				if ( ci->contact.nickname != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.nickname, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.nickname, -1, val, val_length );

					ci->nickname = val;
					val = NULL;
				}

				// Title
				if ( ci->contact.title != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.title, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.title, -1, val, val_length );

					ci->title = val;
					val = NULL;
				}

				// Business Name
				if ( ci->contact.business_name != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.business_name, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.business_name, -1, val, val_length );

					ci->business_name = val;
					val = NULL;
				}

				// Designation
				if ( ci->contact.designation != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.designation, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.designation, -1, val, val_length );

					ci->designation = val;
					val = NULL;
				}

				// Department
				if ( ci->contact.department != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.department, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.department, -1, val, val_length );

					ci->department = val;
					val = NULL;
				}

				// Category
				if ( ci->contact.category != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.category, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.category, -1, val, val_length );

					ci->category = val;
					val = NULL;
				}

				// Email Address
				if ( ci->contact.email_address != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.email_address, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.email_address, -1, val, val_length );

					ci->email_address = val;
					val = NULL;
				}

				// Web Page
				if ( ci->contact.web_page != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.web_page, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.web_page, -1, val, val_length );

					ci->web_page = val;
					val = NULL;
				}

				ci->home_phone_number = FormatPhoneNumber( ci->contact.home_phone_number );
				ci->cell_phone_number = FormatPhoneNumber( ci->contact.cell_phone_number );
				ci->office_phone_number = FormatPhoneNumber( ci->contact.office_phone_number );
				ci->other_phone_number = FormatPhoneNumber( ci->contact.other_phone_number );

				ci->work_phone_number = FormatPhoneNumber( ci->contact.work_phone_number );
				ci->fax_number = FormatPhoneNumber( ci->contact.fax_number );

				ci->displayed = true;

				add_custom_caller_id( ci );

				lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETITEMCOUNT, 0, 0 );
				lvi.lParam = ( LPARAM )ci;	// lParam = our contactinfo structure from the connection thread.
				_SendMessageW( g_hWnd_contact_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
			}
			
			node = node->next;
		}
	}
	else	// Remove all contacts from the contact_list.
	{
		// Clear the contact list.
		_SendMessageW( g_hWnd_contact_list, LVM_DELETEALLITEMS, 0, 0 );

		if ( contact_list != NULL )
		{
			// Free the values of the contact_list.
			node_type *node = dllrbt_get_head( contact_list );
			while ( node != NULL )
			{
				contactinfo *ci = ( contactinfo * )node->val;

				if ( ci != NULL )
				{
					free_contactinfo( &ci );
				}

				node = node->next;
			}

			dllrbt_delete_recursively( contact_list );

			contact_list = dllrbt_create( dllrbt_compare );

			if ( web_server_state == WEB_SERVER_STATE_RUNNING )
			{
				SetContactList( contact_list );
			}
		}

		cleanup_custom_caller_id();
	}

	GlobalFree( ui );

	Processing_Window( g_hWnd_contact_list, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN update_call_log( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	char range_number[ 32 ];

	displayinfo *di = ( displayinfo * )pArguments;

	if ( di != NULL )
	{
		// Each number in our call log contains a linked list. Find the number if it exists and add it to the linked list.

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
			else
			{
				call_log_changed = true;
			}
		}
		else	// If a phone number exits, insert the node into the linked list.
		{
			DLL_AddNode( &dll, di_node, -1 );	// Insert at the end of the doubly linked list.

			call_log_changed = true;
		}

		// Search the ignore_list for a match.
		ignoreinfo *ii = ( ignoreinfo * )dllrbt_find( ignore_list, ( void * )di->ci.call_from, true );

		// Try searching the range list.
		if ( ii == NULL )
		{
			_memzero( range_number, 32 );

			int range_index = lstrlenA( di->ci.call_from );
			range_index = ( range_index > 0 ? range_index - 1 : 0 );

			if ( RangeSearch( &ignore_range_list[ range_index ], di->ci.call_from, range_number ) == true )
			{
				ii = ( ignoreinfo * )dllrbt_find( ignore_list, ( void * )range_number, true );
			}
		}

		if ( ii != NULL )
		{
			di->process_incoming = false;
			di->ignore_phone_number = true;
			di->ci.ignored = true;

			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, IgnoreIncomingCall, ( void * )&( di->ci ), 0, NULL ) );

			ignoreupdateinfo *iui = ( ignoreupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreupdateinfo ) );
			iui->ii = ii;
			iui->phone_number = NULL;
			iui->action = 3;	// Update the call count.
			iui->hWnd = g_hWnd_ignore_list;

			// iui is freed in the update_ignore_list thread.
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_list, ( void * )iui, 0, NULL ) );
		}
		else
		{
			// Search for the first ignore caller ID list match. di->ignore_cid_match_count will be updated here for all keyword matches.
			ignorecidinfo *icidi = find_ignore_caller_id_name_match( di );
			
			if ( icidi != NULL )
			{
				di->process_incoming = false;
				di->ci.ignored = true;

				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, IgnoreIncomingCall, ( void * )&( di->ci ), 0, NULL ) );

				ignorecidupdateinfo *icidui = ( ignorecidupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignorecidupdateinfo ) );
				icidui->icidi = icidi;
				icidui->caller_id = NULL;
				icidui->action = 4;	// Update the call count.
				icidui->hWnd = g_hWnd_ignore_cid_list;
				icidui->match_case = false;
				icidui->match_whole_word = false;

				// icidui is freed in the update_ignore_cid_list thread.
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_cid_list, ( void * )icidui, 0, NULL ) );
			}
		}

		// Search the forward list for a match.
		forwardinfo *fi = ( forwardinfo * )dllrbt_find( forward_list, ( void * )di->ci.call_from, true );

		// Try searching the range list.
		if ( fi == NULL )
		{
			_memzero( range_number, 32 );

			int range_index = lstrlenA( di->ci.call_from );
			range_index = ( range_index > 0 ? range_index - 1 : 0 );

			if ( RangeSearch( &forward_range_list[ range_index ], di->ci.call_from, range_number ) == true )
			{
				fi = ( forwardinfo * )dllrbt_find( forward_list, ( void * )range_number, true );
			}
		}

		if ( fi != NULL )
		{
			di->forward_phone_number = true;

			// Ignore lists have priority. If they're set, then we don't forward.
			if ( di->ignore_phone_number == false && di->ignore_cid_match_count == 0 )
			{
				di->process_incoming = false;
				di->ci.forward_to = GlobalStrDupA( fi->c_forward_to );
				di->ci.forwarded = true;

				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, ForwardIncomingCall, ( void * )&( di->ci ), 0, NULL ) );

				forwardupdateinfo *fui = ( forwardupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardupdateinfo ) );
				fui->fi = fi;
				fui->call_from = NULL;
				fui->forward_to = NULL;
				fui->action = 4;	// Update the call count.
				fui->hWnd = g_hWnd_forward_list;

				// fui is freed in the update_forward_list thread.
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_forward_list, ( void * )fui, 0, NULL ) );
			}
		}
		else
		{
			// Search for the first forward caller ID list match. di->forward_cid_match_count will be updated here for all keyword matches.
			forwardcidinfo *fcidi = find_forward_caller_id_name_match( di );

			if ( fcidi != NULL )
			{
				// Ignore lists have priority. If they're set, then we don't forward.
				if ( di->ignore_phone_number == false && di->ignore_cid_match_count == 0 )
				{
					di->process_incoming = false;
					di->ci.forward_to = GlobalStrDupA( fcidi->c_forward_to );
					di->ci.forwarded = true;

					CloseHandle( ( HANDLE )_CreateThread( NULL, 0, ForwardIncomingCall, ( void * )&( di->ci ), 0, NULL ) );

					forwardcidupdateinfo *fcidui = ( forwardcidupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardcidupdateinfo ) );
					fcidui->fcidi = fcidi;
					fcidui->caller_id = NULL;
					fcidui->forward_to = NULL;
					fcidui->action = 4;	// Update the call count.
					fcidui->hWnd = g_hWnd_forward_cid_list;
					fcidui->match_case = false;
					fcidui->match_whole_word = false;

					// fcidui is freed in the update_forward_cid_list thread.
					CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_forward_cid_list, ( void * )fcidui, 0, NULL ) );
				}
			}
		}

		// If the incoming number matches a contact, then update the caller ID value.
		char *custom_caller_id = get_custom_caller_id( di->ci.call_from );
		if ( custom_caller_id != NULL )
		{
			GlobalFree( di->ci.caller_id );
			di->ci.caller_id = custom_caller_id;
		}

		_SendNotifyMessageW( g_hWnd_main, WM_PROPAGATE, MAKEWPARAM( CW_MODIFY, 0 ), ( LPARAM )di );	// Add entry to listview and show popup.
	}

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN read_call_log_history( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	Processing_Window( g_hWnd_call_log, false );

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\call_log_history.bin\0", 22 );
	base_directory[ base_directory_length + 21 ] = 0;	// Sanity.

	// Open our config file if it exists.
	HANDLE hFile_read = CreateFile( base_directory, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
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

		DWORD fz = GetFileSize( hFile_read, NULL );

		char *history_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 1 ) );	// 512 KB buffer.

		while ( total_read < fz )
		{
			// Stop processing and exit the thread.
			if ( kill_worker_thead == true )
			{
				break;
			}

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
				// Stop processing and exit the thread.
				if ( kill_worker_thead == true )
				{
					break;
				}

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

		CloseHandle( hFile_read );
	}

	Processing_Window( g_hWnd_call_log, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN save_call_log( void *file_path )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	Processing_Window( g_hWnd_call_log, false );

	// Open our config file if it exists.
	HANDLE hFile_call_log = CreateFile( ( wchar_t * )file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
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
			// Stop processing and exit the thread.
			if ( kill_worker_thead == true )
			{
				break;
			}

			DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
			while ( di_node != NULL )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thead == true )
				{
					break;
				}

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

	GlobalFree( file_path );

	Processing_Window( g_hWnd_call_log, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN copy_items( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	copyinfo *c_i = ( copyinfo * )pArguments;

	HWND hWnd = c_i->hWnd;

	unsigned column_type = 0;

	if ( hWnd == g_hWnd_call_log )
	{
		column_type = 0;
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		column_type = 1;
	}
	else if ( hWnd == g_hWnd_forward_list )
	{
		column_type = 2;
	}
	else if ( hWnd == g_hWnd_ignore_list )
	{
		column_type = 3;
	}
	else if ( hWnd == g_hWnd_forward_cid_list )
	{
		column_type = 4;
	}
	else if ( hWnd == g_hWnd_ignore_cid_list )
	{
		column_type = 5;
	}

	Processing_Window( hWnd, false );

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;
	lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

	int item_count = _SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
	int sel_count = _SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );
	
	bool copy_all = false;
	if ( item_count == sel_count )
	{
		copy_all = true;
	}
	else
	{
		item_count = sel_count;
	}

	unsigned int buffer_size = 8192;
	unsigned int buffer_offset = 0;
	wchar_t *copy_buffer = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * buffer_size );	// Allocate 8 kilobytes.

	int value_length = 0;

	int arr[ 17 ];

	// Set to however many columns we'll copy.
	int column_start = 0;
	int column_end = 1;

	if ( c_i->column == MENU_COPY_SEL )
	{
		column_end = ( column_type == 0 ? total_columns1 :
					 ( column_type == 1 ? total_columns2 :
					 ( column_type == 2 ? total_columns3 :
					 ( column_type == 3 ? total_columns4 :
					 ( column_type == 4 ? total_columns5 : total_columns6 ) ) ) ) );
		_SendMessageW( hWnd, LVM_GETCOLUMNORDERARRAY, column_end, ( LPARAM )arr );

		if ( column_type == 0 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, call_log_columns, NUM_COLUMNS1, total_columns1 );
		}
		else if ( column_type == 1 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, contact_list_columns, NUM_COLUMNS2, total_columns2 );
		}
		else if ( column_type == 2 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, forward_list_columns, NUM_COLUMNS3, total_columns3 );
		}
		else if ( column_type == 3 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, ignore_list_columns, NUM_COLUMNS4, total_columns4 );
		}
		else if ( column_type == 4 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, forward_cid_list_columns, NUM_COLUMNS5, total_columns5 );
		}
		else if ( column_type == 5 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, ignore_cid_list_columns, NUM_COLUMNS6, total_columns6 );
		}
	}
	else
	{
		switch ( c_i->column )
		{
			case MENU_COPY_SEL_COL1:
			case MENU_COPY_SEL_COL21:
			case MENU_COPY_SEL_COL31:
			case MENU_COPY_SEL_COL41:
			case MENU_COPY_SEL_COL51:
			case MENU_COPY_SEL_COL61: { arr[ 0 ] = 1; } break;

			case MENU_COPY_SEL_COL2:
			case MENU_COPY_SEL_COL22:
			case MENU_COPY_SEL_COL32:
			case MENU_COPY_SEL_COL42:
			case MENU_COPY_SEL_COL52:
			case MENU_COPY_SEL_COL62:{ arr[ 0 ] = 2; } break;

			case MENU_COPY_SEL_COL3:
			case MENU_COPY_SEL_COL23:
			case MENU_COPY_SEL_COL33:
			case MENU_COPY_SEL_COL53:
			case MENU_COPY_SEL_COL63: { arr[ 0 ] = 3; } break;

			case MENU_COPY_SEL_COL4:
			case MENU_COPY_SEL_COL24:
			case MENU_COPY_SEL_COL54:
			case MENU_COPY_SEL_COL64: { arr[ 0 ] = 4; } break;

			case MENU_COPY_SEL_COL5:
			case MENU_COPY_SEL_COL25:
			case MENU_COPY_SEL_COL55: { arr[ 0 ] = 5; } break;

			case MENU_COPY_SEL_COL6:
			case MENU_COPY_SEL_COL26: { arr[ 0 ] = 6; } break;

			case MENU_COPY_SEL_COL7:
			case MENU_COPY_SEL_COL27: { arr[ 0 ] = 7; } break;

			case MENU_COPY_SEL_COL8:
			case MENU_COPY_SEL_COL28: { arr[ 0 ] = 8; } break;

			case MENU_COPY_SEL_COL9:
			case MENU_COPY_SEL_COL29: { arr[ 0 ] = 9; } break;

			case MENU_COPY_SEL_COL10:
			case MENU_COPY_SEL_COL210: { arr[ 0 ] = 10; } break;

			case MENU_COPY_SEL_COL211: { arr[ 0 ] = 11; } break;
			case MENU_COPY_SEL_COL212: { arr[ 0 ] = 12; } break;
			case MENU_COPY_SEL_COL213: { arr[ 0 ] = 13; } break;
			case MENU_COPY_SEL_COL214: { arr[ 0 ] = 14; } break;
			case MENU_COPY_SEL_COL215: { arr[ 0 ] = 15; } break;
			case MENU_COPY_SEL_COL216: { arr[ 0 ] = 16; } break;
		}
	}

	wchar_t *copy_string = NULL;
	bool is_string = false;
	bool add_newline = false;
	bool add_tab = false;

	// Go through each item, and copy their lParam values.
	for ( int i = 0; i < item_count; ++i )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thead == true )
		{
			break;
		}

		if ( copy_all == true )
		{
			lvi.iItem = i;
		}
		else
		{
			lvi.iItem = _SendMessageW( hWnd, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
		}

		_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

		displayinfo *di = NULL;
		contactinfo *ci = NULL;
		ignoreinfo *ii = NULL;
		forwardinfo *fi = NULL;
		ignorecidinfo *icidi = NULL;
		forwardcidinfo *fcidi = NULL;

		if ( column_type == 0 )
		{
			di = ( displayinfo * )lvi.lParam;
		}
		else if ( column_type == 1 )
		{
			ci = ( contactinfo * )lvi.lParam;
		}
		else if ( column_type == 2 )
		{
			fi = ( forwardinfo * )lvi.lParam;
		}
		else if ( column_type == 3 )
		{
			ii = ( ignoreinfo * )lvi.lParam;
		}
		else if ( column_type == 4 )
		{
			fcidi = ( forwardcidinfo * )lvi.lParam;
		}
		else if ( column_type == 5 )
		{
			icidi = ( ignorecidinfo * )lvi.lParam;
		}

		is_string = add_newline = add_tab = false;

		for ( int j = column_start; j < column_end; ++j )
		{
			switch ( arr[ j ] )
			{
				case 1:
				case 2:
				{
					is_string = true;

					copy_string = ( column_type == 0 ? di->display_values[ arr[ j ] - 1 ] :
								  ( column_type == 1 ? ci->contactinfo_values[ arr[ j ] - 1 ] :
								  ( column_type == 2 ? fi->forwardinfo_values[ arr[ j ] - 1 ] :
								  ( column_type == 3 ? ii->ignoreinfo_values[ arr[ j ] - 1 ] :
								  ( column_type == 4 ? fcidi->forwardcidinfo_values[ arr[ j ] - 1 ] : icidi->ignorecidinfo_values[ arr[ j ] - 1 ] ) ) ) ) );
				}
				break;

				case 3:
				{
					is_string = true;

					copy_string = ( column_type == 0 ? di->display_values[ arr[ j ] - 1 ] :
								  ( column_type == 1 ? ci->contactinfo_values[ arr[ j ] - 1 ] :
								  ( column_type == 2 ? fi->forwardinfo_values[ arr[ j ] - 1 ] :
								  ( column_type == 4 ? fcidi->forwardcidinfo_values[ arr[ j ] - 1 ] : icidi->ignorecidinfo_values[ arr[ j ] - 1 ] ) ) ) );
				}
				break;

				case 4:
				{
					is_string = true;

					copy_string = ( column_type == 0 ? di->display_values[ arr[ j ] - 1 ] :
								  ( column_type == 1 ? ci->contactinfo_values[ arr[ j ] - 1 ] :
								  ( column_type == 4 ? fcidi->forwardcidinfo_values[ arr[ j ] - 1 ] : icidi->ignorecidinfo_values[ arr[ j ] - 1 ] ) ) );
				}
				break;

				case 5:
				{
					is_string = true;

					copy_string = ( column_type == 0 ? di->display_values[ arr[ j ] - 1 ] :
								  ( column_type == 1 ? ci->contactinfo_values[ arr[ j ] - 1 ] : fcidi->forwardcidinfo_values[ arr[ j ] - 1 ] ) );
				}
				break;

				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
				{
					is_string = true;

					copy_string = ( column_type == 0 ? di->display_values[ arr[ j ] - 1 ] : ci->contactinfo_values[ arr[ j ] - 1 ] );
				}
				break;

				case 11:
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				{
					is_string = true;

					copy_string = ci->contactinfo_values[ arr[ j ] - 1 ];
				}
				break;
			}

			if ( is_string == true )
			{
				is_string = false;

				if ( copy_string == NULL )
				{
					if ( j == 0 )
					{
						add_tab = false;
					}

					continue;
				}

				if ( j > 0 && add_tab == true )
				{
					*( copy_buffer + buffer_offset ) = L'\t';
					++buffer_offset;
				}

				add_tab = true;

				value_length = lstrlenW( copy_string );
				while ( buffer_offset + value_length + 3 >= buffer_size )	// Add +3 for \t and \r\n
				{
					buffer_size += 8192;
					wchar_t *realloc_buffer = ( wchar_t * )GlobalReAlloc( copy_buffer, sizeof( wchar_t ) * buffer_size, GMEM_MOVEABLE );
					if ( realloc_buffer == NULL )
					{
						goto CLEANUP;
					}

					copy_buffer = realloc_buffer;
				}
				_wmemcpy_s( copy_buffer + buffer_offset, buffer_size - buffer_offset, copy_string, value_length );
				buffer_offset += value_length;
			}

			add_newline = true;
		}

		if ( i < item_count - 1 && add_newline == true )	// Add newlines for every item except the last.
		{
			*( copy_buffer + buffer_offset ) = L'\r';
			++buffer_offset;
			*( copy_buffer + buffer_offset ) = L'\n';
			++buffer_offset;
		}
		else if ( ( i == item_count - 1 && add_newline == false ) && buffer_offset >= 2 )	// If add_newline is false for the last item, then a newline character is in the buffer.
		{
			buffer_offset -= 2;	// Ignore the last newline in the buffer.
		}
	}

	if ( _OpenClipboard( hWnd ) )
	{
		_EmptyClipboard();

		DWORD len = buffer_offset;

		// Allocate a global memory object for the text.
		HGLOBAL hglbCopy = GlobalAlloc( GMEM_MOVEABLE, sizeof( wchar_t ) * ( len + 1 ) );
		if ( hglbCopy != NULL )
		{
			// Lock the handle and copy the text to the buffer. lptstrCopy doesn't get freed.
			wchar_t *lptstrCopy = ( wchar_t * )GlobalLock( hglbCopy );
			if ( lptstrCopy != NULL )
			{
				_wmemcpy_s( lptstrCopy, len + 1, copy_buffer, len );
				lptstrCopy[ len ] = 0; // Sanity
			}

			GlobalUnlock( hglbCopy );

			if ( _SetClipboardData( CF_UNICODETEXT, hglbCopy ) == NULL )
			{
				GlobalFree( hglbCopy );	// Only free this Global memory if SetClipboardData fails.
			}

			_CloseClipboard();
		}
	}

CLEANUP:

	GlobalFree( copy_buffer );
	GlobalFree( c_i );

	Processing_Window( hWnd, true );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}
