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

#ifndef _STRING_TABLES_H
#define _STRING_TABLES_H

// Values with ":", "&", or "-" are replaced with _
// Values with "..." are replaced with ___ 
// Values with # are replaced with NUM
// Keep the defines case sensitive.
// Try to keep them in order.

extern wchar_t *month_string_table[];
extern wchar_t *day_string_table[];

extern wchar_t *common_message_string_table[];

#define ST_PROMPT_contact_update_in_progress	common_message_string_table[ 0 ]
#define ST_PROMPT_delete_contact				common_message_string_table[ 1 ]
#define ST_PROMPT_remove_entries				common_message_string_table[ 2 ]
#define ST_restricted_area_code					common_message_string_table[ 3 ]
#define ST_PROMPT_auto_log_in					common_message_string_table[ 4 ]
#define ST_PROMPT_delete_login_info				common_message_string_table[ 5 ]
#define ST_already_in_ignore_list				common_message_string_table[ 6 ]
#define ST_already_in_forward_list				common_message_string_table[ 7 ]
#define ST_not_found_in_forward_list			common_message_string_table[ 8 ]
#define ST_enter_valid_phone_number				common_message_string_table[ 9 ]
#define ST_error_while_saving_settings			common_message_string_table[ 10 ]
#define ST_remove_from_forward_list				common_message_string_table[ 11 ]
#define ST_remove_from_ignore_list				common_message_string_table[ 12 ]
#define ST_must_have_column						common_message_string_table[ 13 ]
#define ST_must_restart_program					common_message_string_table[ 14 ]
#define ST_must_enter_pw						common_message_string_table[ 15 ]
#define ST_must_enter_un_and_pw					common_message_string_table[ 16 ]
#define ST_must_enter_un						common_message_string_table[ 17 ]

extern wchar_t *call_log_string_table[];

#define ST_CLL_NUM								call_log_string_table[ 0 ]
#define ST_CLL_Caller_ID						call_log_string_table[ 1 ]
#define ST_CLL_Date_and_Time					call_log_string_table[ 2 ]
#define ST_CLL_Forward							call_log_string_table[ 3 ]
#define ST_CLL_Forwarded_to						call_log_string_table[ 4 ]
#define ST_CLL_Ignore							call_log_string_table[ 5 ]
#define ST_CLL_Phone_Number						call_log_string_table[ 6 ]
#define ST_CLL_Reference						call_log_string_table[ 7 ]
#define ST_CLL_Sent_to							call_log_string_table[ 8 ]

extern wchar_t *contact_list_string_table[];

#define ST_CL_NUM								contact_list_string_table[ 0 ]
#define ST_CL_Cell_Phone_Number					contact_list_string_table[ 1 ]
#define ST_CL_Company							contact_list_string_table[ 2 ]
#define ST_CL_Department						contact_list_string_table[ 3 ]
#define ST_CL_Email_Address						contact_list_string_table[ 4 ]
#define ST_CL_Fax_Number						contact_list_string_table[ 5 ]
#define ST_CL_First_Name						contact_list_string_table[ 6 ]
#define ST_CL_Home_Phone_Number					contact_list_string_table[ 7 ]
#define ST_CL_Job_Title							contact_list_string_table[ 8 ]
#define ST_CL_Last_Name							contact_list_string_table[ 9 ]
#define ST_CL_Nickname							contact_list_string_table[ 10 ]
#define ST_CL_Office_Phone_Number				contact_list_string_table[ 11 ]
#define ST_CL_Other_Phone_Number				contact_list_string_table[ 12 ]
#define ST_CL_Profession						contact_list_string_table[ 13 ]
#define ST_CL_Title								contact_list_string_table[ 14 ]
#define ST_CL_Web_Page							contact_list_string_table[ 15 ]
#define ST_CL_Work_Phone_Number					contact_list_string_table[ 16 ]

extern wchar_t *forward_list_string_table[];

#define ST_FL_NUM								forward_list_string_table[ 0 ]
#define ST_FL_Forward_to						forward_list_string_table[ 1 ]
#define ST_FL_Phone_Number						forward_list_string_table[ 2 ]
#define ST_FL_Total_Calls						forward_list_string_table[ 3 ]

extern wchar_t *ignore_list_string_table[];

#define ST_IL_NUM								ignore_list_string_table[ 0 ]
#define ST_IL_Phone_Number						ignore_list_string_table[ 1 ]
#define ST_IL_Total_Calls						ignore_list_string_table[ 2 ]

extern wchar_t *common_string_table[];

#define ST_Account_Information					common_string_table[ 0 ]
#define ST_Call_Log								common_string_table[ 1 ]
#define ST_Caller_ID							common_string_table[ 2 ]
#define ST_Contact_Information					common_string_table[ 3 ]
#define ST_Contact_List							common_string_table[ 4 ]
#define ST_Dial_Phone_Number					common_string_table[ 5 ]
#define ST_Entry_NUM							common_string_table[ 6 ]
#define ST_Export_Contacts						common_string_table[ 7 ]
#define ST_Forward_List							common_string_table[ 8 ]
#define ST_Forward_Phone_Number					common_string_table[ 9 ]
#define ST_Forward_to_							common_string_table[ 10 ]
#define ST_Ignore_List							common_string_table[ 11 ]
#define ST_Ignore_Phone_Number					common_string_table[ 12 ]
#define ST_Import_Contacts						common_string_table[ 13 ]
#define ST_Importing_Contacts					common_string_table[ 14 ]
#define ST_Login								common_string_table[ 15 ]
#define ST_No									common_string_table[ 16 ]
#define ST_OK									common_string_table[ 17 ]
#define ST_Options								common_string_table[ 18 ]
#define ST_Phone_Number							common_string_table[ 19 ]
#define ST_Phone_Number_						common_string_table[ 20 ]
#define ST_Save_Call_Log						common_string_table[ 21 ]
#define ST_Select_Columns						common_string_table[ 22 ]
#define ST_Show_columns_						common_string_table[ 23 ]
#define ST_Time									common_string_table[ 24 ]
#define ST_Total_Calls							common_string_table[ 25 ]
#define ST_Update_Phone_Number					common_string_table[ 26 ]
#define ST_Yes									common_string_table[ 27 ]

extern wchar_t *contact_string_table[];

#define ST_Add_Contact							contact_string_table[ 0 ]
#define ST_Cancel_Update						contact_string_table[ 1 ]
#define ST_Cell_Phone_							contact_string_table[ 2 ]
#define ST_Choose_File___						contact_string_table[ 3 ]
#define ST_Company_								contact_string_table[ 4 ]
#define ST_Contact_Picture						contact_string_table[ 5 ]
#define ST_Contact_Picture_						contact_string_table[ 6 ]
#define ST_Department_							contact_string_table[ 7 ]
#define ST_Email_Address_						contact_string_table[ 8 ]
#define ST_Fax_									contact_string_table[ 9 ]
#define ST_First_Name_							contact_string_table[ 10 ]
#define ST_Home_Phone_							contact_string_table[ 11 ]
#define ST_Job_Title_							contact_string_table[ 12 ]
#define ST_Last_Name_							contact_string_table[ 13 ]
#define ST_Nickname_							contact_string_table[ 14 ]
#define ST_Office_Phone_						contact_string_table[ 15 ]
#define ST_Other_Phone_							contact_string_table[ 16 ]
#define ST_Profession_							contact_string_table[ 17 ]
#define ST_Remove_Picture						contact_string_table[ 18 ]
#define ST_Title_								contact_string_table[ 19 ]
#define ST_Update_Contact						contact_string_table[ 20 ]
#define ST_Update_Contact___Canceled			contact_string_table[ 21 ]
#define ST_Update_Contact___Complete			contact_string_table[ 22 ]
#define ST_Update_Contact___Failed				contact_string_table[ 23 ]
#define ST_Updating_Contact						contact_string_table[ 24 ]
#define ST_Web_Page_							contact_string_table[ 25 ]
#define ST_Work_Phone_							contact_string_table[ 26 ]

extern wchar_t *account_string_table[];

#define ST_Account_ID_							account_string_table[ 0 ]
#define ST_Account_Status_						account_string_table[ 1 ]
#define ST_Account_Type_						account_string_table[ 2 ]
#define ST_Features_							account_string_table[ 3 ]
#define ST_Principal_ID_						account_string_table[ 4 ]
#define ST_Privacy_Value_						account_string_table[ 5 ]
#define ST_Service_Context_						account_string_table[ 6 ]
#define ST_Service_Status_						account_string_table[ 7 ]
#define ST_Service_Type_						account_string_table[ 8 ]

extern wchar_t *menu_string_table[];

#define ST__About								menu_string_table[ 0 ]
#define ST__Edit								menu_string_table[ 1 ]
#define ST__File								menu_string_table[ 2 ]
#define ST__Help								menu_string_table[ 3 ]
#define ST__Log_In___							menu_string_table[ 4 ]
#define ST__Log_Off								menu_string_table[ 5 ]
#define ST__Options___							menu_string_table[ 6 ]
#define ST__Save_Call_Log___					menu_string_table[ 7 ]
#define ST__Tools								menu_string_table[ 8 ]
#define ST__View								menu_string_table[ 9 ]
#define ST_Add_Contact___						menu_string_table[ 10 ]
#define ST_Add_to_Forward_List___				menu_string_table[ 11 ]
#define ST_Add_to_Ignore_List					menu_string_table[ 12 ]
#define ST_Add_to_Ignore_List___				menu_string_table[ 13 ]
#define ST_Call_Phone_Number___					menu_string_table[ 14 ]
#define ST_Call_Cell_Phone_Number___			menu_string_table[ 15 ]
#define ST_Call_Fax_Number___					menu_string_table[ 16 ]
#define ST_Call_Forward_to_Phone_Number___		menu_string_table[ 17 ]
#define ST_Call_Forwarded_to_Phone_Number___	menu_string_table[ 18 ]
#define ST_Call_Home_Phone_Number___			menu_string_table[ 19 ]
#define ST_Call_Office_Phone_Number___			menu_string_table[ 20 ]
#define ST_Call_Other_Phone_Number___			menu_string_table[ 21 ]
#define ST_Call_Sent_to_Phone_Number___			menu_string_table[ 22 ]
#define ST_Call_Work_Phone_Number___			menu_string_table[ 23 ]
#define ST_Cancel_Import						menu_string_table[ 24 ]
#define ST_Close_Tab							menu_string_table[ 25 ]
#define ST_Contacts								menu_string_table[ 26 ]
#define ST_Copy_Caller_ID						menu_string_table[ 27 ]
#define ST_Copy_Caller_IDs						menu_string_table[ 28 ]
#define ST_Copy_Cell_Phone_Number				menu_string_table[ 29 ]
#define ST_Copy_Cell_Phone_Numbers				menu_string_table[ 30 ]
#define ST_Copy_Companies						menu_string_table[ 31 ]
#define ST_Copy_Company							menu_string_table[ 32 ]
#define ST_Copy_Date_and_Time					menu_string_table[ 33 ]
#define ST_Copy_Dates_and_Times					menu_string_table[ 34 ]
#define ST_Copy_Department						menu_string_table[ 35 ]
#define ST_Copy_Departments						menu_string_table[ 36 ]
#define ST_Copy_Email_Address					menu_string_table[ 37 ]
#define ST_Copy_Email_Addresses					menu_string_table[ 38 ]
#define ST_Copy_Fax_Number						menu_string_table[ 39 ]
#define ST_Copy_Fax_Numbers						menu_string_table[ 40 ]
#define ST_Copy_First_Name						menu_string_table[ 41 ]
#define ST_Copy_First_Names						menu_string_table[ 42 ]
#define ST_Copy_Forward_State					menu_string_table[ 43 ]
#define ST_Copy_Forward_States					menu_string_table[ 44 ]
#define ST_Copy_Forward_to_Phone_Number			menu_string_table[ 45 ]
#define ST_Copy_Forward_to_Phone_Numbers		menu_string_table[ 46 ]
#define ST_Copy_Forwarded_to_Phone_Number		menu_string_table[ 47 ]
#define ST_Copy_Forwarded_to_Phone_Numbers		menu_string_table[ 48 ]
#define ST_Copy_Home_Phone_Number				menu_string_table[ 49 ]
#define ST_Copy_Home_Phone_Numbers				menu_string_table[ 50 ]
#define ST_Copy_Ignore_State					menu_string_table[ 51 ]
#define ST_Copy_Ignore_States					menu_string_table[ 52 ]
#define ST_Copy_Job_Title						menu_string_table[ 53 ]
#define ST_Copy_Job_Titles						menu_string_table[ 54 ]
#define ST_Copy_Last_Name						menu_string_table[ 55 ]
#define ST_Copy_Last_Names						menu_string_table[ 56 ]
#define ST_Copy_Nickname						menu_string_table[ 57 ]
#define ST_Copy_Nicknames						menu_string_table[ 58 ]
#define ST_Copy_Office_Phone_Number				menu_string_table[ 59 ]
#define ST_Copy_Office_Phone_Numbers			menu_string_table[ 60 ]
#define ST_Copy_Other_Phone_Number				menu_string_table[ 61 ]
#define ST_Copy_Other_Phone_Numbers				menu_string_table[ 62 ]
#define ST_Copy_Phone_Number					menu_string_table[ 63 ]
#define ST_Copy_Phone_Numbers					menu_string_table[ 64 ]
#define ST_Copy_Profession						menu_string_table[ 65 ]
#define ST_Copy_Professions						menu_string_table[ 66 ]
#define ST_Copy_Reference						menu_string_table[ 67 ]
#define ST_Copy_References						menu_string_table[ 68 ]
#define ST_Copy_Selected						menu_string_table[ 69 ]
#define ST_Copy_Sent_to_Phone_Number			menu_string_table[ 70 ]
#define ST_Copy_Sent_to_Phone_Numbers			menu_string_table[ 71 ]
#define ST_Copy_Title							menu_string_table[ 72 ]
#define ST_Copy_Titles							menu_string_table[ 73 ]
#define ST_Copy_Total_Calls						menu_string_table[ 74 ]
#define ST_Copy_Web_Page						menu_string_table[ 75 ]
#define ST_Copy_Web_Pages						menu_string_table[ 76 ]
#define ST_Copy_Work_Phone_Number				menu_string_table[ 77 ]
#define ST_Copy_Work_Phone_Numbers				menu_string_table[ 78 ]
#define ST_Delete_Contact						menu_string_table[ 79 ]
#define ST_Dial_Phone_Number___					menu_string_table[ 80 ]
#define ST_E_xit								menu_string_table[ 81 ]
#define ST_Exit									menu_string_table[ 82 ]
#define ST_Edit_Contact___						menu_string_table[ 83 ]
#define ST_Edit_Forward_List_Entry___			menu_string_table[ 84 ]
#define ST_Export_Contacts___					menu_string_table[ 85 ]
#define ST_Forward_Incoming_Call___				menu_string_table[ 86 ]
#define ST_Ignore_Incoming_Call					menu_string_table[ 87 ]
#define ST_Import_Contacts___					menu_string_table[ 88 ]
#define ST_Log_In___							menu_string_table[ 89 ]
#define ST_Open_Caller_ID						menu_string_table[ 90 ]
#define ST_Open_Web_Page						menu_string_table[ 91 ]
#define ST_Options___							menu_string_table[ 92 ]
#define ST_Remove_from_Forward_List				menu_string_table[ 93 ]
#define ST_Remove_from_Ignore_List				menu_string_table[ 94 ]
#define ST_Remove_Selected						menu_string_table[ 95 ]
#define ST_Select_All							menu_string_table[ 96 ]
#define ST_Select__Columns___					menu_string_table[ 97 ]
#define ST_Select_Columns___					menu_string_table[ 98 ]
#define ST_Send_Email___						menu_string_table[ 99 ]
#define ST_Tabs									menu_string_table[ 100 ]
#define ST_Web_Server							menu_string_table[ 101 ]

extern wchar_t *options_string_table[];

#define ST_BTN___								options_string_table[ 0 ]
#define ST_12_hour								options_string_table[ 1 ]
#define ST_24_hour								options_string_table[ 2 ]
#define ST_Apply								options_string_table[ 3 ]
#define ST_Automatically_log_in					options_string_table[ 4 ]
#define ST_Background_color_					options_string_table[ 5 ]
#define ST_Bottom_Left							options_string_table[ 6 ]
#define ST_Bottom_Right							options_string_table[ 7 ]
#define ST_Cancel								options_string_table[ 8 ]
#define ST_Center								options_string_table[ 9 ]
#define ST_Close_to_System_Tray					options_string_table[ 10 ]
#define ST_Connection							options_string_table[ 11 ]
#define ST_Delay_Time_							options_string_table[ 12 ]
#define ST_Down									options_string_table[ 13 ]
#define ST_Enable_Call_Log_history				options_string_table[ 14 ]
#define ST_Enable_contact_picture_downloads		options_string_table[ 15 ]
#define ST_Enable_Popup_windows_				options_string_table[ 16 ]
#define ST_Enable_System_Tray_icon_				options_string_table[ 17 ]
#define ST_Font_								options_string_table[ 18 ]
#define ST_Font_color_							options_string_table[ 19 ]
#define ST_Font_shadow_color_					options_string_table[ 20 ]
#define ST_General								options_string_table[ 21 ]
#define ST_Gradient_background_color_			options_string_table[ 22 ]
#define ST_Gradient_direction_					options_string_table[ 23 ]
#define ST_Height_								options_string_table[ 24 ]
#define ST_Hide_window_border					options_string_table[ 25 ]
#define ST_Horizontal							options_string_table[ 26 ]
#define ST_Justify_text_						options_string_table[ 27 ]
#define ST_Left									options_string_table[ 28 ]
#define ST_Minimize_to_System_Tray				options_string_table[ 29 ]
#define ST_Play_sound_							options_string_table[ 30 ]
#define ST_Popup								options_string_table[ 31 ]
#define ST_Popup_Sound							options_string_table[ 32 ]
#define ST_Preview_Popup						options_string_table[ 33 ]
#define ST_Reconnect_upon_connection_loss_		options_string_table[ 34 ]
#define ST_Retries_								options_string_table[ 35 ]
#define ST_Right								options_string_table[ 36 ]
#define ST_Sample_								options_string_table[ 37 ]
#define ST_Screen_Position_						options_string_table[ 38 ]
#define ST_Show_line_							options_string_table[ 39 ]
#define ST_Silent_startup						options_string_table[ 40 ]
#define ST_SSL_2_0								options_string_table[ 41 ]
#define ST_SSL_3_0								options_string_table[ 42 ]
#define ST_SSL_version_							options_string_table[ 43 ]
#define ST_Time_format_							options_string_table[ 44 ]
#define ST_Timeout_								options_string_table[ 45 ]
#define ST_TLS_1_0								options_string_table[ 46 ]
#define ST_TLS_1_1								options_string_table[ 47 ]
#define ST_TLS_1_2								options_string_table[ 48 ]
#define ST_Top_Left								options_string_table[ 49 ]
#define ST_Top_Right							options_string_table[ 50 ]
#define ST_Transparency_						options_string_table[ 51 ]
#define ST_Up									options_string_table[ 52 ]
#define ST_Vertical								options_string_table[ 53 ]
#define ST_Width_								options_string_table[ 54 ]

extern wchar_t *login_string_table[];

#define ST_Username_		login_string_table[ 0 ]
#define ST_Password_		login_string_table[ 1 ]
#define ST_Remember_login	login_string_table[ 2 ]
#define ST_Log_In			login_string_table[ 3 ]
#define ST_Loggin_In___		login_string_table[ 4 ]
#define ST_Logged_In		login_string_table[ 5 ]
#define ST_Log_Off			login_string_table[ 6 ]

#endif
