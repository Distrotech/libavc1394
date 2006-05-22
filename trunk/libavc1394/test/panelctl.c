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
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#define version "0.1"

// Motorola DCT-6x00 IDs
#define MOTDCT_SPEC_ID    0x00005068
#define MOTDCT_SW_VERSION 0x00010101


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

void ver()
{
	printf("panelctl (libavc1394) %s\n\n", version);
	printf("AV/C Panel control program\n"
		"By: Stacey D. Son, John Woodell, and Dan Dennedy\n"
		"Copyright (C) 2004-2006\n"
	);
	exit(1);
}

void usage()
{
	printf("Usage: panelctl [OPTION] <channel|command>\n"
		"Send control commands via IEEE1394 (firewire),\n"
		"to an AV/C Panel, e.g. Motorola DCT (digital cable box).\n\n"
		"Options:\n"
		"  -d, --debug      Display debug information\n"
		//"  -s, --status     Display status of device and exit\n"
		"  -h, --help       Display this help and exit\n"
		"  -v, --version    Output version information and exit\n\n"
		"Channels:\n"
		"  001 - 999        Tune directly to a specific channel\n\n"
		"Commands:\n"
		);
	int i;
	for (i = 0; 0 != command_table[i].string; ++i) {
		two_col(command_table[i].string, command_table[i].desc);
	}
	printf("  num0 - num9      Emulate a number key pressed\n\n");
	exit(1);
}

int main (int argc, char *argv[])
{
	rom1394_directory dir;
	int device = UNKNOWN;
	int debug = 0;
	quadlet_t cmd[2];
	char *input = NULL;
	int channel;

	if (argc == 1) {
		usage();
	} else if (argc > 1 && argv[1][0] == '-' && (argv[1][1] == 'h' || argv[1][2] == 'h')) {
		usage();
	} else if (argc > 1 && argv[1][0] == '-' && (argv[1][1] == 'v' || argv[1][2] == 'v')) {
		ver();
	} else if (argc > 1 && argv[1][0] == '-' && (argv[1][1] == 's' || argv[1][2] == 's')) {
		status();
	} else if (argc == 3 && argv[1][0] == '-' && argv[1][1] == 'd') {
		debug = 1;
		input = argv[2];
	} else {
		input = argv[1];
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

		if (debug)
			printf("node %d: vendor_id = 0x%08x model_id = 0x%08x\n",
			       i, dir.vendor_id, dir.model_id);

		if ( dir.unit_spec_id == MOTDCT_SPEC_ID &&
		        dir.unit_sw_version == MOTDCT_SW_VERSION) {
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
