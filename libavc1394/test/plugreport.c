/* plugreport.c v0.1
 * A program to read all MPR/PCR registers from all devices and report them.
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
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* linux1394 includes */
#include <libraw1394/raw1394.h>
#include <libplug1394/plug1394.h>

#define FAIL(s) {fprintf(stderr, "libplug1394 error: %s\n", s);}

int main(int argc, const char** argv)
{
    raw1394handle_t handle;
	int device;
	struct oMPR o_mpr;
	struct oPCR o_pcr;
	struct iMPR i_mpr;
	struct iPCR i_pcr;
    int numcards, port, i;
    struct raw1394_portinfo pinf[16];
        
	if (!(handle = raw1394_new_handle())) {
		perror("raw1394 - couldn't get handle");
		printf("This error usually means that the ieee1394 driver is not loaded or that /dev/raw1394 does not exist.\n");
		exit( -1);
	}

	if ((numcards = raw1394_get_port_info(handle, pinf, 16)) < 0) {
		perror("raw1394 - couldn't get card info");
		exit( -1);
	}

	for (port = 0; port < numcards; port++)
	{
		if (raw1394_set_port(handle, port) < 0) {
			perror("raw1394 - couldn't set port");
			exit( -1);
		}
		
		printf( "Host Adapter %d\n==============\n\n", port);
    
		for (device = 0; device < raw1394_get_nodecount(handle); device++ )
		{
			
			printf( "Node %d\n-------\n", device);
			
			if (plug1394_get_oMPR(handle, 0xffc0 | device, &o_mpr) <  0)
				FAIL("error reading oMPR")
			else
			{
				printf( "oMPR #plugs=%d, data rate=%d, broadcast channel=%d\n", o_mpr.n_plugs, i_mpr.data_rate, o_mpr.bcast_channel);
				for (i = 0; i < o_mpr.n_plugs; i++)
				{
					if (plug1394_get_oPCRX( handle, 0xffc0 | device, &o_pcr, i) < 0)
						FAIL("error reading oPCR")
					else
					{
						printf( "oPCR[%d] online=%d, #bcast connections=%d, #p2p connections=%d\n", i,
							o_pcr.online, o_pcr.bcast_connection, o_pcr.n_p2p_connections);
						printf( "\tchannel=%d, data rate=%d, overhead id=%d, payload=%d\n", 
							o_pcr.channel, o_pcr.data_rate, o_pcr.overhead_id, o_pcr.payload);
					}
				}
			}
	
			if (plug1394_get_iMPR(handle, 0xffc0 | device, &i_mpr) <  0)
				FAIL("error reading iMPR")
			else
			{
				printf( "iMPR #plugs=%d, data rate=%d\n", i_mpr.n_plugs, i_mpr.data_rate);
				for (i = 0; i < i_mpr.n_plugs; i++)
				{
					if (plug1394_get_iPCRX( handle, 0xffc0 | device, &i_pcr, i) < 0)
						FAIL("error reading iPCR")
					else
					{
						printf( "iPCR[%d] online=%d, #bcast connections=%d, #p2p connections=%d\n", i,
							i_pcr.online, i_pcr.bcast_connection, i_pcr.n_p2p_connections);
						printf( "\tchannel=%d\n", 
							i_pcr.channel);
					}
				}
			}
		}
		printf("\n");
		
		raw1394_destroy_handle(handle);
		
		if (!(handle = raw1394_new_handle())) {
			perror("raw1394 - couldn't get handle");
			printf("This error usually means that the ieee1394 driver is not loaded or that /dev/raw1394 does not exist.\n");
			exit( -1);
		}

		if ((numcards = raw1394_get_port_info(handle, pinf, 16)) < 0) {
			perror("raw1394 - couldn't get card info");
			exit( -1);
		}

	}
    exit(0);
}


