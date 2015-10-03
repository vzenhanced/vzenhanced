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

// Must use GlobalFree on this.
char *GlobalStrDupA( const char *_Str )
{
	if ( _Str == NULL )
	{
		return NULL;
	}

	size_t size = lstrlenA( _Str ) + sizeof( char );

	char *ret = ( char * )GlobalAlloc( GMEM_FIXED, size );

	if ( ret == NULL )
	{
		return NULL;
	}

	_memcpy_s( ret, size, _Str, size );

	return ret;
}

// Must use GlobalFree on this.
wchar_t *GlobalStrDupW( const wchar_t *_Str )
{
	if ( _Str == NULL )
	{
		return NULL;
	}

	size_t size = lstrlenW( _Str ) * sizeof( wchar_t ) + sizeof( wchar_t );

	wchar_t *ret = ( wchar_t * )GlobalAlloc( GMEM_FIXED, size );

	if ( ret == NULL )
	{
		return NULL;
	}

	_memcpy_s( ret, size, _Str, size );

	return ret;
}

int dllrbt_compare( void *a, void *b )
{
	return lstrcmpA( ( char * )a, ( char * )b );
}

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

		/*for ( int i = 0; i < str_pos_end - str_pos_start; ++i )
		{
			str_pos_start[ i ] = ( char )_CharLowerA( ( LPSTR )str_pos_start[ i ] );
		}*/

		_CharLowerBuffA( str_pos_start, str_pos_end - str_pos_start );
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

unsigned long long strtoull( char *str )
{
	if ( str == NULL )
	{
		return 0;
	}

	char *p = str;

	ULARGE_INTEGER uli;
	uli.QuadPart = 0;

	unsigned char digit = 0;

	while ( *p && ( *p >= '0' && *p <= '9' ) )
	{
		if ( uli.QuadPart > ( ULLONG_MAX / 10 ) )
		{
			uli.QuadPart = ULLONG_MAX;
			break;
		}

		//uli.QuadPart *= 10;

		__asm
		{
			mov     eax, dword ptr [ uli.QuadPart + 4 ]
			cmp		eax, 0					;// See if our QuadPart's value extends to 64 bits.
			mov     ecx, 10					;// Store the base (10) multiplier (low order bits).
			jne     short extend			;// If there are high order bits in QuadPart, then multiply/add high and low bits.

			mov     eax, dword ptr [ uli.QuadPart + 0 ]	;// Store the QuadPart's low order bits.
			mul     ecx						;// Multiply the low order bits.

			jmp		finish					;// Store the results in our 64 bit value.

		extend:

			push    ebx						;// Save value to stack.

			mul     ecx						;// Multiply the high order bits of QuadPart with the low order bits of base (10).
			mov     ebx, eax				;// Store the result.

			mov     eax, dword ptr [ uli.QuadPart + 0 ]	;// Store QuadPart's low order bits.
			mul     ecx						;// Multiply the low order bits of QuadPart with the low order bits of base (10). edx = high, eax = low
			add     edx, ebx				;// Add the low order bits (ebx) to the high order bits (edx).

			pop     ebx						;// Restore value from stack.

		finish:

			mov		uli.HighPart, edx		;// Store the high order bits.
			mov		uli.LowPart, eax		;// Store the low order bits.
		}

		digit = *p - '0';

		if ( uli.QuadPart > ( ULLONG_MAX - digit ) )
		{
			uli.QuadPart = ULLONG_MAX;
			break;
		}

		uli.QuadPart += digit;

		++p;
	}

	return uli.QuadPart;
}

char from_hex( char c )
{
	if ( is_digit( c ) )
	{
		return c - '0';
	}
	else if ( c - 'a' + 0U < 6U )
	{
		return c - 'a' + 10;
	}
	else if ( c - 'A' + 0U < 6U )
	{
		return c - 'A' + 10;
	}

	return c;
}

bool is_hex( char c )
{
	return ( is_digit( c ) || ( c - 'a' + 0U < 6U ) || ( c - 'A' + 0U < 6U ) );
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
		/*else if ( *pstr == '\u' )	// \u0000
		{
			*pbuf++ = '\\';
			*pbuf++ = 'u';
		}*/
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

char *GetUTF8Domain( wchar_t *domain )
{
	int domain_length = WideCharToMultiByte( CP_UTF8, 0, domain, -1, NULL, 0, NULL, NULL );
	char *utf8_domain = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * domain_length ); // Size includes the null character.
	WideCharToMultiByte( CP_UTF8, 0, domain, -1, utf8_domain, domain_length, NULL, NULL );

	return utf8_domain;
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
	static char *mime_type[ 11 ] = { "application/javascript; charset=UTF-8",
									"application/x-shockwave-flash",
									"image/bmp",
									"image/gif",
									"image/jpeg",
									"image/png",
									"image/x-icon",
									"text/css",
									"text/html; charset=UTF-8",
									"audio/mpeg",
									"audio/ogg" };
	if ( filename != NULL )
	{
		if ( filename[ 0 ] == NULL )
		{
			return mime_type[ 8 ];
		}
		else
		{
			char *extension = GetFileExtension( filename ) + 1;

			if ( extension != NULL )
			{
				if ( lstrcmpiA( extension, "html" ) == 0 )
				{
					return mime_type[ 8 ];
				}
				else if ( lstrcmpiA( extension, "htm" ) == 0 )
				{
					return mime_type[ 8 ];
				}
				else if ( lstrcmpiA( extension, "css" ) == 0 )
				{
					return mime_type[ 7 ];
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
				else if ( lstrcmpiA( extension, "ico" ) == 0 )
				{
					return mime_type[ 6 ];
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
					return mime_type[ 9 ];
				}
				else if ( lstrcmpiA( extension, "ogg" ) == 0 )
				{
					return mime_type[ 10 ];
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
