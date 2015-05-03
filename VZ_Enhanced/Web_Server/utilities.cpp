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

#include "utilities.h"

#include "lite_ntdll.h"
#include "lite_ws2_32.h"

#include "lite_shell32.h"
#include "lite_user32.h"

char *CreateAuthentication( wchar_t *authentication_username, wchar_t *authentication_password, DWORD &authentication_length )
{
	int concatenated_offset = 0;

	char *username = NULL;
	int username_length = 0;

	char *password = NULL;
	int password_length = 0;

	if ( authentication_username != NULL )
	{
		username_length = WideCharToMultiByte( CP_UTF8, 0, authentication_username, -1, NULL, 0, NULL, NULL );
		username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * username_length ); // Size includes the null character.
		username_length = WideCharToMultiByte( CP_UTF8, 0, authentication_username, -1, username, username_length, NULL, NULL ) - 1;
	}

	if ( authentication_password != NULL )
	{
		password_length = WideCharToMultiByte( CP_UTF8, 0, authentication_password, -1, NULL, 0, NULL, NULL );
		password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * password_length ); // Size includes the null character.
		password_length = WideCharToMultiByte( CP_UTF8, 0, authentication_password, -1, password, password_length, NULL, NULL ) - 1;
	}

	// username:password
	int concatenated_authentication_length = username_length + 1 + password_length;
	char *concatenated_authentication = ( char * )GlobalAlloc( GMEM_FIXED, sizeof ( char ) * ( concatenated_authentication_length + 1 ) );

	if ( username != NULL )
	{
		_memcpy_s( concatenated_authentication, concatenated_authentication_length + 1, username, username_length );
		concatenated_offset += username_length;

		GlobalFree( username );
	}

	concatenated_authentication[ concatenated_offset++ ] = ':';

	if ( password != NULL )
	{
		_memcpy_s( concatenated_authentication + concatenated_offset, ( concatenated_authentication_length + 1 ) - concatenated_offset, password, password_length );
		concatenated_offset += password_length;

		GlobalFree( password );
	}

	concatenated_authentication[ concatenated_offset ] = 0;	// Sanity.

	_CryptBinaryToStringA( ( BYTE * )concatenated_authentication, concatenated_authentication_length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &authentication_length );

	char *authentication = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * authentication_length );
	_CryptBinaryToStringA( ( BYTE * )concatenated_authentication, concatenated_authentication_length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, ( LPSTR )authentication, &authentication_length );
	authentication[ authentication_length ] = 0; // Sanity.

	GlobalFree( concatenated_authentication );

	return authentication;
}

char *fields_tolower( char *decoded_buffer )
{
	if ( decoded_buffer == NULL )
	{
		return NULL;
	}

	// First find the end of the header.
	char *end_of_header = _StrStrA( decoded_buffer, "\r\n\r\n" );
	if ( end_of_header == NULL )
	{
		return end_of_header;
	}

	char *str_pos_end = decoded_buffer;
	char *str_pos_start = decoded_buffer;

	while ( str_pos_start < end_of_header )
	{
		// The first line won't have a field.
		str_pos_start = _StrStrA( str_pos_end, "\r\n" );
		if ( str_pos_start == NULL || str_pos_start >= end_of_header )
		{
			break;
		}
		str_pos_start += 2;

		str_pos_end = _StrChrA( str_pos_start, ':' );
		if ( str_pos_end == NULL || str_pos_end >= end_of_header )
		{
			break;
		}

		for ( int i = 0; i < str_pos_end - str_pos_start; ++i )
		{
			str_pos_start[ i ] = ( char )_CharLowerA( ( LPSTR )str_pos_start[ i ] );
		}
	}

	return end_of_header;
}

__int64 ntohll( __int64 i )
{
	unsigned int t = 0;
	unsigned int b = 0;

	unsigned int v[ 2 ];

	_memcpy_s( v, sizeof( unsigned int ) * 2, &i, sizeof( unsigned __int64 ) );

	t = _ntohl( v[ 0 ] );
	v[ 0 ] = _ntohl( v[ 1 ] );
	v[ 1 ] = t;

	_memcpy_s( &i, sizeof( __int64 ), ( void * )v, sizeof( unsigned int ) * 2 );
	
	return i;

	//return ( ( __int64 )_ntohl( i & 0xFFFFFFFFU ) << 32 ) | _ntohl( ( __int64 )( i >> 32 ) );
}

char *DeconstructFrame( char *buf, DWORD *buf_length, WS_OPCODE &opcode, char **payload, unsigned __int64 &payload_length )
{
	if ( buf == NULL || *buf_length < 2 )
	{
		return NULL;
	}

	int buffer_offset = 0;
	/*unsigned short websocket_header = 0;
	_memcpy_s( &websocket_header, sizeof( unsigned short ), lpIOContext->wsabuf.buf, sizeof( unsigned short ) );
	buffer_offset += sizeof( unsigned short );

	websocket_header = _ntohs( websocket_header );
	
	unsigned short ws_fin =			websocket_header >> 15 & 0x01;
	unsigned short ws_rsv1 =		websocket_header >> 14 & 0x01;
	unsigned short ws_rsv2 =		websocket_header >> 13 & 0x01;
	unsigned short ws_rsv3 =		websocket_header >> 12 & 0x01;
	unsigned short ws_opcode =		websocket_header >> 8 & 0x0F;
	unsigned short ws_mask =		websocket_header >> 7 & 0x01;
	unsigned short ws_payload_len =	websocket_header >> 0 & 0x7F;
	*/

	unsigned char ws_fin =			buf[ 0 ] >> 7 & 0x01;
	unsigned char ws_rsv1 =			buf[ 0 ] >> 6 & 0x01;
	unsigned char ws_rsv2 =			buf[ 0 ] >> 5 & 0x01;
	unsigned char ws_rsv3 =			buf[ 0 ] >> 4 & 0x01;
	unsigned char ws_opcode =		buf[ 0 ] & 0x0F;

	unsigned char ws_mask =			buf[ 1 ] >> 7 & 0x01;
	unsigned char ws_payload_len =	buf[ 1 ] & 0x7F;

	buffer_offset += ( sizeof( unsigned char ) * 2 );

	opcode = ( WS_OPCODE )ws_opcode;

	if ( ws_payload_len == 126 )
	{
		unsigned short extended_payload1 = 0;
		_memcpy_s( &extended_payload1, sizeof( unsigned short ), buf + buffer_offset, sizeof( unsigned short ) );
		buffer_offset += sizeof( unsigned short );

		payload_length = ( unsigned __int64 )_ntohs( extended_payload1 );
	}
	else if ( ws_payload_len >= 127 )
	{
		_memcpy_s( &payload_length, sizeof( unsigned __int64 ), buf + buffer_offset, sizeof( unsigned __int64 ) );
		buffer_offset += sizeof( unsigned __int64 );

		payload_length = ntohll( payload_length );
	}
	else
	{
		payload_length = ( unsigned __int64 )ws_payload_len;
	}

	unsigned char masking_key[ 4 ];
	_memzero( &masking_key, sizeof( masking_key ) );
	if ( ws_mask == 1 )
	{
		_memcpy_s( masking_key, sizeof( masking_key ), buf + buffer_offset, sizeof( masking_key ) );
		buffer_offset += sizeof( masking_key );
	}

	*payload = buf + buffer_offset;

	for ( unsigned int i = 0; i < payload_length; ++i )
	//for ( unsigned __int64 i = 0; i < payload_length; ++i )
	{
		if ( i + buffer_offset >= ( unsigned int )*buf_length )
		{
			break;
		}

		buf[ buffer_offset + i ] ^= masking_key[ i % 4 ];
	}

	if ( buffer_offset + payload_length < *buf_length )
	{
		*buf_length -= ( int )( buffer_offset + payload_length );
		return buf + ( buffer_offset + payload_length );
	}
	else
	{
		*buf_length = 0;
		return NULL;
	}
}

int ConstructFrameHeader( char *buf, int buf_length, WS_OPCODE opcode, /*char *payload,*/ unsigned __int64 payload_length )
{
	if ( buf == NULL || buf_length < 2 )
	{
		return 0;
	}

	unsigned char fin =		0x01;	// Final frame. We're not going to break up our frames.
	unsigned char rsv1 =	0x00;	// Reserved
	unsigned char rsv2 =	0x00;	// Reserved
	unsigned char rsv3 =	0x00;	// Reserved
	unsigned char mask =	0x00;	// Server doesn't mask payload. Client has this bit set.
	unsigned char len =		0x00;

	// Adjust the length accordingly.
	unsigned char len_type = 0;
	if ( payload_length > 65535 )
	{
		len = 127;
		len_type = 2;
	}
	else if ( payload_length > 125 )
	{
		len = 126;
		len_type = 1;
	}
	else
	{
		len = ( unsigned char )payload_length;
		len_type = 0;
	}

	unsigned short frame_header = 0;

	// Big endian (network byte order)
	// Byte 2.
	frame_header =	 frame_header		 | mask;
	frame_header = ( frame_header << 7 ) | len;
	// Byte 1.
	frame_header = ( frame_header << 1 ) | fin;
	frame_header = ( frame_header << 1 ) | rsv1;
	frame_header = ( frame_header << 1 ) | rsv2;
	frame_header = ( frame_header << 1 ) | rsv3;
	frame_header = ( frame_header << 4 ) | opcode;

	int buf_offset = 0;
	_memcpy_s( buf, buf_length, &frame_header, sizeof( unsigned short ) );
	buf_offset += sizeof( unsigned short );

	if ( len_type == 1 )	// Extended payload 16 bits.
	{
		unsigned short extended_payload_length = _ntohs( ( unsigned short )payload_length );
		_memcpy_s( buf + buf_offset, buf_length - buf_offset, &extended_payload_length, sizeof( unsigned short ) );
		buf_offset += sizeof( unsigned short );
	}
	else if ( len_type == 2 )	// Extended payload 64 bits.
	{
		unsigned __int64 extended_payload_length = ntohll( payload_length );
		_memcpy_s( buf + buf_offset, buf_length - buf_offset, &extended_payload_length, sizeof( unsigned __int64 ) );
		buf_offset += sizeof( unsigned __int64 );
	}

	/*if ( payload_length > 0 && payload_length <= ( buf_length - buf_offset )  )
	{
		_memcpy_s( buf + buf_offset, buf_length - buf_offset, payload, ( rsize_t )payload_length );
		buf_offset += ( int )payload_length;
	}*/

	return buf_offset;
}

char from_hex( char c )
{
  return is_digit( c ) ? c - '0' : ( char )_CharLowerA( ( LPSTR )c ) - 'a' + 10;
}

bool is_hex( char c )
{
	return ( is_digit( c ) || ( ( char )_CharLowerA( ( LPSTR )c ) - 'a' + 0U < 6U ) );
}

char *url_decode( char *str, unsigned int str_len, unsigned int *dec_len )
{
	char *pstr = str;
	char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( str_len + 1 ) );
	char *pbuf = buf;

	while ( pstr < ( str + str_len ) )
	{
		if ( *pstr == '%' )
		{
			// Look at the next two characters.
			if ( ( ( pstr + 3 ) <= ( str + str_len ) ) )
			{
				// See if they're both hex values.
				if ( ( pstr[ 1 ] != NULL && is_hex( pstr[ 1 ] ) ) &&
					 ( pstr[ 2 ] != NULL && is_hex( pstr[ 2 ] ) ) )
				{
					*pbuf++ = from_hex( pstr[ 1 ] ) << 4 | from_hex( pstr[ 2 ] );
					pstr += 2;
				}
				else
				{
					*pbuf++ = *pstr;
				}
			}
			else
			{
				*pbuf++ = *pstr;
			}
		}
		else if ( *pstr == '+' )
		{ 
			*pbuf++ = ' ';
		}
		else
		{
			*pbuf++ = *pstr;
		}

		pstr++;
	}

	*pbuf = '\0';

	if ( dec_len != NULL )
	{
		*dec_len = pbuf - buf;
	}

	return buf;
}


char *json_escape( char *str, unsigned int str_len, unsigned int *enc_len )
{
	if ( str == NULL || str_len == 0 )
	{
		return NULL;
	}

	// Find either of the two escape characters.
	/*char *find_ec = _StrChrA( str, '\\' );
	if ( find_ec == NULL )
	{
		find_ec = _StrChrA( str, '\"' );
		if ( find_ec == NULL )
		{
			return NULL;
		}
	}*/

	char *pstr = str;
	char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( ( str_len * 2 ) + 1 ) );
	char *pbuf = buf;

	while ( pstr < ( str + str_len ) )
	{
		if ( *pstr == '\"' )
		{
			*pbuf++ = '\\';
			*pbuf++ = '\"';
		}
		else if ( *pstr == '\\' )
		{
			*pbuf++ = '\\';
			*pbuf++ = '\\';
		}
		else if ( *pstr == '/' )
		{
			*pbuf++ = '\\';
			*pbuf++ = '/';
		}
		else if ( *pstr == '\n' )
		{
			*pbuf++ = '\\';
			*pbuf++ = 'n';
		}
		else if ( *pstr == '\r' )
		{
			*pbuf++ = '\\';
			*pbuf++ = 'r';
		}
		else if ( *pstr == '\t' )
		{
			*pbuf++ = '\\';
			*pbuf++ = 't';
		}
		else if ( *pstr == '\b' )
		{
			*pbuf++ = '\\';
			*pbuf++ = 'b';
		}
		else if ( *pstr == '\f' )
		{
			*pbuf++ = '\\';
			*pbuf++ = 'f';
		}
		else if ( *pstr == '\u' )
		{
			*pbuf++ = '\\';
			*pbuf++ = 'u';
		}
		else// if ( *pstr > 0x1F && *pstr != 0x7F )
		{
			*pbuf++ = *pstr;
		}

		pstr++;
	}

	*pbuf = '\0';

	/*if ( ( pbuf - buf ) == str_len )
	{
		GlobalFree( buf );
		return NULL;
	}*/

	if ( enc_len != NULL )
	{
		*enc_len = pbuf - buf;
	}

	return buf;
}

char *GetFileExtension( char *path )
{
	if ( path == NULL )
	{
		return NULL;
	}

	unsigned short length = lstrlenA( path );

	while ( length != 0 && path[ --length ] != L'.' );

	return path + length;
}


char *GetFileName( char *path )
{
	if ( path == NULL )
	{
		return NULL;
	}

	unsigned short length = lstrlenA( path );

	while ( length != 0 && path[ --length ] != '\\' );

	if ( path[ length ] == '\\' )
	{
		++length;
	}
	return path + length;
}

char *GetMIMEByFileName( char *filename )
{
	static char *mime_type[ 10 ] = { "application/javascript; charset=UTF-8",
									"application/x-shockwave-flash",
									"image/bmp",
									"image/gif",
									"image/jpeg",
									"image/png",
									"text/css",
									"text/html; charset=UTF-8",
									"audio/mpeg",
									"audio/ogg" };
	if ( filename != NULL )
	{
		if ( filename[ 0 ] == NULL )
		{
			return mime_type[ 7 ];
		}
		else
		{
			char *extension = GetFileExtension( filename ) + 1;

			if ( extension != NULL )
			{
				if ( lstrcmpiA( extension, "html" ) == 0 )
				{
					return mime_type[ 7 ];
				}
				else if ( lstrcmpiA( extension, "htm" ) == 0 )
				{
					return mime_type[ 7 ];
				}
				else if ( lstrcmpiA( extension, "css" ) == 0 )
				{
					return mime_type[ 6 ];
				}
				else if ( lstrcmpiA( extension, "js" ) == 0 )
				{
					return mime_type[ 0 ];
				}
				else if ( lstrcmpiA( extension, "jpg" ) == 0 )
				{
					return mime_type[ 4 ];
				}
				else if ( lstrcmpiA( extension, "png" ) == 0 )
				{
					return mime_type[ 5 ];
				}
				else if ( lstrcmpiA( extension, "gif" ) == 0 )
				{
					return mime_type[ 3 ];
				}
				else if ( lstrcmpiA( extension, "jpeg" ) == 0 )
				{
					return mime_type[ 4 ];
				}
				else if ( lstrcmpiA( extension, "swf" ) == 0 )
				{
					return mime_type[ 1 ];
				}
				else if ( lstrcmpiA( extension, "bmp" ) == 0 )
				{
					return mime_type[ 2 ];
				}
				else if ( lstrcmpiA( extension, "mp3" ) == 0 )
				{
					return mime_type[ 8 ];
				}
				else if ( lstrcmpiA( extension, "ogg" ) == 0 )
				{
					return mime_type[ 9 ];
				}
				else if ( lstrcmpiA( extension, "jpe" ) == 0 )
				{
					return mime_type[ 4 ];
				}
				else if ( lstrcmpiA( extension, "jfif" ) == 0 )
				{
					return mime_type[ 4 ];
				}
				else if ( lstrcmpiA( extension, "dib" ) == 0 )
				{
					return mime_type[ 2 ];
				}
			}
		}
	}
	
	return "application/unknown";
}
