/*
 * librom1394 - GNU/Linux 1394 CSR Config ROM Library
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

#include "rom1394_internal.h"
#include "rom1394.h"
#include "../common/raw1394util.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

/*
 * Read a textual leaf into a malloced ASCII string
 * TODO: This routine should probably care about character sets, Unicode, etc.
 * IN:		phyID:	Physical ID of the node to read from
 *		offset:	Memory offset to read from
 * RETURNS:	pointer to a freshly malloced string that contains the
 *		requested text or NULL if the text could not be read.
 */
int read_textual_leaf(raw1394handle_t handle, nodeid_t node, octlet_t offset,
    rom1394_directory *dir) 
{
	int i, length;
	char *s;
	quadlet_t quadlet;
	quadlet_t language_spec;	// language specifier
	quadlet_t charset_spec;		// character set specifier

	DEBUG(node, "reading textual leaf: 0x%08x%08x\n", (unsigned int) (offset>>32), 
	    (unsigned int) offset&0xFFFFFFFF);

	QUADREADERR(handle, node, offset, &quadlet);
	quadlet = htonl(quadlet);
	length = ((quadlet >> 16) - 2) * 4;
	DEBUG(node, "textual leaf length: %i (0x%08X) %08x %08x\n", length, length, quadlet, htonl (quadlet));

	if (length<=0 || length > 256) {
	    WARN(node, "invalid number of textual leaves", offset);
	    return -1;
	}

	QUADINC(offset);
	QUADREADERR(handle, node, offset, &language_spec);
	language_spec = htonl(language_spec);
	/* assert language specifier=0 */
	if (language_spec != 0) {
		if (!(language_spec & 0x80000000)) 
			WARN(node, "unimplemented language for textual leaf", offset);
	}

	QUADINC(offset);
	QUADREADERR(handle, node, offset, &charset_spec);
	charset_spec = htonl(charset_spec);
	/* assert character set =0 */
	if (charset_spec != 0) {
		if (charset_spec != 0x409) 					// US_ENGLISH (unicode) Microsoft format leaf
			WARN(node, "unimplemented character set for textual leaf", offset);
	}

	if ((s = (char *) malloc(length+1)) == NULL)
		FAIL( node, "out of memory");

	if (!dir->max_textual_leafs) {
		if (!(dir->textual_leafs = (char **) calloc (1, sizeof (char *)))) {
			FAIL( node, "out of memory");
		}
		dir->max_textual_leafs = 1;
	}

	if (dir->nr_textual_leafs == dir->max_textual_leafs) {
		if (!(dir->textual_leafs = (char **) realloc (dir->textual_leafs, 
		    dir->max_textual_leafs * 2 * sizeof (char *)))) {
			FAIL( node, "out of memory");
		}
		dir->max_textual_leafs *= 2;
	}

	for (i=0; i<length; i++) {
		QUADINC(offset);
		QUADREADERR(handle, node, offset, &quadlet);
		quadlet = htonl(quadlet);
		if (charset_spec == 0) {
			s[i] = quadlet>>24;
			if (++i < length) s[i] = (quadlet>>16)&0xFF;
			else break;
			if (++i < length) s[i] = (quadlet>>8)&0xFF;
			else break;
			if (++i < length) s[i] = (quadlet)&0xFF;
			else break;
		} else {
			if (charset_spec == 0x409) {
				s[i] = (quadlet>>24) & 0xFF;
				if (++i < length) s[i] = (quadlet>>8) & 0xFF;
				else break;
			}
		}
	}
	s[i] = '\0';
    DEBUG( node, "textual leaf is: (%s)\n", s);
	dir->textual_leafs[dir->nr_textual_leafs++] = s;
	return 0;
}

int proc_directory (raw1394handle_t handle, nodeid_t node, octlet_t offset,
    rom1394_directory *dir)
{
	int		length, i, key, value;
	quadlet_t 	quadlet;
	octlet_t	subdir, selfdir;

	selfdir = offset;

    QUADREADERR(handle, node, offset, &quadlet);
    if (cooked1394_read(handle, (nodeid_t) 0xffc0 | node, offset, sizeof(quadlet_t), &quadlet) < 0) {
        FAIL( node, "invalid root directory length");
    } else {
        quadlet = htonl(quadlet);
    	length = quadlet>>16;
    
        DEBUG(node, "directory has %d entries\n", length);
    	for (i=0; i<length; i++) {
    		QUADINC(offset);
    		QUADREADERR(handle, node, offset, &quadlet);
    		quadlet = htonl(quadlet);
    		key = quadlet>>24;
    		value = quadlet&0x00FFFFFF;
            DEBUG(node, "key/value: %08x/%08x\n", key, value);
    		switch (key) {
    			case 0x0C:
    				dir->node_capabilities = value;
    				break;
    			case 0x03:
    				dir->vendor_id = value;
    				break;
    			case 0x12:
    				dir->unit_spec_id = value;
    				break;
    			case 0x13:
    				dir->unit_sw_version = value;
    				break;
    			case 0x17:
    				dir->model_id = value;
    				break;
    			case 0x81:
    			case 0x82:
    				read_textual_leaf( handle, node, offset + value * 4, dir);
    				break;
    			case 0xC3:
    			case 0xC7:
    			case 0xD1:
    			case 0xD4:
    			case 0xD8:
    				subdir = offset + value*4;
    				if (subdir > selfdir) {
    					proc_directory( handle, node, subdir, dir);
    				} else {
    					FAIL(node, "unit directory with back reference");
    				}
    				break;
    		}
    	}
    }
    return 0;
}

