/*
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Started:      2000-05-02
 * 
 * Changes:
 * 
 * 2000-05-02:
 * 
 * - inital version
 * 
 */

# include "io.h"
# include "xhdi.h"

# include <assert.h>
# include <stdio.h>
# include <stdlib.h>
# include <stdarg.h>

# include <mintbind.h>


static int16_t drv = -1;
static u_int16_t major = 0;
static u_int16_t minor = 0;
static u_int32_t start = 0;
static u_int32_t sectors = 0;
static u_int32_t ssize = 0;

static int64_t pos = 0;


static long
rwabs_xhdi (ushort rw, void *buf, ulong size, ulong recno)
{
	register ulong n = size / ssize;
	
	if (!n || (recno + n) > sectors)
	{
		printf ("rwabs_xhdi: access outside partition (drv = %c:)\n", 'A'+drv);
		exit (2);
	}
	
	if (n > 65535UL)
	{
		printf ("rwabs_xhdi: n to large (drv = %c)\n", 'A'+drv);
		exit (2);
	}
	
	return XHReadWrite (major, minor, rw, start + recno, n, buf);
}


int
io_init (void)
{
	if (XHDI_init ())
	{
		printf ("No XHDI installed (or to old)!\n");
		return -1;
	}
	
	printf ("Found XHDI level %x.%x (%x).\n\n", (XHDI_installed >> 8), (XHDI_installed & 0x00ff), XHDI_installed);
	return 0;
}

int
io_open (int64_t _dev)
{
	int16_t dev;
	int32_t r;
	
	if (_dev < 0 || _dev > 31)
	{
		printf ("invalid device!\n");
		return -1;
	}
	
	if (drv != -1)
	{
		printf ("can only open one device at a time!\n");
		return -1;
	}
	
	dev = _dev;
	
	r = Dlock (1, dev);
	if (r && r != -32)
	{
		printf ("Can't lock %c:, drive in use?\n", 'A'+dev);
		return -1;
	}
	
	r = XHInqDev2 (dev, &major, &minor, &start, NULL, &sectors, NULL);
	if (r == 0)
		r = XHInqTarget2 (major, minor, &ssize, NULL, NULL, 0);
	else
		printf ("XHInqDev2 = %li", r);
	
	if (r)
	{
		printf ("unable to get geometry for '%c' (%li)\n", 'A'+dev, r);
		return -1;
	}
	
	printf ("Information about %c:\n", 'A'+dev);
	printf ("---------------------\n");
	printf ("XHDI major number    : %d\n", major);
	printf ("XHDI minor number    : %d\n", minor);
	printf ("partition start      : %ld\n", start);
	printf ("partition length     : %ld sectors\n", sectors);
	printf ("physical sector size : %ld bytes\n", ssize);
	printf ("\n");
	
	/* mark as open */
	drv = dev;
	
	return drv;
}

int
io_close (int handle)
{
	int32_t ret;
	
	if (handle != drv)
		return -1;
	
	ret = Dlock (0, drv);
	drv = -1;
	
	return 0;
}

int
io_ioctrl (int handle, int mode, void *buf)
{
	switch (mode)
	{
# define PSECSIZE 1
		case PSECSIZE:
			*(int32_t *) buf = ssize;
			break;
		default:
			return -1;
	}
	
	return 0;
}

int32_t
io_read (int handle, void *buf, int32_t size)
{
	u_int32_t recno;
	int32_t ret;
	
	if (handle != drv)
		return -1;
	
	assert ((size % ssize) == 0);
	assert ((pos % ssize) == 0);
	
	recno = pos / ssize;
	
	ret = rwabs_xhdi (0, buf, size, recno);
	if (!ret)
	{
		pos += size;
		return size;
	}
	
	return -1;
}

int32_t
io_write (int handle, void *buf, int32_t size)
{
	u_int32_t recno;
	int32_t ret;
	
	if (handle != drv)
		return -1;
	
	assert ((size % ssize) == 0);
	assert ((pos % ssize) == 0);
	
	recno = pos / ssize;
	
	ret = rwabs_xhdi (1, buf, size, recno);
	if (!ret)
	{
		pos += size;
		return size;
	}
	
	return -1;
}

int64_t
io_seek (int handle, int whence, int64_t where)
{
	if (handle != drv)
		return -1;
	
	switch (whence)
	{
		case SEEK_SET:
			break;
		case SEEK_CUR:
			where += pos;
			break;
		case SEEK_END:
			where += (int64_t) sectors * ssize;
			break;
		default:
			return -1;
	}
	
	if (where % ssize)
		return -1;
	
	if (where > (int64_t) sectors * ssize)
		return -1;
	
	pos = where;
	return pos;
}
