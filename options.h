/* Copyright lynix <lynix47@gmail.com>, 2009, 2010
 * 
 * This file is part of vcp (verbose cp).
 *
 * vcp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vcp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vcp. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _options_h
#define _options_h

#define BUFFS 1048576		/* 1MiB buffer for read() and write() 		*/
#define BUFFM 10            /* buffer multiplier, see work_list()       */
#define SPEED_N 5			/* speed middle calculation 				*/
#define BAR_WIDTH 20		/* progress bar width (characters) 			*/
#define MAX_SIZE_L 15		/* maximum length of size string, numbers 	*/


struct options {
	char bars;
	char force;
	char filenames;
	char sync;
	char delete;
	char keep;
	char quiet;
	char update;
	char verbose;
    char pretend;
	char debug;
    char ignore_uid_err;
};
#endif
