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

#include "string_tables.h"

// Ordered by month.
wchar_t *month_string_table[] =
{
	L"January",
	L"February",
	L"March",
	L"April",
	L"May",
	L"June",
	L"July",
	L"August",
	L"September",
	L"October",
	L"November",
	L"December"
};

// Ordered by day.
wchar_t *day_string_table[] =
{
	L"Sunday",
	L"Monday",
	L"Tuesday",
	L"Wednesday",
	L"Thursday",
	L"Friday",
	L"Saturday"
};

wchar_t *menu_string_table[] =
{
	L"&About",
	L"&Edit",
	L"&File",
	L"&Help",
	L"&Log In...",
	L"&Log Off",
	L"&Options...",
	L"&Save Call Log...",
	L"&Tools",
	L"&View",
	L"Add Contact...",
	L"Add to Forward List...",
	L"Add to Ignore List",
	L"Add to Ignore List...",
	L"Call Phone Number...",
	L"Call Cell Phone Number...",
	L"Call Fax Number...",
	L"Call Forward to Phone Number...",
	L"Call Forwarded to Phone Number...",
	L"Call Home Phone Number...",
	L"Call Office Phone Number...",
	L"Call Other Phone Number...",
	L"Call Sent to Phone Number...",
	L"Call Work Phone Number...",
	L"Cancel Import",
	L"Cancel Update Download",
	L"Check for &Updates",
	L"Close Tab",
	L"Contacts",
	L"Copy Caller ID",
	L"Copy Caller IDs",
	L"Copy Cell Phone Number",
	L"Copy Cell Phone Numbers",
	L"Copy Companies",
	L"Copy Company",
	L"Copy Date and Time",
	L"Copy Dates and Times",
	L"Copy Department",
	L"Copy Departments",
	L"Copy Email Address",
	L"Copy Email Addresses",
	L"Copy Fax Number",
	L"Copy Fax Numbers",
	L"Copy First Name",
	L"Copy First Names",
	L"Copy Forward State",
	L"Copy Forward States",
	L"Copy Forward to Phone Number",
	L"Copy Forward to Phone Numbers",
	L"Copy Forwarded to Phone Number",
	L"Copy Forwarded to Phone Numbers",
	L"Copy Home Phone Number",
	L"Copy Home Phone Numbers",
	L"Copy Ignore State",
	L"Copy Ignore States",
	L"Copy Job Title",
	L"Copy Job Titles",
	L"Copy Last Name",
	L"Copy Last Names",
	L"Copy Nickname",
	L"Copy Nicknames",
	L"Copy Office Phone Number",
	L"Copy Office Phone Numbers",
	L"Copy Other Phone Number",
	L"Copy Other Phone Numbers",
	L"Copy Phone Number",
	L"Copy Phone Numbers",
	L"Copy Profession",
	L"Copy Professions",
	L"Copy Reference",
	L"Copy References",
	L"Copy Selected",
	L"Copy Sent to Phone Number",
	L"Copy Sent to Phone Numbers",
	L"Copy Title",
	L"Copy Titles",
	L"Copy Total Calls",
	L"Copy Web Page",
	L"Copy Web Pages",
	L"Copy Work Phone Number",
	L"Copy Work Phone Numbers",
	L"Delete Contact",
	L"Dial Phone Number...",
	L"E&xit",
	L"Exit",
	L"Edit Contact...",
	L"Edit Forward List Entry...",
	L"Export Contacts...",
	L"Forward Incoming Call...",
	L"Ignore Incoming Call",
	L"Import Contacts...",
	L"Log In...",
	L"Open Caller ID",
	L"Open Web Page",
	L"Options...",
	L"Remove from Forward List",
	L"Remove from Ignore List",
	L"Remove Selected",
	L"Search with",
	L"Select All",
	L"Select &Columns...",
	L"Select Columns...",
	L"Send Email...",
	L"Tabs",
	L"VZ Enhanced &Home Page",
	L"Web Server"
};

wchar_t *search_string_table[] =
{
	L"800Notes",
	L"Bing",
	L"CallerComplaints",
	L"Callerr",
	L"Google",
	L"PhoneTray",
	L"SafeCaller",
	L"WhitePages",
	L"WhoCallsMe",
	L"WhyCall.me"
};

wchar_t *common_message_string_table[] =
{
	L"A contact update is in progress.\r\n\r\nWould you like to cancel the update?",
	L"Are you sure you want to delete this contact?",
	L"Are you sure you want to remove the selected entries?",
	L"Area code is restricted.",
	L"Do you want to automatically log in when the program starts?",
	L"Do you want to delete your login information?",
	L"Phone number is already in the ignore list.",
	L"Phone number is already in the forward list.",
	L"Phone number was not found in the forward list.",
	L"Please enter a valid phone number.",
	L"There was an error while saving the settings.",
	L"This phone number is forwarded using wildcard digit(s).\r\n\r\nPlease remove it from the Forward List.",
	L"This phone number is ignored using wildcard digit(s).\r\n\r\nPlease remove it from the Ignore List.",
	L"You must have at least one column displayed.",
	L"You must restart the program for this setting to take effect.",
	L"You must supply a password to log in.",
	L"You must supply a username and password to log in.",
	L"You must supply a username to log in."
};

wchar_t *contact_string_table[] =
{
	L"Add Contact",
	L"Cancel Update",
	L"Cell Phone:",
	L"Choose File...",
	L"Company:",
	L"Contact Picture",
	L"Contact Picture:",
	L"Department:",
	L"Email Address:",
	L"Fax:",
	L"First Name:",
	L"Home Phone:",
	L"Job Title:",
	L"Last Name:",
	L"Nickname:",
	L"Office Phone:",
	L"Other Phone:",
	L"Profession:",
	L"Remove Picture",
	L"Title:",
	L"Update Contact",
	L"Update Contact - Canceled",
	L"Update Contact - Complete",
	L"Update Contact - Failed",
	L"Updating Contact",
	L"Web Page:",
	L"Work Phone:"
};

wchar_t *account_string_table[] =
{
	L"Account ID:",
	L"Account Status:",
	L"Account Type:",
	L"Feature(s):",
	L"Principal ID:",
	L"Privacy Value:",
	L"Service Context:",
	L"Service Status:",
	L"Service Type:"
};

wchar_t *login_string_table[] =
{
	L"Username:",
	L"Password:",
	L"Remember login",
	L"Log In",
	L"Logging In...",
	L"Logged In",
	L"Log Off"
};

wchar_t *options_string_table[] =
{
	L"...",
	L"12 hour",
	L"24 hour",
	L"Always on top",
	L"Apply",
	L"Automatically log in",
	L"Background color:",
	L"Bottom Left",
	L"Bottom Right",
	L"Cancel",
	L"Center",
	L"Check for updates upon startup",
	L"Close to System Tray",
	L"Connection",
	L"Delay Time:",
	L"Down",
	L"Enable Call Log history",
	L"Enable contact picture downloads",
	L"Enable Popup windows:",
	L"Enable System Tray icon:",
	L"Font:",
	L"Font color:",
	L"Font shadow color:",
	L"General",
	L"Gradient background color:",
	L"Gradient direction:",
	L"Height:",
	L"Hide window border",
	L"Horizontal",
	L"Justify text:",
	L"Left",
	L"Minimize to System Tray",
	L"Play sound:",
	L"Popup",
	L"Popup Sound",
	L"Preview Popup",
	L"Reconnect upon connection loss:",
	L"Retries:",
	L"Right",
	L"Sample:",
	L"Screen Position:",
	L"Show line:",
	L"Silent startup",
	L"SSL 2.0",
	L"SSL 3.0",
	L"SSL version:",
	L"Time format:",
	L"Timeout:",
	L"TLS 1.0",
	L"TLS 1.1",
	L"TLS 1.2",
	L"Top Left",
	L"Top Right",
	L"Transparency:",
	L"Up",
	L"Vertical",
	L"Width:"
};

wchar_t *call_log_string_table[] =
{
	L"#",
	L"Caller ID",
	L"Date and Time",
	L"Forward",
	L"Forwarded to",
	L"Ignore",
	L"Phone Number",
	L"Reference",
	L"Sent to"
};

wchar_t *contact_list_string_table[] =
{
	L"#",
	L"Cell Phone Number",
	L"Company",
	L"Department",
	L"Email Address",
	L"Fax Number",
	L"First Name",
	L"Home Phone Number",
	L"Job Title",
	L"Last Name",
	L"Nickname",
	L"Office Phone Number",
	L"Other Phone Number",
	L"Profession",
	L"Title",
	L"Web Page",
	L"Work Phone Number"
};

wchar_t *forward_list_string_table[] =
{
	L"#",
	L"Forward to",
	L"Phone Number",
	L"Total Calls"
};

wchar_t *ignore_list_string_table[] =
{
	L"#",
	L"Phone Number",
	L"Total Calls"
};

wchar_t *common_string_table[] =
{
	L"Account Information",
	L"Call Log",
	L"Caller ID",
	L"Contact Information",
	L"Contact List",
	L"Dial Phone Number",
	L"Entry #",
	L"Export Contacts",
	L"Forward List",
	L"Forward Phone Number",
	L"Forward to:",
	L"Ignore List",
	L"Ignore Phone Number",
	L"Import Contacts",
	L"Importing Contacts",
	L"Login",
	L"No",
	L"OK",
	L"Options",
	L"Phone Number",
	L"Phone Number:",
	L"Save Call Log",
	L"Select Columns",
	L"Show columns:",
	L"Time",
	L"Total Calls",
	L"Update Phone Number",
	L"Yes"
};
