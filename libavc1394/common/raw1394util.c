
#include <config.h>
#include "raw1394util.h"
#include <errno.h>
#include <time.h>

int cooked1394_read(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr,
                    size_t length, quadlet_t *buffer)
{
    int retval, i;
	struct timespec ts = {0, RETRY_DELAY};
#ifdef RAW1394_V_0_8
    int ackcode, rcode;
#endif
    for(i=0; i<MAXTRIES; i++) {
        retval = raw1394_read(handle, node, addr, length, buffer);
#ifdef RAW1394_V_0_9
		nanosleep(&ts, NULL);
        if (retval < 0 && errno == EAGAIN)
            nanosleep(&ts, NULL);
        else
            return retval;
    }
    return -1;
#else
        if( retval < 0 ) return retval;
        ackcode = RAW1394_MASK_ACK(retval);
        rcode = RAW1394_MASK_RCODE(retval);
        if( ackcode == ACK_BUSY_X || ackcode == ACK_BUSY_A
                || ackcode == ACK_BUSY_B ) {
            nanosleep(&ts, NULL);
        } else {
            break;
        }
    }

    if( ackcode != ACK_PENDING && ackcode != ACK_COMPLETE &&
            ackcode != ACK_LOCAL)
        return -1;
    return retval;
#endif
}

int cooked1394_write(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr,
                     size_t length, quadlet_t *data)
{
    int retval, i;
	struct timespec ts = {0, RETRY_DELAY};
#ifdef RAW1394_V_0_8
    int ackcode, rcode;
#endif
    for(i=0; i<MAXTRIES; i++) {
        retval = raw1394_write(handle, node, addr, length, data);
#ifdef RAW1394_V_0_9
        if (retval < 0 && errno == EAGAIN)
            nanosleep(&ts, NULL);
        else
            return retval;
    }
    return -1;
#else
        if(retval < 0 )
            return retval;
        ackcode = RAW1394_MASK_ACK(retval);
        rcode = RAW1394_MASK_RCODE(retval);
        if( ackcode == ACK_BUSY_X || ackcode == ACK_BUSY_A
                || ackcode == ACK_BUSY_B ) {
            nanosleep(&ts, NULL);
        } else {
            break;
        }
    }

    if( ackcode != ACK_PENDING && ackcode != ACK_COMPLETE &&
            ackcode != ACK_LOCAL)
        return -1;
    return retval;
#endif
}
