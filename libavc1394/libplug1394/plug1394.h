/*
 * libplug1394 - GNU/Linux 1394 IEC-61883 Plug Control Library
 * Copyright 2002 Dan Dennedy <dan@dennedy.org>
 *
 * Bits of raw1394 ARM handling borrowed from 
 * Christian Toegel's <christian.toegel@gmx.at> demos.
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
 
#ifndef PLUG1394_H
#define PLUG1394_H

#include <stdint.h>
#include <libraw1394/raw1394.h>

#ifdef __cplusplus
extern "C" {
#endif

/* maximum number of PCRs allowed within the standard 
 * MPR/PCR addresses defined in IEC-61883.
 * This refers to the number of output or input PCRs--
 * not the MPRs and not the combined total.
 */
#define PCR_MAX 31


/* standard CSR offsets for plugs */
#define CSR_O_MPR   0x900
#define CSR_O_PCR_0 0x904
#define CSR_O_PCR_1 0x908
#define CSR_O_PCR_2 0x90C
#define CSR_O_PCR_3 0x910

#define CSR_I_MPR   0x980
#define CSR_I_PCR_0 0x984
#define CSR_I_PCR_1 0x988
#define CSR_I_PCR_2 0x98C
#define CSR_I_PCR_3 0x990


/* plug1394-specific enums */
enum PLUG1394_OVERHEAD_ID
{
	OVERHEAD_512, 
	OVERHEAD_32, 
	OVERHEAD_64,
	OVERHEAD_96,
	OVERHEAD_128,
	OVERHEAD_160,
	OVERHEAD_192,
	OVERHEAD_224,
	OVERHEAD_256,
	OVERHEAD_288,
	OVERHEAD_320,
	OVERHEAD_352,
	OVERHEAD_384,
	OVERHEAD_416,
	OVERHEAD_448,
	OVERHEAD_480
};

enum PLUG1394_DATA_RATE
{
    DATA_RATE_100 = 0,
    DATA_RATE_200,
    DATA_RATE_400
};


/*
 * Plug data structures
 */

struct oMPR {
	uint32_t n_plugs:5;
	uint32_t reserved:3;
	uint32_t persist_ext:8;
	uint32_t non_persist_ext:8;
	uint32_t bcast_channel:6;
	uint32_t data_rate:2;
};

struct iMPR {
	uint32_t n_plugs:5;
	uint32_t reserved2:3;
	uint32_t persist_ext:8;
	uint32_t non_persist_ext:8;
	uint32_t reserved:6;
	uint32_t data_rate:2;
};

struct oPCR {
	uint32_t payload:10;
	uint32_t overhead_id:4;
	uint32_t data_rate:2;
	uint32_t channel:6;
	uint32_t reserved:2;
	uint32_t n_p2p_connections:6;
	uint32_t bcast_connection:1;
	uint32_t online:1;
	
};

struct iPCR {
	uint32_t reserved2:16;
	uint32_t channel:6;
	uint32_t reserved:2;
	uint32_t n_p2p_connections:6;
	uint32_t bcast_connection:1;
	uint32_t online:1;
};


/**************************************************************************
 *
 * Low level plug register access functions 
 *
 */

/** Read a node's plug register.
 * 
 *  This function handles bus to host endian conversion.
 *
 * \param h     A raw1394 handle.
 * \param n     The node id of the node to read
 * \param a     The CSR offset address (relative to base) of the register
 *              to read.
 * \param value A pointer to a quadlet where the plug register's value will
 *              be stored.
 * \return      0 for success, a negative number for error (errno).
 */
int
plug1394_get(raw1394handle_t h, nodeid_t n, nodeaddr_t a, quadlet_t *value);


/** Write a node's plug register.
 *
 *  This uses a compare/swap lock() operation to safely write the
 *  new register value, as required by IEC 61883-1.
 *  This function handles host to bus endian conversion.
 *
 * \param h     A raw1394 handle.
 * \param n     The node id of the node to read
 * \param a     The CSR offset address (relative to CSR base) of the
 *              register to write.
 * \param value A quadlet containing the new register value.
 * \return      0 for success, a negative number for error (errno).
 */
int
plug1394_set(raw1394handle_t h, nodeid_t n, nodeaddr_t a, quadlet_t value);


/**************************************************************************
 * 
 * High level plug access macros
 *
 */

#define plug1394_get_oMPR(h,n,v) plug1394_get(h, n, CSR_O_MPR, (quadlet_t *) v)
#define plug1394_set_oMPR(h,n,v) plug1394_set(h, n, CSR_O_MPR, *((quadlet_t *) &v))

#define plug1394_get_oPCR0(h,n,v) plug1394_get(h, n, CSR_O_PCR_0, (quadlet_t *) v)
#define plug1394_set_oPCR0(h,n,v) plug1394_set(h, n, CSR_O_PCR_0, *((quadlet_t *) &v))
#define plug1394_get_oPCR1(h,n,v) plug1394_get(h, n, CSR_O_PCR_1, (quadlet_t *) v)
#define plug1394_set_oPCR1(h,n,v) plug1394_set(h, n, CSR_O_PCR_1, *((quadlet_t *) &v))
#define plug1394_get_oPCR2(h,n,v) plug1394_get(h, n, CSR_O_PCR_2, (quadlet_t *) v)
#define plug1394_set_oPCR2(h,n,v) plug1394_set(h, n, CSR_O_PCR_2, *((quadlet_t *) &v))
#define plug1394_get_oPCR3(h,n,v) plug1394_get(h, n, CSR_O_PCR_3, (quadlet_t *) v)
#define plug1394_set_oPCR3(h,n,v) plug1394_set(h, n, CSR_O_PCR_3, *((quadlet_t *) &v))
#define plug1394_get_oPCRX(h,n,v,x) plug1394_get(h, n, CSR_O_PCR_0+(4*x), (quadlet_t *) v)

#define plug1394_get_iMPR(h,n,v) plug1394_get(h, n, CSR_I_MPR, (quadlet_t *) v)
#define plug1394_set_iMPR(h,n,v) plug1394_set(h, n, CSR_I_MPR, *((quadlet_t *) &v))

#define plug1394_get_iPCR0(h,n,v) plug1394_get(h, n, CSR_I_PCR_0, (quadlet_t *) v)
#define plug1394_set_iPCR0(h,n,v) plug1394_set(h, n, CSR_I_PCR_0, *((quadlet_t *) &v))
#define plug1394_get_iPCR1(h,n,v) plug1394_get(h, n, CSR_I_PCR_1, (quadlet_t *) v)
#define plug1394_set_iPCR1(h,n,v) plug1394_set(h, n, CSR_I_PCR_1, *((quadlet_t *) &v))
#define plug1394_get_iPCR2(h,n,v) plug1394_get(h, n, CSR_I_PCR_2, (quadlet_t *) v)
#define plug1394_set_iPCR2(h,n,v) plug1394_set(h, n, CSR_I_PCR_2, *((quadlet_t *) &v))
#define plug1394_get_iPCR3(h,n,v) plug1394_get(h, n, CSR_I_PCR_3, (quadlet_t *) v)
#define plug1394_set_iPCR3(h,n,v) plug1394_set(h, n, CSR_I_PCR_3, *((quadlet_t *) &v))
#define plug1394_get_iPCRX(h,n,v,x) plug1394_get(h, n, CSR_I_PCR_0+(4*x), (quadlet_t *) v)


/**************************************************************************
 *
 * Functions for Hosting Plug Control Registers 
 *
 */

/** Create local host output plug registers.
 *
 *  This function registers the plugs using raw1394 ARM.
 *  The number of oPCR plugs to be created is set in param
 *  proto_mpr.n_plugs.
 *
 * \param h         A raw1394 handle.
 * \param proto_mpr The initial value for the oMPR plug.
 * \param proto_pcr The initial value for all of the oPCR plugs.
 * \return 0 for success, -1 (errno) on error.
 */
int
plug1394_register_outputs(raw1394handle_t h, struct oMPR proto_mpr, struct oPCR proto_pcr);


/** Create local host input plug registers.
 *
 *  This function registers the plugs using raw1394 ARM.
 *  The number of iPCR plugs to be created is set in param
 *  proto_mpr.n_plugs.
 *
 * \param h         A raw1394 handle.
 * \param proto_mpr The initial value for the iMPR plug.
 * \param proto_pcr The initial value for all of the iPCR plugs.
 * \return 0 for success, -1 (errno) on error.
 */
int
plug1394_register_inputs(raw1394handle_t h, struct iMPR proto_mpr, struct iPCR proto_pcr);


/** Remove local host output plugs.
 *
 * \param h A raw1394 handle.
 * \return 0 for success, -1 (errno) on error.
 */
int
plug1394_unregister_outputs(raw1394handle_t h);


/** Remove local host input plugs.
 *
 * \param h A raw1394 handle.
 * \return 0 for success, -1 (errno) on error.
 */
int
plug1394_unregister_inputs(raw1394handle_t h);


/**************************************************************************
 *
 * constants for use with raw1394 ARM
 * The following probably should be in raw1394.h
 * They are included here for convenience sake if you are doing
 * any other ARM coding.
 */

/* extended transaction codes (lock-request-response) */
#define EXTCODE_MASK_SWAP        0x1
/* IEC 61883 says plugs should only be manipulated using
 * compare_swap lock. */
#define EXTCODE_COMPARE_SWAP     0x2
#define EXTCODE_FETCH_ADD        0x3
#define EXTCODE_LITTLE_ADD       0x4
#define EXTCODE_BOUNDED_ADD      0x5
#define EXTCODE_WRAP_ADD         0x6

/* response codes */
#define RCODE_COMPLETE           0x0
#define RCODE_CONFLICT_ERROR     0x4
#define RCODE_DATA_ERROR         0x5
#define RCODE_TYPE_ERROR         0x6
#define RCODE_ADDRESS_ERROR      0x7


#ifdef __cplusplus
}
#endif

#endif
