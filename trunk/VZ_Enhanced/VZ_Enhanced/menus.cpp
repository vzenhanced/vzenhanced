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
#include "utilities.h"
#include "string_tables.h"
#include "menus.h"

#include "web_server.h"

HMENU g_hMenu = NULL;					// Handle to our menu bar.
HMENU g_hMenuSub_list_context = NULL;	// Handle to our context menu.
HMENU g_hMenuSub_contact_list_context = NULL;
HMENU g_hMenuSub_ignore_list_context = NULL;
HMENU g_hMenuSub_forward_list_context = NULL;
HMENU g_hMenuSub_header_context;		// Handle to our header context menu.
HMENU g_hMenuSub_tray = NULL;			// Handle to our tray menu.

HMENU g_hMenuSub_tabs_context = NULL;

char context_tab_index = -1;

bool column_l_menu_showing = false;		// Set if we right clicked a subitem and are displaying its copy menu item.
bool column_cl_menu_showing = false;
bool column_fl_menu_showing = false;
bool column_il_menu_showing = false;

bool l_phone_menu_showing = false;
bool cl_phone_menu_showing = false;
bool fl_phone_menu_showing = false;
bool il_phone_menu_showing = false;

bool l_incoming_menu_showing = false;

void DestroyMenus()
{
	_DestroyMenu( g_hMenuSub_tray );
	_DestroyMenu( g_hMenuSub_header_context );
	_DestroyMenu( g_hMenuSub_tabs_context );

	// The current edit menu will automatically be destroyed when its associated window is destroyed.
	HMENU hMenuSub_edit = _GetSubMenu( g_hMenu, 1 );

	if ( hMenuSub_edit != g_hMenuSub_list_context )
	{
		_DestroyMenu( g_hMenuSub_list_context );
	}

	if ( hMenuSub_edit != g_hMenuSub_contact_list_context )
	{
		_DestroyMenu( g_hMenuSub_contact_list_context );
	}

	if ( hMenuSub_edit != g_hMenuSub_ignore_list_context )
	{
		_DestroyMenu( g_hMenuSub_ignore_list_context );
	}

	if ( hMenuSub_edit != g_hMenuSub_forward_list_context )
	{
		_DestroyMenu( g_hMenuSub_forward_list_context );
	}
}

void CreateMenus()
{
	// Create our menu objects.
	g_hMenu = _CreateMenu();

	HMENU hMenuSub_file = _CreatePopupMenu();
	HMENU hMenuSub_view = _CreatePopupMenu();
	HMENU hMenuSub_tools = _CreatePopupMenu();
	HMENU hMenuSub_help = _CreatePopupMenu();

	HMENU g_hMenuSub_tabs = _CreatePopupMenu();
	HMENU g_hMenuSub_contacts = _CreatePopupMenu();

	g_hMenuSub_list_context = _CreatePopupMenu();
	g_hMenuSub_contact_list_context = _CreatePopupMenu();
	g_hMenuSub_ignore_list_context = _CreatePopupMenu();
	g_hMenuSub_forward_list_context = _CreatePopupMenu();
	g_hMenuSub_header_context = _CreatePopupMenu();
	g_hMenuSub_tray = _CreatePopupMenu();
	g_hMenuSub_tabs_context = _CreatePopupMenu();


	// FILE MENU
	MENUITEMINFO mii;
	_memzero( &mii, sizeof( MENUITEMINFO ) );
	mii.cbSize = sizeof( MENUITEMINFO );
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST__Log_In___;
	mii.cch = 10;
	mii.wID = MENU_LOGIN;
	_InsertMenuItemW( hMenuSub_file, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_file, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_E_xit;
	mii.cch = 5;
	mii.wID = MENU_EXIT;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( hMenuSub_file, 2, TRUE, &mii );


	// EDIT MENUS
	mii.dwTypeData = ST_Add_to_Forward_List___;
	mii.cch = 22;
	mii.wID = MENU_FORWARD_LIST;
	mii.fState = MFS_DISABLED;
	_InsertMenuItemW( g_hMenuSub_list_context, 0, TRUE, &mii );

	mii.dwTypeData = ST_Add_to_Ignore_List;
	mii.cch = 18;
	mii.wID = MENU_IGNORE_LIST;
	_InsertMenuItemW( g_hMenuSub_list_context, 1, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_list_context, 2, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Add_Contact___;
	mii.cch = 14;
	mii.wID = MENU_ADD_CONTACT;
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 0, TRUE, &mii );

	mii.dwTypeData = ST_Add_to_Ignore_List___;
	mii.cch = 21;
	mii.wID = MENU_ADD_IGNORE_LIST;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( g_hMenuSub_ignore_list_context, 0, TRUE, &mii );

	mii.dwTypeData = ST_Add_to_Forward_List___;
	mii.cch = 22;
	mii.wID = MENU_ADD_FORWARD_LIST;
	_InsertMenuItemW( g_hMenuSub_forward_list_context, 0, TRUE, &mii );

	mii.dwTypeData = ST_Edit_Contact___;
	mii.cch = 15;
	mii.wID = MENU_EDIT_CONTACT;
	mii.fState = MFS_DISABLED;
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 1, TRUE, &mii );

	mii.dwTypeData = ST_Edit_Forward_Entry___;
	mii.wID = MENU_EDIT_FORWARD_LIST;
	mii.cch = 21;
	_InsertMenuItemW( g_hMenuSub_forward_list_context, 1, TRUE, &mii );

	mii.dwTypeData = ST_Remove_Selected;
	mii.cch = 15;
	mii.wID = MENU_REMOVE_SEL;
	_InsertMenuItemW( g_hMenuSub_list_context, 3, TRUE, &mii );

	mii.dwTypeData = ST_Delete_Contact;
	mii.cch = 14;
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 2, TRUE, &mii );

	mii.dwTypeData = ST_Remove_from_Ignore_List;
	mii.cch = 23;
	_InsertMenuItemW( g_hMenuSub_ignore_list_context, 1, TRUE, &mii );

	mii.dwTypeData = ST_Remove_from_Forward_List;
	mii.cch = 24;
	_InsertMenuItemW( g_hMenuSub_forward_list_context, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_list_context, 4, TRUE, &mii );
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 3, TRUE, &mii );
	_InsertMenuItemW( g_hMenuSub_ignore_list_context, 2, TRUE, &mii );
	_InsertMenuItemW( g_hMenuSub_forward_list_context, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Copy_Selected;
	mii.cch = 13;
	mii.wID = MENU_COPY_SEL;
	_InsertMenuItemW( g_hMenuSub_list_context, 5, TRUE, &mii );
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 4, TRUE, &mii );
	_InsertMenuItemW( g_hMenuSub_ignore_list_context, 3, TRUE, &mii );
	_InsertMenuItemW( g_hMenuSub_forward_list_context, 4, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_list_context, 6, TRUE, &mii );
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 5, TRUE, &mii );
	_InsertMenuItemW( g_hMenuSub_ignore_list_context, 4, TRUE, &mii );
	_InsertMenuItemW( g_hMenuSub_forward_list_context, 5, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Select_All;
	mii.cch = 10;
	mii.wID = MENU_SELECT_ALL;
	_InsertMenuItemW( g_hMenuSub_list_context, 7, TRUE, &mii );
	_InsertMenuItemW( g_hMenuSub_contact_list_context, 6, TRUE, &mii );
	_InsertMenuItemW( g_hMenuSub_ignore_list_context, 5, TRUE, &mii );
	_InsertMenuItemW( g_hMenuSub_forward_list_context, 6, TRUE, &mii );


	// VIEW SUBMENU - TABS
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Call_Log;
	mii.cch = 8;
	mii.wID = MENU_VIEW_CALL_LOG;
	mii.fState = MFS_ENABLED | ( cfg_tab_order1 != -1 ? MFS_CHECKED : 0 );
	_InsertMenuItemW( g_hMenuSub_tabs, 0, TRUE, &mii );

	mii.dwTypeData = ST_Contact_List;
	mii.cch = 12;
	mii.wID = MENU_VIEW_CONTACT_LIST;
	mii.fState = MFS_ENABLED | ( cfg_tab_order2 != -1 ? MFS_CHECKED : 0 );
	_InsertMenuItemW( g_hMenuSub_tabs, 1, TRUE, &mii );

	mii.dwTypeData = ST_Forward_List;
	mii.cch = 12;
	mii.wID = MENU_VIEW_FORWARD_LIST;
	mii.fState = MFS_ENABLED | ( cfg_tab_order3 != -1 ? MFS_CHECKED : 0 );
	_InsertMenuItemW( g_hMenuSub_tabs, 2, TRUE, &mii );

	mii.dwTypeData = ST_Ignore_List;
	mii.cch = 11;
	mii.wID = MENU_VIEW_IGNORE_LIST;
	mii.fState = MFS_ENABLED | ( cfg_tab_order4 != -1 ? MFS_CHECKED : 0 );
	_InsertMenuItemW( g_hMenuSub_tabs, 3, TRUE, &mii );


	// VIEW MENU
	mii.dwTypeData = ST_Select__Columns___;
	mii.cch = 18;
	mii.wID = MENU_SELECT_COLUMNS;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( hMenuSub_view, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_view, 1, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Tabs;
	mii.cch = 4;
	mii.hSubMenu = g_hMenuSub_tabs;
	_InsertMenuItemW( hMenuSub_view, 2, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_view, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Account_Information;
	mii.cch = 19;
	mii.wID = MENU_ACCOUNT;
	mii.fState = MFS_DISABLED;
	_InsertMenuItemW( hMenuSub_view, 4, TRUE, &mii );


	// TOOLS SUBMENU - CONTACTS
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Export_Contacts___;
	mii.cch = 18;
	mii.wID = MENU_EXPORT_CONTACTS;
	_InsertMenuItemW( g_hMenuSub_contacts, 0, TRUE, &mii );

	mii.dwTypeData = ST_Import_Contacts___;
	mii.cch = 18;
	mii.wID = MENU_IMPORT_CONTACTS;
	_InsertMenuItemW( g_hMenuSub_contacts, 1, TRUE, &mii );


	// TOOLS MENU
	mii.dwTypeData = ST_Dial_Phone_Number___;
	mii.cch = 20;
	mii.wID = MENU_DIAL_PHONE_NUM;
	_InsertMenuItemW( hMenuSub_tools, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_tools, 1, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Contacts;
	mii.cch = 8;
	mii.hSubMenu = g_hMenuSub_contacts;
	_InsertMenuItemW( hMenuSub_tools, 2, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_tools, 3, TRUE, &mii );

	if ( web_server_state == WEB_SERVER_STATE_RUNNING )
	{
		mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
		mii.fType = MFT_STRING;
		mii.dwTypeData = ST_Web_Server;
		mii.cch = 10;
		mii.hSubMenu = GetWebServerMenu();
		_InsertMenuItemW( hMenuSub_tools, 4, TRUE, &mii );

		mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		mii.fType = MFT_SEPARATOR;
		_InsertMenuItemW( hMenuSub_tools, 5, TRUE, &mii );
	}

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST__Options___;
	mii.cch = 11;
	mii.wID = MENU_OPTIONS;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( hMenuSub_tools, ( web_server_state == WEB_SERVER_STATE_RUNNING ? 6 : 4 ), TRUE, &mii );


	// HELP MENU
	mii.dwTypeData = ST__About;
	mii.cch = 6;
	mii.wID = MENU_ABOUT;
	_InsertMenuItemW( hMenuSub_help, 0, TRUE, &mii );



	// MENU BAR
	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.dwTypeData = ST__File;
	mii.cch = 5;
	mii.hSubMenu = hMenuSub_file;
	_InsertMenuItemW( g_hMenu, 0, TRUE, &mii );

	mii.dwTypeData = ST__Edit;
	mii.cch = 5;
	mii.hSubMenu = g_hMenuSub_list_context;
	_InsertMenuItemW( g_hMenu, 1, TRUE, &mii );

	mii.dwTypeData = ST__View;
	mii.cch = 5;
	mii.hSubMenu = hMenuSub_view;
	_InsertMenuItemW( g_hMenu, 2, TRUE, &mii );

	mii.dwTypeData = ST__Tools;
	mii.cch = 6;
	mii.hSubMenu = hMenuSub_tools;
	_InsertMenuItemW( g_hMenu, 3, TRUE, &mii );

	mii.dwTypeData = ST__Help;
	mii.cch = 5;
	mii.hSubMenu = hMenuSub_help;
	_InsertMenuItemW( g_hMenu, 4, TRUE, &mii );


	// TRAY MENU (for right click)
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Open_Caller_ID;
	mii.cch = 14;
	mii.wID = MENU_RESTORE;
	mii.fState = MFS_DEFAULT | MFS_ENABLED;
	_InsertMenuItemW( g_hMenuSub_tray, 0, TRUE, &mii );

	mii.fState = MFS_ENABLED;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Options___;
	mii.cch = 10;
	mii.wID = MENU_OPTIONS;
	_InsertMenuItemW( g_hMenuSub_tray, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Log_In___;
	mii.cch = 9;
	mii.wID = MENU_LOGIN;
	_InsertMenuItemW( g_hMenuSub_tray, 4, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 5, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Exit;
	mii.cch = 4;
	mii.wID = MENU_EXIT;
	_InsertMenuItemW( g_hMenuSub_tray, 6, TRUE, &mii );


	// HEADER CONTEXT MENU (for right click)
	mii.fState = MFS_ENABLED;
	mii.dwTypeData = ST_Select_Columns___;
	mii.cch = 17;
	mii.wID = MENU_SELECT_COLUMNS;
	_InsertMenuItemW( g_hMenuSub_header_context, 0, TRUE, &mii );


	// TAB CONTEXT MENU (for right click)
	mii.dwTypeData = ST_Close_Tab;
	mii.cch = 9;
	mii.wID = MENU_CLOSE_TAB;
	_InsertMenuItemW( g_hMenuSub_tabs_context, 0, TRUE, &mii );
}

void ChangeMenuByHWND( HWND hWnd )
{
	// SetMenuItemInfo returns 87 when I try to swap submenus. Don't know why.

	MENUITEMINFO mii;
	_memzero( &mii, sizeof( MENUITEMINFO ) );
	mii.cbSize = sizeof( MENUITEMINFO );
	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.dwTypeData = ST__Edit;
	mii.cch = 5;

	_RemoveMenu( g_hMenu, 1, MF_BYPOSITION );

	if ( hWnd == g_hWnd_list )
	{
		mii.hSubMenu = g_hMenuSub_list_context;
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		mii.hSubMenu = g_hMenuSub_contact_list_context;
	}
	else if ( hWnd == g_hWnd_ignore_list )
	{
		mii.hSubMenu = g_hMenuSub_ignore_list_context;
	}
	else if ( hWnd == g_hWnd_forward_list )
	{
		mii.hSubMenu = g_hMenuSub_forward_list_context;
	}

	_InsertMenuItemW( g_hMenu, 1, TRUE, &mii );

	_DrawMenuBar( g_hWnd_main );
}

void HandleRightClick( HWND hWnd )
{
	POINT p;
	_GetCursorPos( &p );

	bool show_call_menu = false;

	if ( hWnd == g_hWnd_list )	// List item was clicked on.
	{
		POINT cp;
		cp.x = p.x;
		cp.y = p.y;

		_ScreenToClient( hWnd, &cp );

		LVHITTESTINFO hti;
		hti.pt = cp;
		_SendMessageW( hWnd, LVM_SUBITEMHITTEST, 0, ( LPARAM )&hti );

		if ( hti.iSubItem > 0 )
		{
			int sel_count = _SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

			if ( sel_count > 0 )
			{
				// Get the virtual index from the column index.
				int vindex = GetVirtualIndexFromColumnIndex( hti.iSubItem, list_columns, NUM_COLUMNS );

				MENUITEMINFO mii;
				_memzero( &mii, sizeof( MENUITEMINFO ) );
				mii.cbSize = sizeof( MENUITEMINFO );
				mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
				mii.fType = MFT_STRING;

				MENUITEMINFO mii2 = mii;

				if ( hti.iItem != -1 )
				{
					mii.fState = MFS_ENABLED;

					if ( login_state != LOGGED_IN )
					{
						mii2.fState = MFS_DISABLED;
					}
					else
					{
						mii2.fState = MFS_ENABLED;
					}
				}
				else
				{
					mii.fState = MFS_DISABLED;
					mii2.fState = MFS_DISABLED;
				}

				switch ( vindex )
				{
					case 1:
					{
						mii.wID = MENU_COPY_SEL_COL1;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Caller_IDs;
							mii.cch = 15;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Caller_ID;
							mii.cch = 14;
						}
					}
					break;

					case 2:
					{
						mii.wID = MENU_COPY_SEL_COL2;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Forward_States;
							mii.cch = 19;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Forward_State;
							mii.cch = 18;
						}
					}
					break;

					case 3:
					{
						mii2.wID = MENU_CALL_PHONE_COL13;
						mii2.dwTypeData = ST_Call_Forwarded_to_Phone_Number___;
						mii2.cch = 33;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL3;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Forwarded_to_Phone_Numbers;
							mii.cch = 31;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Forwarded_to_Phone_Number;
							mii.cch = 30;
						}
					}
					break;

					case 4:
					{
						mii.wID = MENU_COPY_SEL_COL4;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Ignore_States;
							mii.cch = 18;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Ignore_State;
							mii.cch = 17;
						}
					}
					break;

					case 5:
					{
						mii2.wID = MENU_CALL_PHONE_COL15;
						mii2.dwTypeData = ST_Call_Phone_Number___;
						mii2.cch = 20;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL5;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Phone_Numbers;
							mii.cch = 18;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Phone_Number;
							mii.cch = 17;
						}
					}
					break;

					case 6:
					{
						mii.wID = MENU_COPY_SEL_COL6;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_References;
							mii.cch = 14;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Reference;
							mii.cch = 14;
						}
					}
					break;

					case 7:
					{
						mii2.wID = MENU_CALL_PHONE_COL17;
						mii2.dwTypeData = ST_Call_Sent_to_Phone_Number___;
						mii2.cch = 28;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL7;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Sent_to_Phone_Numbers;
							mii.cch = 26;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Sent_to_Phone_Number;
							mii.cch = 25;
						}
					}
					break;

					case 8:
					{
						mii.wID = MENU_COPY_SEL_COL8;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Times;
							mii.cch = 10;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Time;
							mii.cch = 9;
						}
					}
					break;
				}

				unsigned char menu_offset = ( l_incoming_menu_showing == true ? 3 : 0 );

				if ( column_l_menu_showing == true )
				{
					_SetMenuItemInfoW( g_hMenuSub_list_context, ( l_phone_menu_showing == true ? 7 : 5 ) + menu_offset, TRUE, &mii );
				}
				else
				{
					_InsertMenuItemW( g_hMenuSub_list_context, ( l_phone_menu_showing == true ? 7 : 5 ) + menu_offset, TRUE, &mii );

					column_l_menu_showing = true;
				}

				if ( show_call_menu == true )
				{
					if ( l_phone_menu_showing == true )
					{
						_SetMenuItemInfoW( g_hMenuSub_list_context, 0 + menu_offset, TRUE, &mii2 );
					}
					else
					{
						_InsertMenuItemW( g_hMenuSub_list_context, 0 + menu_offset, TRUE, &mii2 );
						
						mii2.fType = MFT_SEPARATOR;
						_InsertMenuItemW( g_hMenuSub_list_context, 1 + menu_offset, TRUE, &mii2 );

						l_phone_menu_showing = true;
					}
				}
				else
				{
					if ( l_phone_menu_showing == true )
					{
						_DeleteMenu( g_hMenuSub_list_context, 1 + menu_offset, MF_BYPOSITION );	// Separator
						_DeleteMenu( g_hMenuSub_list_context, 0 + menu_offset, MF_BYPOSITION );

						l_phone_menu_showing = false;
					}
				}
			}
		}
		else
		{
			if ( l_incoming_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_list_context, 2, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );
				_DeleteMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );

				l_incoming_menu_showing = false;
			}

			if ( l_phone_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );

				l_phone_menu_showing = false;
			}

			if ( column_l_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_list_context, 5, MF_BYPOSITION );

				column_l_menu_showing = false;
			}
		}

		// Display the ignore and forward incoming call menu items for 30 seconds.

		int sel_count1 = _SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

		if ( sel_count1 == 1 )
		{
			// Retrieve the lParam value from the selected listview item.
			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;
			lvi.iItem = _SendMessageW( hWnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

			if ( lvi.iItem != -1 )
			{
				_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

				displayinfo *di = ( displayinfo * )lvi.lParam;

				if ( di->process_incoming == true )
				{
					SYSTEMTIME SystemTime;
					GetLocalTime( &SystemTime );

					FILETIME FileTime;
					SystemTimeToFileTime( &SystemTime, &FileTime );

					//__int64 current_time = 0;
					//_memcpy_s( ( void * )&current_time, sizeof( __int64 ), ( void * )&FileTime, sizeof( __int64 ) );
					LARGE_INTEGER li;
					li.LowPart = FileTime.dwLowDateTime;
					li.HighPart = FileTime.dwHighDateTime;

					// See if the elapsed time is less than 30 seconds.
					if ( ( li.QuadPart - di->time.QuadPart ) <= 30 * FILETIME_TICKS_PER_SECOND )
					{
						if ( l_incoming_menu_showing == false )
						{
							MENUITEMINFO mii3;
							_memzero( &mii3, sizeof( MENUITEMINFO ) );
							mii3.cbSize = sizeof( MENUITEMINFO );
							mii3.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
							mii3.fType = MFT_STRING;

							if ( login_state != LOGGED_IN )
							{
								mii3.fState = MFS_DISABLED;
							}
							else
							{
								mii3.fState = MFS_ENABLED;
							}

							mii3.wID = MENU_INCOMING_FORWARD;
							mii3.dwTypeData = ST_Forward_Incoming_Call___;
							mii3.cch = 24;
							_InsertMenuItemW( g_hMenuSub_list_context, 0, TRUE, &mii3 );

							mii3.wID = MENU_INCOMING_IGNORE;
							mii3.dwTypeData = ST_Ignore_Incoming_Call;
							mii3.cch = 20;
							_InsertMenuItemW( g_hMenuSub_list_context, 1, TRUE, &mii3 );

							mii3.fType = MFT_SEPARATOR;
							_InsertMenuItemW( g_hMenuSub_list_context, 2, TRUE, &mii3 );

							l_incoming_menu_showing = true;
						}
					}
					else	// If it's greater than 20 seconds, then remove the menus and disable them from showing again.
					{
						if ( l_incoming_menu_showing == true )
						{
							_DeleteMenu( g_hMenuSub_list_context, 2, MF_BYPOSITION );	// Separator
							_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );
							_DeleteMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );

							l_incoming_menu_showing = false;
						}

						di->process_incoming = false;
					}
				}
				else if ( di->process_incoming == false && l_incoming_menu_showing == true )
				{
					_DeleteMenu( g_hMenuSub_list_context, 2, MF_BYPOSITION );	// Separator
					_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );
					_DeleteMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );

					l_incoming_menu_showing = false;
				}
			}
		}
		else
		{
			if ( l_incoming_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_list_context, 2, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );
				_DeleteMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );

				l_incoming_menu_showing = false;
			}
		}

		// Show our edit context menu as a popup.
		_TrackPopupMenu( g_hMenuSub_list_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		POINT cp;
		cp.x = p.x;
		cp.y = p.y;

		_ScreenToClient(hWnd, &cp );

		LVHITTESTINFO hti;
		hti.pt = cp;
		_SendMessageW( hWnd, LVM_SUBITEMHITTEST, 0, ( LPARAM )&hti );

		if ( hti.iSubItem > 0 )
		{
			int sel_count = _SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

			if ( sel_count > 0 )
			{
				// Get the virtual index from the column index.
				int vindex = GetVirtualIndexFromColumnIndex( hti.iSubItem, contact_list_columns, NUM_COLUMNS2 );

				MENUITEMINFO mii;
				_memzero( &mii, sizeof( MENUITEMINFO ) );
				mii.cbSize = sizeof( MENUITEMINFO );
				mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
				mii.fType = MFT_STRING;

				MENUITEMINFO mii2 = mii;

				if ( hti.iItem != -1 )
				{
					mii.fState = MFS_ENABLED;

					if ( login_state != LOGGED_IN )
					{
						mii2.fState = MFS_DISABLED;
					}
					else
					{
						mii2.fState = MFS_ENABLED;
					}
				}
				else
				{
					mii.fState = MFS_DISABLED;
					mii2.fState = MFS_DISABLED;
				}

				switch ( vindex )
				{
					case 1:
					{
						mii2.wID = MENU_CALL_PHONE_COL21;
						mii2.dwTypeData = ST_Call_Cell_Phone_Number___;
						mii2.cch = 25;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL21;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Cell_Phone_Numbers;
							mii.cch = 23;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Cell_Phone_Number;
							mii.cch = 22;
						}
					}
					break;

					case 2:
					{
						mii.wID = MENU_COPY_SEL_COL22;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Companies;
							mii.cch = 14;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Company;
							mii.cch = 12;
						}
					}
					break;

					case 3:
					{
						mii.wID = MENU_COPY_SEL_COL23;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Departments;
							mii.cch = 16;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Department;
							mii.cch = 15;
						}
					}
					break;

					case 4:
					{
						mii2.fState = MFS_ENABLED;
						mii2.wID = MENU_OPEN_EMAIL_ADDRESS;
						mii2.dwTypeData = ST_Send_Email___;
						mii2.cch = 13;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL24;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Email_Addresses;
							mii.cch = 20;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Email_Address;
							mii.cch = 18;
						}
					}
					break;

					case 5:
					{
						mii2.wID = MENU_CALL_PHONE_COL25;
						mii2.dwTypeData = ST_Call_Fax_Number___;
						mii2.cch = 18;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL25;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Fax_Numbers;
							mii.cch = 16;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Fax_Number;
							mii.cch = 15;
						}
					}
					break;

					case 6:
					{
						mii.wID = MENU_COPY_SEL_COL26;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_First_Names;
							mii.cch = 16;
						}
						else
						{
							mii.dwTypeData = ST_Copy_First_Name;
							mii.cch = 15;
						}
					}
					break;

					case 7:
					{
						mii2.wID = MENU_CALL_PHONE_COL27;
						mii2.dwTypeData = ST_Call_Home_Phone_Number___;
						mii2.cch = 25;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL27;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Home_Phone_Numbers;
							mii.cch = 23;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Home_Phone_Number;
							mii.cch = 22;
						}
					}
					break;

					case 8:
					{
						mii.wID = MENU_COPY_SEL_COL28;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Job_Titles;
							mii.cch = 15;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Job_Title;
							mii.cch = 14;
						}
					}
					break;

					case 9:
					{
						mii.wID = MENU_COPY_SEL_COL29;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Last_Names;
							mii.cch = 15;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Last_Name;
							mii.cch = 14;
						}
					}
					break;

					case 10:
					{
						mii.wID = MENU_COPY_SEL_COL210;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Nicknames;
							mii.cch = 14;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Nickname;
							mii.cch = 13;
						}
					}
					break;

					case 11:
					{
						mii2.wID = MENU_CALL_PHONE_COL211;
						mii2.dwTypeData = ST_Call_Office_Phone_Number___;
						mii2.cch = 27;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL211;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Office_Phone_Numbers;
							mii.cch = 25;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Office_Phone_Number;
							mii.cch = 24;
						}
					}
					break;

					case 12:
					{
						mii2.wID = MENU_CALL_PHONE_COL212;
						mii2.dwTypeData = ST_Call_Other_Phone_Number___;
						mii2.cch = 26;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL212;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Other_Phone_Numbers;
							mii.cch = 24;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Other_Phone_Number;
							mii.cch = 23;
						}
					}
					break;

					case 13:
					{
						mii.wID = MENU_COPY_SEL_COL213;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Professions;
							mii.cch = 16;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Profession;
							mii.cch = 15;
						}
					}
					break;

					case 14:
					{
						mii.wID = MENU_COPY_SEL_COL214;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Titles;
							mii.cch = 11;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Title;
							mii.cch = 10;
						}
					}
					break;

					case 15:
					{
						mii2.fState = MFS_ENABLED;
						mii2.wID = MENU_OPEN_WEB_PAGE;
						mii2.dwTypeData = ST_Open_Web_Page;
						mii2.cch = 13;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL215;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Web_Pages;
							mii.cch = 14;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Web_Page;
							mii.cch = 13;
						}
					}
					break;

					case 16:
					{
						mii2.wID = MENU_CALL_PHONE_COL216;
						mii2.dwTypeData = ST_Call_Work_Phone_Number___;
						mii2.cch = 25;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL216;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Work_Phone_Numbers;
							mii.cch = 23;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Work_Phone_Number;
							mii.cch = 22;
						}
					}
					break;
				}

				if ( column_cl_menu_showing == true )
				{
					_SetMenuItemInfoW( g_hMenuSub_contact_list_context, ( cl_phone_menu_showing == true ? 6 : 4 ), TRUE, &mii );
				}
				else
				{
					_InsertMenuItemW( g_hMenuSub_contact_list_context, ( cl_phone_menu_showing == true ? 6 : 4 ), TRUE, &mii );

					column_cl_menu_showing = true;
				}

				if ( show_call_menu == true )
				{
					if ( cl_phone_menu_showing == true )
					{
						_SetMenuItemInfoW( g_hMenuSub_contact_list_context, 0, TRUE, &mii2 );
					}
					else
					{
						_InsertMenuItemW( g_hMenuSub_contact_list_context, 0, TRUE, &mii2 );
						
						mii2.fType = MFT_SEPARATOR;
						_InsertMenuItemW( g_hMenuSub_contact_list_context, 1, TRUE, &mii2 );

						cl_phone_menu_showing = true;
					}
				}
				else
				{
					if ( cl_phone_menu_showing == true )
					{
						_DeleteMenu( g_hMenuSub_contact_list_context, 1, MF_BYPOSITION );	// Separator
						_DeleteMenu( g_hMenuSub_contact_list_context, 0, MF_BYPOSITION );

						cl_phone_menu_showing = false;
					}
				}
			}
		}
		else
		{
			if ( cl_phone_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_contact_list_context, 1, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_contact_list_context, 0, MF_BYPOSITION );

				cl_phone_menu_showing = false;
			}

			if ( column_cl_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_contact_list_context, 4, MF_BYPOSITION );

				column_cl_menu_showing = false;
			}
		}

		// Show our edit context menu as a popup.
		_TrackPopupMenu( g_hMenuSub_contact_list_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
	}
	else if ( hWnd == g_hWnd_forward_list )
	{
		POINT cp;
		cp.x = p.x;
		cp.y = p.y;

		_ScreenToClient( hWnd, &cp );

		LVHITTESTINFO hti;
		hti.pt = cp;
		_SendMessageW( hWnd, LVM_SUBITEMHITTEST, 0, ( LPARAM )&hti );

		if ( hti.iSubItem > 0 )
		{
			int sel_count = _SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

			if ( sel_count > 0 )
			{
				// Get the virtual index from the column index.
				int vindex = GetVirtualIndexFromColumnIndex( hti.iSubItem, forward_list_columns, NUM_COLUMNS3 );

				MENUITEMINFO mii;
				_memzero( &mii, sizeof( MENUITEMINFO ) );
				mii.cbSize = sizeof( MENUITEMINFO );
				mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
				mii.fType = MFT_STRING;

				MENUITEMINFO mii2 = mii;

				if ( hti.iItem != -1 )
				{
					mii.fState = MFS_ENABLED;

					if ( login_state != LOGGED_IN )
					{
						mii2.fState = MFS_DISABLED;
					}
					else
					{
						mii2.fState = MFS_ENABLED;
					}
				}
				else
				{
					mii.fState = MFS_DISABLED;
					mii2.fState = MFS_DISABLED;
				}

				switch ( vindex )
				{
					case 1:
					{
						mii2.wID = MENU_CALL_PHONE_COL31;
						mii2.dwTypeData = ST_Call_Forward_to_Phone_Number___;
						mii2.cch = 31;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL31;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Forward_to_Phone_Numbers;
							mii.cch = 29;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Forward_to_Phone_Number;
							mii.cch = 28;
						}
					}
					break;

					case 2:
					{
						mii2.wID = MENU_CALL_PHONE_COL32;
						mii2.dwTypeData = ST_Call_Phone_Number___;
						mii2.cch = 20;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL32;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Phone_Numbers;
							mii.cch = 18;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Phone_Number;
							mii.cch = 17;
						}
					}
					break;

					case 3:
					{
						mii.wID = MENU_COPY_SEL_COL33;

						mii.dwTypeData = ST_Copy_Total_Calls;
						mii.cch = 16;
					}
					break;
				}

				if ( column_fl_menu_showing == true )
				{
					_SetMenuItemInfoW( g_hMenuSub_forward_list_context, ( fl_phone_menu_showing == true ? 6 : 4 ), TRUE, &mii );
				}
				else
				{
					_InsertMenuItemW( g_hMenuSub_forward_list_context, ( fl_phone_menu_showing == true ? 6 : 4 ), TRUE, &mii );

					column_fl_menu_showing = true;
				}

				if ( show_call_menu == true )
				{
					if ( fl_phone_menu_showing == true )
					{
						_SetMenuItemInfoW( g_hMenuSub_forward_list_context, 0, TRUE, &mii2 );
					}
					else
					{
						_InsertMenuItemW( g_hMenuSub_forward_list_context, 0, TRUE, &mii2 );
						
						mii2.fType = MFT_SEPARATOR;
						_InsertMenuItemW( g_hMenuSub_forward_list_context, 1, TRUE, &mii2 );

						fl_phone_menu_showing = true;
					}
				}
			}
		}
		else
		{
			if ( fl_phone_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_forward_list_context, 1, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_forward_list_context, 0, MF_BYPOSITION );

				fl_phone_menu_showing = false;
			}

			if ( column_fl_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_forward_list_context, 4, MF_BYPOSITION );

				column_fl_menu_showing = false;
			}
		}

		_TrackPopupMenu( g_hMenuSub_forward_list_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
	}
	else if ( hWnd == g_hWnd_ignore_list )
	{
		POINT cp;
		cp.x = p.x;
		cp.y = p.y;

		_ScreenToClient( hWnd, &cp );

		LVHITTESTINFO hti;
		hti.pt = cp;
		_SendMessageW( hWnd, LVM_SUBITEMHITTEST, 0, ( LPARAM )&hti );

		if ( hti.iSubItem > 0 )
		{
			int sel_count = _SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

			if ( sel_count > 0 )
			{
				// Get the virtual index from the column index.
				int vindex = GetVirtualIndexFromColumnIndex( hti.iSubItem, ignore_list_columns, NUM_COLUMNS4 );

				MENUITEMINFO mii;
				_memzero( &mii, sizeof( MENUITEMINFO ) );
				mii.cbSize = sizeof( MENUITEMINFO );
				mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
				mii.fType = MFT_STRING;

				MENUITEMINFO mii2 = mii;

				if ( hti.iItem != -1 )
				{
					mii.fState = MFS_ENABLED;

					if ( login_state != LOGGED_IN )
					{
						mii2.fState = MFS_DISABLED;
					}
					else
					{
						mii2.fState = MFS_ENABLED;
					}
				}
				else
				{
					mii.fState = MFS_DISABLED;
					mii2.fState = MFS_DISABLED;
				}

				switch ( vindex )
				{
					case 1:
					{
						mii2.wID = MENU_CALL_PHONE_COL41;
						mii2.dwTypeData = ST_Call_Phone_Number___;
						mii2.cch = 20;

						show_call_menu = true;

						mii.wID = MENU_COPY_SEL_COL41;

						if ( sel_count > 1 )
						{
							mii.dwTypeData = ST_Copy_Phone_Numbers;
							mii.cch = 18;
						}
						else
						{
							mii.dwTypeData = ST_Copy_Phone_Number;
							mii.cch = 17;
						}
					}
					break;

					case 2:
					{
						mii.wID = MENU_COPY_SEL_COL42;

						mii.dwTypeData = ST_Copy_Total_Calls;
						mii.cch = 16;
					}
					break;
				}

				if ( column_il_menu_showing == true )
				{
					_SetMenuItemInfoW( g_hMenuSub_ignore_list_context, ( il_phone_menu_showing == true ? 5 : 3 ), TRUE, &mii );
				}
				else
				{
					_InsertMenuItemW( g_hMenuSub_ignore_list_context, ( il_phone_menu_showing == true ? 5 : 3 ), TRUE, &mii );

					column_il_menu_showing = true;
				}

				if ( show_call_menu == true )
				{
					if ( il_phone_menu_showing == true )
					{
						_SetMenuItemInfoW( g_hMenuSub_ignore_list_context, 0, TRUE, &mii2 );
					}
					else
					{
						_InsertMenuItemW( g_hMenuSub_ignore_list_context, 0, TRUE, &mii2 );
						
						mii2.fType = MFT_SEPARATOR;
						_InsertMenuItemW( g_hMenuSub_ignore_list_context, 1, TRUE, &mii2 );

						il_phone_menu_showing = true;
					}
				}
				else
				{
					if ( il_phone_menu_showing == true )
					{
						_DeleteMenu( g_hMenuSub_ignore_list_context, 1, MF_BYPOSITION );	// Separator
						_DeleteMenu( g_hMenuSub_ignore_list_context, 0, MF_BYPOSITION );

						il_phone_menu_showing = false;
					}
				}
			}
		}
		else
		{
			if ( il_phone_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_ignore_list_context, 1, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_ignore_list_context, 0, MF_BYPOSITION );

				il_phone_menu_showing = false;
			}

			if ( column_il_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_ignore_list_context, 3, MF_BYPOSITION );

				column_il_menu_showing = false;
			}
		}

		_TrackPopupMenu( g_hMenuSub_ignore_list_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
	}
	else if ( hWnd == g_hWnd_tab )
	{
		TCHITTESTINFO tcht;
		tcht.flags = TCHT_ONITEM;
		tcht.pt.x = p.x;
		tcht.pt.y = p.y;
		_ScreenToClient( hWnd, &tcht.pt );

		int index = _SendMessageW( hWnd, TCM_HITTEST, 0, ( LPARAM )&tcht );

		if ( index >= 0 && index < total_tabs )
		{
			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_PARAM;
			_SendMessageW( hWnd, TCM_GETITEM, index, ( LPARAM )&ti );	// Insert a new tab at the end.

			if ( ( HWND )ti.lParam == g_hWnd_ignore_list )
			{
				context_tab_index = 0;
			}
			else if ( ( HWND )ti.lParam == g_hWnd_list )
			{
				context_tab_index = 1;
			}
			else if ( ( HWND )ti.lParam == g_hWnd_contact_list )
			{
				context_tab_index = 2;
			}
			else if ( ( HWND )ti.lParam == g_hWnd_forward_list )
			{
				context_tab_index = 3;
			}
			else
			{
				context_tab_index = -1;
			}

			_TrackPopupMenu( g_hMenuSub_tabs_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
		}
	}
	else	// Header control was clicked on.
	{
		HWND hWnd_parent = _GetParent( hWnd );

		if ( hWnd_parent == g_hWnd_list ||
			 hWnd_parent == g_hWnd_contact_list ||
			 hWnd_parent == g_hWnd_ignore_list ||
			 hWnd_parent == g_hWnd_forward_list )
		{
			// Show our columns context menu as a popup.
			_TrackPopupMenu( g_hMenuSub_header_context, 0, p.x, p.y, 0, g_hWnd_main, NULL );
		}
	}
}

// Enable/Disable the appropriate menu items depending on how many items exist as well as selected.
void UpdateMenus( unsigned char action )
{
	// Enable Menus based on the login state.
	if ( login_state == LOGGED_IN )
	{
		_EnableMenuItem( g_hMenu, MENU_ACCOUNT, MF_ENABLED );
		_EnableMenuItem( g_hMenu, MENU_DIAL_PHONE_NUM, MF_ENABLED );
		_EnableMenuItem( g_hMenu, MENU_EXPORT_CONTACTS, MF_ENABLED );
		_EnableMenuItem( g_hMenu, MENU_IMPORT_CONTACTS, MF_ENABLED );

		MENUITEMINFO mii;
		_memzero( &mii, sizeof( MENUITEMINFO ) );
		mii.cbSize = sizeof( MENUITEMINFO );
		mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		mii.fType = MFT_STRING;
		mii.dwTypeData = ST__Log_Off;
		mii.cch = 8;
		mii.wID = MENU_LOGIN;
		mii.fState = MFS_ENABLED;
		_SetMenuItemInfoW( g_hMenu, MENU_LOGIN, FALSE, &mii );
		_SetMenuItemInfoW( g_hMenuSub_tray, MENU_LOGIN, FALSE, &mii );
	}
	else	// Logging In, or Logged Out
	{
		_EnableMenuItem( g_hMenu, MENU_ACCOUNT, MF_DISABLED );
		_EnableMenuItem( g_hMenu, MENU_DIAL_PHONE_NUM, MF_DISABLED );
		_EnableMenuItem( g_hMenu, MENU_EXPORT_CONTACTS, MF_DISABLED );
		_EnableMenuItem( g_hMenu, MENU_IMPORT_CONTACTS, MF_DISABLED );

		if ( login_state == LOGGED_OUT )
		{
			MENUITEMINFO mii;
			_memzero( &mii, sizeof( MENUITEMINFO ) );
			mii.cbSize = sizeof( MENUITEMINFO );
			mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
			mii.fType = MFT_STRING;
			mii.dwTypeData = ST__Log_In___;
			mii.cch = 10;
			mii.wID = MENU_LOGIN;
			mii.fState = MFS_ENABLED;
			_SetMenuItemInfoW( g_hMenu, MENU_LOGIN, FALSE, &mii );
			_SetMenuItemInfoW( g_hMenuSub_tray, MENU_LOGIN, FALSE, &mii );
		}
		else	// Logging In
		{
			_EnableMenuItem( g_hMenu, MENU_LOGIN, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_tray, MENU_LOGIN, MF_DISABLED );
		}
	}

	// Enable Menus based on which tab is selected.
	int index = _SendMessageW( g_hWnd_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
	if ( index != -1 )
	{
		TCITEM tie;
		_memzero( &tie, sizeof( TCITEM ) );
		tie.mask = TCIF_PARAM; // Get the lparam value
		_SendMessageW( g_hWnd_tab, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information

		int item_count = _SendMessageW( ( HWND )tie.lParam, LVM_GETITEMCOUNT, 0, 0 );
		int sel_count = _SendMessageW( ( HWND )tie.lParam, LVM_GETSELECTEDCOUNT, 0, 0 );

		if ( ( HWND )tie.lParam == g_hWnd_list )
		{
			if ( l_incoming_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_list_context, 2, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );
				_DeleteMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );

				l_incoming_menu_showing = false;
			}

			if ( l_phone_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_list_context, 1, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_list_context, 0, MF_BYPOSITION );

				l_phone_menu_showing = false;
			}

			if ( column_l_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_list_context, 5, MF_BYPOSITION );

				column_l_menu_showing = false;
			}

			// Get the first selected item if it exists.
			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;

			MENUITEMINFO mii3;
			_memzero( &mii3, sizeof( MENUITEMINFO ) );
			mii3.cbSize = sizeof( MENUITEMINFO );
			mii3.fMask = MIIM_TYPE;
			mii3.fType = MFT_STRING;

			lvi.iItem = _SendMessageW( g_hWnd_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

			if ( lvi.iItem != -1 )
			{
				_SendMessageW( g_hWnd_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

				if ( ( ( displayinfo * )lvi.lParam )->forwarded == true )
				{
					mii3.dwTypeData = ST_Remove_from_Forward_List;
					mii3.cch = 24;
					_SetMenuItemInfoW( g_hMenuSub_list_context, 0, TRUE, &mii3 );
				}
				else
				{
					mii3.dwTypeData = ST_Add_to_Forward_List___;
					mii3.cch = 22;
					_SetMenuItemInfoW( g_hMenuSub_list_context, 0, TRUE, &mii3 );
				}

				if ( ( ( displayinfo * )lvi.lParam )->ignored == true )
				{
					mii3.dwTypeData = ST_Remove_from_Ignore_List;
					mii3.cch = 23;
					_SetMenuItemInfoW( g_hMenuSub_list_context, 1, TRUE, &mii3 );
				}
				else
				{
					mii3.dwTypeData = ST_Add_to_Ignore_List;
					mii3.cch = 18;
					_SetMenuItemInfoW( g_hMenuSub_list_context, 1, TRUE, &mii3 );
				}
			}
			else
			{
				mii3.dwTypeData = ST_Add_to_Forward_List___;
				mii3.cch = 22;
				_SetMenuItemInfoW( g_hMenuSub_list_context, 0, TRUE, &mii3 );

				mii3.dwTypeData = ST_Add_to_Ignore_List;
				mii3.cch = 18;
				_SetMenuItemInfoW( g_hMenuSub_list_context, 1, TRUE, &mii3 );
			}

			if ( action == UM_ENABLE )
			{
				if ( sel_count > 0 )
				{
					_EnableMenuItem( g_hMenuSub_list_context, MENU_FORWARD_LIST, MF_ENABLED );
					_EnableMenuItem( g_hMenuSub_list_context, MENU_IGNORE_LIST, MF_ENABLED );
					_EnableMenuItem( g_hMenuSub_list_context, MENU_COPY_SEL, MF_ENABLED );
					_EnableMenuItem( g_hMenuSub_list_context, MENU_REMOVE_SEL, MF_ENABLED );
				}
			}
			else if ( action == UM_DISABLE || sel_count <= 0 || action == UM_DISABLE_OVERRIDE )
			{
				_EnableMenuItem( g_hMenuSub_list_context, MENU_FORWARD_LIST, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_list_context, MENU_IGNORE_LIST, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_list_context, MENU_COPY_SEL, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_list_context, MENU_REMOVE_SEL, MF_DISABLED );
			}

			if ( sel_count == item_count || action == UM_DISABLE_OVERRIDE )
			{
				_EnableMenuItem( g_hMenuSub_list_context, MENU_SELECT_ALL, MF_DISABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_list_context, MENU_SELECT_ALL, MF_ENABLED );
			}
		}
		else if ( ( HWND )tie.lParam == g_hWnd_contact_list )
		{
			if ( cl_phone_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_contact_list_context, 1, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_contact_list_context, 0, MF_BYPOSITION );

				cl_phone_menu_showing = false;
			}

			if ( column_cl_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_contact_list_context, 4, MF_BYPOSITION );

				column_cl_menu_showing = false;
			}

			if ( action == UM_ENABLE )
			{
				if ( sel_count > 0 )
				{
					_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_EDIT_CONTACT, MF_ENABLED );
					_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_REMOVE_SEL, MF_ENABLED );
					_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_COPY_SEL, MF_ENABLED );
				}
			}
			else if ( action == UM_DISABLE || sel_count <= 0 || action == UM_DISABLE_OVERRIDE )
			{
				_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_EDIT_CONTACT, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_REMOVE_SEL, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_COPY_SEL, MF_DISABLED );
			}

			if ( sel_count == item_count || action == UM_DISABLE_OVERRIDE )
			{
				_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_SELECT_ALL, MF_DISABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_SELECT_ALL, MF_ENABLED );
			}

			if ( login_state == LOGGED_IN )
			{
				_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_ADD_CONTACT, MF_ENABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_contact_list_context, MENU_ADD_CONTACT, MF_DISABLED );
			}
		}
		else if ( ( HWND )tie.lParam == g_hWnd_ignore_list )
		{
			if ( il_phone_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_ignore_list_context, 1, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_ignore_list_context, 0, MF_BYPOSITION );

				il_phone_menu_showing = false;
			}

			if ( column_il_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_ignore_list_context, 3, MF_BYPOSITION );

				column_il_menu_showing = false;
			}

			if ( action == UM_ENABLE )
			{
				if ( sel_count > 0 )
				{
					_EnableMenuItem( g_hMenuSub_ignore_list_context, MENU_REMOVE_SEL, MF_ENABLED );
					_EnableMenuItem( g_hMenuSub_ignore_list_context, MENU_COPY_SEL, MF_ENABLED );
				}
			}
			else if ( action == UM_DISABLE || sel_count <= 0 || action == UM_DISABLE_OVERRIDE )
			{
				_EnableMenuItem( g_hMenuSub_ignore_list_context, MENU_REMOVE_SEL, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_ignore_list_context, MENU_COPY_SEL, MF_DISABLED );
			}

			if ( sel_count == item_count || action == UM_DISABLE_OVERRIDE )
			{
				_EnableMenuItem( g_hMenuSub_ignore_list_context, MENU_SELECT_ALL, MF_DISABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_ignore_list_context, MENU_SELECT_ALL, MF_ENABLED );
			}
		}
		else if ( ( HWND )tie.lParam == g_hWnd_forward_list )
		{
			if ( fl_phone_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_forward_list_context, 1, MF_BYPOSITION );	// Separator
				_DeleteMenu( g_hMenuSub_forward_list_context, 0, MF_BYPOSITION );

				fl_phone_menu_showing = false;
			}

			if ( column_fl_menu_showing == true )
			{
				_DeleteMenu( g_hMenuSub_forward_list_context, 4, MF_BYPOSITION );

				column_fl_menu_showing = false;
			}

			if ( action == UM_ENABLE )
			{
				if ( sel_count > 0 )
				{
					_EnableMenuItem( g_hMenuSub_forward_list_context, MENU_EDIT_FORWARD_LIST, MF_ENABLED );
					_EnableMenuItem( g_hMenuSub_forward_list_context, MENU_REMOVE_SEL, MF_ENABLED );
					_EnableMenuItem( g_hMenuSub_forward_list_context, MENU_COPY_SEL, MF_ENABLED );
				}
			}
			else if ( action == UM_DISABLE || sel_count <= 0 || action == UM_DISABLE_OVERRIDE )
			{
				_EnableMenuItem( g_hMenuSub_forward_list_context, MENU_EDIT_FORWARD_LIST, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_forward_list_context, MENU_REMOVE_SEL, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_forward_list_context, MENU_COPY_SEL, MF_DISABLED );
			}

			if ( sel_count == item_count || action == UM_DISABLE_OVERRIDE )
			{
				_EnableMenuItem( g_hMenuSub_forward_list_context, MENU_SELECT_ALL, MF_DISABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_forward_list_context, MENU_SELECT_ALL, MF_ENABLED );
			}
		}
	}
}