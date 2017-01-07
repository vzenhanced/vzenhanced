/*
	VZ Enhanced is a caller ID notifier that can forward and block phone calls.
	Copyright (C) 2013-2017 Eric Kutcher

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

#ifndef _GLOBALS_H
#define _GLOBALS_H

// Pretty window.
#pragma comment( linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"" )

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>

#include "shared_objects.h"
#include "dllrbt.h"

#include "resource.h"

#define FILETIME_TICKS_PER_SECOND	10000000

#define _wcsicmp_s( a, b ) ( ( a == NULL && b == NULL ) ? 0 : ( a != NULL && b == NULL ) ? 1 : ( a == NULL && b != NULL ) ? -1 : lstrcmpiW( a, b ) )
#define _stricmp_s( a, b ) ( ( a == NULL && b == NULL ) ? 0 : ( a != NULL && b == NULL ) ? 1 : ( a == NULL && b != NULL ) ? -1 : lstrcmpiA( a, b ) )

#define SAFESTRA( s ) ( s != NULL ? s : "" )
#define SAFESTR2A( s1, s2 ) ( s1 != NULL ? s1 : ( s2 != NULL ? s2 : "" ) )

#define SAFESTRW( s ) ( s != NULL ? s : L"" )
#define SAFESTR2W( s1, s2 ) ( s1 != NULL ? s1 : ( s2 != NULL ? s2 : L"" ) )

typedef bool ( WINAPI *pWebIgnoreIncomingCall )( displayinfo *di );
typedef bool ( WINAPI *pWebForwardIncomingCall )( displayinfo *di, char *forward_to );

struct sortinfo
{
	HWND hWnd;
	int column;
	unsigned char direction;
};

LRESULT CALLBACK WebServerTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK ConnectionManagerWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

extern HWND g_hWnd_connections;

extern HFONT hFont;

extern int row_height;

extern "C" __declspec ( dllexport )
extern CRITICAL_SECTION *list_cs;

extern "C" __declspec( dllexport )
extern HWND *g_hWnd_apply;

extern "C" __declspec( dllexport )
extern bool *state_changed;

extern "C" __declspec( dllexport )
extern HWND *g_hWnd_connection_manager;

extern bool skip_connections_draw;

extern "C" __declspec( dllexport )
extern dllrbt_tree *contact_list;
extern "C" __declspec( dllexport )
extern dllrbt_tree *ignore_list;
extern "C" __declspec( dllexport )
extern dllrbt_tree *forward_list;
extern "C" __declspec( dllexport )
extern dllrbt_tree *call_log;
extern "C" __declspec( dllexport )
extern dllrbt_tree *ignore_cid_list;
extern "C" __declspec( dllexport )
extern dllrbt_tree *forward_cid_list;

extern "C" __declspec( dllexport )
extern pWebIgnoreIncomingCall *WebIgnoreIncomingCall;
extern "C" __declspec( dllexport )
extern pWebForwardIncomingCall *WebForwardIncomingCall;

extern wchar_t *base_directory;
extern unsigned int base_directory_length;

extern bool cfg_enable_web_server;
extern unsigned char cfg_address_type;
extern unsigned long cfg_ip_address;
extern wchar_t *cfg_hostname;
extern unsigned short cfg_port;

extern wchar_t *g_punycode_hostname;

extern unsigned char cfg_ssl_version;

extern bool cfg_auto_start;
extern bool cfg_verify_origin;
extern bool cfg_allow_keep_alive_requests;
extern bool cfg_enable_ssl;

extern unsigned char cfg_certificate_type;

extern wchar_t *cfg_certificate_pkcs_file_name;
extern wchar_t *cfg_certificate_pkcs_password;

extern wchar_t *cfg_certificate_cer_file_name;
extern wchar_t *cfg_certificate_key_file_name;

extern wchar_t *cfg_document_root_directory;

extern bool cfg_use_authentication;
extern wchar_t *cfg_authentication_username;
extern wchar_t *cfg_authentication_password;

extern unsigned long cfg_thread_count;
extern unsigned long long cfg_resource_cache_size;

extern char *encoded_authentication;
extern DWORD encoded_authentication_length;

extern int g_document_root_directory_length;

extern bool use_ssl;	// Current connection state.

extern unsigned long max_threads;

extern HANDLE server_semaphore;
extern bool in_server_thread;

extern "C" __declspec ( dllexport )
void StartWebServer();

extern "C" __declspec ( dllexport )
void RestartWebServer();

extern "C" __declspec ( dllexport )
void StopWebServer();

extern "C" __declspec ( dllexport )
WNDPROC GetWebServerTabProc();

extern "C" __declspec ( dllexport )
WNDPROC GetConnectionManagerProc();

extern "C" __declspec ( dllexport )
void UpdateCallLog( displayinfo *di );

extern "C" __declspec ( dllexport )
bool InitializeWebServerDLL( wchar_t *directory );

extern "C" __declspec ( dllexport )
void UnInitializeWebServerDLL();

#endif
