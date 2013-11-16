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

#include "menus.h"

#include "lite_ntdll.h"
#include "lite_comdlg32.h"
#include "lite_user32.h"

#include "string_tables.h"

HMENU hMenuSub = NULL;

HMENU g_hMenuSub_connection_manager = NULL;

void UpdateMenus()
{
	int item_count = _SendMessageW( g_hWnd_connections, LVM_GETITEMCOUNT, 0, 0 );
	int sel_count = _SendMessageW( g_hWnd_connections, LVM_GETSELECTEDCOUNT, 0, 0 );

	_EnableMenuItem( g_hMenuSub_connection_manager, MENU_SELECT_ALL, ( sel_count != item_count ? MF_ENABLED : MF_DISABLED ) );
	_EnableMenuItem( g_hMenuSub_connection_manager, MENU_CLOSE_CONNECTION, ( sel_count > 0 ? MF_ENABLED : MF_DISABLED ) );
}

void EnableDisableMenus( bool enable )
{
	_EnableMenuItem( hMenuSub, MENU_START_WEB_SERVER, ( enable == true ? MF_DISABLED : ( cfg_enable_web_server == true ? MF_ENABLED : MF_DISABLED ) ) );
	_EnableMenuItem( hMenuSub, MENU_RESTART_WEB_SERVER, ( enable == true ? MF_ENABLED : MF_DISABLED ) );
	_EnableMenuItem( hMenuSub, MENU_STOP_WEB_SERVER, ( enable == true ? MF_ENABLED : MF_DISABLED ) );
	_EnableMenuItem( hMenuSub, MENU_CONNECTION_MANAGER, ( enable == true ? MF_ENABLED : MF_DISABLED ) );
}

extern "C" __declspec ( dllexport )
HMENU GetWebServerMenu()
{
	if ( hMenuSub != NULL )
	{
		return hMenuSub;
	}

	hMenuSub = _CreatePopupMenu();

	MENUITEMINFO mii;
	_memzero( &mii, sizeof( MENUITEMINFO ) );
	mii.cbSize = sizeof( MENUITEMINFO );
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Start_Web_Server;
	mii.cch = 16;
	mii.wID = MENU_START_WEB_SERVER;
	mii.fState = ( cfg_enable_web_server == true ? MF_ENABLED : MF_DISABLED );
	_InsertMenuItemW( hMenuSub, 0, TRUE, &mii );

	mii.dwTypeData = ST_Restart_Web_Server;
	mii.cch = 18;
	mii.wID = MENU_RESTART_WEB_SERVER;
	mii.fState = MF_DISABLED;
	_InsertMenuItemW( hMenuSub, 1, TRUE, &mii );

	mii.dwTypeData = ST_Stop_Web_Server;
	mii.cch = 15;
	mii.wID = MENU_STOP_WEB_SERVER;
	_InsertMenuItemW( hMenuSub, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Connection_Manager;
	mii.cch = 18;
	mii.wID = MENU_CONNECTION_MANAGER;
	_InsertMenuItemW( hMenuSub, 4, TRUE, &mii );

	return hMenuSub;
}

void CreateContextMenu()
{
	if ( g_hMenuSub_connection_manager != NULL )
	{
		return;
	}

	g_hMenuSub_connection_manager = _CreatePopupMenu();

	MENUITEMINFO mii;
	_memzero( &mii, sizeof( MENUITEMINFO ) );
	mii.cbSize = sizeof( MENUITEMINFO );
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Close_Connection;
	mii.cch = 16;
	mii.wID = MENU_CLOSE_CONNECTION;
	_InsertMenuItemW( g_hMenuSub_connection_manager, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_connection_manager, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Resolve_Addresses;
	mii.cch = 17;
	mii.wID = MENU_RESOLVE_ADDRESSES;
	_InsertMenuItemW( g_hMenuSub_connection_manager, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_connection_manager, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Select_All;
	mii.cch = 10;
	mii.wID = MENU_SELECT_ALL;
	_InsertMenuItemW( g_hMenuSub_connection_manager, 4, TRUE, &mii );
}
