/*
 * card-pteid.c: Support for Portugese EID card
 *
 * Based on the GemSafe V2 applet
 *
 * Copyright (C) 2003, Zetes Belgium
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

/*
 * Card driver for Axalto's IAS applet, according to
 * the "ICITIZEN OPEN V2 IAS APPLET USER MANUAL".
 */

#include "internal.h"
#include "log.h"
#include "cardctl.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* To be removed */
#include <time.h>
static long t1, t2, tot_read = 0, tot_dur = 0, dur;

#define PTEID_VERSION			"1.0"

#define PTEID_MIN_USER_PIN_LEN     4
#define PTEID_MAX_USER_PIN_LEN     8
#define PTEID_MAX_PIN_TRIES        3
#define PTEID_KEY_LENGTH_BYTES     128

struct pteid_priv_data {
	int options;
};

#define DRVDATA(card)        ((struct pteid_priv_data *) ((card)->drv_data))

static struct sc_card_operations pteid_ops;
static struct sc_card_driver pteid_drv = {
	"Portugal eID (GemSafe)",
	"pteid_gemsafe",
	&pteid_ops
};
static const struct sc_card_operations *iso_ops = NULL;

static unsigned char aid_gemsafe1[] = {0x60,0x46,0x32,0xFF,0x00,0x00,0x02};
static unsigned char aid_gemsafe2[] = {0xA0,0x00,0x00,0x00,0x18,0x30,0x08,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

static int pteid_finish(struct sc_card *card)
{
	free(DRVDATA(card));
	return 0;
}

static int pteid_match_card(struct sc_card *card)
{
	int r;
    struct sc_apdu apdu;

	sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xA4, 0x04, 0x00);	
    apdu.lc = sizeof(aid_gemsafe1);
    apdu.data = aid_gemsafe1;
    apdu.datalen = sizeof(aid_gemsafe1);
    apdu.resplen = 0;
    apdu.le = 0;
    r = sc_transmit_apdu(card, &apdu);
	card->ctx->log_errors = 0;
    r = sc_check_sw(card, apdu.sw1, apdu.sw2);
	card->ctx->log_errors = 1;

	if (r != SC_NO_ERROR) {
	    sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xA4, 0x04, 0x00);	
	    apdu.lc = sizeof(aid_gemsafe2);
	    apdu.data = aid_gemsafe2;
	    apdu.datalen = sizeof(aid_gemsafe2);
	    apdu.resplen = 0;
	    apdu.le = 0;

	    r = sc_transmit_apdu(card, &apdu);
		card->ctx->log_errors = 0;
	    r = sc_check_sw(card, apdu.sw1, apdu.sw2);
		card->ctx->log_errors = 1;
	}

	return r == SC_NO_ERROR;
}

static int pteid_init(struct sc_card *card)
{
	struct pteid_priv_data *priv = NULL;
	scconf_block *conf_block;

	sc_debug(card->ctx, "pteid (gemsafe) V%s\n", PTEID_VERSION);

	priv = (struct pteid_priv_data *) calloc(1, sizeof(struct pteid_priv_data));
	if (priv == NULL)
		return SC_ERROR_OUT_OF_MEMORY;
	card->drv_data = priv;
	card->cla = 0x00;
	_sc_card_add_rsa_alg(card, 8 * PTEID_KEY_LENGTH_BYTES,
		SC_ALGORITHM_RSA_PAD_PKCS1 | SC_ALGORITHM_RSA_HASH_NONE, 0);

	/* State that we have an RNG */
	card->caps |= SC_CARD_CAP_RNG;

	card->max_pin_len = PTEID_MAX_USER_PIN_LEN; /* TODO */

	return 0;
}

static void add_acl_entry(struct sc_file *file, int op, u8 byte)
{
	unsigned int method, key_ref = SC_AC_KEY_REF_NONE;

	if (byte == 0)
		method = SC_AC_NONE;
	else if (byte == 0xFF)
		method = SC_AC_NEVER;
	else if (byte & 0x10) {
		/* (byte & 0x0F) contains the SE, instead of reading
		 * it we just hardcode the correct PIN */
		method = SC_AC_CHV;
		if (byte & 0x0F == 0x02)
			key_ref = 0x83;  /* address PIN */
		else if (byte & 0x04)
			key_ref = 0x87;  /* activation PIN */
		else
			key_ref = 0x80 + (byte & 0x0F);
	}
	else if (byte == 0x12) {
		method = SC_AC_CHV;
		key_ref = 0x02; /* TODO */
	}
	else
		method = SC_AC_UNKNOWN;

	sc_file_add_acl_entry(file, op, method, key_ref);
}

/* We only parse the first AMB + SCB combination, 5.5*/
static int parse_security_attributes(struct sc_context *ctx,
	const u8 *buf, int buflen, int file_type, struct sc_file *file)
{
	int len = buf[1];
	u8 amb = buf[2];
	int scb_idx = 0;
	int scb_count = ((amb&0x80)==0x80) + ((amb&0x40)==0x40) + ((amb&0x20)==0x20) + ((amb&0x10)==0x10) +
		((amb&0x08)==0x08) + ((amb&0x04)==0x04) + ((amb&0x02)==0x02) + ((amb&0x01)==0x01);

	if ((file_type != SC_FILE_TYPE_WORKING_EF) && (file_type != SC_FILE_TYPE_DF))
		return -30;

	if (len  + 2 != buflen)
		return -31;
	if (1 + scb_count > len)
		return -32;

	if (amb & 0x80)
		scb_idx++;
	if (amb & 0x40)
		scb_idx++;
	if (amb & 0x20)
		scb_idx++;
	if (amb & 0x10)
		scb_idx++;
	if (amb & 0x08)
		scb_idx++;
	if (amb & 0x04) {
		scb_idx++;
		if (file_type == SC_FILE_TYPE_DF)
			add_acl_entry(file, SC_AC_OP_CREATE, buf[2 + scb_idx]);
	}
	if (amb & 0x02) {
		scb_idx++;
		if (file_type == SC_FILE_TYPE_DF)
			add_acl_entry(file, SC_AC_OP_CREATE, buf[2 + scb_idx]);
		else {
			add_acl_entry(file, SC_AC_OP_UPDATE, buf[2 + scb_idx]);
			add_acl_entry(file, SC_AC_OP_ERASE, buf[2 + scb_idx]);
		}
	}
	if (amb & 0x01) {
		scb_idx++;
		if (file_type == SC_FILE_TYPE_WORKING_EF)
			add_acl_entry(file, SC_AC_OP_READ, buf[2 + scb_idx]);
	}

	return 0;
}

/* E.g. Address: file: 6F 15 81 02 27 10 82 01 01 83 02 EF 05 8A 01 05 8C 05 1B FF FF A1 32 */
static int pteid_parse_fci(struct sc_context *ctx, const u8 *buf, int buflen,
			       struct sc_file *file)
{
	const u8 *sec_attr;
	int sec_attr_len;
	int r;

	if (buflen < 2)
		return -1;
	if (buf[0] != 0x6F)
		return -2;
	if (2 + buf[1] != buflen)
		return -3;

	/* Mandatory fields */
	file->type = SC_FILE_TYPE_DF; /*default */
	sec_attr = NULL;
	file->status = SC_FILE_STATUS_ACTIVATED; /* default */
	file->size = 5000;

	buf += 2;
	buflen -= 2;
	while (buflen >= 2) {
		switch(buf[0]) {
		case 0x81:                 /* file size */
			if (buf[1] != 0x02)
				return -4;
			file->size = 256L * buf[2] + buf[3];
			break;
		case 0x82:                 /* file descriptor (type) */
			if (buf[1] != 0x01)
				return -5;
			if (buf[2] == 0x01) {
				file->type = SC_FILE_TYPE_WORKING_EF;
				file->ef_structure = SC_FILE_EF_TRANSPARENT;
			}
			else if (buf[2] == 0x38)
				file->type = SC_FILE_TYPE_DF;
			else
				return -6;
			break;
		case 0x83:                /* file ID */
			if (buf[1] != 0x02)
				return -7;
			file->id = 256 * buf[2] + buf[3];
			break;
		case 0x8A:                  /* life cycle status */
			if (buf[1] != 0x01)
				return -9;
			if ((buf[2] & 0xFD) == 0x05)
				file->status = SC_FILE_STATUS_ACTIVATED;
			else if ((buf[2] & 0xFD) == 0x04) /* deactivated */
				file->status = SC_FILE_STATUS_INVALIDATED;
			else if ((buf[2] & 0xFC) == 0x0C) /* termiated */
				file->status = SC_FILE_STATUS_INVALIDATED;
			else
				return -10;
			break;
		case 0x8C:                /* security attributes, compact encoding */
			sec_attr = buf;
			sec_attr_len = buf[1] + 2;
			break;
		case 0x84:                /* DF name -> ignore */
			break;
		default:
			sc_error(ctx, "unknown tag %02x in FCI\n", buf[0]);
		}

		buflen -= buf[1] + 2;
		buf += buf[1] + 2;
	}
	if (buflen != 0)
		sc_error(ctx, "FCI buffer: %d bytes unparsed\n", buflen);

	if ((file->type == -1) || (sec_attr == NULL) || (file->status == -1))
		return -21;

	r = parse_security_attributes(ctx, sec_attr, sec_attr_len, file->type, file);

	file->magic = SC_FILE_MAGIC;

	return 0;
}

static int pteid_select_file(struct sc_card *card,
			       const struct sc_path *in_path,
			       struct sc_file **file_out)
{
	int select_root = 0;
	struct sc_apdu apdu;
	u8 pathbuf[SC_MAX_PATH_SIZE];
	u8 rbuf[SC_MAX_APDU_BUFFER_SIZE];
	int r;

try_again:
	if (select_root) {
		sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xA4, 0x00, 0x0C);
		apdu.lc = apdu.datalen = 2;
		pathbuf[0] = 0x3F; pathbuf[1] = 0x00;
		apdu.data = pathbuf;
		apdu.le = apdu.resplen = 0;

		r = sc_transmit_apdu(card, &apdu);
		SC_TEST_RET(card->ctx, r, "Selecting MF failed");
		r = sc_check_sw(card, apdu.sw1, apdu.sw2);
		SC_TEST_RET(card->ctx, r, "Card returned error");
	}

	if (file_out == NULL) {
		sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xA4, 0x08, 0x0C);
		apdu.le = apdu.resplen = 0;		
	}
	else {
		sc_format_apdu(card, &apdu, SC_APDU_CASE_4_SHORT, 0xA4, 0x08, 0x00);
		apdu.resp = rbuf;
		apdu.resplen = sizeof(rbuf);
		apdu.le = 256;
	}

	if (select_root && (in_path->len >= 4)) {
		memcpy(pathbuf, in_path->value + 2, in_path->len - 2);
		apdu.data = pathbuf;
		apdu.lc = apdu.datalen = in_path->len - 2;
	}
	else {
		memcpy(pathbuf, in_path->value + in_path->len - 2, 2);
		apdu.data = pathbuf;
		apdu.lc = apdu.datalen = 2;
		if ((in_path->len == 2) && (pathbuf[0] == 0x3F) && (pathbuf[1] == 0x00))
			apdu.p1 = 0x00;
	}

	if (apdu.lc == 2 && !(pathbuf[1] == 0x00 && (pathbuf[0] == 0x3F || pathbuf[0] == 0x4F || pathbuf[0] == 0x5F)))
		apdu.p1 = 0x00;

	r = sc_transmit_apdu(card, &apdu);
	SC_TEST_RET(card->ctx, r, "Selecting MF failed");
	r = sc_check_sw(card, apdu.sw1, apdu.sw2);
	if ((r == SC_ERROR_FILE_NOT_FOUND) && !select_root) {
		select_root = 1;
		goto try_again;
	}
	SC_TEST_RET(card->ctx, r, "Card returned error");

	if (file_out != NULL) {
		*file_out = sc_file_new();
		if (*file_out == NULL)
			SC_FUNC_RETURN(card->ctx, 0, SC_ERROR_OUT_OF_MEMORY);
		r = pteid_parse_fci(card->ctx, apdu.resp, apdu.resplen, *file_out);
		if (r < 0)
			sc_file_free(*file_out);
		SC_TEST_RET(card->ctx, r, "P�sing of FCI failed");
		(*file_out)->path = *in_path;
	}

	return 0;
}

static int pteid_set_security_env(struct sc_card *card,
				    const struct sc_security_env *env,
				    int se_num)
{
	struct pteid_priv_data *priv = (struct pteid_priv_data *) DRVDATA(card);
	struct sc_apdu apdu;
	u8 sbuf[SC_MAX_APDU_BUFFER_SIZE];
	int r;

	assert(card != NULL && env != NULL);

	sc_debug(card->ctx, "pteid_set_security_env(), keyRef = 0x%0x, algo = 0x%0x\n",
		*env->key_ref, env->algorithm_flags);

	if (env->algorithm_flags != SC_ALGORITHM_RSA_PAD_PKCS1) {
		SC_TEST_RET(card->ctx, SC_ERROR_INVALID_ARGUMENTS, "Only SC_ALGORITHM_RSA_PAD_PKCS1 accepted in MSE SET command");
	}

	sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0x22, 0x41, 0xB6); /* B6: Digital Signature template */
	memcpy(sbuf, "\x80\x01\x02\x84\x01", 5);
	sbuf[5] = *env->key_ref;
	apdu.data = sbuf;
	apdu.datalen = apdu.lc = 6;
	apdu.le = apdu.resplen = 0;

	r = sc_transmit_apdu(card, &apdu);
	SC_TEST_RET(card->ctx, r, "Set Security Env APDU transmit failed");

	r = sc_check_sw(card, apdu.sw1, apdu.sw2);
	SC_TEST_RET(card->ctx, r, "Card's Set Security Env command returned error");

	return r;
}

static int pteid_compute_signature(struct sc_card *card, const u8 *data,
				  size_t data_len, u8 * out, size_t outlen)
{
	struct sc_apdu apdu;
	u8 tbuf[SC_MAX_APDU_BUFFER_SIZE];
	u8 rbuf[SC_MAX_APDU_BUFFER_SIZE];
	int r;
	size_t i;
	
	if (data_len > 36) {
		sc_error(card->ctx, "Illegal input length: %d\n", data_len);
		return SC_ERROR_INVALID_ARGUMENTS;
	}
	if (outlen < data_len) {
		sc_error(card->ctx, "Output buffer too small.\n");
		return SC_ERROR_BUFFER_TOO_SMALL;
	}

	/* PSO: Hash */
	sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0x2A, 0x90, 0xA0);
	apdu.lc = apdu.datalen = data_len + 2;
	tbuf[0] = 0x90;
	tbuf[1] = (u8) data_len;
	memcpy(tbuf + 2, data, data_len);
	apdu.data = tbuf;
	apdu.resplen = apdu.le = 0;

	r = sc_transmit_apdu(card, &apdu);
	SC_TEST_RET(card->ctx, r, "APDU transmit failed");
	if (apdu.sw1 != 0x61) {
		r = sc_check_sw(card, apdu.sw1, apdu.sw2);
		SC_TEST_RET(card->ctx, r, "Card returned error");
	}

	/* PSO: Compute Digital Signature */
	sc_format_apdu(card, &apdu, SC_APDU_CASE_2_SHORT, 0x2A, 0x9E, 0x9A);
	apdu.le = PTEID_KEY_LENGTH_BYTES;
	apdu.resplen = sizeof(rbuf);
	apdu.resp = rbuf;

	r = sc_transmit_apdu(card, &apdu);
	SC_TEST_RET(card->ctx, r, "APDU transmit failed");
	r = sc_check_sw(card, apdu.sw1, apdu.sw2);
	if (r < 0) {
		if (r != SC_ERROR_SECURITY_STATUS_NOT_SATISFIED)
			sc_error(card->ctx, "pteid_compute_signature() returned %s\n", sc_strerror(r));
		return r;
	}

	memcpy(out, rbuf, apdu.resplen);

	return apdu.resplen;
}

static int pteid_logout(struct sc_card *card)
{
	return 0;
}

static int pteid_card_ctl(struct sc_card *card, unsigned long cmd, void *ptr)
{
	switch (cmd) {
	case SC_CARDCTL_PTEID_GET_PIN_INFO:
	{
		sc_apdu_t apdu;
		u8 rbuf[2];
		int buflen, r;
		struct sc_cardctl_pteid_pin_info *pin_info = ptr;

		pin_info->maxtries = PTEID_MAX_PIN_TRIES;
		pin_info->maxlen = PTEID_MAX_USER_PIN_LEN;
		pin_info->minlen = PTEID_MIN_USER_PIN_LEN;

		sc_format_apdu(card, &apdu, SC_APDU_CASE_1, 0x20, 0x00, (u8) pin_info->pinref);
		apdu.resplen = sizeof(rbuf);
		apdu.resp = rbuf;

		r = sc_transmit_apdu(card, &apdu);
		SC_TEST_RET(card->ctx, r, "Get Status(PIN) APDU transmit failed");
		if (apdu.sw1 == 0x90 && apdu.sw2 == 0x00) {
			pin_info->pintries = pin_info->maxtries;
			return 0;
		}
		else if (apdu.sw1 == 0x63 && (apdu.sw2 & 0xC0) == 0xC0) {
			pin_info->pintries = apdu.sw2 & 0x0F;
			return 0;
		}
		else {
			r = sc_check_sw(card, apdu.sw1, apdu.sw2);
			SC_TEST_RET(card->ctx, r, "Card returned error on Get Status(PIN) APDU");
		}
	}
	}

	return SC_ERROR_NOT_SUPPORTED;
}

static pteid_get_challenge(struct sc_card *card, u8 *rnd, size_t len)
{
	int r;

	card->cla = 0x80;
	r = iso_ops->get_challenge(card, rnd, len);
	card->cla = 0x00;

	return r;
}

static struct sc_card_driver * sc_get_driver(void)
{
	if (iso_ops == NULL)
		iso_ops = sc_get_iso7816_driver()->ops;

	memset(&pteid_ops, 0, sizeof(pteid_ops));

	pteid_ops.match_card = pteid_match_card;
	pteid_ops.init = pteid_init;
	pteid_ops.finish = pteid_finish;

	pteid_ops.select_file = pteid_select_file;
	pteid_ops.read_binary = iso_ops->read_binary;
	pteid_ops.update_binary = iso_ops->update_binary;
	pteid_ops.pin_cmd = iso_ops->pin_cmd;
	pteid_ops.set_security_env = pteid_set_security_env;
	pteid_ops.logout = pteid_logout;

	pteid_ops.compute_signature = pteid_compute_signature;
	pteid_ops.get_challenge = pteid_get_challenge;
	pteid_ops.get_response = iso_ops->get_response;
	pteid_ops.check_sw = iso_ops->check_sw;

	pteid_ops.card_ctl = pteid_card_ctl;

        return &pteid_drv;
}

struct sc_card_driver * sc_get_pteid_driver(void)
{
	return sc_get_driver();
}
