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

#define MENU_LOGIN					1001
#define MENU_EXIT					1002
#define MENU_OPTIONS				1003
#define MENU_ABOUT					1004
#define MENU_SELECT_ALL				1005
#define MENU_REMOVE_SEL				1006
#define MENU_RESTORE				1007
#define MENU_SELECT_COLUMNS			1008
#define MENU_ACCOUNT				1009

#define MENU_DIAL_PHONE_NUM			1010

#define MENU_ADD_CONTACT			1011
#define MENU_EDIT_CONTACT			1012

#define MENU_ADD_IGNORE_LIST		1013
#define MENU_IGNORE_LIST			1014

#define MENU_ADD_FORWARD_LIST		1015
#define MENU_EDIT_FORWARD_LIST		1016
#define MENU_FORWARD_LIST			1017

#define MENU_VIEW_CALL_LOG			1018
#define MENU_VIEW_CONTACT_LIST		1019
#define MENU_VIEW_IGNORE_LIST		1020
#define MENU_VIEW_FORWARD_LIST		1021

#define MENU_EXPORT_CONTACTS		1022
#define MENU_IMPORT_CONTACTS		1023

#define MENU_CLOSE_TAB				1024

#define MENU_COPY_SEL				1025

#define MENU_COPY_SEL_COL1			1026
#define MENU_COPY_SEL_COL2			1027
#define MENU_COPY_SEL_COL3			1028
#define MENU_COPY_SEL_COL4			1029
#define MENU_COPY_SEL_COL5			1030
#define MENU_COPY_SEL_COL6			1031
#define MENU_COPY_SEL_COL7			1032
#define MENU_COPY_SEL_COL8			1033

#define MENU_COPY_SEL_COL21			1034
#define MENU_COPY_SEL_COL22			1035
#define MENU_COPY_SEL_COL23			1036
#define MENU_COPY_SEL_COL24			1037
#define MENU_COPY_SEL_COL25			1038
#define MENU_COPY_SEL_COL26			1039
#define MENU_COPY_SEL_COL27			1040
#define MENU_COPY_SEL_COL28			1041
#define MENU_COPY_SEL_COL29			1042
#define MENU_COPY_SEL_COL210		1043
#define MENU_COPY_SEL_COL211		1044
#define MENU_COPY_SEL_COL212		1045
#define MENU_COPY_SEL_COL213		1046
#define MENU_COPY_SEL_COL214		1047
#define MENU_COPY_SEL_COL215		1048
#define MENU_COPY_SEL_COL216		1049

#define MENU_COPY_SEL_COL31			1050
#define MENU_COPY_SEL_COL32			1051
#define MENU_COPY_SEL_COL33			1052

#define MENU_COPY_SEL_COL41			1053
#define MENU_COPY_SEL_COL42			1054



// A phone number column was right clicked.
#define MENU_CALL_PHONE_COL13		1055
#define MENU_CALL_PHONE_COL15		1056
#define MENU_CALL_PHONE_COL17		1057

#define MENU_CALL_PHONE_COL21		1058
#define MENU_CALL_PHONE_COL25		1059
#define MENU_CALL_PHONE_COL27		1060
#define MENU_CALL_PHONE_COL211		1061
#define MENU_CALL_PHONE_COL212		1062
#define MENU_CALL_PHONE_COL216		1063

#define MENU_CALL_PHONE_COL31		1064
#define MENU_CALL_PHONE_COL32		1065

#define MENU_CALL_PHONE_COL41		1066

#define MENU_INCOMING_IGNORE		1067
#define MENU_INCOMING_FORWARD		1068

#define MENU_OPEN_WEB_PAGE			1069
#define MENU_OPEN_EMAIL_ADDRESS		1070

#define MENU_START_WEB_SERVER		1071
#define MENU_RESTART_WEB_SERVER		1072
#define MENU_STOP_WEB_SERVER		1073
#define MENU_CONNECTION_MANAGER		1074


#define UM_DISABLE			0
#define UM_ENABLE			1
#define UM_DISABLE_OVERRIDE	2

void CreateMenus();
void DestroyMenus();
void UpdateMenus( unsigned char action );
void ChangeMenuByHWND( HWND hWnd );

void HandleRightClick( HWND hWnd );

extern HMENU g_hMenu;					// Handle to our menu bar.
extern HMENU g_hMenuSub_tray;			// Handle to our tray menu.

extern char context_tab_index;

#endif
