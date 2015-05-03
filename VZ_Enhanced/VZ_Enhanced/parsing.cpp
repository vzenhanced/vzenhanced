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

#include "globals.h"
#include "parsing.h"
#include "utilities.h"
#include "stack_tree.h"

dllrbt_tree *contact_list = NULL;

struct FORM_ELEMENTS
{
	char *name;
	char *value;
	int name_length;
	int value_length;
};

char *strnchr( const char *s, int c, int n )
{
	if ( s == NULL )
	{
		return NULL;
	}

	while ( *s != NULL && n-- > 0 )
	{
		if ( *s == c )
		{
			return ( ( char * )s );
		}
		s++;
	}
	
	return NULL;
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

// Allocates a new string if characters need encoding. Otherwise, it returns NULL.
char *encode_xml_entities( const char *string )
{
	char *encoded_string = NULL;
	char *q = NULL;
	const char *p = NULL;
	int c = 0;

	if ( string == NULL )
	{
		return NULL;
	}

	// Get the character count and offset it for any entities.
	for ( c = 0, p = string; *p != NULL; ++p ) 
	{
		switch ( *p ) 
		{
			case '&':
			{
				c += 5;	// "&amp;"
			}
			break;

			case '\'':
			{
				c += 6;	// "&apos;"
			}
			break;

			case '"':
			{
				c += 6;	// "&quot;"
			}
			break;

			case '<':
			{
				c += 4;	// "&lt;"
			}
			break;

			case '>':
			{
				c += 4;	// "&gt;"
			}
			break;

			default:
			{
				++c;
			}
			break;
		}
	}

	// If the string has no special characters to encode, then return NULL.
	if ( c <= ( p - string ) )
	{
		return NULL;
	}

	q = encoded_string = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( c + 1 ) );

	for ( p = string; *p != NULL; ++p ) 
	{
		switch ( *p ) 
		{
			case '&':
			{
				_memcpy_s( q, 5, "&amp;", 5 );
				q += 5;
			}
			break;

			case '\'':
			{
				_memcpy_s( q, 6, "&apos;", 6 );
				q += 6;
			}
			break;

			case '"':
			{
				_memcpy_s( q, 6, "&quot;", 6 );
				q += 6;
			}
			break;

			case '<':
			{
				_memcpy_s( q, 4, "&lt;", 4 );
				q += 4;
			}
			break;

			case '>':
			{
				_memcpy_s( q, 4, "&gt;", 4 );
				q += 4;
			}
			break;

			default:
			{
				*q = *p;
				++q;
			}
			break;
		}
	}

	*q = 0;	// Sanity.

	return encoded_string;
}

// Overwrites the string argument and returns it when done.
char *decode_xml_entities( char *string )
{
	char *p = NULL;
	char *q = NULL;

	if ( string == NULL )
	{
		return NULL;
	}

	for ( p = q = string; *p != NULL; ++p, ++q ) 
	{
		if ( *p == '&' )	// Entity name representation
		{
			if ( !_StrCmpNIA( p + 1, "amp;", 4 ) )
			{
				*q = '&';
				p += 4;
			}
			else if ( !_StrCmpNIA( p + 1, "apos;", 5 ) )
			{
				*q = '\'';
				p += 5;
			}
			else if ( !_StrCmpNIA( p + 1, "quot;", 5 ) )
			{
				*q = '"';
				p += 5;
			}
			else if ( !_StrCmpNIA( p + 1, "lt;", 3 ) )
			{
				*q = '<';
				p += 3;
			}
			else if ( !_StrCmpNIA( p + 1, "gt;", 3 ) )
			{
				*q = '>';
				p += 3;
			}
			else if ( *( p + 1 ) == '#' )	// Decimal representation
			{
				if ( !_StrCmpNIA( p + 2, "38;", 3 ) )
				{
					*q = '&';
					p += 4;
				}
				else if ( !_StrCmpNIA( p + 2, "39;", 3 ) )
				{
					*q = '\'';
					p += 4;
				}
				else if ( !_StrCmpNIA( p + 2, "34;", 3 ) )
				{
					*q = '"';
					p += 4;
				}
				else if ( !_StrCmpNIA( p + 2, "60;", 3 ) )
				{
					*q = '<';
					p += 4;
				}
				else if ( !_StrCmpNIA( p + 2, "62;", 3 ) )
				{
					*q = '>';
					p += 4;
				}
				else if ( *( p + 2 ) == 'x' || *( p + 2 ) == 'X' )	// Hexadecimal representation.
				{
					if ( !_StrCmpNIA( p + 3, "26;", 3 ) )
					{
						*q = '&';
						p += 5;
					}
					else if ( !_StrCmpNIA( p + 3, "27;", 3 ) )
					{
						*q = '\'';
						p += 5;
					}
					else if ( !_StrCmpNIA( p + 3, "22;", 3 ) )
					{
						*q = '"';
						p += 5;
					}
					else if ( !_StrCmpNIA( p + 3, "3c;", 3 ) )
					{
						*q = '<';
						p += 5;
					}
					else if ( !_StrCmpNIA( p + 3, "3e;", 3 ) )
					{
						*q = '>';
						p += 5;
					}
					else
					{
						*q = *p;
					}
				}
				else
				{
					*q = *p;
				}
			}
			else
			{
				*q = *p;
			}
		}
		else
		{
			*q = *p;
		}
	}

	*q = 0;	// Sanity.

	return string;
}

bool ParseURL( char *url, char **host, char **resource )
{
	if ( url == NULL )
	{
		return false;
	}

	// Find the start of the host.
	char *str_pos_start = _StrStrA( url, "//" );
	if ( str_pos_start == NULL )
	{
		return false;
	}
	str_pos_start += 2;

	// Find the end of the host.
	char *str_pos_end = _StrChrA( str_pos_start, '/' );
	if ( str_pos_end == NULL )
	{
		return false;
	}

	int host_length = str_pos_end - str_pos_start;

	// Save the host.
	*host = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( host_length + 1 ) );
	_memcpy_s( *host, sizeof( char ) * ( host_length + 1 ), str_pos_start, host_length + 1 );
	*( *host + host_length ) = 0;	// Sanity

	str_pos_start += host_length;

	// Save the resource.
	*resource = GlobalStrDupA( str_pos_start );

	return true;
}

// The form is in HTML.
bool ParseSAMLForm( char *decoded_buffer, char **host, char **resource, char **parameters, int &parameter_length )
{
	char *element_start = decoded_buffer;
	char *element_end = decoded_buffer;
	char *find_attribute = NULL;
	char *name = NULL;
	char *value = NULL;
	char *action = NULL;

	DoublyLinkedList *element_ll = NULL;
	int total_element_length = 0;
	int number_of_elements = 0;

	bool bad_form = false;

	while ( true )
	{
		// Look for the opening of an HTML element.
		element_start = _StrChrA( element_end, '<' );
		if ( element_start == NULL )
		{
			return false;
		}

		// Find the end of the HTML element.
		element_end = _StrChrA( element_start + 1, '>' );
		if ( element_end == NULL )
		{
			return false;
		}

		// See if the length of the element is enough for a form tag with attributes.
		if ( ( element_end - element_start ) >= 12 ) // It should have at least: "<form action"
		{
			// We expect the form to have attributes. Thus the space.
			if ( _StrCmpNIA( element_start, "<form ", 6 ) == 0 )
			{
				element_start += 6;

				find_attribute = element_start;

				// Locate each attribute.
				while ( find_attribute < element_end )
				{
					// Each attribute will begin with an "=".
					find_attribute = _StrChrA( find_attribute, '=' );
					if ( find_attribute == NULL || find_attribute >= element_end )
					{
						break;
					}
					++find_attribute;

					// Go back enough characters and see if the attribute is "action". The subtraction should be guaranteed.

					// Skip whitespace that could appear after the attribute name, but before the "=".
					char *go_back = find_attribute - 1;
					while ( ( go_back - 1 ) >= ( element_start + 6 ) )	// +6 for "action"
					{
						if ( *( go_back - 1 ) != ' ' && *( go_back - 1 ) != '\t' && *( go_back - 1 ) != '\f' )
						{
							break;
						}

						--go_back;
					}

					if ( _StrCmpNIA( go_back - 6, "action", 6 ) == 0 )	// We found the attribute.
					{
						// Skip whitespace that could appear after the "=", but before the attribute value.
						while ( find_attribute < element_end )
						{
							if ( find_attribute[ 0 ] != ' ' && find_attribute[ 0 ] != '\t' && find_attribute[ 0 ] != '\f' )
							{
								break;
							}

							++find_attribute;
						}

						action = find_attribute;
						break;
					}
				}

				if ( action == NULL )
				{
					return false;
				}

				char delimiter = action[ 0 ];
				char *action_end = element_end;

				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++action;

					action_end = _StrChrA( action, delimiter );	// Find the end of the ID.
					if ( action_end == NULL || action_end >= element_end )
					{
						return false;
					}
				}
				else	// No delimiter.
				{
					// Let's see if it ends with a space.
					action_end = _StrChrA( action, ' ' );	// Find the end of the URL.
					if ( action_end == NULL || action_end >= element_end )
					{
						// If not, then it ends before the end of the element.
						action_end = element_end;
					}
				}

				char *domain_start = _StrStrA( action, "//" );		// Find the beginning of the URL's domain name.
				if ( domain_start == NULL || domain_start >= element_end )
				{
					return false;
				}
				domain_start += 2;

				char *domain_end = _StrChrA( domain_start, '/' );	// Find the end of the domain name.
				if ( domain_end == NULL || domain_end >= element_end )
				{
					return false;
				}
				int host_length = domain_end - domain_start;

				*host = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( host_length + 1 ) );
				_memcpy_s( *host, sizeof( char ) * ( host_length + 1 ), domain_start, host_length + 1 );
				*( *host + host_length ) = 0;	// Sanity

				domain_start += host_length;

				int resource_length = action_end - domain_start;

				*resource = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( resource_length + 1 ) );
				_memcpy_s( *resource, sizeof( char ) * ( resource_length + 1 ), domain_start, resource_length + 1 );
				*( *resource + resource_length ) = 0;	// Sanity

				break;
			}
		}
	}

	while ( true )
	{
		// Look for the opening of an HTML element.
		element_start = _StrChrA( element_end, '<' );
		if ( element_start == NULL )
		{
			bad_form = true;
			break;
		}

		// Find the end of the HTML element.
		element_end = _StrChrA( element_start + 1, '>' );
		if ( element_end == NULL )
		{
			bad_form = true;
			break;
		}

		// See if the length of the element is enough for an input tag with attributes.
		if ( ( element_end - element_start ) >= 12 ) // It should have at least: "<input value". "<input name" is implied.
		{
			// We expect the form to have attributes. Thus the space.
			if ( _StrCmpNIA( element_start, "<input ", 7 ) == 0 )
			{
				element_start += 7;

				find_attribute = element_start;

				// Locate each attribute.
				while ( find_attribute < element_end )
				{
					// Each attribute will begin with an "=".
					find_attribute = _StrChrA( find_attribute, '=' );
					if ( find_attribute == NULL || find_attribute >= element_end )
					{
						break;
					}
					++find_attribute;

					// Go back enough characters and see if the attribute is "name" or "value". The subtraction should be guaranteed.

					// Skip whitespace that could appear after the attribute name, but before the "=".
					char *go_back = find_attribute - 1;
					while ( ( go_back - 1 ) >= ( element_start + 4 ) )	// +4 for "name". Setting this to +5 for "value" would cause go_back to be incorrectly offset when there's whitespace before the "=".
					{
						if ( *( go_back - 1 ) != ' ' && *( go_back - 1 ) != '\t' && *( go_back - 1 ) != '\f' )
						{
							break;
						}

						--go_back;
					}

					if ( name == NULL && _StrCmpNIA( go_back - 4, "name", 4 ) == 0 )	// We found the attribute.
					{
						// Skip whitespace that could appear after the "=", but before the attribute value.
						while ( find_attribute < element_end )
						{
							if ( find_attribute[ 0 ] != ' ' && find_attribute[ 0 ] != '\t' && find_attribute[ 0 ] != '\f' )
							{
								break;
							}

							++find_attribute;
						}

						name = find_attribute;
					}
					else if ( value == NULL && _StrCmpNIA( go_back - 5, "value", 5 ) == 0 )	// We found the attribute.
					{
						// Skip whitespace that could appear after the "=", but before the attribute value.
						while ( find_attribute < element_end )
						{
							if ( find_attribute[ 0 ] != ' ' && find_attribute[ 0 ] != '\t' && find_attribute[ 0 ] != '\f' )
							{
								break;
							}

							++find_attribute;
						}

						value = find_attribute;
					}

					if ( name != NULL && value != NULL )
					{
						char delimiter = name[ 0 ];
						char *name_end = element_end;

						if ( delimiter == '\"' || delimiter == '\'' )
						{
							++name;

							name_end = _StrChrA( name, delimiter );	// Find the end of the ID.
							if ( name_end == NULL || name_end >= element_end )
							{
								return false;
							}
						}
						else	// No delimiter.
						{
							// Let's see if it ends with a space.
							name_end = _StrChrA( name, ' ' );	// Find the end of the URL.
							if ( name_end == NULL || name_end >= element_end )
							{
								// If not, then it ends before the end of the element.
								name_end = element_end;
							}
						}

						delimiter = value[ 0 ];
						char *value_end = element_end;

						if ( delimiter == '\"' || delimiter == '\'' )
						{
							++value;

							value_end = _StrChrA( value, delimiter );	// Find the end of the ID.
							if ( value_end == NULL || value_end >= element_end )
							{
								return false;
							}
						}
						else	// No delimiter.
						{
							// Let's see if it ends with a space.
							value_end = _StrChrA( value, ' ' );	// Find the end of the URL.
							if ( value_end == NULL || value_end >= element_end )
							{
								// If not, then it ends before the end of the element.
								value_end = element_end;
							}
						}

						FORM_ELEMENTS *fe = ( FORM_ELEMENTS * )GlobalAlloc( GMEM_FIXED, sizeof( FORM_ELEMENTS ) );

						unsigned int enc_len = 0;
						char *enc_param = url_encode( name, name_end - name, &enc_len );
						fe->name = enc_param;
						fe->name_length = enc_len;

						enc_param = url_encode( value, value_end - value, &enc_len );
						fe->value = enc_param;
						fe->value_length = enc_len;

						total_element_length += ( fe->name_length + fe->value_length );
						++number_of_elements;

						DoublyLinkedList *dll = DLL_CreateNode( ( void * )fe );

						if ( element_ll == NULL )
						{
							element_ll = dll;
						}
						else
						{
							DLL_AddNode( &element_ll, dll, 0 );
						}

						name = NULL;
						value = NULL;
						break;
					}
				}
			}
		}
		else if ( ( element_end - element_start ) == 6 )	// "</form"
		{
			if ( _StrCmpNIA( element_start, "</form", 6 ) == 0 )
			{
				break;
			}
		}

		if ( bad_form == true )
		{
			break;
		}
	}

	DoublyLinkedList *current_node = element_ll;

	if ( bad_form == true )
	{
		while ( current_node != NULL )
		{
			DoublyLinkedList *del_node = current_node;
			current_node = current_node->next;

			GlobalFree( ( ( FORM_ELEMENTS * )del_node->data )->name );
			GlobalFree( ( ( FORM_ELEMENTS * )del_node->data )->value );
			GlobalFree( ( FORM_ELEMENTS * )del_node->data );
			GlobalFree( del_node );
		}

		return false;
	}
	else
	{
		// Add & and = URL parameters and the NULL character.
		total_element_length += ( number_of_elements * 2 );

		int parameters_position = 0;

		*parameters = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * total_element_length );

		for ( int i = 0; i < number_of_elements; ++i )
		{
			if ( current_node != NULL )
			{
				DoublyLinkedList *del_node = current_node;
				FORM_ELEMENTS *fe = ( FORM_ELEMENTS * )del_node->data;

				if ( i > 0 )
				{
					*( *parameters + parameters_position ) = '&';
					++parameters_position;
				}

				_memcpy_s( *parameters + parameters_position, total_element_length - parameters_position, fe->name, fe->name_length );
				parameters_position += fe->name_length;
				*( *parameters + parameters_position ) = '=';
				++parameters_position;
				_memcpy_s( *parameters + parameters_position, total_element_length - parameters_position, fe->value, fe->value_length );
				parameters_position += fe->value_length;

				current_node = current_node->next;

				GlobalFree( fe->name );
				GlobalFree( fe->value );
				GlobalFree( fe );
				GlobalFree( del_node );
			}
		}

		*( *parameters + parameters_position ) = 0;	// Sanity

		parameter_length = parameters_position;

		return true;
	}
}

// Modifies decoded_buffer
bool ParseXApplicationErrorCode( char *decoded_buffer, char **error_code )
{
	char *end_of_header = fields_tolower( decoded_buffer );	// Normalize the header fields. Returns the end of header.
	if ( end_of_header == NULL )
	{
		return false;
	}
	*( end_of_header + 2 ) = 0;	// Make our search string shorter.

	char *str_pos_start = _StrStrA( decoded_buffer, "x-application-error-code:" );
	if ( str_pos_start == NULL )
	{
		*( end_of_header + 2 ) = '\r';	// Restore the end of header.
		return false;
	}
	str_pos_start += 25;

	// Skip whitespace that could appear after the ":", but before the field value.
	while ( str_pos_start < end_of_header )
	{
		if ( str_pos_start[ 0 ] != ' ' && str_pos_start[ 0 ] != '\t' )
		{
			break;
		}

		++str_pos_start;
	}

	char *str_pos_end = _StrStrA( str_pos_start, "\r\n" );	// Find the end of the error code.
	if ( str_pos_end == NULL )
	{
		*( end_of_header + 2 ) = '\r';	// Restore the end of header.
		return false;
	}

	// Skip whitespace that could appear before the "\r\n", but after the field value.
	while ( ( str_pos_end - 1 ) >= str_pos_start )
	{
		if ( *( str_pos_end - 1 ) != ' ' && *( str_pos_end - 1 ) != '\t' )
		{
			break;
		}

		--str_pos_end;
	}

	int error_code_length = str_pos_end - str_pos_start;

	*error_code = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( error_code_length + 1 ) );
	_memcpy_s( *error_code, sizeof( char ) * ( error_code_length + 1 ), str_pos_start, error_code_length + 1 );
	*( *error_code + error_code_length ) = 0;	// Sanity

	*( end_of_header + 2 ) = '\r';	// Restore the end of header.

	return true;
}

// Modifies decoded_buffer
bool ParseRedirect( char *decoded_buffer, char **host, char **resource )
{
	char *end_of_header = fields_tolower( decoded_buffer );	// Normalize the header fields.
	if ( end_of_header == NULL )
	{
		return false;
	}
	*( end_of_header + 2 ) = 0;	// Make our search string shorter.

	char *str_pos_start = _StrStrA( decoded_buffer, "location:" );
	if ( str_pos_start == NULL )
	{
		*( end_of_header + 2 ) = '\r';	// Restore the end of header.
		return false;
	}
	str_pos_start = _StrStrA( str_pos_start + 9, "//" );					// Find the beginning of the URL's domain name.
	if ( str_pos_start == NULL )
	{
		*( end_of_header + 2 ) = '\r';	// Restore the end of header.
		return false;
	}
	str_pos_start += 2;

	char *str_pos_end = _StrChrA( str_pos_start, '/' );
	if ( str_pos_end == NULL )
	{
		*( end_of_header + 2 ) = '\r';	// Restore the end of header.
		return false;
	}
	int host_length = str_pos_end - str_pos_start;

	*host = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( host_length + 1 ) );
	_memcpy_s( *host, sizeof( char ) * ( host_length + 1 ), str_pos_start, host_length + 1 );
	*( *host + host_length ) = 0;	// Sanity

	str_pos_start += host_length;

	str_pos_end = _StrStrA( str_pos_start, "\r\n" );					// Find the end of the URL.
	if ( str_pos_end == NULL )
	{
		*( end_of_header + 2 ) = '\r';	// Restore the end of header.
		return false;
	}

	// Skip whitespace that could appear before the "\r\n", but after the field value.
	while ( ( str_pos_end - 1 ) >= str_pos_start )
	{
		if ( *( str_pos_end - 1 ) != ' ' && *( str_pos_end - 1 ) != '\t' )
		{
			break;
		}

		--str_pos_end;
	}

	int resource_length = str_pos_end - str_pos_start;

	*resource = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( resource_length + 1 ) );
	_memcpy_s( *resource, sizeof( char ) * ( resource_length + 1 ), str_pos_start, resource_length + 1 );
	*( *resource + resource_length ) = 0;	// Sanity

	*( end_of_header + 2 ) = '\r';	// Restore the end of header.

	return true;
}

void ConstructCookie( dllrbt_tree *cookie_tree, char **cookies )
{
	if ( cookie_tree == NULL )
	{
		return;
	}

	unsigned int total_cookie_length = 0;

	// Get the length of all cookies.
	node_type *node = dllrbt_get_head( cookie_tree );
	while ( node != NULL )
	{
		COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )node->val;

		if ( cc != NULL )
		{
			total_cookie_length += ( cc->name_length + cc->value_length + 2 );
		}

		node = node->next;
	}

	*cookies = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( total_cookie_length + 1 ) );

	int cookie_length = 0;
	int count = 0;

	// Construct the cookie string.
	node = dllrbt_get_head( cookie_tree );
	while ( node != NULL )
	{
		COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )node->val;

		if ( cc != NULL )
		{
			// Add "; " at the end of the cookie string (before the current cookie).
			if ( count > 0 )
			{
				*( *cookies + cookie_length++ ) = ';';
				*( *cookies + cookie_length++ ) = ' ';
			}

			++count;

			_memcpy_s( *cookies + cookie_length, total_cookie_length  - cookie_length, cc->cookie_name, cc->name_length );
			cookie_length += cc->name_length;

			_memcpy_s( *cookies + cookie_length, total_cookie_length  - cookie_length, cc->cookie_value, cc->value_length );
			cookie_length += cc->value_length;
		}

		node = node->next;
	}

	*( *cookies + cookie_length ) = 0;	// Sanity
}

// Modifies decoded_buffer
bool ParseCookies( char *decoded_buffer, dllrbt_tree **cookie_tree, char **cookies )
{
	char *end_of_header = fields_tolower( decoded_buffer );	// Normalize the header fields. Returns the end of header.
	if ( end_of_header == NULL )
	{
		return false;
	}
	*( end_of_header + 2 ) = 0;	// Make our search string shorter.

	char *cookie_end = decoded_buffer;
	char *cookie_search = decoded_buffer;

	while ( cookie_search < end_of_header )
	{
		cookie_search = _StrStrA( cookie_end, "set-cookie:" );
		if ( cookie_search == NULL || cookie_search >= end_of_header )
		{
			break;			// No more cookies found.
		}
		cookie_search += 11;

		// Skip whitespace that could appear after the ":", but before the field value.
		while ( cookie_search < end_of_header )
		{
			if ( cookie_search[ 0 ] != ' ' && cookie_search[ 0 ] != '\t' )
			{
				break;
			}

			++cookie_search;
		}

		cookie_end = _StrStrA( cookie_search, "\r\n" );
		if ( cookie_end == NULL )
		{
			*( end_of_header + 2 ) = '\r';	// Restore the end of header.
			return false;	// If this is true, then it means we found the Set-Cookie field, but no termination character. We can't continue.
		}

		char *cookie_value_end = cookie_end;
		// Skip whitespace that could appear before the "\r\n", but after the field value.
		while ( ( cookie_value_end - 1 ) >= cookie_search )
		{
			if ( *( cookie_value_end - 1 ) != ' ' && *( cookie_value_end - 1 ) != '\t' )
			{
				break;
			}

			--cookie_value_end;
		}

		char *cookie_name_end = _StrChrA( cookie_search, '=' );
		if ( cookie_name_end != NULL && cookie_name_end < cookie_value_end )
		{
			// Get the cookie name and add it to the tree.
			if ( *cookie_tree == NULL )
			{
				*cookie_tree = dllrbt_create( dllrbt_compare );
			}

			// Go back to the end of the cookie name if there's any whitespace after it and before the "=".
			while ( ( cookie_name_end - 1 ) >= cookie_search )
			{
				if ( *( cookie_name_end - 1 ) != ' ' && *( cookie_name_end - 1 ) != '\t' )
				{
					break;
				}

				--cookie_name_end;
			}

			COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )GlobalAlloc( GMEM_FIXED, sizeof( COOKIE_CONTAINER ) );

			cc->name_length = ( cookie_name_end - cookie_search );
			cc->cookie_name = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( cc->name_length + 1 ) );

			_memcpy_s( cc->cookie_name, cc->name_length + 1, cookie_search, cc->name_length );
			cc->cookie_name[ cc->name_length ] = 0;	// Sanity.

			// See if the cookie has attributes after it.
			char *cookie_attributes = strnchr( cookie_name_end, ';', ( cookie_value_end - cookie_name_end ) );
			if ( cookie_attributes != NULL && cookie_attributes < cookie_value_end )
			{
				cookie_search = cookie_attributes;
			}
			else
			{
				cookie_search = cookie_value_end;
			}

			cc->value_length = ( cookie_search - cookie_name_end );
			cc->cookie_value = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( cc->value_length + 1 ) );

			_memcpy_s( cc->cookie_value, cc->value_length + 1, cookie_name_end, cc->value_length );
			cc->cookie_value[ cc->value_length ] = 0;	// Sanity.

			// Attempt to add the container to the tree.
			dllrbt_status status = dllrbt_insert( *cookie_tree, ( void * )cc->cookie_name, ( void * )cc );
			if ( status == DLLRBT_STATUS_DUPLICATE_KEY )
			{
				// If there's a duplicate, find it.
				dllrbt_iterator *itr = dllrbt_find( *cookie_tree, ( void * )cc->cookie_name, false );

				// Free its values and remove it from the tree.
				if ( itr != NULL )
				{
					COOKIE_CONTAINER *occ = ( COOKIE_CONTAINER * )( ( node_type * )itr )->val;

					GlobalFree( occ->cookie_name );
					GlobalFree( occ->cookie_value );
					GlobalFree( occ );

					dllrbt_remove( *cookie_tree, itr );
				}

				// Try adding the new cc again. If it fails, then just free it.
				if ( dllrbt_insert( *cookie_tree, ( void * )cc->cookie_name, ( void * )cc ) != DLLRBT_STATUS_OK )
				{
					GlobalFree( cc->cookie_name );
					GlobalFree( cc->cookie_value );
					GlobalFree( cc );
				}
			}
			else if ( status != DLLRBT_STATUS_OK )	// If anything other than OK or duplicate was returned, then free the cookie container.
			{
				GlobalFree( cc->cookie_name );
				GlobalFree( cc->cookie_value );
				GlobalFree( cc );
			}
		}

		cookie_end += 2;
	}

	ConstructCookie( *cookie_tree, cookies );

	*( end_of_header + 2 ) = '\r';	// Restore the end of header.

	return true;
}

StackTree *BuildXMLTree( char *xml )
{
	StackTree *stack = NULL;
	int stack_depth = 0;

	while ( xml != NULL )
	{
		// Find the opening of the element.
		char *element_start = _StrChrA( xml, '<' );
		if ( element_start == NULL )
		{
			break;
		}
		++element_start;

		// Find the closing of the element.
		char *element_end = _StrChrA( element_start, '>' );
		if ( element_end == NULL )
		{
			break;
		}

		StackTree el;
		el.element_name = element_start;
		el.element_length = element_end - element_start;
		// See if the element has any attributes and determine the length of the element name.
		char *has_attributes = _StrChrA( element_start, ' ' );
		if ( has_attributes != NULL && has_attributes < element_end )
		{
			el.element_name_length = has_attributes - element_start;
		}
		else
		{
			el.element_name_length = el.element_length;
		}
		el.child = NULL;
		el.closing_element = NULL;
		el.parent = NULL;

		// Check to see if it's an opening or closing/self-closing element.
		if ( element_end - element_start > 1 && *( element_start ) == '/' && stack != NULL )				// Check if closing element.
		{
			// See if the element on the top of the stack matches the closing element.
			if ( _StrCmpNIA( stack->element_name, el.element_name + 1, stack->element_name_length ) == 0 )
			{
				--stack_depth;

				// Create the closing element.
				StackTree *closing_element = ST_CreateNode( el );

				// Make sure we're not at the root node.
				if ( stack_depth > 0 )
				{
					// Get the opening element and remove it from the top of the stack.
					StackTree *opening_element = ST_PopNode( &stack );
					opening_element->closing_element = closing_element;

					StackTree *parent_node = stack;	// The top of the stack will be the parent node.

					// Make sure the current top of the stack is not NULL.
					if ( parent_node != NULL )
					{
						// Set the parent node of the opening and closing elements.
						opening_element->parent = parent_node;
						closing_element->parent = parent_node;

						// If the parent node already has a child, then add the sibling to the list of child nodes.
						if ( parent_node->child != NULL )
						{
							opening_element->sibling = parent_node->child;
						}
						else
						{
							opening_element->sibling = NULL;	// Reset the sibling from when it was on the stack.
						}

						parent_node->child = opening_element;
					}
					else	// If it is NULL (which it shouldn't be), then we have a problem.
					{
						stack = opening_element;	// Set the opening element back on top of the stack.
					}
				}
				else if ( stack_depth == 0 )	// If we are at the root node, then just set its closing element.
				{
					stack->closing_element = closing_element;
				}
				else	// If we're less than the root, then it means we're missing an opening element.
				{
					GlobalFree( closing_element );
				}
			}
		}
		else if ( ( element_end - 1 ) > element_start && *( element_end - 1 ) == '/' )	// Check if self-closing element.
		{
			// Create the opening/self-closing element.
			StackTree *opening_element = ST_CreateNode( el );

			// Insert siblings into the parent node if the stack has a node on it.
			if ( stack_depth > 0 )
			{
				if ( stack != NULL )
				{
					StackTree *parent_node = stack;	// The top of the stack will be the parent node.

					// Set the parent node of the opening element.
					opening_element->parent = parent_node;

					// If the parent node already has a child, then add the sibling to the list of child nodes.
					if ( parent_node->child != NULL )
					{
						opening_element->sibling = parent_node->child;
					}
					else
					{
						opening_element->sibling = NULL;	// Reset the sibling from when it was on the stack.
					}

					parent_node->child = opening_element;
				}
				else
				{
					stack = opening_element;	// Set the opening element back on top of the stack.
				}
			}
			else	// If there's no node on the stack, then we just insert it normally. (Without inscreasing the stack depth).
			{
				// Add the opening/self-closing element to the top of the stack.
				ST_PushNode( &stack, opening_element );
			}
		}
		else	// Opening element.
		{
			// Create the opening element.
			StackTree *opening_element = ST_CreateNode( el );

			// Add the opening element to the top of the stack.
			ST_PushNode( &stack, opening_element );

			++stack_depth;
		}

		xml = element_end + 1;
	}

	// We should only have one root node, but comments and XML headers might mess up our tree.
	// Any element on the stack with children will need to be siblings instead.
	// Ideally, we'd drop everything without a closing element tag.
	StackTree *cleanup_stack = stack;
	while ( cleanup_stack != NULL && stack_depth > 0 )	// If we have more than 1 item on the stack.
	{
		// See if the current stack item has a child node.
		if ( cleanup_stack->child != NULL )
		{
			// Remove the child from the stack item and insert it back onto the stack as a sibling.
			StackTree *move_element = cleanup_stack->child;
			move_element->parent = NULL;
			cleanup_stack->child = NULL;

			ST_PushNode( &stack, move_element );
		}

		// See if there's more siblings on the stack.
		if ( cleanup_stack->sibling == NULL )
		{
			break;
		}

		// Move to the next sibling on the stack.
		cleanup_stack = cleanup_stack->sibling;

		--stack_depth;
	}

	return stack;
}

void DeleteXMLTree( StackTree *tree )
{
	while ( tree != NULL )
	{
		// Get the last child of the node.
		while ( tree != NULL && tree->child != NULL )
		{
			tree = tree->child;
		}

		// This should not be NULL.
		if ( tree != NULL )
		{
			// See if the node has a sibling.
			if ( tree->sibling != NULL )
			{
				StackTree *del_sibling = tree;

				tree = tree->sibling;	// If it does, then we move to it.

				if ( del_sibling->closing_element != NULL )
				{
					GlobalFree( del_sibling->closing_element );
				}

				GlobalFree( del_sibling );
			}
			else	// If it does not, then we need to go back up to the parent.
			{
				// Keep going up parent nodes until we find one with a sibling.
				while ( tree != NULL && tree->parent != NULL && tree->sibling == NULL )
				{
					StackTree *del_parent = tree;

					// Go back to the parent of the child node.
					tree = tree->parent;

					// Print the parent node's name.
					if ( del_parent != NULL )
					{
						if ( del_parent->closing_element != NULL )
						{
							GlobalFree( del_parent->closing_element );
						}

						GlobalFree( del_parent );
					}
				}

				StackTree *del_sibling = tree;
				
				// We found a sibling. Now move to it.
				tree = tree->sibling;

				if ( del_sibling->closing_element != NULL )
				{
					GlobalFree( del_sibling->closing_element );
				}

				GlobalFree( del_sibling );
			}
		}
	}
}

StackTree *GetElement( StackTree *tree, char *element_name, int element_name_length )
{
	// Go through the current level of the tree.
	while ( tree != NULL && tree->element_name != NULL )
	{
		// And see if any of the element match our parameters.
		if ( _StrCmpNIA( tree->element_name, element_name, element_name_length ) == 0 )
		{
			return tree;	// Return that element if it does match.
		}

		tree = tree->sibling;
	}

	return NULL;
}

StackTree *GetElementValue( StackTree *tree, char *element_name, int element_name_length, char **element_value = 0, int *element_value_length = 0, bool return_element_value = true )
{
	// Go through the current level of the tree.
	while ( tree != NULL && tree->element_name != NULL )
	{
		// And see if any of the elements match our parameters.
		if ( _StrCmpNIA( tree->element_name, element_name, element_name_length ) == 0 )
		{
			// Make sure that element has a closing element.
			if ( tree->closing_element != NULL && tree->closing_element->element_name != NULL )
			{
				// Offset the start and end of our element value.
				char *element_start = tree->element_name + tree->element_length + 1;
				char *element_end = tree->closing_element->element_name - 1;

				// Make sure the length is correct.
				if ( element_end > element_start )
				{
					int value_length = element_end - element_start;

					if ( element_value_length != NULL && return_element_value == true )
					{
						*element_value_length = value_length;
					}

					if ( element_value != NULL && return_element_value == true )
					{
						*element_value = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( value_length + 1 ) );
						_memcpy_s( *element_value, sizeof( char ) * ( value_length + 1 ), element_start, value_length + 1 );
						*( *element_value + value_length ) = 0;	// Sanity
					}

					return tree;
				}
			}

			break;
		}

		tree = tree->sibling;
	}

	return NULL;
}

StackTree *GetElementAttributeValue( StackTree *tree, char *element_name, int element_name_length, char *attribute_name, int attribute_name_length, char **attribute_value = 0, int *attribute_value_length = 0, bool return_attribute_value = true )
{
	while ( tree != NULL && tree->element_name != NULL )
	{
		if ( _StrCmpNIA( tree->element_name, element_name, element_name_length ) == 0 )
		{
			char *element_end = tree->element_name + tree->element_length;
			char *element_start = tree->element_name;

			char *find_attribute = tree->element_name + tree->element_name_length;

			// Locate each attribute.
			while ( find_attribute < element_end )
			{
				// Each attribute will begin with an "=".
				find_attribute = _StrChrA( find_attribute, '=' );
				if ( find_attribute == NULL )
				{
					return NULL;
				}
				else if ( find_attribute >= element_end )	// Missing attribute.
				{
					// If the attribute value length is -1, then we supplied a dummy value. Return the element that we last processed.
					if ( attribute_value_length != NULL && *attribute_value_length == -1 )
					{
						return tree;
					}

					// Go to the next element.
					break;
				}
				++find_attribute;

				// Skip whitespace that could appear after the attribute name, but before the "=".
				char *go_back = find_attribute - 1;
				while ( ( go_back - 1 ) >= ( element_start + attribute_name_length ) )
				{
					if ( *( go_back - 1 ) != ' ' && *( go_back - 1 ) != '\t' && *( go_back - 1 ) != '\r' && *( go_back - 1 ) != '\n' )	// Different whitespace characters for XML
					{
						break;
					}

					--go_back;
				}

				if ( _StrCmpNIA( go_back - attribute_name_length, attribute_name, attribute_name_length ) == 0 )	// We found the attribute.
				{
					// Skip whitespace that could appear after the "=", but before the attribute value.
					while ( find_attribute < element_end )
					{
						if ( find_attribute[ 0 ] != ' ' && find_attribute[ 0 ] != '\t' && find_attribute[ 0 ] != '\r' && find_attribute[ 0 ] != '\n' )	// Different whitespace characters for XML
						{
							break;
						}

						++find_attribute;
					}

					if ( find_attribute != NULL )
					{
						char delimiter = find_attribute[ 0 ];
						char *attribute_end = element_end;

						if ( delimiter == '\"' || delimiter == '\'' )
						{
							++find_attribute;

							attribute_end = _StrChrA( find_attribute, delimiter );	// Find the end of the ID.
							if ( attribute_end == NULL || attribute_end >= element_end )
							{
								return NULL;
							}
						}
						else	// No delimiter.
						{
							// Let's see if it ends with a space.
							attribute_end = _StrChrA( find_attribute, ' ' );
							if ( attribute_end == NULL || attribute_end >= element_end )
							{
								// If not, then it ends before the end of the element.
								attribute_end = element_end;
							}
						}

						int attribute_length = attribute_end - find_attribute;

						// If we supplied an attribute value, then we'll confirm its existence.
						if ( ( attribute_value != NULL && *attribute_value != NULL && attribute_value_length != NULL ) && \
							 ( attribute_length != *attribute_value_length || _StrCmpNIA( find_attribute, *attribute_value, attribute_length ) != 0 ) )	// See if either the lengths or strings don't match.
						{	
							break;	// Attribute value did not match.
						}
						
						if ( attribute_value_length != NULL && return_attribute_value == true )
						{
							*attribute_value_length = attribute_length;
						}

						if ( attribute_value != NULL && return_attribute_value == true )
						{
							*attribute_value = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( attribute_length + 1 ) );
							_memcpy_s( *attribute_value, sizeof( char ) * ( attribute_length + 1 ), find_attribute, attribute_length + 1 );
							*( *attribute_value + attribute_length ) = 0;	// Sanity
						}

						return tree;
					}
				}
			}
		}

		tree = tree->sibling;
	}

	return NULL;
}

bool GetAccountInformation( char *xml, char **client_id, char **account_id, char **account_status, char **account_type, char **principal_id, char **service_type, char **service_status, char **service_context, char ***service_phone_number, char **service_privacy_value, char ***service_notifications, char ***service_features, int &phone_lines )
{
	bool status = false;

	StackTree *xml_tree = BuildXMLTree( xml );

	StackTree *rtree = GetElement( xml_tree, "Registration", 12 );

	if ( rtree != NULL && rtree->child != NULL )
	{
		rtree = rtree->child;

		GetElementAttributeValue( rtree, "Client", 6, "id", 2, client_id );
		GetElementAttributeValue( rtree, "Principal", 9, "id", 2, principal_id );

		StackTree *stree = GetElementAttributeValue( rtree, "Account", 7, "id", 2, account_id );

		if ( stree != NULL && stree->child != NULL )
		{
			stree = stree->child;

			GetElementValue( stree, "Type", 4, account_type );
			GetElementValue( stree, "Status", 6, account_status );

			char *stype = "DigitalPhone";
			int stlength = 12;
			stree = GetElementAttributeValue( stree, "Service", 7, "type", 4, &stype, &stlength );	// This essentially confirms the value of an attribute.
			*service_type = stype;

			if ( stree != NULL && stree->child != NULL )
			{
				stree = stree->child;

				GetElementAttributeValue( stree, "Privacy", 7, "value", 5, service_privacy_value );
				GetElementValue( stree, "Context", 7, service_context );
				GetElementValue( stree, "Status", 6, service_status );

				/*stree = GetElement( stree, "PhoneNumber", 11 );

				if ( stree != NULL && stree->child != NULL )
				{
					stree = stree->child;

					GetElementValue( stree, "Number", 6, service_phone_number );
				}*/
			}
		}

		char *sidtype = "telephone";
		int sidtlength = 9;

		// Count the number of phone lines associated with this account.
		stree = rtree;
		while ( true )
		{
			stree = GetElementAttributeValue( stree, "Service", 7, "idType", 6, &sidtype, &sidtlength, false );	// Attribute is not returned.
			if ( stree != NULL )
			{
				++phone_lines;

				stree = stree->sibling;
			}
			else
			{
				break;
			}
		}

		if ( phone_lines > 0 )
		{
			// These are arrays of char strings.
			*service_phone_number = ( char ** )GlobalAlloc( GPTR, sizeof( char * ) * phone_lines );
			*service_notifications = ( char ** )GlobalAlloc( GPTR, sizeof( char * ) * phone_lines );
			*service_features = ( char ** )GlobalAlloc( GPTR, sizeof( char * ) * phone_lines );

			// Process the service information in reverse order.
			for ( int line = phone_lines - 1; line >= 0; --line )
			{
				stree = rtree = GetElementAttributeValue( rtree, "Service", 7, "idType", 6, &sidtype, &sidtlength, false );	// Attribute is not returned.

				if ( stree != NULL )
				{
					GetElementAttributeValue( stree, "Service", 7, "id", 2, &( *service_phone_number )[ line ] );

					rtree = rtree->sibling;

					if ( stree->child != NULL )
					{
						stree = stree->child;

						StackTree *ftree = stree;	// Save this so we can search for features.

						int notification_length = 0;
						int notification_count = 0;
						int buffer_size = 8;
						char **notification_array = ( char ** )GlobalAlloc( GMEM_FIXED, sizeof( char * ) * buffer_size );

						for ( int i = 0; i < buffer_size; ++i )
						{
							char *notification = NULL;
							int nlength = 0;
							stree = GetElementAttributeValue( stree, "Notification", 12, "type", 4, &notification, &nlength );
							notification_length += nlength;

							if ( notification == NULL )
							{
								break;
							}
							else if ( i == buffer_size - 1 )
							{
								buffer_size += 8;
								char **realloc_array = ( char ** )GlobalReAlloc( notification_array, sizeof( char * ) * buffer_size, GMEM_MOVEABLE );
								if ( realloc_array == NULL )
								{
									break;
								}

								notification_array = realloc_array;
							}

							// Determine the status of the notification.
							if ( stree != NULL && stree->child != NULL )
							{
								StackTree *ttree = stree->child;

								// See if the child element's value is "available".
								char *stat = "available";
								int slength = 9;
								if ( GetElementValue( ttree, "Status", 6, &stat, &slength, false ) != NULL )	// Value is not returned.
								{
									notification_array[ notification_count ] = notification;
									++notification_count;
								}
							}

							if ( stree != NULL && stree->sibling != NULL )
							{
								stree = stree->sibling;
							}
							else
							{
								break;
							}
						}

						notification_array[ notification_count ] = NULL;	// Sanity.

						int offset = 0;
						int service_notification_length = ( notification_count * 43 ) + notification_length + 1;
						( *service_notifications )[ line ] = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * service_notification_length );

						// Process the notifications in reverse order.
						for ( int i = notification_count - 1; i >= 0; --i )
						{
							if ( notification_array[ i ] != NULL )
							{
								_memcpy_s( ( *service_notifications )[ line ] + offset, service_notification_length - offset, "<Notification operation=\"create\" type=\"", 39 );
								offset += 39;
								notification_length = lstrlenA( notification_array[ i ] );
								_memcpy_s( ( *service_notifications )[ line ] + offset, service_notification_length - offset, notification_array[ i ], notification_length );
								offset += notification_length;
								_memcpy_s( ( *service_notifications )[ line ] + offset, service_notification_length - offset, "\" />", 4 );
								offset += 4;

								GlobalFree( notification_array[ i ] );
							}
							else
							{
								break;
							}
						}

						*( ( *service_notifications )[ line ] + ( service_notification_length - 1 ) ) = 0;	// Sanity.

						GlobalFree( notification_array );

						int feature_length = 0;
						int feature_count = 0;
						buffer_size = 4;
						char **feature_array = ( char ** )GlobalAlloc( GMEM_FIXED, sizeof( char * ) * buffer_size );

						for ( int i = 0; i < buffer_size; ++i )
						{
							char *feature = NULL;
							int flength = 0;
							stree = GetElementAttributeValue( ftree, "Feature", 7, "type", 4, &feature, &flength );
							feature_length += flength;

							if ( feature == NULL )
							{
								break;
							}
							else if ( i == buffer_size - 1 )
							{
								buffer_size += 4;
								char **realloc_array = ( char ** )GlobalReAlloc( feature_array, sizeof( char * ) * buffer_size, GMEM_MOVEABLE );
								if ( realloc_array == NULL )
								{
									break;
								}

								feature_array = realloc_array;
							}

							feature_array[ feature_count ] = feature;
							++feature_count;

							if ( ftree != NULL && ftree->sibling != NULL )
							{
								ftree = ftree->sibling;
							}
							else
							{
								break;
							}
						}

						feature_array[ feature_count ] = NULL;	// Sanity.

						offset = 0;
						int service_feature_length = ( ( feature_count > 0 ? ( feature_count - 1 ) : feature_count ) * 2 ) + feature_length + 1;
						( *service_features )[ line ] = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * service_feature_length );

						// Process the features in reverse order.
						for ( int i = feature_count - 1; i >= 0; --i )
						{
							if ( feature_array[ i ] != NULL )
							{
								feature_length = lstrlenA( feature_array[ i ] );
								_memcpy_s( ( *service_features )[ line ] + offset, service_feature_length - offset, feature_array[ i ], feature_length );
								offset += feature_length;

								if ( i > 0 )
								{
									_memcpy_s( ( *service_features )[ line ] + offset, service_feature_length - offset, ", ", 2 );
									offset += 2;
								}

								GlobalFree( feature_array[ i ] );
							}
							else
							{
								break;
							}
						}

						*( ( *service_features )[ line ] + ( service_feature_length - 1 ) ) = 0;	// Sanity.

						GlobalFree( feature_array );

						status = true;
					}
				}
			}
		}
	}

	DeleteXMLTree( xml_tree );

	return status;
}

bool GetCallerIDInformation( char *xml, char **call_to, char **call_from, char **caller_id, char **call_reference_id )
{
	bool status = false;

	StackTree *xml_tree = BuildXMLTree( xml );

	// v1_5 format starts with this.
	StackTree *rtree = GetElement( xml_tree, "GenericMessage", 14 );

	if ( rtree != NULL && rtree->child != NULL )
	{
		rtree = rtree->child;

		GetElementValue( rtree, "CallReferenceId", 15, call_reference_id );
		GetElementAttributeValue( rtree, "From", 4, "id", 2, call_from );
		GetElementAttributeValue( rtree, "From", 4, "name", 4, caller_id );
		GetElementAttributeValue( rtree, "To", 2, "id", 2, call_to );

		decode_xml_entities( *caller_id );

		status = true;
	}

	DeleteXMLTree( xml_tree );

	return status;
}

bool GetContactEntryId( char *xml, char **contact_entry_id )
{
	bool status = false;

	StackTree *xml_tree = BuildXMLTree( xml );

	StackTree *rtree = GetElement( xml_tree, "UABContactList", 14 );

	if ( rtree != NULL && rtree->child != NULL )
	{
		rtree = rtree->child;

		if ( GetElementAttributeValue( rtree, "ContactEntry", 12, "id", 2, contact_entry_id ) != NULL )
		{
			status = true;
		}
	}

	DeleteXMLTree( xml_tree );

	return status;
}

bool GetContactListLocation( char *xml, char **contact_list_location )
{
	bool status = false;

	StackTree *xml_tree = BuildXMLTree( xml );

	StackTree *rtree = GetElement( xml_tree, "UABContactList", 14 );

	if ( rtree != NULL && rtree->child != NULL )
	{
		rtree = rtree->child;

		rtree = GetElement( rtree, "File", 4 );

		if ( rtree != NULL && rtree->child != NULL )
		{
			rtree = rtree->child;

			// Get the value of the location element.
			GetElementValue( rtree, "Location", 8, contact_list_location );

			status = true;
		}
	}

	DeleteXMLTree( xml_tree );

	return status;
}

bool GetMediaLocation( char *xml, char **picture_location )
{
	bool status = false;

	StackTree *xml_tree = BuildXMLTree( xml );

	StackTree *rtree = GetElement( xml_tree, "Media", 5 );

	if ( rtree != NULL && rtree->child != NULL )
	{
		rtree = rtree->child;

		// Get the value of the location element.
		GetElementValue( rtree, "Location", 8, picture_location );

		status = true;
	}

	DeleteXMLTree( xml_tree );

	return status;
}

void ParseImportResponse( char *xml, int &success_count, int &failure_count )
{
	StackTree *xml_tree = BuildXMLTree( xml );

	StackTree *rtree = GetElement( xml_tree, "UABContactList", 14 );

	if ( rtree != NULL && rtree->child != NULL )
	{
		rtree = rtree->child;

		rtree = GetElement( rtree, "File", 4 );

		if ( rtree != NULL && rtree->child != NULL )
		{
			rtree = rtree->child;

			// Get the value of the Successful element.
			char *c_success_count = NULL;
			GetElementValue( rtree, "Successful", 10, &c_success_count );

			char *c_failure_count = NULL;
			GetElementAttributeValue( rtree, "Failures", 8, "count", 5, &c_failure_count );

			if ( c_success_count != NULL )
			{
				success_count = _strtoul( c_success_count, NULL, 10 );

				GlobalFree( c_success_count );
			}

			if ( c_failure_count != NULL )
			{
				failure_count = _strtoul( c_failure_count, NULL, 10 );

				GlobalFree( c_failure_count );
			}
		}
	}

	DeleteXMLTree( xml_tree );
}

bool UpdatePictureLocations( char *xml )
{
	bool status = false;

	StackTree *xml_tree = BuildXMLTree( xml );

	StackTree *rtree = GetElement( xml_tree, "UABContactList", 14 );

	if ( rtree != NULL && rtree->child != NULL )
	{
		rtree = rtree->child;

		char *contact_entry_id = NULL;

		while ( rtree != NULL )
		{
			// Find each ContactEntry that has an ID.
			StackTree *stree = GetElementAttributeValue( rtree, "ContactEntry", 12, "id", 2, &contact_entry_id );

			// If the entry has a child (content element), then proceed.
			if ( ( stree != NULL && stree->child != NULL ) && contact_entry_id != NULL )
			{
				stree = stree->child;

				// Find the content with a picture type.
				char *value = "picture";
				int length = 7;
				StackTree *ttree = GetElementAttributeValue( stree, "Content", 7, "type", 4, &value, &length, false );	// Value is not returned.

				// Make sure it has a child (location element).
				if ( ttree != NULL && ttree->child != NULL )
				{
					ttree = ttree->child;

					// Get the value of the location element.
					char *picture_location = NULL;
					GetElementValue( ttree, "Location", 8, &picture_location );

					// If we got the location, then see if we can update it for the contact.
					if ( picture_location != NULL )
					{
						EnterCriticalSection( &pe_cs );
						// Find the contact in our contact list.
						contactinfo *ci = ( contactinfo * )dllrbt_find( contact_list, ( void * )contact_entry_id, true );

						// If the contact exits (it should).
						if ( ci != NULL )
						{
							// Then update its picture location.
							if ( ci->contact.picture_location != NULL )
							{
								GlobalFree( ci->contact.picture_location );
							}

							ci->contact.picture_location = picture_location;

							status = true;
						}
						else	// This shouldn't happen.
						{
							GlobalFree( picture_location );
						}
						LeaveCriticalSection( &pe_cs );
					}
				}
			}

			// We're done with the contact entry ID.
			GlobalFree( contact_entry_id );
			contact_entry_id = NULL;

			// Go to the next contact.
			rtree = rtree->sibling;
		}
	}

	DeleteXMLTree( xml_tree );

	return status;
}

// Update the Contact's element IDs.
bool UpdateConactInformationIDs( char *xml, contactinfo *ci )
{
	bool status = false;

	StackTree *xml_tree = BuildXMLTree( xml );

	StackTree *rtree = GetElement( xml_tree, "UABContactList", 14 );

	if ( rtree != NULL && rtree->child != NULL )
	{
		rtree = rtree->child;

		rtree = GetElement( rtree, "ContactEntry", 12 );

		if ( rtree != NULL && rtree->child != NULL )
		{
			rtree = rtree->child;

			char *value = "home";
			int length = 4;
			StackTree *ttree = GetElementAttributeValue( rtree, "Web", 3, "type", 4, &value, &length, false );	// Value is not returned.
			GetElementAttributeValue( ttree, "Web", 3, "id", 2, &ci->contact.web_page_id );

			value = "home";
			length = 4;
			ttree = GetElementAttributeValue( rtree, "Email", 4, "type", 4, &value, &length, false );	// Value is not returned.
			GetElementAttributeValue( ttree, "Email", 4, "id", 2, &ci->contact.email_address_id );

			value = "work";
			length = 4;
			ttree = GetElementAttributeValue( rtree, "PhoneNumber", 11, "type", 4, &value, &length, false );	// Value is not returned.
			GetElementAttributeValue( ttree, "PhoneNumber", 11, "id", 2, &ci->contact.work_phone_number_id );

			value = "other";
			length = 5;
			ttree = GetElementAttributeValue( rtree, "PhoneNumber", 11, "type", 4, &value, &length, false );	// Value is not returned.
			GetElementAttributeValue( ttree, "PhoneNumber", 11, "id", 2, &ci->contact.other_phone_number_id );

			// The fax number has no type. Set the length to -1 to return the element it exits from.
			value = "fax";	// This is a dummy value.
			length = -1;
			ttree = GetElementAttributeValue( rtree, "PhoneNumber", 11, "type", 4, &value, &length, false );	// Value is not returned.
			GetElementAttributeValue( ttree, "PhoneNumber", 11, "id", 2, &ci->contact.fax_number_id );

			value = "cell";
			length = 4;
			ttree = GetElementAttributeValue( rtree, "PhoneNumber", 11, "type", 4, &value, &length, false );	// Value is not returned.
			GetElementAttributeValue( ttree, "PhoneNumber", 11, "id", 2, &ci->contact.cell_phone_number_id );

			value = "office";
			length = 6;
			ttree = GetElementAttributeValue( rtree, "PhoneNumber", 11, "type", 4, &value, &length, false );	// Value is not returned.
			GetElementAttributeValue( ttree, "PhoneNumber", 11, "id", 2, &ci->contact.office_phone_number_id );

			value = "home";
			length = 4;
			ttree = GetElementAttributeValue( rtree, "PhoneNumber", 11, "type", 4, &value, &length, false );	// Value is not returned.
			GetElementAttributeValue( ttree, "PhoneNumber", 11, "id", 2, &ci->contact.home_phone_number_id );

			status = true;
		}
	}

	DeleteXMLTree( xml_tree );

	return status;
}

// ci is not freed if it can't be added. The caller must do so.
bool UpdateContactList( char *xml, contactinfo *ci )
{
	bool status = false;

	if ( UpdateConactInformationIDs( xml, ci ) == true )
	{
		EnterCriticalSection( &pe_cs );
		if ( contact_list == NULL )
		{
			contact_list = dllrbt_create( dllrbt_compare );
		}

		// Caller must free ci.
		if ( dllrbt_insert( contact_list, ( void * )ci->contact.contact_entry_id, ( void * )ci ) == DLLRBT_STATUS_OK )
		{
			status = true;
		}
		LeaveCriticalSection( &pe_cs );
	}

	return status;
}

bool BuildContactList( char *xml )
{
	bool status = false;

	StackTree *xml_tree = BuildXMLTree( xml );

	char *items = NULL;
	StackTree *rtree = GetElementAttributeValue( xml_tree, "UABContactList", 14, "items", 5, &items );

	// Hopefully this matches the number of entries we parse.
	unsigned int item_count = 0;
	if ( items != NULL )
	{
		item_count = _strtoul( items, NULL, 10 );

		GlobalFree( items );
	}

	if ( rtree != NULL && rtree->child != NULL )
	{
		rtree = rtree->child;

		while ( rtree != NULL )
		{
			contactinfo *ci = ( contactinfo * )GlobalAlloc( GMEM_FIXED, sizeof( contactinfo ) );
			initialize_contactinfo( &ci );

			StackTree *stree = GetElementAttributeValue( rtree, "ContactEntry", 12, "id", 2, &ci->contact.contact_entry_id );

			if ( stree != NULL && stree->child != NULL )
			{
				stree = stree->child;

				GetElementValue( stree, "RingTone", 8, &ci->contact.ringtone );

				char *value = "home";
				int length = 4;
				StackTree *ttree = GetElementAttributeValue( stree, "Web", 3, "type", 4, &value, &length, false );	// Value is not returned.
				ttree = GetElementAttributeValue( ttree, "Web", 3, "id", 2, &ci->contact.web_page_id );

				if ( ttree != NULL && ttree->child != NULL )
				{
					ttree = ttree->child;

					GetElementValue( ttree, "Url", 3, &ci->contact.web_page );
				}

				value = "home";
				length = 4;
				ttree = GetElementAttributeValue( stree, "Email", 5, "type", 4, &value, &length, false );	// Value is not returned.
				ttree = GetElementAttributeValue( ttree, "Email", 5, "id", 2, &ci->contact.email_address_id );

				if ( ttree != NULL && ttree->child != NULL )
				{
					ttree = ttree->child;

					GetElementValue( ttree, "Address", 7, &ci->contact.email_address );
				}

				// The fax number has no type. Set the length to -1 to return the element it exits from.
				value = "fax";	// This is a dummy value.
				length = -1;
				ttree = GetElementAttributeValue( stree, "PhoneNumber", 11, "type", 4, &value, &length, false );	// Value is not returned.
				ttree = GetElementAttributeValue( ttree, "PhoneNumber", 11, "id", 2, &ci->contact.fax_number_id );

				if ( ttree != NULL && ttree->child != NULL )
				{
					ttree = ttree->child;

					GetElementValue( ttree, "Number", 6, &ci->contact.fax_number );
				}

				value = "other";
				length = 5;
				ttree = GetElementAttributeValue( stree, "PhoneNumber", 11, "type", 4, &value, &length, false );	// Value is not returned.
				ttree = GetElementAttributeValue( ttree, "PhoneNumber", 11, "id", 2, &ci->contact.other_phone_number_id );

				if ( ttree != NULL && ttree->child != NULL )
				{
					ttree = ttree->child;

					GetElementValue( ttree, "Number", 6, &ci->contact.other_phone_number );
				}

				value = "cell";
				length = 4;
				ttree = GetElementAttributeValue( stree, "PhoneNumber", 11, "type", 4, &value, &length, false );	// Value is not returned.
				ttree = GetElementAttributeValue( ttree, "PhoneNumber", 11, "id", 2, &ci->contact.cell_phone_number_id );

				if ( ttree != NULL && ttree->child != NULL )
				{
					ttree = ttree->child;

					GetElementValue( ttree, "Number", 6, &ci->contact.cell_phone_number );
				}

				value = "office";
				length = 6;
				ttree = GetElementAttributeValue( stree, "PhoneNumber", 11, "type", 4, &value, &length, false );	// Value is not returned.
				ttree = GetElementAttributeValue( ttree, "PhoneNumber", 11, "id", 2, &ci->contact.office_phone_number_id );

				if ( ttree != NULL && ttree->child != NULL )
				{
					ttree = ttree->child;

					GetElementValue( ttree, "Number", 6, &ci->contact.office_phone_number );
				}

				value = "work";
				length = 4;
				ttree = GetElementAttributeValue( stree, "PhoneNumber", 11, "type", 4, &value, &length, false );	// Value is not returned.
				ttree = GetElementAttributeValue( ttree, "PhoneNumber", 11, "id", 2, &ci->contact.work_phone_number_id );

				if ( ttree != NULL && ttree->child != NULL )
				{
					ttree = ttree->child;

					GetElementValue( ttree, "Number", 6, &ci->contact.work_phone_number );
				}

				value = "home";
				length = 4;
				ttree = GetElementAttributeValue( stree, "PhoneNumber", 11, "type", 4, &value, &length, false );	// Value is not returned.
				ttree = GetElementAttributeValue( ttree, "PhoneNumber", 11, "id", 2, &ci->contact.home_phone_number_id );

				if ( ttree != NULL && ttree->child != NULL )
				{
					ttree = ttree->child;

					GetElementValue( ttree, "Number", 6, &ci->contact.home_phone_number );
				}

				GetElementValue( stree, "Category", 8, &ci->contact.category );
				GetElementValue( stree, "Department", 10, &ci->contact.department );
				GetElementValue( stree, "Designation", 11, &ci->contact.designation );
				GetElementValue( stree, "BusinessName", 12, &ci->contact.business_name );

				GetElementValue( stree, "NickName", 8, &ci->contact.nickname );
				GetElementValue( stree, "LastName", 8, &ci->contact.last_name );
				GetElementValue( stree, "FirstName", 9, &ci->contact.first_name );

				GetElementValue( stree, "Title", 5, &ci->contact.title );

				decode_xml_entities( ci->contact.web_page );
				decode_xml_entities( ci->contact.email_address );

				decode_xml_entities( ci->contact.category );
				decode_xml_entities( ci->contact.department );
				decode_xml_entities( ci->contact.designation );
				decode_xml_entities( ci->contact.business_name );

				decode_xml_entities( ci->contact.nickname );
				decode_xml_entities( ci->contact.last_name );
				decode_xml_entities( ci->contact.first_name );

				decode_xml_entities( ci->contact.title );

				EnterCriticalSection( &pe_cs );
				if ( contact_list == NULL )
				{
					contact_list = dllrbt_create( dllrbt_compare );
				}

				if ( dllrbt_insert( contact_list, ( void * )ci->contact.contact_entry_id, ( void * )ci ) != DLLRBT_STATUS_OK )
				{
					free_contactinfo( &ci );
				}
				else
				{
					status = true;
				}
				LeaveCriticalSection( &pe_cs );
			}
			else
			{
				free_contactinfo( &ci );
			}

			rtree = rtree->sibling;
		}
	}

	DeleteXMLTree( xml_tree );

	return status;
}
