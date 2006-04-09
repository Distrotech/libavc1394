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
};

#define UNKNOWN -1
struct lookup_table_t lookup_table[] =
    {
	{ "ok",           0x00 }
	, { "select",     0x00 },
	{ "up",           0x01 },
	{ "down",         0x02 },
	{ "left",         0x03 },
	{ "right",        0x04 },
	{ "menu",         0x09 },
	{ "lock",         0x0A }, { "setup",      0x0A },
	{ "guide",        0x0B }, { "contents",   0x0B },
	{ "favorite",     0x0C },
	{ "exit",         0x0D },
	{ "num0",         0x20 },
	{ "num1",         0x21 },
	{ "num2",         0x22 },
	{ "num3",         0x23 },
	{ "num4",         0x24 },
	{ "num5",         0x25 },
	{ "num6",         0x26 },
	{ "num7",         0x27 },
	{ "num8",         0x28 },
	{ "num9",         0x29 },
	// { "dot",          0x2A },
	{ "enter",        0x2B }, { "music",      0x2B },
	// { "clear",        0x2C },
	{ "channelup",    0x30 },
	{ "channeldown",  0x31 },
	{ "last",         0x32 }, { "previous",   0x32 },
	// { "sound",        0x33 }, { "language",   0x33 },
	// { "input",        0x34 },
	{ "info",         0x35 }, { "display",    0x35 },
	{ "help",         0x36 },
	{ "pageup",       0x37 },
	{ "pagedown",     0x38 },
	{ "power",        0x40 },
	{ "volumeup",     0x41 },
	{ "volumedown",   0x42 },
	{ "mute",         0x43 },
	{ "play",         0x44 },
	{ "stop",         0x45 },
	{ "pause",        0x46 },
	{ "record",       0x47 }, { "save",       0x47 },
	{ "rewind",       0x48 },
	{ "fastforward",  0x49 },
	// { "eject",        0x4A },
	// { "forward",      0x4B },
	// { "backward",     0x4C },
	// { "angle",        0x4D },
	// { "pip",          0x4E }, { "subpicture", 0x4E },
	{ "pauseplay",    0x61 },
	{ "pauserecord" , 0x63 },
	{ "dayback",      0x64 },
	// dayforward     ?
	{ "depletevolume",0x65 },
	{ "restorevolume",0x66 },
	{ 0, UNKNOWN }
};

int hex2int(char *input)
{
	static char hex_table[] = "0123456789abcdef";
	signed result = 0;
	int i;
	for (i = 0; i < strlen(input); ++i) {
		char lower = tolower(input[i]);
		char *ptr = strchr(hex_table, lower);
		if (ptr) {
			result = 16 * result + (ptr - hex_table);
		}
	}
	return result;
}

void usage()
{
	fprintf(stderr, "Usage: motdctcmd [-v] <command>\n\n"
		"   <command> can be a channel number or a command:\n"
		);
	int last = 0;
	int i;
	for (i = 0; 0 != lookup_table[i].string; ++i) {
		if (i > 0 && lookup_table[i].value == last) {
			fprintf(stderr," | %s", lookup_table[i].string);
		} else {
			fprintf(stderr,"\n   %s", lookup_table[i].string);
		}
		last = lookup_table[i].value;
	}
	fprintf(stderr, "\n\n");
	exit(1);
}

int main (int argc, char *argv[])
{
	rom1394_directory dir;
	int device = UNKNOWN;
	int verbose = 0;
	quadlet_t cmd[2];
	char *input = NULL;
	int channel;

	if (argc == 1) {
		usage();
	} else if (argc == 2 && argv[1][0] == '-' && (argv[1][1] == 'h' || argv[1][1] == '-')) {
		usage();
	} else if (argc == 3 && argv[1][0] == '-' && argv[1][1] == 'v') {
		verbose = 1;
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

		if (verbose)
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
		
		if (verbose)
			printf("AV/C Command: %d%d%d = Op1=0x%08X Op2=0x%08X Op3=0x%08X\n", 
				digit[0], digit[1], digit[2], 
				CTL_CMD_CHANNEL | digit[0],
				CTL_CMD_CHANNEL | digit[1], 
				CTL_CMD_CHANNEL | digit[2]);
		
		for (i=0; i < 3; i++) {
			cmd[0] = CTL_CMD_CHANNEL | digit[i];
			cmd[1] = 0x0;
			avc1394_transaction_block(handle, device, cmd, 2, 1);
 			usleep(500000); // small delay for button to register
		}
	}
	else
	{
		unsigned value = UNKNOWN;
		int pass = hex2int(input + 2);
		if (strlen(input) == 4 && strncmp(input, "0x", 2) == 0 && pass) {
			value = pass;
		} else {
			int i;
			for (i = 0; 0 != lookup_table[i].string; ++i) {
				if (0 == strcmp(input, lookup_table[i].string)) {
					value = lookup_table[i].value;
					break;
				}
			}
		}
	
		if (value == UNKNOWN) {
			fprintf(stderr, "Sorry, that command is unknown.\n");
		} else {
			if (verbose) {
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
