/* dvcont - A DV Camera Console Control Program
 * By: Jason Howard <jason@spectsoft.com>
 * http://www.spectsoft.com
 * Code Started January 1, 2001.
 * Version 0.1
 * Adapted to libavc1394 by Dan Dennedy <dan@dennedy.org>
 * Timeout handling added by Bevis King
 * Many commands since 0.04 added by Dan Dennedy
 *
 * This code is based off a GNU project called gscanbus by Andreas Micklei
 * -- Thanks Andreas for the great code! --
 * 
 * Thanks to Dan Dennedy for patching this version to get it to work
 * with his camera.  (Panasonic PV-DV910) 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

// Load up the needed includes.
#include <config.h>
#include <libavc1394/avc1394.h>
#include <libavc1394/avc1394_vcr.h>
#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define TYPEREQ_TIMEOUT	5
extern void timeout_happened();
int timeout=0;

int i;

#define version "Version 0.1"


void show_help() {

printf("\n--- DVCONT HELP ---");
printf("\n\nUsage: dvcont <command>");
printf("\n\nCommands:");
printf("\nplay - Tell the camera to play (or toggle slow-mo)");
printf("\nreverse - Tell the camera to play in reverse (or toggle reverse slow-mo)");
printf("\ntrickplay - Tell the camera to play back at -14 to +14 (not supported by all cams)");
printf("\nstop - Tell the camera to stop");
printf("\nrewind - Tell the camera to rewind (stop or play mode)");
printf("\nff - Tell the camera to fast forward (stop or play mode)");
printf("\npause - Tell the camera to toggle pause play");
printf("\nnext - Tell the camera to go to the next frame (pause mode)");
printf("\nnextindex - Tell the camera to go to the next index point and pause");
printf("\nprev - Tell the camera to go to the previous frame (pause mode)");
printf("\nprevindex - Tell the camera to go to the previous index point and pause");
printf("\nrecord - Tell the camera to record (use with caution!)");
printf("\neject - Tell the camera to eject the tape (awe your friends!)");
printf("\ntimecode - Report the timecode from the tape (HH:MM:SS:FF)");
printf("\nstatus - Report the status of the device");
printf("\nverbose - Tell the program to tell you debug info.");
printf("\nversion - Tell the program to tell you the program version.");
printf("\nhelp - Tell the program to show you this screen");
printf("\ndev <number> - Select device number on chain to use.");
printf("\n               (use the dev command BEFORE any other commands)");
printf("\n\n");

}


int main (int argc, char *argv[])
{
	raw1394handle_t handle;
	quadlet_t quadlet;	
	int device;
	int retcode;
	int verbose;
	int speed;
	struct sigaction act;
	char *timecode;

	// Declare some default values.
	
	device = 1;
	verbose = 0;
#ifdef RAW1394_V_0_8
	handle = raw1394_get_handle();
#else
    handle = raw1394_new_handle();
#endif
        if (!handle) {
                if (!errno) {
                        printf("Not Compatable!\n");
                } else {
                        printf("\ncouldn't get handle\n");
                        printf("Not Loaded!\n");
                }
                exit(1);
        } 


	if (raw1394_set_port(handle, 0) < 0) {
		perror("couldn't set port");
		exit(1);
	}

		
	if (raw1394_read(handle, 0xffc0 | raw1394_get_local_id(handle),
		CSR_REGISTER_BASE + CSR_CYCLE_TIME, 4,
		(quadlet_t *) &quadlet) < 0) {
		fprintf(stderr,"something is wrong here\n");
	}

#ifdef SA_RESTART
	act.sa_handler = timeout_happened;
	sigemptyset(&(act.sa_mask));
	act.sa_flags = 0;
	sigaction( SIGALRM, &act, NULL );
#else
	fprintf( stderr, 
		"can't stop system call from restarting - warnings only\n");
	signal( SIGALRM, timeout_happened );
#endif

    	for (i=0; i < raw1394_get_nodecount(handle); ++i)
	{
		timeout=0;
		alarm( TYPEREQ_TIMEOUT );

		retcode= avc1394_check_subunit_type(handle, i,
						AVC1394_SUBUNIT_TYPE_VCR);
		if( timeout >= 1 )
			retcode = 0;  /* assume things failed */
		if( retcode == 1 )
		{
		    device = i;
		    break;
		}
		alarm( 0 );
	}
	signal( SIGALRM, SIG_DFL );

	for (i = 1; i < argc; ++i) {
        
        if (strcmp("play", argv[i]) == 0) {
            avc1394_vcr_play(handle, device);
          
	    } else if (strcmp("reverse", argv[i]) == 0) {
            avc1394_vcr_reverse(handle, device);
	    
	    } else if (strcmp("trickplay", argv[i]) == 0) {
	        if (i+1 < argc) {
                speed = atoi(argv[(i+1)]);
                avc1394_vcr_trick_play(handle, device, speed);
            }
	    
	    } else if (strcmp("stop", argv[i]) == 0) {
            avc1394_vcr_stop(handle, device);
	    
	    } else if (strcmp("rewind", argv[i]) == 0) {
		avc1394_vcr_rewind(handle, device);
	    
	    } else if (strcmp("ff", argv[i]) == 0) {
		avc1394_vcr_forward(handle, device);
	    
	    } else if (strcmp("pause", argv[i]) == 0) {
		avc1394_vcr_pause(handle, device);
	    
	    } else if (strcmp("record", argv[i]) == 0) {
		avc1394_vcr_record(handle, device);
	    
	    } else if (strcmp("eject", argv[i]) == 0) {
		avc1394_vcr_eject(handle, device);
	    
	    } else if (strcmp("status", argv[i]) == 0) {
		printf( "%s\n", avc1394_vcr_decode_status(avc1394_vcr_status(handle, device)));
	    
	    } else if (strcmp("timecode", argv[i]) == 0) {
	        if ( (timecode = avc1394_vcr_get_timecode(handle, device)) != NULL) {
        		printf( "%s\n", timecode);
        		free(timecode);
    		}
	    
	    } else if (strcmp("version", argv[i]) == 0) {
		printf("\nDV Camera Console Control Program\n%s\nBy: Jason Howard, Dan Dennedy, and Andreas Micklei\n", version);
	    
	    } else if (strcmp("verbose", argv[i]) == 0) {
		printf("successfully got handle\n");
      		printf("current generation number: %d\n", raw1394_get_generation(handle));
		printf("using first card found: %d nodes on bus, local ID is %d\n",
		raw1394_get_nodecount(handle),
		raw1394_get_local_id(handle) & 0x3f);
	    	verbose = 1;

	    }	else if (strcmp("next", argv[i]) == 0) {
		avc1394_vcr_next(handle, device);
	    
	    }	else if (strcmp("nextindex", argv[i]) == 0) {
		avc1394_vcr_next_index(handle, device);
	    
	    }	else if (strcmp("prev", argv[i]) == 0) {
		avc1394_vcr_previous(handle, device);
	    
	    }	else if (strcmp("previndex", argv[i]) == 0) {
		avc1394_vcr_previous_index(handle, device);
	    
	    }	else if (strcmp("help", argv[i]) == 0) {
		show_help ();
	    
	    }   else if (strcmp("dev", argv[i]) == 0) {
		
		device = atoi(argv[(i+1)]);
		
		if (verbose == 1) {
			printf("\nUsing Device: %d\n", device);
		}

	    }
	
	}
		
	return 0;
}

void timeout_happened()
{
	fprintf( stderr, "request to node %d about it's type timed out\n",
			i );
	timeout++;

	alarm( 5 );
	return;
}
