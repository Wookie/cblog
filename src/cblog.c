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
#define MAX_PATH_LEN (1024)
static int do_daemon = FALSE;
static int8_t * root_dir = NULL;
static int8_t * pid_file = NULL;
static int8_t * start_file = NULL;
static llsd_t * log_config = NULL;
static log_t * log = NULL;

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
	int8_t * tmp_str;
	llsd_t * tmp;

	CHECK_PTR_RET( config, FALSE );

	/* "directory" is the only mandatory setting */

	/* get the root dir value */
	tmp = llsd_map_find( config, "directory" );
	if ( ( tmp != NULL ) && ( strlen( llsd_as_string( tmp ).str ) > 0 ) )
	{
		root_dir = strdup( llsd_as_string( tmp ).str );
	}
	else
	{
		root_dir = CALLOC( MAX_PATH_LEN, sizeof(int8_t) );
		getcwd( root_dir, MAX_PATH_LEN );
	}
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

	/* get the log config */
	tmp = llsd_map_find( config, "log" );
	if ( ( tmp != NULL ) && ( strlen( llsd_as_string( tmp ).str ) > 0 ) )
	{
		/* get the name of the logging config to look up */
		tmp_str = strdup( llsd_as_string( tmp ).str );

		/* get the log configs */
		tmp = llsd_map_find( tmp, "logs" );
		if ( tmp != NULL )
		{
			log_config = llsd_map_find( tmp, tmp_str );
		}

		FREE( tmp_str );
	}
	else
	{
		/* build a default config */
		log_config = llsd_new_map(1);

		/* { 'type': 'syslog', 'ident': 'cblog' } */
		llsd_map_insert( log_config,
						 llsd_new_string( "type", 4, FALSE, FALSE ),
						 llsd_new_string( "file", 4, FALSE, FALSE ) );
		llsd_map_insert( log_config, 
						 llsd_new_string( "ident", 5, FALSE, FALSE ), 
						 llsd_new_string( "cblog", 5, FALSE, FALSE ) );
	}
	
	return TRUE;
}

static int init_log( void )
{
	llsd_t * tmp = NULL;
	log_type_t type = -1;
	uint8_t * param = NULL;
	int append = FALSE;

	tmp = llsd_map_find( log_config, "type" );
	CHECK_PTR_RET( tmp, FALSE );

	/* figure out the log type */
	if ( strcmp( llsd_as_string( tmp ).str, "file" ) == 0 )
		type = LOG_TYPE_FILE;
	else if ( strcmp( llsd_as_string( tmp ).str, "syslog" ) == 0 )
		type = LOG_TYPE_SYSLOG;
	else
	{
		WARN( "invalid logging facility type\n" );
		return FALSE;
	}

	switch ( type )
	{
		case LOG_TYPE_FILE:
			tmp = llsd_map_find( log_config, "name" );
			CHECK_PTR_RET_MSG( tmp, FALSE, "failed to get log file name\n" );
			param = build_absolute_path( root_dir, llsd_as_string( tmp ).str );
			tmp = llsd_map_find( log_config, "append" );
			if ( tmp != NULL )
				append = llsd_as_bool( tmp );
			break;

		case LOG_TYPE_SYSLOG:
			tmp = llsd_map_find( log_config, "ident" );
			CHECK_PTR_RET_MSG( tmp, FALSE, "failed to get syslog ident\n" );
			param = strdup( llsd_as_string( tmp ).str );
			break;
	}

	log = start_logging( type, param, append );

	if ( log == NULL )
	{
		WARN( "failed to start logging facility\n" );
		FREE( param );
		return FALSE;
	}

	return TRUE;
}

static int deinit_log( void )
{
	CHECK_PTR_RET( log, FALSE );
	stop_logging( log );
}

int main(int argc, char** argv)
{
	/* TODO: build a clean environment */

	/* parse options and load config file */
	CHECK_RET( parse_options( argc, argv ), EXIT_FAILURE );
	CHECK_RET( load_config(), EXIT_FAILURE );

	/* we should have a loaded config */
	CHECK_RET( get_globals(), EXIT_FAILURE );
	
	/* start syslog logging */
	init_log();

	/* banner the syslog */
	NOTICE( "%s\n", VERSION_STRING );

	/* daemonized if we need to */
	if ( do_daemon )
	{
		daemonize();
	}

	/* create the pid/start files */
	create_pid_file( pid_file );
	create_start_file( start_file );

	/* clean up the file descriptors */
	sanitize_files();

	/* start the signal handlers */
	sigevt = new_signals_and_event_loop( NSIGS, asigs );

	/* blocking call to run event loop */
	evt_run( sigevt->el );

	/* stop the signal handlers */
	delete_signals_and_event_loop( sigevt );

	/* close the logging facility */
	deinit_log();

	/* clean up the pid/start files */
	NOTICE("unlinking: %s\n", pid_file );
	unlink( pid_file );
	NOTICE("unlinking: %s\n", start_file );
	unlink( start_file );

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
