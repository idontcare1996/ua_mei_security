/*
 * pkcs15-cache.c: PKCS #15 file caching functions
 *
 * Copyright (C) 2001, 2002  Juha Yrj��<juha.yrjola@iki.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "internal.h"
#include "pkcs15.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

#define TYPE_FIXED_PER_CARD     0x02    /* card-dependend but fixed */
#define TYPE_SOD                0x04    /* SOD-type */

typedef struct {
	const u8 *path;
	int path_len;
	int type;
} pteid_file_info;

const static pteid_file_info pteid_file_infos[] = {
	{(u8 *) "\x50\x31", 2, TYPE_FIXED_PER_CARD},  /* EF(ODF) file */
	{(u8 *) "\x4f\x00\x50\x31", 4, TYPE_FIXED_PER_CARD},  /* EF(ODF) file */
	{(u8 *) "\x5F\x00\xEF\x02", 4, TYPE_FIXED_PER_CARD},  /* ID file */
	{(u8 *) "\x5F\x00\x44\x01", 4, TYPE_FIXED_PER_CARD},  /* AODF */
	{(u8 *) "\x5F\x00\xEF\x0C", 4, TYPE_FIXED_PER_CARD},  /* CDF */
	{(u8 *) "\x5F\x00\xEF\x0D", 4, TYPE_FIXED_PER_CARD},  /* PrKDF */
	{(u8 *) "\x5F\x00\xEF\x0E", 4, TYPE_FIXED_PER_CARD},  /* PuKDF */
	{(u8 *) "\x5F\x00\xEF\x08", 4, TYPE_FIXED_PER_CARD},  /* Signature cert */
	{(u8 *) "\x5F\x00\xEF\x09", 4, TYPE_FIXED_PER_CARD},  /* Authentication cert */
	{(u8 *) "\x5F\x00\xEF\x0F", 4, TYPE_FIXED_PER_CARD},  /* Signature CA cert */
	{(u8 *) "\x5F\x00\xEF\x10", 4, TYPE_FIXED_PER_CARD},  /* Authentication CA cert */
	{(u8 *) "\x5F\x00\xEF\x11", 4, TYPE_FIXED_PER_CARD},  /* Root CA cert */
	{(u8 *) "\x5F\x00\xEF\x06", 4, TYPE_SOD},             /* SOD file */
	{NULL, 0, 0}
};

#ifdef _WIN32
#pragma pack(1)
#define __PACKED
#else
#define __PACKED __attribute__((__packed__))
#endif

#define HEADER_VERSION  0x10
typedef struct {
	u8 version;   /* currently 0x10 */
	u8 crc[4];    /* checksum over the contents */
	u8 rfu[15];    /* set to 0 for this version */
} __PACKED cache_header;

static char cachedir[PATH_MAX];
static int cachedir_known = 0;

static int generate_cache_filename(struct sc_pkcs15_card *p15card,
	const struct sc_path *path, char *buf, size_t bufsize);
static void make_header(cache_header *header, const u8 *contents, size_t contentslen);
static int check_header(struct sc_pkcs15_card *p15card, const char *fname,
	cache_header *header, const u8 *contents, size_t contentslen);
static int get_asn1_len2(const u8 *data, int len);
static int get_last_16_bytes(struct sc_pkcs15_card *p15card,
	const struct sc_path *path, int end, u8 *data);

const static unsigned long crc_table[256];

int sc_pkcs15_read_cached_file(struct sc_pkcs15_card *p15card,
			       const struct sc_path *path, int *action,
			       u8 **buf, size_t *bufsize)
{
	const pteid_file_info *fi;
	cache_header header;
	char fname[PATH_MAX];
	size_t count, got;
	struct stat stbuf;
	u8 *data = NULL;
	int offset;
	int r;
	FILE *f;

	*action = 0;

	/* See if we do 'something cacheable' with the file */
	for (fi = &pteid_file_infos[0]; fi->path != NULL; fi++) {
		if ((path->len == fi->path_len + 2) && !memcmp(path->value + 2, fi->path, fi->path_len))
			break;
	}
	if (fi->path == NULL)
		return SC_ERROR_FILE_NOT_FOUND;

	r = generate_cache_filename(p15card, path, fname, sizeof(fname));
	if (r != 0)
		return r;
	*action = ACTION_CACHE;

	r = stat(fname, &stbuf);
	if (r)
		return SC_ERROR_FILE_NOT_FOUND;

	if (path->count < 0)
		count = stbuf.st_size - sizeof(header);
	else
		return SC_ERROR_FILE_NOT_FOUND; /* only reading from offset 0 is supported */

	if (*buf == NULL) {
		data = (u8 *) malloc(count);
		if (data == NULL)
			return SC_ERROR_OUT_OF_MEMORY;
	} else
		if (count > *bufsize)
			return SC_ERROR_BUFFER_TOO_SMALL;

	f = fopen(fname, "rb");
	if (f == NULL) {
		r = SC_ERROR_FILE_NOT_FOUND;
		goto error;
	}
	got = fread(&header, 1, sizeof(cache_header), f);
	if (got != sizeof(cache_header))
		r=  SC_ERROR_BUFFER_TOO_SMALL;
	else {
		got = fread(data, 1, count, f);
		if (got != count)
			r = SC_ERROR_BUFFER_TOO_SMALL;
	}
	fclose(f);
	if (r != 0)
		goto error;

	r = check_header(p15card, fname, &header, data, count);
	if (r != 0)
		goto error;

	/* The SOD can change, so we read the last 16 bytes of the SOD (= the
	 * last 10 bytes of the SOD's signature) and compare with the cached data. */
	if (fi->type == TYPE_SOD) {
		unsigned char last_16_bytes[16];
		int asn1_len = get_asn1_len2(data, count);
		if (asn1_len < 0)
			goto error;
		r = get_last_16_bytes(p15card, path, asn1_len, last_16_bytes);
		if (r < 0)
			goto error;
		if (memcmp(last_16_bytes, data + asn1_len - 16, 16) == 0)
			r = SC_NO_ERROR;
		else
		{
			r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
			goto error;
		}
	}

	*action = 0;
	*bufsize = count;
	if (data)
		*buf = data;
	return 0;

error:
	if (data)
		free(data);
	return r;
}

int sc_pkcs15_cache_file(struct sc_pkcs15_card *p15card,
			 const struct sc_path *path,
			 const u8 *buf, size_t bufsize)
{
	const pteid_file_info *fi;
	cache_header header;
	char fname[PATH_MAX];
	int fileindex = -1;
	int r;
	FILE *f;
	size_t c;

	/* See if we do 'something cacheable' with the file */
	for (fi = &pteid_file_infos[0]; fi->path != NULL; fi++) {
		if ((path->len == fi->path_len + 2) && !memcmp(path->value + 2, fi->path, fi->path_len))
			break;
	}
	if (fi->path == NULL)
		return SC_ERROR_FILE_NOT_FOUND;

	r = generate_cache_filename(p15card, path, fname, sizeof(fname));
	if (r != 0)
		return r;

	memset(&header, 0, sizeof(header));

	f = fopen(fname, "wb");
	/* If the open failed because the cache directory does
	 * not exist, create it and a re-try the fopen() call.
	 */
	if (f == NULL && errno == ENOENT) {
		if ((r = sc_make_cache_dir()) < 0)
			goto error;
		f = fopen(fname, "wb");
	}
	if (f == NULL) {
		r = SC_ERROR_INTERNAL;
		goto error;
	}

	make_header(&header, buf, bufsize);

	c = fwrite(&header, 1, sizeof(header), f);
	if (c == sizeof(header))
		c += fwrite(buf, 1, bufsize, f);
	fclose(f);

	if (c != (bufsize + sizeof(header))) {
		sc_error(p15card->card->ctx, "fwrite() failed: wrote only %d bytes", c);
		unlink(fname);
		r = SC_ERROR_INTERNAL;
		goto error;
	}
	r = 0;

error:
	return r;
}

int sc_pkcs15_cache_clear(struct sc_pkcs15_card *p15card)
{
	const pteid_file_info *fi;
	char fname[PATH_MAX];
	sc_path_t path;
	int i, r, ret = 0, endloop;

	path.type = SC_PATH_TYPE_PATH;

	if (p15card == NULL)
		return SC_ERROR_INVALID_ARGUMENTS;
	else {
		for (fi = &pteid_file_infos[0]; fi->path != NULL; fi++) {
			memcpy(path.value, fi->path, fi->path_len);
			path.len = fi->path_len;
			r = generate_cache_filename(p15card, &path, fname, sizeof(fname));
			if (r != 0) {
				ret = r;
				continue;
			}

			for (i = 0; i < 2; i++) {
				r = unlink(fname);
				if ((r == EBUSY) && (i == 0)) { /* busy -> wait 0,5 sec and try again */
#ifdef _WIN32
					Sleep(500);
#else
					usleep(500000);
#endif
					continue;
				}
				if ((r != 0) && (errno != ENOENT)) {
					sc_error(p15card->card->ctx, "unlink-ing \"%s\" failed: %d\n", fname, r);
					ret = r;
				}
				break;
			}
		}
	}

	return ret;
}

static int generate_cache_filename(struct sc_pkcs15_card *p15card,
	const struct sc_path *path, char *buf, size_t bufsize)
{
	char pathname[SC_MAX_PATH_SIZE*2+1];
	int  r;
	const u8 *pathptr;
	size_t i, pathlen;

	if (path->type != SC_PATH_TYPE_PATH)
		return SC_ERROR_INVALID_ARGUMENTS;
	assert(path->len <= SC_MAX_PATH_SIZE);

	if (!cachedir_known) {
		r = sc_get_cache_dir(cachedir, sizeof(cachedir));
		if (r)
			return r;
		cachedir_known = 1;
	}

	pathptr = path->value;
	pathlen = path->len;
	if (pathlen > 2 && memcmp(pathptr, "\x3F\x00", 2) == 0) {
		pathptr += 2;
		pathlen -= 2;
	}
	for (i = 0; i < pathlen; i++)
		sprintf(pathname + 2*i, "%02X", pathptr[i]);

	r = snprintf(buf, bufsize, "%s/%s_%s.bin", cachedir, p15card->serial_number, pathname);
	if (r < 0)
		return SC_ERROR_BUFFER_TOO_SMALL;

	return 0;
}

static void make_header(cache_header *header, const u8 *contents, size_t contentslen)
{
	unsigned long crc = 0xFFFFFFFF;
	int i;

	header->version = HEADER_VERSION;

	for (i = 0; i < contentslen; i++)
		crc = crc_table[(crc ^ contents[i]) & 0xff] ^ (crc >> 8);
	crc ^= 0xFFFFFFFF;
	for (i = 3; i >= 0; i--) {
		header->crc[i] = 0xFF & crc;
		crc >>= 8;
	}
}

/* Check header version + CRC, return 0 if OK */
static int check_header(struct sc_pkcs15_card *p15card, const char *fname,
	cache_header *header, const u8 *contents, size_t contentslen)
{
	unsigned long crc = 0xFFFFFFFF;
	int i;

	/* Incompatible versions have a different most significant nibble */
	if ((header->version & 0xF0) != (HEADER_VERSION & 0xF0)) {
		sc_error(p15card->card->ctx, "Unsupported cache file version (%d) for file \"%s\"\n",
			header->version, fname);
		return SC_ERROR_NOT_SUPPORTED;
	}

	for (i = 0; i < contentslen; i++)
		crc = crc_table[(crc ^ contents[i]) & 0xff] ^ (crc >> 8);
	crc ^= 0xFFFFFFFF;

	for (i = 3; i >= 0; i--) {
		if (header->crc[i] != (0xFF & crc)) {
			sc_error(p15card->card->ctx, "Wrong checksum for file \"%s\"\n", fname);
			return SC_ERROR_OBJECT_NOT_VALID;
		}
		crc >>= 8;
	}

	return 0;
}

static int get_asn1_len2(const u8 *data, int len)
{
	if (len < 20)
		return SC_ERROR_WRONG_LENGTH;

	/* Only used for the SOD, of which we know it's > 256 bytes */
	if (data[1] != 0x82 || data[5] != 0x82)
		return SC_ERROR_WRONG_LENGTH;
	
	return 8 + (256 * data[6]) + data[7];
}

static int get_last_16_bytes(struct sc_pkcs15_card *p15card,
	const struct sc_path *path, int end, u8 *data)
{
	int r;

	r = sc_lock(p15card->card);
	SC_TEST_RET(p15card->card->ctx, r, "sc_lock() failed");

	r = sc_select_file(p15card->card, path, NULL);
	if (r) {
		sc_unlock(p15card->card);
		return r;
	}

	r = sc_read_binary(p15card->card, end - 16, data, 16, 0);

	sc_unlock(p15card->card);

	return r;
}

/* CRC-32 checksum table, used in PNG, see http://www.w3.org/TR/PNG-CRCAppendix.html */
const static unsigned long crc_table[] = {
	0x00000000,0x77073096,0xee0e612c,0x990951ba,0x076dc419,0x706af48f,0xe963a535,0x9e6495a3,
	0x0edb8832,0x79dcb8a4,0xe0d5e91e,0x97d2d988,0x09b64c2b,0x7eb17cbd,0xe7b82d07,0x90bf1d91,
	0x1db71064,0x6ab020f2,0xf3b97148,0x84be41de,0x1adad47d,0x6ddde4eb,0xf4d4b551,0x83d385c7,
	0x136c9856,0x646ba8c0,0xfd62f97a,0x8a65c9ec,0x14015c4f,0x63066cd9,0xfa0f3d63,0x8d080df5,
	0x3b6e20c8,0x4c69105e,0xd56041e4,0xa2677172,0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,
	0x35b5a8fa,0x42b2986c,0xdbbbc9d6,0xacbcf940,0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,
	0x26d930ac,0x51de003a,0xc8d75180,0xbfd06116,0x21b4f4b5,0x56b3c423,0xcfba9599,0xb8bda50f,
	0x2802b89e,0x5f058808,0xc60cd9b2,0xb10be924,0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,
	0x76dc4190,0x01db7106,0x98d220bc,0xefd5102a,0x71b18589,0x06b6b51f,0x9fbfe4a5,0xe8b8d433,
	0x7807c9a2,0x0f00f934,0x9609a88e,0xe10e9818,0x7f6a0dbb,0x086d3d2d,0x91646c97,0xe6635c01,
	0x6b6b51f4,0x1c6c6162,0x856530d8,0xf262004e,0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,
	0x65b0d9c6,0x12b7e950,0x8bbeb8ea,0xfcb9887c,0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,
	0x4db26158,0x3ab551ce,0xa3bc0074,0xd4bb30e2,0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,
	0x4369e96a,0x346ed9fc,0xad678846,0xda60b8d0,0x44042d73,0x33031de5,0xaa0a4c5f,0xdd0d7cc9,
	0x5005713c,0x270241aa,0xbe0b1010,0xc90c2086,0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,
	0x5edef90e,0x29d9c998,0xb0d09822,0xc7d7a8b4,0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,
	0xedb88320,0x9abfb3b6,0x03b6e20c,0x74b1d29a,0xead54739,0x9dd277af,0x04db2615,0x73dc1683,
	0xe3630b12,0x94643b84,0x0d6d6a3e,0x7a6a5aa8,0xe40ecf0b,0x9309ff9d,0x0a00ae27,0x7d079eb1,
	0xf00f9344,0x8708a3d2,0x1e01f268,0x6906c2fe,0xf762575d,0x806567cb,0x196c3671,0x6e6b06e7,
	0xfed41b76,0x89d32be0,0x10da7a5a,0x67dd4acc,0xf9b9df6f,0x8ebeeff9,0x17b7be43,0x60b08ed5,
	0xd6d6a3e8,0xa1d1937e,0x38d8c2c4,0x4fdff252,0xd1bb67f1,0xa6bc5767,0x3fb506dd,0x48b2364b,
	0xd80d2bda,0xaf0a1b4c,0x36034af6,0x41047a60,0xdf60efc3,0xa867df55,0x316e8eef,0x4669be79,
	0xcb61b38c,0xbc66831a,0x256fd2a0,0x5268e236,0xcc0c7795,0xbb0b4703,0x220216b9,0x5505262f,
	0xc5ba3bbe,0xb2bd0b28,0x2bb45a92,0x5cb36a04,0xc2d7ffa7,0xb5d0cf31,0x2cd99e8b,0x5bdeae1d,
	0x9b64c2b0,0xec63f226,0x756aa39c,0x026d930a,0x9c0906a9,0xeb0e363f,0x72076785,0x05005713,
	0x95bf4a82,0xe2b87a14,0x7bb12bae,0x0cb61b38,0x92d28e9b,0xe5d5be0d,0x7cdcefb7,0x0bdbdf21,
	0x86d3d2d4,0xf1d4e242,0x68ddb3f8,0x1fda836e,0x81be16cd,0xf6b9265b,0x6fb077e1,0x18b74777,
	0x88085ae6,0xff0f6a70,0x66063bca,0x11010b5c,0x8f659eff,0xf862ae69,0x616bffd3,0x166ccf45,
	0xa00ae278,0xd70dd2ee,0x4e048354,0x3903b3c2,0xa7672661,0xd06016f7,0x4969474d,0x3e6e77db,
	0xaed16a4a,0xd9d65adc,0x40df0b66,0x37d83bf0,0xa9bcae53,0xdebb9ec5,0x47b2cf7f,0x30b5ffe9,
	0xbdbdf21c,0xcabac28a,0x53b39330,0x24b4a3a6,0xbad03605,0xcdd70693,0x54de5729,0x23d967bf,
	0xb3667a2e,0xc4614ab8,0x5d681b02,0x2a6f2b94,0xb40bbe37,0xc30c8ea1,0x5a05df1b,0x2d02ef8d,
};
