/*
 * panelctl - Control a Motorola DCT 6200/6400 series settop box
 *
 * Copyright (C) 2004-2006 by Stacey D. Son <mythdev@son.org>,
 * John Woodell <woodie@netpress.com>, and Dan Dennedy <dan@dennedy.org>
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

#include <config.h>

#include "../librom1394/rom1394.h"
#include "../libavc1394/avc1394.h"

#include <libraw1394/raw1394.h>
#include <sys/types.h>
#include <stdio.h>
#include <argp.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

// Motorola DCT-6x00 IDs
#define MOTDCT_SPEC_ID    0x00005068
#define MOTDCT_SW_VERSION 0x00010101

const char *argp_program_version =
"panelctl  0.2";

char *input;            /* the argument passed to the program */
int verbose;              /* The -v flag */
int debug;                /* -d flag */
int printcommands;	/* -c flag */
unsigned ctl_guid;		    /* non-zero if -g flag is specified */
unsigned ctl_spec_id;
unsigned ctl_sw_version;

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC}.
*/
static struct argp_option options[] =
{
	{"verbose", 	'v', 0, 0, "Produce verbose output"},
	{"debug",   	'd', 0, 0, "Debug mode"},
	{"commands",	'c', 0, 0, "Print command list (requires a dummy argument)"},
	{"guid",   	'g', "GUID", 0, "Specify GUID for the STB to control"},
	{"specid",  	's', "SPEC_ID", 0, "Specify spec_id of STB to control"},
	{"swversion", 	'n', "SW_VERSION", 0, "Specify sofware version of STB"},
	{0}
};


/*
   PARSER. Field 2 in ARGP.
   Order of parameters: KEY, ARG, STATE.
*/
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
	switch (key)
		{
		case 'v':
			verbose = 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 'c' :
			printcommands = 1;
			break;
		case 'g':
			sscanf (arg, "%x", &ctl_guid);
			break;
		case 's':
			sscanf (arg, "%x", &ctl_spec_id);
			break;
		case 'n' :
			sscanf (arg, "%x", &ctl_sw_version);
			break;
		case ARGP_KEY_ARG:
			if (state->arg_num != 0)
				{
				argp_usage(state);
				}
			input = arg;
			break;
		case ARGP_KEY_END:
			if (state->arg_num != 1)
				{
				argp_usage (state);
				}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
		}
	return 0;
}

/*
   ARGS_DOC. Field 3 in ARGP.
   A description of the non-option command-line arguments
     that we accept.
*/
static char args_doc[] = "<channel|command>";

/*
  DOC.  Field 4 in ARGP.
  Program documentation.
*/
static char doc[] =
"AV/C panelctl - change channels on, or issue commands to, a firewire AV device\v\
This program is mostly useful for a firewire tuner or set-top box with an AV interface. \
Use it to issue a command (panelctl <command>) or to change channels on the tuner (panelctl <channel>). \
\nTo get a list of legal commands, use the --commands switch. \
\n\n\
By default, panelctl will control the first Motorola STB on the firewire chain. This will only work \
with some Motorola STBs. To control any other STB, or to control multiple STBs, specify the GUID or both the \
spec_id and software version for the desired \
STB. This can be found out by running \"panelctl -v -g 1 1\". Because there won\'t be a STV with GUID of 1, \
it will run through all possible devices and print the info for each one. Once the GUID, or the spec_id and sw_version of the \
desired device has been learned, it can be used in following commands, e.g. \"panelctl -g 0x123456 666\". Generally, it will \
be a better approach to use guid, since this will be unique to each STB. \
\n\n\
By: Stacey D. Son, John Woodell, Dan Dennedy, and Jerry Fiddler\n\
Copyright (C) 2004-2009\n";

/*
   The ARGP structure itself.
*/
static struct argp argp = {options, parse_opt, args_doc, doc};

#define CTL_CMD_PRESS AVC1394_CTYPE_CONTROL | AVC1394_SUBUNIT_TYPE_PANEL | \
        AVC1394_SUBUNIT_ID_0 | AVC1394_PANEL_COMMAND_PASS_THROUGH | \
        AVC1394_PANEL_OPERAND_PRESS

#define CTL_CMD_RELEASE AVC1394_CTYPE_CONTROL | AVC1394_SUBUNIT_TYPE_PANEL | \
        AVC1394_SUBUNIT_ID_0 | AVC1394_PANEL_COMMAND_PASS_THROUGH | \
        AVC1394_PANEL_OPERAND_RELEASE

#define CTL_CMD_CHANNEL AVC1394_CTYPE_CONTROL | AVC1394_SUBUNIT_TYPE_PANEL | \
        AVC1394_SUBUNIT_ID_0 | AVC1394_PANEL_COMMAND_PASS_THROUGH | AVC1394_PANEL_OPERATION_0

struct lookup_table_t
{
	char *string;
	int value;
	char *desc;
};

#define UNKNOWN -1

struct lookup_table_t command_table[] = {
{ "ok",           AVC1394_PANEL_OPERATION_SELECT, "Select the highlighted item" },
{ "select",       AVC1394_PANEL_OPERATION_SELECT, "" },
{ "up",           AVC1394_PANEL_OPERATION_UP, "Move up in the menu or guide" },
{ "down",         AVC1394_PANEL_OPERATION_DOWN, "Move down in the menu or guide" },
{ "left",         AVC1394_PANEL_OPERATION_LEFT, "Move left in the menu or guide" },
{ "right",        AVC1394_PANEL_OPERATION_RIGHT, "Move right in the menu or guide" },
{ "menu",         AVC1394_PANEL_OPERATION_ROOT_MENU, "Enter or Exit the Main Menu" },
{ "lock",         AVC1394_PANEL_OPERATION_SETUP_MENU, "Bring up Parental Control screen" },
{ "guide",        AVC1394_PANEL_OPERATION_CONTENTS_MENU, "Bring up Listings By Time screen" },
{ "favorite",     AVC1394_PANEL_OPERATION_FAVORITE_MENU, "Scan through just your favorite channels" },
{ "exit",         AVC1394_PANEL_OPERATION_EXIT, "Return to live TV from the menu or guide" },
{ "num0",         AVC1394_PANEL_OPERATION_0, "" },
{ "num1",         AVC1394_PANEL_OPERATION_1, "" },
{ "num2",         AVC1394_PANEL_OPERATION_2, "" },
{ "num3",         AVC1394_PANEL_OPERATION_3, "" },
{ "num4",         AVC1394_PANEL_OPERATION_4, "" },
{ "num5",         AVC1394_PANEL_OPERATION_5, "" },
{ "num6",         AVC1394_PANEL_OPERATION_6, "" },
{ "num7",         AVC1394_PANEL_OPERATION_7, "" },
{ "num8",         AVC1394_PANEL_OPERATION_8, "" },
{ "num9",         AVC1394_PANEL_OPERATION_9, "" },
{ "enter",        AVC1394_PANEL_OPERATION_ENTER, "" },
{ "music",        AVC1394_PANEL_OPERATION_ENTER, "Enter the digital Music menu" },
{ "channelup",    AVC1394_PANEL_OPERATION_CHANNEL_UP, "Change channel up" },
{ "channeldown",  AVC1394_PANEL_OPERATION_CHANNEL_DOWN, "Change channel down" },
{ "last",         AVC1394_PANEL_OPERATION_PREVIOUS_CHANNEL, "Return to the previous menu or channel" },
{ "previous",     AVC1394_PANEL_OPERATION_PREVIOUS_CHANNEL, "" },
{ "info",         AVC1394_PANEL_OPERATION_DISPLAY_INFO, "See a description of the current show" },
{ "display",      AVC1394_PANEL_OPERATION_DISPLAY_INFO, "" },
{ "help",         AVC1394_PANEL_OPERATION_HELP, "See helpful information" },
{ "pageup",       AVC1394_PANEL_OPERATION_PAGE_UP, "Move up one page in the menu or guide" },
{ "pagedown",     AVC1394_PANEL_OPERATION_PAGE_DOWN, "Move down one page in the menu or guide" },
{ "power",        AVC1394_PANEL_OPERATION_POWER, "Toggle the device on or off" },
{ "volumeup",     AVC1394_PANEL_OPERATION_VOLUME_UP, "Change volume up" },
{ "volumedown",   AVC1394_PANEL_OPERATION_VOLUME_DOWN, "Change volume down" },
{ "mute",         AVC1394_PANEL_OPERATION_MUTE, "Toggle sound on or off" },
{ "play",         AVC1394_PANEL_OPERATION_PLAY, "Play DVR or On-Demand content" },
{ "stop",         AVC1394_PANEL_OPERATION_STOP, "Stop DVR or On-Demand content" },
{ "pause",        AVC1394_PANEL_OPERATION_PAUSE, "Pause DVR or On-Demand content" },
{ "record",       AVC1394_PANEL_OPERATION_RECORD, "Record content on the DVR" },
{ "save",         AVC1394_PANEL_OPERATION_RECORD, "" },
{ "rewind",       AVC1394_PANEL_OPERATION_REWIND, "Rewind DVR or On-Demand content" },
{ "reverse",      AVC1394_PANEL_OPERATION_REWIND, "" },
{ "fastforward",  AVC1394_PANEL_OPERATION_FASTFORWARD, "Fast Forward DVR or On-Demand content" },
{ "forward",      AVC1394_PANEL_OPERATION_FASTFORWARD, "" },
{ "ff",           AVC1394_PANEL_OPERATION_FASTFORWARD, "" },
{ "dayback",      0x64, "" },
// dayforward     ?
{ "soundoff",      0x65, "Turn sound off" },
{ "soundon",     0x66, "Turn sound on" },
{ 0,           UNKNOWN, "" }
};

void two_col(char *cmd, char *desc)
{
	if (strlen(desc)) {
		printf("  %s", cmd);
		int n;
	        for (n = 16; n > strlen(cmd); --n) { printf(" "); }
		printf(" %s\n", desc);
	}
}

void status()
{
	printf("Unknown\n");
	exit(1);
}


void doprintcommands()
{
	printf("Panelctl commands:\n"
		);
	int i;
	for (i = 0; 0 != command_table[i].string; ++i) {
		two_col(command_table[i].string, command_table[i].desc);
	}
	printf("  num0 - num9      Emulate a number key pressed\n");
	exit(1);
}

int main (int argc, char *argv[])
{
	rom1394_directory dir;
	int device = UNKNOWN;

	quadlet_t cmd[2];
	int channel;
	int guid;


	/* Set default argument defaults */
	input = NULL;
	verbose = 0;
	ctl_guid = 0;
	ctl_spec_id = MOTDCT_SPEC_ID;
	ctl_sw_version =  MOTDCT_SW_VERSION;

	/* Get the switches and argument */
	argp_parse (&argp, argc, argv, 0, 0, 0);

	if (printcommands) {
		doprintcommands ();
		exit (0);
		}

	if (debug != 0) {
		/* Print argument values */
		printf ("verbose = %d.  guid = 0x%x\n", verbose, ctl_guid);
		printf ("ARG = %s\n", input);
		}

	raw1394handle_t handle = raw1394_new_handle_on_port(0);

	if (!handle) {
		if (!errno) {
			fprintf(stderr, "Not Compatable!\n");
		} else {
			perror("Could not get 1394 handle");
			fprintf(stderr, "Is ieee1394, driver, and raw1394 loaded?\n");
		}
		exit(1);
	}

	int nc = raw1394_get_nodecount(handle);
	int i;
	for (i = 0; i < nc; ++i) {
		if (rom1394_get_directory(handle, i, &dir) < 0) {
			fprintf(stderr,"error reading config rom directory for node %d\n", i);
			continue;
		}
		guid = rom1394_get_guid(handle, i);
		if (verbose)
			printf("node %d: vendor_id=0x%x, model_id=0x%x, spec_id=0x%x, sw_version=0x%x, node_capabilities=0x%x, guid=0x%x.\n",
			       i, dir.vendor_id, dir.model_id, dir.unit_spec_id, dir.unit_sw_version, dir.node_capabilities, guid);

		if (guid == ctl_guid) {
			device = i;
			break;
		}

		if ( ctl_guid == 0 && dir.unit_spec_id == ctl_spec_id &&
		        dir.unit_sw_version == ctl_sw_version) {
			device = i;
			break;
		}
	}

	if (device == UNKNOWN) {
		fprintf(stderr, "Could not find device on the 1394 bus.\n");
		raw1394_destroy_handle(handle);
		exit(1);
	}

	channel = atoi(input);
	if ( channel )
	{
		int digit[3];
		int i;
		
		digit[2] = (channel % 10);
		digit[1] = (channel % 100)  / 10;
		digit[0] = (channel % 1000) / 100;
		
		if (verbose)
			printf ("Changing to channel %d on node %d.\n", channel, device);
		if (debug)
			printf("AV/C Command: %d%d%d = Op1=0x%08X Op2=0x%08X Op3=0x%08X\n", 
				digit[0], digit[1], digit[2], 
				CTL_CMD_CHANNEL | digit[0],
				CTL_CMD_CHANNEL | digit[1], 
				CTL_CMD_CHANNEL | digit[2]);
		
		for (i=0; i < 3; i++) {
			cmd[0] = CTL_CMD_CHANNEL | digit[i];
			cmd[1] = 0x0;
			avc1394_transaction_block(handle, device, cmd, 2, 1);
 			usleep(100000); // small delay for button to register
		}
	}
	else
	{
		unsigned value = UNKNOWN;
		int i;
		for (i = 0; 0 != command_table[i].string; ++i) {
			if (0 == strcmp(input, command_table[i].string)) {
				value = command_table[i].value;
				break;
			}
		}
	
		if (value == UNKNOWN) {
			fprintf(stderr, "Sorry, that command is unknown.\n");
		} else {
			if (verbose)
				printf ("Issuing command %s to node %d.\n", input, device);
			if (debug) {
				printf("AV/C Press Command: Op1=0x%08X\n", CTL_CMD_PRESS | value );
			}
			cmd[0] = CTL_CMD_PRESS | value;
			cmd[1] = 0x0;
			avc1394_transaction_block(handle, device, cmd, 2, 1);
 			usleep(100000); // small delay for button to register
			cmd[0] = CTL_CMD_RELEASE;
			cmd[1] = 0x0;
			avc1394_transaction_block(handle, device, cmd, 2, 1);
	
		}
	}
	raw1394_destroy_handle(handle);
	exit(0);
}
