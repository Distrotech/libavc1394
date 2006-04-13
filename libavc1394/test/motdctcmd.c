/*
 * motdctcmd - Control a Motorola DCT 6200/6400 series settop box
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

#define AVC1394_SUBUNIT_TYPE_MOTDCT (9 << 19)  /* uses a reserved subunit type */
#define AVC1394_MOTDCT_COMMAND 0x000007C00     /* 6200 subunit command */
#define AVC1394_MOTDCT_OPERAND_PRESS 0x00      /* 6200 subunit command operand */
#define AVC1394_MOTDCT_OPERAND_RELEASE 0x80    /* 6200 subunit command operand */

#define CTL_CMD_PRESS AVC1394_CTYPE_CONTROL | AVC1394_SUBUNIT_TYPE_MOTDCT | \
        AVC1394_SUBUNIT_ID_0 | AVC1394_MOTDCT_COMMAND | \
        AVC1394_MOTDCT_OPERAND_PRESS

#define CTL_CMD_RELEASE AVC1394_CTYPE_CONTROL | AVC1394_SUBUNIT_TYPE_MOTDCT | \
        AVC1394_SUBUNIT_ID_0 | AVC1394_MOTDCT_COMMAND | \
        AVC1394_MOTDCT_OPERAND_RELEASE

#define CTL_CMD_CHANNEL AVC1394_CTYPE_CONTROL | AVC1394_SUBUNIT_TYPE_MOTDCT | \
        AVC1394_SUBUNIT_ID_0 | AVC1394_MOTDCT_COMMAND | 0x20

struct lookup_table_t
{
	char *string;
	int value;
	char *desc;
};

#define UNKNOWN -1

struct lookup_table_t command_table[] = 
{
	{ "ok",           0x00, "Select the highlighted item" },
	{ "select",       0x00, "" },
	{ "up",           0x01, "Move up in the menu or guide" },
	{ "down",         0x02, "Move down in the menu or guide" },
	{ "left",         0x03, "Move left in the menu or guide" },
	{ "right",        0x04, "Move right in the menu or guide" },
	{ "menu",         0x09, "Enter or Exit the Main Menu" },
	{ "lock",         0x0A, "Bring up Parental Control screen" },
	{ "guide",        0x0B, "Bring up Listings By Time screen" },
	{ "favorite",     0x0C, "Scan through just your favorite channels" },
	{ "exit",         0x0D, "Return to live TV from the menu or guide" },
	{ "num0",         0x20, "" }, { "num1",         0x21, "" },
	{ "num2",         0x22, "" }, { "num3",         0x23, "" },
	{ "num4",         0x24, "" }, { "num5",         0x25, "" },
	{ "num6",         0x26, "" }, { "num7",         0x27, "" },
	{ "num8",         0x28, "" }, { "num9",         0x29, "" },
	{ "enter",        0x2B, "Enter the digital Music menu" },
	{ "music",        0x2B, "" },
	{ "channelup",    0x30, "Change channel up" },
	{ "channeldown",  0x31, "Change channel down" },
	{ "last",         0x32, "Return to the previous menu or channel" },
	{ "previous",     0x32, "" },
	{ "info",         0x35, "See a description of the current show" },
	{ "display",      0x35, "" },
	{ "help",         0x36, "See helpful information" },
	{ "pageup",       0x37, "Move up one page in the menu or guide" },
	{ "pagedown",     0x38, "Move down one page in the menu or guide" },
	{ "power",        0x40, "Toggle the device on or off" },
	{ "volumeup",     0x41, "Change volume up" },
	{ "volumedown",   0x42, "Change volume down" },
	{ "mute",         0x43, "Toggle sound on or off" },
	{ "play",         0x44, "Play DVR or On-Demand content" },
	{ "stop",         0x45, "Stop DVR or On-Demand content" },
	{ "pause",        0x46, "Pause DVR or On-Demand content" },
	{ "record",       0x47, "Record content on the DVR" },
	{ "save",         0x47, "" },
	{ "rewind",       0x48, "Rewind DVR or On-Demand content" },
	{ "reverse",      0x48, "" },
	{ "fastforward",  0x49, "Fast Forward DVR or On-Demand content" },
	{ "forward",      0x49, "" }, { "ff",           0x49, "" },
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
	printf("motdctcmd (libavc1394) %s\n\n", version);
	printf("Motorola DCT (digital cable box) control program\n"
		"By: Stacey D. Son, John Woodell, and Dan Dennedy\n"
		"Copyright (C) 2004-2006\n"
	);
	exit(1);
}

void usage()
{
	printf("Usage: motdctcmd [OPTION] <channel|command>\n"
		"Send control commands via IEEE1394 (firewire),\n"
		"to a Motorola DCT (digital cable box).\n\n"
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
		fprintf(stderr, "Could not find Motorola DCT on the 1394 bus.\n");
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
