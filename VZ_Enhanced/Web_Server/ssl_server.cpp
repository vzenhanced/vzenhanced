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

#include "ssl_server.h"
#include "connection.h"

#include "lite_dlls.h"
#include "lite_ntdll.h"
#include "lite_rpcrt4.h"
#include "lite_advapi32.h"
#include "lite_crypt32.h"
#include "lite_shell32.h"
#include "lite_user32.h"

#include "string_tables.h"

HMODULE g_hSecurity = NULL;
PSecurityFunctionTableA g_pSSPI;

unsigned char ssl_state = SSL_STATE_SHUTDOWN;

typedef BOOL ( WINAPI *pSslEmptyCacheW )( LPSTR pszTargetName, DWORD dwFlags );
pSslEmptyCacheW	_SslEmptyCacheW;

CredHandle g_hCreds;

void ResetCredentials()
{
	if ( SecIsValidHandle( &g_hCreds ) )
	{
		g_pSSPI->FreeCredentialsHandle( &g_hCreds );
		SecInvalidateHandle( &g_hCreds );
	}
}

int SSL_library_init( void )
{
	if ( ssl_state != SSL_STATE_SHUTDOWN )
	{
		return 1;
	}

	g_hSecurity = LoadLibraryDEMW( L"schannel.dll" );
	if ( g_hSecurity == NULL )
	{
		return 0;
	}

	_SslEmptyCacheW = ( pSslEmptyCacheW )GetProcAddress( g_hSecurity, "SslEmptyCacheW" );
	if ( _SslEmptyCacheW == NULL )
	{
		FreeLibrary( g_hSecurity );
		g_hSecurity = NULL;
		return 0;
	}

	INIT_SECURITY_INTERFACE_A pInitSecurityInterface = ( INIT_SECURITY_INTERFACE_A )GetProcAddress( g_hSecurity, SECURITY_ENTRYPOINT_ANSIA );
	if ( pInitSecurityInterface != NULL ) 
	{
		g_pSSPI = pInitSecurityInterface();
	}

	if ( g_pSSPI == NULL )
	{
		FreeLibrary( g_hSecurity );
		g_hSecurity = NULL;
		return 0;
	}

	ssl_state = SSL_STATE_RUNNING;

	SecInvalidateHandle( &g_hCreds );
	
	return 1;
}

int SSL_library_uninit( void )
{
	BOOL ret = 0;

	if ( ssl_state != SSL_STATE_SHUTDOWN )
	{
		ResetCredentials();

		_SslEmptyCacheW( NULL, 0 );

		ret = FreeLibrary( g_hSecurity );
	}

	ssl_state = SSL_STATE_SHUTDOWN;

	return ret;
}

SSL *SSL_new( DWORD protocol )
{
	if ( g_hSecurity == NULL )
	{
		return NULL;
	}

	SSL *ssl = ( SSL * )GlobalAlloc( GPTR, sizeof( SSL ) );	// Zeros the structure as well.
	if ( ssl == NULL )
	{
		return NULL;
	}

	//ssl->dwProtocol = protocol;

	if ( !SecIsValidHandle( &g_hCreds ) )
	{
		SCHANNEL_CRED SchannelCred;
		TimeStamp tsExpiry;

		_memzero( &SchannelCred, sizeof( SchannelCred ) );

		SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
		SchannelCred.cCreds = 1;
		SchannelCred.paCred = &g_pCertContext;
		SchannelCred.dwMinimumCipherStrength = -1;
		SchannelCred.grbitEnabledProtocols = protocol;//ssl->dwProtocol;
		SchannelCred.dwFlags = ( SCH_CRED_NO_SYSTEM_MAPPER | SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_REVOCATION_CHECK_CHAIN );

		SECURITY_STATUS scRet = g_pSSPI->AcquireCredentialsHandleA(
										NULL,
										UNISP_NAME_A,
										SECPKG_CRED_INBOUND,
										NULL,
										&SchannelCred,
										NULL,
										NULL,
										&g_hCreds,
										&tsExpiry );

		if ( scRet != SEC_E_OK )
		{
			ResetCredentials();

			GlobalFree( ssl );
			return NULL;
		}
	}


	return ssl;
}

void SSL_free( SSL *ssl )
{
	if ( ssl == NULL )
	{
		return;
	}

	if ( ssl->sdd.OutBuffers[ 0 ].pvBuffer != NULL )
	{
		g_pSSPI->FreeContextBuffer( ssl->sdd.OutBuffers[ 0 ].pvBuffer );

		ssl->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
	}

	if ( ssl->ad.OutBuffers[ 0 ].pvBuffer != NULL )
	{
		g_pSSPI->FreeContextBuffer( ssl->ad.OutBuffers[ 0 ].pvBuffer );

		ssl->ad.OutBuffers[ 0 ].pvBuffer = NULL;
	}

	if ( ssl->sd.pbDataBuffer != NULL )
	{
		GlobalFree( ssl->sd.pbDataBuffer );
		ssl->sd.pbDataBuffer = NULL;
	}

	if ( ssl->pbRecDataBuf != NULL )
	{
		GlobalFree( ssl->pbRecDataBuf );
		ssl->pbRecDataBuf = NULL;
	}

	if ( ssl->pbIoBuffer != NULL )
	{
		GlobalFree( ssl->pbIoBuffer );
		ssl->pbIoBuffer = NULL;
	}

	if ( SecIsValidHandle( &ssl->hContext ) )
	{
		g_pSSPI->DeleteSecurityContext( &ssl->hContext );
		SecInvalidateHandle( &ssl->hContext );
	}

	GlobalFree( ssl );
}

SECURITY_STATUS WSAAPI SSL_WSASend( SOCKET_CONTEXT *socket_context, WSABUF *send_buf, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent )
{
	sent = false;
	if ( socket_context != NULL && socket_context->ssl != NULL && socket_context->io_context != NULL )
	{
		SSL *ssl = socket_context->ssl;

		DWORD dwFlags = 0;

		SecBufferDesc Message;

		WSABUF encrypted_buf;

		// ssl->sd.pbDataBuffer is freed when we clean up the connection.
		if ( ssl->sd.pbDataBuffer == NULL )
		{
			ssl->sd.scRet = g_pSSPI->QueryContextAttributesA( &ssl->hContext, SECPKG_ATTR_STREAM_SIZES, &ssl->sd.Sizes );
			if ( ssl->sd.scRet != SEC_E_OK )
			{
				return ssl->sd.scRet;
			}

			// The size includes the SSL header, max message length (16 KB), and SSL trailer.
			ssl->sd.pbDataBuffer = ( PUCHAR )GlobalAlloc( GPTR, ( ssl->sd.Sizes.cbHeader + ssl->sd.Sizes.cbMaximumMessage + ssl->sd.Sizes.cbTrailer ) );
		}

		// Copy our message to the buffer. Truncate the message if it's larger than the maximum allowed size (16 KB).
		ssl->sd.cbMessage = min( ssl->sd.Sizes.cbMaximumMessage, ( DWORD )send_buf->len );
		_memcpy_s( ssl->sd.pbDataBuffer + ssl->sd.Sizes.cbHeader, ssl->sd.Sizes.cbMaximumMessage, send_buf->buf, ssl->sd.cbMessage );

		send_buf->len -= ssl->sd.cbMessage;
		send_buf->buf += ssl->sd.cbMessage;

		// Header location. (Beginning of the data buffer).
		ssl->sd.Buffers[ 0 ].pvBuffer = ssl->sd.pbDataBuffer;
		ssl->sd.Buffers[ 0 ].cbBuffer = ssl->sd.Sizes.cbHeader;
		ssl->sd.Buffers[ 0 ].BufferType = SECBUFFER_STREAM_HEADER;

		// Message location. (After the header).
		ssl->sd.Buffers[ 1 ].pvBuffer = ssl->sd.pbDataBuffer + ssl->sd.Sizes.cbHeader;
		ssl->sd.Buffers[ 1 ].cbBuffer = ssl->sd.cbMessage;
		ssl->sd.Buffers[ 1 ].BufferType = SECBUFFER_DATA;

		// Trailer location. (After the message).
		ssl->sd.Buffers[ 2 ].pvBuffer = ssl->sd.pbDataBuffer + ssl->sd.Sizes.cbHeader + ssl->sd.cbMessage;
		ssl->sd.Buffers[ 2 ].cbBuffer = ssl->sd.Sizes.cbTrailer;
		ssl->sd.Buffers[ 2 ].BufferType = SECBUFFER_STREAM_TRAILER;

		ssl->sd.Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

		Message.ulVersion = SECBUFFER_VERSION;
		Message.cBuffers = 4;
		Message.pBuffers = ssl->sd.Buffers;

		if ( g_pSSPI->EncryptMessage != NULL )
		{
			ssl->sd.scRet = g_pSSPI->EncryptMessage( &ssl->hContext, 0, &Message, 0 );
		}
		else
		{
			ssl->sd.scRet = ( ( ENCRYPT_MESSAGE_FN )g_pSSPI->Reserved3 )( &ssl->hContext, 0, &Message, 0 );
		}

		if ( FAILED( ssl->sd.scRet ) )
		{
			return ssl->sd.scRet;
		}

		encrypted_buf.buf = ( char * )ssl->sd.pbDataBuffer;
		encrypted_buf.len = ssl->sd.Buffers[ 0 ].cbBuffer + ssl->sd.Buffers[ 1 ].cbBuffer + ssl->sd.Buffers[ 2 ].cbBuffer; // Calculate encrypted packet size

		socket_context->io_context->nTotalBytes += ( ssl->sd.Buffers[ 0 ].cbBuffer + ssl->sd.Buffers[ 2 ].cbBuffer );	// Add the header and trailer to the total size (for each packet).

		sent = true;

		int nRet = _WSASend( ssl->s, &encrypted_buf, 1, NULL, dwFlags, lpWSAOverlapped, NULL );
		if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
		{
			socket_context->io_context->nTotalBytes = 0;

			sent = false;
			g_pSSPI->DeleteSecurityContext( &ssl->hContext );
			SecInvalidateHandle( &ssl->hContext );

			ssl->sd.scRet = SEC_E_INTERNAL_ERROR;
		}

		return ssl->sd.scRet;
	}
	else
	{
		return SEC_E_INTERNAL_ERROR;
	}
}


SECURITY_STATUS WSAAPI SSL_WSARecv( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent )
{
	sent = false;
	if ( socket_context != NULL && socket_context->ssl != NULL && socket_context->io_context != NULL )
	{
		SSL *ssl = socket_context->ssl;

		DWORD dwFlags = 0;
		WSABUF encrypted_buf;
		int nRet = 0;

		if ( ssl->cbIoBuffer == 0 || ssl->rd.scRet == SEC_E_INCOMPLETE_MESSAGE )
		{
			if ( ssl->sbIoBuffer <= ssl->cbIoBuffer )
			{
				ssl->sbIoBuffer += 2048;

				if ( ssl->pbIoBuffer == NULL )
				{
					ssl->pbIoBuffer = ( PUCHAR )GlobalAlloc( GPTR, ssl->sbIoBuffer );
				}
				else
				{
					ssl->pbIoBuffer = ( PUCHAR )GlobalReAlloc( ssl->pbIoBuffer, ssl->sbIoBuffer, GMEM_MOVEABLE );
				}
			}

			sent = true;

			encrypted_buf.buf = ( char * )ssl->pbIoBuffer + ssl->cbIoBuffer;
			encrypted_buf.len = ssl->sbIoBuffer - ssl->cbIoBuffer;
			nRet = _WSARecv( ssl->s, &encrypted_buf, 1, NULL, &dwFlags, lpWSAOverlapped, NULL );

			if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
			{
				sent = false;
				ssl->rd.scRet = SEC_E_INTERNAL_ERROR;
				return ssl->rd.scRet;
			}

			return SEC_E_OK;
		}

		return ssl->rd.scRet;
	}
	else
	{
		return SEC_E_INTERNAL_ERROR;
	}
}

SECURITY_STATUS WSAAPI SSL_WSAAccept( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( socket_context != NULL && socket_context->ssl != NULL && socket_context->io_context != NULL )
	{
		SSL *ssl = socket_context->ssl;
		scRet = ssl->ad.scRet;

		DWORD dwFlags = 0;

		// Begin our handshake.
		ssl->ad.fInitContext = true;

		ssl->cbIoBuffer = 0;

		ssl->ad.fDoRead = true;

		ssl->ad.scRet = SEC_I_CONTINUE_NEEDED;
		scRet = ssl->ad.scRet;

		WSABUF encrypted_buf;

		// If buffer not large enough reallocate buffer
		if ( ssl->sbIoBuffer <= ssl->cbIoBuffer )
		{
			ssl->sbIoBuffer += 2048;
			if ( ssl->pbIoBuffer == NULL )
			{
				ssl->pbIoBuffer = ( PUCHAR )GlobalAlloc( GPTR, ssl->sbIoBuffer );
			}
			else
			{
				ssl->pbIoBuffer = ( PUCHAR )GlobalReAlloc( ssl->pbIoBuffer, ssl->sbIoBuffer, GMEM_MOVEABLE );
			}
		}

		sent = true;

		encrypted_buf.buf = ( char * )ssl->pbIoBuffer + ssl->cbIoBuffer;
		encrypted_buf.len = ssl->sbIoBuffer - ssl->cbIoBuffer;
		int nRet = _WSARecv( ssl->s, &encrypted_buf, 1, NULL, &dwFlags, lpWSAOverlapped, NULL );

		if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
		{
			sent = false;
			ssl->ad.scRet = SEC_E_INTERNAL_ERROR;
			scRet = ssl->ad.scRet;
		}
	}

	return scRet;
}

SECURITY_STATUS SSL_WSAAccept_Reply( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( socket_context != NULL && socket_context->ssl != NULL && socket_context->io_context != NULL )
	{
		WSABUF encrypted_buf;

		SecBufferDesc InBuffer;
		SecBufferDesc OutBuffer;
		TimeStamp tsExpiry;
		DWORD dwSSPIFlags;
		DWORD dwSSPIOutFlags;

		DWORD dwFlags = 0;

		SSL *ssl = socket_context->ssl;
		scRet = ssl->ad.scRet;

		dwSSPIFlags = ASC_REQ_SEQUENCE_DETECT	|
					  ASC_REQ_REPLAY_DETECT		|
					  ASC_REQ_CONFIDENTIALITY	|
					  ASC_REQ_EXTENDED_ERROR	|
					  ASC_REQ_ALLOCATE_MEMORY	|
					  ASC_REQ_STREAM;

		if ( scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_INCOMPLETE_CREDENTIALS ) 
		{
			// Set up the input buffers. Buffer 0 is used to pass in data
			// received from the server. Schannel will consume some or all
			// of this. Leftover data (if any) will be placed in buffer 1 and
			// given a buffer type of SECBUFFER_EXTRA.

			ssl->ad.InBuffers[ 0 ].pvBuffer = ssl->pbIoBuffer;
			ssl->ad.InBuffers[ 0 ].cbBuffer = ssl->cbIoBuffer;
			ssl->ad.InBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;

			ssl->ad.InBuffers[ 1 ].pvBuffer = NULL;
			ssl->ad.InBuffers[ 1 ].cbBuffer = 0;
			ssl->ad.InBuffers[ 1 ].BufferType = SECBUFFER_EMPTY;

			InBuffer.cBuffers = 2;
			InBuffer.pBuffers = ssl->ad.InBuffers;
			InBuffer.ulVersion = SECBUFFER_VERSION;

			// Set up the output buffers. These are initialized to NULL
			// so as to make it less likely we'll attempt to free random
			// garbage later.

			ssl->ad.OutBuffers[ 0 ].pvBuffer = NULL;
			ssl->ad.OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
			ssl->ad.OutBuffers[ 0 ].cbBuffer = 0;

			OutBuffer.cBuffers = 1;
			OutBuffer.pBuffers = ssl->ad.OutBuffers;
			OutBuffer.ulVersion = SECBUFFER_VERSION;

			/*if ( ssl->ad.fInitContext == false )
			{
				dwSSPIFlags |= ASC_REQ_MUTUAL_AUTH;
			}*/

			ssl->ad.scRet = g_pSSPI->AcceptSecurityContext(
									&g_hCreds,
									( ssl->ad.fInitContext == true ? NULL : &ssl->hContext ),
									&InBuffer,
									dwSSPIFlags,
									SECURITY_NATIVE_DREP,
									( ssl->ad.fInitContext == true ? &ssl->hContext : NULL ),
									&OutBuffer,
									&dwSSPIOutFlags,
									&tsExpiry );

			scRet = ssl->ad.scRet;

			ssl->ad.fInitContext = false;

			// If success (or if the error was one of the special extended ones), send the contents of the output buffer to the client.
			if ( scRet == SEC_E_OK || scRet == SEC_I_CONTINUE_NEEDED || FAILED( scRet ) && ( dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR ) )
			{
				if ( ssl->ad.OutBuffers[ 0 ].cbBuffer != 0 && ssl->ad.OutBuffers[ 0 ].pvBuffer != NULL )
				{
					sent = true;

					encrypted_buf.buf = ( char * )ssl->ad.OutBuffers[ 0 ].pvBuffer;
					encrypted_buf.len = ssl->ad.OutBuffers[ 0 ].cbBuffer;

					socket_context->io_context->nTotalBytes = ssl->ad.OutBuffers[ 0 ].cbBuffer;

					int nRet = _WSASend( ssl->s, &encrypted_buf, 1, NULL, dwFlags, lpWSAOverlapped, NULL );

					if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
					{
						socket_context->io_context->nTotalBytes = 0;

						sent = false;
						g_pSSPI->FreeContextBuffer( ssl->ad.OutBuffers[ 0 ].pvBuffer );
						ssl->ad.OutBuffers[ 0 ].pvBuffer = NULL;
						g_pSSPI->DeleteSecurityContext( &ssl->hContext );
						SecInvalidateHandle( &ssl->hContext );

						return SEC_E_INTERNAL_ERROR;
					}

					// ssl->ad.OutBuffers[ 0 ].pvBuffer is freed in SSL_WSAAccept_Response (assuming we get a response).

					return SEC_I_CONTINUE_NEEDED;
				}
				else if ( scRet == SEC_I_CONTINUE_NEEDED )	// Safari likes to send us 0 byte output buffers. Request more data until we get something.
				{
					// We're going to call this function again when we receive more data.
					socket_context->io_context->IOOperation = ClientIoHandshakeReply;

					ssl->cbIoBuffer = 0;

					sent = true;

					encrypted_buf.buf = ( char * )ssl->pbIoBuffer + ssl->cbIoBuffer;
					encrypted_buf.len = ssl->sbIoBuffer - ssl->cbIoBuffer;
					int nRet = _WSARecv( ssl->s, &encrypted_buf, 1, NULL, &dwFlags, lpWSAOverlapped, NULL );

					if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
					{
						sent = false;

						if ( ssl->pbIoBuffer != NULL )
						{
							GlobalFree( ssl->pbIoBuffer );
							ssl->pbIoBuffer = NULL;
						}

						ssl->sbIoBuffer = 0;

						g_pSSPI->DeleteSecurityContext( &ssl->hContext );
						SecInvalidateHandle( &ssl->hContext );

						return SEC_E_INTERNAL_ERROR;
					}

					return SEC_I_COMPLETE_AND_CONTINUE;
				}
				else if ( scRet == SEC_E_OK )
				{
					// Store remaining data for further use
					if ( ssl->ad.InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA )	// Seems to occur with Opera 12. The extra data is actually the HTTP request.
					{
						_memmove( ssl->pbIoBuffer, ssl->pbIoBuffer + ( ssl->cbIoBuffer - ssl->ad.InBuffers[ 1 ].cbBuffer ), ssl->ad.InBuffers[ 1 ].cbBuffer );
						ssl->cbIoBuffer = ssl->ad.InBuffers[ 1 ].cbBuffer;

						// We have read data in this extra buffer. Post a read.
						socket_context->io_context->LastIOOperation = ClientIoReadMore;
					}
					else
					{
						ssl->cbIoBuffer = 0;

						if ( ssl->pbIoBuffer != NULL )
						{
							GlobalFree( ssl->pbIoBuffer );
							ssl->pbIoBuffer = NULL;
						}

						ssl->sbIoBuffer = 0;
					}

					if ( ssl->ad.OutBuffers[ 0 ].pvBuffer != NULL )
					{
						g_pSSPI->FreeContextBuffer( ssl->ad.OutBuffers[ 0 ].pvBuffer );
						ssl->ad.OutBuffers[ 0 ].pvBuffer = NULL;
					}
				}
			}
		}
	}

	return scRet;
}


SECURITY_STATUS SSL_WSAAccept_Response( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( socket_context != NULL && socket_context->ssl != NULL && socket_context->io_context != NULL )
	{
		DWORD dwFlags = 0;

		WSABUF encrypted_buf;

		SSL *ssl = socket_context->ssl;
		scRet = ssl->ad.scRet;

		// Created in our call to SSL_WSAAccept_Reply.
		g_pSSPI->FreeContextBuffer( ssl->ad.OutBuffers[ 0 ].pvBuffer );
		ssl->ad.OutBuffers[ 0 ].pvBuffer = NULL;

		if ( scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_INCOMPLETE_CREDENTIALS ) 
		{
			// server just requested client authentication. 
			if ( scRet == SEC_I_INCOMPLETE_CREDENTIALS )
			{
				// Go around again.
				ssl->ad.fDoRead = false;
				ssl->ad.scRet = SEC_I_CONTINUE_NEEDED;

				return ssl->ad.scRet;
			}

			// we need to read more data from the server and try again.
			if ( scRet == SEC_E_INCOMPLETE_MESSAGE ) 
			{
				return scRet;
			}

			// Copy any leftover data from the buffer, and go around again.
			if ( ssl->ad.InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA )
			{
				_memmove( ssl->pbIoBuffer, ssl->pbIoBuffer + ( ssl->cbIoBuffer - ssl->ad.InBuffers[ 1 ].cbBuffer ), ssl->ad.InBuffers[ 1 ].cbBuffer );

				ssl->cbIoBuffer = ssl->ad.InBuffers[ 1 ].cbBuffer;
			}
			else
			{
				ssl->cbIoBuffer = 0;
			}

			// Read client data.
			if ( ssl->cbIoBuffer == 0 )
			{
				if ( ssl->ad.fDoRead == true )
				{
					// Reallocate the buffer if it needs to be larger.
					if ( ssl->sbIoBuffer <= ssl->cbIoBuffer )
					{
						ssl->sbIoBuffer += 2048;
						if ( ssl->pbIoBuffer == NULL )
						{
							ssl->pbIoBuffer = ( PUCHAR )GlobalAlloc( GPTR, ssl->sbIoBuffer );
						}
						else
						{
							ssl->pbIoBuffer = ( PUCHAR )GlobalReAlloc( ssl->pbIoBuffer, ssl->sbIoBuffer, GMEM_MOVEABLE );
						}
					}

					sent = true;

					encrypted_buf.buf = ( char * )ssl->pbIoBuffer + ssl->cbIoBuffer;
					encrypted_buf.len = ssl->sbIoBuffer - ssl->cbIoBuffer;
					int nRet = _WSARecv( ssl->s, &encrypted_buf, 1, NULL, &dwFlags, lpWSAOverlapped, NULL );

					if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
					{
						sent = false;

						if ( ssl->pbIoBuffer != NULL )
						{
							GlobalFree( ssl->pbIoBuffer );
							ssl->pbIoBuffer = NULL;
						}

						ssl->sbIoBuffer = 0;

						g_pSSPI->DeleteSecurityContext( &ssl->hContext );
						SecInvalidateHandle( &ssl->hContext );

						return SEC_E_INTERNAL_ERROR;
					}
				}
				else
				{
					ssl->ad.fDoRead = true;
				}
			}

			return scRet;
		}
		else if ( scRet == SEC_E_OK )	// Handshake completed successfully.
		{
			// Store remaining data for further use
			if ( ssl->ad.InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA )	// Seems to occur with Opera 12. The extra data is actually the HTTP request.
			{
				_memmove( ssl->pbIoBuffer, ssl->pbIoBuffer + ( ssl->cbIoBuffer - ssl->ad.InBuffers[ 1 ].cbBuffer ), ssl->ad.InBuffers[ 1 ].cbBuffer );
				ssl->cbIoBuffer = ssl->ad.InBuffers[ 1 ].cbBuffer;

				// We have read data in this extra buffer. Post a read.
				socket_context->io_context->LastIOOperation = ClientIoReadMore;
			}
			else
			{
				ssl->cbIoBuffer = 0;

				if ( ssl->pbIoBuffer != NULL )
				{
					GlobalFree( ssl->pbIoBuffer );
					ssl->pbIoBuffer = NULL;
				}

				ssl->sbIoBuffer = 0;
			}
		}
		else if ( FAILED( scRet ) )	// Delete the security context in the case of a fatal error.
		{
			g_pSSPI->DeleteSecurityContext( &ssl->hContext );
			SecInvalidateHandle( &ssl->hContext );
		}
	}

	// We've completed everything above. Return our final status.
	return scRet;
}

SECURITY_STATUS SSL_WSAShutdown( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped, bool &sent )
{
	sent = false;
	if ( socket_context != NULL && socket_context->ssl != NULL && socket_context->io_context != NULL )
	{
		DWORD dwType;

		DWORD dwSSPIFlags;
		DWORD dwSSPIOutFlags;
		TimeStamp tsExpiry;
		DWORD Status;

		SecBufferDesc OutBuffer;

		WSABUF encrypted_buf;

		DWORD dwFlags = 0;

		SSL *ssl = socket_context->ssl;

		if ( ssl == NULL )
		{
			return SOCKET_ERROR;
		}

		dwType = SCHANNEL_SHUTDOWN;

		ssl->sdd.OutBuffers[ 0 ].pvBuffer = &dwType;	// NEVER MAKE THIS NULL. System will break and restart.
		ssl->sdd.OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
		ssl->sdd.OutBuffers[ 0 ].cbBuffer = sizeof( dwType );

		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = ssl->sdd.OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		Status = g_pSSPI->ApplyControlToken( &ssl->hContext, &OutBuffer );
		if ( FAILED( Status ) )
		{
			ssl->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
			return Status;
		}

		// Build an SSL close notify message.

		ssl->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
		ssl->sdd.OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
		ssl->sdd.OutBuffers[ 0 ].cbBuffer = 0;

		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = ssl->sdd.OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		dwSSPIFlags = ASC_REQ_SEQUENCE_DETECT	|
					  ASC_REQ_REPLAY_DETECT		|
					  ASC_REQ_CONFIDENTIALITY	|
					  ASC_REQ_EXTENDED_ERROR	|
					  ASC_REQ_ALLOCATE_MEMORY	|
					  ASC_REQ_STREAM;

		Status = g_pSSPI->AcceptSecurityContext(
						&g_hCreds,
						&ssl->hContext,
						NULL,
						dwSSPIFlags,
						SECURITY_NATIVE_DREP,
						NULL,
						&OutBuffer,
						&dwSSPIOutFlags,
						&tsExpiry );

		if ( FAILED( Status ) )
		{
			if ( ssl->sdd.OutBuffers[ 0 ].pvBuffer != NULL )
			{
				g_pSSPI->FreeContextBuffer( ssl->sdd.OutBuffers[ 0 ].pvBuffer );
				ssl->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
			}

			return Status;
		}

		// Send the close notify message to the server.
		if ( ssl->sdd.OutBuffers[ 0 ].pvBuffer != NULL && ssl->sdd.OutBuffers[ 0 ].cbBuffer != 0 )
		{
			sent = true;

			encrypted_buf.buf = ( char * )ssl->sdd.OutBuffers[ 0 ].pvBuffer;
			encrypted_buf.len = ssl->sdd.OutBuffers[ 0 ].cbBuffer;

			int nRet = _WSASend( ssl->s, &encrypted_buf, 1, NULL, dwFlags, lpWSAOverlapped, NULL );
			
			if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
			{
				sent = false;
				g_pSSPI->FreeContextBuffer( ssl->sdd.OutBuffers[ 0 ].pvBuffer );
				ssl->sdd.OutBuffers[ 0 ].pvBuffer = NULL;

				Status = SEC_E_INTERNAL_ERROR;
			}
			/*else	// Freed in SSL_free.
			{
				g_pSSPI->FreeContextBuffer( ssl->sdd.OutBuffers[ 0 ].pvBuffer );
				ssl->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
			}*/
		}

		// Freed in SSL_free.
		// Free the security context.
		//g_pSSPI->DeleteSecurityContext( &ssl->hContext );
		//SecInvalidateHandle( &ssl->hContext );

		return Status;
	}
	else
	{
		return SEC_E_INTERNAL_ERROR;
	}
}

SECURITY_STATUS SSL_WSARecv_Decode( SSL *ssl, LPWSABUF lpBuffers, DWORD &lpNumberOfBytesDecoded )
{
	lpNumberOfBytesDecoded = 0;
	SecBufferDesc Message;
	SecBuffer *pDataBuffer;
	SecBuffer *pExtraBuffer;

	if ( ssl == NULL )
	{
		return -1;
	}

	if ( ssl->rd.scRet == SEC_I_CONTINUE_NEEDED )
	{
		// Handle any remaining data in the recv buffer.
		if ( lpBuffers->buf != NULL && lpBuffers->len > 0 && ssl->cbRecDataBuf > 0 )
		{
			lpNumberOfBytesDecoded = min( ( DWORD )lpBuffers->len, ssl->cbRecDataBuf );
			_memcpy_s( lpBuffers->buf, lpBuffers->len, ssl->pbRecDataBuf, lpNumberOfBytesDecoded );

			DWORD rbytes = ssl->cbRecDataBuf - lpNumberOfBytesDecoded;
			_memmove( ssl->pbRecDataBuf, ( ( char * )ssl->pbRecDataBuf ) + lpNumberOfBytesDecoded, rbytes );
			ssl->cbRecDataBuf = rbytes;
		}

		ssl->cbIoBuffer = ssl->cbRecDataBuf;

		ssl->rd.scRet = SEC_E_OK;

		return ssl->rd.scRet;
	}

	_memzero( ssl->rd.Buffers, sizeof( SecBuffer ) * 4 );

	ssl->cbIoBuffer = lpBuffers->len;

	// Attempt to decrypt the received data.
	ssl->rd.Buffers[ 0 ].pvBuffer = ssl->pbIoBuffer;
	ssl->rd.Buffers[ 0 ].cbBuffer = ssl->cbIoBuffer;
	ssl->rd.Buffers[ 0 ].BufferType = SECBUFFER_DATA;

	ssl->rd.Buffers[ 1 ].BufferType = SECBUFFER_EMPTY;
	ssl->rd.Buffers[ 2 ].BufferType = SECBUFFER_EMPTY;
	ssl->rd.Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

	Message.ulVersion = SECBUFFER_VERSION;
	Message.cBuffers = 4;
	Message.pBuffers = ssl->rd.Buffers;

	if ( g_pSSPI->DecryptMessage != NULL )
	{
		ssl->rd.scRet = g_pSSPI->DecryptMessage( &ssl->hContext, &Message, 0, NULL );
	}
	else
	{
		ssl->rd.scRet = ( ( DECRYPT_MESSAGE_FN )g_pSSPI->Reserved4 )( &ssl->hContext, &Message, 0, NULL );
	}

	if ( ssl->rd.scRet == SEC_E_INCOMPLETE_MESSAGE )
	{
		// The input buffer contains only a fragment of an encrypted record. Need to read some more data.
		return ssl->rd.scRet;
	}

	// Sender has signaled end of session
	if ( ssl->rd.scRet == SEC_I_CONTEXT_EXPIRED )
	{
		return ssl->rd.scRet;
	}

	if ( ssl->rd.scRet != SEC_E_OK && ssl->rd.scRet != SEC_I_RENEGOTIATE )
	{
		return ssl->rd.scRet;
	}

	// Locate data and (optional) extra buffers.
	pDataBuffer  = NULL;
	pExtraBuffer = NULL;
	for ( int i = 1; i < 4; i++ )
	{
		if ( pDataBuffer == NULL && ssl->rd.Buffers[ i ].BufferType == SECBUFFER_DATA )
		{
			pDataBuffer = &ssl->rd.Buffers[ i ];
		}

		if ( pExtraBuffer == NULL && ssl->rd.Buffers[ i ].BufferType == SECBUFFER_EXTRA )
		{
			pExtraBuffer = &ssl->rd.Buffers[ i ];
		}
	}

	// Return decrypted data.
	if ( pDataBuffer != NULL )
	{
		lpNumberOfBytesDecoded = min( ( DWORD )lpBuffers->len, pDataBuffer->cbBuffer );
		_memcpy_s( lpBuffers->buf, lpBuffers->len, pDataBuffer->pvBuffer, lpNumberOfBytesDecoded );

		// Remaining bytes.
		DWORD rbytes = pDataBuffer->cbBuffer - lpNumberOfBytesDecoded;
		if ( rbytes > 0 )
		{
			if ( ssl->sbRecDataBuf < rbytes ) 
			{
				ssl->sbRecDataBuf = rbytes;

				if ( ssl->pbRecDataBuf == NULL )
				{
					ssl->pbRecDataBuf = ( PUCHAR )GlobalAlloc( GPTR, ssl->sbRecDataBuf );
				}
				else
				{
					ssl->pbRecDataBuf = ( PUCHAR )GlobalReAlloc( ssl->pbRecDataBuf, ssl->sbRecDataBuf, GMEM_MOVEABLE );
				}
			}
	
			_memcpy_s( ssl->pbRecDataBuf, ssl->sbRecDataBuf, ( char * )pDataBuffer->pvBuffer + lpNumberOfBytesDecoded, rbytes );
			ssl->cbRecDataBuf = rbytes;

			ssl->rd.scRet = SEC_I_CONTINUE_NEEDED;
		}
	}

	// Move any extra data to the input buffer.
	if ( pExtraBuffer != NULL )
	{
		_memmove( ssl->pbIoBuffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer );
		ssl->cbIoBuffer = pExtraBuffer->cbBuffer;
	}
	else
	{
		ssl->cbIoBuffer = 0;
	}

	/*if ( pDataBuffer != NULL && lpNumberOfBytesDecoded != 0 )
	{
		return ssl->rd.scRet;
	}

	if ( ssl->rd.scRet == SEC_I_RENEGOTIATE )
	{
		return ssl->rd.scRet;
	}*/

	return ssl->rd.scRet;
}

PCCERT_CONTEXT LoadPublicPrivateKeyPair( wchar_t *cer, wchar_t *key )
{
	bool failed = false;
	PCCERT_CONTEXT	pCertContext = NULL;
	HCRYPTPROV hProv = NULL;
	HCRYPTKEY hKey = NULL;


	HCERTSTORE hMyCertStore = NULL;
	HCRYPTMSG hCryptMsg = NULL;

	DWORD dwMsgAndCertEncodingType = 0;
	DWORD dwContentType = 0;
	DWORD dwFormatType = 0;

	LPWSTR pwszUuid = NULL;

	LPBYTE pbBuffer = NULL, pbKeyBlob = NULL;
	DWORD dwBufferLen = 0, cbKeyBlob = 0;
	BYTE *szPemPrivKey = NULL;

	#ifndef RPCRT4_USE_STATIC_LIB
		if ( rpcrt4_state == RPCRT4_STATE_SHUTDOWN )
		{
			if ( InitializeRpcRt4() == false ){ return NULL; }
		}
	#endif

	//hMyCertStore = _CertOpenStore( CERT_STORE_PROV_FILENAME, 0, NULL, CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG, cer );
	//pCertContext = _CertFindCertificateInStore( hMyCertStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, NULL );
	_CryptQueryObject( CERT_QUERY_OBJECT_FILE, cer, CERT_QUERY_CONTENT_FLAG_CERT, CERT_QUERY_FORMAT_FLAG_ALL, 0, &dwMsgAndCertEncodingType, &dwContentType, &dwFormatType, &hMyCertStore, &hCryptMsg, ( const void ** )&pCertContext );

	HANDLE hFile_cfg = CreateFile( key, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		dwBufferLen = GetFileSize( hFile_cfg, NULL );

		szPemPrivKey = ( BYTE * )GlobalAlloc( GMEM_FIXED, sizeof( BYTE ) * dwBufferLen );

		ReadFile( hFile_cfg, szPemPrivKey, sizeof( char ) * dwBufferLen, &dwBufferLen, NULL );

		CloseHandle( hFile_cfg );

		// Let's assume the key is also in the same format.
		if ( dwFormatType == CERT_QUERY_FORMAT_BASE64_ENCODED )	// PEM format.
		{
			if ( _CryptStringToBinaryA( ( LPCSTR )szPemPrivKey, 0, CRYPT_STRING_BASE64HEADER, NULL, &dwBufferLen, NULL, NULL ) == FALSE )
			{
				failed = true;
				_MessageBoxW( NULL, ST_Binary_conversion_of_private_key_failed, L"Web Server", 0 );
				goto CLEANUP;
			}

			pbBuffer = ( LPBYTE )GlobalAlloc( GMEM_FIXED, dwBufferLen );
			if ( _CryptStringToBinaryA( ( LPCSTR )szPemPrivKey, 0, CRYPT_STRING_BASE64HEADER, pbBuffer, &dwBufferLen, NULL, NULL ) == FALSE )
			{
				failed = true;
				_MessageBoxW( NULL, ST_Binary_conversion_of_private_key_failed, L"Web Server", 0 );
				goto CLEANUP;
			}
		}
		else	// DER format.
		{
			pbBuffer = szPemPrivKey;
			szPemPrivKey = NULL;
		}

		if ( _CryptDecodeObjectEx( dwMsgAndCertEncodingType, PKCS_RSA_PRIVATE_KEY, pbBuffer, dwBufferLen, 0, NULL, NULL, &cbKeyBlob ) == FALSE )
		{
			failed = true;
			_MessageBoxW( NULL, ST_Decoding_of_private_key_failed, L"Web Server", 0 );
			goto CLEANUP;
		}

		pbKeyBlob = ( LPBYTE )GlobalAlloc( GMEM_FIXED, cbKeyBlob );
		if ( _CryptDecodeObjectEx( dwMsgAndCertEncodingType, PKCS_RSA_PRIVATE_KEY, pbBuffer, dwBufferLen, 0, NULL, pbKeyBlob, &cbKeyBlob ) == FALSE )
		{
			failed = true;
			_MessageBoxW( NULL, ST_Decoding_of_private_key_failed, L"Web Server", 0 );
			goto CLEANUP;
		}

		UUID uuid;
		_UuidCreate( &uuid );
		_UuidToStringW( &uuid, ( RPC_WSTR * )&pwszUuid );

		if ( _CryptAcquireContextW( &hProv, pwszUuid, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET ) == FALSE )
		{
			failed = true;
			_MessageBoxW( NULL, ST_Acquiring_cryptographic_service_provider_failed, L"Web Server", 0 );
			goto CLEANUP;
		}

		if ( _CryptImportKey( hProv, pbKeyBlob, cbKeyBlob, NULL, 0, &hKey ) == FALSE )
		{
			failed = true;
			_MessageBoxW( NULL, ST_Import_of_private_key_failed, L"Web Server", 0 );
			goto CLEANUP;
		}

		CRYPT_KEY_PROV_INFO privateKeyData;
		_memzero( &privateKeyData, sizeof( CRYPT_KEY_PROV_INFO ) );
		privateKeyData.pwszContainerName = pwszUuid;
		privateKeyData.pwszProvName = MS_ENHANCED_PROV;
		privateKeyData.dwProvType = PROV_RSA_FULL;
		privateKeyData.dwFlags = 0;
		privateKeyData.dwKeySpec = AT_KEYEXCHANGE;

		if ( _CertSetCertificateContextProperty( pCertContext, CERT_KEY_PROV_INFO_PROP_ID, 0, &privateKeyData ) == FALSE )
		{
			failed = true;
			_MessageBoxW( NULL, ST_Setting_certificate_context_property_failed, L"Web Server", 0 );
			goto CLEANUP;
		}
	}

CLEANUP:

	if ( pwszUuid != NULL )
	{
		_RpcStringFreeW( ( RPC_WSTR * )&pwszUuid );
	}

	if ( pbBuffer != NULL )
	{
		GlobalFree( pbBuffer );
	}

	if ( szPemPrivKey != NULL )
	{
		GlobalFree( szPemPrivKey );
	}

	if ( pbKeyBlob != NULL )
	{
		GlobalFree( pbKeyBlob );
	}

	if ( hCryptMsg != NULL )
	{
		_CryptMsgClose( hCryptMsg );
	}

	if ( hKey != NULL )
	{
		_CryptDestroyKey( hKey );
	}
	if ( hProv != NULL )
	{
		_CryptReleaseContext( hProv, 0 );
	}

	if ( hMyCertStore != NULL )
	{
		_CertCloseStore( hMyCertStore, 0 );
	}

	if ( failed == true && pCertContext != NULL )
	{
		_CertFreeCertificateContext( pCertContext );
		pCertContext = NULL;
	}

	return pCertContext;
}

PCCERT_CONTEXT LoadPKCS12( wchar_t *p12_file, wchar_t *password )
{
	PCCERT_CONTEXT	pCertContext = NULL;

	HANDLE hFile_cfg = CreateFile( p12_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		CRYPT_DATA_BLOB cdb;
		DWORD read = 0, pos = 0;
		DWORD fz = GetFileSize( hFile_cfg, NULL );

		cdb.cbData = fz;
		cdb.pbData = ( BYTE * )GlobalAlloc( GMEM_FIXED, sizeof( BYTE ) * fz );

		ReadFile( hFile_cfg, cdb.pbData, sizeof( char ) * fz, &read, NULL );

		CloseHandle( hFile_cfg );

		HCERTSTORE hMyCertStore = _PFXImportCertStore( &cdb, password, 0 );

		GlobalFree( cdb.pbData );

		int err = GetLastError();
		if ( hMyCertStore == NULL || err != 0 )
		{
			if ( err == ERROR_INVALID_PASSWORD )
			{
				_MessageBoxW( NULL, ST_Invalid_password_supplied_for_the_certificate_store, L"Web Server", 0 );
			}
			else
			{
				_MessageBoxW( NULL, ST_Failed_to_import_the_certificate_store, L"Web Server", 0 );
			}

			_CertCloseStore( hMyCertStore, 0 );
			return NULL;
		}

		pCertContext = _CertFindCertificateInStore( hMyCertStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, NULL );

		_CertCloseStore( hMyCertStore, 0 );

		if ( pCertContext == NULL )
		{
			_MessageBoxW( NULL, ST_Failed_to_find_certificate_in_the_certificate_store, L"Web Server", 0 );
		}
	}

	return pCertContext;
}
