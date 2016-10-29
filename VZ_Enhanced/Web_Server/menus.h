/*
	VZ Enhanced is a caller ID notifier that can forward and block phone calls.
	Copyright (C) 2013-2016 Eric Kutcher

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

#ifndef _MENUS_H
#define _MENUS_H

#include "lite_comdlg32.h"

#define MENU_START_WEB_SERVER		10001
#define MENU_RESTART_WEB_SERVER		10002
#define MENU_STOP_WEB_SERVER		10003
#define MENU_CONNECTION_MANAGER		10004

#define MENU_CLOSE_CONNECTION		10005
#define	MENU_RESOLVE_ADDRESSES		10006
#define	MENU_SELECT_ALL				10007

extern "C" __declspec ( dllexport )
HMENU GetWebServerMenu();

void UpdateMenus();
void EnableDisableMenus( bool enable );
void CreateContextMenu();

extern HMENU hMenuSub;

extern HMENU g_hMenuSub_connection_manager;

#endif
