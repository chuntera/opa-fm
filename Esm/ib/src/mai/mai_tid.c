/* BEGIN_ICS_COPYRIGHT5 ****************************************

Copyright (c) 2015-2020, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

** END_ICS_COPYRIGHT5   ****************************************/

/****************************************************************************
 *                                                                          *
 * FILE NAME                                               VERSION          *
 *      mai_tid.c                                          MAPI 0.05        *
 *                                                                          *
 * DESCRIPTION                                                              *
 *      This file contains the code for the TID allocation API              *
 *                                                                          *
 *                                                                          *
 * DATA STRUCTURES                                                          *
 *      None                                                                *
 *                                                                          *
 ****************************************************************************/

/*
 * Local definitions:
 *
 *
 */

#include "mai_l.h"		/* Local mai function definitions */
#include "ispinlock.h"

/*
 * FUNCTION
 *      mai_alloc_tid
 *
 * DESCRIPTION
 *      Returns a unique TID to be tack on to mads and filters.
 *
 *
 * INPUTS
 *      fd       A valid channel handle
 *      *tid     Location to copy the TID to.
 *
 * OUTPUTS
 *      VSTATUS_OK
 *      VSTATUS_ILLPARM
 *
 * HISTORY
 *      NAME      DATE          REMARKS
 *      PAW     06/4/01 Initial entry
 */
static ATOMIC_UINT layer_counter=0;

Status_t
mai_alloc_tid(IBhandle_t fd, uint8_t mclass, uint64_t * tid)
{
    int             rc = 0;
    ATOMIC_UINT     counter = AtomicIncrement(&layer_counter);

    IB_ENTER(__func__, fd, 0, 0, 0);

    if (!gMAI_INITIALIZED)
      {
	  rc = VSTATUS_UNINIT;
	  IB_LOG_ERROR0("MAPI library not initialized");
	  return rc;
      }

#if defined(CAL_IBACCESS)
    // For the embedded SM, inject the process ID into the TID
    // to ensure responses are delivered to the correct destination.
    Threadname_t    pid;
    if ((rc = vs_thread_name(&pid)) != VSTATUS_OK) {
        IB_EXIT(__func__, rc);
        return (rc);
    }
    uint64_t        mpid = (uint64_t) pid;
    *tid = (((mpid & 0xffffff) << 40) |	// 24 bits for pid
	        ((counter & 0xffff) << 24));// 16 bits for counter
#elif defined(IB_STACK_OPENIB)
	// OpenIB reserves the top 32 bits of the TID for its own use.
	// (See infiniband/core/user_mad.c.)
	*tid = (counter & 0xffffffff);      // 32 bits for counter
#else
#error Either CAL_IBACCESS or IB_STACK_OPENIB must be defined.
#endif

    IB_EXIT(__func__, rc);
    return (rc);
}

/*
 * mai_increment_tid
 *   Given a tid allocated by mai_alloc_tid, computes a new unique TID
 *
 * INPUTS
 *      the previous tid
 *
 * RETURNS
 *      new tid
 */
uint64_t
mai_increment_tid(uint64_t tid)
{
    ATOMIC_UINT     counter = AtomicIncrement(&layer_counter);

#if defined(CAL_IBACCESS)
    return (tid & 0xffffff0000ffffffULL) | ((counter & 0xffff) << 24);
#elif defined(IB_STACK_OPENIB)
    return (tid & 0xffffffff00000000ULL) | (counter & 0xffffffff);
#else
#error Either CAL_IBACCESS or IB_STACK_OPENIB must be defined.
#endif
}
