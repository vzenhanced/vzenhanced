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
#include "utilities.h"
#include "message_log_utilities.h"
#include "menus.h"
#include "string_tables.h"
#include "lite_gdi32.h"

#define CID_LIST_COUNT	6

HANDLE worker_semaphore = NULL;			// Blocks shutdown while a worker thread is active.
bool kill_worker_thread_flag = false;	// Allow for a clean shutdown.

CRITICAL_SECTION pe_cs;					// Queues additional worker threads.

bool in_worker_thread = false;			// Flag to indicate that we're in a worker thread.

dllrbt_tree *call_log = NULL;
bool call_log_changed = false;

dllrbt_tree **custom_cid_list = NULL;

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

bool cfg_enable_call_log_history = true;

bool cfg_enable_message_log = false;

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
int cfg_column_width4 = 140;
int cfg_column_width5 = 140;
int cfg_column_width6 = 100;
int cfg_column_width7 = 130;
int cfg_column_width8 = 130;
int cfg_column_width9 = 100;
int cfg_column_width10 = 300;
int cfg_column_width11 = 100;

// Column / Virtual position
char cfg_column_order1 = 0;		// 0 # (always 0)
char cfg_column_order2 = 1;		// 1 Caller ID
char cfg_column_order3 = 3;		// 2 Date and Time
char cfg_column_order4 = -1;	// 3 Forwarded Caller ID
char cfg_column_order5 = -1;	// 4 Forwarded Phone Number
char cfg_column_order6 = -1;	// 5 Forward to Phone Number
char cfg_column_order7 = -1;	// 6 Ignored Caller ID
char cfg_column_order8 = -1;	// 7 Ignored Phone Number
char cfg_column_order9 = 2;		// 8 Phone Number
char cfg_column_order10 = -1;	// 9 Reference
char cfg_column_order11 = -1;	// 10 Sent to Phone Number

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

int cfg_column5_width1 = 35;
int cfg_column5_width2 = 100;
int cfg_column5_width3 = 100;
int cfg_column5_width4 = 75;
int cfg_column5_width5 = 110;
int cfg_column5_width6 = 70;

char cfg_column5_order1 = 0;	// 0 # (always 0)
char cfg_column5_order2 = 1;	// 1 Caller ID
char cfg_column5_order3 = 2;	// 2 Forward to
char cfg_column5_order4 = 3;	// 3 Match Case
char cfg_column5_order5 = 4;	// 4 Match Whole Word
char cfg_column5_order6 = 5;	// 5 Total Calls

int cfg_column6_width1 = 35;
int cfg_column6_width2 = 100;
int cfg_column6_width3 = 75;
int cfg_column6_width4 = 110;
int cfg_column6_width5 = 70;

char cfg_column6_order1 = 0;	// 0 # (always 0)
char cfg_column6_order2 = 1;	// 1 Caller ID
char cfg_column6_order3 = 2;	// 2 Match Case
char cfg_column6_order4 = 3;	// 3 Match Whole Word
char cfg_column6_order5 = 4;	// 4 Total Calls

bool cfg_connection_auto_login = false;
bool cfg_connection_reconnect = true;
bool cfg_download_pictures = true;
unsigned char cfg_connection_retries = 3;

unsigned short cfg_connection_timeout = 60;	// Seconds.

unsigned char cfg_connection_ssl_version = 2;	// TLS 1.0

char *tab_order[ NUM_TABS ] = { &cfg_tab_order1, &cfg_tab_order2, &cfg_tab_order3, &cfg_tab_order4 };

char *call_log_columns[ NUM_COLUMNS1 ] =	 { &cfg_column_order1, &cfg_column_order2, &cfg_column_order3, &cfg_column_order4, &cfg_column_order5, &cfg_column_order6, &cfg_column_order7, &cfg_column_order8, &cfg_column_order9, &cfg_column_order10, &cfg_column_order11 };
char *contact_list_columns[ NUM_COLUMNS2 ] = { &cfg_column2_order1, &cfg_column2_order2, &cfg_column2_order3, &cfg_column2_order4, &cfg_column2_order5, &cfg_column2_order6, &cfg_column2_order7, &cfg_column2_order8, &cfg_column2_order9,
											   &cfg_column2_order10, &cfg_column2_order11, &cfg_column2_order12, &cfg_column2_order13, &cfg_column2_order14, &cfg_column2_order15, &cfg_column2_order16, &cfg_column2_order17 };
char *forward_list_columns[ NUM_COLUMNS3 ] = { &cfg_column3_order1, &cfg_column3_order2, &cfg_column3_order3, &cfg_column3_order4 };
char *ignore_list_columns[ NUM_COLUMNS4 ] =  { &cfg_column4_order1, &cfg_column4_order2, &cfg_column4_order3 };
char *forward_cid_list_columns[ NUM_COLUMNS5 ] = { &cfg_column5_order1, &cfg_column5_order2, &cfg_column5_order3, &cfg_column5_order4, &cfg_column5_order5, &cfg_column5_order6 };
char *ignore_cid_list_columns[ NUM_COLUMNS6 ] =  { &cfg_column6_order1, &cfg_column6_order2, &cfg_column6_order3, &cfg_column6_order4, &cfg_column6_order5 };


int *call_log_columns_width[ NUM_COLUMNS1 ] =	  { &cfg_column_width1, &cfg_column_width2, &cfg_column_width3, &cfg_column_width4, &cfg_column_width5, &cfg_column_width6, &cfg_column_width7, &cfg_column_width8, &cfg_column_width9, &cfg_column_width10, &cfg_column_width11 };
int *contact_list_columns_width[ NUM_COLUMNS2 ] = { &cfg_column2_width1, &cfg_column2_width2, &cfg_column2_width3, &cfg_column2_width4, &cfg_column2_width5, &cfg_column2_width6, &cfg_column2_width7, &cfg_column2_width8, &cfg_column2_width9,
													&cfg_column2_width10, &cfg_column2_width11, &cfg_column2_width12, &cfg_column2_width13, &cfg_column2_width14, &cfg_column2_width15, &cfg_column2_width16, &cfg_column2_width17 };
int *forward_list_columns_width[ NUM_COLUMNS3 ] = { &cfg_column3_width1, &cfg_column3_width2, &cfg_column3_width3, &cfg_column3_width4 };
int *ignore_list_columns_width[ NUM_COLUMNS4 ] =  { &cfg_column4_width1, &cfg_column4_width2, &cfg_column4_width3 };
int *forward_cid_list_columns_width[ NUM_COLUMNS5 ] = { &cfg_column5_width1, &cfg_column5_width2, &cfg_column5_width3, &cfg_column5_width4, &cfg_column5_width5, &cfg_column5_width6 };
int *ignore_cid_list_columns_width[ NUM_COLUMNS6 ] =  { &cfg_column6_width1, &cfg_column6_width2, &cfg_column6_width3, &cfg_column6_width4, &cfg_column6_width5 };

//unsigned short bad_area_codes[ 29 ] = { 211, 242, 246, 264, 268, 284, 311, 345, 411, 441, 473, 511, 611, 649, 664, 711, 758, 767, 784, 809, 811, 829, 849, 868, 869, 876, 900, 911, 976 };

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

int dllrbt_icid_compare( void *a, void *b )
{
	ignorecidinfo *a1 = ( ignorecidinfo * )a;
	ignorecidinfo *b1 = ( ignorecidinfo * )b;

	int ret = lstrcmpA( a1->c_caller_id, b1->c_caller_id );

	// See if the strings match.
	if ( ret == 0 )
	{
		// If they do, then check if the booleans differ.
		if ( a1->match_case == true && b1->match_case == false )
		{
			ret = 1;
		}
		else if ( a1->match_case == false && b1->match_case == true )
		{
			ret = -1;
		}
		else
		{
			if ( a1->match_whole_word == true && b1->match_whole_word == false )
			{
				ret = 1;
			}
			else if ( a1->match_whole_word == false && b1->match_whole_word == true )
			{
				ret = -1;
			}
		}
	}

	return ret;
}

int dllrbt_fcid_compare( void *a, void *b )
{
	forwardcidinfo *a1 = ( forwardcidinfo * )a;
	forwardcidinfo *b1 = ( forwardcidinfo * )b;

	int ret = lstrcmpA( a1->c_caller_id, b1->c_caller_id );

	// See if the strings match.
	if ( ret == 0 )
	{
		// If they do, then check if the booleans differ.
		if ( a1->match_case == true && b1->match_case == false )
		{
			ret = 1;
		}
		else if ( a1->match_case == false && b1->match_case == true )
		{
			ret = -1;
		}
		else
		{
			if ( a1->match_whole_word == true && b1->match_whole_word == false )
			{
				ret = 1;
			}
			else if ( a1->match_whole_word == false && b1->match_whole_word == true )
			{
				ret = -1;
			}
		}
	}

	return ret;
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
	GlobalFree( ( *di )->w_forward_caller_id );
	GlobalFree( ( *di )->w_forward_phone_number );
	GlobalFree( ( *di )->w_ignore_caller_id );
	GlobalFree( ( *di )->w_ignore_phone_number );
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

void free_forwardcidinfo( forwardcidinfo **fcidi )
{
	GlobalFree( ( *fcidi )->caller_id );
	GlobalFree( ( *fcidi )->forward_to );
	GlobalFree( ( *fcidi )->w_match_case );
	GlobalFree( ( *fcidi )->w_match_whole_word );
	GlobalFree( ( *fcidi )->total_calls );
	GlobalFree( ( *fcidi )->c_caller_id );
	GlobalFree( ( *fcidi )->c_forward_to );
	GlobalFree( ( *fcidi )->c_match_case );
	GlobalFree( ( *fcidi )->c_match_whole_word );
	GlobalFree( ( *fcidi )->c_total_calls );
	GlobalFree( *fcidi );
	*fcidi = NULL;
}

void free_ignorecidinfo( ignorecidinfo **icidi )
{
	GlobalFree( ( *icidi )->caller_id );
	GlobalFree( ( *icidi )->w_match_case );
	GlobalFree( ( *icidi )->w_match_whole_word );
	GlobalFree( ( *icidi )->total_calls );
	GlobalFree( ( *icidi )->c_caller_id );
	GlobalFree( ( *icidi )->c_match_case );
	GlobalFree( ( *icidi )->c_match_whole_word );
	GlobalFree( ( *icidi )->c_total_calls );
	GlobalFree( *icidi );
	*icidi = NULL;
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
	_SendMessageW( g_hWnd_call_log, LVM_GETCOLUMNORDERARRAY, total_columns1, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS1; ++i )
	{
		if ( *call_log_columns[ i ] != -1 )
		{
			*call_log_columns[ i ] = ( char )arr[ offset++ ];
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

	offset = 0;
	_SendMessageW( g_hWnd_forward_cid_list, LVM_GETCOLUMNORDERARRAY, total_columns5, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS5; ++i )
	{
		if ( *forward_cid_list_columns[ i ] != -1 )
		{
			*forward_cid_list_columns[ i ] = ( char )arr[ offset++ ];
		}
	}

	offset = 0;
	_SendMessageW( g_hWnd_ignore_cid_list, LVM_GETCOLUMNORDERARRAY, total_columns6, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS6; ++i )
	{
		if ( *ignore_cid_list_columns[ i ] != -1 )
		{
			*ignore_cid_list_columns[ i ] = ( char )arr[ offset++ ];
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
			cfg_column_order4 = -1;		// Forwarded Caller ID
			cfg_column_order5 = -1;		// Forwarded Phone Number
			cfg_column_order6 = -1;		// Forward to Phone Number
			cfg_column_order7 = -1;		// Ignored Caller ID
			cfg_column_order8 = -1;		// Ignored Phone Number
			cfg_column_order9 = 2;		// Phone Number
			cfg_column_order10 = -1;	// Reference
			cfg_column_order11 = -1;	// Sent to Phone Number
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

		case 4:
		{
			cfg_column5_order1 = 0;	// 0 # (always 0)
			cfg_column5_order2 = 1;	// 1 Caller ID
			cfg_column5_order3 = 2;	// 2 Forward to
			cfg_column5_order4 = 3;	// 3 Match Case
			cfg_column5_order5 = 4;	// 4 Match Whole Word
			cfg_column5_order6 = 5;	// 5 Total Calls
		}
		break;

		case 5:
		{
			cfg_column6_order1 = 0;	// 0 # (always 0)
			cfg_column6_order2 = 1;	// 1 Caller ID
			cfg_column6_order3 = 2;	// 2 Match Case
			cfg_column6_order4 = 3;	// 3 Match Whole Word
			cfg_column6_order5 = 4;	// 4 Total Calls
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
	if ( cfg_column_width4 < 0 || cfg_column_width4 > 2560 ) { cfg_column_width4 = 140; }
	if ( cfg_column_width5 < 0 || cfg_column_width5 > 2560 ) { cfg_column_width5 = 140; }
	if ( cfg_column_width6 < 0 || cfg_column_width6 > 2560 ) { cfg_column_width6 = 100; }
	if ( cfg_column_width7 < 0 || cfg_column_width7 > 2560 ) { cfg_column_width7 = 130; }
	if ( cfg_column_width8 < 0 || cfg_column_width8 > 2560 ) { cfg_column_width8 = 130; }
	if ( cfg_column_width9 < 0 || cfg_column_width9 > 2560 ) { cfg_column_width9 = 100; }
	if ( cfg_column_width10 < 0 || cfg_column_width10 > 2560 ) { cfg_column_width10 = 300; }
	if ( cfg_column_width11 < 0 || cfg_column_width11 > 2560 ) { cfg_column_width11 = 100; }

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

	if ( cfg_column5_width1 < 0 || cfg_column5_width1 > 2560 ) { cfg_column5_width1 = 35; }
	if ( cfg_column5_width2 < 0 || cfg_column5_width2 > 2560 ) { cfg_column5_width2 = 100; }
	if ( cfg_column5_width3 < 0 || cfg_column5_width3 > 2560 ) { cfg_column5_width3 = 100; }
	if ( cfg_column5_width4 < 0 || cfg_column5_width4 > 2560 ) { cfg_column5_width4 = 75; }
	if ( cfg_column5_width5 < 0 || cfg_column5_width5 > 2560 ) { cfg_column5_width5 = 110; }
	if ( cfg_column5_width6 < 0 || cfg_column5_width6 > 2560 ) { cfg_column5_width6 = 70; }

	if ( cfg_column6_width1 < 0 || cfg_column6_width1 > 2560 ) { cfg_column6_width1 = 35; }
	if ( cfg_column6_width2 < 0 || cfg_column6_width2 > 2560 ) { cfg_column6_width2 = 100; }
	if ( cfg_column6_width3 < 0 || cfg_column6_width3 > 2560 ) { cfg_column6_width3 = 75; }
	if ( cfg_column6_width4 < 0 || cfg_column6_width4 > 2560 ) { cfg_column6_width4 = 110; }
	if ( cfg_column6_width5 < 0 || cfg_column6_width5 > 2560 ) { cfg_column6_width5 = 70; }
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

HWND GetCurrentListView()
{
	HWND hWnd = NULL;

	int index = _SendMessageW( g_hWnd_tab, TCM_GETCURSEL, 0, 0 );	// Get the selected tab
	if ( index != -1 )
	{
		TCITEM tie;
		_memzero( &tie, sizeof( TCITEM ) );
		tie.mask = TCIF_PARAM; // Get the lparam value
		_SendMessageW( g_hWnd_tab, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information

		if ( ( HWND )tie.lParam == g_hWnd_ignore_tab || ( HWND )tie.lParam == g_hWnd_forward_tab )
		{
			index = _SendMessageW( ( HWND )tie.lParam, TCM_GETCURSEL, 0, 0 );	// Get the selected tab
			if ( index != -1 )
			{
				_SendMessageW( ( HWND )tie.lParam, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
			}
		}

		hWnd = ( HWND )tie.lParam;
	}

	return hWnd;
}

char *GetSelectedColumnPhoneNumber( unsigned int column_id )
{
	char *phone_number = NULL;

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;

	if ( column_id == MENU_CALL_PHONE_COL15 ||
		 column_id == MENU_CALL_PHONE_COL18 ||
		 column_id == MENU_CALL_PHONE_COL110 )
	{
		lvi.iItem = _SendMessageW( g_hWnd_call_log, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

		if ( lvi.iItem != -1 )
		{
			_SendMessageW( g_hWnd_call_log, LVM_GETITEM, 0, ( LPARAM )&lvi );

			switch ( column_id )
			{
				case MENU_CALL_PHONE_COL15: { phone_number = ( ( displayinfo * )lvi.lParam )->ci.forward_to; } break;
				case MENU_CALL_PHONE_COL18: { phone_number = ( ( displayinfo * )lvi.lParam )->ci.call_from; } break;
				case MENU_CALL_PHONE_COL110: { phone_number = ( ( displayinfo * )lvi.lParam )->ci.call_to; } break;
			}
		}
	}
	else if ( column_id == MENU_CALL_PHONE_COL21 ||
			  column_id == MENU_CALL_PHONE_COL25 ||
			  column_id == MENU_CALL_PHONE_COL27 ||
			  column_id == MENU_CALL_PHONE_COL211 ||
			  column_id == MENU_CALL_PHONE_COL212 || 
			  column_id == MENU_CALL_PHONE_COL216 )
	{
		lvi.iItem = _SendMessageW( g_hWnd_contact_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

		if ( lvi.iItem != -1 )
		{
			_SendMessageW( g_hWnd_contact_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

			switch ( column_id )
			{
				case MENU_CALL_PHONE_COL21: { phone_number = ( ( contactinfo * )lvi.lParam )->contact.cell_phone_number; } break;
				case MENU_CALL_PHONE_COL25: { phone_number = ( ( contactinfo * )lvi.lParam )->contact.fax_number; } break;
				case MENU_CALL_PHONE_COL27: { phone_number = ( ( contactinfo * )lvi.lParam )->contact.home_phone_number; } break;
				case MENU_CALL_PHONE_COL211: { phone_number = ( ( contactinfo * )lvi.lParam )->contact.office_phone_number; } break;
				case MENU_CALL_PHONE_COL212: { phone_number = ( ( contactinfo * )lvi.lParam )->contact.other_phone_number; } break;
				case MENU_CALL_PHONE_COL216: { phone_number = ( ( contactinfo * )lvi.lParam )->contact.work_phone_number; } break;
			}
		}
	}
	else if ( column_id == MENU_CALL_PHONE_COL31 ||
			  column_id == MENU_CALL_PHONE_COL32 )
	{
		lvi.iItem = _SendMessageW( g_hWnd_forward_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

		if ( lvi.iItem != -1 )
		{
			_SendMessageW( g_hWnd_forward_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

			switch ( column_id )
			{
				case MENU_CALL_PHONE_COL31: { phone_number = ( ( forwardinfo * )lvi.lParam )->c_forward_to; } break;
				case MENU_CALL_PHONE_COL32: { phone_number = ( ( forwardinfo * )lvi.lParam )->c_call_from; } break;
			}
		}
	}
	else if ( column_id == MENU_CALL_PHONE_COL41 )
	{
		lvi.iItem = _SendMessageW( g_hWnd_ignore_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

		if ( lvi.iItem != -1 )
		{
			_SendMessageW( g_hWnd_ignore_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

			phone_number = ( ( ignoreinfo * )lvi.lParam )->c_phone_number;
		}
	}
	else if ( column_id == MENU_CALL_PHONE_COL52 )
	{
		lvi.iItem = _SendMessageW( g_hWnd_forward_cid_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

		if ( lvi.iItem != -1 )
		{
			_SendMessageW( g_hWnd_forward_cid_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

			phone_number = ( ( forwardcidinfo * )lvi.lParam )->c_forward_to;
		}
	}

	return phone_number;
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

	SHARED_SETTINGS *shared_settings = ( SHARED_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( SHARED_SETTINGS ) );
	shared_settings->popup_gradient = cfg_popup_gradient;
	shared_settings->popup_gradient_direction = cfg_popup_gradient_direction;
	shared_settings->popup_background_color1 = cfg_popup_background_color1;
	shared_settings->popup_background_color2 = cfg_popup_background_color2;
	shared_settings->popup_time = cfg_popup_time;
	shared_settings->popup_play_sound = cfg_popup_play_sound;
	shared_settings->popup_sound = GlobalStrDupW( cfg_popup_sound );

	LOGFONT lf;
	_memzero( &lf, sizeof( LOGFONT ) );

	POPUP_SETTINGS *ls1 = ( POPUP_SETTINGS * )GlobalAlloc( GMEM_FIXED, sizeof( POPUP_SETTINGS ) );
	ls1->shared_settings = shared_settings;
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
	ls2->shared_settings = shared_settings;
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
	ls3->shared_settings = shared_settings;
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

	DoublyLinkedList *t_ll = DLL_CreateNode( ( void * )ls1 );

	DoublyLinkedList *ll = DLL_CreateNode( ( void * )ls2 );
	DLL_AddNode( &t_ll, ll, -1 );

	ll = DLL_CreateNode( ( void * )ls3 );
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
		// This semaphore will be released when the thread gets killed.
		worker_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		kill_worker_thread_flag = true;	// Causes secondary threads to cease processing and release the semaphore.

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( worker_semaphore, 5000 );
		CloseHandle( worker_semaphore );
		worker_semaphore = NULL;
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
	kill_ml_update_thread();
	kill_ml_worker_thread();

	// DestroyWindow won't work on a window from a different thread. So we'll send a message to trigger it.
	_SendMessageW( g_hWnd_main, WM_DESTROY_ALT, 0, 0 );

	_ExitThread( 0 );
	return 0;
}

// Returns the first match after counting the number of total keyword matches.
ignorecidinfo *find_ignore_caller_id_name_match( displayinfo *di )
{
	if ( di == NULL )
	{
		return NULL;
	}

	ignorecidinfo *ret_icidi = NULL;

	// Find a keyword that matches the value.
	node_type *node = dllrbt_get_head( ignore_cid_list );
	while ( node != NULL )
	{
		ignorecidinfo *icidi = ( ignorecidinfo * )node->val;
		
		if ( icidi->match_case == true && icidi->match_whole_word == true )
		{
			if ( lstrcmpA( di->ci.caller_id, icidi->c_caller_id ) == 0 )
			{
				++( di->ignore_cid_match_count );

				icidi->active = true;

				if ( ret_icidi == NULL )
				{
					ret_icidi = icidi;
				}
			}
		}
		else if ( icidi->match_case == false && icidi->match_whole_word == true )
		{
			if ( lstrcmpiA( di->ci.caller_id, icidi->c_caller_id ) == 0 )
			{
				++( di->ignore_cid_match_count );

				icidi->active = true;

				if ( ret_icidi == NULL )
				{
					ret_icidi = icidi;
				}
			}
		}
		else if ( icidi->match_case == true && icidi->match_whole_word == false )
		{
			if ( _StrStrA( di->ci.caller_id, icidi->c_caller_id ) != NULL )
			{
				++( di->ignore_cid_match_count );

				icidi->active = true;

				if ( ret_icidi == NULL )
				{
					ret_icidi = icidi;
				}
			}
		}
		else if ( icidi->match_case == false && icidi->match_whole_word == false )
		{
			if ( _StrStrIA( di->ci.caller_id, icidi->c_caller_id ) != NULL )
			{
				++( di->ignore_cid_match_count );

				icidi->active = true;

				if ( ret_icidi == NULL )
				{
					ret_icidi = icidi;
				}
			}
		}

		node = node->next;
	}

	return ret_icidi;
}

// Returns the first match after counting the number of total keyword matches.
forwardcidinfo *find_forward_caller_id_name_match( displayinfo *di )
{
	if ( di == NULL )
	{
		return NULL;
	}

	forwardcidinfo *ret_fcidi = NULL;

	// Find a keyword that matches the value.
	node_type *node = dllrbt_get_head( forward_cid_list );
	while ( node != NULL )
	{
		forwardcidinfo *fcidi = ( forwardcidinfo * )node->val;
		
		if ( fcidi->match_case == true && fcidi->match_whole_word == true )
		{
			if ( lstrcmpA( di->ci.caller_id, fcidi->c_caller_id ) == 0 )
			{
				++( di->forward_cid_match_count );

				fcidi->active = true;

				if ( ret_fcidi == NULL )
				{
					ret_fcidi = fcidi;
				}
			}
		}
		else if ( fcidi->match_case == false && fcidi->match_whole_word == true )
		{
			if ( lstrcmpiA( di->ci.caller_id, fcidi->c_caller_id ) == 0 )
			{
				++( di->forward_cid_match_count );

				fcidi->active = true;

				if ( ret_fcidi == NULL )
				{
					ret_fcidi = fcidi;
				}
			}
		}
		else if ( fcidi->match_case == true && fcidi->match_whole_word == false )
		{
			if ( _StrStrA( di->ci.caller_id, fcidi->c_caller_id ) != NULL )
			{
				++( di->forward_cid_match_count );

				fcidi->active = true;

				if ( ret_fcidi == NULL )
				{
					ret_fcidi = fcidi;
				}
			}
		}
		else if ( fcidi->match_case == false && fcidi->match_whole_word == false )
		{
			if ( _StrStrIA( di->ci.caller_id, fcidi->c_caller_id ) != NULL )
			{
				++( di->forward_cid_match_count );

				fcidi->active = true;

				if ( ret_fcidi == NULL )
				{
					ret_fcidi = fcidi;
				}
			}
		}

		node = node->next;
	}

	return ret_fcidi;
}

char *combine_names( char *first_name, char *last_name )
{
	int first_name_length = 0;
	int last_name_length = 0;
	int combined_name_length = 0;
	char *combined_name = NULL;

	if ( first_name == NULL && last_name == NULL )
	{
		return NULL;
	}
	else if ( first_name != NULL && last_name == NULL )
	{
		return GlobalStrDupA( first_name );
	}
	else if ( first_name == NULL && last_name != NULL )
	{
		return GlobalStrDupA( last_name );
	}

	first_name_length = lstrlenA( first_name );
	last_name_length = lstrlenA( last_name );

	combined_name_length = first_name_length + last_name_length + 1 + 1;
	combined_name = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * combined_name_length );
	_memcpy_s( combined_name, combined_name_length, first_name, first_name_length );
	combined_name[ first_name_length ] = ' ';
	_memcpy_s( combined_name + first_name_length + 1, combined_name_length - ( first_name_length + 1 ), last_name, last_name_length );
	combined_name[ combined_name_length - 1 ] = 0;	// Sanity.

	return combined_name;
}

void cleanup_custom_caller_id()
{
	if ( custom_cid_list != NULL )
	{
		for ( char i = 0; i < CID_LIST_COUNT; ++i )
		{
			// Free the values of the call_log.
			node_type *node = dllrbt_get_head( custom_cid_list[ i ] );
			while ( node != NULL )
			{
				// Free the linked list if there is one.
				DoublyLinkedList *ci_node = ( DoublyLinkedList * )node->val;
				while ( ci_node != NULL )
				{
					DoublyLinkedList *del_ci_node = ci_node;

					ci_node = ci_node->next;

					GlobalFree( del_ci_node );
				}

				node = node->next;
			}

			dllrbt_delete_recursively( custom_cid_list[ i ] );
			custom_cid_list[ i ] = NULL;
		}

		GlobalFree( custom_cid_list );
		custom_cid_list = NULL;
	}
}

char *get_custom_caller_id( char *phone_number )
{
	if ( phone_number != NULL && custom_cid_list != NULL )
	{
		contactinfo *ci = NULL;
		DoublyLinkedList *dll = NULL;

		for ( char i = 0; i < CID_LIST_COUNT; ++i )
		{
			dll = ( DoublyLinkedList * )dllrbt_find( custom_cid_list[ i ], ( void * )phone_number, true );
			while ( dll != NULL )
			{
				ci = ( contactinfo * )dll->data;

				if ( ci != NULL )
				{
					// If there are multiple people with the same office phone number, then use the business name to identify them.
					if ( ( i == 2 && dll->prev != NULL ) || i == 3 )
					{
						if ( ci->contact.business_name != NULL && ci->contact.business_name[ 0 ] != NULL )
						{
							return GlobalStrDupA( ci->contact.business_name );
						}
					}
					else
					{
						// See if there's a first name available.
						if ( ci->contact.first_name != NULL && ci->contact.first_name[ 0 ] != NULL )
						{
							// See if there's a last name available.
							if ( ci->contact.last_name != NULL && ci->contact.last_name[ 0 ] != NULL )
							{
								return combine_names( ci->contact.first_name, ci->contact.last_name );
							}
							else	// If there's no last name, then use the first name.
							{
								return GlobalStrDupA( ci->contact.first_name );
							}
						}
						else if ( ci->contact.nickname != NULL && ci->contact.nickname[ 0 ] != NULL )	// If there's no first name, then try the nickname.
						{
							return GlobalStrDupA( ci->contact.nickname );
						}
					}
				}

				dll = dll->next;
			}
		}
	}

	return NULL;
}

void remove_custom_caller_id( contactinfo *ci )
{
	if ( ci == NULL || custom_cid_list == NULL )
	{
		return;
	}

	char *phone_number = NULL;

	for ( char i = 0; i < CID_LIST_COUNT; ++i )
	{
		switch ( i )
		{
			case 0: { phone_number = ci->contact.home_phone_number; } break;
			case 1: { phone_number = ci->contact.cell_phone_number; } break;
			case 2: { phone_number = ci->contact.office_phone_number; } break;
			case 3: { phone_number = ci->contact.work_phone_number; } break;
			case 4: { phone_number = ci->contact.other_phone_number; } break;
			case 5: { phone_number = ci->contact.fax_number; } break;
		}

		// Skip blank phone numbers.
		if ( phone_number == NULL || ( phone_number != NULL && phone_number[ 0 ] == NULL ) )
		{
			continue;
		}

		dllrbt_iterator *itr = dllrbt_find( custom_cid_list[ i ], ( void * )phone_number, false );
		if ( itr != NULL )
		{
			// Head of the linked list.
			DoublyLinkedList *ll = ( DoublyLinkedList * )( ( node_type * )itr )->val;

			// Go through each linked list node and remove the one with ci.

			DoublyLinkedList *current_node = ll;
			while ( current_node != NULL )
			{
				if ( ( contactinfo * )current_node->data == ci )
				{
					DLL_RemoveNode( &ll, current_node );
					GlobalFree( current_node );

					if ( ll != NULL && ll->data != NULL )
					{
						// Reset the head in the tree.
						( ( node_type * )itr )->val = ( void * )ll;

						switch ( i )
						{
							case 0: { ( ( node_type * )itr )->key = ( void * )( ( contactinfo * )ll->data )->contact.home_phone_number; } break;
							case 1: { ( ( node_type * )itr )->key = ( void * )( ( contactinfo * )ll->data )->contact.cell_phone_number; } break;
							case 2: { ( ( node_type * )itr )->key = ( void * )( ( contactinfo * )ll->data )->contact.office_phone_number; } break;
							case 3: { ( ( node_type * )itr )->key = ( void * )( ( contactinfo * )ll->data )->contact.work_phone_number; } break;
							case 4: { ( ( node_type * )itr )->key = ( void * )( ( contactinfo * )ll->data )->contact.other_phone_number; } break;
							case 5: { ( ( node_type * )itr )->key = ( void * )( ( contactinfo * )ll->data )->contact.fax_number; } break;
						}
					}

					break;
				}

				current_node = current_node->next;
			}

			// If the head of the linked list is NULL, then we can remove the linked list from the tree.
			if ( ll == NULL )
			{
				dllrbt_remove( custom_cid_list[ i ], itr );	// Remove the node from the tree. The tree will rebalance itself.
			}
		}
	}
}

void add_custom_caller_id( contactinfo *ci )
{
	if ( ci == NULL )
	{
		return;
	}

	char *phone_number = NULL;

	if ( custom_cid_list == NULL )
	{
		custom_cid_list = ( dllrbt_tree ** )GlobalAlloc( GPTR, sizeof( dllrbt_tree * ) * CID_LIST_COUNT );
	}

	for ( char i = 0; i < CID_LIST_COUNT; ++i )
	{
		if ( custom_cid_list[ i ] == NULL )
		{
			 custom_cid_list[ i ] = dllrbt_create( dllrbt_compare );
		}

		switch ( i )
		{
			case 0: { phone_number = ci->contact.home_phone_number; } break;
			case 1: { phone_number = ci->contact.cell_phone_number; } break;
			case 2: { phone_number = ci->contact.office_phone_number; } break;
			case 3: { phone_number = ci->contact.work_phone_number; } break;
			case 4: { phone_number = ci->contact.other_phone_number; } break;
			case 5: { phone_number = ci->contact.fax_number; } break;
		}

		// Skip blank phone numbers.
		if ( phone_number == NULL || ( phone_number != NULL && phone_number[ 0 ] == NULL ) )
		{
			continue;
		}

		// Create the node to insert into a linked list.
		DoublyLinkedList *ci_node = DLL_CreateNode( ( void * )ci );

		// See if our tree has the phone number to add the node to.
		DoublyLinkedList *dll = ( DoublyLinkedList * )dllrbt_find( custom_cid_list[ i ], ( void * )phone_number, true );
		if ( dll == NULL )
		{
			// If no phone number exits, insert the node into the tree.
			if ( dllrbt_insert( custom_cid_list[ i ], ( void * )phone_number, ( void * )ci_node ) != DLLRBT_STATUS_OK )
			{
				GlobalFree( ci_node );	// This shouldn't happen.
			}
		}
		else	// If a phone number exits, insert the node into the linked list.
		{
			DLL_AddNode( &dll, ci_node, -1 );	// Insert at the end of the doubly linked list.
		}
	}
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
		//buffer_length = 64;	// Should be enough to hold most translated values.
		buffer_length = __snwprintf( NULL, 0, L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) ) + 1;	// Include the NULL character.
	#else
		buffer_length = _scwprintf( L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) ) + 1;	// Include the NULL character.
	#endif

	di->w_time = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * buffer_length );

	__snwprintf( di->w_time, buffer_length, L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) );

	di->w_forward_phone_number = ( di->forward_phone_number == true ? GlobalStrDupW( ST_Yes ) : GlobalStrDupW( ST_No ) );
	di->w_ignore_phone_number = ( di->ignore_phone_number == true ? GlobalStrDupW( ST_Yes ) : GlobalStrDupW( ST_No ) );

	di->w_forward_caller_id = ( di->forward_cid_match_count > 0 ? GlobalStrDupW( ST_Yes ) : GlobalStrDupW( ST_No ) );
	di->w_ignore_caller_id = ( di->ignore_cid_match_count > 0 ? GlobalStrDupW( ST_Yes ) : GlobalStrDupW( ST_No ) );

	// Forward to phone number
	di->forward_to = FormatPhoneNumber( di->ci.forward_to );

	// Sent to phone number
	di->sent_to = FormatPhoneNumber( di->ci.call_to );
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
