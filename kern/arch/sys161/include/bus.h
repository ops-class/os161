/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS161_BUS_H_
#define _SYS161_BUS_H_

/*
 * Generic bus interface file.
 *
 * The only bus on System/161 is LAMEbus.
 * This would need to be a bit more complicated if that weren't the case.
 */

#include <machine/vm.h>		/* for MIPS_KSEG1 */
#include <lamebus/lamebus.h>	/* for LAMEbus definitions */

#define bus_write_register(bus, slot, offset, val) \
    lamebus_write_register(bus, slot, offset, val)

#define bus_read_register(bus, slot, offset) \
    lamebus_read_register(bus, slot, offset)

#define bus_map_area(bus, slot, offset) \
    lamebus_map_area(bus, slot, offset)

/*
 * Machine-dependent LAMEbus definitions
 */

/* Base address of the LAMEbus mapping area */
#define LB_BASEADDR  (MIPS_KSEG1 + 0x1fe00000)


#endif /* _SYS161_BUS_H_ */
