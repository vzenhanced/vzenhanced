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

#ifndef _LITE_OLE32_H
#define _LITE_OLE32_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <objbase.h>

//#define OLE32_USE_STATIC_LIB

#ifdef OLE32_USE_STATIC_LIB

	//__pragma( comment( lib, "ole32.lib" ) )

	#define _CoTaskMemFree	CoTaskMemFree

#else

	#define OLE32_STATE_SHUTDOWN	0
	#define OLE32_STATE_RUNNING		1

	typedef void ( WINAPI *pCoTaskMemFree )( LPVOID pv );

	extern pCoTaskMemFree	_CoTaskMemFree;



	extern unsigned char ole32_state;

	bool InitializeOle32();
	bool UnInitializeOle32();

#endif

#endif
