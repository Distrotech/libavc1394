#ifndef ROM1394_INTERNAL_H
#define ROM1394_INTERNAL_H 1

#include "rom1394.h"

#define QUADINC(x) x+=4
#define WARN(node, s, addr) fprintf(stderr,"rom1394_%i warning: %s: 0x%08x%08x\n", node, s, (int) (addr>>32), (int) addr)
#define QUADREADERR(handle, node, offset, buf) if(cooked1394_read(handle, (nodeid_t) 0xffc0 | node, (nodeaddr_t) offset, (size_t) sizeof(quadlet_t), (quadlet_t *) buf) < 0) WARN(node, "read failed", offset);
#define FAIL(node, s) {fprintf(stderr, "rom1394_%i error: %s\n", node, s);exit(-1);}
#ifdef ROM1394_DEBUG
#define DEBUG(node, s, args...) \
printf("rom1394_%i debug: " s "\n", node, ## args)
#else
#define DEBUG(node, s, args...)
#endif

void read_textual_leaf(raw1394handle_t handle, nodeid_t node, octlet_t offset,
    rom1394_directory *dir);
void proc_directory (raw1394handle_t handle, nodeid_t node, octlet_t offset,
    rom1394_directory *dir);
    
#endif