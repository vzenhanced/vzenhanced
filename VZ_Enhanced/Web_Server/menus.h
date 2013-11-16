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

#ifndef _MENUS_H
#define _MENUS_H

#include "lite_comdlg32.h"

#define MENU_START_WEB_SERVER		1071
#define MENU_RESTART_WEB_SERVER		1072
#define MENU_STOP_WEB_SERVER		1073
#define MENU_CONNECTION_MANAGER		1074

#define MENU_CLOSE_CONNECTION		1075
#define	MENU_RESOLVE_ADDRESSES		1076
#define	MENU_SELECT_ALL				1077

extern "C" __declspec ( dllexport )
HMENU GetWebServerMenu();

void UpdateMenus();
void EnableDisableMenus( bool enable );
void CreateContextMenu();

extern HMENU hMenuSub;

extern HMENU g_hMenuSub_connection_manager;

#endif
