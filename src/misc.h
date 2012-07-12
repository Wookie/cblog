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

#ifndef MISC_H
#define MISC_H

#include <cutil/debug.h>
#include <cutil/macros.h>
#include <cutil/events.h>

typedef struct sig_callback_s
{
	int signum;
	evt_fn sighandler;
	void* user_data;
} sig_callback_t;

typedef struct evt_sigs_s
{
	evt_loop_t * el;
	int nevents;
	evt_t ** events;
} evt_sigs_t;

/* creates the event loop and hooks up signal handlers for the specified signals */
evt_sigs_t * new_signals_and_event_loop( int nsigs, sig_callback_t sigs[] );
int delete_signals_and_event_loop( evt_sigs_t * sigevt );

/* builds an absolute path */
int8_t * build_absolute_path( int8_t const * const root_dir, int8_t const * const rel_path );

#endif/*MISC_H*/

