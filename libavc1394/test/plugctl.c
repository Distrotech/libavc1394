/* plugctl.c v0.1
 * A program to read and manipulate MPR/PCR registers.
 * Copyright 2002 Dan Dennedy
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* standard system includes */
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>

/* linux1394 includes */
#include <libraw1394/raw1394.h>
#include <libplug1394/plug1394.h>

#define FAIL(s) {fprintf(stderr, "libplug1394 error: %s\n", s);return(-1);}

#define RAW1394_PORT 0
int g_is_done = 0;

raw1394handle_t open_raw1394(void)
{
    int numcards;
    struct raw1394_portinfo pinf[16];
    raw1394handle_t handle;

    if (!(handle = raw1394_new_handle())) {
            perror("raw1394 - couldn't get handle");
    printf("This error usually means that the ieee1394 driver is not loaded or that /dev/raw1394 does not exist.\n");
            exit( -1);
    }

    if ((numcards = raw1394_get_port_info(handle, pinf, 16)) < 0) {
            perror("raw1394 - couldn't get card info");
            exit( -1);
    }

    /* port 0 is the first host adapter card */
    if (raw1394_set_port(handle, RAW1394_PORT) < 0) {
            perror("raw1394 - couldn't set port");
            exit( -1);
    }
    
    return handle;
}


void close_raw1394(raw1394handle_t handle)
{
    raw1394_destroy_handle(handle);
}

/* this is a common unix function that gets called when a process 
   receives a signal (e.g. ctrl-c) */
void signal_handler(int sig)
{
    /* replace this signal handler with the default (which aborts) */
    signal(SIGINT, SIG_IGN);
	g_is_done = 1;
}

int main(int argc, const char** argv)
{
    raw1394handle_t handle;
	int device;
	struct oMPR o_mpr;
	struct oPCR o_pcr;
	struct iMPR i_mpr;
	struct iPCR i_pcr;

        
    signal(SIGINT, signal_handler);

    handle = open_raw1394();
	if (argc < 2) exit(0);
	if (argc > 2) device = 0xffc0 | atoi(argv[1]);
//	device = raw1394_get_local_id(handle);

	if (argc > 2 && strcasecmp(argv[2], "start_tx") == 0)
	{
		if (plug1394_get_oMPR(handle, device, &o_mpr) <  0)
			FAIL("error reading oMPR")
		else
		{
			printf( "oMPR .plugs=%d .bcast_channel=%d\n", o_mpr.n_plugs, o_mpr.bcast_channel);
			o_mpr.bcast_channel = 63;
			if (plug1394_set_oMPR(handle, device, o_mpr) < 0)
				FAIL("error setting oMPR")
		}

		if (plug1394_get_oPCR0( handle, device, &o_pcr) < 0)
			FAIL("error reading oPCR")
		else
		{
			printf( "oPCR .online=%d .bcast_connection=%d .n_p2p_connections=%d\n", 
				o_pcr.online, o_pcr.bcast_connection, o_pcr.n_p2p_connections);
			printf( "oPCR .data_rate=%d .overhead_id==480?%d .channel=%d .payload=%d\n", 
				o_pcr.data_rate, (o_pcr.overhead_id == OVERHEAD_480), o_pcr.channel, o_pcr.payload);
			o_pcr.bcast_connection=1;
			if (plug1394_set_oPCR0( handle, device, o_pcr) < 0)
				FAIL("error setting oPCR")
			
			if (plug1394_get_oPCR0( handle, device, &o_pcr) < 0)
				FAIL("error reading oPCR")
			else
			{
				printf( "oPCR .online=%d .bcast_connection=%d .n_p2p_connections=%d\n", 
					o_pcr.online, o_pcr.bcast_connection, o_pcr.n_p2p_connections);
				printf( "oPCR .data_rate=%d .overhead_id==480?%d .channel=%d\n", 
					o_pcr.data_rate, (o_pcr.overhead_id == OVERHEAD_480), o_pcr.channel);
			}
		}
	}
	else if (argc > 2 && strcasecmp(argv[2], "stop_tx") == 0)
	{
		plug1394_get_oPCR0( handle, device, &o_pcr);
		o_pcr.bcast_connection=0;
		plug1394_set_oPCR0( handle, device, o_pcr);
		plug1394_get_oPCR0( handle, device, &o_pcr);
		printf( "oPCR .online=%d .bcast_connection=%d .n_p2p_connections=%d\n", 
			o_pcr.online, o_pcr.bcast_connection, o_pcr.n_p2p_connections);
		printf( "oPCR .data_rate=%d .overhead_id==480?%d .channel=%d\n", 
			o_pcr.data_rate, (o_pcr.overhead_id == OVERHEAD_480), o_pcr.channel);
	}
	else if (argc > 2 && strcasecmp(argv[2], "start_rx") == 0) 
	{
		plug1394_get_iMPR( handle, device, &i_mpr);
		printf( "iMPR .plugs=%d .data_rate=%d\n", i_mpr.n_plugs, i_mpr.data_rate);

		plug1394_get_iPCR0( handle, device, &i_pcr);
		printf( "iPCR .channel=%d .bcast_connection=%d .n_p2p_connections=%d\n", 
			i_pcr.channel, i_pcr.bcast_connection, i_pcr.n_p2p_connections);
		i_pcr.channel = i_pcr.n_p2p_connections;
		i_pcr.n_p2p_connections ++;
		plug1394_set_iPCR0( handle, device, i_pcr);
		
		plug1394_get_iPCR0( handle, device, &i_pcr);
		printf( "iPCR .channel=%d .bcast_connection=%d .n_p2p_connections=%d\n", 
			i_pcr.channel, i_pcr.bcast_connection, i_pcr.n_p2p_connections);
	} 
	else if (argc > 2 && strcasecmp( argv[2], "stop_rx") == 0) 
	{
		plug1394_get_iPCR0( handle, device, &i_pcr);
		printf( "iPCR .channel=%d .bcast_connection=%d .n_p2p_connections=%d\n", 
			i_pcr.channel, i_pcr.bcast_connection, i_pcr.n_p2p_connections);

		i_pcr.n_p2p_connections --;
		plug1394_set_iPCR0( handle, device, i_pcr);

		plug1394_get_iPCR0( handle, device, &i_pcr);
		printf( "iPCR .channel=%d .bcast_connection=%d .n_p2p_connections=%d\n", 
			i_pcr.channel, i_pcr.bcast_connection, i_pcr.n_p2p_connections);
	}
	else if (strcasecmp(argv[1], "host") == 0)
	{
		struct pollfd raw1394_poll;
		raw1394_poll.fd = raw1394_get_fd(handle);
		raw1394_poll.events = POLLIN;

		o_mpr.n_plugs = 1;
		o_mpr.data_rate = DATA_RATE_100;
		o_mpr.bcast_channel = 63;
		o_pcr.online = 1;
		o_pcr.bcast_connection = 0;
		o_pcr.n_p2p_connections = 0;
		o_pcr.data_rate = DATA_RATE_100;
		o_pcr.overhead_id = OVERHEAD_480;
		o_pcr.channel = 63;
		/* DV contains 120 quadlets for DIF block plus 2 quadlet of CIP header */
		o_pcr.payload = 122;
		plug1394_register_outputs( handle, o_mpr, o_pcr);
		
		i_mpr.n_plugs = 1;
		i_mpr.data_rate = DATA_RATE_100;
		i_pcr.online = 1;
		i_pcr.bcast_connection = 0;
		i_pcr.n_p2p_connections = 0;
		i_pcr.channel = 63;
		plug1394_register_inputs( handle, i_mpr, i_pcr);
		
		while (g_is_done != 1)
		{
			if ( poll( &raw1394_poll, 1, 1000) > 0 )
			{
				if ( (raw1394_poll.revents & POLLIN) 
						|| (raw1394_poll.revents & POLLPRI) )
					raw1394_loop_iterate(handle);
			}

		}
		plug1394_unregister_outputs( handle);
		plug1394_unregister_inputs( handle);
	}
	
    close_raw1394(handle);
    exit(0);
}


