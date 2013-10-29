/*
 * This module contain functions to traverse through the dir tree and associate 
 * the inotify instance to watch on the dir, for SNAPSync.
 *
 * Copyright (C) 2013 Garvit Sharma <garvits45@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, visit the http://fsf.org website.

*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#include "user_sync.h"
 #include "inotify.h"

enum {
	WALK_OK = 0,
	WALK_BADPATTERN,
	WALK_NAMETOOLONG,
	WALK_BADIO,
};
 
#define WS_NONE		0
#define WS_RECURSIVE	(1 << 0)
#define WS_DEFAULT	WS_RECURSIVE
#define WS_FOLLOWLINK	(1 << 1)	/* follow symlinks */
#define WS_DOTFILES	(1 << 2)	/* per unix convention, .file is hidden */
#define WS_MATCHDIRS	(1 << 3)	/* if pattern is used on dir names too */
 
static int walk_recur(char *dname)
{
	struct dirent *dent;
	DIR *dir;
	struct stat st;
	char fn[FILENAME_MAX];
	int res = WALK_OK, wd1;
	int len = strlen(dname);
	if (len >= FILENAME_MAX - 1)
		return WALK_NAMETOOLONG;
 
	strcpy(fn, dname);
	if(fn[len-1] != '/')
		fn[len++] = '/';
 
	if (!(dir = opendir(dname))) {
		warn("can't open %s", dname);
		return WALK_BADIO;
	}
 
	errno = 0;
	while ((dent = readdir(dir))) {
		if (dent->d_name[0] == '.')
			continue;
		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
			continue;
 
		strncpy(fn + len, dent->d_name, FILENAME_MAX - len);
		if (lstat(fn, &st) == -1) {
			warn("Can't stat %s", fn);
			//res = WALK_BADIO;
			continue;
		}
 
		if (S_ISLNK(st.st_mode))
			continue;
 
		/* will be false for symlinked dirs */
		if (S_ISDIR(st.st_mode)) {
			*(fn+len+strlen(dent->d_name))='/';
			walk_recur(fn);
			wd1 = inotify_add_watch(infd, fn, standard_event_mask);
			prefix[wd1]=(char *)calloc(strlen(fn), sizeof(char *));
			strcpy(prefix[wd1], fn);
 		}
	}
 
	if (dir) closedir(dir);
	return res ? res : errno ? WALK_BADIO : WALK_OK;
}
 

int traverse_main(char *dir)
{
	int r = walk_recur(dir);
	switch(r) {
	case WALK_OK:		break;
	case WALK_BADIO:	print_err("IO error"); exit(1);

	case WALK_BADPATTERN:	print_err("Bad pattern"); exit(1);

	case WALK_NAMETOOLONG:	print_err("Filename too long"); exit(1);

	default:
		print_err("Unknown error?"); exit(1);
	}
	return 0;
}
