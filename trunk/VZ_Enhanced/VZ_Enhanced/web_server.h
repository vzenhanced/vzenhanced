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

#ifndef _WEB_SERVER_H
#define _WEB_SERVER_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "globals.h"
#include "dllrbt.h"

#define WEB_SERVER_STATE_SHUTDOWN	0
#define WEB_SERVER_STATE_RUNNING	1

typedef void ( WINAPI *pSaveWebServerSettings )();

typedef HMENU ( WINAPI *pGetWebServerMenu )();

typedef void ( WINAPI *pStartWebServer )();
typedef void ( WINAPI *pRestartWebServer )();
typedef void ( WINAPI *pStopWebServer )();

typedef WNDPROC ( WINAPI *pGetWebServerTabProc )();
typedef WNDPROC ( WINAPI *pGetConnectionManagerProc )();

typedef bool ( WINAPI *pInitializeWebServerDLL )();
typedef void ( WINAPI *pUnInitializeWebServerDLL )();

typedef void ( WINAPI *pUpdateCallLog )( displayinfo *di );

extern pSaveWebServerSettings		SaveWebServerSettings;

extern pGetWebServerMenu			GetWebServerMenu;

extern pStartWebServer				StartWebServer;
extern pRestartWebServer			RestartWebServer;
extern pStopWebServer				StopWebServer;

extern pGetWebServerTabProc			GetWebServerTabProc;
extern pGetConnectionManagerProc	GetConnectionManagerProc;

extern pInitializeWebServerDLL		InitializeWebServerDLL;
extern pUnInitializeWebServerDLL	UnInitializeWebServerDLL;

extern pUpdateCallLog				UpdateCallLog;

extern unsigned char web_server_state;

bool InitializeWebServer();
bool UnInitializeWebServer();

bool WebIgnoreIncomingCall( displayinfo *di );
bool WebForwardIncomingCall( displayinfo *di, char *forward_to );

typedef bool ( WINAPI *pWebIgnoreIncomingCall )( displayinfo *di );
typedef bool ( WINAPI *pWebForwardIncomingCall )( displayinfo *di, char *forward_to );

void SetCriticalSection( CRITICAL_SECTION *cs );

void SetWebIgnoreIncomingCall( pWebIgnoreIncomingCall *fpWebIgnoreIncomingCall );
void SetWebForwardIncomingCall( pWebForwardIncomingCall *fpWebForwardIncomingCall );

void SetWindowStateInfo( HWND *hWnd, bool *state );

void SetConnectionManagerHWND( HWND *hWnd );

void SetIgnoreList( dllrbt_tree *ignore_list );
void SetForwardList( dllrbt_tree *forward_list );
void SetContactList( dllrbt_tree *contact_list );
void SetCallLog( dllrbt_tree *call_log );
void SetIgnoreCIDList( dllrbt_tree *ignore_cid_list );
void SetForwardCIDList( dllrbt_tree *forward_cid_list );

#endif
