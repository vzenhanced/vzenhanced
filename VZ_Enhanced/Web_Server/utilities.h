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

#ifndef _UTILITIES_H
#define _UTILITIES_H

#include "lite_crypt32.h"

#define is_digit( c ) ( c - '0' + 0U <= 9U )

enum WS_OPCODE
{
	WS_CONTINUATION_FRAME	= 0x00,
	WS_TEXT_FRAME			= 0x01,
	WS_BINARY_FRAME			= 0x02,
	WS_RESERVED1			= 0x03,
	WS_RESERVED2			= 0x04,
	WS_RESERVED3			= 0x05,
	WS_RESERVED4			= 0x06,
	WS_RESERVED5			= 0x07,
	WS_CONNECTION_CLOSE		= 0x08,
	WS_PING					= 0x09,
	WS_PONG					= 0x0A,
	WS_RESERVED6			= 0x0B,
	WS_RESERVED7			= 0x0C,
	WS_RESERVED8			= 0x0D,
	WS_RESERVED9			= 0x0E,
	WS_RESERVED10			= 0x0F
};

struct PAYLOAD_INFO
{
	WS_OPCODE opcode;
	char *payload_value;
};

char *CreateAuthentication( wchar_t *authentication_username, wchar_t *authentication_password, DWORD &authentication_length );

char *fields_tolower( char *decoded_buffer );

__int64 ntohll( __int64 i );
char *DeconstructFrame( char *buf, DWORD *buf_length, WS_OPCODE &opcode, char **payload, unsigned __int64 &payload_length );
int ConstructFrame( char *buf, int buf_length, WS_OPCODE opcode, char *payload, unsigned __int64 payload_length );

int ConstructFrameHeader( char *buf, int buf_length, WS_OPCODE opcode, /*char *payload,*/ unsigned __int64 payload_length );

char *url_decode( char *str, unsigned int str_len, unsigned int *dec_len );
char *json_escape( char *str, unsigned int str_len, unsigned int *enc_len );

char *GetFileExtension( char *path );
char *GetFileName( char *path );
char *GetMIMEByFileName( char *filename );

#endif
