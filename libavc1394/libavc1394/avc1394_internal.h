
#include "avc1394.h"

/* FCP Register Space */
#define FCP_COMMAND_ADDR 0xFFFFF0000B00
#define FCP_RESPONSE_ADDR 0xFFFFF0000D00

#define MAX_RESPONSE_SIZE 512
#define AVC1394_RETRY 2
#define AVC1394_SLEEP 10
/* #define DEBUG */

void htonl_block(quadlet_t *buf, int len);
void ntohl_block(quadlet_t *buf, int len);
char *decode_response(quadlet_t response);
int avc_fcp_handler(raw1394handle_t handle, nodeid_t nodeid, int response,
                    size_t length, unsigned char *data);
void init_avc_response_handler(raw1394handle_t handle);
void stop_avc_response_handler(raw1394handle_t handle);

