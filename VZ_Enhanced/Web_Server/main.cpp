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

#include "file_operations.h"

#include "connection.h"

#include "lite_ntdll.h"
#include "lite_shell32.h"
#include "lite_gdi32.h"
#include "lite_comdlg32.h"
#include "lite_ws2_32.h"
#include "lite_advapi32.h"
#include "lite_crypt32.h"
#include "lite_rpcrt4.h"
#include "lite_ole32.h"
#include "lite_user32.h"

#include "dllrbt.h"

#include "menus.h"

CRITICAL_SECTION cc_cs;	// Close connection critical section.

extern "C" __declspec( dllexport )
CRITICAL_SECTION *list_cs = NULL;

wchar_t base_directory[ MAX_PATH ];
unsigned int base_directory_length = 0;

bool cfg_enable_web_server = false;
unsigned char cfg_address_type = 0;	// 0 = Host name, 1 = IP address
unsigned long cfg_ip_address = 2130706433;	// 127.0.0.1
wchar_t *cfg_hostname = NULL;
unsigned short cfg_port = 80;

unsigned char cfg_ssl_version = 2;	// TLS 1.0

bool cfg_auto_start = false;
bool cfg_verify_origin = false;
bool cfg_enable_ssl = false;

unsigned char cfg_certificate_type;

wchar_t *cfg_certificate_pkcs_file_name = NULL;
wchar_t *cfg_certificate_pkcs_password = NULL;

wchar_t *cfg_certificate_cer_file_name = NULL;
wchar_t *cfg_certificate_key_file_name = NULL;

wchar_t *cfg_document_root_directory = NULL;

bool cfg_use_authentication = false;
wchar_t *cfg_authentication_username = NULL;
wchar_t *cfg_authentication_password = NULL;

unsigned long cfg_thread_count = 2;
unsigned long max_threads = 2;

char *encoded_authentication = NULL;
DWORD encoded_authentication_length = 0;


char *index_file_buf = NULL;
DWORD index_file_buf_length = 0;

// DO NOT FREE
extern "C" __declspec( dllexport )
dllrbt_tree *ignore_list = NULL;
extern "C" __declspec( dllexport )
dllrbt_tree *forward_list = NULL;
extern "C" __declspec( dllexport )
dllrbt_tree *contact_list = NULL;
extern "C" __declspec( dllexport )
dllrbt_tree *call_log = NULL;
//

extern "C" __declspec( dllexport )
HWND *g_hWnd_apply = NULL;

extern "C" __declspec( dllexport )
bool *state_changed = NULL;

extern "C" __declspec( dllexport )
HWND *g_hWnd_connection_manager = NULL;

extern "C" __declspec( dllexport )
pWebIgnoreIncomingCall *WebIgnoreIncomingCall = NULL;
extern "C" __declspec( dllexport )
pWebForwardIncomingCall *WebForwardIncomingCall = NULL;

HANDLE server_semaphore = NULL;

extern "C" __declspec ( dllexport )
void StartWebServer()
{
	if ( g_hCleanupEvent[ 0 ] == WSA_INVALID_EVENT )
	{
		CloseHandle( ( HANDLE )_CreateThread( NULL, 0, Server, ( LPVOID )NULL, 0, NULL ) );
	}
}

extern "C" __declspec ( dllexport )
void RestartWebServer()
{	
	if ( ws2_32_state == WS2_32_STATE_RUNNING )
	{
		g_bRestart = true;

		_WSASetEvent( g_hCleanupEvent[ 0 ] );
	}
}

extern "C" __declspec ( dllexport )
void StopWebServer()
{
	if ( ws2_32_state == WS2_32_STATE_RUNNING )
	{
		g_bEndServer = true;

		_WSASetEvent( g_hCleanupEvent[ 0 ] );
	}
}

extern "C" __declspec ( dllexport )
WNDPROC GetWebServerTabProc()
{
	return WebServerTabWndProc;
}

extern "C" __declspec ( dllexport )
WNDPROC GetConnectionManagerProc()
{
	return ConnectionManagerWndProc;
}

extern "C" __declspec ( dllexport )
void UpdateCallLog( displayinfo *di )
{
	// Make sure the Winsock library has been initialized and that the server is running.
	if ( ws2_32_state == WS2_32_STATE_SHUTDOWN || in_server_thread == false || di == NULL )
	{
		return;
	}

	CloseHandle( _CreateThread( NULL, 0, SendCallLogUpdate, ( LPVOID )di, 0, NULL ) );
}

extern "C" __declspec ( dllexport )
bool InitializeWebServerDLL()
{
	#ifndef USER32_USE_STATIC_LIB
		if ( InitializeUser32() == false ){ return false; }
	#endif
	#ifndef NTDLL_USE_STATIC_LIB
		if ( InitializeNTDLL() == false ){ return false; }
	#endif
	#ifndef GDI32_USE_STATIC_LIB
		if ( InitializeGDI32() == false ){ return false; }
	#endif
	#ifndef ADVAPI32_USE_STATIC_LIB
		if ( InitializeAdvApi32() == false ){ return false; }
	#endif
	#ifndef COMDLG32_USE_STATIC_LIB
		if ( InitializeComDlg32() == false ){ return false; }
	#endif
	#ifndef SHELL32_USE_STATIC_LIB
		if ( InitializeShell32() == false ){ return false; }
	#endif
	#ifndef CRYPT32_USE_STATIC_LIB
		if ( InitializeCrypt32() == false ){ return false; }
	#endif

	InitializeCriticalSection( &cc_cs );
	InitializeCriticalSection( &g_CriticalSection );

	// Get the default message system font.
	NONCLIENTMETRICS ncm;
	_memzero( &ncm, sizeof( NONCLIENTMETRICS ) );
	ncm.cbSize = sizeof( NONCLIENTMETRICS );
	_SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, sizeof( NONCLIENTMETRICS ), &ncm, 0 );

	/*// Force the font to be 8 points.
	HDC hdc_screen = _GetDC( NULL );
	int logical_height = -MulDiv( 8, _GetDeviceCaps( hdc_screen, LOGPIXELSY ), 72 );
	_ReleaseDC( NULL, hdc_screen );

	ncm.lfMessageFont.lfHeight = logical_height;*/

	// Set our global font to the LOGFONT value obtained from the system.
	hFont = _CreateFontIndirectW( &ncm.lfMessageFont );

	base_directory_length = GetCurrentDirectory( MAX_PATH, base_directory );	// Get the full path

	SYSTEM_INFO systemInfo;
	GetSystemInfo( &systemInfo );
	max_threads = systemInfo.dwNumberOfProcessors * 2;

	cfg_thread_count = max_threads;

	read_config();

	g_hCleanupEvent[ 0 ] = WSA_INVALID_EVENT;

	if ( cfg_enable_web_server == true && cfg_auto_start == true )
	{
		GetWebServerMenu();	// Menus need to be loaded first so that the Server thread can set them.
		StartWebServer();
	}

	return true;
}

extern "C" __declspec ( dllexport )
void UnInitializeWebServerDLL()
{
	if ( in_server_thread == true )
	{
		// This semaphore will be released when the thread gets killed.
		server_semaphore = CreateSemaphore( NULL, 0, 1, NULL );
	
		StopWebServer();

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( server_semaphore, 5000 );
		CloseHandle( server_semaphore );
		server_semaphore = NULL;
	}

	save_config();

	if ( cfg_hostname != NULL )
	{
		GlobalFree( cfg_hostname );
	}
	if ( cfg_certificate_pkcs_file_name != NULL )
	{
		GlobalFree( cfg_certificate_pkcs_file_name );
	}
	if ( cfg_certificate_pkcs_password != NULL )
	{
		GlobalFree( cfg_certificate_pkcs_password );
	}
	if ( cfg_certificate_cer_file_name != NULL )
	{
		GlobalFree( cfg_certificate_cer_file_name );
	}
	if ( cfg_certificate_key_file_name != NULL )
	{
		GlobalFree( cfg_certificate_key_file_name );
	}
	if ( cfg_document_root_directory != NULL )
	{
		GlobalFree( cfg_document_root_directory );
	}
	if ( cfg_authentication_username != NULL )
	{
		GlobalFree( cfg_authentication_username );
	}
	if ( cfg_authentication_password != NULL )
	{
		GlobalFree( cfg_authentication_password );
	}
	if ( encoded_authentication != NULL )
	{
		GlobalFree( encoded_authentication );
	}

	if ( index_file_buf != NULL )
	{
		GlobalFree( index_file_buf );
	}

	// Perform any necessary cleanup.
	_DeleteObject( hFont );

	DeleteCriticalSection( &g_CriticalSection );
	DeleteCriticalSection( &cc_cs );

	// Delay loaded.
	SSL_library_uninit();

	#ifndef WS2_32_USE_STATIC_LIB
		UnInitializeWS2_32();
	#endif
	#ifndef RPCRT4_USE_STATIC_LIB
		UnInitializeRpcRt4();
	#endif
	#ifndef OLE32_USE_STATIC_LIB
		UnInitializeOle32();
	#endif

	// Start up loaded DLLs
	#ifndef CRYPT32_USE_STATIC_LIB
		UnInitializeCrypt32();
	#endif
	#ifndef SHELL32_USE_STATIC_LIB
		UnInitializeShell32();
	#endif
	#ifndef COMDLG32_USE_STATIC_LIB
		UnInitializeComDlg32();
	#endif
	#ifndef ADVAPI32_USE_STATIC_LIB
		UnInitializeAdvApi32();
	#endif
	#ifndef GDI32_USE_STATIC_LIB
		UnInitializeGDI32();
	#endif
	#ifndef NTDLL_USE_STATIC_LIB
		UnInitializeNTDLL();
	#endif
	#ifndef USER32_USE_STATIC_LIB
		UnInitializeUser32();
	#endif
}

#ifndef NTDLL_USE_STATIC_LIB
BOOL WINAPI _DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved )
#else
BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved )
#endif
{
	// Perform actions based on the reason for calling.
	switch ( fdwReason ) 
	{ 
		case DLL_PROCESS_ATTACH:
		{
			// Initialize once for each new process.
			// Return FALSE to fail DLL load.

			// Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications.
			DisableThreadLibraryCalls( hinstDLL );
		}
		break;

		/*case DLL_THREAD_ATTACH:
		{
			// Do thread-specific initialization.
		}
		break;

		case DLL_THREAD_DETACH:
		{
			// Do thread-specific cleanup.
		}
		break;

		case DLL_PROCESS_DETACH:
		{
			
		}
		break;*/
	}

	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
