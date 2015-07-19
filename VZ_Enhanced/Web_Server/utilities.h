/*
	VZ Enhanced is a caller ID notifier that can forward and block phone calls.
	Copyright (C) 2013-2015 Eric Kutcher

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

#ifndef _UTILITIES_H
#define _UTILITIES_H

#include "lite_crypt32.h"

#define is_digit( c ) ( c - '0' + 0U <= 9U )

char *GlobalStrDupA( const char *_Str );
wchar_t *GlobalStrDupW( const wchar_t *_Str );

int dllrbt_compare( void *a, void *b );

char *CreateAuthentication( wchar_t *authentication_username, wchar_t *authentication_password, DWORD &authentication_length );

char *fields_tolower( char *decoded_buffer );

__int64 ntohll( __int64 i );
unsigned long long strtoull( char *str );

char *url_decode( char *str, unsigned int str_len, unsigned int *dec_len );
char *json_escape( char *str, unsigned int str_len, unsigned int *enc_len );

char *GetUTF8Domain( wchar_t *domain );
char *GetFileExtension( char *path );
char *GetFileName( char *path );
char *GetMIMEByFileName( char *filename );

#endif
