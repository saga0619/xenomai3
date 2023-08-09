/*
 * Copyright (C) 2001,2002,2003,2004,2005 Philippe Gerum <rpm@xenomai.org>.
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
#ifndef _COBALT_ASM_GENERIC_SYSCALL_H
#define _COBALT_ASM_GENERIC_SYSCALL_H

#include <linux/types.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <asm/xenomai/features.h>
#include <asm/xenomai/wrappers.h>
#include <asm/xenomai/machine.h>
#include <cobalt/uapi/asm-generic/syscall.h>
#include <cobalt/uapi/kernel/types.h>

#define __xn_copy_from_user(dstP, srcP, n)	raw_copy_from_user(dstP, srcP, n)
#define __xn_copy_to_user(dstP, srcP, n)	raw_copy_to_user(dstP, srcP, n)
#define __xn_put_user(src, dstP)		__put_user(src, dstP)
#define __xn_get_user(dst, srcP)		__get_user(dst, srcP)
#define __xn_strncpy_from_user(dstP, srcP, n)	strncpy_from_user(dstP, srcP, n)

static inline int cobalt_copy_from_user(void *dst, const void __user *src,
					size_t size)
{
	size_t remaining = size;

	if (likely(access_ok(src, size)))
		remaining = __xn_copy_from_user(dst, src, size);

	if (unlikely(remaining > 0)) {
		memset(dst + (size - remaining), 0, remaining);
		return -EFAULT;
	}
	return 0;
}

static inline int cobalt_copy_to_user(void __user *dst, const void *src,
				      size_t size)
{
	if (unlikely(!access_ok(dst, size) ||
	    __xn_copy_to_user(dst, src, size)))
		return -EFAULT;
	return 0;
}

static inline int cobalt_strncpy_from_user(char *dst, const char __user *src,
					   size_t count)
{
	if (unlikely(!access_ok(src, 1)))
		return -EFAULT;

	return __xn_strncpy_from_user(dst, src, count);
}


/*
 * NOTE: those copy helpers won't work in compat mode: use
 * sys32_get_*(), sys32_put_*() instead.
 */

static inline int
cobalt_get_old_timespec(struct timespec64 *dst,
			const struct __kernel_old_timespec __user *src)
{
	struct __kernel_old_timespec u_ts;
	int ret;

	ret = cobalt_copy_from_user(&u_ts, src, sizeof(u_ts));
	if (ret)
		return ret;

	dst->tv_sec = u_ts.tv_sec;
	dst->tv_nsec = u_ts.tv_nsec;

	return 0;
}

static inline int
cobalt_put_old_timespec(struct __kernel_old_timespec __user *dst,
			const struct timespec64 *src)
{
	struct __kernel_old_timespec u_ts;
	int ret;

	u_ts.tv_sec = src->tv_sec;
	u_ts.tv_nsec = src->tv_nsec;

	ret = cobalt_copy_to_user(dst, &u_ts, sizeof(*dst));
	if (ret)
		return ret;

	return 0;
}

static inline int
cobalt_get_itimerspec32(struct itimerspec64 *dst,
			const struct old_itimerspec32 __user *src)
{
	struct old_itimerspec32 u_its;
	int ret;

	ret = cobalt_copy_from_user(&u_its, src, sizeof(u_its));
	if (ret)
		return ret;

	dst->it_interval.tv_sec = u_its.it_interval.tv_sec;
	dst->it_interval.tv_nsec = u_its.it_interval.tv_nsec;
	dst->it_value.tv_sec = u_its.it_value.tv_sec;
	dst->it_value.tv_nsec = u_its.it_value.tv_nsec;

	return 0;
}

static inline int cobalt_put_itimerspec32(struct old_itimerspec32 __user *dst,
					  const struct itimerspec64 *src)
{
	struct old_itimerspec32 u_its;

	u_its.it_interval.tv_sec = src->it_interval.tv_sec;
	u_its.it_interval.tv_nsec = src->it_interval.tv_nsec;
	u_its.it_value.tv_sec = src->it_value.tv_sec;
	u_its.it_value.tv_nsec = src->it_value.tv_nsec;

	return cobalt_copy_to_user(dst, &u_its, sizeof(*dst));
}

static inline struct timespec64
cobalt_timeval_to_timespec64(const struct __kernel_old_timeval *src)
{
	struct timespec64 ts;

	ts.tv_sec = src->tv_sec + (src->tv_usec / USEC_PER_SEC);
	ts.tv_nsec = (src->tv_usec % USEC_PER_SEC) * NSEC_PER_USEC;

	return ts;
}

static inline struct __kernel_old_timeval
cobalt_timespec64_to_timeval(const struct timespec64 *src)
{
	struct __kernel_old_timeval tv;

	tv.tv_sec = src->tv_sec + (src->tv_nsec / NSEC_PER_SEC);
	tv.tv_usec = (src->tv_nsec % NSEC_PER_SEC) / NSEC_PER_USEC;

	return tv;
}

/* 32bit syscall emulation */
#define __COBALT_COMPAT_BIT	0x1
/* 32bit syscall emulation - extended form */
#define __COBALT_COMPATX_BIT	0x2

#endif /* !_COBALT_ASM_GENERIC_SYSCALL_H */
