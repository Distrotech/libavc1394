/*
 * librom1394 - GNU/Linux IEEE 1394 CSR Config ROM Library
 *
 * Originally written by Andreas Micklei <andreas.micklei@ivistar.de>
 * Better directory and textual leaf processing provided by Stefan Lucke
 * Libtoolize-d and modifications by Dan Dennedy <dan@dennedy.org>
 * Currently maintained by Dan Dennedy <dan@dennedy.org>
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

#include "rom1394.h"
#include "rom1394_internal.h"
#include "../common/raw1394util.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#define MAXLINE 80
#define MAX_OFFSETS	256

int rom1394_get_bus_info_block_length(raw1394handle_t handle, nodeid_t node)
{
	quadlet_t 	quadlet;
	octlet_t 	offset;
	int 		length;

	NODECHECK(handle, node);
	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + ROM1394_HEADER;
	QUADREADERR (handle, node, offset, &quadlet);
	quadlet = htonl (quadlet);
	length = quadlet >> 24;
	if (length != 4)
		WARN (node, "wrong bus info block length", offset);
	return length;
}
	
quadlet_t rom1394_get_bus_id(raw1394handle_t handle, nodeid_t node)
{
	quadlet_t 	quadlet;
	octlet_t 	offset;

	NODECHECK(handle, node);
	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + ROM1394_BUS_ID;
	QUADREADERR (handle, node, offset, &quadlet);
	quadlet = htonl (quadlet);

	if (quadlet != 0x31333934)
		WARN (node, "invalid bus id", offset);
    return quadlet;
}

int rom1394_get_bus_options(raw1394handle_t handle, nodeid_t node, rom1394_bus_options* bus_options)
{
	quadlet_t 	quadlet;
	octlet_t 	offset;

	NODECHECK(handle, node);
	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + ROM1394_BUS_OPTIONS;
	QUADREADERR (handle, node, offset, &quadlet);
	quadlet = htonl (quadlet);
	bus_options->irmc = quadlet >> 31;
	bus_options->cmc = (quadlet >> 30) & 1;
	bus_options->isc = (quadlet >> 29) & 1;
	bus_options->bmc = (quadlet >> 28) & 1;
	bus_options->cyc_clk_acc = (quadlet >> 16) & 0xFF;
	bus_options->max_rec = (quadlet >> 12) & 0xF;
	bus_options->max_rec = pow( 2, bus_options->max_rec+1);
	return 0;
}

octlet_t rom1394_get_guid(raw1394handle_t handle, nodeid_t node)
{
	quadlet_t 	quadlet;
	octlet_t 	offset;
	octlet_t    guid = 0;

	NODECHECK(handle, node);
	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + ROM1394_GUID_HI;
	QUADREADERR (handle, node, offset, &quadlet);
	quadlet = htonl (quadlet);
	guid = quadlet;
	guid <<= 32;
	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + ROM1394_GUID_LO;
	QUADREADERR (handle, node, offset, &quadlet);
	quadlet = htonl (quadlet);
	guid += quadlet;

    return guid;
}

int rom1394_get_directory(raw1394handle_t handle, nodeid_t node, rom1394_directory *dir)
{
	octlet_t 	offset;
	int i, j;
	char *p;

	NODECHECK(handle, node);
    dir->node_capabilities = 0;
    dir->vendor_id = 0;
    dir->unit_spec_id = 0;
    dir->unit_sw_version = 0;
    dir->model_id = 0;
	dir->max_textual_leafs = dir->nr_textual_leafs = 0;
	dir->label = NULL;
	dir->textual_leafs = NULL;

	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + ROM1394_ROOT_DIRECTORY;
	proc_directory (handle, node, offset, dir);

	 /* Calculate label */
    if (dir->nr_textual_leafs != 0 && dir->textual_leafs[0]) {
        for (i = 0, j = 0; i < dir->nr_textual_leafs; i++)
            if (dir->textual_leafs[i]) j += (strlen(dir->textual_leafs[i]) + 1);
    	if ( (dir->label = (char *) malloc(j)) ) {
    	    for (i = 0, p = dir->label; i < dir->nr_textual_leafs; i++, p++) {
    	        if (dir->textual_leafs[i]) {
    		        strcpy ( p, dir->textual_leafs[i]);
		            p += strlen(dir->textual_leafs[i]);
			        if (i < dir->nr_textual_leafs-1) p[0] = ' ';
		        }
		    }
		}
	}
	return 0;
}

/* ----------------------------------------------------------------------------
 * Get the type / protocol of a node
 * IN:		rom_info:	pointer to the Rom_info structure of the node
 * RETURNS:	one of the defined node types, i.e. NODE_TYPE_AVC, etc.
 */
rom1394_node_types rom1394_get_node_type(rom1394_directory *dir)
{
	if (dir->unit_spec_id == 0xA02D) {
		if (dir->unit_sw_version == 0x100) {
			return ROM1394_NODE_TYPE_DC;
//		} else if (unit_sw_version == 0x10000 || unit_sw_version == 0x10001) {
        } else if (dir->unit_sw_version & 0x010000) {
			return ROM1394_NODE_TYPE_AVC;
		}
	} else if (dir->unit_spec_id == 0x609E && dir->unit_sw_version == 0x10483) {
		return ROM1394_NODE_TYPE_SBP2;
	}
	return ROM1394_NODE_TYPE_UNKNOWN;
}

void rom1394_free_directory(rom1394_directory *dir)
{
    int i;
    for (i = 0; dir->textual_leafs && i < dir->nr_textual_leafs; i++)
        if (dir->textual_leafs[i]) free(dir->textual_leafs[i]);
    if (dir->textual_leafs) free(dir->textual_leafs);
    dir->textual_leafs = NULL;
    dir->max_textual_leafs = dir->nr_textual_leafs = 0;
    if (dir->label) free(dir->label);
}
