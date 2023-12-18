/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Char <yanjianbo@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __API_H__
#define __API_H__

/*
 *    function:    write the key data to secure storage and private partition
 *    input    :    item_name    : key name
 *                buffer         :        key data
 *                length        :        key lent
 *    ruturn    :    0: seccure  -1: fail
 */
int private_data_write(const char *item_name, char *buffer, int length);


/*
 *    function:    get key data from secure storage
 *    input    :    item_name    : key name
 *                buffer         :
 *                buffer_len    :
 *                 data_len    : key lenth  (get key lenth from private partition or secure storage)
 *    ruturn    :    0: seccure  -1: fail
 */
int private_data_read(const char *item_name, char *buffer, int buffer_len, int*data_len);

#endif