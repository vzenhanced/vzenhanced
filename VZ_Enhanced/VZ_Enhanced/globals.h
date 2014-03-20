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

#ifndef _GLOBALS_H
#define _GLOBALS_H

// Pretty window.
#pragma comment( linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"" )

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <process.h>

#include "lite_user32.h"
#include "lite_shell32.h"
#include "lite_ntdll.h"

#include "doublylinkedlist.h"
#include "dllrbt.h"
#include "range.h"

#include "resource.h"

#define PROGRAM_CAPTION		L"VZ Enhanced"
#define PROGRAM_CAPTION_A	"VZ Enhanced"

#define MIN_WIDTH			480
#define MIN_HEIGHT			320

#define MAX_POPUP_TIME		300		// Maximum time in seconds that the popup window can be shown.
#define SNAP_WIDTH			10		// The minimum distance at which our windows will attach together.

#define WM_DESTROY_ALT		WM_APP		// Allows non-window threads to call DestroyWindow.
#define WM_PROPAGATE		WM_APP + 1
#define WM_CHANGE_CURSOR	WM_APP + 2	// Updates the window cursor.

#define WM_TRAY_NOTIFY		WM_APP + 3
#define WM_EXIT				WM_APP + 4
#define WM_ALERT			WM_APP + 5

#define FILETIME_TICKS_PER_SECOND	10000000LL

///// Contact Window messages /////
#define CW_MODIFY			0	// Add or Update contacts.
#define CW_UPDATE			1	// Update various windows in the Contact Window.

// wParam values
#define UPDATE_END			0	// The update completed or was canceled.
#define UPDATE_BEGIN		1	// Starting to update.
#define UPDATE_PROGRESS		2	// Used for upload progress.
#define UPDATE_FAIL			3	// Update failed.
///////////////////////////////////

#define LINE_TIME			1
#define LINE_CALLER_ID		2
#define LINE_PHONE			3
#define TOTAL_LINES			3

#define LOGGED_OUT			0
#define LOGGED_IN			1
#define LOGGING_IN			2
#define AUTO_LOGIN			3

#define NUM_TABS			4

#define NUM_COLUMNS			9
#define NUM_COLUMNS2		17
#define NUM_COLUMNS3		4
#define NUM_COLUMNS4		3

#define SAFESTRA( s ) ( s != NULL ? s : "" )
#define SAFESTR2A( s1, s2 ) ( s1 != NULL ? s1 : ( s2 != NULL ? s2 : "" ) )

#define SAFESTRW( s ) ( s != NULL ? s : L"" )
#define SAFESTR2W( s1, s2 ) ( s1 != NULL ? s1 : ( s2 != NULL ? s2 : L"" ) )

#define is_close( a, b ) ( _abs( a - b ) < SNAP_WIDTH )

#define is_digit( c ) ( c - '0' + 0U <= 9U )

#define GET_X_LPARAM( lp )	( ( int )( short )LOWORD( lp ) )
#define GET_Y_LPARAM( lp )	( ( int )( short )HIWORD( lp ) )

struct contactinfo;
struct ignoreinfo;

struct callerinfo
{
	char *call_to;
	char *call_from;
	char *caller_id;
	char *forward_to;

	char *call_reference_id;

	bool ignored;			// The phone number has been ignored
	bool forwarded;			// The phone number has been forwarded
};

struct displayinfo
{
	union
	{
		struct	// Keep these in alphabetical order.
		{
			wchar_t *caller_id;		// Caller ID
			wchar_t *w_time;		// Date and Time
			wchar_t *w_forward;		// Forward
			wchar_t *forward_to;	// Forward to
			wchar_t *w_ignore;		// Ignore
			wchar_t *phone_number;	// Phone Number
			wchar_t *reference;		// Reference
			wchar_t *sent_to;		// Sent to
		};

		wchar_t *display_values[ 8 ];
	};

	callerinfo ci;

	LARGE_INTEGER time;

	bool ignore;			// Ignore	(phone number is in ignore_list)
	bool forward;			// Forward	(phone number is in forward_list)

	bool process_incoming;	// false, true = ignore or forward
};

struct CONTACT
{
	union
	{
		struct	// Keep these in alphabetical order.
		{
			char *cell_phone_number;	// Cell Phone Number
			char *business_name;		// Company
			char *department;			// Department
			char *email_address;		// Email Address
			char *fax_number;			// Fax Number
			char *first_name;			// First Name
			char *home_phone_number;	// Home Phone Number
			char *designation;			// Job Title
			char *last_name;			// Last Name
			char *nickname;				// Nickname
			char *office_phone_number;	// Office Phone Number
			char *other_phone_number;	// Other Phone Number
			char *category;				// Profession
			char *title;				// Title
			char *web_page;				// Web Page
			char *work_phone_number;	// Work Phone Number
		};

		char *contact_values[ 16 ];
	};

	char *ringtone;

	char *cell_phone_number_id;
	char *contact_entry_id;
	char *email_address_id;
	char *fax_number_id;
	char *home_phone_number_id;
	char *office_phone_number_id;
	char *other_phone_number_id;
	char *web_page_id;
	char *work_phone_number_id;

	char *picture_location;		// Picture URL. For downloading.
};

struct contactinfo
{
	CONTACT contact;

	union
	{
		struct	// Keep these in alphabetical order.
		{
			wchar_t *cell_phone_number;		// Cell Phone Number
			wchar_t *business_name;			// Company
			wchar_t *department;			// Department
			wchar_t *email_address;			// Email Address
			wchar_t *fax_number;			// Fax Number
			wchar_t *first_name;			// First Name
			wchar_t *home_phone_number;		// Home Phone Number
			wchar_t *designation;			// Job Title
			wchar_t *last_name;				// Last Name
			wchar_t *nickname;				// Nickname
			wchar_t *office_phone_number;	// Office Phone Number
			wchar_t *other_phone_number;	// Other Phone Number
			wchar_t *category;				// Profession
			wchar_t *title;					// Title
			wchar_t *web_page;				// Web Page
			wchar_t *work_phone_number;		// Work Phone Number
		};

		wchar_t *contactinfo_values[ 16 ];
	};

	wchar_t *picture_path;			// Local path to picture.

	bool displayed;					// Set to true if it's displayed in the contact list.
};

struct updateinfo
{
	contactinfo *old_ci;
	contactinfo *new_ci;
	bool picture_only;
	bool remove_picture;
};

struct ignoreinfo
{
	union
	{
		struct	// Keep these in alphabetical order.
		{
			char *c_phone_number;
			char *c_total_calls;
		};

		char *c_ignoreinfo_values[ 2 ];
	};

	union
	{
		struct	// Keep these in alphabetical order.
		{
			wchar_t *phone_number;
			wchar_t *total_calls;
		};

		wchar_t *ignoreinfo_values[ 2 ];
	};

	unsigned int count;		// Number of times the call has been ignored.
	unsigned char state;	// 0 = keep, 1 = remove.
};

struct ignoreupdateinfo
{
	HWND hWnd;
	char *phone_number;		// Phone number to add.
	ignoreinfo *ii;			// Ignore info to update.
	unsigned char action;	// 0 Add, 1 Remove, 2 Add all from ignorelist, 3 Update ii.
};

struct forwardinfo
{
	union
	{
		struct	// Keep these in alphabetical order.
		{
			char *c_forward_to;		// Forward to
			char *c_call_from;		// Phone Number
			char *c_total_calls;	// Total Calls
		};

		char *c_forwardinfo_value[ 3 ];
	};

	union
	{
		struct	// Keep these in alphabetical order.
		{
			wchar_t *forward_to;	// Forward to
			wchar_t *call_from;		// Phone Number
			wchar_t *total_calls;	// Total Calls
		};

		wchar_t *forwardinfo_values[ 3 ];
	};

	unsigned int count;		// Number of times the call has been forwarded.
	unsigned char state;	// 0 = keep, 1 = remove.
};

struct forwardupdateinfo
{
	HWND hWnd;
	char *call_from;		// Phone number to add.
	char *forward_to;
	forwardinfo *fi;		// Forward info to update.
	unsigned char action;	// 0 Add, 1 Remove, 2 Add all from forward_list, 3 Update entry, 4 Update fi.
};

// These are all variables that are shared among the separate .cpp files.

// Object handles.
extern HWND g_hWnd_main;				// Handle to our main window.
extern HWND g_hWnd_login;				// Handle to our login window.
extern HWND g_hWnd_options;				// Handle to our options window.
extern HWND g_hWnd_columns;
extern HWND g_hWnd_contact;
extern HWND g_hWnd_account;
extern HWND g_hWnd_dial;
extern HWND g_hWnd_forward;
extern HWND g_hWnd_ignore_phone_number;
extern HWND g_hWnd_list;				// Handle to the listview control.
extern HWND g_hWnd_contact_list;
extern HWND g_hWnd_ignore_list;
extern HWND g_hWnd_forward_list;
extern HWND g_hWnd_tab;
extern HWND g_hWnd_connection_manager;
extern HWND g_hWnd_active;				// Handle to the active window. Used to handle tab stops.

extern CRITICAL_SECTION pe_cs;			// Allow only one worker thread to be active.
extern CRITICAL_SECTION ct_cs;			// Allow only one connection thread to be active.
extern CRITICAL_SECTION cwt_cs;			// Allow only one connection worker thread to be active.
extern CRITICAL_SECTION cit_cs;

extern HANDLE connection_mutex;			// Blocks shutdown while the connection thread is active.
extern HANDLE connection_worker_mutex;
extern HANDLE connection_incoming_mutex;

extern HFONT hFont;						// Handle to the system's message font.

extern HCURSOR wait_cursor;				// Temporary cursor while processing entries.

extern NOTIFYICONDATA g_nid;			// Tray icon information.

extern wchar_t base_directory[ MAX_PATH ];
extern unsigned int base_directory_length;

// Thread variables
extern bool kill_worker_thead;			// Allow for a clean shutdown.

extern bool in_worker_thread;			// Flag to indicate that we're in a worker thread.
extern bool in_connection_thread;		// Flag to indicate that we're in the connection thread.
extern bool in_connection_worker_thread;
extern bool in_connection_incoming_thread;

extern bool skip_log_draw;				// Prevents WM_DRAWITEM from accessing listview items while we're removing them.
extern bool skip_contact_draw;
extern bool skip_ignore_draw;
extern bool skip_forward_draw;

extern RANGE *ignore_range_list;
extern RANGE *forward_range_list;

extern dllrbt_tree *ignore_list;
extern bool ignore_list_changed;

extern dllrbt_tree *forward_list;
extern bool forward_list_changed;

extern dllrbt_tree *contact_list;

extern dllrbt_tree *call_log;
extern bool call_log_changed;


extern unsigned char total_tabs;

extern unsigned char total_columns;
extern unsigned char total_columns2;
extern unsigned char total_columns3;
extern unsigned char total_columns4;

extern unsigned char login_state;

// Account information
extern char *account_id;
extern char *account_status;
extern char *account_type;
extern char *principal_id;
extern char *service_type;
extern char *service_status;
extern char *service_context;
extern char *service_phone_number;
extern char *service_privacy_value;
extern char *service_features;

// Saved configuration variables
extern wchar_t *cfg_username;
extern wchar_t *cfg_password;
extern bool cfg_remember_login;
extern int cfg_pos_x;
extern int cfg_pos_y;
extern int cfg_width;
extern int cfg_height;

extern bool cfg_tray_icon;
extern bool cfg_close_to_tray;
extern bool cfg_minimize_to_tray;
extern bool cfg_silent_startup;

extern bool cfg_always_on_top;

extern bool cfg_enable_call_log_history;

extern bool cfg_enable_popups;
extern bool cfg_popup_hide_border;
extern int cfg_popup_width;
extern int cfg_popup_height;
extern unsigned char cfg_popup_position;
extern unsigned short cfg_popup_time;
extern unsigned char cfg_popup_transparency;
extern bool cfg_popup_gradient;
extern unsigned char cfg_popup_gradient_direction;
extern COLORREF cfg_popup_background_color1;
extern COLORREF cfg_popup_background_color2;

extern wchar_t *cfg_popup_font_face1;
extern wchar_t *cfg_popup_font_face2;
extern wchar_t *cfg_popup_font_face3;
extern COLORREF cfg_popup_font_color1;
extern COLORREF cfg_popup_font_color2;
extern COLORREF cfg_popup_font_color3;
extern LONG cfg_popup_font_height1;
extern LONG cfg_popup_font_height2;
extern LONG cfg_popup_font_height3;
extern LONG cfg_popup_font_weight1;
extern LONG cfg_popup_font_weight2;
extern LONG cfg_popup_font_weight3;
extern BYTE cfg_popup_font_italic1;
extern BYTE cfg_popup_font_italic2;
extern BYTE cfg_popup_font_italic3;
extern BYTE cfg_popup_font_underline1;
extern BYTE cfg_popup_font_underline2;
extern BYTE cfg_popup_font_underline3;
extern BYTE cfg_popup_font_strikeout1;
extern BYTE cfg_popup_font_strikeout2;
extern BYTE cfg_popup_font_strikeout3;
extern bool cfg_popup_font_shadow1;
extern bool cfg_popup_font_shadow2;
extern bool cfg_popup_font_shadow3;
extern COLORREF cfg_popup_font_shadow_color1;
extern COLORREF cfg_popup_font_shadow_color2;
extern COLORREF cfg_popup_font_shadow_color3;
extern unsigned char cfg_popup_justify_line1;
extern unsigned char cfg_popup_justify_line2;
extern unsigned char cfg_popup_justify_line3;
extern char cfg_popup_line_order1;
extern char cfg_popup_line_order2;
extern char cfg_popup_line_order3;
extern unsigned char cfg_popup_time_format;

extern bool cfg_popup_play_sound;
extern wchar_t *cfg_popup_sound;

extern char cfg_tab_order1;
extern char cfg_tab_order2;
extern char cfg_tab_order3;
extern char cfg_tab_order4;

extern int cfg_column_width1;
extern int cfg_column_width2;
extern int cfg_column_width3;
extern int cfg_column_width4;
extern int cfg_column_width5;
extern int cfg_column_width6;
extern int cfg_column_width7;
extern int cfg_column_width8;
extern int cfg_column_width9;

extern char cfg_column_order1;
extern char cfg_column_order2;
extern char cfg_column_order3;
extern char cfg_column_order4;
extern char cfg_column_order5;
extern char cfg_column_order6;
extern char cfg_column_order7;
extern char cfg_column_order8;
extern char cfg_column_order9;

extern int cfg_column2_width1;
extern int cfg_column2_width2;
extern int cfg_column2_width3;
extern int cfg_column2_width4;
extern int cfg_column2_width5;
extern int cfg_column2_width6;
extern int cfg_column2_width7;
extern int cfg_column2_width8;
extern int cfg_column2_width9;
extern int cfg_column2_width10;
extern int cfg_column2_width11;
extern int cfg_column2_width12;
extern int cfg_column2_width13;
extern int cfg_column2_width14;
extern int cfg_column2_width15;
extern int cfg_column2_width16;
extern int cfg_column2_width17;

extern char cfg_column2_order1;
extern char cfg_column2_order2;
extern char cfg_column2_order3;
extern char cfg_column2_order4;
extern char cfg_column2_order5;
extern char cfg_column2_order6;
extern char cfg_column2_order7;
extern char cfg_column2_order8;
extern char cfg_column2_order9;
extern char cfg_column2_order10;
extern char cfg_column2_order11;
extern char cfg_column2_order12;
extern char cfg_column2_order13;
extern char cfg_column2_order14;
extern char cfg_column2_order15;
extern char cfg_column2_order16;
extern char cfg_column2_order17;

extern int cfg_column3_width1;
extern int cfg_column3_width2;
extern int cfg_column3_width3;
extern int cfg_column3_width4;

extern char cfg_column3_order1;
extern char cfg_column3_order2;
extern char cfg_column3_order3;
extern char cfg_column3_order4;

extern int cfg_column4_width1;
extern int cfg_column4_width2;
extern int cfg_column4_width3;

extern char cfg_column4_order1;
extern char cfg_column4_order2;
extern char cfg_column4_order3;

extern bool cfg_connection_auto_login;
extern bool cfg_connection_reconnect;
extern bool cfg_download_pictures;
extern unsigned char cfg_connection_retries;

extern unsigned short cfg_connection_timeout;
extern unsigned char cfg_connection_ssl_version;

extern char *tab_order[ NUM_TABS ];

extern char *list_columns[ NUM_COLUMNS ];
extern char *contact_list_columns[ NUM_COLUMNS2 ];
extern char *forward_list_columns[ NUM_COLUMNS3 ];
extern char *ignore_list_columns[ NUM_COLUMNS4 ];

extern int *list_columns_width[ NUM_COLUMNS ];
extern int *contact_list_columns_width[ NUM_COLUMNS2 ];
extern int *forward_list_columns_width[ NUM_COLUMNS3 ];
extern int *ignore_list_columns_width[ NUM_COLUMNS4 ];

struct WINDOW_SETTINGS
{
	POINT window_position;
	POINT drag_position;
	bool is_dragging;
};

struct SHARED_SETTINGS	// These are settings that the popup window will need.
{
	COLORREF popup_background_color1;
	COLORREF popup_background_color2;
	wchar_t *popup_sound;
	unsigned short popup_time;
	unsigned char popup_gradient_direction;
	bool popup_gradient;
	bool popup_play_sound;
};

struct POPUP_SETTINGS
{
	WINDOW_SETTINGS window_settings;
	HFONT font;
	COLORREF font_color;
	COLORREF font_shadow_color;
	LONG font_height;
	LONG font_weight;
	wchar_t *font_face;
	wchar_t *line_text;
	unsigned char popup_justify;
	char popup_line_order;
	BYTE font_italic;
	BYTE font_underline;
	BYTE font_strikeout;
	bool font_shadow;
};

struct LOGIN_SETTINGS
{
	char *username;
	char *password;
};

#endif
