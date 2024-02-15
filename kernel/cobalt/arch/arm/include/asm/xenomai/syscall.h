/*
 * Copyright (C) 2001,2002,2003,2004 Philippe Gerum <rpm@xenomai.org>.
 *
 * ARM port
 *   Copyright (C) 2005 Stelian Pop
 *
 * Xenomai is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifndef _COBALT_ARM_SYSCALL_H
#define _COBALT_ARM_SYSCALL_H

#include <linux/errno.h>
#include <linux/uaccess.h>
#include <asm/unistd.h>
#include <asm/ptrace.h>
#include <asm-generic/xenomai/syscall.h>

/*
 * Cobalt syscall and Linux syscall numbers can be fetched directly from
 * ARM_r7. Since we have to work with Dovetail whilst remaining binary
 * compatible with applications built for the I-pipe, the syscall signature
 * is possibly ORed with __COBALT_SYSCALL_BIT by Dovetail (IPIPE_COMPAT
 * mode).
 *
 * FIXME: We also have __COBALT_SYSCALL_BIT (equal to
 * __OOB_SYSCALL_BIT) present in the actual syscall number in r0,
 * which is pretty much useless. Oh, well...  When support for the
 * I-pipe is dropped, we may switch back to the regular convention
 * Dovetail abides by, with the actual syscall number into r7 ORed
 * with __OOB_SYSCALL_BIT, freeing r0 for passing a call argument.
 */
#define __xn_reg_sys(__regs)	((__regs)->ARM_r7)
#define __xn_syscall_p(__regs)	((__regs)->ARM_r7 & __COBALT_SYSCALL_BIT)
#define __xn_syscall(__regs)	(__xn_reg_sys(__regs) & ~__COBALT_SYSCALL_BIT)

/*
 * Root syscall number with predicate (valid only if
 * !__xn_syscall_p(__regs)).
 */
#define __xn_rootcall_p(__regs, __code)					\
	({								\
		*(__code) = (__regs)->ARM_r7;				\
		*(__code) < NR_syscalls || *(__code) >= __ARM_NR_BASE;	\
	})

#define __xn_reg_rval(__regs)	((__regs)->ARM_r0)
#define __xn_reg_pc(__regs)	((__regs)->ARM_ip)
#define __xn_reg_sp(__regs)	((__regs)->ARM_sp)

static inline void __xn_error_return(struct pt_regs *regs, int v)
{
	__xn_reg_rval(regs) = v;
}

static inline void __xn_status_return(struct pt_regs *regs, long v)
{
	__xn_reg_rval(regs) = v;
}

static inline int __xn_interrupted_p(struct pt_regs *regs)
{
	return __xn_reg_rval(regs) == -EINTR;
}

#endif /* !_COBALT_ARM_SYSCALL_H */
