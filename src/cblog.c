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

#include "misc.h"

#if __STDC__ == 1
#define VERSION_MAJOR "0"
#define VERSION_MINOR "1"
#define VERSION_STRING ("cblog " VERSION_MAJOR  "." VERSION_MINOR "." __DATE__ "." __TIME__ "\n")
#else
#error cbot requires a conforming standard C compiler, this one is not
#endif

static int8_t const * config_file = NULL;
static llsd_t * config = NULL;

/* global config options */
static int do_daemon = FALSE;
static int8_t * root_dir = NULL;
static int8_t * pid_file = NULL;
static int8_t * start_file = NULL;


static evt_ret_t signal_cb( evt_loop_t * const el,
							evt_t * const evt,
							evt_params_t * const params,
							void * user_data );
static evt_sigs_t * sigevt = NULL;
#define NSIGS (2)
static sig_callback_t asigs[NSIGS] =
{
	{ SIGINT, &signal_cb, NULL },
	{ SIGTERM, &signal_cb, NULL }
};

static evt_ret_t signal_cb( evt_loop_t * const el,
							evt_t * const evt,
							evt_params_t * const params,
							void * user_data )
{
	switch( params->signal_params.signum )
	{
		case SIGINT:
			NOTICE( "received SIGINT\n" );
			break;
		case SIGTERM:
			NOTICE( "received SIGTERM\n" );
			break;
	}

	/* stop the event loop so we'll exit */
	NOTICE("stopping event loop\n");
	evt_stop( el );

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
	int len1, len2;
	llsd_t * tmp;

	CHECK_PTR_RET( config, FALSE );

	/* "directory" is the only mandatory setting */

	/* get the root dir value */
	tmp = llsd_map_find( config, "directory" );
	CHECK_PTR_RET( tmp, FALSE );
	root_dir = strdup( llsd_as_string( tmp ).str );
	NOTICE( "root directory: %s\n", root_dir );

	/* get the daemonize flag */
	tmp = llsd_map_find( config, "daemon" );
	if ( tmp != NULL )
	{
		do_daemon = llsd_as_bool( tmp );
	}
	NOTICE( "daemon: %s\n", (do_daemon ? "true" : "false") );

	/* get the pid file path */
	tmp = llsd_map_find( config, "pidfile" );
	if ( ( tmp != NULL ) && ( strlen( llsd_as_string( tmp ).str ) > 0 ) )
	{
		if ( llsd_as_string( tmp ).str[0] == '/' )
		{
			/* just dupe the absolute path to the pid file */
			pid_file = strdup( llsd_as_string( tmp ).str );
		}
		else
		{
			pid_file = build_absolute_path( root_dir, llsd_as_string( tmp ).str );
		}
		NOTICE( "pidfile: %s\n", pid_file );
	}

	/* get the start file path */
	tmp = llsd_map_find( config, "startfile" );
	if ( ( tmp != NULL ) && ( strlen( llsd_as_string( tmp ).str ) > 0 ) )
	{
		if ( llsd_as_string( tmp ).str[0] == '/' )
		{
			start_file = strdup( llsd_as_string( tmp ).str );
		}
		else
		{
			start_file = build_absolute_path( root_dir, llsd_as_string( tmp ).str );
		}
		NOTICE( "startfile: %s\n", start_file );
	}
	
	return TRUE;
}

int main(int argc, char** argv)
{
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

	if ( do_daemon )
	{
		daemonize();
	}

	/* clean up the file descriptors */
	sanitize_files();

	/* start the signal handlers */
	sigevt = new_signals_and_event_loop( NSIGS, asigs );

	/* blocking call to run event loop */
	evt_run( sigevt->el );

	/* stop the signal handlers */
	delete_signals_and_event_loop( sigevt );

	/* close the logging facility */
	stop_logging();

	/* clean up our memory */
	if ( config != NULL )
		llsd_delete( config );
	if ( root_dir != NULL )
		FREE( root_dir );
	if ( pid_file != NULL )
		FREE( pid_file );
	if ( start_file != NULL )
		FREE( start_file );

	return EXIT_SUCCESS;
}
