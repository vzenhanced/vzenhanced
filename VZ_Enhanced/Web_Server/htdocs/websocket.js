// Connection
var connected = false;
var host = null;
var protocol = null;
var ws = null;

// List
var l_lines = false;
var cl_lines = false;
var il_lines = false;
var fl_lines = false;
var icidl_lines = false;
var fcidl_lines = false;
var display_width = "";

// Tabs
var last_div = null;
var last_header = null;

// Time
var minute_offset = 0;
var months = new Array( "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" );
var days = new Array( "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" );

// Forward Window
var forward_window = null;
var forward_to_input = null;
var forward_from_input = null;
var forward_reference_hidden = null;
var forward_call_log_id = null;
var last_forward_number = "";

// Loading Window
var loading = null;

// Title
var title_timer = null;
var timer_set = false;
var is_blurred = false;
var title_message = "";

function create_loading_window()
{
	loading = document.createElement( "div" );
	loading.style.cssText = "display: none; position: absolute; top: 70px; width: 100px; left: 50%; margin-left: -50px; color: #777777; font-size: 14px; font-weight: bold; text-align: center;";
	loading.appendChild( document.createTextNode( "Loading..." ) );
	document.body.appendChild( loading );
}

function getScrollbarWidth()
{
	var outer = document.createElement( "div" );
	outer.style.visibility = "hidden";
	outer.style.width = "100px";
	document.body.appendChild( outer );

	var width_without_scrollbar = outer.offsetWidth;
	outer.style.overflow = "scroll";

	var inner = document.createElement( "div" );
	inner.style.width = "100%";
	outer.appendChild( inner );        

	var width_with_scrollbar = inner.offsetWidth;
	outer.parentNode.removeChild( outer );

	return width_without_scrollbar - width_with_scrollbar;
}

function check_width( id1, id2, id3 )
{
	var e = document.getElementById( id1 );
	if ( e.scrollHeight > e.clientHeight )
	{
		document.getElementById( id2 ).style.width = display_width;
		document.getElementById( id3 ).style.width = display_width;
	}
	else
	{
		document.getElementById( id2 ).style.width = "640px";
		document.getElementById( id3 ).style.width = "640px";
	}
}

function toggle_header( obj, id )
{
	var e = document.getElementById( id );

	if ( last_div != e )
	{
		last_div.style.display = "none";

		e.style.display = "table";

		last_div = e;

		last_header.style.backgroundColor = "";
		last_header.style.backgroundImage = "";

		obj.style.backgroundColor = "#EEEEEE";
		obj.style.backgroundImage = "linear-gradient(to bottom, #CCCCCC, #EEEEEE)";

		last_header = obj;

		if ( id == "columns_l" ) { check_width( "lo", "call_log", "l_row" ); }
		else if ( id == "columns_cl" ) { check_width( "clo", "contact_list", "cl_row" ); }
		else if ( id == "columns_fl" ) { check_width( "flo", "forward_list", "fl_row" ); }
		else if ( id == "columns_il" ) { check_width( "ilo", "ignore_list", "il_row" ); }
	}
}

function initialize_page()
{
	var url = location;
	host = url.host;
	protocol = url.protocol;

	if ( protocol == "https:" )
	{
		protocol = "wss";
	}
	else if ( protocol == "http:" )
	{
		protocol = "ws";
	}

	last_div = document.getElementById( "columns_l" );
	last_header = document.getElementById( "header1" );

	display_width = ( document.getElementById( "call_log" ).offsetWidth - getScrollbarWidth() ) + "px";

	last_header.style.backgroundColor = "#EEEEEE";
	last_header.style.backgroundImage = "linear-gradient(to bottom, #CCCCCC, #EEEEEE)";

	var local_date = new Date();
	minute_offset = local_date.getTimezoneOffset();

	create_loading_window();
	create_forward_window();
}

var got_il = false;
var got_fl = false;
var got_cl = false;
var got_l = false;
var got_fcidl = false;
var got_icidl = false;

function get_ignore_list()
{
	if ( ws != null && got_il == false )
	{
		got_il = true;
		ws.send("GET_IGNORE_LIST");
	}
}

function get_forward_list()
{
	if ( ws != null && got_fl == false )
	{
		got_fl = true;
		ws.send("GET_FORWARD_LIST");
	}
}

function get_contact_list()
{
	if ( ws != null && got_cl == false )
	{
		got_cl = true;
		ws.send("GET_CONTACT_LIST");
	}
}

function get_call_log()
{
	if ( ws != null && got_l == false )
	{
		got_l = true;
		ws.send("GET_CALL_LOG");
	}
}

function get_forward_cid_list()
{
	if ( ws != null && got_fcidl == false )
	{
		got_fcidl = true;
		ws.send("GET_FORWARD_CID_LIST");
	}
}

function get_ignore_cid_list()
{
	if ( ws != null && got_icidl == false )
	{
		got_icidl = true;
		ws.send("GET_IGNORE_CID_LIST");
	}
}

function format_timestamp( ts )
{
	// Friday, October 11, 2013 5:35:27 PM
	var date = new Date( ( ts + ( minute_offset * 60 ) ) * 1000 );
	return days[ date.getDay() ] + ", " + months[ date.getMonth() ] + " " + date.getDate() + ", " + date.getFullYear() + " " + date.toLocaleTimeString();
}

function format_time( ts )
{
	var date = new Date( ( ts + ( minute_offset * 60 ) ) * 1000 );
	return date.toLocaleTimeString();
}

function format_phone_number( pn )
{
	if ( !isNaN( pn ) )
	{
		if ( pn.length == 10 )	// (555) 555-5555
		{
			return "(" + pn.substring( 0, 3 ) + ") " + pn.substring( 3, 6 ) + "-" + pn.substring( 6 );
		}
		else if ( pn.length == 11 && pn[ 0 ] == '1' )	// 1-555-555-5555
		{
			return pn[ 0 ] + "-" + pn.substring( 1, 4 ) + "-" + pn.substring( 4, 7 ) + "-" + pn.substring( 7 );
		}
	}

	return pn;
}

function ab_to_str( buffer )
{
	return String.fromCharCode.apply( null, new Uint8Array( buffer ) );
}

function set_forward_info_values( to, from, reference, id )
{
	forward_to_input.value = to;
	forward_from_input.value = from;
	forward_reference_hidden.value = reference;
	forward_call_log_id = id;
}

function update_input( num )
{
	if ( forward_to_input.value.length < 15 )
	{
		forward_to_input.value += num;
	}
}

function validate_input( evt )
{
	if ( evt == null )
	{
		evt = window.event;
	}

	var key = ( evt.which ) ? evt.which : evt.keyCode;
    if ( key < 48 || key > 57 )
	{
        return false;
	}

    return true;
}

function create_forward_window()
{
	forward_window = document.createElement("div");
	forward_window.style.cssText = "display: none; z-index: 100; position: absolute; top: 27%; left: 50%; margin-left: -100px; width: 200px; padding-top: 5px; border-radius: 8px; border: 1px solid #AAAAAA; box-shadow: 2px 2px 5px 0px #AAAAAA; background-color: #FFFFFF;";

		var dial_row_0 = document.createElement("div");
		dial_row_0.style.cssText = "display: table; border-spacing: 10px 5px; width: 100%";

			var label_from = document.createElement( "label" );
			label_from.style.cssText = "color: #777777; font-weight: bold; text-shadow: 0px 1px 0px #FFFFFF;"
			label_from.appendChild( document.createTextNode( "From:" ) );
			label_from.appendChild( document.createElement( "br" ) );

				forward_from_input = document.createElement("input");
				forward_from_input.style.cssText = "outline: 0; border: 1px solid #CCCCCC; background-color: #EEEEEE; text-align: center; width: 176px; border-radius: 12px; box-shadow: 0px 0px 4px 0px #AAAAAA;/* color: #777777; font-weight: bold; text-shadow: 0px 1px 0px #FFFFFF;*/";
				forward_from_input.maxLength = "16";    // Allow the '+' digit.
				forward_from_input.disabled = "disabled";
				forward_from_input.onfocus = function() { forward_from_input.style.borderColor = "#AAAAAA"; };
				forward_from_input.onblur = function() { forward_from_input.style.borderColor = "#CCCCCC"; };

			label_from.appendChild( forward_from_input );
			dial_row_0.appendChild( label_from );

			dial_row_0.appendChild( document.createElement( "br" ) );

			var label_to = document.createElement( "label" );
			label_to.style.cssText = "color: #777777; font-weight: bold; text-shadow: 0px 1px 0px #FFFFFF;"
			label_to.appendChild( document.createTextNode( "To:" ) );
			label_to.appendChild( document.createElement( "br" ) );

				forward_to_input = document.createElement("input");
				forward_to_input.style.cssText = "outline: 0; border: 1px solid #CCCCCC; background-color: #FFFFFF; text-align: center; width: 176px; border-radius: 12px; box-shadow: 0px 0px 4px 0px #AAAAAA;/* color: #777777; font-weight: bold; text-shadow: 0px 1px 0px #FFFFFF;*/";
				forward_to_input.maxLength = "15";  // Don't allow the '+' digit.
				forward_to_input.onkeypress = validate_input;
				forward_to_input.onfocus = function() { forward_to_input.style.borderColor = "#AAAAAA"; };
				forward_to_input.onblur = function() { forward_to_input.style.borderColor = "#CCCCCC"; };

			label_to.appendChild( forward_to_input );
			dial_row_0.appendChild( label_to );

			forward_reference_hidden = document.createElement("input");
			forward_reference_hidden.type = "hidden";

			dial_row_0.appendChild( forward_reference_hidden );

		var dial_row_1 = document.createElement("div");
		dial_row_1.style.cssText = "display: table; border-spacing: 10px 5px; width: 100%; text-align: center;";

			var dial_1 = document.createElement("a");
			dial_1.className = "popup_button";
			dial_1.style.cssText = "width: 40px; height: 40px;";
			dial_1.appendChild( document.createTextNode( "1" ) );
			dial_1.onclick = function(){ update_input( "1" ); return false; };
			dial_1.href = "";

			var dial_2 = document.createElement("a");
			dial_2.className = "popup_button";
			dial_2.style.cssText = "width: 40px; height: 40px;";
			dial_2.appendChild( document.createTextNode( "2" ) );
			dial_2.appendChild( document.createElement( "br" ) );
			dial_2.appendChild( document.createTextNode( "ABC" ) );
			dial_2.onclick = function(){ update_input( "2" ); return false; };
			dial_2.href = "";

			var dial_3 = document.createElement("a");
			dial_3.className = "popup_button";
			dial_3.style.cssText = "width: 40px; height: 40px;";
			dial_3.appendChild( document.createTextNode( "3" ) );
			dial_3.appendChild( document.createElement( "br" ) );
			dial_3.appendChild( document.createTextNode( "DEF" ) );
			dial_3.onclick = function(){ update_input( "3" ); return false; };
			dial_3.href = "";

		dial_row_1.appendChild( dial_1 );
		dial_row_1.appendChild( dial_2 );
		dial_row_1.appendChild( dial_3 );

		var dial_row_2 = document.createElement("div");
		dial_row_2.style.cssText = "display: table; border-spacing: 10px 5px; width: 100%; text-align: center;";

			var dial_4 = document.createElement("a");
			dial_4.className = "popup_button";
			dial_4.style.cssText = "width: 40px; height: 40px;";
			dial_4.appendChild( document.createTextNode( "4" ) );
			dial_4.appendChild( document.createElement( "br" ) );
			dial_4.appendChild( document.createTextNode( "GHI" ) );
			dial_4.onclick = function(){ update_input( "4" ); return false; };
			dial_4.href = "";

			var dial_5 = document.createElement("a");
			dial_5.className = "popup_button";
			dial_5.style.cssText = "width: 40px; height: 40px;";
			dial_5.appendChild( document.createTextNode( "5" ) );
			dial_5.appendChild( document.createElement( "br" ) );
			dial_5.appendChild( document.createTextNode( "JKL" ) );
			dial_5.onclick = function(){ update_input( "5" ); return false; };
			dial_5.href = "";

			var dial_6 = document.createElement("a");
			dial_6.className = "popup_button";
			dial_6.style.cssText = "width: 40px; height: 40px;";
			dial_6.appendChild( document.createTextNode( "6" ) );
			dial_6.appendChild( document.createElement( "br" ) );
			dial_6.appendChild( document.createTextNode( "MNO" ) );
			dial_6.onclick = function(){ update_input( "6" ); return false; };
			dial_6.href = "";

		dial_row_2.appendChild( dial_4 );
		dial_row_2.appendChild( dial_5 );
		dial_row_2.appendChild( dial_6 );

		var dial_row_3 = document.createElement("div");
		dial_row_3.style.cssText = "display: table; border-spacing: 10px 5px; width: 100%; text-align: center;";

			var dial_7 = document.createElement("a");
			dial_7.className = "popup_button";
			dial_7.style.cssText = "width: 40px; height: 40px;";
			dial_7.appendChild( document.createTextNode( "7" ) );
			dial_7.appendChild( document.createElement( "br" ) );
			dial_7.appendChild( document.createTextNode( "PQRS" ) );
			dial_7.onclick = function(){ update_input( "7" ); return false; };
			dial_7.href = "";

			var dial_8 = document.createElement("a");
			dial_8.className = "popup_button";
			dial_8.style.cssText = "width: 40px; height: 40px;";
			dial_8.appendChild( document.createTextNode( "8" ) );
			dial_8.appendChild( document.createElement( "br" ) );
			dial_8.appendChild( document.createTextNode( "TUV" ) );
			dial_8.onclick = function(){ update_input( "8" ); return false; };
			dial_8.href = "";

			var dial_9 = document.createElement("a");
			dial_9.className = "popup_button";
			dial_9.style.cssText = "width: 40px; height: 40px;";
			dial_9.appendChild( document.createTextNode( "9" ) );
			dial_9.appendChild( document.createElement( "br" ) );
			dial_9.appendChild( document.createTextNode( "WXYZ" ) );
			dial_9.onclick = function(){ update_input( "9" ); return false; };
			dial_9.href = "";

		dial_row_3.appendChild( dial_7 );
		dial_row_3.appendChild( dial_8 );
		dial_row_3.appendChild( dial_9 );

		var dial_row_4 = document.createElement("div");
		dial_row_4.style.cssText = "display: table; border-spacing: 10px 5px; width: 100%; text-align: center;";

			var dial_0a = document.createElement("div");
			dial_0a.className = "popup_button";
			dial_0a.style.cssText = "width: 40px; height: 40px;";
			dial_0a.style.visibility = "hidden";

			var dial_0 = document.createElement("a");
			dial_0.className = "popup_button";
			dial_0.style.cssText = "width: 40px; height: 40px;";
			dial_0.appendChild( document.createTextNode( "0" ) );
			dial_0.onclick = function(){ update_input( "0" ); return false; };
			dial_0.href = "";

			var dial_0b = document.createElement("div");
			dial_0b.className = "popup_button";
			dial_0b.style.cssText = "width: 40px; height: 40px;";
			dial_0b.style.visibility = "hidden";

		dial_row_4.appendChild( dial_0a );
		dial_row_4.appendChild( dial_0 );
		dial_row_4.appendChild( dial_0b );

		var dial_row_5 = document.createElement("div");
		dial_row_5.style.cssText = "display: table; border-top: 1px solid #AAAAAA; margin-top: 5px; border-spacing: 10px; width: 100%; text-align: center;";

			var dial_forward = document.createElement("a");
			dial_forward.className = "popup_button";
			dial_forward.appendChild( document.createTextNode( "Forward" ) );
			dial_forward.onclick = function(){ forward_incoming_call( forward_to_input.value, forward_from_input.value, forward_reference_hidden.value, forward_call_log_id ); show_hide_forward_window( 0 ); return false; };
			dial_forward.href = "";

			var dial_cancel = document.createElement("a");
			dial_cancel.className = "popup_button";
			dial_cancel.appendChild( document.createTextNode( "Cancel" ) );
			dial_cancel.onclick = function(){ show_hide_forward_window( 0 ); return false; };
			dial_cancel.href = "";

		dial_row_5.appendChild( dial_forward );
		dial_row_5.appendChild( dial_cancel );

	forward_window.appendChild( dial_row_0 );
	forward_window.appendChild( dial_row_1 );
	forward_window.appendChild( dial_row_2 );
	forward_window.appendChild( dial_row_3 );
	forward_window.appendChild( dial_row_4 );
	forward_window.appendChild( dial_row_5 );

	document.body.appendChild( forward_window );
}

function forward_incoming_call( to, from, reference, id )
{
	var number = to.replace( /\s/g, 'X' );	// Replace whitepsace with X. Prevent the number from going through.
	if ( ws != null && isNaN( number ) == false )
	{
		last_forward_number = to;
		ws.send( "FORWARD_INCOMING:" + from + ":" + number + ":" + reference );

		document.getElementById( id ).style.color = "#FF8000";	// Orange
	}
}

function ignore_incoming_call( from, reference, id )
{
	if ( ws != null )
	{
		ws.send( "IGNORE_INCOMING:" + from + ":" + reference );

		document.getElementById( id ).style.color = "#FF0000";	// Red
	}
}

function show_hide_forward_window( type )
{
	if ( type == 0 )
	{
		forward_window.style.display = "none";
	}
	else
	{
		forward_window.style.display = "block";
		forward_to_input.focus();
	}
}

function set_title( reset )
{
	if ( reset == true )
	{
		if ( timer_set == false )
		{
			timer_set = true;
			title_timer = window.setInterval( function()
			{
				document.title = title_message;
				title_message = title_message.substring( 1 ) + title_message.substring( 0, 1 );
			}, 1000 );
		}
	}
	else
	{
		clearInterval( title_timer );
		timer_set = false;
		title_message = "";
		document.title = "VZ Enhanced";
	}
}

window.onblur = function()
{
	is_blurred = true;
}

window.onfocus = function()
{
	is_blurred = false;
	set_title( false );	// Reset the title.
}

function create_popup( caller_id, from, timestamp, reference )
{
	var popup = document.createElement("div");
	popup.style.cssText = "position: absolute; top: 15px; left: 50%; margin-left: -120px; width: 240px; border-radius: 8px; border: 1px solid #AAAAAA; box-shadow: 2px 2px 5px 0px #AAAAAA; background-color: #FFFFFF;";

		var t_div = document.createElement("div");
		t_div.style.cssText = "height: 20px; padding-right: 5px; padding-left: 5px; padding-top: 5px; text-align: right;/* color: #777777; font-weight: bold; text-shadow: 0px 1px 0px #FFFFFF;*/";
		t_div.appendChild( document.createTextNode( format_time( timestamp ) ) );

		var c_div = document.createElement("div");
		c_div.style.cssText = "height: 20px; padding-right: 5px; padding-left: 5px;/* color: #777777; font-weight: bold; text-shadow: 0px 1px 0px #FFFFFF;*/";
		c_div.appendChild( document.createTextNode( caller_id ) );

		var p_div = document.createElement("div");
		p_div.style.cssText = "height: 20px; padding-right: 5px; padding-left: 5px;/* color: #777777; font-weight: bold; text-shadow: 0px 1px 0px #FFFFFF;*/";
		var txt_phone_number = format_phone_number( from );
		p_div.appendChild( document.createTextNode( txt_phone_number ) );

		var action_div = document.createElement("div");
		action_div.style.cssText = "display: table; border-top: 1px solid #AAAAAA; border-spacing: 10px; width: 100%; text-align: center;";

			var ignore_a = document.createElement("a");
			ignore_a.className = "popup_button";
			ignore_a.onclick = function(){ ignore_incoming_call( from, reference, timestamp ); document.body.removeChild( popup ); return false; }; // Timestamp is used as the id of a row in the call log.
			ignore_a.href = "";
			ignore_a.appendChild( document.createTextNode( "Ignore" ) );

			var forward_a = document.createElement("a");
			forward_a.className = "popup_button";
			forward_a.onclick = function(){ set_forward_info_values( last_forward_number, from, reference, timestamp ); show_hide_forward_window( 1 ); document.body.removeChild( popup ); return false; };
			forward_a.href = "";
			forward_a.appendChild( document.createTextNode( "Forward" ) );

			var close_a = document.createElement("a");
			close_a.className = "popup_button";
			close_a.onclick = function(){ document.body.removeChild( popup ); return false; };
			close_a.href = "";
			close_a.appendChild( document.createTextNode( "Close" ) );

			action_div.appendChild( ignore_a );
			action_div.appendChild( forward_a );
			action_div.appendChild( close_a );

		popup.appendChild( t_div );
		popup.appendChild( c_div );
		popup.appendChild( p_div );
		popup.appendChild( action_div );

	document.body.appendChild( popup );

	setTimeout( function()
	{
		document.body.removeChild( popup );
	}, 30000 )
	
	if ( is_blurred == true )
	{
		title_message = caller_id + " : " + txt_phone_number + " : ";
		set_title( true );	// Set and rotate the title message.
	}
}

function show_hide_loading_window( type )
{
	loading.style.display = ( type == 1 ) ? "block" : "none";
}

function connect()
{
	if ( "WebSocket" in window )
	{
		if ( connected == true )
		{
			if ( ws != null )
			{
				ws.close();
			}

			return;
		}

		ws = new WebSocket( protocol + "://" + host + "/connect");
		ws.binaryType = "arraybuffer";
		ws.onopen = function()
		{
			connected = true;
			document.getElementById( "connection" ).textContent = "Disconnect";

			show_hide_loading_window( 1 );

			get_call_log();
			get_contact_list();
			get_forward_list();
			get_ignore_list();
			get_forward_cid_list();
			get_ignore_cid_list();
		};

		ws.onmessage = function (evt) 
		{
			var received_msg = evt.data;
			received_msg = ab_to_str( received_msg );

			var jobj = JSON.parse( received_msg );
			
			var type = jobj.CALL_LOG_UPDATE;
			if ( type != null )
			{
				var result = document.getElementById("call_log");

				var div = document.createElement("div");
				div.id = jobj.CALL_LOG_UPDATE[ 0 ].T;
				div.className = ( l_lines == false ) ? "list_item_even" : "list_item_odd";
				l_lines = !l_lines;

				if ( jobj.CALL_LOG_UPDATE[ 0 ].I == 1 )	// The entry had been ignored. Ignore has priority over forwarded.
				{
					div.style.color = "#FF0000";	// Red
				}
				else if ( jobj.CALL_LOG_UPDATE[ 0 ].F == 1 )	// The entry had been forwarded.
				{
					div.style.color = "#FF8000";	// Orange
				}

				var caller_id = document.createElement("div");
				caller_id.style.cssText = "display: table-cell; width: 150px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

				var phone_number = document.createElement("div");
				phone_number.style.cssText = "display: table-cell; width: 150px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

				var time = document.createElement("div");
				time.style.cssText = "display: table-cell; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; padding-right: 10px;";

				var node = document.createTextNode( jobj.CALL_LOG_UPDATE[ 0 ].C );
				caller_id.appendChild( node );

				node = document.createTextNode( format_phone_number( jobj.CALL_LOG_UPDATE[ 0 ].N ) );
				phone_number.appendChild( node );

				node = document.createTextNode( format_timestamp( jobj.CALL_LOG_UPDATE[ 0 ].T ) );
				time.appendChild( node );

				div.appendChild( caller_id );
				div.appendChild( phone_number );
				div.appendChild( time );

				result.appendChild( div );

				check_width( "lo", "call_log", "l_row" );

				create_popup( jobj.CALL_LOG_UPDATE[ 0 ].C, jobj.CALL_LOG_UPDATE[ 0 ].N, jobj.CALL_LOG_UPDATE[ 0 ].T, jobj.CALL_LOG_UPDATE[ 0 ].R );

				type = null;

				return;
			}

			type = jobj.CALL_LOG;
			if ( type != null )
			{
				var result = document.getElementById("call_log");

				for ( var i = 0; i < jobj.CALL_LOG.length; i++ )
				{
					var div = document.createElement("div");
					div.className = ( l_lines == false ) ? "list_item_even" : "list_item_odd";
					l_lines = !l_lines;

					if ( jobj.CALL_LOG[ i ].I == 1 )	// The entry had been ignored. Ignore has priority over forwarded.
					{
						div.style.color = "#FF0000";	// Red
					}
					else if ( jobj.CALL_LOG[ i ].F == 1 )	// The entry had been forwarded.
					{
						div.style.color = "#FF8000";	// Orange
					}

					var caller_id = document.createElement("div");
					caller_id.style.cssText = "display: table-cell; width: 150px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var phone_number = document.createElement("div");
					phone_number.style.cssText = "display: table-cell; width: 150px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var time = document.createElement("div");
					time.style.cssText = "display: table-cell; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; padding-right: 10px;";

					var node = document.createTextNode( jobj.CALL_LOG[ i ].C );
					caller_id.appendChild( node );

					node = document.createTextNode( format_phone_number( jobj.CALL_LOG[ i ].N ) );
					phone_number.appendChild( node );

					node = document.createTextNode( format_timestamp( jobj.CALL_LOG[ i ].T ) );
					time.appendChild( node );

					div.appendChild( caller_id );
					div.appendChild( phone_number );
					div.appendChild( time );

					result.appendChild( div );

					check_width( "lo", "call_log", "l_row" );
				}

				type = null;

				return;
			}

			type = jobj.CONTACT_LIST;
			if ( type != null )
			{
				var result = document.getElementById("contact_list");

				for ( var i = 0; i < jobj.CONTACT_LIST.length; i++ )
				{
					var div = document.createElement("div");
					div.className = ( cl_lines == false ) ? "list_item_even" : "list_item_odd";
					cl_lines = !cl_lines;

					var div_first_name = document.createElement("div");
					div_first_name.style.cssText = "display: table-cell; width: 130px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_last_name = document.createElement("div");
					div_last_name.style.cssText = "display: table-cell; width: 130px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_home_number = document.createElement("div");
					div_home_number.style.cssText = "display: table-cell; width: 140px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_cell_number = document.createElement("div");
					div_cell_number.style.cssText = "display: table-cell; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; padding-right: 10px;";

					var node = document.createTextNode( jobj.CONTACT_LIST[ i ].F );
					div_first_name.appendChild( node );

					node = document.createTextNode( jobj.CONTACT_LIST[ i ].L );
					div_last_name.appendChild( node );

					node = document.createTextNode( format_phone_number( jobj.CONTACT_LIST[ i ].H ) );
					div_home_number.appendChild( node );

					node = document.createTextNode( format_phone_number( jobj.CONTACT_LIST[ i ].C ) );
					div_cell_number.appendChild( node );

					div.appendChild( div_first_name );
					div.appendChild( div_last_name );
					div.appendChild( div_home_number );
					div.appendChild( div_cell_number );

					result.appendChild( div );

					check_width( "clo", "contact_list", "cl_row" );
				}

				type = null;

				return;
			}

			type = jobj.FORWARD_LIST;
			if ( type != null )
			{
				var result = document.getElementById("forward_list");

				for ( var i = 0; i < jobj.FORWARD_LIST.length; i++ )
				{
					var div = document.createElement("div");
					div.className = ( fl_lines == false ) ? "list_item_even" : "list_item_odd";
					fl_lines = !fl_lines;

					var div_from = document.createElement("div");
					div_from.style.cssText = "display: table-cell; width: 220px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_to = document.createElement("div");
					div_to.style.cssText = "display: table-cell; width: 220px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_count = document.createElement("div");
					div_count.style.cssText = "display: table-cell; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; padding-right: 10px;";

					var node = document.createTextNode( format_phone_number( jobj.FORWARD_LIST[ i ].F ) );
					div_from.appendChild( node );

					node = document.createTextNode( format_phone_number( jobj.FORWARD_LIST[ i ].T ) );
					div_to.appendChild( node );

					node = document.createTextNode( jobj.FORWARD_LIST[ i ].C );
					div_count.appendChild( node );

					div.appendChild( div_from );
					div.appendChild( div_to );
					div.appendChild( div_count );

					result.appendChild( div );

					check_width( "flo", "forward_list", "fl_row" );
				}

				type = null;

				return;
			}

			type = jobj.IGNORE_LIST;
			if ( type != null )
			{
				var result = document.getElementById("ignore_list");

				for ( var i = 0; i < jobj.IGNORE_LIST.length; i++ )
				{
					var div = document.createElement("div");
					div.className = ( il_lines == false ) ? "list_item_even" : "list_item_odd";
					il_lines = !il_lines;

					var div_number = document.createElement("div");
					div_number.style.cssText = "display: table-cell; width: 300px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_count = document.createElement("div");
					div_count.style.cssText = "display: table-cell; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; padding-right: 10px;";

					var node = document.createTextNode( format_phone_number( jobj.IGNORE_LIST[ i ].N ) );
					div_number.appendChild( node );

					node = document.createTextNode( jobj.IGNORE_LIST[ i ].C );
					div_count.appendChild( node );

					div.appendChild( div_number );
					div.appendChild( div_count );

					result.appendChild( div );

					check_width( "ilo", "ignore_list", "il_row" );
				}

				type = null;

				return;
			}

			type = jobj.FORWARD_CID_LIST;
			if ( type != null )
			{
				var result = document.getElementById("forward_cid_list");

				for ( var i = 0; i < jobj.FORWARD_CID_LIST.length; i++ )
				{
					var div = document.createElement("div");
					div.className = ( fcidl_lines == false ) ? "list_item_even" : "list_item_odd";
					fcidl_lines = !fcidl_lines;

					var div_caller_id = document.createElement("div");
					div_caller_id.style.cssText = "display: table-cell; width: 130px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_forward_to = document.createElement("div");
					div_forward_to.style.cssText = "display: table-cell; width: 130px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_match_case = document.createElement("div");
					div_match_case.style.cssText = "display: table-cell; width: 80px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_match_whole_word = document.createElement("div");
					div_match_whole_word.style.cssText = "display: table-cell; width: 140px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_count = document.createElement("div");
					div_count.style.cssText = "display: table-cell; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; padding-right: 10px;";

					var node = document.createTextNode( jobj.FORWARD_CID_LIST[ i ].I );
					div_caller_id.appendChild( node );

					node = document.createTextNode( jobj.FORWARD_CID_LIST[ i ].F );
					div_forward_to.appendChild( node );

					node = document.createTextNode( ( jobj.FORWARD_CID_LIST[ i ].C == "1" ? "Yes" : "No" ) );
					div_match_case.appendChild( node );

					node = document.createTextNode( ( jobj.FORWARD_CID_LIST[ i ].W == "1" ? "Yes" : "No" ) );
					div_match_whole_word.appendChild( node );

					node = document.createTextNode( jobj.FORWARD_CID_LIST[ i ].T );
					div_count.appendChild( node );

					div.appendChild( div_caller_id );
					div.appendChild( div_forward_to );
					div.appendChild( div_match_case );
					div.appendChild( div_match_whole_word );
					div.appendChild( div_count );

					result.appendChild( div );

					check_width( "fcidlo", "forward_cid_list", "fcidl_row" );
				}

				type = null;

				return;
			}

			type = jobj.IGNORE_CID_LIST;
			if ( type != null )
			{
				var result = document.getElementById("ignore_cid_list");

				for ( var i = 0; i < jobj.IGNORE_CID_LIST.length; i++ )
				{
					var div = document.createElement("div");
					div.className = ( icidl_lines == false ) ? "list_item_even" : "list_item_odd";
					icidl_lines = !icidl_lines;

					var div_caller_id = document.createElement("div");
					div_caller_id.style.cssText = "display: table-cell; width: 130px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_match_case = document.createElement("div");
					div_match_case.style.cssText = "display: table-cell; width: 130px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_match_whole_word = document.createElement("div");
					div_match_whole_word.style.cssText = "display: table-cell; width: 140px; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; border-right: 1px solid #DDDDDD;";

					var div_count = document.createElement("div");
					div_count.style.cssText = "display: table-cell; padding-top: 1px; padding-bottom: 1px; padding-left: 10px; padding-right: 10px;";

					var node = document.createTextNode( jobj.IGNORE_CID_LIST[ i ].I );
					div_caller_id.appendChild( node );

					node = document.createTextNode( ( jobj.IGNORE_CID_LIST[ i ].C == "1" ? "Yes" : "No" ) );
					div_match_case.appendChild( node );

					node = document.createTextNode( ( jobj.IGNORE_CID_LIST[ i ].W == "1" ? "Yes" : "No" ) );
					div_match_whole_word.appendChild( node );

					node = document.createTextNode( jobj.IGNORE_CID_LIST[ i ].T );
					div_count.appendChild( node );

					div.appendChild( div_caller_id );
					div.appendChild( div_match_case );
					div.appendChild( div_match_whole_word );
					div.appendChild( div_count );

					result.appendChild( div );

					check_width( "icidlo", "ignore_cid_list", "icidl_row" );
				}

				type = null;

				show_hide_loading_window( 0 );

				return;
			}
		};

		ws.onclose = function()
		{
			connected = false;
			document.getElementById( "connection" ).textContent = "Connect";

			document.getElementById( "call_log" ).innerHTML = "";
			check_width( "lo", "call_log", "l_row" );
			document.getElementById( "contact_list" ).innerHTML = "";
			check_width( "clo", "contact_list", "cl_row" );
			document.getElementById( "forward_list" ).innerHTML = "";
			check_width( "flo", "forward_list", "fl_row" );
			document.getElementById( "ignore_list" ).innerHTML = "";
			check_width( "ilo", "ignore_list", "il_row" );
			document.getElementById( "forward_cid_list" ).innerHTML = "";
			check_width( "fcidlo", "forward_cid_list", "fcidl_row" );
			document.getElementById( "ignore_cid_list" ).innerHTML = "";
			check_width( "icidlo", "ignore_cid_list", "icidl_row" );

			show_hide_loading_window( 0 );

			got_icidl = false;
			got_fcidl = false;
			got_il = false;
			got_fl = false;
			got_cl = false;
			got_l = false;

			l_lines = false;
			cl_lines = false;
			il_lines = false;
			fl_lines = false;
			icidl_lines = false;
			fcidl_lines = false;
		};
	}
	else
	{
		alert( "This browser does not support WebSockets." );
	}
}