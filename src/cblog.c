/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with main.c; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#define DEBUG_ON
#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/events.h>
#include <cutil/log.h>
#include <cutil/child.h>
#include <cutil/sanitize.h>
#include <cllsd/llsd.h>
#include <cllsd/llsd_util.h>

#if __STDC__ == 1
#define VERSION_MAJOR "0"
#define VERSION_MINOR "1"
#define VERSION_STRING ("cblog " VERSION_MAJOR  "." VERSION_MINOR "." __DATE__ "." __TIME__ "\n")
#else
#error cbot requires a conforming standard C compiler, this one is not
#endif

static int8_t const * config_file = NULL;
static llsd_t * config = NULL;
static evt_loop_t * el = NULL;
static evt_t int_h;
static evt_t term_h;
static evt_params_t int_params;
static evt_params_t term_params;
	
static evt_ret_t signal_cb( evt_loop_t * const el,
							evt_t * const evt,
							evt_params_t * const params,
							void * user_data )
{
	switch( params->signal_params.signum )
	{
		case SIGINT:
			DEBUG( "received SIGINT\n" );
			break;
		case SIGTERM:
			DEBUG( "received SIGTERM\n" );
			break;
	}

	/* stop the event loop so we'll exit */
	DEBUG("stopping event loop\n");
	evt_stop( session->el );

	return EVT_OK;
}

static void print_help( int8_t const * const app )
{
	printf(VERSION_STRING);
	printf("\n");
	printf("Usage: %s -C <config file>\n", app );
	printf("\n");
}

static int parse_options( int argc, char** argv )
{
	int opt = 0;
	config_file = NULL;

	opt = getopt( argc, argv, "C:" );
	if ( opt == -1 )
	{
		print_help( T(argv[0]) );
		return FALSE;
	}
	if ( opt != 'C' )
	{
		WARN( "unknown command line option\n" );
		print_help( T(argv[0]) );
		return FALSE;
	}

	/* get the file name */
	config_file = optarg;

	return TRUE;
}

static int load_config( void )
{
	FILE *cfg = NULL;
	config = NULL;
	CHECK_PTR_RET_MSG( config_file, FALSE, "no config file path specified\n" );

	if ( cfg = fopen( config_file, "r" ) )
	{
		config = llsd_parse( cfg );
		CHECK_PTR_RET_MSG( config, FALSE, "failed to parse LLSD config file\n" );

		/* print the config file */
		llsd_format( config, LLSD_ENC_XML, stdout, TRUE );
	}
	else
	{
		WARN( "unable to open/read config file: %s\n", config_file );
		return FALSE;
	}
	return TRUE;
}

static int get_globals( void )
{
	llsd_t * tmp;

	CHECK_PTR_RET( config, FALSE );

	/* get the root dir value */
	tmp = llsd_map_find( config, "directory" );
	CHECK_PTR_RET( tmp, FALSE );
	NOTICE( "root directory: %s\n", llsd_as_string( tmp ).str );

	/* get the daemonize flag */
	tmp = llsd_map_find( config, "daemon" );
	CHECK_PTR_RET( tmp, FALSE );
	NOTICE( "daemon: %s\n", (llsd_as_bool( tmp ) ? "true" : "false") );

	/* get the pid file path */
	tmp = llsd_map_find( config, "pidfile" );
	CHECK_PTR_RET( tmp, FALSE );
	NOTICE( "pidfile: %s\n", llsd_as_string( tmp ).str );

	/* get the start file path */
	tmp = llsd_map_find( config, "startfile" );
	CHECK_PTR_RET( tmp, FALSE );
	NOTICE( "startfile: %s\n", llsd_as_string( tmp ).str );
	
	return TRUE;
}

static void start_event_handlers( void )
{
	/* create the event loop */
	el = evt_new();

	/* create SIGINT signal handler */
	MEMSET( &int_params, 0, sizeof(evt_params_t) );
	int_params.signal_params.signum = SIGINT;
	evt_initialize_event_handler( &int_h, el, EVT_SIGNAL, &int_params, signal_cb, (void*)&session );
	
	/* create SIGTERM signal handler */
	MEMSET( &term_params, 0, sizeof(evt_params_t) );
	term_params.signal_params.signum = SIGTERM;
	evt_initialize_event_handler( &term_h, el, EVT_SIGNAL, &term_params, signal_cb, (void*)&session );

	/* start both event handlers */
	evt_start_event_handler( el, &int_h );
	evt_start_event_handler( el, &term_h );
}

static void stop_event_handlers( void )
{
	/* stop the event handlers */
	evt_stop_event_handler( el, &term_h );
	evt_stop_event_handler( el, &int_h );

	/* clean up both event handlers */
	evt_deinitialize_event_handler( &term_h );
	evt_deinitialize_event_handler( &int_h );

	/* clean up the event loop */
	evt_delete( el );
}

int main(int argc, char** argv)
{
	/* clean up the file descriptors */
	sanitize_files();

	/* TODO: build a clean environment */

	/* start syslog logging */
	start_logging( "cblog" );

	/* banner the syslog */
	NOTICE( "%s\n", VERSION_STRING );

	/* parse options and load config file */
	CHECK_RET( parse_options( argc, argv ), EXIT_FAILURE );
	CHECK_RET( load_config(), EXIT_FAILURE );

	/* we should have a loaded config */
	CHECK_RET( get_globals(), EXIT_FAILURE );

	/* blocking call to run event loop */
	evt_run( el );

	/* close the logging facility */
	stop_logging();

	return (EXIT_SUCCESS);
}
