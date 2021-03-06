/*
 * cmd_gpt.c -- GPT (GUID Partition Table) handling command
 *
 * Copyright (C) 2012 Samsung Electronics
 * author: Lukasz Majewski <l.majewski@samsung.com>
 * author: Piotr Wilczek <p.wilczek@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <malloc.h>
#include <command.h>
#include <mmc.h>
#include <part_efi.h>
#include <exports.h>
#include <linux/ctype.h>
#include <div64.h>

#ifndef CONFIG_PARTITION_UUIDS
#error CONFIG_PARTITION_UUIDS must be enabled for CONFIG_CMD_GPT to be enabled
#endif

/**
 * extract_env(): Expand env name from string format '&{env_name}'
 *                and return pointer to the env (if the env is set)
 *
 * @param str - pointer to string
 * @param env - pointer to pointer to extracted env
 *
 * @return - zero on successful expand and env is set
 */
static char extract_env(const char *str, char **env)
{
	char *e, *s;

	if (!str || strlen(str) < 4)
		return -1;

	if ((strncmp(str, "${", 2) == 0) && (str[strlen(str) - 1] == '}')) {
		s = strdup(str);
		if (s == NULL)
			return -1;
		memset(s + strlen(s) - 1, '\0', 1);
		memmove(s, s + 2, strlen(s) - 1);
		e = getenv(s);
		free(s);
		if (e == NULL) {
			printf("Environmental '%s' not set\n", str);
			return -1; /* env not set */
		}
		*env = e;
		return 0;
	}

	return -1;
}

/**
 * extract_val(): Extract value from a key=value pair list (comma separated).
 *                Only value for the given key is returend.
 *                Function allocates memory for the value, remember to free!
 *
 * @param str - pointer to string with key=values pairs
 * @param key - pointer to the key to search for
 *
 * @return - pointer to allocated string with the value
 */
static char *extract_val(const char *str, const char *key)
{
	char *v, *k;
	char *s, *strcopy;
	char *new = NULL;

	strcopy = strdup(str);
	if (strcopy == NULL)
		return NULL;

	s = strcopy;
	while (s) {
		v = strsep(&s, ",");
		if (!v)
			break;
		k = strsep(&v, "=");
		if (!k)
			break;
		if  (strcmp(k, key) == 0) {
			new = strdup(v);
			break;
		}
	}

	free(strcopy);

	return new;
}

/**
 * set_gpt_info(): Fill partition information from string
 *		function allocates memory, remember to free!
 *
 * @param dev_desc - pointer block device descriptor
 * @param str_part - pointer to string with partition information
 * @param str_disk_guid - pointer to pointer to allocated string with disk guid
 * @param partitions - pointer to pointer to allocated partitions array
 * @param parts_count - number of partitions
 *
 * @return - zero on success, otherwise error
 *
 */
static int set_gpt_info(block_dev_desc_t *dev_desc,
			const char *str_part,
			char **str_disk_guid,
			disk_partition_t **partitions,
			u8 *parts_count)
{
	char *tok, *str, *s;
	int i;
	char *val, *p;
	int p_count;
	disk_partition_t *parts;
	int errno = 0;
	uint64_t size_ll, start_ll;

	debug("%s: MMC lba num: 0x%x %d\n", __func__,
	      (unsigned int)dev_desc->lba, (unsigned int)dev_desc->lba);

	if (str_part == NULL)
		return -1;

	str = strdup(str_part);

	/* extract disk guid */
	s = str;
	tok = strsep(&s, ";");
	val = extract_val(tok, "uuid_disk");
	if (!val) {
		free(str);
		return -2;
	}
	if (extract_env(val, &p))
		p = val;
	*str_disk_guid = strdup(p);
	free(val);

	if (strlen(s) == 0)
		return -3;

	i = strlen(s) - 1;
	if (s[i] == ';')
		s[i] = '\0';

	/* calculate expected number of partitions */
	p_count = 1;
	p = s;
	while (*p) {
		if (*p++ == ';')
			p_count++;
	}

	/* allocate memory for partitions */
	parts = calloc(sizeof(disk_partition_t), p_count);

	/* retrive partions data from string */
	for (i = 0; i < p_count; i++) {
		tok = strsep(&s, ";");

		if (tok == NULL)
			break;

		/* uuid */
		val = extract_val(tok, "uuid");
		if (!val) { /* 'uuid' is mandatory */
			errno = -4;
			goto err;
		}
		if (extract_env(val, &p))
			p = val;
		if (strlen(p) >= sizeof(parts[i].uuid)) {
			printf("Wrong uuid format for partition %d\n", i);
			errno = -4;
			goto err;
		}
		strcpy((char *)parts[i].uuid, p);
		free(val);

		/* name */
		val = extract_val(tok, "name");
		if (!val) { /* name is mandatory */
			errno = -4;
			goto err;
		}
		if (extract_env(val, &p))
			p = val;
		if (strlen(p) >= sizeof(parts[i].name)) {
			errno = -4;
			goto err;
		}
		strcpy((char *)parts[i].name, p);
		free(val);

		/* size */
		val = extract_val(tok, "size");
		if (!val) { /* 'size' is mandatory */
			errno = -4;
			goto err;
		}
		if (extract_env(val, &p))
			p = val;
		size_ll = ustrtoull(p, &p, 0);
		parts[i].size = lldiv(size_ll, dev_desc->blksz);
		free(val);

		/* start address */
		val = extract_val(tok, "start");
		if (val) { /* start address is optional */
			if (extract_env(val, &p))
				p = val;
			start_ll = ustrtoull(p, &p, 0);
			parts[i].start = lldiv(start_ll, dev_desc->blksz);
			free(val);
		}
	}

	*parts_count = p_count;
	*partitions = parts;
	free(str);

	return 0;
err:
	free(str);
	free(*str_disk_guid);
	free(parts);

	return errno;
}

static int gpt_mmc_default(int dev, const char *str_part)
{
	int ret;
	char *str_disk_guid;
	u8 part_count = 0;
	disk_partition_t *partitions = NULL;

	struct mmc *mmc = find_mmc_device(dev);

	if (mmc == NULL) {
		printf("%s: mmc dev %d NOT available\n", __func__, dev);
		return CMD_RET_FAILURE;
	}

	if (!str_part)
		return -1;

	/* fill partitions */
	ret = set_gpt_info(&mmc->block_dev, str_part,
			&str_disk_guid, &partitions, &part_count);
	if (ret) {
		if (ret == -1)
			printf("No partition list provided\n");
		if (ret == -2)
			printf("Missing disk guid\n");
		if ((ret == -3) || (ret == -4))
			printf("Partition list incomplete\n");
		return -1;
	}

	/* save partitions layout to disk */
	gpt_restore(&mmc->block_dev, str_disk_guid, partitions, part_count);
	free(str_disk_guid);
	free(partitions);

	return 0;
}

/**
 * do_gpt(): Perform GPT operations
 *
 * @param cmdtp - command name
 * @param flag
 * @param argc
 * @param argv
 *
 * @return zero on success; otherwise error
 */
static int do_gpt(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = CMD_RET_SUCCESS;
	int dev = 0;
	char *pstr;

	if (argc < 5)
		return CMD_RET_USAGE;

	/* command: 'write' */
	if ((strcmp(argv[1], "write") == 0) && (argc == 5)) {
		/* device: 'mmc' */
		if (strcmp(argv[2], "mmc") == 0) {
			/* check if 'dev' is a number */
			for (pstr = argv[3]; *pstr != '\0'; pstr++)
				if (!isdigit(*pstr)) {
					printf("'%s' is not a number\n",
						argv[3]);
					return CMD_RET_USAGE;
				}
			dev = (int)simple_strtoul(argv[3], NULL, 10);
			/* write to mmc */
			if (gpt_mmc_default(dev, argv[4]))
				return CMD_RET_FAILURE;
		}
	} else {
		return CMD_RET_USAGE;
	}
	return ret;
}

U_BOOT_CMD(gpt, CONFIG_SYS_MAXARGS, 1, do_gpt,
	"GUID Partition Table",
	"<command> <interface> <dev> <partions_list>\n"
	" - GUID partition table restoration\n"
	" Restore GPT information on a device connected\n"
	" to interface\n"
);
