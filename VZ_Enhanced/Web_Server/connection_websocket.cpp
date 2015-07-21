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
#include "connection_websocket.h"
#include "utilities.h"

#include "lite_shell32.h"

DoublyLinkedList *g_update_buffer_pool = NULL;

CRITICAL_SECTION g_update_buffer_pool_cs;

// payload will point to an offset buf.
char *DeconstructFrame( char *buf, DWORD *buf_length, WS_OPCODE &opcode, char **payload, unsigned __int64 &payload_length )
{
	if ( buf == NULL || *buf_length < 2 )
	{
		return NULL;
	}

	int buffer_offset = 0;
	/*unsigned short websocket_header = 0;
	_memcpy_s( &websocket_header, sizeof( unsigned short ), lpIOContext->wsabuf.buf, sizeof( unsigned short ) );
	buffer_offset += sizeof( unsigned short );

	websocket_header = _ntohs( websocket_header );
	
	unsigned short ws_fin =			websocket_header >> 15 & 0x01;
	unsigned short ws_rsv1 =		websocket_header >> 14 & 0x01;
	unsigned short ws_rsv2 =		websocket_header >> 13 & 0x01;
	unsigned short ws_rsv3 =		websocket_header >> 12 & 0x01;
	unsigned short ws_opcode =		websocket_header >> 8 & 0x0F;
	unsigned short ws_mask =		websocket_header >> 7 & 0x01;
	unsigned short ws_payload_len =	websocket_header >> 0 & 0x7F;
	*/

	unsigned char ws_fin =			buf[ 0 ] >> 7 & 0x01;
	unsigned char ws_rsv1 =			buf[ 0 ] >> 6 & 0x01;
	unsigned char ws_rsv2 =			buf[ 0 ] >> 5 & 0x01;
	unsigned char ws_rsv3 =			buf[ 0 ] >> 4 & 0x01;
	unsigned char ws_opcode =		buf[ 0 ] & 0x0F;

	unsigned char ws_mask =			buf[ 1 ] >> 7 & 0x01;
	unsigned char ws_payload_len =	buf[ 1 ] & 0x7F;

	buffer_offset += ( sizeof( unsigned char ) * 2 );

	opcode = ( WS_OPCODE )ws_opcode;

	if ( ws_payload_len == 126 )
	{
		unsigned short extended_payload1 = 0;
		_memcpy_s( &extended_payload1, sizeof( unsigned short ), buf + buffer_offset, sizeof( unsigned short ) );
		buffer_offset += sizeof( unsigned short );

		payload_length = ( unsigned __int64 )_ntohs( extended_payload1 );
	}
	else if ( ws_payload_len >= 127 )
	{
		_memcpy_s( &payload_length, sizeof( unsigned __int64 ), buf + buffer_offset, sizeof( unsigned __int64 ) );
		buffer_offset += sizeof( unsigned __int64 );

		payload_length = ntohll( payload_length );
	}
	else
	{
		payload_length = ( unsigned __int64 )ws_payload_len;
	}

	unsigned char masking_key[ 4 ];
	_memzero( &masking_key, sizeof( masking_key ) );
	if ( ws_mask == 1 )
	{
		_memcpy_s( masking_key, sizeof( masking_key ), buf + buffer_offset, sizeof( masking_key ) );
		buffer_offset += sizeof( masking_key );
	}

	*payload = buf + buffer_offset;

	for ( unsigned int i = 0; i < payload_length; ++i )
	//for ( unsigned __int64 i = 0; i < payload_length; ++i )
	{
		if ( i + buffer_offset >= ( unsigned int )*buf_length )
		{
			break;
		}

		buf[ buffer_offset + i ] ^= masking_key[ i % 4 ];
	}

	if ( buffer_offset + payload_length < *buf_length )
	{
		*buf_length -= ( int )( buffer_offset + payload_length );
		return buf + ( buffer_offset + payload_length );
	}
	else
	{
		*buf_length = 0;
		return NULL;
	}
}

int ConstructFrameHeader( char *buf, int buf_length, WS_OPCODE opcode, /*char *payload,*/ unsigned __int64 payload_length )
{
	if ( buf == NULL || buf_length < 2 )
	{
		return 0;
	}

	unsigned char fin =		0x01;	// Final frame. We're not going to break up our frames.
	unsigned char rsv1 =	0x00;	// Reserved
	unsigned char rsv2 =	0x00;	// Reserved
	unsigned char rsv3 =	0x00;	// Reserved
	unsigned char mask =	0x00;	// Server doesn't mask payload. Client has this bit set.
	unsigned char len =		0x00;

	// Adjust the length accordingly.
	unsigned char len_type = 0;
	if ( payload_length > 65535 )
	{
		len = 127;
		len_type = 2;
	}
	else if ( payload_length > 125 )
	{
		len = 126;
		len_type = 1;
	}
	else
	{
		len = ( unsigned char )payload_length;
		len_type = 0;
	}

	unsigned short frame_header = 0;

	// Big endian (network byte order)
	// Byte 2.
	frame_header =	 frame_header		 | mask;
	frame_header = ( frame_header << 7 ) | len;
	// Byte 1.
	frame_header = ( frame_header << 1 ) | fin;
	frame_header = ( frame_header << 1 ) | rsv1;
	frame_header = ( frame_header << 1 ) | rsv2;
	frame_header = ( frame_header << 1 ) | rsv3;
	frame_header = ( frame_header << 4 ) | opcode;

	int buf_offset = 0;
	_memcpy_s( buf, buf_length, &frame_header, sizeof( unsigned short ) );
	buf_offset += sizeof( unsigned short );

	if ( len_type == 1 )	// Extended payload 16 bits.
	{
		unsigned short extended_payload_length = _ntohs( ( unsigned short )payload_length );
		_memcpy_s( buf + buf_offset, buf_length - buf_offset, &extended_payload_length, sizeof( unsigned short ) );
		buf_offset += sizeof( unsigned short );
	}
	else if ( len_type == 2 )	// Extended payload 64 bits.
	{
		unsigned __int64 extended_payload_length = ntohll( payload_length );
		_memcpy_s( buf + buf_offset, buf_length - buf_offset, &extended_payload_length, sizeof( unsigned __int64 ) );
		buf_offset += sizeof( unsigned __int64 );
	}

	/*if ( payload_length > 0 && payload_length <= ( buf_length - buf_offset )  )
	{
		_memcpy_s( buf + buf_offset, buf_length - buf_offset, payload, ( rsize_t )payload_length );
		buf_offset += ( int )payload_length;
	}*/

	return buf_offset;
}

void ReadWebSocketRequest( SOCKET_CONTEXT *socket_context, DWORD request_size )
{
	socket_context->connection_info.rx_bytes += request_size;

	socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer + socket_context->io_context.nBufferOffset;
	socket_context->io_context.wsabuf.len = MAX_BUFFER_SIZE - socket_context->io_context.nBufferOffset;

	if ( use_ssl == true && DecodeRecv( socket_context, request_size ) == false )
	{
		socket_context->io_context.nBufferOffset = 0;
		return;
	}

	socket_context->io_context.nBufferOffset += request_size;

	// Make sure we have enough to decode a websocket frame. (At least 2 bytes).
	if ( socket_context->io_context.nBufferOffset >= 2 )
	{
		// Set to the size of the payload.
		socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer;
		socket_context->io_context.wsabuf.len = ( socket_context->io_context.nBufferOffset < MAX_BUFFER_SIZE ) ? socket_context->io_context.nBufferOffset : ( MAX_BUFFER_SIZE - 1 );

		socket_context->io_context.wsabuf.buf[ socket_context->io_context.wsabuf.len ] = 0;	// Sanity.

		socket_context->io_context.nBufferOffset = 0;

		ParseWebsocketInfo( socket_context );
	}
	else	// We need to read more data. The websocket frame is incomplete. Occurs on TSL 1.0 and below for browsers that handle the BEAST attack.
	{
		bool ret = TryReceive( socket_context, &( socket_context->io_context.overlapped ), ClientIoReadWebSocketRequest );
		if ( ret == false )
		{
			socket_context->io_context.nBufferOffset = 0;

			BeginClose( socket_context );
		}
	}
}

void ParseWebsocketInfo( SOCKET_CONTEXT *socket_context )
{
	char *payload_buf = socket_context->io_context.wsabuf.buf;
	DWORD decoded_size = socket_context->io_context.wsabuf.len;

	char *payload = NULL;
	unsigned __int64 payload_length = 0;
	WS_OPCODE opcode = WS_CONTINUATION_FRAME;	// Placeholder.

	DoublyLinkedList *q_dll = NULL;

	// It's possible for these requests to get queued in the buffer.
	// We should process them all, and when done, post a single read operation (if required).
	do
	{
		// payload is not allocated. So don't free it!
		payload_buf = DeconstructFrame( payload_buf, ( DWORD * )&decoded_size, opcode, &payload, payload_length );

		PAYLOAD_INFO *pi = ( PAYLOAD_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( PAYLOAD_INFO ) );

		pi->payload_value = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( ( SIZE_T )payload_length + 1 ) );
		_memcpy_s( pi->payload_value, ( size_t )payload_length, payload, ( size_t )payload_length );
		pi->payload_value[ payload_length ] = 0;	// Sanity.

		pi->opcode = opcode;

		DoublyLinkedList *node = DLL_CreateNode( pi );
		DLL_AddNode( &q_dll, node, -1 );
	}
	while ( payload_buf != NULL );

	EnterCriticalSection( &context_list_cs );

	// Add the requests to the end of our list.
	DLL_AddNode( &( socket_context->list_data ), q_dll, -1 );
	
	LeaveCriticalSection( &context_list_cs );

	SendListData( socket_context );
}

void SendListData( SOCKET_CONTEXT *socket_context )
{
	EnterCriticalSection( &context_list_cs );

	bool process_payload = false;

	do
	{
		process_payload = false;

		DoublyLinkedList *dll = socket_context->list_data;

		OVERLAP_TYPE ot = OVERLAP_PING;

		bool ret = true;	// Used for send/recv status.

		bool do_read = false;
		bool do_write = true;

		if ( dll != NULL )
		{
			PAYLOAD_INFO *pi = ( PAYLOAD_INFO * )dll->data;

			if ( pi->opcode == WS_PONG )	// Something to note: Internet Explorer sends an unsolicited PONG frame every 30 seconds. It need not be replied to.
			{
				InterlockedExchange( &socket_context->ping_sent, 0 );	// Reset the ping counter.

				do_read = true;
				do_write = false;
			}
			else if ( pi->opcode == WS_TEXT_FRAME )
			{
				char *payload_value = pi->payload_value;
				int payload_value_length = lstrlenA( payload_value );

				if ( ignore_list != NULL && payload_value_length >= 15 && _memcmp( payload_value, "GET_IGNORE_LIST", 15 ) == 0 )
				{
					ot = OVERLAP_IGNORE_LIST;
				}
				else if ( forward_list != NULL && payload_value_length >= 16 && _memcmp( payload_value, "GET_FORWARD_LIST", 16 ) == 0 )
				{
					ot = OVERLAP_FORWARD_LIST;
				}
				else if ( contact_list != NULL && payload_value_length >= 16 && _memcmp( payload_value, "GET_CONTACT_LIST", 16 ) == 0 )
				{
					ot = OVERLAP_CONTACT_LIST;
				}
				else if ( call_log != NULL && payload_value_length >= 12 && _memcmp( payload_value, "GET_CALL_LOG", 12 ) == 0 )
				{
					ot = OVERLAP_CALL_LOG;
				}
				else if ( forward_cid_list != NULL && payload_value_length >= 20 && _memcmp( payload_value, "GET_FORWARD_CID_LIST", 20 ) == 0 )
				{
					ot = OVERLAP_FORWARD_CID_LIST;
				}
				else if ( ignore_cid_list != NULL && payload_value_length >= 19 && _memcmp( payload_value, "GET_IGNORE_CID_LIST", 19 ) == 0 )
				{
					ot = OVERLAP_IGNORE_CID_LIST;
				}
				else if ( payload_value_length >= 16 && _memcmp( payload_value, "IGNORE_INCOMING:", 16 ) == 0 )
				{
					do_read = true;
					do_write = false;

					char *from = _StrChrA( payload_value + 16, ':' );
					if ( from != NULL )
					{
						char *reference = from + 1;
						int reference_length = ( payload_value + payload_value_length ) - reference;

						*from = 0;
						from = payload_value + 16;

						EnterCriticalSection( list_cs );

						DoublyLinkedList *dll_w = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )from, true );
						if ( dll_w != NULL )
						{
							if ( dll_w->prev != NULL && dll_w->prev->data != NULL )
							{
								displayinfo *di = ( displayinfo * )dll_w->prev->data;

								// Verify that the reference value is the same.
								if ( _memcmp( di->ci.call_reference_id, reference, reference_length ) == 0 )
								{
									( ( pWebIgnoreIncomingCall )WebIgnoreIncomingCall )( di );
								}
							}
							else if ( dll_w->data != NULL )	// The incoming call is the only item in the list.
							{
								displayinfo *di = ( displayinfo * )dll_w->data;

								// Verify that the reference value is the same.
								if ( _memcmp( di->ci.call_reference_id, reference, reference_length ) == 0 )
								{
									( ( pWebIgnoreIncomingCall )WebIgnoreIncomingCall )( di );
								}
							}
						}

						LeaveCriticalSection( list_cs );
					}
				}
				else if ( payload_value_length >= 17 && _memcmp( payload_value, "FORWARD_INCOMING:", 17 ) == 0 )
				{
					do_read = true;
					do_write = false;

					char *from = _StrChrA( payload_value + 17, ':' );
					if ( from != NULL )
					{
						char *to = from + 1;

						char *reference = _StrChrA( to, ':' );
						if ( reference != NULL )
						{
							int to_length = ( reference - to );

							*reference = 0;
							reference++;

							int reference_length = ( payload_value + payload_value_length ) - reference;

							*from = 0;
							from = payload_value + 17;

							EnterCriticalSection( list_cs );

							DoublyLinkedList *dll_w = ( DoublyLinkedList * )dllrbt_find( call_log, ( void * )from, true );
							if ( dll_w != NULL )
							{
								if ( dll_w->prev != NULL && dll_w->prev->data != NULL )
								{
									displayinfo *di = ( displayinfo * )dll_w->prev->data;

									// Verify that the reference value is the same.
									if ( _memcmp( di->ci.call_reference_id, reference, reference_length ) == 0 )
									{
										char *forward_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( to_length + 1 ) );
										_memcpy_s( forward_to, to_length + 1, to, to_length + 1 );
										forward_to[ to_length ] = 0;	// Sanity.

										if ( ( ( pWebForwardIncomingCall )WebForwardIncomingCall )( di, forward_to ) == false )
										{
											GlobalFree( forward_to );
										}
									}
								}
								else if ( dll_w->data != NULL )	// The incoming call is the only item in the list.
								{
									displayinfo *di = ( displayinfo * )dll_w->data;

									// Verify that the reference value is the same.
									if ( _memcmp( di->ci.call_reference_id, reference, reference_length ) == 0 )
									{
										char *forward_to = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( to_length + 1 ) );
										_memcpy_s( forward_to, to_length + 1, to, to_length + 1 );
										forward_to[ to_length ] = 0;	// Sanity.

										if ( ( ( pWebForwardIncomingCall )WebForwardIncomingCall )( di, forward_to ) == false )
										{
											GlobalFree( forward_to );
										}
									}
								}
							}

							LeaveCriticalSection( list_cs );
						}
					}
				}
				else	// Post another read for any unknown text code.
				{
					do_read = true;
					do_write = false;
				}
			}
			else// if ( pi->opcode == WS_CONNECTION_CLOSE )	// We'll just close everything that isn't a PONG or TEXT_FRAME.
			{
				do_read = false;
				do_write = false;

				socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer;

				socket_context->io_context.wsabuf.buf[ 0 ] = -120;	//0x88;	Close byte
				socket_context->io_context.wsabuf.buf[ 1 ] = 0;

				socket_context->io_context.wsabuf.len = 2;

				socket_context->connection_type = CON_TYPE_HTTP;

				ret = TrySend( socket_context, &( socket_context->io_context.overlapped ), ClientIoShutdown );
			}
			/*else	// Post another read for any unknown opcode.
			{
				do_read = true;
				do_write = false;
			}*/

			bool remove_node = true;
			if ( do_write == true )
			{
				socket_context->list_type = ot;

				remove_node = WriteListData( socket_context, ( dll->next == NULL ? true : false ) );
			}
			else if ( do_read == true )
			{
				// If there's no more nodes after this, then post a read, otherwise continue to write from the nodes.
				if ( dll->next == NULL )
				{
					ret = TryReceive( socket_context, &( socket_context->io_context.overlapped ), ClientIoReadWebSocketRequest );
				}
				else
				{
					process_payload = true;	// Continue to process list data.
				}
			}

			if ( remove_node == true )
			{
				DoublyLinkedList *del_node = dll;
				dll = dll->next;

				GlobalFree( ( ( PAYLOAD_INFO * )del_node->data )->payload_value );
				GlobalFree( ( PAYLOAD_INFO * )del_node->data );
				GlobalFree( del_node );

				socket_context->list_data = dll;
			}

			// If we need to close the connection, do so after we remove the node above.
			if ( ret == false )
			{
				BeginClose( socket_context );
			}
		}
	}
	while ( process_payload == true );

	LeaveCriticalSection( &context_list_cs );
}

bool WriteListData( SOCKET_CONTEXT *socket_context, bool do_read )
{
	node_type *node = NULL;

	EnterCriticalSection( &context_list_cs );

	socket_context->io_context.wsabuf.buf = socket_context->io_context.buffer;
	socket_context->io_context.wsabuf.len = MAX_BUFFER_SIZE;

	int reply_buf_length = 0;

	EnterCriticalSection( list_cs );

	switch ( socket_context->list_type )
	{
		case OVERLAP_CALL_LOG:
		{
			reply_buf_length = WriteCallLog( socket_context );

			node = socket_context->call_node;
		}
		break;

		case OVERLAP_CONTACT_LIST:
		{
			reply_buf_length = WriteContactList( socket_context );

			node = socket_context->contact_node;
		}
		break;

		case OVERLAP_FORWARD_LIST:
		{
			reply_buf_length = WriteForwardList( socket_context );

			node = socket_context->forward_node;
		}
		break;

		case OVERLAP_IGNORE_LIST:
		{
			reply_buf_length = WriteIgnoreList( socket_context );

			node = socket_context->ignore_node;
		}
		break;

		case OVERLAP_FORWARD_CID_LIST:
		{
			reply_buf_length = WriteForwardCIDList( socket_context );

			node = socket_context->forward_cid_node;
		}
		break;

		case OVERLAP_IGNORE_CID_LIST:
		{
			reply_buf_length = WriteIgnoreCIDList( socket_context );

			node = socket_context->ignore_cid_node;
		}
		break;
	}

	LeaveCriticalSection( list_cs );

	socket_context->io_context.IOOperation = ClientIoWriteWebSocketLists;

	// Determine whether we need to read more data, or not.
	if ( node == NULL )
	{
		if ( do_read == true )
		{
			socket_context->io_context.IOOperation = ClientIoReadMoreWebSocketRequest;
		}
	}

	socket_context->io_context.wsabuf.len = reply_buf_length;

	bool ret = TrySend( socket_context, &( socket_context->io_context.overlapped ), socket_context->io_context.IOOperation );
	if ( ret == false )
	{
		BeginClose( socket_context );
	}

	LeaveCriticalSection( &context_list_cs );

	return ( node != NULL ? false : true );
}

int WriteIgnoreList( SOCKET_CONTEXT *socket_context )
{
	char *json_buf = socket_context->io_context.wsabuf.buf + 10;	// 10 byte offset so that we can encode the websocket frame header.
	int json_buf_length = 0;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"IGNORE_LIST\":[", 16 );
	json_buf_length += 16;

	node_type *node = NULL;

	if ( socket_context->ignore_node == NULL )
	{
		node = dllrbt_get_head( ignore_list );
	}
	else
	{
		node = socket_context->ignore_node;
	}

	while ( node != NULL )
	{
		ignoreinfo *ii = ( ignoreinfo * )node->val;

		int cfg_val_length = lstrlenA( SAFESTRA( ii->c_phone_number ) );

		if ( json_buf_length + cfg_val_length + 13 + 10 + 2 + 1 >= ( MAX_BUFFER_SIZE - 10 ) )		// 13 for the json, 10 for the int count, 2 for "]}".
		{
			--json_buf_length;

			break;
		}

		json_buf_length += __snprintf( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"N\":\"%s\",\"C\":%lu}", SAFESTRA( ii->c_phone_number ), ii->count );
		node = node->next;

		if ( node != NULL )
		{
			json_buf[ json_buf_length++ ] = ',';
		}
	}

	socket_context->ignore_node = node;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "]}", 2 );
	json_buf_length += 2;

	// Websocket payload size. Adjust the start of our buffer if the payload is less than the given amounts.
	if ( json_buf_length <= 125 )
	{
		socket_context->io_context.wsabuf.buf += 8;
	}
	else if ( json_buf_length <= 65535 )
	{
		socket_context->io_context.wsabuf.buf += 6;
	}

	return ConstructFrameHeader( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE, WS_BINARY_FRAME, /*json_buf,*/ json_buf_length ) + json_buf_length;
}

int WriteIgnoreCIDList( SOCKET_CONTEXT *socket_context )
{
	char *json_buf = socket_context->io_context.wsabuf.buf + 10;	// 10 byte offset so that we can encode the websocket frame header.
	int json_buf_length = 0;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"IGNORE_CID_LIST\":[", 20 );
	json_buf_length += 20;

	node_type *node = NULL;

	if ( socket_context->ignore_cid_node == NULL )
	{
		node = dllrbt_get_head( ignore_cid_list );
	}
	else
	{
		node = socket_context->ignore_cid_node;
	}

	while ( node != NULL )
	{
		ignorecidinfo *icidi = ( ignorecidinfo * )node->val;

		int cfg_val_length = lstrlenA( SAFESTRA( icidi->c_caller_id ) );

		if ( json_buf_length + cfg_val_length + 23 + 10 + 2 + 2 + 1 >= ( MAX_BUFFER_SIZE - 10 ) )		// 23 for the json, 10 for the int count, 2 for chars, 2 for "]}".
		{
			--json_buf_length;

			break;
		}

		json_buf_length += __snprintf( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"I\":\"%s\",\"C\":%c,\"W\":%c,\"T\":%lu}", SAFESTRA( icidi->c_caller_id ), ( icidi->match_case == true ? '1' : '0' ), ( icidi->match_whole_word == true ? '1' : '0' ), icidi->count );
		node = node->next;

		if ( node != NULL )
		{
			json_buf[ json_buf_length++ ] = ',';
		}
	}

	socket_context->ignore_cid_node = node;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "]}", 2 );
	json_buf_length += 2;

	// Websocket payload size. Adjust the start of our buffer if the payload is less than the given amounts.
	if ( json_buf_length <= 125 )
	{
		socket_context->io_context.wsabuf.buf += 8;
	}
	else if ( json_buf_length <= 65535 )
	{
		socket_context->io_context.wsabuf.buf += 6;
	}

	return ConstructFrameHeader( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE, WS_BINARY_FRAME, /*json_buf,*/ json_buf_length ) + json_buf_length;
}

int WriteForwardList( SOCKET_CONTEXT *socket_context )
{
	char *json_buf = socket_context->io_context.wsabuf.buf + 10;	// 10 byte offset so that we can encode the websocket frame header.
	int json_buf_length = 0;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"FORWARD_LIST\":[", 17 );
	json_buf_length += 17;

	node_type *node = NULL;

	if ( socket_context->forward_node == NULL )
	{
		node = dllrbt_get_head( forward_list );
	}
	else
	{
		node = socket_context->forward_node;
	}

	while ( node != NULL )
	{
		forwardinfo *fi = ( forwardinfo * )node->val;

		int cfg_val_length = lstrlenA( SAFESTRA( fi->c_call_from ) );
		cfg_val_length += lstrlenA( SAFESTRA( fi->c_forward_to ) );

		if ( json_buf_length + cfg_val_length + 20 + 10 + 2 + 1 >= ( MAX_BUFFER_SIZE - 10 ) )	// 20 for the json, 10 for the int count, 2 for "]}".
		{
			--json_buf_length;

			break;
		}

		json_buf_length += __snprintf( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"F\":\"%s\",\"T\":\"%s\",\"C\":%lu}", SAFESTRA( fi->c_call_from ), SAFESTRA( fi->c_forward_to ), fi->count );
		node = node->next;

		if ( node != NULL )
		{
			json_buf[ json_buf_length++ ] = ',';
		}
	}

	socket_context->forward_node = node;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "]}", 2 );
	json_buf_length += 2;

	// Websocket payload size. Adjust the start of our buffer if the payload is less than the given amounts.
	if ( json_buf_length <= 125 )
	{
		socket_context->io_context.wsabuf.buf += 8;
	}
	else if ( json_buf_length <= 65535 )
	{
		socket_context->io_context.wsabuf.buf += 6;
	}

	return ConstructFrameHeader( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE, WS_BINARY_FRAME, /*json_buf,*/ json_buf_length ) + json_buf_length;
}

int WriteForwardCIDList( SOCKET_CONTEXT *socket_context )
{
	char *json_buf = socket_context->io_context.wsabuf.buf + 10;	// 10 byte offset so that we can encode the websocket frame header.
	int json_buf_length = 0;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"FORWARD_CID_LIST\":[", 21 );
	json_buf_length += 21;

	node_type *node = NULL;

	if ( socket_context->forward_cid_node == NULL )
	{
		node = dllrbt_get_head( forward_cid_list );
	}
	else
	{
		node = socket_context->forward_cid_node;
	}

	while ( node != NULL )
	{
		forwardcidinfo *fcidi = ( forwardcidinfo * )node->val;

		int cfg_val_length = lstrlenA( SAFESTRA( fcidi->c_caller_id ) );
		cfg_val_length += lstrlenA( SAFESTRA( fcidi->c_forward_to ) );

		if ( json_buf_length + cfg_val_length + 30 + 10 + 2 + 2 + 1 >= ( MAX_BUFFER_SIZE - 10 ) )	// 30 for the json, 10 for the int count, 2 for chars, 2 for "]}".
		{
			--json_buf_length;

			break;
		}

		json_buf_length += __snprintf( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"I\":\"%s\",\"F\":\"%s\",\"C\":%c,\"W\":%c,\"T\":%lu}", SAFESTRA( fcidi->c_caller_id ), SAFESTRA( fcidi->c_forward_to ), ( fcidi->match_case == true ? '1' : '0' ), ( fcidi->match_whole_word == true ? '1' : '0' ), fcidi->count );
		node = node->next;

		if ( node != NULL )
		{
			json_buf[ json_buf_length++ ] = ',';
		}
	}

	socket_context->forward_cid_node = node;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "]}", 2 );
	json_buf_length += 2;

	// Websocket payload size. Adjust the start of our buffer if the payload is less than the given amounts.
	if ( json_buf_length <= 125 )
	{
		socket_context->io_context.wsabuf.buf += 8;
	}
	else if ( json_buf_length <= 65535 )
	{
		socket_context->io_context.wsabuf.buf += 6;
	}

	return ConstructFrameHeader( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE, WS_BINARY_FRAME, /*json_buf,*/ json_buf_length ) + json_buf_length;
}

int WriteContactList( SOCKET_CONTEXT *socket_context )
{
	char *json_buf = socket_context->io_context.wsabuf.buf + 10;	// 10 byte offset so that we can encode the websocket frame header.
	int json_buf_length = 0;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"CONTACT_LIST\":[", 17 );
	json_buf_length += 17;

	node_type *node = NULL;

	if ( socket_context->contact_node == NULL )
	{
		node = dllrbt_get_head( contact_list );
	}
	else
	{
		node = socket_context->contact_node;
	}

	while ( node != NULL )
	{
		contactinfo *ci = ( contactinfo * )node->val;

		int cfg_val_length = lstrlenA( SAFESTRA( ci->contact.home_phone_number ) );
		cfg_val_length += lstrlenA( SAFESTRA( ci->contact.cell_phone_number ) );

		cfg_val_length += lstrlenA( SAFESTRA( ci->contact.first_name ) );
		cfg_val_length += lstrlenA( SAFESTRA( ci->contact.last_name ) );

		if ( json_buf_length + cfg_val_length + 29 + 2 + 1 >= ( MAX_BUFFER_SIZE - 10 ) )	// 29 for the json, 2 for "]}".
		{
			--json_buf_length;

			break;
		}

		json_buf_length += __snprintf( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"F\":\"%s\",\"L\":\"%s\",\"H\":\"%s\",\"C\":\"%s\"}", SAFESTRA( ci->contact.first_name ), SAFESTRA( ci->contact.last_name ), SAFESTRA( ci->contact.home_phone_number ), SAFESTRA( ci->contact.cell_phone_number ) );
		node = node->next;

		if ( node != NULL )
		{
			json_buf[ json_buf_length++ ] = ',';
		}
	}

	socket_context->contact_node = node;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "]}", 2 );
	json_buf_length += 2;

	// Websocket payload size. Adjust the start of our buffer if the payload is less than the given amounts.
	if ( json_buf_length <= 125 )
	{
		socket_context->io_context.wsabuf.buf += 8;
	}
	else if ( json_buf_length <= 65535 )
	{
		socket_context->io_context.wsabuf.buf += 6;
	}

	return ConstructFrameHeader( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE, WS_BINARY_FRAME, /*json_buf,*/ json_buf_length ) + json_buf_length;
}

int WriteCallLog( SOCKET_CONTEXT *socket_context )
{
	char *json_buf = socket_context->io_context.wsabuf.buf + 10;	// 10 byte offset so that we can encode the websocket frame header.
	int json_buf_length = 0;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"CALL_LOG\":[", 13 );
	json_buf_length += 13;

	node_type *node = NULL;

	if ( socket_context->call_node == NULL )
	{
		node = dllrbt_get_head( call_log );
	}
	else
	{
		node = socket_context->call_node;	// Start from the last node if we broke the packet up.
	}

	DoublyLinkedList *dll = NULL;

	bool exit_loop = false;
	while ( node != NULL )
	{
		if ( socket_context->call_list != NULL )
		{
			dll = socket_context->call_list;
			socket_context->call_list = NULL;	// Reset the last list node.
		}
		else
		{
			dll = ( DoublyLinkedList * )node->val;
		}

		while ( dll != NULL )
		{
			displayinfo *di = ( displayinfo * )dll->data;

			int cfg_val_length = lstrlenA( SAFESTRA( di->ci.caller_id ) );

			unsigned int ec_length = 0;
			char *escaped_caller_id = json_escape( SAFESTRA( di->ci.caller_id ), cfg_val_length, &ec_length );

			if ( escaped_caller_id != NULL )
			{
				cfg_val_length = ec_length;
			}

			cfg_val_length += lstrlenA( SAFESTRA( di->ci.call_from ) );


			ULARGE_INTEGER date;
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

			if ( json_buf_length + cfg_val_length + 20 + 2 + 30 + 2 + 1 >= ( MAX_BUFFER_SIZE - 10 ) )	// 20 for timestamp, 2 for Ignore and Forward, 30 for the json, 2 for "]}".
			{
				--json_buf_length;

				if ( escaped_caller_id != NULL )
				{
					GlobalFree( escaped_caller_id );
					escaped_caller_id = NULL;
				}

				exit_loop = true;
				break;
			}

			json_buf_length += __snprintf( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"C\":\"%s\",\"N\":\"%s\",\"T\":%lld,\"I\":%c,\"F\":%c}", ( escaped_caller_id != NULL ? escaped_caller_id : SAFESTRA( di->ci.caller_id ) ), SAFESTRA( di->ci.call_from ), date.QuadPart, ( di->ci.ignored == true ? '1' : '0' ), ( di->ci.forwarded == true ? '1' : '0' ) );

			if ( escaped_caller_id != NULL )
			{
				GlobalFree( escaped_caller_id );
				escaped_caller_id = NULL;
			}

			dll = dll->next;

			if ( dll != NULL || node->next != NULL )
			{
				json_buf[ json_buf_length++ ] = ',';
			}
		}

		if ( exit_loop == true )
		{
			break;
		}

		node = node->next;
	}

	// Remember our last nodes so we can continue from the last position if we broke the data up.
	socket_context->call_list = dll;
	socket_context->call_node = node;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "]}", 2 );
	json_buf_length += 2;

	// Websocket payload size. Adjust the start of our buffer if the payload is less than the given amounts.
	if ( json_buf_length <= 125 )
	{
		socket_context->io_context.wsabuf.buf += 8;
	}
	else if ( json_buf_length <= 65535 )
	{
		socket_context->io_context.wsabuf.buf += 6;
	}

	return ConstructFrameHeader( socket_context->io_context.wsabuf.buf, MAX_BUFFER_SIZE, WS_BINARY_FRAME, /*json_buf,*/ json_buf_length ) + json_buf_length;
}

THREAD_RETURN SendCallLogUpdate( LPVOID pArg )
{
	// Make sure this assignment is done before we enter the critical section.
	displayinfo *di = ( displayinfo * )pArg;

	EnterCriticalSection( &context_list_cs );

	if ( total_clients == 0 )
	{
		goto CLEANUP;
	}

	UPDATE_BUFFER *ub = NULL;

	EnterCriticalSection( &g_update_buffer_pool_cs );

	ub = GetAvailableUpdateBuffer( g_update_buffer_pool );
	if ( ub == NULL )
	{
		ub = ( UPDATE_BUFFER * )GlobalAlloc( GPTR, sizeof( UPDATE_BUFFER ) );
		ub->buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * MAX_BUFFER_SIZE );
		InterlockedExchange( &ub->count, 0 );

		DoublyLinkedList *dll_node = DLL_CreateNode( ( void * )ub );
		DLL_AddNode( &g_update_buffer_pool, dll_node, -1 );
	}

	LeaveCriticalSection( &g_update_buffer_pool_cs );

	char *buffer = ub->buffer;
	char *pBuf = buffer;

	char *json_buf = pBuf + 10;	// 10 byte offset so that we can encode the websocket frame header.
	int json_buf_length = 0;

	_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"CALL_LOG_UPDATE\":[", 20 );
	json_buf_length += 20;

	int cfg_val_length = lstrlenA( SAFESTRA( di->ci.caller_id ) );

	unsigned int ec_length = 0;
	char *escaped_caller_id = json_escape( SAFESTRA( di->ci.caller_id ), cfg_val_length, &ec_length );

	if ( escaped_caller_id != NULL )
	{
		cfg_val_length = ec_length;
	}

	cfg_val_length += lstrlenA( SAFESTRA( di->ci.call_from ) );
	cfg_val_length += lstrlenA( SAFESTRA( di->ci.call_reference_id ) );

	ULARGE_INTEGER date;
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

	if ( json_buf_length + cfg_val_length + 20 + 2 + 37 + 2 + 1 < ( MAX_BUFFER_SIZE - 10 ) )	// 20 for timestamp, 2 for ignore/forward state, 37 for the json, 2 for "]}".
	{
		json_buf_length += __snprintf( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "{\"C\":\"%s\",\"N\":\"%s\",\"T\":%lld,\"R\":\"%s\",\"I\":%c,\"F\":%c}", ( escaped_caller_id != NULL ? escaped_caller_id : SAFESTRA( di->ci.caller_id ) ), SAFESTRA( di->ci.call_from ), date.QuadPart, SAFESTRA( di->ci.call_reference_id ), ( di->ci.ignored == true ? '1' : '0' ), ( di->ci.forwarded == true ? '1' : '0' ) );

		_memcpy_s( json_buf + json_buf_length, ( MAX_BUFFER_SIZE - 10 ) - json_buf_length, "]}", 2 );
		json_buf_length += 2;

		// Websocket payload size. Adjust the start of our buffer if the payload is less than the given amounts.
		if ( json_buf_length <= 125 )
		{
			pBuf += 8;
		}
		else if ( json_buf_length <= 65535 )
		{
			pBuf += 6;
		}

		int reply_buf_length = ConstructFrameHeader( pBuf, MAX_BUFFER_SIZE, WS_BINARY_FRAME, /*json_buf,*/ json_buf_length ) + json_buf_length;

		// When count is decremented to 0, then the buffer is free to be reused.
		InterlockedExchange( &ub->count, total_clients );

		DoublyLinkedList *context_node = client_context_list;

		while ( context_node != NULL )
		{
			if ( g_bEndServer == true )
			{
				break;
			}

			SOCKET_CONTEXT *socket_context = ( SOCKET_CONTEXT * )context_node->data;

			if ( socket_context->connection_type == CON_TYPE_WEBSOCKET )
			{
				InterlockedExchange( &socket_context->ping_sent, 0 );	// Reset the ping count. We'll consider this a ping.

				int nRet = 0;
				DWORD dwFlags = 0;

				bool sent = true;
				InterlockedIncrement( &socket_context->io_context.ref_update_count );

				UPDATE_BUFFER_STATE *ubs = NULL;

				EnterCriticalSection( &socket_context->write_cs );

				ubs = GetAvailableUpdateBufferState( socket_context );
				if ( ubs == NULL )
				{
					ubs = ( UPDATE_BUFFER_STATE * )GlobalAlloc( GPTR, sizeof( UPDATE_BUFFER_STATE ) );
					DoublyLinkedList *dll_node = DLL_CreateNode( ( void * )ubs );
					DLL_AddNode( &socket_context->io_context.update_buffer_state, dll_node, -1 );
				}

				ubs->in_use = true;	// If this is still true when we close the client (CloseClient), then it'll be used to decrement the g_update_buffer_pool count.
				ubs->wsabuf.buf = pBuf;
				ubs->wsabuf.len = reply_buf_length;
				ubs->IOOperation = ClientIoWrite;
				ubs->NextIOOperation = ClientIoReadWebSocketRequest;
				ubs->count = &( ub->count );

				LeaveCriticalSection( &socket_context->write_cs );

				if ( use_ssl == true )
				{
					SEND_BUFFER *sb = NULL;

					EnterCriticalSection( &socket_context->write_cs );

					sb = GetAvailableSendBuffer( socket_context );
					if ( sb == NULL )
					{
						sb = ( SEND_BUFFER * )GlobalAlloc( GPTR, sizeof( SEND_BUFFER ) );
						DoublyLinkedList *dll_node = DLL_CreateNode( ( void * )sb );
						DLL_AddNode( &socket_context->ssl->sd.send_buffer_pool, dll_node, -1 );
					}

					sb->in_use = true;
					sb->wsabuf = &( ubs->wsabuf );
					sb->IOOperation = &( ubs->IOOperation );
					sb->NextIOOperation = &( ubs->NextIOOperation );
					sb->overlapped = &( ubs->overlapped );

					LeaveCriticalSection( &socket_context->write_cs );

					nRet = SSL_WSASend( socket_context, &( ubs->wsabuf ), sb, sent );
					if ( nRet != SEC_E_OK )
					{
						sent = false;
					}
				}
				else
				{
					nRet = _WSASend( socket_context->Socket, &( ubs->wsabuf ), 1, NULL, dwFlags, &( ubs->overlapped ), NULL );
					if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
					{
						sent = false;
					}
				}

				if ( sent == false )
				{
					InterlockedDecrement( &socket_context->io_context.ref_update_count );
					BeginClose( socket_context );
				}
			}
			else
			{
				// We didn't send to this context, so decrement it from our total.
				InterlockedDecrement( &ub->count );
			}

			context_node = context_node->next;
		}
	}

	if ( escaped_caller_id != NULL )
	{
		GlobalFree( escaped_caller_id );
		escaped_caller_id = NULL;
	}

CLEANUP:

	LeaveCriticalSection( &context_list_cs );

	ExitThread( 0 );
	return 0;
}

UPDATE_BUFFER *GetAvailableUpdateBuffer( DoublyLinkedList *update_buffer_pool )
{
	UPDATE_BUFFER *ub = NULL;

	DoublyLinkedList *node = g_update_buffer_pool;
	while ( node != NULL )
	{
		if ( ( ( UPDATE_BUFFER * )node->data )->count == 0 )
		{
			ub = ( UPDATE_BUFFER * )node->data;
			break;
		}

		node = node->next;
	}

	return ub;
}

UPDATE_BUFFER_STATE *GetAvailableUpdateBufferState( SOCKET_CONTEXT *socket_context )
{
	UPDATE_BUFFER_STATE *ubs = NULL;
	if ( socket_context != NULL )
	{
		DoublyLinkedList *node = socket_context->io_context.update_buffer_state;
		while ( node != NULL )
		{
			if ( !( ( UPDATE_BUFFER_STATE * )node->data )->in_use )
			{
				ubs = ( UPDATE_BUFFER_STATE * )node->data;
				break;
			}

			node = node->next;
		}
	}

	return ubs;
}

UPDATE_BUFFER_STATE *FindUpdateBufferState( SOCKET_CONTEXT *socket_context, LPWSAOVERLAPPED lpWSAOverlapped )
{
	UPDATE_BUFFER_STATE *ubs = NULL;
	if ( socket_context != NULL && lpWSAOverlapped != NULL )
	{
		DoublyLinkedList *node = socket_context->io_context.update_buffer_state;
		while ( node != NULL )
		{
			if ( &( ( ( UPDATE_BUFFER_STATE * )node->data )->overlapped ) == lpWSAOverlapped )
			{
				ubs = ( UPDATE_BUFFER_STATE * )node->data;
				break;
			}

			node = node->next;
		}
	}

	return ubs;
}

void CleanupUpdateBufferPool( DoublyLinkedList **update_buffer_pool )
{
	while ( *update_buffer_pool != NULL )
	{
		DoublyLinkedList *del_node = *update_buffer_pool;
		*update_buffer_pool = ( *update_buffer_pool )->next;

		GlobalFree( ( ( UPDATE_BUFFER * )del_node->data )->buffer );
		GlobalFree( ( UPDATE_BUFFER * )del_node->data );
		GlobalFree( del_node );
	}

	*update_buffer_pool = NULL;
}
