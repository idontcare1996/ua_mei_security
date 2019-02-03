/*
 * card.c: General SmartCard functions
 *
 * Copyright (C) 2001, 2002  Juha Yrj�l� <juha.yrjola@iki.fi>
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
#include "asn1.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
    #include <sys/unistd.h>
#endif

int sc_check_sw(struct sc_card *card, int sw1, int sw2)
{
	assert(card->ops->check_sw != NULL);
	return card->ops->check_sw(card, sw1, sw2);
}

static int sc_check_apdu(struct sc_context *ctx, const struct sc_apdu *apdu)
{
	if (apdu->le > 256) {
		sc_error(ctx, "Value of Le too big (maximum 256 bytes)\n");
		SC_FUNC_RETURN(ctx, 4, SC_ERROR_INVALID_ARGUMENTS);
	}
	if (apdu->lc > 255) {
		sc_error(ctx, "Value of Lc too big (maximum 255 bytes)\n");
		SC_FUNC_RETURN(ctx, 4, SC_ERROR_INVALID_ARGUMENTS);
	}
	switch (apdu->cse) {
	case SC_APDU_CASE_1:
		if (apdu->datalen > 0) {
			sc_error(ctx, "Case 1 APDU with data supplied\n");
			SC_FUNC_RETURN(ctx, 4, SC_ERROR_INVALID_ARGUMENTS);
		}
		break;
	case SC_APDU_CASE_2_SHORT:
		if (apdu->datalen > 0) {
			sc_error(ctx, "Case 2 APDU with data supplied\n");
			SC_FUNC_RETURN(ctx, 4, SC_ERROR_INVALID_ARGUMENTS);
		}
		if (apdu->le == 0) {
			sc_error(ctx, "Case 2 APDU with no response expected\n");
			SC_FUNC_RETURN(ctx, 4, SC_ERROR_INVALID_ARGUMENTS);
		}
		if (apdu->resplen < apdu->le) {
			sc_error(ctx, "Response buffer size < Le\n");
			SC_FUNC_RETURN(ctx, 4, SC_ERROR_INVALID_ARGUMENTS);
		}
		break;
	case SC_APDU_CASE_3_SHORT:
		if (apdu->datalen == 0 || apdu->data == NULL) {
			sc_error(ctx, "Case 3 APDU with no data supplied\n");
			SC_FUNC_RETURN(ctx, 4, SC_ERROR_INVALID_ARGUMENTS);
		}
		break;
	case SC_APDU_CASE_4_SHORT:
		if (apdu->datalen == 0 || apdu->data == NULL) {
			sc_error(ctx, "Case 4 APDU with no data supplied\n");
			SC_FUNC_RETURN(ctx, 4, SC_ERROR_INVALID_ARGUMENTS);
		}
		if (apdu->le == 0) {
			sc_error(ctx, "Case 4 APDU with no response expected\n");
			SC_FUNC_RETURN(ctx, 4, SC_ERROR_INVALID_ARGUMENTS);
		}
		if (apdu->resplen < apdu->le) {
			sc_error(ctx, "Le > response buffer size\n");
			SC_FUNC_RETURN(ctx, 4, SC_ERROR_INVALID_ARGUMENTS);
		}
		break;
	case SC_APDU_CASE_2_EXT:
	case SC_APDU_CASE_3_EXT:
	case SC_APDU_CASE_4_EXT:
		SC_FUNC_RETURN(ctx, 4, SC_ERROR_INVALID_ARGUMENTS);
	}
	return 0;
}

static int sc_transceive(struct sc_card *card, struct sc_apdu *apdu)
{
	u8 sbuf[SC_MAX_APDU_BUFFER_SIZE];
	u8 rbuf[SC_MAX_APDU_BUFFER_SIZE];
	size_t sendsize, recvsize;
	u8 *data = sbuf;
	size_t data_bytes = apdu->lc;
	int r;

	if (card->reader->ops->transmit == NULL)
		return SC_ERROR_NOT_SUPPORTED;
	assert(card->reader->ops->transmit != NULL);
	if (data_bytes == 0)
		data_bytes = 256;
	*data++ = apdu->cla;
	*data++ = apdu->ins;
	*data++ = apdu->p1;
	*data++ = apdu->p2;
	switch (apdu->cse) {
	case SC_APDU_CASE_1:
		break;
	case SC_APDU_CASE_2_SHORT:
		*data++ = (u8) apdu->le;
		break;
	case SC_APDU_CASE_2_EXT:
		*data++ = (u8) 0;
		*data++ = (u8) (apdu->le >> 8);
		*data++ = (u8) (apdu->le & 0xFF);
		break;
	case SC_APDU_CASE_3_SHORT:
		*data++ = (u8) apdu->lc;
		if (apdu->datalen != data_bytes)
			return SC_ERROR_INVALID_ARGUMENTS;
		memcpy(data, apdu->data, data_bytes);
		data += data_bytes;
		break;
	case SC_APDU_CASE_4_SHORT:
		*data++ = (u8) apdu->lc;
		if (apdu->datalen != data_bytes)
			return SC_ERROR_INVALID_ARGUMENTS;
		memcpy(data, apdu->data, data_bytes);
		data += data_bytes;
		if (apdu->le == 256)
			*data++ = 0x00;
		else
			*data++ = (u8) apdu->le;
		break;
	}
	sendsize = data - sbuf;
	recvsize = apdu->resplen + 2;	/* space for the SW's */
	if (card->ctx->debug >= 5) {
		char buf[2048];

		buf[0] = 0;
		if (!apdu->sensitive )
			sc_hex_dump(card->ctx, sbuf, sendsize, buf, sizeof(buf));
		sc_debug(card->ctx, "Sending %d bytes (resp. %d bytes%s):\n%s",
			sendsize, recvsize,
			apdu->sensitive ? ", sensitive" : "", buf);
	}

	r = card->reader->ops->transmit(card->reader, card->slot, sbuf,
					sendsize, rbuf, &recvsize,
					apdu->control);
	if (apdu->sensitive)
		memset(sbuf, 0, sendsize);
	SC_TEST_RET(card->ctx, r, "Unable to transmit");

	assert(recvsize >= 2);
	apdu->sw1 = (unsigned int) rbuf[recvsize-2];
	apdu->sw2 = (unsigned int) rbuf[recvsize-1];
	if (apdu->sensitive)
		rbuf[recvsize-2] = rbuf[recvsize-1] = 0;
	recvsize -= 2;
	if (recvsize > apdu->resplen)
		data_bytes = apdu->resplen;
	else
		data_bytes = apdu->resplen = recvsize;
	if (recvsize > 0) {
		memcpy(apdu->resp, rbuf, data_bytes);
		if (apdu->sensitive)
			memset(rbuf, 0, recvsize);
	}
	return 0;
}

int sc_transmit_apdu(struct sc_card *card, struct sc_apdu *apdu)
{
	int r;
	size_t orig_resplen;
	
	assert(card != NULL && apdu != NULL);
	SC_FUNC_CALLED(card->ctx, 4);
	orig_resplen = apdu->resplen;
	r = sc_check_apdu(card->ctx, apdu);
	SC_TEST_RET(card->ctx, r, "APDU sanity check failed");
	r = sc_lock(card);
	SC_TEST_RET(card->ctx, r, "sc_lock() failed");
	r = sc_transceive(card, apdu);
	if (r != 0) {
		sc_unlock(card);
		SC_TEST_RET(card->ctx, r, "transceive() failed");
	}
	if (card->ctx->debug >= 5) {
		char buf[2048];

		buf[0] = '\0';
		if (apdu->resplen > 0) {
			sc_hex_dump(card->ctx, apdu->resp, apdu->resplen,
				    buf, sizeof(buf));
		}
		sc_debug(card->ctx, "Received %d bytes (SW1=%02X SW2=%02X)\n%s",
		      apdu->resplen, apdu->sw1, apdu->sw2, buf);
	}
	if (apdu->sw1 == 0x6C && apdu->resplen == 0) {
		apdu->resplen = orig_resplen;
		apdu->le = apdu->sw2;

		r = sc_transceive(card, apdu);

        if (r != 0) {
			sc_unlock(card);
			SC_TEST_RET(card->ctx, r, "transceive() failed");
		}
	}
	if (apdu->sw1 == 0x61 && apdu->resplen == 0) {
		unsigned int le;

		if (orig_resplen == 0) {
			apdu->sw1 = 0x90;	/* FIXME: should we do this? */
			apdu->sw2 = 0;
			sc_unlock(card);
			return 0;
		}

		le = apdu->sw2? (size_t) apdu->sw2 : 256;

		if (card->ops->get_response == NULL) {
			sc_unlock(card);
			SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_NOT_SUPPORTED);
		}
		r = card->ops->get_response(card, apdu, le);
		sc_unlock(card);
		if (r < 0) {
			SC_FUNC_RETURN(card->ctx, 2, r);
		}
		else
			return 0;
	}
	sc_unlock(card);
	return 0;
}

void sc_format_apdu(struct sc_card *card, struct sc_apdu *apdu,
		    int cse, int ins, int p1, int p2)
{
	assert(card != NULL && apdu != NULL);
	memset(apdu, 0, sizeof(*apdu));
	apdu->cla = (u8) card->cla;
	apdu->cse = cse;
	apdu->ins = (u8) ins;
	apdu->p1 = (u8) p1;
	apdu->p2 = (u8) p2;

	return;
}

static struct sc_card * sc_card_new()
{
	struct sc_card *card;
	
	card = (struct sc_card *) malloc(sizeof(struct sc_card));
	if (card == NULL)
		return NULL;
	memset(card, 0, sizeof(struct sc_card));
	card->ops = (struct sc_card_operations *) malloc(sizeof(struct sc_card_operations));
	if (card->ops == NULL) {
		free(card);
		return NULL;
	}
	card->max_le = SC_APDU_CHOP_SIZE;
	card->app_count = -1;
	card->magic = SC_CARD_MAGIC;
	card->mutex = sc_mutex_new();

	return card;
}

static void sc_card_free(struct sc_card *card)
{
	assert(sc_card_valid(card));
	sc_free_apps(card);
	if (card->ef_dir != NULL)
		sc_file_free(card->ef_dir);
	free(card->ops);
	if (card->algorithms != NULL)
		free(card->algorithms);
	sc_mutex_free(card->mutex);
	memset(card, 0, sizeof(*card));
	free(card);
}

int sc_connect_card(struct sc_reader *reader, int slot_id,
		    struct sc_card **card_out)
{
	struct sc_card *card;
	struct sc_context *ctx = reader->ctx;
	struct sc_slot_info *slot = _sc_get_slot_info(reader, slot_id);
	struct sc_card_driver *driver;
	int i, r = 0, connected = 0;

	assert(card_out != NULL);
	SC_FUNC_CALLED(ctx, 1);
	if (reader->ops->connect == NULL)
		SC_FUNC_RETURN(ctx, 0, SC_ERROR_NOT_SUPPORTED);
	if (slot == NULL)
		SC_FUNC_RETURN(ctx, 0, SC_ERROR_SLOT_NOT_FOUND);

	card = sc_card_new();
	if (card == NULL)
		SC_FUNC_RETURN(ctx, 1, SC_ERROR_OUT_OF_MEMORY);
	r = reader->ops->connect(reader, slot);
	if (r)
		goto err;
	connected = 1;

	card->reader = reader;
	card->slot = slot;
	card->ctx = ctx;

	memcpy(card->atr, slot->atr, slot->atr_len);
	card->atr_len = slot->atr_len;

	_sc_parse_atr(reader->ctx, slot);

	/* See if the ATR matches any ATR specified in the config file */
	if ((driver = ctx->forced_driver) == NULL) {
		for (i = 0; ctx->card_drivers[i] != NULL; i++) {
			driver = ctx->card_drivers[i];
			if (_sc_match_atr(card, driver->atr_map, NULL) >= 0)
				break;
			driver = NULL;
		}
	}

	if (driver != NULL) {
		/* Forced driver, or matched via ATR mapping from
		 * config file */
		card->driver = driver;
		memcpy(card->ops, card->driver->ops, sizeof(struct sc_card_operations));
		if (card->ops->init != NULL) {
			r = card->ops->init(card);
			if (r) {
				sc_error(ctx, "driver '%s' init() failed: %s\n", card->driver->name,
				      sc_strerror(r));
				goto err;
			}
		}
	} else for (i = 0; ctx->card_drivers[i] != NULL; i++) {
		struct sc_card_driver *drv = ctx->card_drivers[i];
		const struct sc_card_operations *ops = drv->ops;
		
		if (ctx->debug >= 3)
			sc_debug(ctx, "trying driver: %s\n", drv->name);
		if (ops == NULL || ops->match_card == NULL)
			continue;

        memcpy(card->ops, ops, sizeof(struct sc_card_operations));
		card->driver = drv;

		if (ops->match_card(card) != 1)
        {
            struct sc_card_operations tOps = {0};
            memcpy(card->ops, &tOps, sizeof(struct sc_card_operations));
		    card->driver = NULL;
			continue;
        }
		if (ctx->debug >= 3)
			sc_debug(ctx, "matched: %s\n", drv->name);

        r = ops->init(card);
		if (r) {
			sc_error(ctx, "driver '%s' init() failed: %s\n", drv->name,
			      sc_strerror(r));
			if (r == SC_ERROR_INVALID_CARD) {
				card->driver = NULL;
				continue;
			}
			goto err;
		}
		break;
	}
	if (card->driver == NULL) {
		sc_error(ctx, "unable to find driver for inserted card\n");
		r = SC_ERROR_INVALID_CARD;
		goto err;
	}
	if (card->name == NULL)
		card->name = card->driver->name;
	*card_out = card;

	SC_FUNC_RETURN(ctx, 1, 0);
err:
	if (card != NULL)
		sc_card_free(card);
	SC_FUNC_RETURN(ctx, 1, r);
}

int sc_disconnect_card(struct sc_card *card, int action)
{
	struct sc_context *ctx;
    int r = 0;
	assert(sc_card_valid(card));
	ctx = card->ctx;
	SC_FUNC_CALLED(ctx, 1);
	assert(card->lock_count == 0);
	if (card->ops->finish) 
    {
      	sc_debug(ctx, "Disconnect opt finish\n");

		r = card->ops->finish(card);
		if (r)
			sc_error(card->ctx, "card driver finish() failed: %s\n",
			      sc_strerror(r)); 
      	sc_debug(ctx, "Disconnect opt finish done\n");
	}
	if (card->reader->ops->disconnect) {
      	sc_debug(ctx, "Disconnect opt disconnect slot=%d\n", card->slot);
		r = card->reader->ops->disconnect(card->reader, card->slot, action);
		if (r)
			sc_error(card->ctx, "disconnect() failed: %s\n",
			      sc_strerror(r));
      	sc_debug(ctx, "Disconnect opt disconnect done\n");
	}

    sc_debug(ctx, "Before card free\n");
	sc_card_free(card);
    sc_debug(ctx, "After card free\n");
	SC_FUNC_RETURN(ctx, 1, 0);
}

int sc_lock(struct sc_card *card)
{
	int r = 0;
	
	assert(card != NULL);
	SC_FUNC_CALLED(card->ctx, 7);
	sc_mutex_lock(card->mutex);
        if (card->lock_count == 0) {
		if (card->reader->ops->lock != NULL)
			r = card->reader->ops->lock(card->reader, card->slot);
		if (r == 0)
			card->cache_valid = 1;
	}
	if (r == 0)
		card->lock_count++;
	sc_mutex_unlock(card->mutex);
        SC_FUNC_RETURN(card->ctx, 7, r);
}

int sc_unlock(struct sc_card *card)
{
	int r = 0;

	assert(card != NULL);
	SC_FUNC_CALLED(card->ctx, 7);
	sc_mutex_lock(card->mutex);
	assert(card->lock_count >= 1);
	if (card->lock_count == 1) {
		memset(&card->cache, 0, sizeof(card->cache));
		card->cache_valid = 0;

#if 0   /* Don't log out */
		if (card->ops->logout != NULL) {
			sc_mutex_unlock(card->mutex);
			card->ops->logout(card);
			sc_mutex_lock(card->mutex);
		}
#endif
	}
	/* Check again, lock count may have changed
	 * while we were in logout() */
	if (card->lock_count == 1) {
		if (card->reader->ops->unlock != NULL)
			r = card->reader->ops->unlock(card->reader, card->slot);
        }
        card->lock_count--;
        sc_mutex_unlock(card->mutex);
	SC_FUNC_RETURN(card->ctx, 7, r);
}

int sc_list_files(struct sc_card *card, u8 *buf, size_t buflen)
{
	int r;

	assert(card != NULL);
	SC_FUNC_CALLED(card->ctx, 1);
        if (card->ops->list_files == NULL)
		SC_FUNC_RETURN(card->ctx, 1, SC_ERROR_NOT_SUPPORTED);
	r = card->ops->list_files(card, buf, buflen);
        SC_FUNC_RETURN(card->ctx, 1, r);
}

int sc_create_file(struct sc_card *card, struct sc_file *file)
{
	int r;

	assert(card != NULL);
	if (card->ctx->debug >= 1) {
		const sc_path_t *in_path = &file->path;

		sc_debug(card->ctx, "called; type=%d, path=%s\n",
				in_path->type,
				sc_print_path(in_path));
	}
        if (card->ops->create_file == NULL)
		SC_FUNC_RETURN(card->ctx, 1, SC_ERROR_NOT_SUPPORTED);
	r = card->ops->create_file(card, file);
        SC_FUNC_RETURN(card->ctx, 1, r);
}

int sc_delete_file(struct sc_card *card, const struct sc_path *path)
{
	int r;

	assert(card != NULL);
	if (card->ctx->debug >= 1) {
		sc_debug(card->ctx, "called; type=%d, path=%s\n",
				path->type,
				sc_print_path(path));
	}
        if (card->ops->delete_file == NULL)
		SC_FUNC_RETURN(card->ctx, 1, SC_ERROR_NOT_SUPPORTED);
	r = card->ops->delete_file(card, path);
        SC_FUNC_RETURN(card->ctx, 1, r);
}

/** Returns 1 is the last 200 bytes of buf are 0x00 */
static int last200zero(const unsigned char *buf, size_t count)
{
	int i;

	if (count < 200)
		return 0;

	for (i = count - 200; i < count; i++)
	{
		if (buf[i] != 0x00)
			return 0;
	}

	return 1;
}

int sc_read_binary(struct sc_card *card, unsigned int idx,
		   unsigned char *buf, size_t count, unsigned long flags)
{
	int r;

	assert(card != NULL && card->ops != NULL && buf != NULL);
	if (card->ctx->debug >= 2)
		sc_debug(card->ctx, "sc_read_binary: %d bytes at index %d\n", count, idx);
	if (count == 0)
		return 0;
	if (card->ops->read_binary == NULL)
		SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_NOT_SUPPORTED);
	if (count > card->max_le) {
		int bytes_read = 0;
		unsigned char *p = buf;
		int last_zeros = 0; // pteid: optimisation: stop reading if the last 200 bytes are 0x00

		r = sc_lock(card);
		SC_TEST_RET(card->ctx, r, "sc_lock() failed");
		while (count > 0 && !last_zeros) {
			int n = count > card->max_le ? card->max_le : count;
			r = sc_read_binary(card, idx, p, n, flags);
			if (r < 0) {
				sc_unlock(card);
				SC_TEST_RET(card->ctx, r, "sc_read_binary() failed");
			}
			last_zeros = last200zero(p, r);
			p += r;
			idx += r;
			bytes_read += r;
			count -= r;
			if (r == 0) {
				sc_unlock(card);
				SC_FUNC_RETURN(card->ctx, 2, bytes_read);
			}
		}
		sc_unlock(card);
		SC_FUNC_RETURN(card->ctx, 2, bytes_read);
	}
	r = card->ops->read_binary(card, idx, buf, count, flags);
        SC_FUNC_RETURN(card->ctx, 2, r);
}

int sc_read_binary_full(struct sc_card *card, unsigned int idx,
		   unsigned char *buf, size_t count, unsigned long flags)
{
	int r;

	assert(card != NULL && card->ops != NULL && buf != NULL);
	if (card->ctx->debug >= 2)
		sc_debug(card->ctx, "sc_read_binary_full: %d bytes at index %d\n", count, idx);
	if (count == 0)
		return 0;
	if (card->ops->read_binary == NULL)
		SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_NOT_SUPPORTED);
	if (count > card->max_le) {
		int bytes_read = 0;
		unsigned char *p = buf;
		//int last_zeros = 0; // pteid: optimisation: stop reading if the last 200 bytes are 0x00

		r = sc_lock(card);
		SC_TEST_RET(card->ctx, r, "sc_lock() failed");
		while (count > 0 /*&& !last_zeros*/) {
			int n = count > card->max_le ? card->max_le : count;
			r = sc_read_binary_full(card, idx, p, n, flags);
			if (r < 0) {
				sc_unlock(card);
				SC_TEST_RET(card->ctx, r, "sc_read_binary_full() failed");
			}
			//last_zeros = last200zero(p, r);
			p += r;
			idx += r;
			bytes_read += r;
			count -= r;
			if (r == 0) {
				sc_unlock(card);
				SC_FUNC_RETURN(card->ctx, 2, bytes_read);
			}
		}
		sc_unlock(card);
		SC_FUNC_RETURN(card->ctx, 2, bytes_read);
	}
	r = card->ops->read_binary(card, idx, buf, count, flags);
        SC_FUNC_RETURN(card->ctx, 2, r);
}

int sc_write_binary(struct sc_card *card, unsigned int idx,
		    const u8 *buf, size_t count, unsigned long flags)
{
	int r;

	assert(card != NULL && card->ops != NULL && buf != NULL);
	if (card->ctx->debug >= 2)
		sc_debug(card->ctx, "sc_write_binary: %d bytes at index %d\n", count, idx);
	if (count == 0)
		return 0;
	if (card->ops->write_binary == NULL)
		SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_NOT_SUPPORTED);
	if (count > card->max_le) {
		int bytes_written = 0;
		const u8 *p = buf;

		r = sc_lock(card);
		SC_TEST_RET(card->ctx, r, "sc_lock() failed");
		while (count > 0) {
			int n = count > card->max_le ? card->max_le : count;
			r = sc_write_binary(card, idx, p, n, flags);
			if (r < 0) {
				sc_unlock(card);
				SC_TEST_RET(card->ctx, r, "sc_read_binary() failed");
			}
			p += r;
			idx += r;
			bytes_written += r;
			count -= r;
			if (r == 0) {
				sc_unlock(card);
				SC_FUNC_RETURN(card->ctx, 2, bytes_written);
			}
		}
		sc_unlock(card);
		SC_FUNC_RETURN(card->ctx, 2, bytes_written);
	}
	r = card->ops->write_binary(card, idx, buf, count, flags);
        SC_FUNC_RETURN(card->ctx, 2, r);
}

int sc_update_binary(struct sc_card *card, unsigned int idx,
		     const u8 *buf, size_t count, unsigned long flags)
{
	int r;

	assert(card != NULL && card->ops != NULL && buf != NULL);
	if (card->ctx->debug >= 2)
		sc_debug(card->ctx, "sc_update_binary: %d bytes at index %d\n", count, idx);
	if (count == 0)
		return 0;
	if (card->ops->update_binary == NULL)
		SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_NOT_SUPPORTED);
	if (count > card->max_le) {
		int bytes_written = 0;
		const u8 *p = buf;

		r = sc_lock(card);
		SC_TEST_RET(card->ctx, r, "sc_lock() failed");
		while (count > 0) {
			int n = count > card->max_le ? card->max_le : count;
			r = sc_update_binary(card, idx, p, n, flags);
			if (r < 0) {
				sc_unlock(card);
				SC_TEST_RET(card->ctx, r, "sc_update_binary() failed");
			}
			p += r;
			idx += r;
			bytes_written += r;
			count -= r;
			if (r == 0) {
				sc_unlock(card);
				SC_FUNC_RETURN(card->ctx, 2, bytes_written);
			}
		}
		sc_unlock(card);
		SC_FUNC_RETURN(card->ctx, 2, bytes_written);
	}
	r = card->ops->update_binary(card, idx, buf, count, flags);
        SC_FUNC_RETURN(card->ctx, 2, r);
}

int sc_select_file(struct sc_card *card,
		   const struct sc_path *in_path,
		   struct sc_file **file)
{
	int r;

	assert(card != NULL && in_path != NULL);
	if (card->ctx->debug >= 1) {
		sc_debug(card->ctx, "called; type=%d, path=%s\n",
				in_path->type,
				sc_print_path(in_path));
	}
	if (in_path->len > SC_MAX_PATH_SIZE)
		SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_INVALID_ARGUMENTS);
	if (in_path->type == SC_PATH_TYPE_PATH) {
		/* Perform a sanity check */
		size_t i;
		if ((in_path->len & 1) != 0)
			SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_INVALID_ARGUMENTS);
		for (i = 0; i < in_path->len/2; i++) {
			u8 p1 = in_path->value[2*i],
			   p2 = in_path->value[2*i+1];
			if ((p1 == 0x3F && p2 == 0x00) && i != 0)
				SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_INVALID_ARGUMENTS);
		}
	}
        if (card->ops->select_file == NULL)
		SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_NOT_SUPPORTED);
	r = card->ops->select_file(card, in_path, file);
	/* Remember file path */
	if (r == 0 && file && *file)
		(*file)->path = *in_path;
        SC_FUNC_RETURN(card->ctx, 1, r);
}

int sc_get_challenge(struct sc_card *card, u8 *rnd, size_t len)
{
	int r;

	assert(card != NULL);
	SC_FUNC_CALLED(card->ctx, 2);
        if (card->ops->get_challenge == NULL)
		SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_NOT_SUPPORTED);
	r = card->ops->get_challenge(card, rnd, len);
        SC_FUNC_RETURN(card->ctx, 2, r);
}

int sc_read_record(struct sc_card *card, unsigned int rec_nr, u8 *buf,
		   size_t count, unsigned long flags)
{
	int r;

	assert(card != NULL);
	SC_FUNC_CALLED(card->ctx, 2);
        if (card->ops->read_record == NULL)
		SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_NOT_SUPPORTED);
	r = card->ops->read_record(card, rec_nr, buf, count, flags);
        SC_FUNC_RETURN(card->ctx, 2, r);
}

int sc_write_record(struct sc_card *card, unsigned int rec_nr, const u8 * buf,
		    size_t count, unsigned long flags)
{
	int r;

	assert(card != NULL);
	SC_FUNC_CALLED(card->ctx, 2);
        if (card->ops->write_record == NULL)
		SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_NOT_SUPPORTED);
	r = card->ops->write_record(card, rec_nr, buf, count, flags);
        SC_FUNC_RETURN(card->ctx, 2, r);
}

int sc_append_record(struct sc_card *card, const u8 * buf, size_t count,
		     unsigned long flags)
{
	int r;

	assert(card != NULL);
	SC_FUNC_CALLED(card->ctx, 2);
        if (card->ops->append_record == NULL)
		SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_NOT_SUPPORTED);
	r = card->ops->append_record(card, buf, count, flags);
        SC_FUNC_RETURN(card->ctx, 2, r);
}

int sc_update_record(struct sc_card *card, unsigned int rec_nr, const u8 * buf,
		     size_t count, unsigned long flags)
{
	int r;

	assert(card != NULL);
	SC_FUNC_CALLED(card->ctx, 2);
        if (card->ops->update_record == NULL)
		SC_FUNC_RETURN(card->ctx, 2, SC_ERROR_NOT_SUPPORTED);
	r = card->ops->update_record(card, rec_nr, buf, count, flags);
        SC_FUNC_RETURN(card->ctx, 2, r);
}

inline int sc_card_valid(const struct sc_card *card) {
#ifndef NDEBUG
	assert(card != NULL);
#endif
	return card->magic == SC_CARD_MAGIC;
}

int
sc_card_ctl(struct sc_card *card, unsigned long cmd, void *args)
{
	int r = SC_ERROR_NOT_SUPPORTED;

	assert(card != NULL);
	SC_FUNC_CALLED(card->ctx, 2);
        if (card->ops->card_ctl != NULL)
		r = card->ops->card_ctl(card, cmd, args);

	/* suppress "not supported" error messages */
	if (r == SC_ERROR_NOT_SUPPORTED) {
		sc_debug(card->ctx, "card_ctl(%lu) not supported\n", cmd);
		return r;
	}
        SC_FUNC_RETURN(card->ctx, 2, r);
}

int _sc_card_add_algorithm(struct sc_card *card, const struct sc_algorithm_info *info)
{
	struct sc_algorithm_info *p;
	
	assert(sc_card_valid(card) && info != NULL);
	card->algorithms = (struct sc_algorithm_info *) realloc(card->algorithms, (card->algorithm_count + 1) * sizeof(*info));
	if (card->algorithms == NULL) {
		card->algorithm_count = 0;
		return SC_ERROR_OUT_OF_MEMORY;
	}
	p = card->algorithms + card->algorithm_count;
	card->algorithm_count++;
	*p = *info;
	return 0;
}

int _sc_card_add_rsa_alg(struct sc_card *card, unsigned int key_length,
			 unsigned long flags, unsigned long exponent)
{
	struct sc_algorithm_info info;

	memset(&info, 0, sizeof(info));
	info.algorithm = SC_ALGORITHM_RSA;
	info.key_length = key_length;
	info.flags = flags;
	info.u._rsa.exponent = exponent;
	
	return _sc_card_add_algorithm(card, &info);
}

struct sc_algorithm_info * _sc_card_find_rsa_alg(struct sc_card *card,
						 unsigned int key_length)
{
	int i;

	for (i = 0; i < card->algorithm_count; i++) {
		struct sc_algorithm_info *info = &card->algorithms[i];
		
		if (info->algorithm != SC_ALGORITHM_RSA)
			continue;
		if (info->key_length != key_length)
			continue;
		return info;
	}
	return NULL;
}

int _sc_match_atr(struct sc_card *card, struct sc_atr_table *table, int *id_out)
{
	const u8 *atr = card->atr;
	size_t atr_len = card->atr_len;
	int i = 0;
	
	if (table == NULL)
		return -1;

	for (i = 0; table[i].atr != NULL; i++) {
		if (table[i].atr_len != atr_len)
			continue;
		if (memcmp(table[i].atr, atr, atr_len) != 0)
			continue;
		if (id_out != NULL)
			*id_out = table[i].id;
		return i;
	}
	return -1;
}

int _sc_add_atr(struct sc_card_driver *driver,
		 const u8 *atr, size_t atrlen, int id)
{
	struct sc_atr_table *map, *entry;
	u8 *dst_atr;

	map = (struct sc_atr_table *) realloc(driver->atr_map,
			(driver->natrs + 2) * sizeof(struct sc_atr_table));
	if (!map)
		return SC_ERROR_OUT_OF_MEMORY;
	driver->atr_map = map;
	if (!(dst_atr = (u8 *) malloc(atrlen)))
		return SC_ERROR_OUT_OF_MEMORY;
	entry = &driver->atr_map[driver->natrs++];
	memset(entry+1, 0, sizeof(*entry));
	entry->atr = dst_atr;
	entry->atr_len = atrlen;
	entry->id = id;
	memcpy(dst_atr, atr, atrlen);
	return 0;
}