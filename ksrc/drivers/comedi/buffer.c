/**
 * @file
 * Comedi for RTDM, buffer related features
 *
 * Copyright (C) 1997-2000 David A. Schleef <ds@schleef.org>
 * Copyright (C) 2008 Alexis Berlemont <alexis.berlemont@free.fr>
 *
 * Xenomai is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DOXYGEN_CPP

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mman.h>
#include <linux/vmalloc.h>
#include <asm/errno.h>
#include <rtdm/rtdm_driver.h>
#include <comedi/context.h>
#include <comedi/device.h>
#include <comedi/buffer.h>
#include <comedi/transfer.h>

/* --- Buffer allocation / free functions --- */

void comedi_free_buffer(comedi_buf_t * buf_desc)
{
	if (buf_desc->pg_list != NULL) {
		rtdm_free(buf_desc->pg_list);
		buf_desc->pg_list = NULL;
	}

	if (buf_desc->buf != NULL) {
		char *vaddr, *vabase = buf_desc->buf;
		for (vaddr = vabase; vaddr < vabase + buf_desc->size;
		     vaddr += PAGE_SIZE)
			ClearPageReserved(vmalloc_to_page(vaddr));
		vfree(buf_desc->buf);
		buf_desc->buf = NULL;
	}
}

int comedi_alloc_buffer(comedi_buf_t * buf_desc)
{
	int ret = 0;
	char *vaddr, *vabase;

	if (buf_desc->size == 0)
		buf_desc->size = COMEDI_BUF_DEFSIZE;

	buf_desc->size = PAGE_ALIGN(buf_desc->size);

	buf_desc->buf = vmalloc(buf_desc->size);

	if (buf_desc->buf == NULL) {
		ret = -ENOMEM;
		goto out_virt_contig_alloc;
	}

	vabase = buf_desc->buf;

	for (vaddr = vabase; vaddr < vabase + buf_desc->size;
	     vaddr += PAGE_SIZE)
		SetPageReserved(vmalloc_to_page(vaddr));

	buf_desc->pg_list = rtdm_malloc(((buf_desc->size) >> PAGE_SHIFT) *
					   sizeof(unsigned long));
	if (buf_desc->pg_list == NULL) {
		ret = -ENOMEM;
		goto out_virt_contig_alloc;
	}

	for (vaddr = vabase; vaddr < vabase + buf_desc->size;
	     vaddr += PAGE_SIZE)
		buf_desc->pg_list[(vaddr - vabase) >> PAGE_SHIFT] =
			(unsigned long) page_to_phys(vmalloc_to_page(vaddr));

      out_virt_contig_alloc:
	if (ret != 0)
		comedi_free_buffer(buf_desc);

	return ret;
}

/* --- Current Command management function --- */

comedi_cmd_t *comedi_get_cmd(comedi_subd_t *subd)
{
	comedi_dev_t *dev = subd->dev;

	/* Check that subdevice supports commands */
	if (dev->transfer.bufs == NULL)
		return NULL;

	return dev->transfer.bufs[subd->idx]->cur_cmd;
}

/* --- Munge related function --- */

int comedi_get_chan(comedi_subd_t *subd)
{
	comedi_dev_t *dev = subd->dev;
	int i, tmp_count, tmp_size = 0;	
	comedi_cmd_t *cmd;

	/* Check that subdevice supports commands */
	if (dev->transfer.bufs == NULL)
		return -EINVAL;

	/* Check a command is executed */
	if (dev->transfer.bufs[subd->idx]->cur_cmd == NULL)
		return -EINVAL;

	/* Retrieve the proper command descriptor */
	cmd = dev->transfer.bufs[subd->idx]->cur_cmd;

	/* There is no need to check the channel idx, 
	   it has already been controlled in command_test */

	/* We assume channels can have different sizes;
	   so, we have to compute the global size of the channels
	   in this command... */
	for (i = 0; i < cmd->nb_chan; i++)
		tmp_size += dev->transfer.subds[subd->idx]->chan_desc->
			chans[CR_CHAN(cmd->chan_descs[i])].nb_bits;

	/* Translation bits -> bytes */
	tmp_size /= 8;

	tmp_count = dev->transfer.bufs[subd->idx]->mng_count % tmp_size;

	/* Translation bytes -> bits */
	tmp_count *= 8;

	/* ...and find the channel the last munged sample 
	   was related with */
	for (i = 0; tmp_count > 0 && i < cmd->nb_chan; i++)
		tmp_count -= dev->transfer.subds[subd->idx]->chan_desc->
			chans[CR_CHAN(cmd->chan_descs[i])].nb_bits;

	if (tmp_count == 0)
		return i;
	else
		return -EINVAL;
}

/* --- Transfer / copy functions --- */

int comedi_buf_prepare_absput(comedi_subd_t *subd, unsigned long count)
{
	comedi_dev_t *dev;
	comedi_buf_t *buf;
	
	if ((subd->flags & COMEDI_SUBD_MASK_READ) == 0)
		return -EINVAL;

	dev = subd->dev;
	buf = dev->transfer.bufs[subd->idx];

	return __pre_abs_put(buf, count);
}


int comedi_buf_commit_absput(comedi_subd_t *subd, unsigned long count)
{
	comedi_dev_t *dev;
	comedi_buf_t *buf;
	
	if ((subd->flags & COMEDI_SUBD_MASK_READ) == 0)
		return -EINVAL;

	dev = subd->dev;
	buf = dev->transfer.bufs[subd->idx];

	return __abs_put(buf, count);
}

int comedi_buf_prepare_put(comedi_subd_t *subd, unsigned long count)
{
	comedi_dev_t *dev;
	comedi_buf_t *buf;
	
	if ((subd->flags & COMEDI_SUBD_MASK_READ) == 0)
		return -EINVAL;

	dev = subd->dev;
	buf = dev->transfer.bufs[subd->idx];

	return __pre_put(buf, count);
}

int comedi_buf_commit_put(comedi_subd_t *subd, unsigned long count)
{
	comedi_dev_t *dev;
	comedi_buf_t *buf;
	
	if ((subd->flags & COMEDI_SUBD_MASK_READ) == 0)
		return -EINVAL;

	dev = subd->dev;
	buf = dev->transfer.bufs[subd->idx];

	return __put(buf, count);	
}

int comedi_buf_put(comedi_subd_t *subd, void *bufdata, unsigned long count)
{
	int err;
	comedi_dev_t *dev;
	comedi_buf_t *buf;
	
	if ((subd->flags & COMEDI_SUBD_MASK_READ) == 0)
		return -EINVAL;

	dev = subd->dev;
	buf = dev->transfer.bufs[subd->idx];

	if (__count_to_put(buf) < count)
		return -EAGAIN;

	err = __produce(NULL, buf, bufdata, count);
	if (err < 0)
		return err;	

	err = __put(buf, count);

	return err;
}

int comedi_buf_prepare_absget(comedi_subd_t *subd, unsigned long count)
{
	comedi_dev_t *dev;
	comedi_buf_t *buf;
	
	if ((subd->flags & COMEDI_SUBD_MASK_WRITE) == 0)
		return -EINVAL;

	dev = subd->dev;
	buf = dev->transfer.bufs[subd->idx];

	return __pre_abs_get(buf, count);
}

int comedi_buf_commit_absget(comedi_subd_t *subd, unsigned long count)
{
	comedi_dev_t *dev;
	comedi_buf_t *buf;
	
	if ((subd->flags & COMEDI_SUBD_MASK_WRITE) == 0)
		return -EINVAL;

	dev = subd->dev;
	buf = dev->transfer.bufs[subd->idx];

	return __abs_get(buf, count);
}

int comedi_buf_prepare_get(comedi_subd_t *subd, unsigned long count)
{
	comedi_dev_t *dev;
	comedi_buf_t *buf;
	
	if ((subd->flags & COMEDI_SUBD_MASK_WRITE) == 0)
		return -EINVAL;

	dev = subd->dev;
	buf = dev->transfer.bufs[subd->idx];

	return __pre_get(buf, count);
}

int comedi_buf_commit_get(comedi_subd_t *subd, unsigned long count)
{
	comedi_dev_t *dev;
	comedi_buf_t *buf;
	
	if ((subd->flags & COMEDI_SUBD_MASK_WRITE) == 0)
		return -EINVAL;

	dev = subd->dev;
	buf = dev->transfer.bufs[subd->idx];

	return __get(buf, count);
}

int comedi_buf_get(comedi_subd_t *subd, void *bufdata, unsigned long count)
{
	int err;
	comedi_dev_t *dev;
	comedi_buf_t *buf;
	
	if ((subd->flags & COMEDI_SUBD_MASK_WRITE) == 0)
		return -EINVAL;

	dev = subd->dev;
	buf = dev->transfer.bufs[subd->idx];

	if (__count_to_get(buf) < count)
		return -EAGAIN;

	err = __consume(NULL, buf, bufdata, count);
	if (err < 0)
		return err;

	err = __get(buf, count);

	return err;
}

int comedi_buf_evt(comedi_subd_t *subd, unsigned long evts)
{
	comedi_dev_t *dev = subd->dev;
	comedi_buf_t *buf = dev->transfer.bufs[subd->idx];
	int tmp;

	/* Basic checking */
	if (!test_bit(COMEDI_TSF_BUSY, &(dev->transfer.status[subd->idx])))
		return -ENOENT;

	/* Even if it is a little more complex,
	   atomic operations are used so as 
	   to prevent any kind of corner case */
	while ((tmp = ffs(evts) - 1) != -1) {
		set_bit(tmp, &buf->evt_flags);
		clear_bit(tmp, &evts);
	}

	/* Notifies the user-space side */
	comedi_signal_sync(&buf->sync);

	return 0;
}

unsigned long comedi_buf_count(comedi_subd_t *subd)
{
	unsigned long ret = 0;
	comedi_dev_t *dev = subd->dev;
	comedi_buf_t *buf = dev->transfer.bufs[subd->idx];

	if (subd->flags & COMEDI_SUBD_MASK_READ)
		ret = __count_to_put(buf);
	else if (subd->flags & COMEDI_SUBD_MASK_WRITE)
		ret = __count_to_get(buf);	

	return ret;
}

/* --- Mmap functions --- */

void comedi_map(struct vm_area_struct *area)
{
	unsigned long *status = (unsigned long *)area->vm_private_data;
	set_bit(COMEDI_TSF_MMAP, status);
}

void comedi_unmap(struct vm_area_struct *area)
{
	unsigned long *status = (unsigned long *)area->vm_private_data;
	clear_bit(COMEDI_TSF_MMAP, status);
}

static struct vm_operations_struct comedi_vm_ops = {
      open:comedi_map,
      close:comedi_unmap,
};

int comedi_ioctl_mmap(comedi_cxt_t * cxt, void *arg)
{
	comedi_mmap_t map_cfg;
	comedi_dev_t *dev;
	int ret;

	__comedi_dbg(1, core_dbg, 
		     "comedi_ioctl_mmap: minor=%d\n", comedi_get_minor(cxt));

	dev = comedi_get_dev(cxt);

	/* Basically checks the device */
	if (!test_bit(COMEDI_DEV_ATTACHED, &dev->flags))
		return -EINVAL;

	/* The mmap operation cannot be performed in a 
	   real-time context */
	if (comedi_test_rt() != 0)
		return -EPERM;

	/* Recovers the argument structure */
	if (comedi_copy_from_user(cxt,
				  &map_cfg, arg, sizeof(comedi_mmap_t)) != 0)
		return -EFAULT;

	/* Checks the subdevice */
	if (map_cfg.idx_subd >= dev->transfer.nb_subd ||
	    (dev->transfer.subds[map_cfg.idx_subd]->flags & COMEDI_SUBD_CMD) ==
	    0
	    || (dev->transfer.subds[map_cfg.idx_subd]->
		flags & COMEDI_SUBD_MMAP) == 0)
		return -EINVAL;

	/* Checks the buffer is not already mapped */
	if (test_bit(COMEDI_TSF_MMAP,
		     &(dev->transfer.status[map_cfg.idx_subd])))
		return -EBUSY;

	/* Basically checks the size to be mapped */
	if ((map_cfg.size & ~(PAGE_MASK)) != 0 ||
	    map_cfg.size > dev->transfer.bufs[map_cfg.idx_subd]->size)
		return -EFAULT;

	ret = rtdm_mmap_to_user(cxt->rtdm_usrinf,
				dev->transfer.bufs[map_cfg.idx_subd]->buf,
				map_cfg.size,
				PROT_READ | PROT_WRITE,
				&map_cfg.ptr,
				&comedi_vm_ops,
				&(dev->transfer.status[map_cfg.idx_subd]));

	if (ret < 0)
		return ret;

	return comedi_copy_to_user(cxt, arg, &map_cfg, sizeof(comedi_mmap_t));
}

/* --- IOCTL / FOPS functions --- */

int comedi_ioctl_bufcfg(comedi_cxt_t * cxt, void *arg)
{
	comedi_dev_t *dev = comedi_get_dev(cxt);
	comedi_bufcfg_t buf_cfg;

	__comedi_dbg(1, core_dbg, 
		     "comedi_ioctl_bufcfg: minor=%d\n", comedi_get_minor(cxt));

	/* Basic checking */
	if (!test_bit(COMEDI_DEV_ATTACHED, &dev->flags))
		return -EINVAL;

	/* As Linux API is used to allocate a virtual buffer,
	   the calling process must not be in primary mode */
	if (comedi_test_rt() != 0)
		return -EPERM;

	if (comedi_copy_from_user(cxt,
				  &buf_cfg, arg, sizeof(comedi_bufcfg_t)) != 0)
		return -EFAULT;

	if (buf_cfg.idx_subd >= dev->transfer.nb_subd)
		return -EINVAL;

	if (buf_cfg.buf_size > COMEDI_BUF_MAXSIZE)
		return -EINVAL;

	/* If a transfer is occuring or if the buffer is mmapped,
	   no buffer size change is allowed */
	if (test_bit(COMEDI_TSF_BUSY,
		     &(dev->transfer.status[buf_cfg.idx_subd])))
		return -EBUSY;

	if (test_bit(COMEDI_TSF_MMAP,
		     &(dev->transfer.status[buf_cfg.idx_subd])))
		return -EPERM;

	/* Performs the re-allocation */
	comedi_free_buffer(dev->transfer.bufs[buf_cfg.idx_subd]);

	dev->transfer.bufs[buf_cfg.idx_subd]->size = buf_cfg.buf_size;

	return comedi_alloc_buffer(dev->transfer.bufs[buf_cfg.idx_subd]);
}

int comedi_ioctl_bufinfo(comedi_cxt_t * cxt, void *arg)
{
	comedi_dev_t *dev = comedi_get_dev(cxt);
	comedi_bufinfo_t info;
	comedi_buf_t *buf;
	unsigned long tmp_cnt;
	int ret;

	__comedi_dbg(1, core_dbg, 
		     "comedi_ioctl_bufinfo: minor=%d\n", comedi_get_minor(cxt));

	/* Basic checking */
	if (!test_bit(COMEDI_DEV_ATTACHED, &dev->flags))
		return -EINVAL;

	if (comedi_copy_from_user(cxt,
				  &info, arg, sizeof(comedi_bufinfo_t)) != 0)
		return -EFAULT;

	if (info.idx_subd > dev->transfer.nb_subd)
		return -EINVAL;

	if ((dev->transfer.subds[info.idx_subd]->flags & COMEDI_SUBD_CMD) == 0)
		return -EINVAL;

	buf = dev->transfer.bufs[info.idx_subd];

	ret = __handle_event(buf);

	if (info.idx_subd == dev->transfer.idx_read_subd) {

		/* Updates consume count if rw_count is not null */
		if (info.rw_count != 0)
			buf->cns_count += info.rw_count;

		/* Retrieves the data amount to read */
		tmp_cnt = info.rw_count = __count_to_get(buf);

		__comedi_dbg(1, core_dbg, 
			     "comedi_ioctl_bufinfo: count to read=%lu\n", tmp_cnt);

		if ((ret < 0 && ret != -ENOENT) ||
		    (ret == -ENOENT && tmp_cnt == 0)) {
			comedi_cancel_transfer(cxt, info.idx_subd);
			return ret;
		}
	} else if (info.idx_subd == dev->transfer.idx_write_subd) {

		if (ret < 0) {
			comedi_cancel_transfer(cxt, info.idx_subd);
			if (info.rw_count != 0)
				return ret;
		}

		/* If rw_count is not null, 
		   there is something to write / munge  */
		if (info.rw_count != 0 && info.rw_count <= __count_to_put(buf)) {

			/* Updates the production pointer */
			buf->prd_count += info.rw_count;

			/* Sets the munge count */
			tmp_cnt = info.rw_count;
		} else
			tmp_cnt = 0;

		/* Retrieves the data amount which is writable */
		info.rw_count = __count_to_put(buf);

		__comedi_dbg(1, core_dbg, 
			     "comedi_ioctl_bufinfo: count to write=%lu\n", 
			     info.rw_count);

	} else
		return -EINVAL;

	/* Performs the munge if need be */
	if (dev->transfer.subds[info.idx_subd]->munge != NULL) {
		__munge(dev->transfer.subds[info.idx_subd],
			dev->transfer.subds[info.idx_subd]->munge,
			buf, tmp_cnt);

		/* Updates munge count */
		buf->mng_count += tmp_cnt;
	}

	/* Sets the buffer size */
	info.buf_size = buf->size;

	/* Sends the structure back to user space */
	if (comedi_copy_to_user(cxt, arg, &info, sizeof(comedi_bufinfo_t)) != 0)
		return -EFAULT;

	return 0;
}

ssize_t comedi_read(comedi_cxt_t * cxt, void *bufdata, size_t nbytes)
{
	comedi_dev_t *dev = comedi_get_dev(cxt);
	int idx_subd = dev->transfer.idx_read_subd;
	comedi_buf_t *buf = dev->transfer.bufs[idx_subd];
	ssize_t count = 0;

	/* Basic checkings */
	if (!test_bit(COMEDI_DEV_ATTACHED, &dev->flags))
		return -EINVAL;

	if (!test_bit(COMEDI_TSF_BUSY, &(dev->transfer.status[idx_subd])))
		return -ENOENT;

	/* Checks the subdevice capabilities */
	if ((dev->transfer.subds[idx_subd]->flags & COMEDI_SUBD_CMD) == 0)
		return -EINVAL;

	while (count < nbytes) {

		/* Checks the events */
		int ret = __handle_event(buf);

		/* Computes the data amount to copy */
		unsigned long tmp_cnt = __count_to_get(buf);

		/* Checks tmp_cnt count is not higher than
		   the global count to read */
		if (tmp_cnt > nbytes - count)
			tmp_cnt = nbytes - count;

		if ((ret < 0 && ret != -ENOENT) ||
		    (ret == -ENOENT && tmp_cnt == 0)) {
			comedi_cancel_transfer(cxt, idx_subd);
			count = ret;
			goto out_comedi_read;
		}

		if (tmp_cnt > 0) {

			/* Performs the munge if need be */
			if (dev->transfer.subds[idx_subd]->munge != NULL) {
				__munge(dev->transfer.subds[idx_subd],
					dev->transfer.subds[idx_subd]->munge,
					buf, tmp_cnt);

				/* Updates munge count */
				buf->mng_count += tmp_cnt;
			}

			/* Performs the copy */
			ret = __consume(cxt, buf, bufdata + count, tmp_cnt);

			if (ret < 0) {
				count = ret;
				goto out_comedi_read;
			}

			/* Updates consume count */
			buf->cns_count += tmp_cnt;

			/* Updates the return value */
			count += tmp_cnt;

			/* If the driver does not work in bulk mode,
			   we must leave this function */
			if (!test_bit(COMEDI_TSF_BULK,
				      &(dev->transfer.status[idx_subd])))
				goto out_comedi_read;
		}
		/* If the acquisition is not over, we must not
		   leave the function without having read a least byte */
		else if (ret != -ENOENT) {
			ret = comedi_wait_sync(&(buf->sync), comedi_test_rt());
			if (ret < 0) {
				if (ret == -ERESTARTSYS)
					ret = -EINTR;
				count = ret;
				goto out_comedi_read;
			}
		}
	}

      out_comedi_read:

	return count;
}

ssize_t comedi_write(comedi_cxt_t *cxt, 
		     const void *bufdata, size_t nbytes)
{
	comedi_dev_t *dev = comedi_get_dev(cxt);
	int idx_subd = dev->transfer.idx_write_subd;
	comedi_buf_t *buf = dev->transfer.bufs[idx_subd];
	ssize_t count = 0;

	/* Basic checkings */
	if (!test_bit(COMEDI_DEV_ATTACHED, &dev->flags))
		return -EINVAL;

	if (!test_bit(COMEDI_TSF_BUSY, &(dev->transfer.status[idx_subd])))
		return -ENOENT;

	/* Checks the subdevice capabilities */
	if ((dev->transfer.subds[idx_subd]->flags & COMEDI_SUBD_CMD) == 0)
		return -EINVAL;

	while (count < nbytes) {

		/* Checks the events */
		int ret = __handle_event(buf);

		/* Computes the data amount to copy */
		unsigned long tmp_cnt = __count_to_put(buf);

		/* Checks tmp_cnt count is not higher than
		   the global count to write */
		if (tmp_cnt > nbytes - count)
			tmp_cnt = nbytes - count;

		if (ret < 0) {
			comedi_cancel_transfer(cxt, idx_subd);
			count = (ret == -ENOENT) ? -EINVAL : ret;
			goto out_comedi_write;
		}

		if (tmp_cnt > 0) {

			/* Performs the copy */
			ret =
			    __produce(cxt, buf, (void *)bufdata + count,
				      tmp_cnt);
			if (ret < 0) {
				count = ret;
				goto out_comedi_write;
			}

			/* Performs the munge if need be */
			if (dev->transfer.subds[idx_subd]->munge != NULL) {
				__munge(dev->transfer.subds[idx_subd],
					dev->transfer.subds[idx_subd]->munge,
					buf, tmp_cnt);

				/* Updates munge count */
				buf->mng_count += tmp_cnt;
			}

			/* Updates produce count */
			buf->prd_count += tmp_cnt;

			/* Updates the return value */
			count += tmp_cnt;

			/* If the driver does not work in bulk mode,
			   we must leave this function */
			if (!test_bit(COMEDI_TSF_BULK,
				      &(dev->transfer.status[idx_subd])))
				goto out_comedi_write;
		} else {
			/* The buffer is full, we have to wait for a slot to free */
			ret = comedi_wait_sync(&(buf->sync), comedi_test_rt());
			if (ret < 0) {
				if (ret == -ERESTARTSYS)
					ret = -EINTR;
				count = ret;
				goto out_comedi_write;
			}
		}
	}

      out_comedi_write:

	return count;
}

int comedi_select(comedi_cxt_t *cxt, 
		  rtdm_selector_t *selector,
		  enum rtdm_selecttype type, unsigned fd_index)
{
	comedi_dev_t *dev = comedi_get_dev(cxt);
	int idx_subd = (type == RTDM_SELECTTYPE_READ) ? 
		dev->transfer.idx_read_subd :
		dev->transfer.idx_write_subd;
	comedi_buf_t *buf = dev->transfer.bufs[idx_subd];

	/* Checks the RTDM select type 
	   (RTDM_SELECTTYPE_EXCEPT is not supported) */
	if(type != RTDM_SELECTTYPE_READ && 
	   type != RTDM_SELECTTYPE_WRITE)
		return -EINVAL;

	/* Basic checkings */
	if (!test_bit(COMEDI_DEV_ATTACHED, &dev->flags))
		return -EINVAL;

	if (!test_bit(COMEDI_TSF_BUSY, &(dev->transfer.status[idx_subd])))
		return -ENOENT;	

	/* Checks the subdevice capabilities */
	if ((dev->transfer.subds[idx_subd]->flags & COMEDI_SUBD_CMD) == 0)
		return -EINVAL;

	/* Performs a bind on the Comedi synchronization element */
	return comedi_select_sync(&(buf->sync), selector, type, fd_index);	
}

int comedi_ioctl_poll(comedi_cxt_t * cxt, void *arg)
{
	int ret = 0;
	unsigned long tmp_cnt = 0;
	comedi_dev_t *dev = comedi_get_dev(cxt);
	comedi_buf_t *buf;
	comedi_poll_t poll;

	/* Basic checking */
	if (!test_bit(COMEDI_DEV_ATTACHED, &dev->flags))
		return -EINVAL;

	if (comedi_copy_from_user(cxt, &poll, arg, sizeof(comedi_poll_t)) != 0)
		return -EFAULT;

	/* Checks the subdevice capabilities */
	if ((dev->transfer.subds[poll.idx_subd]->flags &
	     COMEDI_SUBD_CMD) == 0 ||
	    (dev->transfer.subds[poll.idx_subd]->flags &
	     COMEDI_SUBD_MASK_SPECIAL) != 0)
		return -EINVAL;

	/* Checks a transfer is occuring */
	if (!test_bit(COMEDI_TSF_BUSY, &(dev->transfer.status[poll.idx_subd])))
		return -EINVAL;

	buf = dev->transfer.bufs[poll.idx_subd];

	/* Checks the buffer events */
	ret = __handle_event(buf);

	/* Retrieves the data amount to compute 
	   according to the subdevice type */
	if ((dev->transfer.subds[poll.idx_subd]->flags &
	     COMEDI_SUBD_MASK_READ) != 0) {

		tmp_cnt = __count_to_get(buf);

		/* If some error occured... */
		if ((ret < 0 && ret != -ENOENT) ||
		    /* ...or if we reached the end of the input transfer... */
		    (ret == -ENOENT && tmp_cnt == 0)) {
			/* ...cancel the transfer */
			comedi_cancel_transfer(cxt, poll.idx_subd);
			return ret;
		}
	} else {

		/* If some error was detected, cancel the transfer */
		if (ret < 0) {
			comedi_cancel_transfer(cxt, poll.idx_subd);
			return ret;
		}

		tmp_cnt = __count_to_put(buf);
	}

	if (poll.arg == COMEDI_NONBLOCK || tmp_cnt != 0)
		goto out_poll;

	if (poll.arg == COMEDI_INFINITE)
		ret = comedi_wait_sync(&(buf->sync), comedi_test_rt());
	else {
		unsigned long long ns = ((unsigned long long)poll.arg) *
		    ((unsigned long long)NSEC_PER_MSEC);
		ret = comedi_timedwait_sync(&(buf->sync), comedi_test_rt(), ns);
	}

	if (ret == 0) {
		/* Retrieves the count once more */
		if ((dev->transfer.subds[poll.idx_subd]->flags &
		     COMEDI_SUBD_MASK_READ) != 0)
			tmp_cnt = __count_to_get(buf);
		else
			tmp_cnt = __count_to_put(buf);
	}

      out_poll:

	poll.arg = tmp_cnt;

	/* Sends the structure back to user space */
	ret = comedi_copy_to_user(cxt, arg, &poll, sizeof(comedi_poll_t));

	return ret;
}

#endif /* !DOXYGEN_CPP */
