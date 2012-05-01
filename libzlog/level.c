/*
 * This file is part of the zlog Library.
 *
 * Copyright (C) 2011 by Hardy Simpson <HardySimpson1984@gmail.com>
 *
 * The zlog Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The zlog Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the zlog Library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "syslog.h"

#include "zc_defs.h"
#include "level.h"

void zlog_level_profile(zlog_level_t *a_level, int flag)
{
	zc_assert(a_level,);
	zc_profile(flag, "---level[%p][%d,%s,%s,%d,%d]---",
		a_level,
		a_level->int_level,
		a_level->str_uppercase,
		a_level->str_lowercase,
		(int) a_level->str_len,
		a_level->syslog_level);
	return;
}

/*******************************************************************************/
void zlog_level_del(zlog_level_t *a_level)
{
	zc_assert(a_level,);
	free(a_level);
	zc_debug("zlog_level_del[%p]", a_level);
	return;
}

static int syslog_level_atoi(char *str)
{
	/* guess no unix system will choose -187
	 * as its syslog level, so it is a safe return value
	 */
	zc_assert_debug(str, -187);

	if (STRICMP(str, ==, "LOG_EMERG"))
		return LOG_EMERG;
	if (STRICMP(str, ==, "LOG_ALERT"))
		return LOG_ALERT;
	if (STRICMP(str, ==, "LOG_CRIT"))
		return LOG_CRIT;
	if (STRICMP(str, ==, "LOG_ERR"))
		return LOG_ERR;
	if (STRICMP(str, ==, "LOG_WARNING"))
		return LOG_WARNING;
	if (STRICMP(str, ==, "LOG_NOTICE"))
		return LOG_NOTICE;
	if (STRICMP(str, ==, "LOG_INFO"))
		return LOG_INFO;
	if (STRICMP(str, ==, "LOG_DEBUG"))
		return LOG_DEBUG;

	zc_error("wrong syslog level[%s]", str);
	return -187;
}

/* line: TRACE = 10, LOG_ERR */
zlog_level_t *zlog_level_new(char *line)
{
	int rc;
	zlog_level_t *a_level;
	int i;
	int nscan;
	char str[MAXLEN_CFG_LINE + 1];
	int l;
	char sl[MAXLEN_CFG_LINE + 1];

	zc_assert_debug(line, -1);

	nscan = sscanf(line, " %[^= ] = %d ,%s",
		str, &l, sl);
	if (nscan < 2) {
		zc_error("level[%s], syntax wrong", line);
		return NULL;
	}

	/* check level and str */
	if ((l < 0) || (l > 255)) {
		zc_error("l[%d] not in [0,255], wrong", l);
		return NULL;
	}

	if (*str == '\0') {
		zc_error("str[0] = 0");
		return NULL;
	}

	a_level = calloc(1, sizeof(*a_level));
	if (!a_level) {
		zc_error("calloc fail, errno[%d]", errno);
		return NULL;
	}

	a_level->int_level = l;

	/* fill syslog level */
	if (*sl == '\0') {
		a_level->syslog_level = LOG_DEBUG;
	} else {
		a_level->syslog_level = syslog_level_atoi(sl);
		if (a_level->syslog_level == -187) {
			zc_error("syslog_level_atoi fail");
			rc = -1;
			goto zlog_level_new_exit;
		}
	}

	/* strncpy and toupper(str)  */
	for (i = 0; (i < sizeof(a_level->str_uppercase) - 1) && str[i] != '\0'; i++) {
		(a_level->str_uppercase)[i] = toupper(str[i]);
		(a_level->str_lowercase)[i] = tolower(str[i]);
	}

	if (str[i] != '\0') {
		/* overflow */
		zc_error("not enough space for str, str[%s] > %d", str, i);
		rc = -1;
		goto zlog_level_new_exit;
	} else {
		(a_level->str_uppercase)[i] = '\0';
		(a_level->str_lowercase)[i] = '\0';
	}

	a_level->str_len = i;

      zlog_level_new_exit:
	if (rc) {
		zc_error("line[%s]", line);
		zlog_level_del(a_level);
		return NULL;
	} else {
		zlog_level_profile(a_level, ZC_DEBUG);
		return a_level;
	}
}

#if 0
/*******************************************************************************/

static int zlog_level_set_inner(zc_arraylist_t *levels, char *line)
{
	int rc;
	zlog_level_t *a_level;

	zc_assert_debug(line, -1);
	zc_assert_debug(levels, -1);

	a_level = zlog_level_new(line);
	if (!a_level) {
		zc_error("zlog_level_new fail");
		return -1;
	}

	rc = zc_arraylist_set(levels, a_level->int_level, a_level);
	if (rc) {
		zc_error("zc_arraylist_set fail");
		rc = -1;
		goto zlog_level_set_exit;
	}

      zlog_level_set_exit:
	if (rc) {
		zc_error("line[%s]", line);
		zlog_level_del(a_level);
		return -1;
	} else {
		return 0;
	}
}

static int zlog_levels_set_default(zc_arraylist_t *levels)
{
	zc_assert_debug(levels, -1);

	return zlog_level_set_inner(levels, "* = 0, LOG_INFO")
	|| zlog_level_set_inner(levels, "DEBUG = 20, LOG_DEBUG")
	|| zlog_level_set_inner(levels, "INFO = 40, LOG_INFO")
	|| zlog_level_set_inner(levels, "NOTICE = 60, LOG_NOTICE")
	|| zlog_level_set_inner(levels, "WARN = 80, LOG_WARNING")
	|| zlog_level_set_inner(levels, "ERROR = 100, LOG_ERR")
	|| zlog_level_set_inner(levels, "FATAL = 120, LOG_ALERT")
	|| zlog_level_set_inner(levels, "UNKNOWN = 254, LOG_ERR")
	|| zlog_level_set_inner(levels, "! = 255, LOG_INFO");
}

/*******************************************************************************/

void zlog_levels_fini(void)
{
	if (zlog_env_levels) {
		zc_arraylist_del(zlog_env_levels);
	}
	zlog_env_levels = NULL;
	return;
}

int zlog_levels_init(void)
{
	int rc;
	zc_arraylist_t *levels;

	levels = zc_arraylist_new((zc_arraylist_del_fn)zlog_level_del);
	if (!levels) {
		zc_error("zc_arraylist_new fail");
		return -1;
	}

	rc = zlog_levels_set_default(levels);
	if (rc) {
		zc_error("zlog_level_set_default fail");
		rc = -1;
		goto zlog_level_init_exit;
	}

      zlog_level_init_exit:
	if (rc) {
		zc_arraylist_del(levels);
		return -1;
	} else {
		zlog_env_levels = levels;
		return 0;
	}
}

int zlog_levels_reset(void)
{
	int rc;
	zc_arraylist_t *levels;

	levels = zc_arraylist_new((zc_arraylist_del_fn)zlog_level_del);
	if (!levels) {
		zc_error("zc_arraylist_new fail");
		return -1;
	}

	rc = zlog_levels_set_default(levels);
	if (rc) {
		zc_error("zlog_level_set_default fail");
		rc = -1;
		goto zlog_level_reset_exit;
	}

      zlog_level_reset_exit:
	if (rc) {
		zc_arraylist_del(levels);
		return -1;
	} else {
		zlog_levels_fini();
		zlog_env_levels = levels;
		return 0;
	}
}

/*******************************************************************************/
int zlog_level_set(char *line)
{
	int rc;
	rc = zlog_level_set_inner(zlog_env_levels, line);
	if (rc) {
		zc_error("zlog_leve_set_inner fail");
		return -1;
	}
	return 0;
}

int zlog_level_atoi(char *str)
{
	int i;
	zlog_level_t *a_level;

	if (str == NULL || *str == '\0') {
		zc_error("str is [%s], can't find level", str);
		return -1;
	}

	zc_arraylist_foreach(zlog_env_levels, i, a_level) {
		if (a_level && STRICMP(str, ==, a_level->str_uppercase)) {
			return i;
		}
	}

	zc_error("str[%s] can't found in level map", str);
	return -1;
}

zlog_level_t *zlog_level_get(int p)
{
	zlog_level_t *a_level;

	if ((p <= 0) || (p > 254)) {
		/* illegal input from zlog() */
		zc_error("p[%d] not in (0,254), set to UNKOWN", p);
		p = 254;
	}

	a_level = zc_arraylist_get(zlog_env_levels, p);
	if (!a_level) {
		/* empty slot */
		zc_error("p[%d] in (0,254), but not in map,"
			"see configure file define, set ot UNKOWN", p);
		a_level = zc_arraylist_get(zlog_env_levels, 254);
	}

	return a_level;
}



void zlog_levels_profile(void)
{
	int i;
	zlog_level_t *a_level;

	zc_arraylist_foreach(zlog_env_levels, i, a_level) {
		if (a_level) {
			zc_error("level:%s = %d, %d",
				a_level->str_uppercase, 
				i, a_level->syslog_level);
		}
	}

	return;
}

#endif
