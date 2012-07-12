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

#include "misc.h"

evt_sigs_t * new_signals_and_event_loop( int nsigs, sig_callback_t sigs[] )
{
	int i;
	evt_params_t evt_params;
	evt_sigs_t * evtsigs = NULL;

	evtsigs = (evt_sigs_t*)CALLOC( 1, sizeof( evt_sigs_t ) );
	CHECK_PTR_RET( evtsigs, NULL );

	/* create the event loop */
	evtsigs->el = evt_new();

	if ( nsigs > 0 )
	{
		/* allocate the array of evt_t pointers */
		evtsigs->events = (evt_t**)CALLOC( nsigs, sizeof(evt_t*));
		
		if ( evtsigs->events == NULL )
		{
			evt_delete( evtsigs->el );
			FREE( evtsigs );
			return FALSE;
		}

		/* remember how many event handlers we have */
		evtsigs->nevents = nsigs;

		for ( i = 0; i < nsigs; i++ )
		{
			/* create signal handler */
			MEMSET( &evt_params, 0, sizeof(evt_params_t) );
			evt_params.signal_params.signum = sigs[i].signum;
			evtsigs->events[i] = evt_new_event_handler( EVT_SIGNAL, 
														&evt_params, 
														sigs[i].sighandler, 
														sigs[i].user_data );

			/* start the event handler */
			evt_start_event_handler( evtsigs->el, evtsigs->events[i] );
		}
	}

	return evtsigs;
}

int delete_signals_and_event_loop( evt_sigs_t * sigevt )
{
	int i;
	CHECK_PTR_RET( sigevt, FALSE );

	for ( i = 0; i < sigevt->nevents; i++ )
	{
		/* stop the event handler */
		evt_stop_event_handler( sigevt->el, sigevt->events[i] );

		/* delete the event handler */
		evt_delete_event_handler( (void*)sigevt->events[i] );
	}

	/* clean up the event loop */
	evt_delete( sigevt->el );

	/* clean up the memory */
	FREE( sigevt->events );
	FREE( sigevt );

	return TRUE;
}

int8_t * build_absolute_path( int8_t const * const root_dir, int8_t const * const rel_path )
{
	int8_t * abs_path = NULL;
	int len1, len2;
	CHECK_PTR_RET( root_dir, NULL );
	CHECK_PTR_RET( rel_path, NULL );

	len1 = strlen( root_dir );
	len2 = strlen( rel_path );

	CHECK_RET( len1 > 0, NULL );
	CHECK_RET( len2 > 0, NULL );

	abs_path = CALLOC( len1 + len2 + 2, sizeof(int8_t) );
	MEMCPY( abs_path, root_dir, len1 );
	abs_path[len1] = '/';
	MEMCPY( &abs_path[len1+1], rel_path, len2 );
	abs_path[len1+len2+1] = '\0';

	return abs_path;
}

