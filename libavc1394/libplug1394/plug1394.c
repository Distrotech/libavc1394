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
 
/* standard system includes */
#include <sys/types.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <stdint.h>

/* linux1394 includes */
#include "plug1394.h"
#include "../common/raw1394util.h"
#include <libraw1394/csr.h>

#undef PLUG1394_DEBUG

#ifdef PLUG1394_DEBUG
#define DEBUG(s, args...) printf("libplug1394 debug: " s "\n", ## args)
#else
#define DEBUG(s, args...)
#endif
#define FAIL(s) {fprintf(stderr, "libplug1394 error: %s\n", s);return(-1);}


/*
 * Plug register access functions
 *
 * Please see the convenience macros defined in plug1394.h.
 */

int
plug1394_get(raw1394handle_t h, nodeid_t n, nodeaddr_t a, quadlet_t *value)
{
	quadlet_t temp; /* register value */
	int result;
	
	result = cooked1394_read( h, n, CSR_REGISTER_BASE + a, sizeof(temp), &temp);
	if (result >= 0)
		*value = ntohl(temp); /* endian conversion */
	return result;
}


int
plug1394_set(raw1394handle_t h, nodeid_t n, nodeaddr_t a, quadlet_t value)
{
	quadlet_t compare, swap, new;
	int result;
	
	/* get the current register value for comparison */
	result = plug1394_get( h, n, a, &compare );
	if (result >= 0)
	{
		/* convert endian */
		compare = htonl(compare);
		swap = htonl(value);
		result = raw1394_lock( h, n, CSR_REGISTER_BASE + a, EXTCODE_COMPARE_SWAP, swap, compare, &new);
	}
	return result;
}


/* 
 * Local host plugs implementation
 *
 * This requires the address range mapping feature of libraw1394 1.0.
 */

/* ARM context identifiers */
static char g_arm_callback_context_out[] = "libplug1394 output context";
static char g_arm_callback_context_in[] = "libplug1394 input context";
static struct raw1394_arm_reqhandle g_arm_reqhandle_out;
static struct raw1394_arm_reqhandle g_arm_reqhandle_in;

/* register space */

/* Please note that this is host global. Each port (host adapter)
   does not get its own register space. Also, this is only intended
   for use by a single application. It appears that when multiple processes
   host these plugs, it still works; as long as the application uses the
   plug access functions above, then it should work. */
static struct output_registers {
	struct oMPR mpr;
	struct oPCR pcr[PCR_MAX];
} g_data_out;

static struct input_registers {
	struct iMPR mpr;
	struct iPCR pcr[PCR_MAX];
} g_data_in;


/** Send an async packet in response to a register read.
 *
 *  This function handles host to bus endian conversion.
 *
 * \param handle  A raw1394 handle.
 * \param arm_req A pointer to an arm_request struct from the ARM callback
 *                handler.
 * \param a       The CSR offset address of the register.
 * \param data    The base address of the register space to read.
 * \return        0 for success, -1 on error.
 */
static int
do_arm_read(raw1394handle_t handle, struct arm_request *arm_req, 
		nodeaddr_t a, quadlet_t *data)
{
	quadlet_t *response;
	int num=4, offset;
	
	/* allocate response packet */
	response = malloc(num * sizeof(quadlet_t));
	if (!response)
		FAIL("unable to allocate response packet");
	memset(response, 0x00, num * sizeof(quadlet_t));
	
	/* fill data of response */
	response[0] = 
		((arm_req->source_nodeid & 0xFFFF) << 16) +
		((arm_req->tlabel        & 0x3F)   << 10) +
		(6 << 4); /* tcode = 6 */
	response[1] = ((arm_req->destination_nodeid & 0xFFFF) << 16);
		/* rcode = resp_complete implied */
	
	DEBUG ("      destination_offset=%d", 
		arm_req->destination_offset - CSR_REGISTER_BASE - a);
	offset = (arm_req->destination_offset - CSR_REGISTER_BASE - a)/4;
	response[3] = htonl(data[offset]);
	
	DEBUG("      response: 0x%8.8X",response[0]);
	DEBUG("                0x%8.8X",response[1]);
	DEBUG("                0x%8.8X",response[2]);
	DEBUG("                0x%8.8X",response[3]);

	/* send response */
	raw1394_start_async_send(handle, 16 , 16, 0, response, 0);

	free (response);
	return 0;
}


/** Update a local register value, and send a response packet.
 *
 *  This function performs a compare/swap lock operation only.
 *  This function handles host to bus endian conversion.
 *
 * \param handle  A raw1394 handle.
 * \param arm_req A pointer to an arm_request struct from the ARM callback
 *                handler.
 * \param a       The CSR offset address of the register.
 * \param data    The base address of the register space to update.
 * \return        0 for success, -1 on error.
 */
static int
do_arm_lock(raw1394handle_t handle, struct arm_request *arm_req,
		nodeaddr_t a, quadlet_t *data)
{
	quadlet_t *response;
	int num, offset;
	int rcode = RCODE_COMPLETE;
	int requested_length = 4;
			
	if (arm_req->extended_transaction_code == EXTCODE_COMPARE_SWAP)
	{
		quadlet_t arg_q, data_q, old_q, new_q;
		
		/* allocate response packet */
		num = 4 + requested_length;
		response = malloc(num * sizeof(quadlet_t));
		if (!response)
			FAIL("unable to allocate response packet");
		memset(response, 0x00, num * sizeof(quadlet_t));
		
		/* load data */
		offset = (arm_req->destination_offset - CSR_REGISTER_BASE - a)/4;
		response[4] = htonl(data[offset]);
		
		/* compare */
		arg_q  = *(quadlet_t *) (&arm_req->buffer[0]);
		data_q = *(quadlet_t *) (&arm_req->buffer[4]);
		old_q = *(quadlet_t *) (&response[4]);
		new_q = (old_q == arg_q) ? data_q : old_q;

		/* swap */
		data[offset] = ntohl(new_q);
		
	}
	else
	{
		rcode = RCODE_TYPE_ERROR;
		requested_length = 0;
	}

	/* fill data of response */
	response[0] = 
		((arm_req->source_nodeid & 0xFFFF) << 16) +
		((arm_req->tlabel        & 0x3F)   << 10) +
		(0xB << 4); /* tcode = B */
	response[1] = 
		((arm_req->destination_nodeid & 0xFFFF) << 16) +
		((rcode & 0xF) << 12);
	response[3] = 
		((requested_length & 0xFFFF) << 16) +
		(arm_req->extended_transaction_code & 0xFF);

	DEBUG("      response: 0x%8.8X",response[0]);
	DEBUG("                0x%8.8X",response[1]);
	DEBUG("                0x%8.8X",response[2]);
	DEBUG("                0x%8.8X",response[3]);
	DEBUG("                0x%8.8X",response[4]);

	/* send response */
	raw1394_start_async_send(handle, requested_length + 16, 16, 0, response, 0);

	free (response);
	return 0;
}


/* local plug ARM handler */
static int
plug1394_arm_callback (raw1394handle_t handle, 
	struct arm_request_response *arm_req_resp,
	unsigned int requested_length,
	void *pcontext, byte_t request_type)
{
	struct arm_request  *arm_req  = arm_req_resp->request;
	
	DEBUG( "request type=%d tcode=%d length=%d", request_type, arm_req->tcode, requested_length);
	DEBUG( "context = %s", (char *) pcontext);
	fflush(stdout);
	
	if (pcontext == g_arm_callback_context_out && requested_length == 4)
	{
		if (request_type == ARM_READ && arm_req->tcode == 4)
		{
			do_arm_read( handle, arm_req, CSR_O_MPR, (quadlet_t *) &g_data_out );
		}
		else if (request_type == ARM_LOCK)
		{
			do_arm_lock( handle, arm_req, CSR_O_MPR, (quadlet_t *) &g_data_out );
		}
		else
		{
			/* error response */
		}
	}
	else if (pcontext == g_arm_callback_context_in && requested_length == 4)
	{
		if (request_type == ARM_READ)
		{
			do_arm_read( handle, arm_req, CSR_I_MPR, (quadlet_t *) &g_data_in  );
		}
		else if (request_type == ARM_LOCK)
		{
			do_arm_lock( handle, arm_req, CSR_I_MPR, (quadlet_t *) &g_data_in );
		}
		else
		{
			/* error response */
		}
	}
	else
	{
		/* error response */
	}
	fflush(stdout);
	return 0;
}


int
plug1394_register_outputs(raw1394handle_t h, struct oMPR proto_mpr, struct oPCR proto_pcr)
{
	int num_plugs = (proto_mpr.n_plugs > PCR_MAX) ? PCR_MAX : proto_mpr.n_plugs;
	int i;
	
	g_data_out.mpr = proto_mpr;
	g_data_out.mpr.n_plugs = num_plugs;
	for (i = 0; i < num_plugs; i++)
		g_data_out.pcr[i] = proto_pcr;

	g_arm_reqhandle_out.arm_callback = (arm_req_callback_t) plug1394_arm_callback;
	g_arm_reqhandle_out.pcontext = g_arm_callback_context_out;

	return raw1394_arm_register( h, CSR_REGISTER_BASE + CSR_O_MPR, sizeof(g_data_out),
		(byte_t *) &g_data_out, (unsigned long) &g_arm_reqhandle_out,
		0, 0, ( ARM_READ | ARM_LOCK ) );
}


int
plug1394_register_inputs(raw1394handle_t h, struct iMPR proto_mpr, struct iPCR proto_pcr)
{
	int num_plugs = (proto_mpr.n_plugs > PCR_MAX) ? PCR_MAX : proto_mpr.n_plugs;
	int i;
	
	g_data_in.mpr = proto_mpr;
	g_data_in.mpr.n_plugs = num_plugs;
	for (i = 0; i < num_plugs; i++)
		g_data_in.pcr[i] = proto_pcr;

	g_arm_reqhandle_in.arm_callback = (arm_req_callback_t) plug1394_arm_callback;
	g_arm_reqhandle_in.pcontext = g_arm_callback_context_in;

	return raw1394_arm_register( h, CSR_REGISTER_BASE + CSR_I_MPR, sizeof(g_data_in),
		(byte_t *) &g_data_in, (unsigned long) &g_arm_reqhandle_in, 
		0, 0, ( ARM_READ | ARM_LOCK ) );
}


int
plug1394_unregister_outputs(raw1394handle_t h)
{
	return raw1394_arm_unregister(h, CSR_REGISTER_BASE + CSR_O_MPR);
}


int
plug1394_unregister_inputs(raw1394handle_t h)
{
	return raw1394_arm_unregister(h, CSR_REGISTER_BASE + CSR_I_MPR);
}

