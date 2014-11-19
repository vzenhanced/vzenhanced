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

#ifndef _LITE_SHELL32_H
#define _LITE_SHELL32_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <shellapi.h>

//#define SHELL32_USE_STATIC_LIB

#ifdef SHELL32_USE_STATIC_LIB

	//__pragma( comment( lib, "shell32.lib" ) )
	//__pragma( comment( lib, "shlwapi.lib" ) )

	#include <shlwapi.h>

	#define _Shell_NotifyIconW	Shell_NotifyIconW
	#define _ShellExecuteW		ShellExecuteW

	#define _StrChrA			StrChrA
	#define _StrStrA			StrStrA
	#define _StrStrIA			StrStrIA

	#define _StrCmpNA			StrCmpNA
	#define _StrCmpNIA			StrCmpNIA

	#define _StrCmpNIW			StrCmpNIW

	//#define	_CommandLineToArgvW	CommandLineToArgvW;

#else

	#define SHELL32_STATE_SHUTDOWN		0
	#define SHELL32_STATE_RUNNING		1

	typedef BOOL ( WINAPI *pShell_NotifyIconW )( DWORD dwMessage, PNOTIFYICONDATA lpdata );
	typedef HINSTANCE ( WINAPI *pShellExecuteW )( HWND hwnd, LPCTSTR lpOperation, LPCTSTR lpFile, LPCTSTR lpParameters, LPCTSTR lpDirectory, INT nShowCmd );

	typedef PSTR ( WINAPI *pStrChrA )( PSTR pszStart, CHAR wMatch );
	typedef PSTR ( WINAPI *pStrStrA )( PSTR pszFirst, PCSTR pszSrch );
	typedef PSTR ( WINAPI *pStrStrIA )( PSTR pszFirst, PCSTR pszSrch );

	typedef int ( WINAPI *pStrCmpNA )( PCSTR psz1, PCSTR psz2, int nChar );
	typedef int ( WINAPI *pStrCmpNIA )( PCSTR psz1, PCSTR psz2, int nChar );

	typedef int ( WINAPI *pStrCmpNIW )( PCTSTR psz1, PCTSTR psz2, int nChar );

	//typedef LPWSTR * ( WINAPI *pCommandLineToArgvW )( LPCWSTR lpCmdLine, int *pNumArgs );

	extern pShell_NotifyIconW	_Shell_NotifyIconW;
	extern pShellExecuteW		_ShellExecuteW;

	extern pStrChrA				_StrChrA;
	extern pStrStrA				_StrStrA;
	extern pStrStrIA			_StrStrIA;

	extern pStrCmpNA			_StrCmpNA;
	extern pStrCmpNIA			_StrCmpNIA;

	extern pStrCmpNIW			_StrCmpNIW;

	//extern pCommandLineToArgvW	_CommandLineToArgvW;

	extern unsigned char shell32_state;

	bool InitializeShell32();
	bool UnInitializeShell32();

#endif

#endif
