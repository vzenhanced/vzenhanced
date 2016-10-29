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

#include "lite_dlls.h"
#include "web_server.h"
#include "connection.h"
#include "utilities.h"

pSaveWebServerSettings		SaveWebServerSettings;

pGetWebServerMenu			GetWebServerMenu;

pStartWebServer				StartWebServer;
pRestartWebServer			RestartWebServer;
pStopWebServer				StopWebServer;

pGetWebServerTabProc		GetWebServerTabProc;
pGetConnectionManagerProc	GetConnectionManagerProc;

pInitializeWebServerDLL		InitializeWebServerDLL;
pUnInitializeWebServerDLL	UnInitializeWebServerDLL;

pUpdateCallLog				UpdateCallLog;

HMODULE hModule_web_server = NULL;

unsigned char web_server_state = 0;	// 0 = Not running, 1 = running.

bool InitializeWebServer()
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		return true;
	}

	hModule_web_server = LoadLibraryDEMW( L"Web_Server.dll" );

	if ( hModule_web_server == NULL )
	{
		return false;
	}

	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_web_server, ( void ** )&SaveWebServerSettings, "SaveWebServerSettings" ) )

	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_web_server, ( void ** )&GetWebServerMenu, "GetWebServerMenu" ) )

	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_web_server, ( void ** )&StartWebServer, "StartWebServer" ) )
	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_web_server, ( void ** )&RestartWebServer, "RestartWebServer" ) )
	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_web_server, ( void ** )&StopWebServer, "StopWebServer" ) )

	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_web_server, ( void ** )&GetWebServerTabProc, "GetWebServerTabProc" ) )
	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_web_server, ( void ** )&GetConnectionManagerProc, "GetConnectionManagerProc" ) )

	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_web_server, ( void ** )&InitializeWebServerDLL, "InitializeWebServerDLL" ) )
	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_web_server, ( void ** )&UnInitializeWebServerDLL, "UnInitializeWebServerDLL" ) )

	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_web_server, ( void ** )&UpdateCallLog, "UpdateCallLog" ) )

	web_server_state = WEB_SERVER_STATE_RUNNING;

	return true;
}

void SetCriticalSection( CRITICAL_SECTION *cs )
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		CRITICAL_SECTION **g_cs = ( CRITICAL_SECTION ** )GetProcAddress( hModule_web_server, "list_cs" );
		if ( g_cs != NULL )
		{
			*g_cs = cs;
		}
	}
}

void SetWebIgnoreIncomingCall( pWebIgnoreIncomingCall *fpWebIgnoreIncomingCall )
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		pWebIgnoreIncomingCall **fp = ( pWebIgnoreIncomingCall ** )GetProcAddress( hModule_web_server, "WebIgnoreIncomingCall" );
		if ( fp != NULL )
		{
			*fp = fpWebIgnoreIncomingCall;
		}
	}
}

void SetWebForwardIncomingCall( pWebForwardIncomingCall *fpWebForwardIncomingCall )
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		pWebForwardIncomingCall **fp = ( pWebForwardIncomingCall ** )GetProcAddress( hModule_web_server, "WebForwardIncomingCall" );
		if ( fp != NULL )
		{
			*fp = fpWebForwardIncomingCall;
		}
	}
}

void SetWindowStateInfo( HWND *hWnd, bool *state )
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		HWND **phWnd = ( HWND ** )GetProcAddress( hModule_web_server, "g_hWnd_apply" );
		if ( phWnd != NULL )
		{
			*phWnd = hWnd;
		}

		bool **pstate_changed = ( bool ** )GetProcAddress( hModule_web_server, "state_changed" );
		if ( pstate_changed != NULL )
		{
			*pstate_changed = state;
		}
	}
}

void SetConnectionManagerHWND( HWND *hWnd )
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		HWND **phWnd = ( HWND ** )GetProcAddress( hModule_web_server, "g_hWnd_connection_manager" );
		if ( phWnd != NULL )
		{
			*phWnd = hWnd;
		}
	}
}

void SetIgnoreList( dllrbt_tree *ignore_list )
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		dllrbt_tree **dll = ( dllrbt_tree ** )GetProcAddress( hModule_web_server, "ignore_list" );
		if ( dll != NULL )
		{
			EnterCriticalSection( &pe_cs );
			*dll = ignore_list;
			LeaveCriticalSection( &pe_cs );
		}
	}
}

void SetForwardList( dllrbt_tree *forward_list )
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		dllrbt_tree **dll = ( dllrbt_tree ** )GetProcAddress( hModule_web_server, "forward_list" );
		if ( dll != NULL )
		{
			EnterCriticalSection( &pe_cs );
			*dll = forward_list;
			LeaveCriticalSection( &pe_cs );
		}
	}
}

void SetContactList( dllrbt_tree *contact_list )
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		dllrbt_tree **dll = ( dllrbt_tree ** )GetProcAddress( hModule_web_server, "contact_list" );
		if ( dll != NULL )
		{
			EnterCriticalSection( &pe_cs );
			*dll = contact_list;
			LeaveCriticalSection( &pe_cs );
		}
	}
}

void SetCallLog( dllrbt_tree *call_log )
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		dllrbt_tree **dll = ( dllrbt_tree ** )GetProcAddress( hModule_web_server, "call_log" );
		if ( dll != NULL )
		{
			EnterCriticalSection( &pe_cs );
			*dll = call_log;
			LeaveCriticalSection( &pe_cs );
		}
	}
}

void SetIgnoreCIDList( dllrbt_tree *ignore_cid_list )
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		dllrbt_tree **dll = ( dllrbt_tree ** )GetProcAddress( hModule_web_server, "ignore_cid_list" );
		if ( dll != NULL )
		{
			EnterCriticalSection( &pe_cs );
			*dll = ignore_cid_list;
			LeaveCriticalSection( &pe_cs );
		}
	}
}

void SetForwardCIDList( dllrbt_tree *forward_cid_list )
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		dllrbt_tree **dll = ( dllrbt_tree ** )GetProcAddress( hModule_web_server, "forward_cid_list" );
		if ( dll != NULL )
		{
			EnterCriticalSection( &pe_cs );
			*dll = forward_cid_list;
			LeaveCriticalSection( &pe_cs );
		}
	}
}

bool UnInitializeWebServer()
{
	if ( web_server_state != WEB_SERVER_STATE_SHUTDOWN )
	{
		UnInitializeWebServerDLL();

		web_server_state = WEB_SERVER_STATE_SHUTDOWN;

		return ( FreeLibrary( hModule_web_server ) == FALSE ? false : true );
	}

	return true;
}

bool WebIgnoreIncomingCall( displayinfo *di )
{
	if ( di == NULL || main_con.state != LOGGED_IN )
	{
		return false;
	}

	di->process_incoming = false;
	di->ci.ignored = true;

	_InvalidateRect( g_hWnd_call_log, NULL, TRUE );

	CloseHandle( ( HANDLE )_CreateThread( NULL, 0, IgnoreIncomingCall, ( void * )&( di->ci ), 0, NULL ) );

	return true;
}

bool WebForwardIncomingCall( displayinfo *di, char *forward_to )
{
	if ( di == NULL || forward_to == NULL || main_con.state != LOGGED_IN )
	{
		return false;
	}

	di->process_incoming = false;
	di->ci.forward_to = forward_to;
	di->ci.forwarded = true;

	// Forward to phone number
	di->forward_to = FormatPhoneNumber( di->ci.forward_to );

	_InvalidateRect( g_hWnd_call_log, NULL, TRUE );

	CloseHandle( ( HANDLE )_CreateThread( NULL, 0, ForwardIncomingCall, ( void * )&( di->ci ), 0, NULL ) );

	return true;
}
