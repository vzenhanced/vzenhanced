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

#include "lite_dlls.h"
#include "readwritelock.h"

bool use_rwl_library = true;

//
// -----------------------------------------------
//

pInitializeSRWLock			_InitializeSRWLock;

pAcquireSRWLockShared		_AcquireSRWLockShared;
pReleaseSRWLockShared		_ReleaseSRWLockShared;

pAcquireSRWLockExclusive	_AcquireSRWLockExclusive;
pReleaseSRWLockExclusive	_ReleaseSRWLockExclusive;

HMODULE hModule_kernel32 = NULL;

unsigned char kernel32_state = 0;	// 0 = Not running, 1 = running.

bool InitializeKernel32()
{
	if ( kernel32_state != KERNEL32_STATE_SHUTDOWN )
	{
		return true;
	}

	hModule_kernel32 = LoadLibraryDEMW( L"kernel32.dll" );

	if ( hModule_kernel32 == NULL )
	{
		return false;
	}

	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_kernel32, ( void ** )&_InitializeSRWLock, "InitializeSRWLock" ) )

	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_kernel32, ( void ** )&_AcquireSRWLockShared, "AcquireSRWLockShared" ) )
	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_kernel32, ( void ** )&_ReleaseSRWLockShared, "ReleaseSRWLockShared" ) )

	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_kernel32, ( void ** )&_AcquireSRWLockExclusive, "AcquireSRWLockExclusive" ) )
	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_kernel32, ( void ** )&_ReleaseSRWLockExclusive, "ReleaseSRWLockExclusive" ) )

	kernel32_state = KERNEL32_STATE_RUNNING;

	return true;
}

bool UnInitializeKernel32()
{
	if ( kernel32_state != KERNEL32_STATE_SHUTDOWN )
	{
		kernel32_state = KERNEL32_STATE_SHUTDOWN;

		return ( FreeLibrary( hModule_kernel32 ) == FALSE ? false : true );
	}

	return true;
}

VOID _AcquireSRWLockSharedFairly( PCRITICAL_SECTION CriticalSection, PSRWLOCK_L SRWLock )
{
	EnterCriticalSection( CriticalSection );

	_AcquireSRWLockShared( SRWLock );

	LeaveCriticalSection( CriticalSection );
}

VOID _AcquireSRWLockExclusiveFairly( PCRITICAL_SECTION CriticalSection, PSRWLOCK_L SRWLock )
{
	EnterCriticalSection( CriticalSection );

	_AcquireSRWLockExclusive( SRWLock );

	LeaveCriticalSection( CriticalSection );
}

//
// -----------------------------------------------
//

// Another version using 3 CRITICAL_SECTIONs.
// This version should be faster than the others below and should operate mostly in user-mode.

void InitializeReaderWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	InitializeCriticalSection( &reader_writer_lock->mutex_lock );
	InitializeCriticalSection( &reader_writer_lock->readers_access );
	InitializeCriticalSection( &reader_writer_lock->resource_access );

	reader_writer_lock->readers = 0;
}

void DeleteReaderWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	DeleteCriticalSection( &reader_writer_lock->mutex_lock );
	DeleteCriticalSection( &reader_writer_lock->readers_access );
	DeleteCriticalSection( &reader_writer_lock->resource_access );
}

void EnterReaderLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Wait for exclusive access to the reader.
	EnterCriticalSection( &reader_writer_lock->mutex_lock );

	// Wait to access the readers count if no other reader is modifying it.
	EnterCriticalSection( &reader_writer_lock->readers_access );

	// Aquire exclusive access to the resource for the readers if we're the first to read it.
	if ( ++reader_writer_lock->readers == 1 )
	{
		EnterCriticalSection( &reader_writer_lock->resource_access );
	}

	// Release exclusive access to any reader or writer.
	// We do this before releasing access to the readers count so that other threads can acquire the mutex lock as quickly as possible.
	LeaveCriticalSection( &reader_writer_lock->mutex_lock );

	// Allow access to the readers count.
	LeaveCriticalSection( &reader_writer_lock->readers_access );
}

void LeaveReaderLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Wait to access the readers count if no other reader is modifying it.
	EnterCriticalSection( &reader_writer_lock->readers_access );

	// Release exclusive access to the resource if there are no more readers.
	if ( --reader_writer_lock->readers == 0 )
	{
		LeaveCriticalSection( &reader_writer_lock->resource_access );
	}

	// Allow access to the readers count.
	LeaveCriticalSection( &reader_writer_lock->readers_access );
}

void EnterWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Wait for exclusive access to the writer.
	EnterCriticalSection( &reader_writer_lock->mutex_lock );

	// Wait for exclusive access to the resource.
	EnterCriticalSection( &reader_writer_lock->resource_access );

	// Release exclusive access to any reader or writer.
	// We do this before accessing the resource so that other threads can acquire the mutex lock as quickly as possible.
	LeaveCriticalSection( &reader_writer_lock->mutex_lock );
}

void LeaveWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Release exclusive access to the resource.
	LeaveCriticalSection( &reader_writer_lock->resource_access );
}

// -----------------------------------------------

/*
// Another version using 2 CRITICAL_SECTIONs and 1 Event.
// This version should be faster than the others below and should operate mostly in user-mode.

void InitializeReaderWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	InitializeCriticalSection( &reader_writer_lock->mutex_lock );
	InitializeCriticalSection( &reader_writer_lock->readers_access );

	reader_writer_lock->resource_access = CreateEvent( NULL, TRUE, TRUE, NULL );	// Initialized to signaled.

	reader_writer_lock->readers = 0;
}

void DeleteReaderWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	WaitForSingleObject( reader_writer_lock->resource_access, INFINITE );
	CloseHandle( reader_writer_lock->resource_access );

	DeleteCriticalSection( &reader_writer_lock->mutex_lock );
	DeleteCriticalSection( &reader_writer_lock->readers_access );
}

void EnterReaderLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Wait for exclusive access to the reader.
	EnterCriticalSection( &reader_writer_lock->mutex_lock );

	// Wait to access the readers count if no other reader is modifying it.
	EnterCriticalSection( &reader_writer_lock->readers_access );

	// Aquire exclusive access to the resource for the readers if we're the first to read it.
	if ( ++reader_writer_lock->readers == 1 )
	{
		ResetEvent( reader_writer_lock->resource_access );	// Set to non-signaled.
	}

	// Release exclusive access to any reader or writer.
	// We do this before releasing access to the readers count so that other threads can acquire the mutex lock as quickly as possible.
	LeaveCriticalSection( &reader_writer_lock->mutex_lock );

	// Allow access to the readers count.
	LeaveCriticalSection( &reader_writer_lock->readers_access );
}

void LeaveReaderLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Wait to access the readers count if no other reader is modifying it.
	EnterCriticalSection( &reader_writer_lock->readers_access );

	// Release exclusive access to the resource if there are no more readers.
	if ( --reader_writer_lock->readers == 0 )
	{
		SetEvent( reader_writer_lock->resource_access );	// Set to signaled.
	}

	// Allow access to the readers count.
	LeaveCriticalSection( &reader_writer_lock->readers_access );
}

void EnterWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Wait for exclusive access to the writer.
	EnterCriticalSection( &reader_writer_lock->mutex_lock );

	// Wait for exclusive access to the resource.
	WaitForSingleObject( reader_writer_lock->resource_access, INFINITE );
}

void LeaveWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Release exclusive access to the resource.
	LeaveCriticalSection( &reader_writer_lock->mutex_lock );
}
*/

// -----------------------------------------------

/*
// Another version using 2 Semaphores and 1 Event.
// This will be slower since it has to transition from user-mode to kernel-mode.

void InitializeReaderWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	reader_writer_lock->mutex_lock = CreateSemaphore( NULL, 1, 1, NULL );			// State is signaled (initialized to 1).
	reader_writer_lock->readers_access = CreateSemaphore( NULL, 1, 1, NULL );		// State is signaled (initialized to 1).

	reader_writer_lock->resource_access = CreateEvent( NULL, TRUE, TRUE, NULL );	// Initialized to signaled.

	reader_writer_lock->readers = 0;
}

void DeleteReaderWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	WaitForSingleObject( reader_writer_lock->resource_access, INFINITE );
	CloseHandle( reader_writer_lock->resource_access );

	WaitForSingleObject( reader_writer_lock->readers_access, INFINITE );
	CloseHandle( reader_writer_lock->readers_access );
	WaitForSingleObject( reader_writer_lock->mutex_lock, INFINITE );
	CloseHandle( reader_writer_lock->mutex_lock );
}

void EnterReaderLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Wait for exclusive access to the reader.
	WaitForSingleObject( reader_writer_lock->mutex_lock, INFINITE );

	// Wait to access the readers count if no other reader is modifying it.
	WaitForSingleObject( reader_writer_lock->readers_access, INFINITE );

	// Wait for exclusive access to the resource if we're the first to read it.
	if ( ++reader_writer_lock->readers == 1 )
	{
		ResetEvent( reader_writer_lock->resource_access );	// Set to non-signaled.
	}

	// Release exclusive access to any reader or writer.
	// We do this before releasing access to the readers count so that other threads can acquire the mutex lock as quickly as possible.
	ReleaseSemaphore( reader_writer_lock->mutex_lock, 1, NULL );

	// Allow access to the readers count.
	ReleaseSemaphore( reader_writer_lock->readers_access, 1, NULL );
}

void LeaveReaderLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Wait to access the readers count if no other reader is modifying it.
	WaitForSingleObject( reader_writer_lock->readers_access, INFINITE );

	// Release exclusive access to the resource if there are no more readers.
	if ( --reader_writer_lock->readers == 0 )
	{
		SetEvent( reader_writer_lock->resource_access );	// Set to signaled.
	}

	// Allow access to the readers count.
	ReleaseSemaphore( reader_writer_lock->readers_access, 1, NULL );
}

void EnterWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Wait for exclusive access to the writer.
	WaitForSingleObject( reader_writer_lock->mutex_lock, INFINITE );

	// Wait for exclusive access to the resource.
	WaitForSingleObject( reader_writer_lock->resource_access, INFINITE );
}

void LeaveWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Release exclusive access to any reader or writer.
	ReleaseSemaphore( reader_writer_lock->mutex_lock, 1, NULL );
}
*/

// -----------------------------------------------

/*
// Another version using 3 Semaphores.
// This will be slower since it has to transition from user-mode to kernel-mode.

void InitializeReaderWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	reader_writer_lock->mutex_lock = CreateSemaphore( NULL, 1, 1, NULL );		// State is signaled (initialized to 1).
	reader_writer_lock->readers_access = CreateSemaphore( NULL, 1, 1, NULL );	// State is signaled (initialized to 1).
	reader_writer_lock->resource_access = CreateSemaphore( NULL, 1, 1, NULL );	// State is signaled (initialized to 1).

	reader_writer_lock->readers = 0;
}

void DeleteReaderWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	WaitForSingleObject( reader_writer_lock->resource_access, INFINITE );
	CloseHandle( reader_writer_lock->resource_access );
	WaitForSingleObject( reader_writer_lock->readers_access, INFINITE );
	CloseHandle( reader_writer_lock->readers_access );
	WaitForSingleObject( reader_writer_lock->mutex_lock, INFINITE );
	CloseHandle( reader_writer_lock->mutex_lock );
}

void EnterReaderLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Wait for exclusive access to the reader.
	WaitForSingleObject( reader_writer_lock->mutex_lock, INFINITE );

	// Wait to access the readers count if no other reader is modifying it.
	WaitForSingleObject( reader_writer_lock->readers_access, INFINITE );

	// Wait for exclusive access to the resource if we're the first to read it.
	if ( ++reader_writer_lock->readers == 1 )
	{
		WaitForSingleObject( reader_writer_lock->resource_access, INFINITE );
	}

	// Release exclusive access to any reader or writer.
	// We do this before releasing access to the readers count so that other threads can acquire the mutex lock as quickly as possible.
	ReleaseSemaphore( reader_writer_lock->mutex_lock, 1, NULL );

	// Allow access to the readers count.
	ReleaseSemaphore( reader_writer_lock->readers_access, 1, NULL );
}

void LeaveReaderLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Wait to access the readers count if no other reader is modifying it.
	WaitForSingleObject( reader_writer_lock->readers_access, INFINITE );

	// Release exclusive access to the resource if there are no more readers.
	if ( --reader_writer_lock->readers == 0 )
	{
		ReleaseSemaphore( reader_writer_lock->resource_access, 1, NULL );
	}

	// Allow access to the readers count.
	ReleaseSemaphore( reader_writer_lock->readers_access, 1, NULL );
}

void EnterWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Wait for exclusive access to the writer.
	WaitForSingleObject( reader_writer_lock->mutex_lock, INFINITE );

	// Wait for exclusive access to the resource.
	WaitForSingleObject( reader_writer_lock->resource_access, INFINITE );

	// Release exclusive access to any reader or writer.
	// We do this before accessing the resource so that other threads can acquire the mutex lock as quickly as possible.
	ReleaseSemaphore( reader_writer_lock->mutex_lock, 1, NULL );
}

void LeaveWriterLock( READER_WRITER_LOCK *reader_writer_lock )
{
	// Release exclusive access to the resource.
	ReleaseSemaphore( reader_writer_lock->resource_access, 1, NULL );
}
*/
