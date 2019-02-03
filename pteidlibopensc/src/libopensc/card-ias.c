/*
 * card-ias.c: Support for IAS applet
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

#define PTEID_MIN_USER_PIN_LEN     4
#define PTEID_MAX_USER_PIN_LEN     8
#define PTEID_MAX_PIN_TRIES        3

#undef OLD_TEST_CARDS
//#define OLD_TEST_CARDS

/* To be removed */
#include <time.h>
static long t1, t2, tot_read = 0, tot_dur = 0, dur;

#define PTEID_VERSION			"1.0"

#define IAS_MAX_USER_PIN_LEN     8

struct ias_priv_data {
	int lang;
};

#define DRVDATA(card)        ((struct ias_priv_data *) ((card)->drv_data))

static struct sc_card_operations ias_ops;
static struct sc_card_driver ias_drv = {
	"Portugal eID (IAS)",
	"pteid_ias",
	&ias_ops
};
static const struct sc_card_operations *iso_ops = NULL;

static unsigned char aid_ias1[] = {0x60, 0x46, 0x32, 0xFF, 0x00, 0x01, 0x02};

#ifdef OLD_TEST_CARDS
static unsigned char aid_ias2[] = {0xA0, 0x00, 0x00, 0x00, 0x30, 0x30, 0x04, 0x02, 0x01}; // old test cards
#endif

static int ias_finish(struct sc_card *card)
{
	free(DRVDATA(card));
	return 0;
}

static int ias_match_card(struct sc_card *card)
{
	int	r;
    struct sc_path pathAid;
    struct sc_apdu apdu;

	sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xA4, 0x04, 0x00);	
    apdu.lc = sizeof(aid_ias1);
    apdu.data = aid_ias1;
    apdu.datalen = sizeof(aid_ias1);
    apdu.resplen = 0;
    apdu.le = 0;
    r = sc_transmit_apdu(card, &apdu);
	card->ctx->log_errors = 0;
    r = sc_check_sw(card, apdu.sw1, apdu.sw2);
	card->ctx->log_errors = 1;

#ifdef OLD_TEST_CARDS
	if (r != SC_NO_ERROR) {
	    sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xA4, 0x04, 0x00);	
	    apdu.lc = sizeof(aid_ias2);
	    apdu.data = aid_ias2;
	    apdu.datalen = sizeof(aid_ias2);
	    apdu.resplen = 0;
	    apdu.le = 0;

	    r = sc_transmit_apdu(card, &apdu);
		card->ctx->log_errors = 0;
	    r = sc_check_sw(card, apdu.sw1, apdu.sw2);
		card->ctx->log_errors = 1;
	}
#endif

	return r == SC_NO_ERROR;
}

static int ias_init(struct sc_card *card)
{
	struct ias_priv_data *priv = NULL;
	scconf_block *conf_block;

	sc_debug(card->ctx, "pteid (ias) V%s\n", PTEID_VERSION);

	priv = (struct ias_priv_data *) calloc(1, sizeof(struct ias_priv_data));
	if (priv == NULL)
		return SC_ERROR_OUT_OF_MEMORY;
	card->drv_data = priv;
	card->cla = 0x00;
	_sc_card_add_rsa_alg(card, 1024,
		SC_ALGORITHM_RSA_PAD_PKCS1 | SC_ALGORITHM_RSA_HASH_NONE, 0);

	/* State that we have an RNG */
	card->caps |= SC_CARD_CAP_RNG;

	card->max_pin_len = IAS_MAX_USER_PIN_LEN;

	return 0;
}


static void add_acl_entry(struct sc_file *file, int op, u8 byte)
{
	unsigned int method, key_ref = SC_AC_KEY_REF_NONE;

	if (byte == 0)
		method = SC_AC_NONE;
	else if (byte == 0xFF)
		method = SC_AC_NEVER;
	else if (byte == 0x11) {
		method = SC_AC_CHV;
		key_ref = 0x01;
	}
	else if (byte == 0x12) {
		method = SC_AC_CHV;
		key_ref = 0x02;
	}

	sc_file_add_acl_entry(file, op, method, key_ref);
}

/* We only parse the first AMB + SCB combination, §5.5*/
static int parse_security_attributes(struct sc_context *ctx,
	const u8 *buf, int buflen, int file_type, struct sc_file *file)
{
	int len = buf[1];
	u8 amb = buf[2];
	int scb_idx = 0;
	int scb_count = ((amb&0x80)==0x80) + ((amb&0x40)==0x40) + ((amb&0x20)==0x20) + ((amb&0x10)==0x10) +
		((amb&0x08)==0x08) + ((amb&0x04)==0x04) + ((amb&0x02)==0x02) + ((amb&0x01)==0x01);

	if ((file_type == SC_FILE_TYPE_WORKING_EF) && (file_type == SC_FILE_TYPE_DF))
		return -30;

	if (len != buflen)
		return -31;
	if (1 + scb_count > len)
		return -32;

	if (amb & 0x80)
		scb_idx++;
	if (amb & 0x40) {
		scb_idx++;
		if (file_type == SC_FILE_TYPE_DF)
			add_acl_entry(file, SC_AC_OP_DELETE, buf[2 + scb_idx]);
	}
	if (amb & 0x20)
		scb_idx++;
	if (amb & 0x10) {
		scb_idx++;
		if (file_type == SC_FILE_TYPE_DF)
			add_acl_entry(file, SC_AC_OP_REHABILITATE, buf[2 + scb_idx]);
	}
	if (amb & 0x08) {
		scb_idx++;
		if (file_type == SC_FILE_TYPE_DF)
			add_acl_entry(file, SC_AC_OP_INVALIDATE, buf[2 + scb_idx]);
	}
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
			add_acl_entry(file, SC_AC_OP_WRITE, buf[2 + scb_idx]);
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

/* Parse FCI, §4.2 */
static int ias_parse_fci(struct sc_context *ctx, const u8 *buf, int buflen,
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
	file->type = -1;
	sec_attr = NULL;
	file->status = -1;

	buf += 2;
	buflen -= 2;
	while (buflen >= 2) {
		switch(buf[0]) {
		case 0x80:                 /* file size */
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
		case 0x83:                /* file ID -> ignore (we already know it) */
			if (buf[1] != 0x02)
				return -7;
			break;
		case 0x88:                /* shot file ID -> ignore */
			if (buf[1] != 0x01)
				return -8;
			break;
		case 0xA5:                /* prop. data, BER encoded -> ignore */
			break;
		case 0x85:                /* prop. data, not BER encoded -> ignore */
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
			sec_attr_len = buf[1];
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
	SC_TEST_RET(ctx, r, "Parsing ACs failed");

	file->magic = SC_FILE_MAGIC;

	return 0;
}

static int ias_select_file(struct sc_card *card,
			       const struct sc_path *in_path,
			       struct sc_file **file_out)
{
	int select_root = 0;
	struct sc_apdu apdu;
	u8 pathbuf[SC_MAX_PATH_SIZE];
	u8 rbuf[SC_MAX_APDU_BUFFER_SIZE];
	int r;

	if (in_path->len == 4 && in_path->value[3] == 0x00 &&
	    (in_path->value[2] == 0x4F || in_path->value[2] == 0x5F))
	    	select_root = 1;

try_again:
	if (select_root) {
		sc_format_apdu(card, &apdu, SC_APDU_CASE_1, 0xA4, 0x03, 0x0C);
		apdu.lc = apdu.datalen = 0;
		apdu.le = apdu.resplen = 0;

		r = sc_transmit_apdu(card, &apdu);
		SC_TEST_RET(card->ctx, r, "Selecting MF failed");
	}

	if (file_out == NULL) {
		sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xA4, 0x09, 0x0C);
		apdu.le = apdu.resplen = 0;		
	}
	else {
		sc_format_apdu(card, &apdu, SC_APDU_CASE_4_SHORT, 0xA4, 0x09, 0x00);
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

	r = sc_transmit_apdu(card, &apdu);
	SC_TEST_RET(card->ctx, r, "Sending Select File command failed");
	card->ctx->log_errors = 0;
	r = sc_check_sw(card, apdu.sw1, apdu.sw2);
	card->ctx->log_errors = 1;
	if ((r == SC_ERROR_FILE_NOT_FOUND) && !select_root) {
		select_root = 1;
		goto try_again;
	}
	if (r < 0)
	{
		sc_error(card->ctx, "Select(%s) failed: %s (%d)\n", sc_print_path(in_path), sc_strerror(r), r);
		return r;
	}

	if (file_out != NULL) {
		*file_out = sc_file_new();
		if (*file_out == NULL)
			SC_FUNC_RETURN(card->ctx, 0, SC_ERROR_OUT_OF_MEMORY);
		r = ias_parse_fci(card->ctx, apdu.resp, apdu.resplen, *file_out);
		if (r < 0)
			sc_file_free(*file_out);
		SC_TEST_RET(card->ctx, r, "Pärsing of FCI failed");
		(*file_out)->path = *in_path;
	}

	return 0;
}

static int ias_set_security_env(struct sc_card *card,
				    const struct sc_security_env *env,
				    int se_num)
{
	struct ias_priv_data *priv = (struct ias_priv_data *) DRVDATA(card);
	struct sc_apdu apdu;
	u8 sbuf[SC_MAX_APDU_BUFFER_SIZE];
	int r;

	assert(card != NULL && env != NULL);

	sc_debug(card->ctx, "ias_set_security_env(), keyRef = 0x%0x, algo = 0x%0x\n",
		*env->key_ref, env->algorithm_flags);

	if (env->algorithm_flags != SC_ALGORITHM_RSA_PAD_PKCS1) {
		SC_TEST_RET(card->ctx, SC_ERROR_INVALID_ARGUMENTS, "Checking algorithm for MSE SET command");
	}

	/* See § 8.4 */
	sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0x22, 0x41, 0xA4);
	memcpy(sbuf, "\x95\x01\x40\x84\x01\x00\x80\x01\x02", 11);
	sbuf[5] = *env->key_ref;
	apdu.data = sbuf;
	apdu.datalen = apdu.lc = 9;
	apdu.le = apdu.resplen = 0;

	r = sc_transmit_apdu(card, &apdu);
	SC_TEST_RET(card->ctx, r, "Set Security Env APDU transmit failed");

	r = sc_check_sw(card, apdu.sw1, apdu.sw2);
	SC_TEST_RET(card->ctx, r, "Card's Set Security Env command returned error");

	return r;
}

static int ias_compute_signature(struct sc_card *card, const u8 *data,
				  size_t data_len, u8 * out, size_t outlen)
{
	struct sc_apdu apdu;
	u8 sbuf[SC_MAX_APDU_BUFFER_SIZE];
	int r;
	size_t i;
	
	if (data_len > 64) {
		sc_error(card->ctx, "Illegal input length: %d\n", data_len);
		return SC_ERROR_INVALID_ARGUMENTS;
	}
	if (outlen < data_len) {
		sc_error(card->ctx, "Output buffer too small.\n");
		return SC_ERROR_BUFFER_TOO_SMALL;
	}

	/* See § 10.11.6 */
	sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0x88, 0x02, 0x00);
	apdu.lc = data_len = apdu.datalen = data_len;
	memcpy(sbuf, data, data_len);
	apdu.data = sbuf;
	apdu.resplen = outlen > sizeof(sbuf) ? sizeof(sbuf) : outlen;
	apdu.resp = sbuf;

	r = sc_transmit_apdu(card, &apdu);
	SC_TEST_RET(card->ctx, r, "APDU transmit failed");
	if (apdu.sw1 == 0x6a && apdu.sw2 == 0x88)
		return SC_ERROR_SECURITY_STATUS_NOT_SATISFIED;
	else {
		r = sc_check_sw(card, apdu.sw1, apdu.sw2);
		SC_TEST_RET(card->ctx, r, "Card returned error");
	}

	memcpy(out, sbuf, apdu.resplen);

	return apdu.resplen;
}

static int ias_logout(struct sc_card *card)
{
	return 0;
}

static int ias_get_pin_tries(struct sc_card *card, struct sc_cardctl_pteid_pin_info *pin_info)
{
	sc_apdu_t apdu;
	u8 rbuf[2];
	int buflen, r;

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
		if (r >= 0 || r == SC_ERROR_CARD_CMD_FAILED)
			return r;
		SC_TEST_RET(card->ctx, r, "Card returned error on Get Status(PIN) APDU");
	}

	return r;
}

static int ias_card_ctl(struct sc_card *card, unsigned long cmd, void *ptr)
{
	switch (cmd) {
	case SC_CARDCTL_PTEID_GET_PIN_INFO:
		{
			struct sc_cardctl_pteid_pin_info *pin_info = ptr;
			int r;

			pin_info->maxtries = PTEID_MAX_PIN_TRIES;
			pin_info->maxlen = PTEID_MAX_USER_PIN_LEN;
			pin_info->minlen = PTEID_MIN_USER_PIN_LEN;

			r = ias_get_pin_tries(card, pin_info);
			if (r == SC_ERROR_CARD_CMD_FAILED) {
				// Very likely the wrong dir -> go to 5F00 and try again
				struct sc_path path;
				sc_format_path("3F005F00", &path);
				ias_select_file(card, &path, NULL);
				r = ias_get_pin_tries(card, pin_info);
			}
			SC_TEST_RET(card->ctx, r, "Card returned error on ias_card_ctl(PIN_INFO)");
			return r;
		}
		break;
	}

	return SC_ERROR_NOT_SUPPORTED;
}

static int ias_change_pin(struct sc_card * card, struct sc_pin_cmd_data *data,
			int *tries_left)
{
	u8 sbuf[SC_MAX_APDU_BUFFER_SIZE];
	struct sc_apdu apdu;
	int r, len, pad = (data->flags & SC_PIN_CMD_NEED_PADDING) ? 1 : 0;

	if ((r = sc_build_pin(sbuf, sizeof(sbuf), &data->pin2, pad)) < 0)
		return r;
	len = r;

	sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT,
				0x24, 0x01, data->pin_reference);

	apdu.lc = len;
	apdu.datalen = len;
	apdu.data = sbuf;
	apdu.resplen = 0;
	apdu.sensitive = 1;

	r = sc_transmit_apdu(card, &apdu);
	SC_TEST_RET(card->ctx, r, "ias_change_pin(): APDU transmit failed");

	SC_FUNC_RETURN(card->ctx, 2, sc_check_sw(card, apdu.sw1, apdu.sw2));
}

static int ias_reset_retry_counter(struct sc_card * card, struct sc_pin_cmd_data *data, int change)
{
	struct sc_apdu apdu;
	int r;

	if (!change) {   // Only reset the counter, no change
		sc_format_apdu(card, &apdu, SC_APDU_CASE_NONE, 0x2C, 0x03, data->pin_reference);
		apdu.lc = 0;
		apdu.datalen = 0;
		apdu.resplen = 0;

		r = sc_transmit_apdu(card, &apdu);
		SC_TEST_RET(card->ctx, r, "ias_reset_retry_counter(): APDU transmit failed");
	
		SC_FUNC_RETURN(card->ctx, 2, sc_check_sw(card, apdu.sw1, apdu.sw2));
	}
	else {               // Reset the counter + change PIN
		u8 sbuf[SC_MAX_APDU_BUFFER_SIZE];
		int pad = (data->flags & SC_PIN_CMD_NEED_PADDING) ? 1 : 0;
		if ((r = sc_build_pin(sbuf, sizeof(sbuf), &data->pin2, pad)) < 0)
			return r;

		sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0x2C, 0x02, data->pin_reference);
		apdu.lc = r;
		apdu.datalen = r;
		apdu.data = sbuf;
		apdu.resplen = 0;
		apdu.sensitive = 1;

		r = sc_transmit_apdu(card, &apdu);
		SC_TEST_RET(card->ctx, r, "ias_reset_retry_counter(): APDU transmit failed");
	
		SC_FUNC_RETURN(card->ctx, 2, sc_check_sw(card, apdu.sw1, apdu.sw2));
	}
}

/**
 * For the IAS card, a PIN change and unblock with PIN change consists
 * of 2 APDUS:
 * - first a verify with PIN resp. PUK
 * - then a Change Reference Data
 * If no pinpad reader is used, then the PIN dialog box already appeared
 * (if the PIN/PUK values weren't specified) and so we can here simply
 * do the 2 commands separately.
 * But in case of a pinpad reader, the PIN notification dialog will be
 * displayed down in the pin_cmd() call. And if we would call it twice,
 * then a PIN notification dialog will appear twice, which is not OK. And
 * therefore we call the pin_cmd() just once with the necessary info
 * (the SC_PIN_CMD_DOUBLE) so the pinpad handling functions know what to do
 * with it.
 * Note: an unblock without PIN change also requires 2 APDUS (Verify PUK
 * and Reset Retry Counter), but for this second APDU no PIN value is needed
 * so we can do this with 2 separate pin_cmd() calls.
 */
static int ias_pin_cmd(sc_card_t * card, struct sc_pin_cmd_data *data,
			int *tries_left)
{
	int r = SC_ERROR_INVALID_ARGUMENTS;
	int pin_ref_backup = data->pin_reference;
	int puk_ref_backup = data->puk_reference;
	int cmd_backup = data->cmd;

	SC_FUNC_CALLED(card->ctx, 3);

	data->pin1.offset = 5;
	data->pin1.length_offset = 4;
	data->pin2.offset = 5;
	data->pin2.length_offset = 4;
	data->apdu = NULL;

	if (data->cmd == SC_PIN_CMD_UNBLOCK) {
		switch(data->pin_reference) {
			case 0x01: data->puk_reference = 0x11; break;
			case 0x82: data->puk_reference = 0x92; break;
			case 0x83: data->puk_reference = 0x93; break;
			default: 
				sc_error(card->ctx, "ias_pin_cmd(): unknown pin ref 0x%0x\n", data->pin_reference);
				return SC_ERROR_INVALID_ARGUMENTS;
		}

		if (data->pin_reference == 0x01) {
			struct sc_path path;
			sc_format_path("3F00", &path);
			r = ias_select_file(card, &path, NULL);
			SC_TEST_RET(card->ctx, r,  "select MF before unblocking Auth PIN failed");
		}
	}

	if (!(data->flags & SC_PIN_CMD_USE_PINPAD)) {
		if (data->cmd == SC_PIN_CMD_VERIFY)
			r = iso_ops->pin_cmd(card, data, tries_left);
		else if (data->cmd == SC_PIN_CMD_CHANGE) {
			data->cmd = SC_PIN_CMD_VERIFY;
			r = iso_ops->pin_cmd(card, data, tries_left);
			data->cmd = SC_PIN_CMD_CHANGE;
			if (r >= 0)
				r = ias_change_pin(card, data, tries_left);
		}
		else if (data->cmd == SC_PIN_CMD_UNBLOCK) {
			// Trick: replace the PIN data by the PUK data
			data->pin_reference = data->puk_reference;
			
			data->cmd = SC_PIN_CMD_VERIFY;
			r = iso_ops->pin_cmd(card, data, tries_left);
			data->cmd = SC_PIN_CMD_UNBLOCK;

			data->pin_reference = pin_ref_backup;

			if (r >= 0)
				r = ias_reset_retry_counter(card, data, data->flags & SC_PIN_CMD_NEW_PIN);
		}
		else
			sc_error(card->ctx, "ias_pin_cmd(): unknown pin cmd (0x%0x) and flags(0x%0x)\n", data->cmd, data->flags);
	}
	else {
		// If PIN change or unblock with PIN change, we'll do a double operation in pin_cmd()
		if (data->cmd == SC_PIN_CMD_CHANGE || (data->cmd == SC_PIN_CMD_UNBLOCK && (data->flags & SC_PIN_CMD_NEW_PIN)))
			data->flags |= SC_PIN_CMD_DOUBLE;

		if (data->cmd == SC_PIN_CMD_UNBLOCK) {
			data->pin_reference = data->puk_reference;
			data->puk_reference = pin_ref_backup;
		}

		data->cmd_gui = data->cmd;
		data->cmd = SC_PIN_CMD_VERIFY;

		r = iso_ops->pin_cmd(card, data, tries_left);

		data->pin_reference = pin_ref_backup;
		data->puk_reference = puk_ref_backup;
		data->cmd = cmd_backup;

		if ((r >= 0) && (data->cmd == SC_PIN_CMD_UNBLOCK) && !(data->flags & SC_PIN_CMD_NEW_PIN))
			r = ias_reset_retry_counter(card, data, 0);
	}

	return r;
}

static struct sc_card_driver * sc_get_driver(void)
{
	if (iso_ops == NULL)
		iso_ops = sc_get_iso7816_driver()->ops;

	memset(&ias_ops, 0, sizeof(ias_ops));

	ias_ops.match_card = ias_match_card;
	ias_ops.init = ias_init;
	ias_ops.finish = ias_finish;

	ias_ops.select_file = ias_select_file;
	ias_ops.read_binary = iso_ops->read_binary;
	ias_ops.update_binary = iso_ops->update_binary;
	ias_ops.pin_cmd = ias_pin_cmd;
	ias_ops.set_security_env = ias_set_security_env;
	ias_ops.logout = ias_logout;

	ias_ops.compute_signature = ias_compute_signature;
	ias_ops.get_challenge = iso_ops->get_challenge;
	ias_ops.get_response = iso_ops->get_response;
	ias_ops.check_sw = iso_ops->check_sw;

	ias_ops.card_ctl = ias_card_ctl;

        return &ias_drv;
}

#if 1
struct sc_card_driver * sc_get_ias_driver(void)
{
	return sc_get_driver();
}
#endif
