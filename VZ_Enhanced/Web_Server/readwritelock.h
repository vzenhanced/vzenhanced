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

#ifndef _READWRITELOCK_H
#define _READWRITELOCK_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//
// -----------------------------------------------
//

// Slim Read Write Lock for Vista and above. The library functions are not fair or FIFO.

// We're going to dynamically load these functions so we can switch to another method if the system (XP) doesn't support them.
// If they were to get statically linked in, then older systems (XP) won't be able to run the web server.

typedef struct _SRWLOCK_L
{                            
	PVOID Ptr;                                       
} SRWLOCK_L, *PSRWLOCK_L;

#define KERNEL32_STATE_SHUTDOWN		0
#define KERNEL32_STATE_RUNNING		1

typedef VOID ( WINAPI *pInitializeSRWLock )( PSRWLOCK_L SRWLock );

// Reader
typedef VOID ( WINAPI *pAcquireSRWLockShared )( PSRWLOCK_L SRWLock );
typedef VOID ( WINAPI *pReleaseSRWLockShared )( PSRWLOCK_L SRWLock );

// Writer
typedef VOID ( WINAPI *pAcquireSRWLockExclusive )( PSRWLOCK_L SRWLock );
typedef VOID ( WINAPI *pReleaseSRWLockExclusive )( PSRWLOCK_L SRWLock );

extern pInitializeSRWLock			_InitializeSRWLock;

extern pAcquireSRWLockShared		_AcquireSRWLockShared;
extern pReleaseSRWLockShared		_ReleaseSRWLockShared;

extern pAcquireSRWLockExclusive		_AcquireSRWLockExclusive;
extern pReleaseSRWLockExclusive		_ReleaseSRWLockExclusive;

extern unsigned char kernel32_state;

bool InitializeKernel32();
bool UnInitializeKernel32();

// These implementations are not FIFO, and thus, semi-fair.
VOID _AcquireSRWLockSharedFairly( PCRITICAL_SECTION CriticalSection, PSRWLOCK_L SRWLock );
VOID _AcquireSRWLockExclusiveFairly( PCRITICAL_SECTION CriticalSection, PSRWLOCK_L SRWLock );

//
// -----------------------------------------------
//

// Read Write Locks for XP and below. The implementations are not FIFO, and thus, semi-fair.

// Version 1 with 3 CRITICAL_SECTIONs. (Fastest)

struct READER_WRITER_LOCK
{
	CRITICAL_SECTION mutex_lock;
	CRITICAL_SECTION readers_access;
	CRITICAL_SECTION resource_access;

	unsigned long readers;
};

/*
// Version 2 with 2 CRITICAL_SECTIONs and 1 Event. (Faster)

struct READER_WRITER_LOCK
{
	CRITICAL_SECTION mutex_lock;
	CRITICAL_SECTION readers_access;

	HANDLE resource_access;

	unsigned long readers;
};
*/

/*
// Version 3 with 2 Semaphores and 1 Event. (Slower)

struct READER_WRITER_LOCK
{
	HANDLE mutex_lock;
	HANDLE readers_access;

	HANDLE resource_access;

	unsigned int readers;
};
*/

/*
// Version 4 with 3 Semaphores. (Slowest)

struct READER_WRITER_LOCK
{
	HANDLE mutex_lock;
	HANDLE readers_access;
	HANDLE resource_access;

	unsigned int readers;
};
*/

void InitializeReaderWriterLock( READER_WRITER_LOCK *reader_writer_lock );
void DeleteReaderWriterLock( READER_WRITER_LOCK *reader_writer_lock );

void EnterReaderLock( READER_WRITER_LOCK *reader_writer_lock );
void LeaveReaderLock( READER_WRITER_LOCK *reader_writer_lock );

void EnterWriterLock( READER_WRITER_LOCK *reader_writer_lock );
void LeaveWriterLock( READER_WRITER_LOCK *reader_writer_lock );

//
// -----------------------------------------------
//

extern bool use_rwl_library;	// This will be used to determine if we should use the Kernel32 library functions, or our own reader-writer lock functions.

#endif
