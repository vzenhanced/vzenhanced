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

#ifndef _LIST_OPERATIONS_H
#define _LIST_OPERATIONS_H

THREAD_RETURN remove_items( void *pArguments );

THREAD_RETURN update_ignore_list( void *pArguments );
THREAD_RETURN update_ignore_cid_list( void *pArguments );
THREAD_RETURN update_forward_list( void *pArguments );
THREAD_RETURN update_forward_cid_list( void *pArguments );
THREAD_RETURN update_contact_list( void *pArguments );
THREAD_RETURN update_call_log( void *pArguments );

THREAD_RETURN read_call_log_history( void *pArguments );
THREAD_RETURN save_call_log( void *file_path );

THREAD_RETURN copy_items( void *pArguments );

#endif
