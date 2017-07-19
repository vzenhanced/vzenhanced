/*
	VZ Enhanced is a caller ID notifier that can forward and block phone calls.
	Copyright (C) 2013-2017 Eric Kutcher

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
	L"Add to Forward Caller ID Name List...",
	L"Add to Forward Phone Number List...",
	L"Add to Ignore Caller ID Name List...",
	L"Add to Ignore Phone Number List",
	L"Add to Ignore Phone Number List...",
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
	/*L"Cancel Update Download",*/
	L"Check for &Updates",
	L"Close Tab",
	L"Contacts",
	L"Copy Caller ID Name",
	L"Copy Caller ID Names",
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
	L"Copy Forward Caller ID Name State",
	L"Copy Forward Caller ID Name States",
	L"Copy Forward Phone Number State",
	L"Copy Forward Phone Number States",
	L"Copy Forward to Phone Number",
	L"Copy Forward to Phone Numbers",
	L"Copy Forwarded to Phone Number",
	L"Copy Forwarded to Phone Numbers",
	L"Copy Home Phone Number",
	L"Copy Home Phone Numbers",
	L"Copy Ignore Caller ID Name State",
	L"Copy Ignore Caller ID Name States",
	L"Copy Ignore Phone Number State",
	L"Copy Ignore Phone Number States",
	L"Copy Job Title",
	L"Copy Job Titles",
	L"Copy Last Name",
	L"Copy Last Names",
	L"Copy Match Case State",
	L"Copy Match Case States",
	L"Copy Match Whole Word State",
	L"Copy Match Whole Word States",
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
	L"Edit Contact...",
	L"Edit Forward Caller ID Name List Entry...",
	L"Edit Forward Phone Number List Entry...",
	L"Edit Ignore Caller ID Name List Entry...",
	L"Exit",
	L"Export...",
	L"Export Contacts...",
	L"Forward and Ignore Lists",
	L"Forward Incoming Call...",
	L"Ignore Incoming Call",
	L"Import...",
	L"Import Contacts...",
	L"Log In...",
	L"Open VZ Enhanced",
	L"Open Web Page",
	L"Options...",
	L"Remove from Forward Caller ID Name List",
	L"Remove from Forward Phone Number List",
	L"Remove from Ignore Caller ID Name List",
	L"Remove from Ignore Phone Number List",
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
	L"Callerr",
	L"Google",
	L"OkCaller",
	L"PhoneTray",
	L"WhitePages",
	L"WhoCallsMe",
	L"WhyCall.me"
};

wchar_t *common_message_string_table[] =
{
	L"A contact update is in progress.\r\n\r\nWould you like to cancel the update?",
	/*L"A new version of VZ Enhanced is available.\r\n\r\nWould you like to download it now?",*/
	L"Are you sure you want to delete this contact?",
	L"Are you sure you want to remove the selected entries?",
	L"Are you sure you want to remove the selected entries from the forward caller ID name list?",
	L"Are you sure you want to remove the selected entries from the forward phone number list?",
	L"Are you sure you want to remove the selected entries from the ignore caller ID name list?",
	L"Are you sure you want to remove the selected entries from the ignore phone number list?",
	/*L"Area code is restricted.",*/
	L"Caller ID name is already in the forward caller ID name list.",
	L"Caller ID name is already in the ignore caller ID name list.",
	L"Do you want to automatically log in when the program starts?",
	L"Do you want to delete your login information?",
	L"Phone number is already in the ignore phone number list.",
	L"Phone number is already in the forward phone number list.",
	L"Please enter a valid caller ID name.",
	L"Please enter a valid phone number.",
	L"Popup windows and ringtones must be enable to play per-contact ringtones.\r\n\r\nWould you like to enable these settings?",
	L"Ringtones must be enable to play per-contact ringtones.\r\n\r\nWould you like to enable this setting?",
	L"There was an error while saving the settings.",
	L"Selected caller ID name is forwarded using multiple keywords.\r\n\r\nPlease remove it from the forward caller ID name list.",
	L"Selected caller ID name is ignored using multiple keywords.\r\n\r\nPlease remove it from the ignore caller ID name list.",
	L"Selected phone number is forwarded using wildcard digit(s).\r\n\r\nPlease remove it from the forward phone number list.",
	L"Selected phone number is ignored using wildcard digit(s).\r\n\r\nPlease remove it from the ignore phone number list.",
	L"The download could not be completed.\r\n\r\nWould you like to visit the VZ Enhanced home page instead?",
	L"The file(s) could not be imported because the format is incorrect.",
	/*L"The Message Log thread is still running.\r\n\r\nPlease wait for it to terminate, or restart the program.",*/
	L"The update check could not be completed.\r\n\r\nWould you like to visit the VZ Enhanced home page instead?",
	L"Tree-View image list was not destroyed.",
	/*L"VZ Enhanced is up to date.",*/
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
	L"Play",
	L"Profession:",
	L"Remove Picture",
	L"Ringtone:",
	L"Stop",
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
	L"Log Off",
	L"Phone Number Selection",
	L"Select the phone number you wish to log in with."
};

wchar_t *update_string_table[] =
{
	L"A new version is available.",
	L"Checking for updates...",
	L"Checking For Updates...",
	L"Download Complete",
	L"Download Failed",
	L"Download Update",
	L"[DOWNLOAD URL]\r\n",
	L"Downloading the update...",
	L"Downloading Update...",
	L"Incomplete Download",
	L"[NOTES]\r\n",
	L"The download has completed.",
	L"The download has failed.",
	L"The update check has failed.",
	L"Up To Date",
	L"Update Check Failed",
	L"[VERSION]\r\n",
	L"VZ Enhanced is up to date."
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
	L"Default ringtone:",
	L"Delay Time (seconds):",
	/*L"Down",*/
	L"Enable Call Log history",
	L"Enable contact picture downloads",
	L"Enable popup windows:",
	L"Enable ringtone(s)",
	L"Enable System Tray icon:",
	L"Font:",
	L"Font color:",
	L"Font shadow color:",
	L"General",
	L"Gradient background color:",
	L"Gradient direction:",
	L"Height (pixels):",
	L"Hide window border",
	L"Horizontal",
	L"Justify text:",
	L"Left",
	L"Log events to Message Log",
	L"Minimize to System Tray",
	L"Popup",
	L"Preview Popup",
	L"Reconnect upon connection loss:",
	L"Retries:",
	L"Right",
	/*L"Sample:",*/
	L"Screen Position:",
	L"Show contact picture",
	L"Show line:",
	L"Silent startup",
	L"SSL 2.0",
	L"SSL 3.0",
	L"SSL / TLS version:",
	L"Time format:",
	L"Timeout (seconds):",
	L"TLS 1.0",
	L"TLS 1.1",
	L"TLS 1.2",
	L"Top Left",
	L"Top Right",
	L"Transparency:",
	/*L"Up",*/
	L"Vertical",
	L"Width (pixels):"
};

wchar_t *call_log_string_table[] =
{
	L"#",
	L"Caller ID Name",
	L"Date and Time",
	L"Forward Caller ID Name",
	L"Forward Phone Number",
	L"Forwarded to",
	L"Ignore Caller ID Name",
	L"Ignore Phone Number",
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

wchar_t *forward_cid_list_string_table[] =
{
	L"#",
	L"Caller ID Name",
	L"Forward to",
	L"Match Case",
	L"Match Whole Word",
	L"Total Calls"
};

wchar_t *ignore_cid_list_string_table[] =
{
	L"#",
	L"Caller ID Name",
	L"Match Case",
	L"Match Whole Word",
	L"Total Calls"
};

wchar_t *message_log_list_string_table[] =
{
	L"#",
	L"Date and Time (UTC)",
	L"Level",
	L"Message"
};

wchar_t *common_string_table[] =
{
	L"\xABmultiple caller ID names\xBB",
	L"\xABmultiple phone numbers\xBB",
	L"Account Information",
	L"Call Log",
	L"Caller ID Name",
	L"Caller ID Names",
	L"Caller ID Name:",
	L"Clear Message Log",
	L"Close",
	L"Contact Information",
	L"Contact List",
	L"Dial Phone Number",
	L"Entry #",
	L"Export",
	L"Export Contacts",
	L"Forward Caller ID Name",
	L"Forward Lists",
	L"Forward Phone Number",
	L"Forward to:",
	L"Ignore Caller ID Name",
	L"Ignore Lists",
	L"Ignore Phone Number",
	L"Import",
	L"Import Contacts",
	L"Importing Contacts",
	L"Login",
	L"Match case",
	L"Match whole word",
	L"Message Log",
	L"No",
	L"OK",
	L"Options",
	L"Phone Number",
	L"Phone Numbers",
	L"Phone Number:",
	L"Save Call Log",
	L"Save Message Log...",
	L"Save Message Log",
	L"Select Columns",
	L"Show columns:",
	L"Time",
	L"Total Calls",
	L"Update Phone Number",
	L"Update Caller ID Name",
	L"Yes"
};

wchar_t *connection_string_table[] =
{
	L"A connection could not be established.\r\n\r\nPlease check your network connection and try again.",
	/*L"A new version of the program is available.",*/
	L"Added contact.",
	L"Adding contact.",
	/*L"Checking for updates.",*/
	L"Closed connection.",
	L"Deleted contact.",
	L"Deleting contact.",
	L"Downloaded contact picture(s).",
	/*L"Downloaded update.",*/
	L"Downloading contact picture(s).",
	/*L"Downloading update.",*/
	L"Exported contact list.",
	L"Exporting contact list.",
	/*L"Failed to connect to SAML server.",
	L"Failed to connect to VPNS server.",
	L"Failed to connect to wayfarer server.",*/
	L"Failed to export contact list.",
	L"Failed to parse account information.",
	L"Failed to parse notification setup cookies.",
	L"Failed to parse SAML response.",
	L"Failed to parse SAML request.",
	L"Failed to parse server cookies.",
	L"Failed to receive proper reply when adding contact.",
	L"Failed to receive proper status code from SAML server.",
	L"Failed to receive proper status code from VPNS server.",
	L"Failed to receive proper status code from wayfarer server.",
	L"Failed to receive proper status code when attempting notification setup.",
	L"Failed to receive proper status code when requesting contact information.",
	L"Failed to receive proper status code when requesting contact list.",
	L"Failed to request available services.\r\n\r\nPlease check your username and password and try again.",
	L"Failed to subscribe to available services.",
	L"File could not be saved.",
	L"File size must be less than 5 megabytes.",
	L"Forwarded incoming call.",
	L"Forwarding incoming call.",
	L"Ignored incoming call.",
	L"Ignoring incoming call.",
	L"Importing contact list.",
	L"Incorrect username or password.",
	L"Initiated outbound call.",
	L"Initiating outbound call.",
	L"Joining notification server.",
	L"Loaded contact list.",
	L"Logged in.",
	L"Logged out.",
	L"Logging in.",
	L"No contact picture location(s) were found.",
	L"Parsing account information.",
	L"Parsing SAML response.",
	L"Parsing SAML request.",
	L"Polling for incoming data.",
	L"Reauthorizing login information.",
	L"Received incoming caller ID information.",
	L"Reestablishing connection.",
	L"Removed contact picture.",
	L"Removing contact picture.",
	L"Requesting available services.",
	L"Requesting contact list.",
	L"Requesting contact picture location(s).",
	L"Subscribing to available services.",
	/*L"The download could not be completed.",*/
	L"The login was unsuccessful.\r\n\r\nPlease check your username and password and try again.",
	/*L"The program is up to date.",*/
	/*L"The update check could not be completed.",*/
	L"Updated contact information.",
	L"Updated registration.",
	L"Updating contact information.",
	L"Updating registration.",
	L"Uploaded contact picture.",
	L"Uploading contact picture."
};

wchar_t *list_string_table[] =
{
	L"Added caller ID name to forward caller ID name list.",
	L"Added caller ID name(s) to forward caller ID name list.",
	L"Added caller ID name to ignore caller ID name list.",
	L"Added caller ID name(s) to ignore caller ID name list.",
	L"Added phone number to forward phone number list.",
	L"Added phone number(s) to forward phone number list.",
	L"Added phone number to ignore phone number list.",
	L"Added phone number(s) to ignore phone number list.",
	L"Automatic save has completed.",
	L"Exported call log history.",
	L"Exported forward caller ID name list.",
	L"Exported forward phone number list.",
	L"Exported ignore caller ID name list.",
	L"Exported ignore phone number list.",
	L"Imported call log history.",
	L"Imported forward caller ID name list.",
	L"Imported forward phone number list.",
	L"Imported ignore caller ID name list.",
	L"Imported ignore phone number list.",
	L"Loaded call log history.",
	L"Loaded forward caller ID name list.",
	L"Loaded forward phone number list.",
	L"Loaded ignore caller ID name list.",
	L"Loaded ignore phone number list.",
	L"Loaded ringtone directory.",
	L"Performing automatic save.",
	L"Removed call log entry / entries.",
	L"Removed caller ID name(s) from forward caller ID name list.",
	L"Removed caller ID name(s) from ignore caller ID name list.",
	L"Removed phone number(s) from forward phone number list.",
	L"Removed phone number(s) from ignore phone number list.",
	L"Updated forwarded caller ID name.",
	L"Updated forwarded caller ID name's call count.",
	L"Updated forwarded phone number.",
	L"Updated forwarded phone number's call count.",
	L"Updated ignored caller ID name.",
	L"Updated ignored caller ID name's call count.",
	L"Updated ignored phone number's call count."
};
