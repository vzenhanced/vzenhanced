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
#include "utilities.h"
#include "menus.h"
#include "string_tables.h"
#include "lite_gdi32.h"
#include "connection.h"

#include "web_server.h"

HANDLE worker_mutex = NULL;				// Blocks shutdown while a worker thread is active.
bool kill_worker_thead = false;			// Allow for a clean shutdown.

CRITICAL_SECTION pe_cs;					// Queues additional worker threads.

bool in_worker_thread = false;			// Flag to indicate that we're in a worker thread.
bool skip_log_draw = false;				// Prevents WM_DRAWITEM from accessing listview items while we're removing them.
bool skip_contact_draw = false;
bool skip_ignore_draw = false;
bool skip_forward_draw = false;

dllrbt_tree *call_log = NULL;
bool call_log_changed = false;

wchar_t *cfg_username = NULL;
wchar_t *cfg_password = NULL;
bool cfg_remember_login = false;
int cfg_pos_x = 0;
int cfg_pos_y = 0;
int cfg_width = MIN_WIDTH;
int cfg_height = MIN_HEIGHT;

bool cfg_tray_icon = true;
bool cfg_close_to_tray = false;
bool cfg_minimize_to_tray = false;
bool cfg_silent_startup = false;

bool cfg_always_on_top = false;

bool cfg_enable_call_log_history = false;

bool cfg_check_for_updates = false;

bool cfg_enable_popups = false;
bool cfg_popup_hide_border = false;
int cfg_popup_width = MIN_WIDTH / 2;
int cfg_popup_height = MIN_HEIGHT / 3;
unsigned char cfg_popup_position = 3;		// 0 = Top Left, 1 = Top Right, 2 = Bottom Left, 3 = Bottom Right

unsigned short cfg_popup_time = 10;			// Time in seconds.
unsigned char cfg_popup_transparency = 255;	// 255 = opaque
COLORREF cfg_popup_background_color1 = RGB( 255, 255, 255 );

bool cfg_popup_gradient = false;
unsigned char cfg_popup_gradient_direction = 0;	// 0 = Vertical, 1 = Horizontal
COLORREF cfg_popup_background_color2 = RGB( 255, 255, 255 );

wchar_t *cfg_popup_font_face1 = NULL;
wchar_t *cfg_popup_font_face2 = NULL;
wchar_t *cfg_popup_font_face3 = NULL;
COLORREF cfg_popup_font_color1 = RGB( 0, 0, 0 );
COLORREF cfg_popup_font_color2 = RGB( 0, 0, 0 );
COLORREF cfg_popup_font_color3 = RGB( 0, 0, 0 );
LONG cfg_popup_font_height1 = 16;
LONG cfg_popup_font_height2 = 16;
LONG cfg_popup_font_height3 = 16;
LONG cfg_popup_font_weight1 = FW_NORMAL;
LONG cfg_popup_font_weight2 = FW_NORMAL;
LONG cfg_popup_font_weight3 = FW_NORMAL;
BYTE cfg_popup_font_italic1 = FALSE;
BYTE cfg_popup_font_italic2 = FALSE;
BYTE cfg_popup_font_italic3 = FALSE;
BYTE cfg_popup_font_underline1 = FALSE;
BYTE cfg_popup_font_underline2 = FALSE;
BYTE cfg_popup_font_underline3 = FALSE;
BYTE cfg_popup_font_strikeout1 = FALSE;
BYTE cfg_popup_font_strikeout2 = FALSE;
BYTE cfg_popup_font_strikeout3 = FALSE;
bool cfg_popup_font_shadow1 = false;
bool cfg_popup_font_shadow2 = false;
bool cfg_popup_font_shadow3 = false;
COLORREF cfg_popup_font_shadow_color1 = RGB( 0, 0, 0 );
COLORREF cfg_popup_font_shadow_color2 = RGB( 0, 0, 0 );
COLORREF cfg_popup_font_shadow_color3 = RGB( 0, 0, 0 );
unsigned char cfg_popup_justify_line1 = 2;	// 0 = Left, 1 = Center, 2 = Right
unsigned char cfg_popup_justify_line2 = 0;	// 0 = Left, 1 = Center, 2 = Right
unsigned char cfg_popup_justify_line3 = 0;	// 0 = Left, 1 = Center, 2 = Right
char cfg_popup_line_order1 = LINE_TIME;
char cfg_popup_line_order2 = LINE_CALLER_ID;
char cfg_popup_line_order3 = LINE_PHONE;
unsigned char cfg_popup_time_format = 0;	// 0 = 12 hour, 1 = 24 hour

bool cfg_popup_play_sound = false;
wchar_t *cfg_popup_sound = NULL;

char cfg_tab_order1 = 0;		// 0 Call Log
char cfg_tab_order2 = 1;		// 1 Contact List
char cfg_tab_order3 = 2;		// 2 Forward List
char cfg_tab_order4 = 3;		// 3 Ignore List

int cfg_column_width1 = 35;
int cfg_column_width2 = 100;
int cfg_column_width3 = 205;
int cfg_column_width4 = 70;
int cfg_column_width5 = 100;
int cfg_column_width6 = 70;
int cfg_column_width7 = 100;
int cfg_column_width8 = 300;
int cfg_column_width9 = 100;

// Column / Virtual position
char cfg_column_order1 = 0;		// 0 # (always 0)
char cfg_column_order2 = 1;		// 1 Caller ID
char cfg_column_order3 = 3;		// 2 Date and Time
char cfg_column_order4 = -1;	// 3 Forwarded
char cfg_column_order5 = -1;	// 4 Forward to Phone Number
char cfg_column_order6 = -1;	// 5 Ignored
char cfg_column_order7 = 2;		// 6 Phone Number
char cfg_column_order8 = -1;	// 7 Reference
char cfg_column_order9 = -1;	// 8 Sent to Phone Number

int cfg_column2_width1 = 35;
int cfg_column2_width2 = 120;
int cfg_column2_width3 = 100;
int cfg_column2_width4 = 100;
int cfg_column2_width5 = 150;
int cfg_column2_width6 = 120;
int cfg_column2_width7 = 100;
int cfg_column2_width8 = 120;
int cfg_column2_width9 = 100;
int cfg_column2_width10 = 100;
int cfg_column2_width11 = 100;
int cfg_column2_width12 = 120;
int cfg_column2_width13 = 120;
int cfg_column2_width14 = 100;
int cfg_column2_width15 = 70;
int cfg_column2_width16 = 300;
int cfg_column2_width17 = 120;

char cfg_column2_order1 = 0;	// 0 # (always 0)
char cfg_column2_order2 = 14;	// 1 Cell Phone
char cfg_column2_order3 = 6;	// 2 Company
char cfg_column2_order4 = 9;	// 3 Department
char cfg_column2_order5 = 10;	// 4 Email Address
char cfg_column2_order6 = 7;	// 5 Fax Number
char cfg_column2_order7 = 1;	// 6 First Name
char cfg_column2_order8 = 13;	// 7 Home Phone
char cfg_column2_order9 = 8;	// 8 Job Title
char cfg_column2_order10 = 2;	// 9 Last Name
char cfg_column2_order11 = 16;	// 10 Nickname
char cfg_column2_order12 = 3;	// 11 Office Phone
char cfg_column2_order13 = 11;	// 12 Other Phone
char cfg_column2_order14 = 12;	// 13 Profession
char cfg_column2_order15 = 5;	// 14 Title
char cfg_column2_order16 = 4;	// 15 Web Page
char cfg_column2_order17 = 15;	// 16 Work Phone

int cfg_column3_width1 = 35;
int cfg_column3_width2 = 100;
int cfg_column3_width3 = 100;
int cfg_column3_width4 = 70;

char cfg_column3_order1 = 0;	// 0 # (always 0)
char cfg_column3_order2 = 2;	// 1 Forward to
char cfg_column3_order3 = 1;	// 2 Phone Number
char cfg_column3_order4 = 3;	// 3 Total Calls

int cfg_column4_width1 = 35;
int cfg_column4_width2 = 100;
int cfg_column4_width3 = 70;

char cfg_column4_order1 = 0;	// 0 # (always 0)
char cfg_column4_order2 = 1;	// 1 Phone Number
char cfg_column4_order3 = 2;	// 2 Total Calls



bool cfg_connection_auto_login = false;
bool cfg_connection_reconnect = true;
bool cfg_download_pictures = true;
unsigned char cfg_connection_retries = 3;

unsigned short cfg_connection_timeout = 15;	// Seconds.

unsigned char cfg_connection_ssl_version = 2;	// TLS 1.0

char *tab_order[ NUM_TABS ] = { &cfg_tab_order1, &cfg_tab_order2, &cfg_tab_order3, &cfg_tab_order4 };

char *list_columns[ NUM_COLUMNS ] =			 { &cfg_column_order1, &cfg_column_order2, &cfg_column_order3, &cfg_column_order4, &cfg_column_order5, &cfg_column_order6, &cfg_column_order7, &cfg_column_order8, &cfg_column_order9 };
char *contact_list_columns[ NUM_COLUMNS2 ] = { &cfg_column2_order1, &cfg_column2_order2, &cfg_column2_order3, &cfg_column2_order4, &cfg_column2_order5, &cfg_column2_order6, &cfg_column2_order7, &cfg_column2_order8, &cfg_column2_order9,
											   &cfg_column2_order10, &cfg_column2_order11, &cfg_column2_order12, &cfg_column2_order13, &cfg_column2_order14, &cfg_column2_order15, &cfg_column2_order16, &cfg_column2_order17 };
char *forward_list_columns[ NUM_COLUMNS3 ] = { &cfg_column3_order1, &cfg_column3_order2, &cfg_column3_order3, &cfg_column3_order4 };
char *ignore_list_columns[ NUM_COLUMNS4 ] =  { &cfg_column4_order1, &cfg_column4_order2, &cfg_column4_order3 };


int *list_columns_width[ NUM_COLUMNS ] =		  { &cfg_column_width1, &cfg_column_width2, &cfg_column_width3, &cfg_column_width4, &cfg_column_width5, &cfg_column_width6, &cfg_column_width7, &cfg_column_width8, &cfg_column_width9 };
int *contact_list_columns_width[ NUM_COLUMNS2 ] = { &cfg_column2_width1, &cfg_column2_width2, &cfg_column2_width3, &cfg_column2_width4, &cfg_column2_width5, &cfg_column2_width6, &cfg_column2_width7, &cfg_column2_width8, &cfg_column2_width9,
													&cfg_column2_width10, &cfg_column2_width11, &cfg_column2_width12, &cfg_column2_width13, &cfg_column2_width14, &cfg_column2_width15, &cfg_column2_width16, &cfg_column2_width17 };
int *forward_list_columns_width[ NUM_COLUMNS3 ] = { &cfg_column3_width1, &cfg_column3_width2, &cfg_column3_width3, &cfg_column3_width4 };
int *ignore_list_columns_width[ NUM_COLUMNS4 ] =  { &cfg_column4_width1, &cfg_column4_width2, &cfg_column4_width3 };

#define ROTATE_LEFT( x, n ) ( ( ( x ) << ( n ) ) | ( ( x ) >> ( 8 - ( n ) ) ) )
#define ROTATE_RIGHT( x, n ) ( ( ( x ) >> ( n ) ) | ( ( x ) << ( 8 - ( n ) ) ) )

void encode_cipher( char *buffer, int buffer_length )
{
	int offset = buffer_length + 128;
	for ( int i = 0; i < buffer_length; ++i )
	{
		*buffer ^= ( unsigned char )buffer_length;
		*buffer = ( *buffer + offset ) % 256;
		*buffer = ROTATE_LEFT( ( unsigned char )*buffer, offset % 8 );

		buffer++;
		--offset;
	}
}

void decode_cipher( char *buffer, int buffer_length )
{
	int offset = buffer_length + 128;
	for ( int i = buffer_length; i > 0; --i )
	{
		*buffer = ROTATE_RIGHT( ( unsigned char )*buffer, offset % 8 );
		*buffer = ( *buffer - offset ) % 256;
		*buffer ^= ( unsigned char )buffer_length;

		buffer++;
		--offset;
	}
}

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

char is_num( const char *str )
{
	if ( str == NULL )
	{
		return -1;
	}

	unsigned char *s = ( unsigned char * )str;

	char ret = 0;	// 0 = regular number, 1 = number with wildcard(s), -1 = not a number.

	if ( *s != NULL && *s == '+' )
	{
		*s++;
	}

	while ( *s != NULL )
	{
		if ( *s == '*' )
		{
			ret = 1;
		}
		else if ( !is_digit( *s ) )
		{
			return -1;
		}

		*s++;
	}

	return ret;
}

/*
char from_hex( char ch )
{
	return is_digit( ch ) ? ch - '0' : ( char )_CharUpperA( ( LPSTR )ch ) - 'A' + 10;
}
*/
char to_hex( char code )
{
	static char hex[] = "0123456789ABCDEF";
	return hex[ code & 15 ];
}

char *url_encode( char *str, unsigned int str_len, unsigned int *enc_len )
{
	char *pstr = str;
	char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( ( str_len * 3 ) + 1 ) );
	char *pbuf = buf;

	while ( pstr < ( str + str_len ) )
	{
		if ( _IsCharAlphaNumericA( *pstr ) )
		{
			*pbuf++ = *pstr;
		}
		else if ( *pstr == ' ' )
		{
			*pbuf++ = '+';
		}
		else
		{
			*pbuf++ = '%';
			*pbuf++ = to_hex( *pstr >> 4 );
			*pbuf++ = to_hex( *pstr & 15 );
		}

		pstr++;
	}

	*pbuf = '\0';

	if ( enc_len != NULL )
	{
		*enc_len = pbuf - buf;
	}

	return buf;
}

wchar_t *GetMonth( unsigned short month )
{
	if ( month > 12 || month < 1 )
	{
		return L"";
	}

	return month_string_table[ month - 1 ];
}

wchar_t *GetDay( unsigned short day )
{
	if ( day > 6 )
	{
		return L"";
	}

	return day_string_table[ day ];
}

void free_displayinfo( displayinfo **di )
{
	// Free the strings in our info structure.
	GlobalFree( ( *di )->ci.call_from );
	GlobalFree( ( *di )->ci.call_reference_id );
	GlobalFree( ( *di )->ci.call_to );
	GlobalFree( ( *di )->ci.caller_id );
	GlobalFree( ( *di )->ci.forward_to );
	GlobalFree( ( *di )->phone_number );
	GlobalFree( ( *di )->forward_to );
	GlobalFree( ( *di )->caller_id );
	GlobalFree( ( *di )->reference );
	GlobalFree( ( *di )->sent_to );
	GlobalFree( ( *di )->w_forward );
	GlobalFree( ( *di )->w_ignore );
	GlobalFree( ( *di )->w_time );
	// Then free the displayinfo structure.
	GlobalFree( *di );
	*di = NULL;
}

void initialize_contactinfo( contactinfo **ci )
{
	( *ci )->contact.cell_phone_number = NULL;
	( *ci )->contact.cell_phone_number_id = NULL;
	( *ci )->contact.contact_entry_id = NULL;
	( *ci )->contact.first_name = NULL;
	( *ci )->contact.home_phone_number = NULL;
	( *ci )->contact.home_phone_number_id = NULL;
	( *ci )->contact.last_name = NULL;
	( *ci )->contact.nickname = NULL;
	( *ci )->contact.office_phone_number = NULL;
	( *ci )->contact.office_phone_number_id = NULL;
	( *ci )->contact.other_phone_number = NULL;
	( *ci )->contact.other_phone_number_id = NULL;
	( *ci )->contact.ringtone = NULL;

	( *ci )->contact.title = NULL;
	( *ci )->contact.business_name = NULL;
	( *ci )->contact.designation = NULL;
	( *ci )->contact.department = NULL;
	( *ci )->contact.category = NULL;
	( *ci )->contact.work_phone_number_id = NULL;
	( *ci )->contact.work_phone_number = NULL;
	( *ci )->contact.fax_number_id = NULL;
	( *ci )->contact.fax_number = NULL;
	( *ci )->contact.email_address_id = NULL;
	( *ci )->contact.email_address = NULL;
	( *ci )->contact.web_page_id = NULL;
	( *ci )->contact.web_page = NULL;

	( *ci )->contact.picture_location = NULL;

	( *ci )->first_name = NULL;
	( *ci )->last_name = NULL;
	( *ci )->nickname = NULL;
	( *ci )->home_phone_number = NULL;
	( *ci )->cell_phone_number = NULL;
	( *ci )->office_phone_number = NULL;
	( *ci )->other_phone_number = NULL;

	( *ci )->title = NULL;
	( *ci )->business_name = NULL;
	( *ci )->designation = NULL;
	( *ci )->department = NULL;
	( *ci )->category = NULL;
	( *ci )->work_phone_number = NULL;
	( *ci )->fax_number = NULL;
	( *ci )->email_address = NULL;
	( *ci )->web_page = NULL;

	( *ci )->picture_path = NULL;

	( *ci )->displayed = false;	// Not displayed yet.
}

void free_contactinfo( contactinfo **ci )
{
	// Free the strings in our info structure.
	GlobalFree( ( *ci )->contact.cell_phone_number );
	GlobalFree( ( *ci )->contact.cell_phone_number_id );
	GlobalFree( ( *ci )->contact.contact_entry_id );
	GlobalFree( ( *ci )->contact.first_name );
	GlobalFree( ( *ci )->contact.home_phone_number );
	GlobalFree( ( *ci )->contact.home_phone_number_id );
	GlobalFree( ( *ci )->contact.last_name );
	GlobalFree( ( *ci )->contact.nickname );
	GlobalFree( ( *ci )->contact.office_phone_number );
	GlobalFree( ( *ci )->contact.office_phone_number_id );
	GlobalFree( ( *ci )->contact.other_phone_number );
	GlobalFree( ( *ci )->contact.other_phone_number_id );
	GlobalFree( ( *ci )->contact.ringtone );

	GlobalFree( ( *ci )->contact.title );
	GlobalFree( ( *ci )->contact.business_name );
	GlobalFree( ( *ci )->contact.designation );
	GlobalFree( ( *ci )->contact.department );
	GlobalFree( ( *ci )->contact.category );
	GlobalFree( ( *ci )->contact.work_phone_number_id );
	GlobalFree( ( *ci )->contact.work_phone_number );
	GlobalFree( ( *ci )->contact.fax_number_id );
	GlobalFree( ( *ci )->contact.fax_number );
	GlobalFree( ( *ci )->contact.email_address_id );
	GlobalFree( ( *ci )->contact.email_address );
	GlobalFree( ( *ci )->contact.web_page_id );
	GlobalFree( ( *ci )->contact.web_page );

	GlobalFree( ( *ci )->contact.picture_location );

	GlobalFree( ( *ci )->first_name );
	GlobalFree( ( *ci )->last_name );
	GlobalFree( ( *ci )->nickname );
	GlobalFree( ( *ci )->home_phone_number );
	GlobalFree( ( *ci )->cell_phone_number );
	GlobalFree( ( *ci )->office_phone_number );
	GlobalFree( ( *ci )->other_phone_number );

	GlobalFree( ( *ci )->title );
	GlobalFree( ( *ci )->business_name );
	GlobalFree( ( *ci )->designation );
	GlobalFree( ( *ci )->department );
	GlobalFree( ( *ci )->category );
	GlobalFree( ( *ci )->work_phone_number );
	GlobalFree( ( *ci )->fax_number );
	GlobalFree( ( *ci )->email_address );
	GlobalFree( ( *ci )->web_page );

	GlobalFree( ( *ci )->picture_path );

	// Then free the contactinfo structure.
	GlobalFree( *ci );
	*ci = NULL;
}

void free_forwardinfo( forwardinfo **fi )
{
	GlobalFree( ( *fi )->call_from );
	GlobalFree( ( *fi )->forward_to );
	GlobalFree( ( *fi )->total_calls );
	GlobalFree( ( *fi )->c_call_from );
	GlobalFree( ( *fi )->c_forward_to );
	GlobalFree( ( *fi )->c_total_calls );
	GlobalFree( *fi );
	*fi = NULL;
}

void free_ignoreinfo( ignoreinfo **ii )
{
	GlobalFree( ( *ii )->phone_number );
	GlobalFree( ( *ii )->total_calls );
	GlobalFree( ( *ii )->c_phone_number );
	GlobalFree( ( *ii )->c_total_calls );
	GlobalFree( *ii );
	*ii = NULL;
}

void OffsetVirtualIndices( int *arr, char *column_arr[], unsigned char num_columns, unsigned char total_columns )
{
	for ( unsigned char i = 0; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] == -1 )	// See which columns are disabled.
		{
			for ( unsigned char j = 0; j < total_columns; ++j )
			{
				if ( arr[ j ] >= i )	// Increment each virtual column that comes after the disabled column.
				{
					arr[ j ]++;
				}
			}
		}
	}
}

int GetVirtualIndexFromColumnIndex( int column_index, char *column_arr[], unsigned char num_columns )
{
	int count = 0;

	for ( int i = 0; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] != -1 )
		{
			if ( count == column_index )
			{
				return i;
			}
			else
			{
				++count;
			}
		}
	}

	return -1;
}

int GetColumnIndexFromVirtualIndex( int virtual_index, char *column_arr[], unsigned char num_columns )
{
	int count = 0;

	for ( int i = 0; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] != -1 && *column_arr[ i ] == virtual_index )
		{
			return count;
		}
		else if ( *column_arr[ i ] != -1 )
		{
			++count;
		}
	}

	return -1;
}

void UpdateColumnOrders()
{
	int arr[ 17 ];
	int offset = 0;
	_SendMessageW( g_hWnd_list, LVM_GETCOLUMNORDERARRAY, total_columns, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS; ++i )
	{
		if ( *list_columns[ i ] != -1 )
		{
			*list_columns[ i ] = ( char )arr[ offset++ ];
		}
	}

	offset = 0;
	_SendMessageW( g_hWnd_contact_list, LVM_GETCOLUMNORDERARRAY, total_columns2, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS2; ++i )
	{
		if ( *contact_list_columns[ i ] != -1 )
		{
			*contact_list_columns[ i ] = ( char )arr[ offset++ ];
		}
	}

	offset = 0;
	_SendMessageW( g_hWnd_forward_list, LVM_GETCOLUMNORDERARRAY, total_columns3, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS3; ++i )
	{
		if ( *forward_list_columns[ i ] != -1 )
		{
			*forward_list_columns[ i ] = ( char )arr[ offset++ ];
		}
	}

	offset = 0;
	_SendMessageW( g_hWnd_ignore_list, LVM_GETCOLUMNORDERARRAY, total_columns4, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS4; ++i )
	{
		if ( *ignore_list_columns[ i ] != -1 )
		{
			*ignore_list_columns[ i ] = ( char )arr[ offset++ ];
		}
	}
}

void SetDefaultColumnOrder( unsigned char list )
{
	switch ( list )
	{
		case 0:
		{
			cfg_column_order1 = 0;		// # (always 0)
			cfg_column_order2 = 1;		// Caller ID
			cfg_column_order3 = 3;		// Date and Time
			cfg_column_order4 = -1;		// Forwarded
			cfg_column_order5 = -1;		// Forward to Phone Number
			cfg_column_order6 = -1;		// Ignored
			cfg_column_order7 = 2;		// Phone Number
			cfg_column_order8 = -1;		// Reference
			cfg_column_order9 = -1;		// Sent to Phone Number
		}
		break;

		case 1:
		{
			cfg_column2_order1 = 0;		// 0 # (always 0)
			cfg_column2_order2 = 14;	// 1 Cell Phone
			cfg_column2_order3 = 6;		// 2 Company
			cfg_column2_order4 = 9;		// 3 Department
			cfg_column2_order5 = 10;	// 4 Email Address
			cfg_column2_order6 = 7;		// 5 Fax Number
			cfg_column2_order7 = 1;		// 6 First Name
			cfg_column2_order8 = 13;	// 7 Home Phone
			cfg_column2_order9 = 8;		// 8 Job Title
			cfg_column2_order10 = 2;	// 9 Last Name
			cfg_column2_order11 = 16;	// 10 Nickname
			cfg_column2_order12 = 3;	// 11 Office Phone
			cfg_column2_order13 = 11;	// 12 Other Phone
			cfg_column2_order14 = 12;	// 13 Profession
			cfg_column2_order15 = 5;	// 14 Title
			cfg_column2_order16 = 4;	// 15 Web Page
			cfg_column2_order17 = 15;	// 16 Work Phone
		}
		break;

		case 2:
		{
			cfg_column3_order1 = 0;		// 0 # (always 0)
			cfg_column3_order2 = 2;		// 1 Forward to
			cfg_column3_order3 = 1;		// 2 Phone Number
			cfg_column3_order4 = 3;		// 3 Total Calls
		}
		break;

		case 3:
		{
			cfg_column4_order1 = 0;		// 0 # (always 0)
			cfg_column4_order2 = 1;		// 1 Phone Number
			cfg_column4_order3 = 2;		// 2 Total Calls
		}
		break;
	}
}

void CheckColumnOrders( unsigned char list, char *column_arr[], unsigned char num_columns )
{
	// Make sure the first column is always 0 or -1.
	if ( *column_arr[ 0 ] > 0 || *column_arr[ 0 ] < -1 )
	{
		SetDefaultColumnOrder( list );
		return;
	}

	// Look for duplicates, or values that are out of range.
	unsigned char *is_set = ( unsigned char * )GlobalAlloc( GMEM_FIXED, sizeof( unsigned char ) * num_columns );
	_memzero( is_set, sizeof( unsigned char ) * num_columns );

	// Check ever other column.
	for ( int i = 1; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] != -1 )
		{
			if ( *column_arr[ i ] < num_columns )
			{
				if ( is_set[ *column_arr[ i ] ] == 0 )
				{
					is_set[ *column_arr[ i ] ] = 1;
				}
				else	// Revert duplicate values.
				{
					SetDefaultColumnOrder( list );

					break;
				}
			}
			else	// Revert out of range values.
			{
				SetDefaultColumnOrder( list );

				break;
			}
		}
	}

	GlobalFree( is_set );
}

void CheckColumnWidths()
{
	if ( cfg_column_width1 < 0 || cfg_column_width1 > 2560 ) { cfg_column_width1 = 35; }
	if ( cfg_column_width2 < 0 || cfg_column_width2 > 2560 ) { cfg_column_width2 = 100; }
	if ( cfg_column_width3 < 0 || cfg_column_width3 > 2560 ) { cfg_column_width3 = 205; }
	if ( cfg_column_width4 < 0 || cfg_column_width4 > 2560 ) { cfg_column_width4 = 70; }
	if ( cfg_column_width5 < 0 || cfg_column_width5 > 2560 ) { cfg_column_width5 = 100; }
	if ( cfg_column_width6 < 0 || cfg_column_width6 > 2560 ) { cfg_column_width6 = 70; }
	if ( cfg_column_width7 < 0 || cfg_column_width7 > 2560 ) { cfg_column_width7 = 100; }
	if ( cfg_column_width8 < 0 || cfg_column_width8 > 2560 ) { cfg_column_width8 = 300; }
	if ( cfg_column_width9 < 0 || cfg_column_width9 > 2560 ) { cfg_column_width9 = 100; }


	if ( cfg_column2_width1 < 0 || cfg_column2_width1 > 2560 ) { cfg_column2_width1 = 35; }
	if ( cfg_column2_width2 < 0 || cfg_column2_width2 > 2560 ) { cfg_column2_width2 = 120; }
	if ( cfg_column2_width3 < 0 || cfg_column2_width3 > 2560 ) { cfg_column2_width3 = 100; }
	if ( cfg_column2_width4 < 0 || cfg_column2_width4 > 2560 ) { cfg_column2_width4 = 100; }
	if ( cfg_column2_width5 < 0 || cfg_column2_width5 > 2560 ) { cfg_column2_width5 = 150; }
	if ( cfg_column2_width6 < 0 || cfg_column2_width6 > 2560 ) { cfg_column2_width6 = 120; }
	if ( cfg_column2_width7 < 0 || cfg_column2_width7 > 2560 ) { cfg_column2_width7 = 100; }
	if ( cfg_column2_width8 < 0 || cfg_column2_width8 > 2560 ) { cfg_column2_width8 = 120; }
	if ( cfg_column2_width9 < 0 || cfg_column2_width9 > 2560 ) { cfg_column2_width9 = 100; }
	if ( cfg_column2_width10 < 0 || cfg_column2_width10 > 2560 ) { cfg_column2_width10 = 100; }
	if ( cfg_column2_width11 < 0 || cfg_column2_width11 > 2560 ) { cfg_column2_width11 = 100; }
	if ( cfg_column2_width12 < 0 || cfg_column2_width12 > 2560 ) { cfg_column2_width12 = 120; }
	if ( cfg_column2_width13 < 0 || cfg_column2_width13 > 2560 ) { cfg_column2_width13 = 120; }
	if ( cfg_column2_width14 < 0 || cfg_column2_width14 > 2560 ) { cfg_column2_width14 = 100; }
	if ( cfg_column2_width15 < 0 || cfg_column2_width15 > 2560 ) { cfg_column2_width15 = 70; }
	if ( cfg_column2_width16 < 0 || cfg_column2_width16 > 2560 ) { cfg_column2_width16 = 300; }
	if ( cfg_column2_width17 < 0 || cfg_column2_width17 > 2560 ) { cfg_column2_width17 = 120; }

	if ( cfg_column3_width1 < 0 || cfg_column3_width1 > 2560 ) { cfg_column3_width1 = 35; }
	if ( cfg_column3_width2 < 0 || cfg_column3_width2 > 2560 ) { cfg_column3_width2 = 100; }
	if ( cfg_column3_width3 < 0 || cfg_column3_width3 > 2560 ) { cfg_column3_width3 = 100; }
	if ( cfg_column3_width4 < 0 || cfg_column3_width4 > 2560 ) { cfg_column3_width4 = 70; }

	if ( cfg_column4_width1 < 0 || cfg_column4_width1 > 2560 ) { cfg_column4_width1 = 35; }
	if ( cfg_column4_width2 < 0 || cfg_column4_width2 > 2560 ) { cfg_column4_width2 = 100; }
	if ( cfg_column4_width3 < 0 || cfg_column4_width3 > 2560 ) { cfg_column4_width3 = 70; }
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
	static char *image_type[ 4 ] = { "image/bmp", "image/gif", "image/jpeg", "image/png" };
	if ( filename != NULL )
	{
		char *extension = GetFileExtension( filename ) + 1;

		if ( extension != NULL )
		{
			if ( lstrcmpiA( extension, "jpg" ) == 0 )
			{
				return image_type[ 2 ];
			}
			else if ( lstrcmpiA( extension, "png" ) == 0 )
			{
				return image_type[ 3 ];
			}
			else if ( lstrcmpiA( extension, "gif" ) == 0 )
			{
				return image_type[ 1 ];
			}
			else if ( lstrcmpiA( extension, "bmp" ) == 0 )
			{
				return image_type[ 0 ];
			}
			else if ( lstrcmpiA( extension, "jpeg" ) == 0 )
			{
				return image_type[ 2 ];
			}
			else if ( lstrcmpiA( extension, "jpe" ) == 0 )
			{
				return image_type[ 2 ];
			}
			else if ( lstrcmpiA( extension, "jfif" ) == 0 )
			{
				return image_type[ 2 ];
			}
			else if ( lstrcmpiA( extension, "dib" ) == 0 )
			{
				return image_type[ 0 ];
			}
		}
	}
	
	return "application/unknown";
}

void CreatePopup( displayinfo *fi )
{
	RECT wa;
	_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );	

	int left = 0;
	int top = 0;

	if ( cfg_popup_position == 0 )		// Top Left
	{
		left = wa.left;
		top = wa.top;
	}
	else if ( cfg_popup_position == 1 )	// Top Right
	{
		left = wa.right - cfg_popup_width;
		top = wa.top;
	}
	else if ( cfg_popup_position == 2 )	// Bottom Left
	{
		left = wa.left;
		top = wa.bottom - cfg_popup_height;
	}
	else					// Bottom Right
	{
		left = wa.right - cfg_popup_width;
		top = wa.bottom - cfg_popup_height;
	}


	HWND hWnd_popup = _CreateWindowExW( WS_EX_NOACTIVATE | WS_EX_TOPMOST, L"popup", NULL, WS_CLIPCHILDREN | WS_POPUP | ( cfg_popup_hide_border == true ? NULL : WS_THICKFRAME ), left, top, cfg_popup_width, cfg_popup_height, NULL, NULL, NULL, NULL );

	_SetWindowLongW( hWnd_popup, GWL_EXSTYLE, _GetWindowLongW( hWnd_popup, GWL_EXSTYLE ) | WS_EX_LAYERED );
	_SetLayeredWindowAttributes( hWnd_popup, 0, cfg_popup_transparency, LWA_ALPHA );

	// ALL OF THIS IS FREED IN WM_DESTROY OF THE POPUP WINDOW.

	wchar_t *phone_line = GlobalStrDupW( fi->phone_number );;
	wchar_t *caller_id_line = GlobalStrDupW( fi->caller_id );
	wchar_t *time_line = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 16 );

	SYSTEMTIME st;
	FILETIME ft;
	ft.dwLowDateTime = fi->time.LowPart;
	ft.dwHighDateTime = fi->time.HighPart;
	
	FileTimeToSystemTime( &ft, &st );
	if ( cfg_popup_time_format == 0 )	// 12 hour
	{
		__snwprintf( time_line, 16, L"%d:%02d %s", ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, ( st.wHour >= 12 ? L"PM" : L"AM" ) );
	}
	else	// 24 hour
	{
		__snwprintf( time_line, 16, L"%s%d:%02d", ( st.wHour > 9 ? L"" : L"0" ), st.wHour, st.wMinute );
	}

	LOGFONT lf;
	_memzero( &lf, sizeof( LOGFONT ) );


	POPUP_SETTINGS *ls1 = ( POPUP_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( POPUP_SETTINGS ) );
	ls1->popup_line_order = cfg_popup_line_order1;
	ls1->popup_justify = cfg_popup_justify_line1;
	ls1->font_color = cfg_popup_font_color1;
	ls1->font_shadow = cfg_popup_font_shadow1;
	ls1->font_shadow_color = cfg_popup_font_shadow_color1;
	ls1->font_face = NULL;
	ls1->line_text = ( _abs( cfg_popup_line_order1 ) == LINE_TIME ? time_line : ( _abs( cfg_popup_line_order1 ) == LINE_CALLER_ID ? caller_id_line : phone_line ) );

	if ( cfg_popup_font_face1 != NULL )
	{
		_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, cfg_popup_font_face1, LF_FACESIZE );
		lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.
		lf.lfHeight = cfg_popup_font_height1;
		lf.lfWeight = cfg_popup_font_weight1;
		lf.lfItalic = cfg_popup_font_italic1;
		lf.lfUnderline = cfg_popup_font_underline1;
		lf.lfStrikeOut = cfg_popup_font_strikeout1;
		ls1->font = _CreateFontIndirectW( &lf );
	}
	else
	{
		ls1->font = NULL;
	}

	POPUP_SETTINGS *ls2 = ( POPUP_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( POPUP_SETTINGS ) );
	ls2->popup_line_order = cfg_popup_line_order2;
	ls2->popup_justify = cfg_popup_justify_line2;
	ls2->font_color = cfg_popup_font_color2;
	ls2->font_shadow = cfg_popup_font_shadow2;
	ls2->font_shadow_color = cfg_popup_font_shadow_color2;
	ls2->font_face = NULL;
	ls2->line_text = ( _abs( cfg_popup_line_order2 ) == LINE_TIME ? time_line : ( _abs( cfg_popup_line_order2 ) == LINE_CALLER_ID ? caller_id_line : phone_line ) );

	if ( cfg_popup_font_face2 != NULL )
	{
		_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, cfg_popup_font_face2, LF_FACESIZE );
		lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.
		lf.lfHeight = cfg_popup_font_height2;
		lf.lfWeight = cfg_popup_font_weight2;
		lf.lfItalic = cfg_popup_font_italic2;
		lf.lfUnderline = cfg_popup_font_underline2;
		lf.lfStrikeOut = cfg_popup_font_strikeout2;
		ls2->font = _CreateFontIndirectW( &lf );
	}
	else
	{
		ls2->font = NULL;
	}

	POPUP_SETTINGS *ls3 = ( POPUP_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( POPUP_SETTINGS ) );
	ls3->popup_line_order = cfg_popup_line_order3;
	ls3->popup_justify = cfg_popup_justify_line3;
	ls3->font_color = cfg_popup_font_color3;
	ls3->font_shadow = cfg_popup_font_shadow3;
	ls3->font_shadow_color = cfg_popup_font_shadow_color3;
	ls3->font_face = NULL;
	ls3->line_text = ( _abs( cfg_popup_line_order3 ) == LINE_TIME ? time_line : ( _abs( cfg_popup_line_order3 ) == LINE_CALLER_ID ? caller_id_line : phone_line ) );

	if ( cfg_popup_font_face3 != NULL )
	{
		_wcsncpy_s( lf.lfFaceName, LF_FACESIZE, cfg_popup_font_face3, LF_FACESIZE );
		lf.lfFaceName[ LF_FACESIZE - 1 ] = 0;	// Sanity.
		lf.lfHeight = cfg_popup_font_height3;
		lf.lfWeight = cfg_popup_font_weight3;
		lf.lfItalic = cfg_popup_font_italic3;
		lf.lfUnderline = cfg_popup_font_underline3;
		lf.lfStrikeOut = cfg_popup_font_strikeout3;
		ls3->font = _CreateFontIndirectW( &lf );
	}
	else
	{
		ls3->font = NULL;
	}

	SHARED_SETTINGS *shared_settings = ( SHARED_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( SHARED_SETTINGS ) );
	shared_settings->popup_gradient = cfg_popup_gradient;
	shared_settings->popup_gradient_direction = cfg_popup_gradient_direction;
	shared_settings->popup_background_color1 = cfg_popup_background_color1;
	shared_settings->popup_background_color2 = cfg_popup_background_color2;
	shared_settings->popup_time = cfg_popup_time;
	shared_settings->popup_play_sound = cfg_popup_play_sound;
	shared_settings->popup_sound = GlobalStrDupW( cfg_popup_sound );


	DoublyLinkedList *t_ll = DLL_CreateNode( ( void * )ls1, ( void * )shared_settings );

	DoublyLinkedList *ll = DLL_CreateNode( ( void * )ls2, ( void * )shared_settings );
	DLL_AddNode( &t_ll, ll, -1 );

	ll = DLL_CreateNode( ( void * )ls3, ( void * )shared_settings );
	DLL_AddNode( &t_ll, ll, -1 );

	_SetWindowLongW( hWnd_popup, 0, ( LONG_PTR )t_ll );

	_SendMessageW( hWnd_popup, WM_PROPAGATE, 0, 0 );
}

void Processing_Window( HWND hWnd, bool enable )
{
	if ( enable == false )
	{
		//_SetWindowTextW( g_hWnd_main, L"VZ Enhanced - Please wait..." );	// Update the window title.
		_EnableWindow( hWnd, FALSE );										// Prevent any interaction with the listview while we're processing.
		_SendMessageW( g_hWnd_main, WM_CHANGE_CURSOR, TRUE, 0 );			// SetCursor only works from the main thread. Set it to an arrow with hourglass.
		UpdateMenus( UM_DISABLE );											// Disable all processing menu items.
	}
	else
	{
		UpdateMenus( UM_ENABLE );									// Enable the appropriate menu items.
		_SendMessageW( g_hWnd_main, WM_CHANGE_CURSOR, FALSE, 0 );	// Reset the cursor.
		_EnableWindow( hWnd, TRUE );								// Allow the listview to be interactive. Also forces a refresh to update the item count column.
		_SetFocus( hWnd );											// Give focus back to the listview to allow shortcut keys.
		//_SetWindowTextW( g_hWnd_main, PROGRAM_CAPTION );			// Reset the window title.
	}
}

void kill_worker_thread()
{
	if ( in_worker_thread == true )
	{
		// This mutex will be released when the thread gets killed.
		worker_mutex = CreateSemaphore( NULL, 0, 1, NULL );

		kill_worker_thead = true;	// Causes secondary threads to cease processing and release the mutex.

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( worker_mutex, 5000 );
		CloseHandle( worker_mutex );
		worker_mutex = NULL;
	}
}

// This will allow our main thread to continue while secondary threads finish their processing.
THREAD_RETURN cleanup( void *pArguments )
{
	kill_worker_thread();
	kill_connection_worker_thread();
	kill_connection_incoming_thread();
	kill_connection_thread();
	kill_update_check_thread();

	// DestroyWindow won't work on a window from a different thread. So we'll send a message to trigger it.
	_SendMessageW( g_hWnd_main, WM_DESTROY_ALT, 0, 0 );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN copy_items( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	copyinfo *c_i = ( copyinfo * )pArguments;

	HWND hWnd = c_i->hWnd;

	unsigned column_type = 0;

	if ( hWnd == g_hWnd_list )
	{
		column_type = 0;
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		column_type = 1;
	}
	else if ( hWnd == g_hWnd_forward_list )
	{
		column_type = 2;
	}
	else if ( hWnd == g_hWnd_ignore_list )
	{
		column_type = 3;
	}

	Processing_Window( hWnd, false );

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;
	lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

	int item_count = _SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
	int sel_count = _SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );
	
	bool copy_all = false;
	if ( item_count == sel_count )
	{
		copy_all = true;
	}
	else
	{
		item_count = sel_count;
	}

	unsigned int buffer_size = 8192;
	unsigned int buffer_offset = 0;
	wchar_t *copy_buffer = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * buffer_size );	// Allocate 8 kilobytes.

	int value_length = 0;

	int arr[ 17 ];

	// Set to however many columns we'll copy.
	int column_start = 0;
	int column_end = 1;

	if ( c_i->column == MENU_COPY_SEL )
	{
		column_end = ( column_type == 0 ? total_columns : ( column_type == 1 ? total_columns2 : ( column_type == 2 ? total_columns3 : total_columns4 ) ) );
		_SendMessageW( hWnd, LVM_GETCOLUMNORDERARRAY, column_end, ( LPARAM )arr );

		if ( column_type == 0 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, list_columns, NUM_COLUMNS, total_columns );
		}
		else if ( column_type == 1 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, contact_list_columns, NUM_COLUMNS2, total_columns2 );
		}
		else if ( column_type == 2 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, forward_list_columns, NUM_COLUMNS3, total_columns3 );
		}
		else if ( column_type == 3 )
		{
			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr, ignore_list_columns, NUM_COLUMNS4, total_columns4 );
		}
	}
	else
	{
		switch ( c_i->column )
		{
			case MENU_COPY_SEL_COL1:
			case MENU_COPY_SEL_COL21:
			case MENU_COPY_SEL_COL31:
			case MENU_COPY_SEL_COL41: { arr[ 0 ] = 1; } break;
			case MENU_COPY_SEL_COL2:
			case MENU_COPY_SEL_COL22:
			case MENU_COPY_SEL_COL32:
			case MENU_COPY_SEL_COL42: { arr[ 0 ] = 2; } break;
			case MENU_COPY_SEL_COL3:
			case MENU_COPY_SEL_COL23:
			case MENU_COPY_SEL_COL33: { arr[ 0 ] = 3; } break;
			case MENU_COPY_SEL_COL4:
			case MENU_COPY_SEL_COL24: { arr[ 0 ] = 4; } break;
			case MENU_COPY_SEL_COL5:
			case MENU_COPY_SEL_COL25: { arr[ 0 ] = 5; } break;
			case MENU_COPY_SEL_COL6:
			case MENU_COPY_SEL_COL26: { arr[ 0 ] = 6; } break;
			case MENU_COPY_SEL_COL7:
			case MENU_COPY_SEL_COL27: { arr[ 0 ] = 7; } break;
			case MENU_COPY_SEL_COL8:
			case MENU_COPY_SEL_COL28: { arr[ 0 ] = 8; } break;
			case MENU_COPY_SEL_COL29: { arr[ 0 ] = 9; } break;
			case MENU_COPY_SEL_COL210: { arr[ 0 ] = 10; } break;
			case MENU_COPY_SEL_COL211: { arr[ 0 ] = 11; } break;
			case MENU_COPY_SEL_COL212: { arr[ 0 ] = 12; } break;
			case MENU_COPY_SEL_COL213: { arr[ 0 ] = 13; } break;
			case MENU_COPY_SEL_COL214: { arr[ 0 ] = 14; } break;
			case MENU_COPY_SEL_COL215: { arr[ 0 ] = 15; } break;
			case MENU_COPY_SEL_COL216: { arr[ 0 ] = 16; } break;
		}
	}

	wchar_t *copy_string = NULL;
	bool is_string = false;
	bool add_newline = false;
	bool add_tab = false;

	// Go through each item, and copy their lParam values.
	for ( int i = 0; i < item_count; ++i )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thead == true )
		{
			break;
		}

		if ( copy_all == true )
		{
			lvi.iItem = i;
		}
		else
		{
			lvi.iItem = _SendMessageW( hWnd, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
		}

		_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );


		displayinfo *di = NULL;
		contactinfo *ci = NULL;
		ignoreinfo *ii = NULL;
		forwardinfo *fi = NULL;

		if ( column_type == 0 )
		{
			di = ( displayinfo * )lvi.lParam;
		}
		else if ( column_type == 1 )
		{
			ci = ( contactinfo * )lvi.lParam;
		}
		else if ( column_type == 2 )
		{
			fi = ( forwardinfo * )lvi.lParam;
		}
		else if ( column_type == 3 )
		{
			ii = ( ignoreinfo * )lvi.lParam;
		}

		is_string = add_newline = add_tab = false;

		for ( int j = column_start; j < column_end; ++j )
		{
			switch ( arr[ j ] )
			{
				case 1:
				case 2:
				{
					is_string = true;

					copy_string = ( column_type == 0 ? di->display_values[ arr[ j ] - 1 ] : ( column_type == 1 ? ci->contactinfo_values[ arr[ j ] - 1 ] : ( column_type == 2 ? fi->forwardinfo_values[ arr[ j ] - 1 ] : ii->ignoreinfo_values[ arr[ j ] - 1 ] ) ) );
				}
				break;

				case 3:
				{
					is_string = true;

					copy_string = ( column_type == 0 ? di->display_values[ arr[ j ] - 1 ] : ( column_type == 1 ? ci->contactinfo_values[ arr[ j ] - 1 ] : fi->forwardinfo_values[ arr[ j ] - 1 ] ) );
				}
				break;

				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				{
					is_string = true;

					copy_string = ( column_type == 0 ? di->display_values[ arr[ j ] - 1 ] : ci->contactinfo_values[ arr[ j ] - 1 ] );
				}
				break;

				case 9:
				case 10:
				case 11:
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				{
					is_string = true;

					copy_string = ci->contactinfo_values[ arr[ j ] - 1 ];
				}
				break;
			}

			if ( is_string == true )
			{
				is_string = false;

				if ( copy_string == NULL )
				{
					if ( j == 0 )
					{
						add_tab = false;
					}

					continue;
				}

				if ( j > 0 && add_tab == true )
				{
					*( copy_buffer + buffer_offset ) = L'\t';
					++buffer_offset;
				}

				add_tab = true;

				value_length = lstrlenW( copy_string );
				while ( buffer_offset + value_length + 3 >= buffer_size )	// Add +3 for \t and \r\n
				{
					buffer_size += 8192;
					wchar_t *realloc_buffer = ( wchar_t * )GlobalReAlloc( copy_buffer, sizeof( wchar_t ) * buffer_size, GMEM_MOVEABLE );
					if ( realloc_buffer == NULL )
					{
						goto CLEANUP;
					}

					copy_buffer = realloc_buffer;
				}
				_wmemcpy_s( copy_buffer + buffer_offset, buffer_size - buffer_offset, copy_string, value_length );
				buffer_offset += value_length;
			}

			add_newline = true;
		}

		if ( i < item_count - 1 && add_newline == true )	// Add newlines for every item except the last.
		{
			*( copy_buffer + buffer_offset ) = L'\r';
			++buffer_offset;
			*( copy_buffer + buffer_offset ) = L'\n';
			++buffer_offset;
		}
		else if ( ( i == item_count - 1 && add_newline == false ) && buffer_offset >= 2 )	// If add_newline is false for the last item, then a newline character is in the buffer.
		{
			buffer_offset -= 2;	// Ignore the last newline in the buffer.
		}
	}

	if ( _OpenClipboard( hWnd ) )
	{
		_EmptyClipboard();

		DWORD len = buffer_offset;

		// Allocate a global memory object for the text.
		HGLOBAL hglbCopy = GlobalAlloc( GMEM_MOVEABLE, sizeof( wchar_t ) * ( len + 1 ) );
		if ( hglbCopy != NULL )
		{
			// Lock the handle and copy the text to the buffer. lptstrCopy doesn't get freed.
			wchar_t *lptstrCopy = ( wchar_t * )GlobalLock( hglbCopy );
			if ( lptstrCopy != NULL )
			{
				_wmemcpy_s( lptstrCopy, len + 1, copy_buffer, len );
				lptstrCopy[ len ] = 0; // Sanity
			}

			GlobalUnlock( hglbCopy );

			if ( _SetClipboardData( CF_UNICODETEXT, hglbCopy ) == NULL )
			{
				GlobalFree( hglbCopy );	// Only free this Global memory if SetClipboardData fails.
			}

			_CloseClipboard();
		}
	}

CLEANUP:

	GlobalFree( copy_buffer );
	GlobalFree( c_i );

	Processing_Window( hWnd, true );

	// Release the mutex if we're killing the thread.
	if ( worker_mutex != NULL )
	{
		ReleaseSemaphore( worker_mutex, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}


THREAD_RETURN remove_items( void *pArguments )
{
	removeinfo *ri = ( removeinfo * )pArguments;
	
	HWND hWnd = NULL;
	bool disable_critical_section = false;

	if ( ri != NULL )
	{
		disable_critical_section = ri->disable_critical_section;
		hWnd = ri->hWnd;

		GlobalFree( ri );
	}

	// This will block every other thread from entering until the first thread is complete.
	if ( disable_critical_section == false )
	{
		EnterCriticalSection( &pe_cs );
	}

	in_worker_thread = true;
	
	bool *skip_draw = NULL;

	// Prevent the listviews from drawing while freeing lParam values.
	if ( hWnd == g_hWnd_list )
	{
		skip_draw = &skip_log_draw;
	}
	else if ( hWnd == g_hWnd_contact_list )
	{
		skip_draw = &skip_contact_draw;
	}
	else if ( hWnd == g_hWnd_ignore_list )
	{
		skip_draw = &skip_ignore_draw;
	}
	else if ( hWnd == g_hWnd_forward_list )
	{
		skip_draw = &skip_forward_draw;
	}
	
	if ( skip_draw != NULL )
	{
		*skip_draw = true;
	}

	if ( disable_critical_section == false )
	{
		Processing_Window( hWnd, false );
	}

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;

	int item_count = _SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
	int sel_count = 1;

	// If we're removing contacts, then we can only do one at a time. Otherwise, handle all selected.
	if ( hWnd != g_hWnd_contact_list )
	{
		sel_count = _SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );
	}

	int *index_array = NULL;

	bool handle_all = false;
	if ( item_count == sel_count )
	{
		handle_all = true;
	}
	else
	{
		_SendMessageW( hWnd, LVM_ENSUREVISIBLE, 0, FALSE );

		index_array = ( int * )GlobalAlloc( GMEM_FIXED, sizeof( int ) * sel_count );

		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

		// Create an index list of selected items (in reverse order).
		for ( int i = 0; i < sel_count; i++ )
		{
			lvi.iItem = index_array[ sel_count - 1 - i ] = _SendMessageW( hWnd, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
		}

		item_count = sel_count;
	}

	// Go through each item, and free their lParam values.
	for ( int i = 0; i < item_count; ++i )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thead == true )
		{
			break;
		}

		if ( handle_all == true )
		{
			lvi.iItem = i;
		}
		else
		{
			lvi.iItem = index_array[ i ];
		}

		_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

		if ( hWnd == g_hWnd_list )
		{
			displayinfo *di = ( displayinfo * )lvi.lParam;

			if ( di != NULL )
			{
				// Update each displayinfo item.
				dllrbt_iterator *itr = dllrbt_find( call_log, ( void * )di->ci.call_from, false );
				if ( itr != NULL )
				{
					// Head of the linked list.
					DoublyLinkedList *ll = ( DoublyLinkedList * )( ( node_type * )itr )->val;

					// Go through each linked list node and remove the one with di.

					DoublyLinkedList *current_node = ll;
					while ( current_node != NULL )
					{
						if ( ( displayinfo * )current_node->data == di )
						{
							DLL_RemoveNode( &ll, current_node );
							GlobalFree( current_node );

							if ( ll != NULL )
							{
								// Reset the head in the tree.
								( ( node_type * )itr )->val = ( void * )ll;
								( ( node_type * )itr )->key = ( void * )( ( displayinfo * )ll->data )->ci.call_from;

								call_log_changed = true;
							}

							break;
						}

						current_node = current_node->next;
					}

					// If the head of the linked list is NULL, then we can remove the linked list from the call log tree.
					if ( ll == NULL )
					{
						dllrbt_remove( call_log, itr );	// Remove the node from the tree. The tree will rebalance itself.

						call_log_changed = true;
					}
				}

				free_displayinfo( &di );
			}
		}
		else if ( hWnd == g_hWnd_contact_list )
		{
			contactinfo *ci = ( contactinfo * )lvi.lParam;

			if ( ci != NULL )
			{
				dllrbt_iterator *itr = dllrbt_find( contact_list, ( void * )ci->contact.contact_entry_id, false );
				if ( itr != NULL )
				{
					dllrbt_remove( contact_list, itr );	// Remove the node from the tree. The tree will rebalance itself.
				}

				free_contactinfo( &ci );
			}
		}
		else if ( hWnd == g_hWnd_ignore_list )
		{
			ignoreinfo *ii = ( ignoreinfo * )lvi.lParam;

			if ( ii != NULL )
			{
				char range_number[ 32 ];
				_memzero( range_number, 32 );

				int range_index = lstrlenA( ii->c_phone_number );
				range_index = ( range_index > 0 ? range_index - 1 : 0 );

				// If the number we've removed is a range, then remove it from the range list.
				if ( is_num( ii->c_phone_number ) == 1 )
				{
					RangeRemove( &ignore_range_list[ range_index ], ii->c_phone_number );

					// Update each displayinfo item to indicate that it is no longer ignored.
					node_type *node = dllrbt_get_head( call_log );
					while ( node != NULL )
					{
						DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
						while ( di_node != NULL )
						{
							displayinfo *mdi = ( displayinfo * )di_node->data;

							// Process display values that are set to be ignored.
							if ( mdi->ignore == true )
							{
								// First, see if the display value falls within another range.
								if ( RangeSearch( &ignore_range_list[ range_index ], mdi->ci.call_from, range_number ) == true )
								{
									// If it does, then we'll skip the display value.
									break;
								}

								// Next, see if the display value exists as a non-range value.
								if ( dllrbt_find( ignore_list, ( void * )mdi->ci.call_from, false ) != NULL )
								{
									// If it does, then we'll skip the display value.
									break;
								}

								// Finally, see if the display item falls within the range we've removed.
								if ( RangeCompare( ii->c_phone_number, mdi->ci.call_from ) == true )
								{
									mdi->ignore = false;
									GlobalFree( mdi->w_ignore );
									mdi->w_ignore = GlobalStrDupW( ST_No );
								}
							}

							di_node = di_node->next;
						}

						node = node->next;
					}
				}
				else
				{
					// See if the value we remove falls within a range. If it doesn't, then set its new display values.
					if ( RangeSearch( &ignore_range_list[ range_index ], ii->c_phone_number, range_number ) == false )
					{
						// Update each displayinfo item to indicate that it is no longer ignored.
						DoublyLinkedList *ll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )ii->c_phone_number, true );
						if ( ll != NULL )
						{
							DoublyLinkedList *di_node = ll;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								if ( mdi->ignore == true )
								{
									mdi->ignore = false;
									GlobalFree( mdi->w_ignore );
									mdi->w_ignore = GlobalStrDupW( ST_No );
								}

								di_node = di_node->next;
							}
						}
					}
				}

				// See if the ignore_list phone number exits. It should.
				dllrbt_iterator *itr = dllrbt_find( ignore_list, ( void * )ii->c_phone_number, false );
				if ( itr != NULL )
				{
					dllrbt_remove( ignore_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					ignore_list_changed = true;	// Causes us to save the ignore_list on shutdown.
				}

				free_ignoreinfo( &ii );
			}
		}
		else if ( hWnd == g_hWnd_forward_list )
		{
			forwardinfo *fi = ( forwardinfo * )lvi.lParam;

			if ( fi != NULL )
			{
				char range_number[ 32 ];
				_memzero( range_number, 32 );

				int range_index = lstrlenA( fi->c_call_from );
				range_index = ( range_index > 0 ? range_index - 1 : 0 );

				// If the number we've removed is a range, then remove it from the range list.
				if ( is_num( fi->c_call_from ) == 1 )
				{
					RangeRemove( &forward_range_list[ range_index ], fi->c_call_from );

					// Update each displayinfo item to indicate that it is no longer forwarded.
					node_type *node = dllrbt_get_head( call_log );
					while ( node != NULL )
					{
						DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
						while ( di_node != NULL )
						{
							displayinfo *mdi = ( displayinfo * )di_node->data;

							// Process display values that are set to be forwarded.
							if ( mdi->forward == true )
							{
								// First, see if the display value falls within another range.
								if ( RangeSearch( &forward_range_list[ range_index ], mdi->ci.call_from, range_number ) == true )
								{
									// If it does, then we'll skip the display value.
									break;
								}

								// Next, see if the display value exists as a non-range value.
								if ( dllrbt_find( forward_list, ( void * )mdi->ci.call_from, false ) != NULL )
								{
									// If it does, then we'll skip the display value.
									break;
								}

								// Finally, see if the display item falls within the range we've removed.
								if ( RangeCompare( fi->c_call_from, mdi->ci.call_from ) == true )
								{
									mdi->forward = false;
									GlobalFree( mdi->w_forward );
									mdi->w_forward = GlobalStrDupW( ST_No );
								}
							}

							di_node = di_node->next;
						}

						node = node->next;
					}
				}
				else
				{
					// See if the value we remove falls within a range. If it doesn't, then set its new display values.
					if ( RangeSearch( &forward_range_list[ range_index ], fi->c_call_from, range_number ) == false )
					{
						// Update each displayinfo item to indicate that it is no longer forwarded.
						DoublyLinkedList *ll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )fi->c_call_from, true );
						if ( ll != NULL )
						{
							DoublyLinkedList *di_node = ll;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								if ( mdi->forward == true )
								{
									mdi->forward = false;
									GlobalFree( mdi->w_forward );
									mdi->w_forward = GlobalStrDupW( ST_No );
								}

								di_node = di_node->next;
							}
						}
					}
				}

				// See if the forward_list phone number exits. It should.
				dllrbt_iterator *itr = dllrbt_find( forward_list, ( void * )fi->c_call_from, false );
				if ( itr != NULL )
				{
					dllrbt_remove( forward_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

					forward_list_changed = true;	// Causes us to save the forward_list on shutdown.
				}

				free_forwardinfo( &fi );
			}
		}

		if ( handle_all == false )
		{
			_SendMessageW( hWnd, LVM_DELETEITEM, index_array[ i ], 0 );
		}
	}

	if ( handle_all == true )
	{
		_SendMessageW( hWnd, LVM_DELETEALLITEMS, 0, 0 );
	}
	else
	{
		GlobalFree( index_array );
	}

	if ( skip_draw != NULL )
	{
		*skip_draw = false;
	}

	if ( disable_critical_section == false )
	{
		Processing_Window( hWnd, true );

		// Release the mutex if we're killing the thread.
		if ( worker_mutex != NULL )
		{
			ReleaseSemaphore( worker_mutex, 1, NULL );
		}
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	if ( disable_critical_section == false )
	{
		LeaveCriticalSection( &pe_cs );
	}

	_ExitThread( 0 );
	return 0;
}

wchar_t *FormatPhoneNumber( char *phone_number )
{
	wchar_t *val = NULL;

	if ( phone_number != NULL )
	{
		int val_length = MultiByteToWideChar( CP_UTF8, 0, phone_number, -1, NULL, 0 );	// Include the NULL terminator.

		if ( is_num( phone_number ) >= 0 )
		{
			if ( val_length == 11 )	// 10 digit phone number + 1 NULL character. (555) 555-1234
			{
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( val_length + 4 ) );
				MultiByteToWideChar( CP_UTF8, 0, phone_number, -1, val + 4, val_length );

				val[ 0 ] = L'(';
				_wmemcpy_s( val + 1, 14, val + 4, 3 );
				val[ 4 ] = L')';
				val[ 5 ] = L' ';
				_wmemcpy_s( val + 6, 9, val + 7, 3 );
				val[ 9 ] = L'-';
				val[ 14 ] = 0;	// Sanity
			}
			else if ( val_length == 12 && phone_number[ 0 ] == '1' )	// 11 digit phone number + 1 NULL character. 1-555-555-1234
			{
				val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( val_length + 3 ) );
				MultiByteToWideChar( CP_UTF8, 0, phone_number, -1, val + 3, val_length );

				val[ 0 ] = *( val + 3 );
				val[ 1 ] = L'-';
				_wmemcpy_s( val + 2, 13, val + 4, 3 );
				val[ 5 ] = L'-';
				_wmemcpy_s( val + 6, 9, val + 7, 3 );
				val[ 9 ] = L'-';
				val[ 14 ] = 0;	// Sanity
			}
		}
		
		if ( val == NULL )	// Number has some weird length, is unavailable, unknown, etc.
		{
			val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
			MultiByteToWideChar( CP_UTF8, 0, phone_number, -1, val, val_length );
		}
	}

	return val;
}

void FormatDisplayInfo( displayinfo *di )
{
	wchar_t *val = NULL;
	int val_length = 0;

	// Phone
	di->phone_number = FormatPhoneNumber( di->ci.call_from );

	// Caller ID
	if ( di->ci.caller_id != NULL )
	{
		val_length = MultiByteToWideChar( CP_UTF8, 0, di->ci.caller_id, -1, NULL, 0 );	// Include the NULL terminator.
		val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
		MultiByteToWideChar( CP_UTF8, 0, di->ci.caller_id, -1, val, val_length );

		di->caller_id = val;
		val = NULL;
	}

	// Reference
	if ( di->ci.call_reference_id != NULL )
	{
		val_length = MultiByteToWideChar( CP_UTF8, 0, di->ci.call_reference_id, -1, NULL, 0 );	// Include the NULL terminator.
		val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
		MultiByteToWideChar( CP_UTF8, 0, di->ci.call_reference_id, -1, val, val_length );

		di->reference = val;
		val = NULL;
	}

	SYSTEMTIME st;
	FILETIME ft;
	ft.dwLowDateTime = di->time.LowPart;
	ft.dwHighDateTime = di->time.HighPart;

	FileTimeToSystemTime( &ft, &st );

	int buffer_length = 0;

	#ifndef NTDLL_USE_STATIC_LIB
		buffer_length = 64;	// Should be enough to hold most translated values.
	#else
		buffer_length = _scwprintf( L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) ) + 1;	// Include the NULL character.
	#endif

	di->w_time = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * buffer_length );

	__snwprintf( di->w_time, buffer_length, L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) );

	di->w_forward = ( di->forward == true ? GlobalStrDupW( ST_Yes ) : GlobalStrDupW( ST_No ) );
	di->w_ignore = ( di->ignore == true ? GlobalStrDupW( ST_Yes ) : GlobalStrDupW( ST_No ) );

	// Forward to phone number
	di->forward_to = FormatPhoneNumber( di->ci.forward_to );

	// Sent to phone number
	di->sent_to = FormatPhoneNumber( di->ci.call_to );
}

THREAD_RETURN update_call_log( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	displayinfo *di = ( displayinfo * )pArguments;

	char range_number[ 32 ];

	// Each number in our call log contains a linked list. Find the number if it exists and add it to the linked list.

	// Create the node to insert into a linked list.
	DoublyLinkedList *di_node = DLL_CreateNode( ( void * )di );

	// See if our tree has the phone number to add the node to.
	DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
	if ( dll == NULL )
	{
		// If no phone number exits, insert the node into the tree.
		if ( dllrbt_insert( call_log, ( void * )di->ci.call_from, ( void * )di_node ) != DLLRBT_STATUS_OK )
		{
			GlobalFree( di_node );	// This shouldn't happen.
		}
		else
		{
			call_log_changed = true;
		}
	}
	else	// If a phone number exits, insert the node into the linked list.
	{
		DLL_AddNode( &dll, di_node, -1 );	// Insert at the end of the doubly linked list.

		call_log_changed = true;
	}

	// Search the ignore_list for a match.
	ignoreinfo *ii = ( ignoreinfo * )dllrbt_find( ignore_list, ( void * )di->ci.call_from, true );

	// Try searching the range list.
	if ( ii == NULL )
	{
		_memzero( range_number, 32 );

		int range_index = lstrlenA( di->ci.call_from );
		range_index = ( range_index > 0 ? range_index - 1 : 0 );

		if ( RangeSearch( &ignore_range_list[ range_index ], di->ci.call_from, range_number ) == true )
		{
			ii = ( ignoreinfo * )dllrbt_find( ignore_list, ( void * )range_number, true );
		}
	}

	if ( ii != NULL )
	{
		di->process_incoming = false;
		di->ignore = true;
		di->ci.ignored = true;

		CloseHandle( ( HANDLE )_CreateThread( NULL, 0, IgnoreIncomingCall, ( void * )&( di->ci ), 0, NULL ) );

		ignoreupdateinfo *iui = ( ignoreupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreupdateinfo ) );
		iui->ii = ii;
		iui->phone_number = NULL;
		iui->action = 3;	// Update the call count.
		iui->hWnd = g_hWnd_ignore_list;

		// iui is freed in the update_ignore_list thread.
		CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_ignore_list, ( void * )iui, 0, NULL ) );
	}

	// Search the forward list for a match.
	forwardinfo *fi = ( forwardinfo * )dllrbt_find( forward_list, ( void * )di->ci.call_from, true );

	// Try searching the range list.
	if ( fi == NULL )
	{
		_memzero( range_number, 32 );

		int range_index = lstrlenA( di->ci.call_from );
		range_index = ( range_index > 0 ? range_index - 1 : 0 );

		if ( RangeSearch( &forward_range_list[ range_index ], di->ci.call_from, range_number ) == true )
		{
			fi = ( forwardinfo * )dllrbt_find( forward_list, ( void * )range_number, true );
		}
	}

	if ( fi != NULL )
	{
		di->process_incoming = false;
		di->forward = true;
		di->ci.forward_to = GlobalStrDupA( fi->c_forward_to );
		di->ci.forwarded = true;

		// Ignore list has priority. If it's set, then we don't forward.
		if ( di->ignore == false )
		{
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, ForwardIncomingCall, ( void * )&( di->ci ), 0, NULL ) );

			forwardupdateinfo *fui = ( forwardupdateinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardupdateinfo ) );
			fui->fi = fi;
			fui->call_from = NULL;
			fui->forward_to = NULL;
			fui->action = 4;	// Update the call count.
			fui->hWnd = g_hWnd_forward_list;

			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, update_forward_list, ( void * )fui, 0, NULL ) );
		}
	}

	_SendNotifyMessageW( g_hWnd_main, WM_PROPAGATE, MAKEWPARAM( CW_MODIFY, 0 ), ( LPARAM )di );	// Add entry to listview and show popup.

	// Release the mutex if we're killing the thread.
	if ( worker_mutex != NULL )
	{
		ReleaseSemaphore( worker_mutex, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN update_contact_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	Processing_Window( g_hWnd_contact_list, false );

	wchar_t *val = NULL;
	int val_length = 0;

	// Insert a row into our listview.
	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
	lvi.iSubItem = 0;

	updateinfo *ui = ( updateinfo * )pArguments;	// Freed at the end of this thread.
	contactinfo *ci = NULL;
	contactinfo *uci = NULL;
	if ( ui != NULL )
	{
		ci = ( contactinfo * )ui->old_ci;
		uci = ( contactinfo * )ui->new_ci;
	}

	if ( ci != NULL && uci != NULL )	// Update a contact. Free uci when done.
	{
		if ( ui->remove_picture == true )	// We want to remove the contact's picture.
		{
			if ( ci->picture_path != NULL )
			{
				GlobalFree( ci->picture_path );
				ci->picture_path = NULL;
			}

			if ( ci->contact.picture_location != NULL )
			{
				GlobalFree( ci->contact.picture_location );
				ci->contact.picture_location = NULL;
			}
		}

		if ( uci->contact.first_name != NULL )
		{
			GlobalFree( ci->contact.first_name );
			ci->contact.first_name = uci->contact.first_name;
			GlobalFree( ci->first_name );
			ci->first_name = uci->first_name;
		}

		if ( uci->contact.last_name != NULL )
		{
			GlobalFree( ci->contact.last_name );
			ci->contact.last_name = uci->contact.last_name;
			GlobalFree( ci->last_name );
			ci->last_name = uci->last_name;
		}

		if ( uci->contact.nickname != NULL )
		{
			GlobalFree( ci->contact.nickname );
			ci->contact.nickname = uci->contact.nickname;
			GlobalFree( ci->nickname );
			ci->nickname = uci->nickname;
		}

		if ( uci->contact.title != NULL )
		{
			GlobalFree( ci->contact.title );
			ci->contact.title = uci->contact.title;
			GlobalFree( ci->title );
			ci->title = uci->title;
		}

		if ( uci->contact.business_name != NULL )
		{
			GlobalFree( ci->contact.business_name );
			ci->contact.business_name = uci->contact.business_name;
			GlobalFree( ci->business_name );
			ci->business_name = uci->business_name;
		}

		if ( uci->contact.designation != NULL )
		{
			GlobalFree( ci->contact.designation );
			ci->contact.designation = uci->contact.designation;
			GlobalFree( ci->designation );
			ci->designation = uci->designation;
		}

		if ( uci->contact.department != NULL )
		{
			GlobalFree( ci->contact.department );
			ci->contact.department = uci->contact.department;
			GlobalFree( ci->department );
			ci->department = uci->department;
		}

		if ( uci->contact.category != NULL )
		{
			GlobalFree( ci->contact.category );
			ci->contact.category = uci->contact.category;
			GlobalFree( ci->category );
			ci->category = uci->category;
		}

		if ( uci->contact.email_address != NULL )
		{
			GlobalFree( ci->contact.email_address );
			ci->contact.email_address = uci->contact.email_address;
			GlobalFree( ci->email_address );
			ci->email_address = uci->email_address;
		}

		if ( uci->contact.email_address_id != NULL )
		{
			GlobalFree( ci->contact.email_address_id );
			ci->contact.email_address_id = uci->contact.email_address_id;
		}

		if ( uci->contact.web_page != NULL )
		{
			GlobalFree( ci->contact.web_page );
			ci->contact.web_page = uci->contact.web_page;
			GlobalFree( ci->web_page );
			ci->web_page = uci->web_page;
		}

		if ( uci->contact.web_page_id != NULL )
		{
			GlobalFree( ci->contact.web_page_id );
			ci->contact.web_page_id = uci->contact.web_page_id;
		}

		if ( uci->contact.home_phone_number != NULL )
		{
			GlobalFree( ci->contact.home_phone_number );
			ci->contact.home_phone_number = uci->contact.home_phone_number;
			GlobalFree( ci->home_phone_number );
			ci->home_phone_number = FormatPhoneNumber( ci->contact.home_phone_number );
		}

		if ( uci->contact.home_phone_number_id != NULL )
		{
			GlobalFree( ci->contact.home_phone_number_id );
			ci->contact.home_phone_number_id = uci->contact.home_phone_number_id;
		}

		if ( uci->contact.cell_phone_number != NULL )
		{
			GlobalFree( ci->contact.cell_phone_number );
			ci->contact.cell_phone_number = uci->contact.cell_phone_number;
			GlobalFree( ci->cell_phone_number );
			ci->cell_phone_number = FormatPhoneNumber( ci->contact.cell_phone_number );
		}

		if ( uci->contact.cell_phone_number_id != NULL )
		{
			GlobalFree( ci->contact.cell_phone_number_id );
			ci->contact.cell_phone_number_id = uci->contact.cell_phone_number_id;
		}

		if ( uci->contact.office_phone_number != NULL )
		{
			GlobalFree( ci->contact.office_phone_number );
			ci->contact.office_phone_number = uci->contact.office_phone_number;
			GlobalFree( ci->office_phone_number );
			ci->office_phone_number = FormatPhoneNumber( ci->contact.office_phone_number );
		}

		if ( uci->contact.office_phone_number_id != NULL )
		{
			GlobalFree( ci->contact.office_phone_number_id );
			ci->contact.office_phone_number_id = uci->contact.office_phone_number_id;
		}

		if ( uci->contact.other_phone_number != NULL )
		{
			GlobalFree( ci->contact.other_phone_number );
			ci->contact.other_phone_number = uci->contact.other_phone_number;
			GlobalFree( ci->other_phone_number );
			ci->other_phone_number = FormatPhoneNumber( ci->contact.other_phone_number );
		}

		if ( uci->contact.other_phone_number_id != NULL )
		{
			GlobalFree( ci->contact.other_phone_number_id );
			ci->contact.other_phone_number_id = uci->contact.other_phone_number_id;
		}

		if ( uci->contact.work_phone_number != NULL )
		{
			GlobalFree( ci->contact.work_phone_number );
			ci->contact.work_phone_number = uci->contact.work_phone_number;
			GlobalFree( ci->work_phone_number );
			ci->work_phone_number = FormatPhoneNumber( ci->contact.work_phone_number );
		}

		if ( uci->contact.work_phone_number_id != NULL )
		{
			GlobalFree( ci->contact.work_phone_number_id );
			ci->contact.work_phone_number_id = uci->contact.work_phone_number_id;
		}

		if ( uci->contact.fax_number != NULL )
		{
			GlobalFree( ci->contact.fax_number );
			ci->contact.fax_number = uci->contact.fax_number;
			GlobalFree( ci->fax_number );
			ci->fax_number = FormatPhoneNumber( ci->contact.fax_number );
		}

		if ( uci->contact.fax_number_id != NULL )
		{
			GlobalFree( ci->contact.fax_number_id );
			ci->contact.fax_number_id = uci->contact.fax_number_id;
		}

		if ( uci->picture_path != NULL )
		{
			GlobalFree( uci->picture_path );
		}

		GlobalFree( uci );
	}
	else if ( ci != NULL && uci == NULL )	// Add a single contact to the contact list listview.
	{
		ci->home_phone_number = FormatPhoneNumber( ci->contact.home_phone_number );
		ci->cell_phone_number = FormatPhoneNumber( ci->contact.cell_phone_number );
		ci->office_phone_number = FormatPhoneNumber( ci->contact.office_phone_number );
		ci->other_phone_number = FormatPhoneNumber( ci->contact.other_phone_number );

		ci->work_phone_number = FormatPhoneNumber( ci->contact.work_phone_number );
		ci->fax_number = FormatPhoneNumber( ci->contact.fax_number );

		ci->displayed = true;

		lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETITEMCOUNT, 0, 0 );
		lvi.lParam = ( LPARAM )ci;	// lParam = our contactinfo structure from the connection thread.
		_SendMessageW( g_hWnd_contact_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
	}
	else if ( ui != NULL && ci == NULL && uci == NULL )	// Add all contacts in the contact_list to the contact list listview.
	{
		node_type *node = dllrbt_get_head( contact_list );
		while ( node != NULL )
		{
			ci = ( contactinfo * )node->val;

			// Add contacts that haven't been displayed yet.
			if ( ci != NULL && ci->displayed == false )
			{
				// First Name
				if ( ci->contact.first_name != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.first_name, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.first_name, -1, val, val_length );

					ci->first_name = val;
					val = NULL;
				}

				// Last Name
				if ( ci->contact.last_name != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.last_name, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.last_name, -1, val, val_length );

					ci->last_name = val;
					val = NULL;
				}

				// Nickname
				if ( ci->contact.nickname != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.nickname, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.nickname, -1, val, val_length );

					ci->nickname = val;
					val = NULL;
				}

				// Title
				if ( ci->contact.title != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.title, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.title, -1, val, val_length );

					ci->title = val;
					val = NULL;
				}

				// Business Name
				if ( ci->contact.business_name != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.business_name, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.business_name, -1, val, val_length );

					ci->business_name = val;
					val = NULL;
				}

				// Designation
				if ( ci->contact.designation != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.designation, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.designation, -1, val, val_length );

					ci->designation = val;
					val = NULL;
				}

				// Department
				if ( ci->contact.department != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.department, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.department, -1, val, val_length );

					ci->department = val;
					val = NULL;
				}

				// Category
				if ( ci->contact.category != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.category, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.category, -1, val, val_length );

					ci->category = val;
					val = NULL;
				}

				// Email Address
				if ( ci->contact.email_address != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.email_address, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.email_address, -1, val, val_length );

					ci->email_address = val;
					val = NULL;
				}

				// Web Page
				if ( ci->contact.web_page != NULL )
				{
					val_length = MultiByteToWideChar( CP_UTF8, 0, ci->contact.web_page, -1, NULL, 0 );	// Include the NULL terminator.
					val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ci->contact.web_page, -1, val, val_length );

					ci->web_page = val;
					val = NULL;
				}

				ci->home_phone_number = FormatPhoneNumber( ci->contact.home_phone_number );
				ci->cell_phone_number = FormatPhoneNumber( ci->contact.cell_phone_number );
				ci->office_phone_number = FormatPhoneNumber( ci->contact.office_phone_number );
				ci->other_phone_number = FormatPhoneNumber( ci->contact.other_phone_number );

				ci->work_phone_number = FormatPhoneNumber( ci->contact.work_phone_number );
				ci->fax_number = FormatPhoneNumber( ci->contact.fax_number );

				ci->displayed = true;

				lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETITEMCOUNT, 0, 0 );
				lvi.lParam = ( LPARAM )ci;	// lParam = our contactinfo structure from the connection thread.
				_SendMessageW( g_hWnd_contact_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
			}
			
			node = node->next;
		}
	}
	else	// Remove all contacts from the contact_list.
	{
		// Clear the contact list.
		_SendMessageW( g_hWnd_contact_list, LVM_DELETEALLITEMS, 0, 0 );

		if ( contact_list != NULL )
		{
			// Free the values of the contact_list.
			node_type *node = dllrbt_get_head( contact_list );
			while ( node != NULL )
			{
				contactinfo *ci = ( contactinfo * )node->val;
				free_contactinfo( &ci );
				node = node->next;
			}

			dllrbt_delete_recursively( contact_list );

			contact_list = dllrbt_create( dllrbt_compare );

			if ( web_server_state == WEB_SERVER_STATE_RUNNING )
			{
				SetContactList( contact_list );
			}
		}
	}

	GlobalFree( ui );

	Processing_Window( g_hWnd_contact_list, true );

	// Release the mutex if we're killing the thread.
	if ( worker_mutex != NULL )
	{
		ReleaseSemaphore( worker_mutex, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}




THREAD_RETURN update_ignore_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	HWND hWnd = NULL;
	ignoreupdateinfo *iui = ( ignoreupdateinfo * )pArguments;
	if ( iui != NULL )
	{
		hWnd = iui->hWnd;
	}

	Processing_Window( hWnd, false );

	if ( iui != NULL )
	{
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

		if ( iui->hWnd == g_hWnd_ignore_list )
		{
			if ( iui->action == 0 )	// Add a single item to ignore_list and ignore list listview.
			{
				ignoreinfo *ii = ( ignoreinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreinfo ) );

				ii->phone_number = FormatPhoneNumber( iui->phone_number );
				ii->c_phone_number = iui->phone_number;

				ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
				*ii->c_total_calls = '0';
				*( ii->c_total_calls + 1 ) = 0;	// Sanity

				int val_length = MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
				wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
				MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, val, val_length );

				ii->total_calls = val;

				ii->count = 0;
				ii->state = 0;

				// Try to insert the ignore_list info in the tree.
				if ( dllrbt_insert( ignore_list, ( void * )ii->c_phone_number, ( void * )ii ) != DLLRBT_STATUS_OK )
				{
					free_ignoreinfo( &ii );

					_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_already_in_ignore_list );
				}
				else	// If it was able to be inserted into the tree, then update our displayinfo items and the ignore list listview.
				{
					// See if the value we're adding is a range (has wildcard values in it).
					if ( ii->c_phone_number != NULL && is_num( ii->c_phone_number ) == 1 )
					{
						int phone_number_length = lstrlenA( ii->c_phone_number );

						RangeAdd( &ignore_range_list[ ( phone_number_length > 0 ? phone_number_length - 1 : 0 ) ], ii->c_phone_number, phone_number_length );

						// Update each displayinfo item to indicate that it is now ignored.
						node_type *node = dllrbt_get_head( call_log );
						while ( node != NULL )
						{
							DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								// Process values that are not set to be ignored. See if the value falls within our range.
								if ( mdi->ignore == false && RangeCompare( ii->c_phone_number, mdi->ci.call_from ) == true )
								{
									mdi->ignore = true;
									GlobalFree( mdi->w_ignore );
									mdi->w_ignore = GlobalStrDupW( ST_Yes );
								}

								di_node = di_node->next;
							}

							node = node->next;
						}
					}
					else
					{
						// Update each displayinfo item to indicate that it is now ignored.
						DoublyLinkedList *ll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )ii->c_phone_number, true );
						if ( ll != NULL )
						{
							DoublyLinkedList *di_node = ll;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								if ( mdi->ignore == false )
								{
									mdi->ignore = true;
									GlobalFree( mdi->w_ignore );
									mdi->w_ignore = GlobalStrDupW( ST_Yes );
								}

								di_node = di_node->next;
							}
						}
					}

					ignore_list_changed = true;

					LVITEM lvi1;
					_memzero( &lvi1, sizeof( LVITEM ) );
					lvi1.mask = LVIF_PARAM;
					lvi1.iSubItem = 0;
					lvi1.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi1.lParam = ( LPARAM )ii;	// lParam = our contactinfo structure from the connection thread.
					_SendMessageW( g_hWnd_ignore_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi1 );
				}
			}
			else if ( iui->action == 1 )	// Remove from ignore_list and ignore list listview.
			{
				removeinfo *ri = ( removeinfo * )GlobalAlloc( GMEM_FIXED, sizeof( removeinfo ) );
				ri->disable_critical_section = true;
				ri->hWnd = g_hWnd_ignore_list;

				// ri will be freed in the remove_items thread.
				HANDLE hThread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL );

				if ( hThread != NULL )
				{
					WaitForSingleObject( hThread, INFINITE );	// Wait at most 10 seconds for the remove_items thread to finish.
					CloseHandle( hThread );
				}
			}
			else if ( iui->action == 2 )	// Add all items in the ignore_list to the ignore list listview.
			{
				// Insert a row into our listview.
				LVITEM lvi1;
				_memzero( &lvi1, sizeof( LVITEM ) );
				lvi1.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
				lvi1.iSubItem = 0;

				node_type *node = dllrbt_get_head( ignore_list );
				while ( node != NULL )
				{
					ignoreinfo *ii = ( ignoreinfo * )node->val;

					if ( ii != NULL )
					{
						lvi1.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi1.lParam = ( LPARAM )ii;	// lParam = our contactinfo structure from the connection thread.
						_SendMessageW( g_hWnd_ignore_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi1 );
					}

					node = node->next;
				}
			}
			else if ( iui->action == 3 )	// Update the recently called number's call count.
			{
				if ( iui->ii != NULL )
				{
					iui->ii->count++;

					GlobalFree( iui->ii->c_total_calls );

					iui->ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 11 );
					__snprintf( iui->ii->c_total_calls, 11, "%lu", iui->ii->count );

					GlobalFree( iui->ii->total_calls );

					int val_length = MultiByteToWideChar( CP_UTF8, 0, iui->ii->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, iui->ii->c_total_calls, -1, val, val_length );

					iui->ii->total_calls = val;

					ignore_list_changed = true;
				}
			}

			_InvalidateRect( g_hWnd_list, NULL, TRUE );
		}
		else if ( iui->hWnd == g_hWnd_list )	// call log listview
		{
			int item_count = _SendMessageW( g_hWnd_list, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = _SendMessageW( g_hWnd_list, LVM_GETSELECTEDCOUNT, 0, 0 );
			
			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				item_count = sel_count;
			}

			bool remove_state_changed = false;	// If true, then go through the ignore list listview and remove the items that have changed state.
			bool skip_range_warning = false;	// Skip the range warning if we've already displayed it.

			// Go through each item in the listview.
			for ( int i = 0; i < item_count; ++i )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thead == true )
				{
					break;
				}

				if ( handle_all == true )
				{
					lvi.iItem = i;
				}
				else
				{
					lvi.iItem = _SendMessageW( g_hWnd_list, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
				}

				_SendMessageW( g_hWnd_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

				displayinfo *di = ( displayinfo * )lvi.lParam;

				if ( di->ignore == true && iui->action == 1 )		// Remove from ignore_list.
				{
					char range_number[ 32 ];
					_memzero( range_number, 32 );

					int range_index = lstrlenA( di->ci.call_from );
					range_index = ( range_index > 0 ? range_index - 1 : 0 );

					// First, see if the phone number is in our range list.
					if ( RangeSearch( &ignore_range_list[ range_index ], di->ci.call_from, range_number ) == false )
					{
						// If not, then see if the phone number is in our ignore_list.
						dllrbt_iterator *itr = dllrbt_find( ignore_list, ( void * )di->ci.call_from, false );
						if ( itr != NULL )
						{
							// Update each displayinfo item to indicate that it is no longer ignored.
							DoublyLinkedList *ll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
							if ( ll != NULL )
							{
								DoublyLinkedList *di_node = ll;
								while ( di_node != NULL )
								{
									displayinfo *mdi = ( displayinfo * )di_node->data;

									if ( mdi->ignore == true )
									{
										mdi->ignore = false;
										GlobalFree( mdi->w_ignore );
										mdi->w_ignore = GlobalStrDupW( ST_No );
									}

									di_node = di_node->next;
								}
							}

							// If an ignore list listview item also shows up in the call log listview, then we'll remove it after this loop.
							ignoreinfo *ii = ( ignoreinfo * )( ( node_type * )itr )->val;
							ii->state = 1;
							remove_state_changed = true;	// This will let us go through the ignore list listview and free items with a state == 1.

							dllrbt_remove( ignore_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

							ignore_list_changed = true;	// Causes us to save the ignore_list on shutdown.
						}

						di->ignore = false;
						GlobalFree( di->w_ignore );
						di->w_ignore = GlobalStrDupW( ST_No );
					}
					else
					{
						if ( skip_range_warning == false )
						{
							_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_remove_from_ignore_list );
							skip_range_warning = true;
						}
					}
				}
				else if ( di->ignore == false && iui->action == 0 )	// Add to ignore_list.
				{
					ignoreinfo *ii = ( ignoreinfo * )GlobalAlloc( GMEM_FIXED, sizeof( ignoreinfo ) );

					ii->phone_number = FormatPhoneNumber( di->ci.call_from );
					ii->c_phone_number = GlobalStrDupA( di->ci.call_from );

					ii->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
					*ii->c_total_calls = '0';
					*( ii->c_total_calls + 1 ) = 0;	// Sanity

					int val_length = MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, ii->c_total_calls, -1, val, val_length );

					ii->total_calls = val;

					ii->count = 0;

					ii->state = 0;

					// Update all nodes if it already exits.
					DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
					if ( dll != NULL )
					{
						DoublyLinkedList *node = dll;
						while ( node != NULL )
						{
							displayinfo *mdi = ( displayinfo * )node->data;

							if ( mdi->ignore == false )
							{
								mdi->ignore = true;
								GlobalFree( mdi->w_ignore );
								mdi->w_ignore = GlobalStrDupW( ST_Yes );
							}

							node = node->next;
						}
					}

					di->ignore = true;
					GlobalFree( di->w_ignore );
					di->w_ignore = GlobalStrDupW( ST_Yes );

					if ( dllrbt_insert( ignore_list, ( void * )ii->c_phone_number, ( void * )ii ) != DLLRBT_STATUS_OK )
					{
						free_ignoreinfo( &ii );
					}
					else	// Add to ignore list listview as well.
					{
						/*	// Wildcard values shouldn't appear in the call log listview.
						// See if the value we're adding is a range (has wildcard values in it). Only allow 10 digit numbers.
						if ( ii->c_phone_number != NULL && lstrlenA( ii->c_phone_number ) == 10 && is_num( ii->c_phone_number ) == 1 )
						{
							AddRange( &range_list, ii->c_phone_number );
						}*/

						ignore_list_changed = true;

						LVITEM lvi1;
						_memzero( &lvi1, sizeof( LVITEM ) );
						lvi1.mask = LVIF_PARAM;
						lvi1.iSubItem = 0;
						lvi1.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi1.lParam = ( LPARAM )ii;
						_SendMessageW( g_hWnd_ignore_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi1 );
					}
				}
			}

			// If we removed items in the call log listview from the ignore_list, then remove them from the ignore list listview.
			if ( remove_state_changed == true )
			{
				int item_count = _SendMessageW( g_hWnd_ignore_list, LVM_GETITEMCOUNT, 0, 0 );

				skip_ignore_draw = true;
				_EnableWindow( g_hWnd_ignore_list, FALSE );

				// Start from the end and work backwards.
				for ( int i = item_count - 1; i >= 0; --i )
				{
					lvi.iItem = i;
					_SendMessageW( g_hWnd_ignore_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					ignoreinfo *ii = ( ignoreinfo * )lvi.lParam;

					if ( ii != NULL && ii->state == 1 )
					{
						free_ignoreinfo( &ii );

						_SendMessageW( g_hWnd_ignore_list, LVM_DELETEITEM, i, 0 );
					}
				}

				skip_ignore_draw = false;
				_EnableWindow( g_hWnd_ignore_list, TRUE );
			}
		}

		GlobalFree( iui );
	}

	Processing_Window( hWnd, true );

	// Release the mutex if we're killing the thread.
	if ( worker_mutex != NULL )
	{
		ReleaseSemaphore( worker_mutex, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}





THREAD_RETURN update_forward_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	HWND hWnd = NULL;
	forwardupdateinfo *fui = ( forwardupdateinfo * )pArguments;
	if ( fui != NULL )
	{
		hWnd = fui->hWnd;
	}

	Processing_Window( hWnd, false );

	if ( fui != NULL )
	{
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

		if ( fui->hWnd == g_hWnd_forward_list )
		{
			if ( fui->action == 0 )	// Add a single item to the forward_list and forward list listview.
			{
				forwardinfo *fi = ( forwardinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardinfo ) );

				fi->call_from = FormatPhoneNumber( fui->call_from );
				fi->c_call_from = fui->call_from;

				fi->forward_to = FormatPhoneNumber( fui->forward_to );
				fi->c_forward_to = fui->forward_to;

				fi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
				*fi->c_total_calls = '0';
				*( fi->c_total_calls + 1 ) = 0;	// Sanity

				int val_length = MultiByteToWideChar( CP_UTF8, 0, fi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
				wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
				MultiByteToWideChar( CP_UTF8, 0, fi->c_total_calls, -1, val, val_length );

				fi->total_calls = val;

				fi->count = 0;

				fi->state = 0;

				// Try to insert the forward_list info in the tree.
				if ( dllrbt_insert( forward_list, ( void * )fi->c_call_from, ( void * )fi ) != DLLRBT_STATUS_OK )
				{
					free_forwardinfo( &fi );

					_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_already_in_forward_list );
				}
				else	// If it was able to be inserted into the tree, then update our displayinfo items and the forward list listview.
				{
					// See if the value we're adding is a range (has wildcard values in it).
					if ( fi->c_call_from != NULL && is_num( fi->c_call_from ) == 1 )
					{
						int phone_number_length = lstrlenA( fi->c_call_from );

						RangeAdd( &forward_range_list[ ( phone_number_length > 0 ? phone_number_length - 1 : 0 ) ], fi->c_call_from, phone_number_length );

						// Update each displayinfo item to indicate that it is now forwarded.
						node_type *node = dllrbt_get_head( call_log );
						while ( node != NULL )
						{
							DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								// Process values that are not set to be forwarded. See if the value falls within our range.
								if ( mdi->forward == false && RangeCompare( fi->c_call_from, mdi->ci.call_from ) == true )
								{
									mdi->forward = true;
									GlobalFree( mdi->w_forward );
									mdi->w_forward = GlobalStrDupW( ST_Yes );
								}

								di_node = di_node->next;
							}

							node = node->next;
						}
					}
					else
					{
						// Update each displayinfo item to indicate that it is now forwarded.
						DoublyLinkedList *ll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )fi->c_call_from, true );
						if ( ll != NULL )
						{
							DoublyLinkedList *di_node = ll;
							while ( di_node != NULL )
							{
								displayinfo *mdi = ( displayinfo * )di_node->data;

								if ( mdi->forward == false )
								{
									mdi->forward = true;
									GlobalFree( mdi->w_forward );
									mdi->w_forward = GlobalStrDupW( ST_Yes );

									// This doesn't need to be updated.
									/*if ( mdi->ci.forward_to != NULL )
									{
										GlobalFree( mdi->ci.forward_to );
										mdi->ci.forward_to = GlobalStrDupA( fi->c_forward_to );
									}*/
								}

								di_node = di_node->next;
							}
						}
					}

					forward_list_changed = true;

					LVITEM lvi1;
					_memzero( &lvi1, sizeof( LVITEM ) );
					lvi1.mask = LVIF_PARAM;
					lvi1.iSubItem = 0;
					lvi1.iItem = _SendMessageW( g_hWnd_forward_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi1.lParam = ( LPARAM )fi;	// lParam = our contactinfo structure from the connection thread.
					_SendMessageW( g_hWnd_forward_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi1 );
				}
			}
			else if ( fui->action == 1 )	// Remove from forward_list and forward list listview.
			{
				removeinfo *ri = ( removeinfo * )GlobalAlloc( GMEM_FIXED, sizeof( removeinfo ) );
				ri->disable_critical_section = true;
				ri->hWnd = g_hWnd_forward_list;

				// ri will be freed in the remove_items thread.
				HANDLE hThread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )ri, 0, NULL );

				if ( hThread != NULL )
				{
					WaitForSingleObject( hThread, INFINITE );	// Wait at most 10 seconds for the remove_items thread to finish.
					CloseHandle( hThread );
				}
			}
			else if ( fui->action == 2 )	// Add all items in the foward_list to the forward list listview.
			{
				// Insert a row into our listview.
				LVITEM lvi1;
				_memzero( &lvi1, sizeof( LVITEM ) );
				lvi1.mask = LVIF_PARAM; // Our listview items will display the text contained the lParam value.
				lvi1.iSubItem = 0;

				node_type *node = dllrbt_get_head( forward_list );
				while ( node != NULL )
				{
					forwardinfo *fi = ( forwardinfo * )node->val;

					if ( fi != NULL )
					{
						lvi1.iItem = _SendMessageW( g_hWnd_forward_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi1.lParam = ( LPARAM )fi;	// lParam = our contactinfo structure from the connection thread.
						_SendMessageW( g_hWnd_forward_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi1 );
					}

					node = node->next;
				}
			}
			else if ( fui->action == 3 )	// Update entry.
			{
				// See if the phone number we want to update is in our forward_list.
				dllrbt_iterator *itr = dllrbt_find( forward_list, ( void * )fui->call_from, false );
				if ( itr != NULL )
				{
					forwardinfo *fi = ( forwardinfo * )( ( node_type * )itr )->val;

					if ( fi != NULL )
					{
						GlobalFree( fi->c_forward_to );
						GlobalFree( fi->forward_to );

						fi->c_forward_to = fui->forward_to;
						fi->forward_to = FormatPhoneNumber( fui->forward_to );

						forward_list_changed = true;	// Makes us save the new forward_list.
					}
					else	// This shouldn't happen.
					{
						GlobalFree( fui->forward_to );
					}
				}
				else	// If it's not then don't add.
				{
					GlobalFree( fui->forward_to );

					_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_not_found_in_forward_list );
				}

				GlobalFree( fui->call_from );	// We don't need this anymore. fui is freed at the end of this thread.
			}
			else if ( fui->action == 4 )	// Update the recently called number's call count.
			{
				if ( fui->fi != NULL )
				{
					fui->fi->count++;

					GlobalFree( fui->fi->c_total_calls );

					fui->fi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 11 );
					__snprintf( fui->fi->c_total_calls, 11, "%lu", fui->fi->count );

					GlobalFree( fui->fi->total_calls );

					int val_length = MultiByteToWideChar( CP_UTF8, 0, fui->fi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, fui->fi->c_total_calls, -1, val, val_length );

					fui->fi->total_calls = val;

					forward_list_changed = true;
				}
			}

			_InvalidateRect( g_hWnd_list, NULL, TRUE );
		}
		else if ( fui->hWnd == g_hWnd_list )	// call log listview
		{
			int item_count = _SendMessageW( g_hWnd_list, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = _SendMessageW( g_hWnd_list, LVM_GETSELECTEDCOUNT, 0, 0 );
			
			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				item_count = sel_count;
			}

			bool remove_state_changed = false;	// If true, then go through the forward list listview and remove the items that have changed state.
			bool skip_range_warning = false;	// Skip the range warning if we've already displayed it.

			// Go through each item in the listview.
			for ( int i = 0; i < item_count; ++i )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thead == true )
				{
					break;
				}

				if ( handle_all == true )
				{
					lvi.iItem = i;
				}
				else
				{
					lvi.iItem = _SendMessageW( g_hWnd_list, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
				}

				_SendMessageW( g_hWnd_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

				displayinfo *di = ( displayinfo * )lvi.lParam;

				if ( di->forward == true && fui->action == 1 )		// Remove from forward_list.
				{
					char range_number[ 32 ];
					_memzero( range_number, 32 );

					int range_index = lstrlenA( di->ci.call_from );
					range_index = ( range_index > 0 ? range_index - 1 : 0 );

					// First, see if the phone number is in our range list.
					if ( RangeSearch( &forward_range_list[ range_index ], di->ci.call_from, range_number ) == false )
					{
						// If not, then see if the phone number is in our forward_list.
						dllrbt_iterator *itr = dllrbt_find( forward_list, ( void * )di->ci.call_from, false );
						if ( itr != NULL )
						{
							// Update each displayinfo item to indicate that it is no longer forwarded.
							DoublyLinkedList *ll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
							if ( ll != NULL )
							{
								DoublyLinkedList *di_node = ll;
								while ( di_node != NULL )
								{
									displayinfo *mdi = ( displayinfo * )di_node->data;

									if ( mdi->forward == true )
									{
										mdi->forward = false;
										GlobalFree( mdi->w_forward );
										mdi->w_forward = GlobalStrDupW( ST_No );

										// This doesn't need to be updated.
										/*if ( mdi->ci.forward_to != NULL )
										{
											GlobalFree( mdi->ci.forward_to );
											mdi->ci.forward_to = NULL;
										}*/
									}

									di_node = di_node->next;
								}
							}

							// If a forward list listview item also shows up in the call log listview, then we'll remove it after this loop.
							forwardinfo *fi = ( forwardinfo * )( ( node_type * )itr )->val;
							fi->state = 1;	// Remove item.
							remove_state_changed = true;	// This will let us go through the forward list listview and free items with a state == 1.

							dllrbt_remove( forward_list, itr );	// Remove the node from the tree. The tree will rebalance itself.

							forward_list_changed = true;	// Causes us to save the forward_list on shutdown.
						}

						di->forward = false;
						GlobalFree( di->w_forward );
						di->w_forward = GlobalStrDupW( ST_No );
					}
					else
					{
						if ( skip_range_warning == false )
						{
							_SendNotifyMessageW( g_hWnd_login, WM_ALERT, 0, ( LPARAM )ST_remove_from_forward_list );
							skip_range_warning = true;
						}
					}
				}
				else if ( di->forward == false && fui->action == 0 )	// Add to forward_list.
				{
					forwardinfo *fi = ( forwardinfo * )GlobalAlloc( GMEM_FIXED, sizeof( forwardinfo ) );

					fi->call_from = FormatPhoneNumber( di->ci.call_from );
					fi->c_call_from = GlobalStrDupA( di->ci.call_from );

					fi->forward_to = FormatPhoneNumber( fui->forward_to );
					fi->c_forward_to = fui->forward_to;

					fi->c_total_calls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
					*fi->c_total_calls = '0';
					*( fi->c_total_calls + 1 ) = 0;	// Sanity

					int val_length = MultiByteToWideChar( CP_UTF8, 0, fi->c_total_calls, -1, NULL, 0 );	// Include the NULL terminator.
					wchar_t *val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
					MultiByteToWideChar( CP_UTF8, 0, fi->c_total_calls, -1, val, val_length );

					fi->total_calls = val;

					fi->count = 0;

					fi->state = 0;

					// Update all nodes if it already exits.
					DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
					if ( dll != NULL )
					{
						DoublyLinkedList *node = dll;
						while ( node != NULL )
						{
							displayinfo *mdi = ( displayinfo * )node->data;

							if ( mdi->forward == false )
							{
								mdi->forward = true;
								GlobalFree( mdi->w_forward );
								mdi->w_forward = GlobalStrDupW( ST_Yes );

								// This doesn't need to be updated.
								/*if ( mdi->ci.forward_to != NULL )
								{
									GlobalFree( mdi->ci.forward_to );
									mdi->ci.forward_to = GlobalStrDupA( fi->c_forward_to );
								}*/
							}

							node = node->next;
						}
					}

					di->forward = true;
					GlobalFree( di->w_forward );
					di->w_forward = GlobalStrDupW( ST_Yes );

					if ( dllrbt_insert( forward_list, ( void * )fi->c_call_from, ( void * )fi ) != DLLRBT_STATUS_OK )
					{
						free_forwardinfo( &fi );
					}
					else	// Add to forward list listview as well.
					{
						/*	// Wildcard values shouldn't appear in the call log listview.
						// See if the value we're adding is a range (has wildcard values in it). Only allow 10 digit numbers.
						if ( fi->c_call_from != NULL && lstrlenA( fi->c_call_from ) == 10 && is_num( fi->c_call_from ) == 1 )
						{
							AddRange( &forward_range_list, fi->c_call_from );
						}*/

						forward_list_changed = true;

						LVITEM lvi1;
						_memzero( &lvi1, sizeof( LVITEM ) );
						lvi1.mask = LVIF_PARAM;
						lvi1.iSubItem = 0;
						lvi1.iItem = _SendMessageW( g_hWnd_forward_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi1.lParam = ( LPARAM )fi;
						_SendMessageW( g_hWnd_forward_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi1 );
					}
				}
			}

			// If we removed items in the call log listview from the forward_list, then remove them from the forward list listview.
			if ( remove_state_changed == true )
			{
				int item_count = _SendMessageW( g_hWnd_forward_list, LVM_GETITEMCOUNT, 0, 0 );

				skip_forward_draw = true;
				_EnableWindow( g_hWnd_forward_list, FALSE );

				// Start from the end and work backwards.
				for ( int i = item_count - 1; i >= 0; --i )
				{
					lvi.iItem = i;
					_SendMessageW( g_hWnd_forward_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					forwardinfo *fi = ( forwardinfo * )lvi.lParam;

					if ( fi != NULL && fi->state == 1 )
					{
						free_forwardinfo( &fi );

						_SendMessageW( g_hWnd_forward_list, LVM_DELETEITEM, i, 0 );
					}
				}

				skip_forward_draw = false;
				_EnableWindow( g_hWnd_forward_list, TRUE );
			}
		}

		GlobalFree( fui );
	}

	Processing_Window( hWnd, true );

	// Release the mutex if we're killing the thread.
	if ( worker_mutex != NULL )
	{
		ReleaseSemaphore( worker_mutex, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

// Allocates a new string if characters need escaping. Otherwise, it returns NULL.
char *escape_csv( const char *string )
{
	char *escaped_string = NULL;
	char *q = NULL;
	const char *p = NULL;
	int c = 0;

	if ( string == NULL )
	{
		return NULL;
	}

	// Get the character count and offset it for any quotes.
	for ( c = 0, p = string; *p != NULL; ++p ) 
	{
		if ( *p != '\"' )
		{
			++c;
		}
		else
		{
			c += 2;
		}
	}

	// If the string has no special characters to escape, then return NULL.
	if ( c <= ( p - string ) )
	{
		return NULL;
	}

	q = escaped_string = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( c + 1 ) );

	for ( p = string; *p != NULL; ++p ) 
	{
		if ( *p != '\"' )
		{
			*q = *p;
			++q;
		}
		else
		{
			*q++ = '\"';
			*q++ = '\"';
		}
	}

	*q = 0;	// Sanity.

	return escaped_string;
}

THREAD_RETURN save_call_log( void *file_path )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	Processing_Window( g_hWnd_list, false );

	// Open our config file if it exists.
	HANDLE hFile_call_log = CreateFile( ( wchar_t * )file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_call_log != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;
		char unix_timestamp[ 21 ];
		_memzero( unix_timestamp, 21 );

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		// Write the UTF-8 BOM and CSV column titles.
		WriteFile( hFile_call_log, "\xEF\xBB\xBF\"Caller ID\",\"Phone Number\",\"Date and Time\",\"Unix Timestamp\",Ignore,Forward,\"Forwarded to\",\"Sent to\"", 102, &write, NULL );

		node_type *node = dllrbt_get_head( call_log );
		while ( node != NULL )
		{
			// Stop processing and exit the thread.
			if ( kill_worker_thead == true )
			{
				break;
			}

			DoublyLinkedList *di_node = ( DoublyLinkedList * )node->val;
			while ( di_node != NULL )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thead == true )
				{
					break;
				}

				displayinfo *di = ( displayinfo * )di_node->data;

				if ( di != NULL )
				{
					// lstrlen is safe for NULL values.
					int phone_number_length1 = lstrlenA( di->ci.call_from );
					int phone_number_length2 = lstrlenA( di->ci.forward_to );
					int phone_number_length3 = lstrlenA( di->ci.call_to );

					wchar_t *w_ignore = ( di->ignore == true ? ST_Yes : ST_No );
					wchar_t *w_forward = ( di->forward == true ? ST_Yes : ST_No );

					int ignore_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore, -1, NULL, 0, NULL, NULL );
					char *utf8_ignore = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ignore_length ); // Size includes the null character.
					ignore_length = WideCharToMultiByte( CP_UTF8, 0, w_ignore, -1, utf8_ignore, ignore_length, NULL, NULL ) - 1;

					int forward_length = WideCharToMultiByte( CP_UTF8, 0, w_forward, -1, NULL, 0, NULL, NULL );
					char *utf8_forward = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * forward_length ); // Size includes the null character.
					forward_length = WideCharToMultiByte( CP_UTF8, 0, w_forward, -1, utf8_forward, forward_length, NULL, NULL ) - 1;

					char *escaped_caller_id = escape_csv( di->ci.caller_id );

					int caller_id_length = lstrlenA( ( escaped_caller_id != NULL ? escaped_caller_id : di->ci.caller_id ) );

					int time_length = WideCharToMultiByte( CP_UTF8, 0, di->w_time, -1, NULL, 0, NULL, NULL );
					char *utf8_time = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * time_length ); // Size includes the null character.
					time_length = WideCharToMultiByte( CP_UTF8, 0, di->w_time, -1, utf8_time, time_length, NULL, NULL ) - 1;

					// Convert the time into a 32bit Unix timestamp.
					LARGE_INTEGER date;
					date.HighPart = di->time.HighPart;
					date.LowPart = di->time.LowPart;

					date.QuadPart -= ( 11644473600000 * 10000 );

					// Divide the 64bit value.
					__asm
					{
						xor edx, edx;				//; Zero out the register so we don't divide a full 64bit value.
						mov eax, date.HighPart;		//; We'll divide the high order bits first.
						mov ecx, FILETIME_TICKS_PER_SECOND;
						div ecx;
						mov date.HighPart, eax;		//; Store the high order quotient.
						mov eax, date.LowPart;		//; Now we'll divide the low order bits.
						div ecx;
						mov date.LowPart, eax;		//; Store the low order quotient.
						//; Any remainder will be stored in edx. We're not interested in it though.
					}

					int timestamp_length = __snprintf( unix_timestamp, 21, "%llu", date.QuadPart );

					// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
					if ( pos + phone_number_length1 + phone_number_length2 + phone_number_length3 + caller_id_length + time_length + timestamp_length + ignore_length + forward_length + 13 > size )
					{
						// Dump the buffer.
						WriteFile( hFile_call_log, write_buf, pos, &write, NULL );
						pos = 0;
					}

					// Add to the buffer.
					write_buf[ pos++ ] = '\r';
					write_buf[ pos++ ] = '\n';

					write_buf[ pos++ ] = '\"';
					_memcpy_s( write_buf + pos, size - pos, ( escaped_caller_id != NULL ? escaped_caller_id : di->ci.caller_id ), caller_id_length );
					pos += caller_id_length;
					write_buf[ pos++ ] = '\"';
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, di->ci.call_from, phone_number_length1 );
					pos += phone_number_length1;
					write_buf[ pos++ ] = ',';

					write_buf[ pos++ ] = '\"';
					_memcpy_s( write_buf + pos, size - pos, utf8_time, time_length );
					pos += time_length;
					write_buf[ pos++ ] = '\"';
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, unix_timestamp, timestamp_length );
					pos += timestamp_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_ignore, ignore_length );
					pos += ignore_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, utf8_forward, forward_length );
					pos += forward_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, di->ci.forward_to, phone_number_length2 );
					pos += phone_number_length2;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, di->ci.call_to, phone_number_length3 );
					pos += phone_number_length3;

					GlobalFree( utf8_forward );
					GlobalFree( utf8_ignore );
					GlobalFree( utf8_time );
					GlobalFree( escaped_caller_id );
				}

				di_node = di_node->next;
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile_call_log, write_buf, pos, &write, NULL );
		}

		GlobalFree( write_buf );

		CloseHandle( hFile_call_log );
	}

	GlobalFree( file_path );

	Processing_Window( g_hWnd_list, true );

	// Release the mutex if we're killing the thread.
	if ( worker_mutex != NULL )
	{
		ReleaseSemaphore( worker_mutex, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN read_call_log_history( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &pe_cs );

	in_worker_thread = true;

	Processing_Window( g_hWnd_list, false );

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\call_log_history.bin\0", 22 );
	base_directory[ base_directory_length + 21 ] = 0;	// Sanity.

	// Open our config file if it exists.
	HANDLE hFile_read = CreateFile( base_directory, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_read != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		bool ignored = false;
		bool forwarded = false;
		LONGLONG time = 0;
		char *caller_id = NULL;
		char *call_from = NULL;
		char *call_to = NULL;
		char *forward_to = NULL;
		char *call_reference_id = NULL;

		char range_number[ 32 ];

		DWORD fz = GetFileSize( hFile_read, NULL );

		char *history_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 1 ) );	// 512 KB buffer.

		while ( total_read < fz )
		{
			// Stop processing and exit the thread.
			if ( kill_worker_thead == true )
			{
				break;
			}

			ReadFile( hFile_read, history_buf, sizeof( char ) * 524288, &read, NULL );

			history_buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

			// Make sure that we have at least part of the entry. 5 = 5 NULL strings. This is the minimum size an entry could be.
			if ( read < ( ( sizeof( bool ) * 2 ) + sizeof( LONGLONG ) + 5 ) )
			{
				break;
			}

			total_read += read;

			// Prevent an infinite loop if a really really long entry causes us to jump back to the same point in the file.
			// If it's larger than our buffer, then the file is probably invalid/corrupt.
			if ( total_read == last_total )
			{
				break;
			}

			last_total = total_read;

			p = history_buf;
			offset = last_entry = 0;

			while ( offset < read )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thead == true )
				{
					break;
				}

				caller_id = NULL;
				call_from = NULL;
				call_to = NULL;
				forward_to = NULL;
				call_reference_id = NULL;

				// Ignored state
				offset += sizeof( bool );
				if ( offset >= read ) { goto CLEANUP; }
				_memcpy_s( &ignored, sizeof( bool ), p, sizeof( bool ) );
				p += sizeof( bool );

				// Forwarded state
				offset += sizeof( bool );
				if ( offset >= read ) { goto CLEANUP; }
				_memcpy_s( &forwarded, sizeof( bool ), p, sizeof( bool ) );
				p += sizeof( bool );

				// Call time
				offset += sizeof( LONGLONG );
				if ( offset >= read ) { goto CLEANUP; }
				_memcpy_s( &time, sizeof( LONGLONG ), p, sizeof( LONGLONG ) );
				p += sizeof( LONGLONG );

				// Caller ID
				int string_length = lstrlenA( p ) + 1;

				offset += string_length;
				if ( offset >= read ) { goto CLEANUP; }

				caller_id = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
				_memcpy_s( caller_id, string_length, p, string_length );
				*( caller_id + ( string_length - 1 ) ) = 0;	// Sanity

				p += string_length;

				// Incoming number
				string_length = lstrlenA( p );

				offset += ( string_length + 1 );
				if ( offset >= read ) { goto CLEANUP; }

				int call_from_length = min( string_length, 16 );
				int adjusted_size = call_from_length + 1;

				call_from = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * adjusted_size );
				_memcpy_s( call_from, adjusted_size, p, adjusted_size );
				*( call_from + ( adjusted_size - 1 ) ) = 0;	// Sanity

				p += ( string_length + 1 );

				// The number that was called (our number)
				string_length = lstrlenA( p );

				offset += ( string_length + 1 );
				if ( offset >= read ) { goto CLEANUP; }

				adjusted_size = min( string_length, 16 ) + 1;

				call_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * adjusted_size );
				_memcpy_s( call_to, adjusted_size, p, adjusted_size );
				*( call_to + ( adjusted_size - 1 ) ) = 0;	// Sanity

				p += ( string_length + 1 );

				// The number that the incoming call was forwarded to
				string_length = lstrlenA( p );

				offset += ( string_length + 1 );
				if ( offset >= read ) { goto CLEANUP; }

				adjusted_size = min( string_length, 16 ) + 1;

				forward_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * adjusted_size );
				_memcpy_s( forward_to, adjusted_size, p, adjusted_size );
				*( forward_to + ( adjusted_size - 1 ) ) = 0;	// Sanity

				p += ( string_length + 1 );

				// Server reference ID
				string_length = lstrlenA( p ) + 1;

				offset += string_length;
				if ( offset > read ) { goto CLEANUP; }	// Offset must equal read for the last string in the file. It's truncated/corrupt if it doesn't.

				call_reference_id = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
				_memcpy_s( call_reference_id, string_length, p, string_length );
				*( call_reference_id + ( string_length - 1 ) ) = 0;	// Sanity

				p += string_length;

				last_entry = offset;	// This value is the ending offset of the last valid entry.

				displayinfo *di = ( displayinfo * )GlobalAlloc( GMEM_FIXED, sizeof( displayinfo ) );

				di->ci.call_to = call_to;
				di->ci.call_from = call_from;
				di->ci.call_reference_id = call_reference_id;
				di->ci.caller_id = caller_id;
				di->ci.forward_to = forward_to;
				di->ci.ignored = ignored;
				di->ci.forwarded = forwarded;
				di->caller_id = NULL;
				di->phone_number = NULL;
				di->reference = NULL;
				di->forward_to = NULL;
				di->sent_to = NULL;
				di->w_forward = NULL;
				di->w_ignore = NULL;
				di->w_time = NULL;
				di->time.QuadPart = time;
				di->process_incoming = false;
				di->ignore = false;
				di->forward = false;

				// Create the node to insert into a linked list.
				DoublyLinkedList *di_node = DLL_CreateNode( ( void * )di );

				// See if our tree has the phone number to add the node to.
				DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )di->ci.call_from, true );
				if ( dll == NULL )
				{
					// If no phone number exits, insert the node into the tree.
					if ( dllrbt_insert( call_log, ( void * )di->ci.call_from, ( void * )di_node ) != DLLRBT_STATUS_OK )
					{
						GlobalFree( di_node );	// This shouldn't happen.
					}
				}
				else	// If a phone number exits, insert the node into the linked list.
				{
					DLL_AddNode( &dll, di_node, -1 );	// Insert at the end of the doubly linked list.
				}

				// Search the ignore_list for a match.
				ignoreinfo *ii = ( ignoreinfo * )dllrbt_find( ignore_list, ( void * )di->ci.call_from, true );

				// Try searching the range list.
				if ( ii == NULL )
				{
					_memzero( range_number, 32 );

					int range_index = call_from_length;
					range_index = ( range_index > 0 ? range_index - 1 : 0 );

					if ( RangeSearch( &ignore_range_list[ range_index ], di->ci.call_from, range_number ) == true )
					{
						ii = ( ignoreinfo * )dllrbt_find( ignore_list, ( void * )range_number, true );
					}
				}

				if ( ii != NULL )
				{
					di->ignore = true;
				}

				// Search the forward list for a match.
				forwardinfo *fi = ( forwardinfo * )dllrbt_find( forward_list, ( void * )di->ci.call_from, true );

				// Try searching the range list.
				if ( fi == NULL )
				{
					_memzero( range_number, 32 );

					int range_index = call_from_length;
					range_index = ( range_index > 0 ? range_index - 1 : 0 );

					if ( RangeSearch( &forward_range_list[ range_index ], di->ci.call_from, range_number ) == true )
					{
						fi = ( forwardinfo * )dllrbt_find( forward_list, ( void * )range_number, true );
					}
				}

				if ( fi != NULL )
				{
					di->forward = true;
				}

				_SendNotifyMessageW( g_hWnd_main, WM_PROPAGATE, MAKEWPARAM( CW_MODIFY, 1 ), ( LPARAM )di );	// Add entry to listview and don't show popup.

				continue;

CLEANUP:
				GlobalFree( caller_id );
				GlobalFree( call_from );
				GlobalFree( call_to );
				GlobalFree( forward_to );
				GlobalFree( call_reference_id );

				// Go back to the last valid entry.
				if ( total_read < fz )
				{
					total_read -= ( read - last_entry );
					SetFilePointer( hFile_read, total_read, NULL, FILE_BEGIN );
				}

				break;
			}
		}

		GlobalFree( history_buf );

		CloseHandle( hFile_read );
	}

	Processing_Window( g_hWnd_list, true );

	// Release the mutex if we're killing the thread.
	if ( worker_mutex != NULL )
	{
		ReleaseSemaphore( worker_mutex, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &pe_cs );

	_ExitThread( 0 );
	return 0;
}
