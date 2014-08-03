/*
 * Copyright (C) 2014-2015  liangdong <liangdong01@baidu.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "smem.h"

int main(int argc, char **argv)
{
	int size = 1000;
	int count = 100000;

	void **arr = calloc(count, sizeof(*arr));
	assert(arr);

	int i;
	for (i = 0; i < count; ++i) {
		arr[i] = malloc(size);
		assert(arr[i]);
	}
	smem_stat();

	for (i = 0; i < count; ++i)
		free(arr[i]);

	smem_stat();

	return 0;
}
