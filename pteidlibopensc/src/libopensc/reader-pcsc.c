/*
 * reader-pcsc.c: Reader driver for PC/SC Lite
 *
 * Copyright (C) 2002  Juha Yrj��<juha.yrjola@iki.fi>
 * Copyright (C) 2004, Martin Paljak <martin@paljak.pri.ee>
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

#undef DUMP_APDUS
//#define DUMP_APDUS   // MAKE SURE THIS IS TURNED OFF!!! */

#include "internal.h"
#ifdef HAVE_PCSC
#include "ctbcs.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __APPLE__
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#else
#include <winscard.h>
#endif

#define PINPAD_ENABLED

#ifdef PINPAD_ENABLED

static int pinpad_transmit_gui(struct sc_reader *reader, struct sc_slot_info *slot,
			 const u8 *sendbuf, size_t sendsize,
			 u8 *recvbuf, size_t *recvsize,
			 int control, struct sc_pin_cmd_data *pin_data);

static int part10_build_modifyunblock_pin_block(u8 * buf, size_t * size, struct sc_pin_cmd_data *data);
static int part10_build_verify_pin_block(u8 * buf, size_t * size, struct sc_pin_cmd_data *data);

#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#include <Winsock.h>
#else
#ifndef __APPLE__
#include <sys/io.h>
#endif
#include <dirent.h>
#include <arpa/inet.h>
#endif
#include "part10.h"
#define PTEIDPP_IMPORT
#include "pteidpinpad1.h"
#endif

/* Default timeout value for SCardGetStatusChange
 * Needs to be increased for some broken PC/SC
 * Lite implementations.
 */
#ifndef SC_CUSTOM_STATUS_TIMEOUT
#define SC_STATUS_TIMEOUT 0
#else
#define SC_STATUS_TIMEOUT SC_CUSTOM_STATUS_TIMEOUT
#endif

#define SCARD_PROTOCOL_PCSC_ANY (SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1)

#ifdef _WIN32
/* Some windows specific kludge */
#define SCARD_SCOPE_GLOBAL SCARD_SCOPE_USER

/* Error printing */
#define PCSC_ERROR(ctx, desc, rv) sc_error(ctx, desc ": %lx\n", rv);

#else

#define PCSC_ERROR(ctx, desc, rv) sc_error(ctx, desc ": %s\n", pcsc_stringify_error(rv));

#endif

/* Default value for apdu_fix option */
#ifndef _WIN32
# define DEF_APDU_FIX	0
#else
# define DEF_APDU_FIX	1
#endif

#define GET_SLOT_PTR(s, i) (&(s)->slot[(i)])
#define GET_PRIV_DATA(r) ((struct pcsc_private_data *) (r)->drv_data)
#define GET_SLOT_DATA(r) ((struct pcsc_slot_data *) (r)->drv_data)

#ifdef PINPAD_ENABLED

struct pinpad_info {
	void *module;
	void *init;
	void *cmd;
	tGuiInfo gui_info;
};
typedef struct pinpad_info pinpad_info_t;

static long pinpad_load_module(struct sc_reader *reader);

#endif

struct pcsc_global_private_data {
	SCARDCONTEXT pcsc_ctx;
	int apdu_fix;  /* flag to indicate whether to 'fix' some T=0 APDUs */
};

struct pcsc_private_data {
	SCARDCONTEXT pcsc_ctx;
	char *reader_name;
	struct pcsc_global_private_data *gpriv;
#ifdef PINPAD_ENABLED
	struct pinpad_info pinpad;
#endif
};

struct pcsc_slot_data {
	SCARDHANDLE pcsc_card;
	SCARD_READERSTATE_A readerState;
	DWORD verify_ioctl;
	DWORD verify_ioctl_start;
	DWORD verify_ioctl_finish;
	
	DWORD modify_ioctl;
	DWORD modify_ioctl_start;
	DWORD modify_ioctl_finish;
	int locked;
};

#ifdef DUMP_APDUS
static void dumphex(char *msg, const u8 *buf, int len)
{
	int i;

	printf("%s: ", msg);
	for (i = 0; i < len; i++)
		printf("%02X ", buf[i]);
	printf(" (%d bytes)\n", len);
}
#endif

static int pcsc_detect_card_presence(struct sc_reader *reader, struct sc_slot_info *slot);

static int pcsc_ret_to_error(long rv)
{
	switch (rv) {
	case SCARD_W_REMOVED_CARD:
		return SC_ERROR_CARD_REMOVED;
	case SCARD_W_RESET_CARD:
		return SC_ERROR_CARD_RESET;
	case SCARD_E_NOT_TRANSACTED:
		return SC_ERROR_TRANSMIT_FAILED;
	case SCARD_W_UNRESPONSIVE_CARD:
		return SC_ERROR_CARD_UNRESPONSIVE;
	default:
		return SC_ERROR_UNKNOWN;
	}
}

static unsigned int pcsc_proto_to_opensc(DWORD proto)
{
	switch (proto) {
	case SCARD_PROTOCOL_T0:
		return SC_PROTO_T0;
	case SCARD_PROTOCOL_T1:
		return SC_PROTO_T1;
	case SCARD_PROTOCOL_RAW:
		return SC_PROTO_RAW;
	default:
		return 0;
	}
}

static DWORD opensc_proto_to_pcsc(unsigned int proto)
{
	switch (proto) {
	case SC_PROTO_T0:
		return SCARD_PROTOCOL_T0;
	case SC_PROTO_T1:
		return SCARD_PROTOCOL_T1;
	case SC_PROTO_RAW:
		return SCARD_PROTOCOL_RAW;
	default:
		return 0;
	}
}

static int pcsc_transmit(struct sc_reader *reader, struct sc_slot_info *slot,
			 const u8 *sendbuf, size_t sendsize,
			 u8 *recvbuf, size_t *recvsize,
			 int control)
{
	SCARD_IO_REQUEST sSendPci, sRecvPci;
	DWORD dwSendLength, dwRecvLength;
	LONG rv;
	SCARDHANDLE card;
	struct pcsc_slot_data *pslot = GET_SLOT_DATA(slot);
	struct pcsc_private_data *prv = GET_PRIV_DATA(reader);

	assert(pslot != NULL);
	card = pslot->pcsc_card;

	sSendPci.dwProtocol = opensc_proto_to_pcsc(slot->active_protocol);
	sSendPci.cbPciLength = sizeof(sSendPci);
	sRecvPci.dwProtocol = opensc_proto_to_pcsc(slot->active_protocol);
	sRecvPci.cbPciLength = sizeof(sRecvPci);
	
	if (prv->gpriv->apdu_fix && sendsize >= 6
	 && slot->active_protocol == SC_PROTO_T0) {
		/* Check if the APDU in question is of Case 4 */
		const u8 *p = sendbuf;
		int lc;
		
		p += 4;
		lc = *p;
		if (lc == 0)
			lc = 256;
		if (sendsize == lc + 6) {
			/* Le is present, cut it out */
			sc_debug(reader->ctx, "Cutting out Le byte from Case 4 APDU\n");
			sendsize--;
		}
	}

#ifdef DUMP_APDUS
	dumphex("  sent", sendbuf, sendsize);
#endif

	dwSendLength = sendsize;
	dwRecvLength = *recvsize;
	if (!control) {
		rv = SCardTransmit(card, &sSendPci, sendbuf, dwSendLength,
				   &sRecvPci, recvbuf, &dwRecvLength);
	} else {
#ifdef DUMP_APDUS
		printf("  ctrl: 0x%0x\n", control);
#endif
#ifndef __OLD_PCSC_API__
		rv = SCardControl(card, control, sendbuf, dwSendLength,
				  recvbuf, dwRecvLength, &dwRecvLength);
#else
		rv = SCardControl(card, sendbuf, dwSendLength,
				  recvbuf, &dwRecvLength);
#endif
	}

#ifdef DUMP_APDUS
	if (rv == 0)
		dumphex("    recv", recvbuf, dwRecvLength);
	else
		printf("    recv: ERR = 0x%0x (%d)\n", rv, rv);
#endif

	if (rv != SCARD_S_SUCCESS) {
		switch (rv) {
		case SCARD_W_REMOVED_CARD:
			return SC_ERROR_CARD_REMOVED;
		case SCARD_W_RESET_CARD:
			return SC_ERROR_CARD_RESET;
		case SCARD_E_NOT_TRANSACTED:
			if ((pcsc_detect_card_presence(reader, slot) &
			    SC_SLOT_CARD_PRESENT) == 0)
				return SC_ERROR_CARD_REMOVED;
			return SC_ERROR_TRANSMIT_FAILED;
		default:
			/* Windows' PC/SC returns 0x8010002f (??) if a card is removed */
			if (pcsc_detect_card_presence(reader, slot) != 1)
				return SC_ERROR_CARD_REMOVED;
			if (control)
				return SC_ERROR_NOT_SUPPORTED;
			else
				PCSC_ERROR(reader->ctx, "SCardTransmit failed", rv);
			return SC_ERROR_TRANSMIT_FAILED;
		}
	}
	if (!control && dwRecvLength < 2)
		return SC_ERROR_UNKNOWN_DATA_RECEIVED;
	*recvsize = dwRecvLength;
	
	return 0;
}

static int refresh_slot_attributes(struct sc_reader *reader, struct sc_slot_info *slot)
{
	struct pcsc_private_data *priv = GET_PRIV_DATA(reader);
	struct pcsc_slot_data *pslot = GET_SLOT_DATA(slot);
	LONG ret;

	if (pslot->readerState.szReader == NULL) {
		pslot->readerState.szReader = priv->reader_name;
		pslot->readerState.dwCurrentState = SCARD_STATE_UNAWARE;
		pslot->readerState.dwEventState = SCARD_STATE_UNAWARE;
	} else {
		pslot->readerState.dwCurrentState = pslot->readerState.dwEventState;
	}

	ret = SCardGetStatusChange(priv->pcsc_ctx, SC_STATUS_TIMEOUT, &pslot->readerState, 1);
	if (ret == SCARD_E_TIMEOUT) { /* timeout: nothing changed */
		slot->flags &= ~SCARD_STATE_CHANGED;
		return 0;
	}
	if (ret != 0) {
		PCSC_ERROR(reader->ctx, "SCardGetStatusChange failed", ret);
		return pcsc_ret_to_error(ret);
	}
	if (pslot->readerState.dwEventState & SCARD_STATE_PRESENT) {
		int old_flags = slot->flags;
		int maybe_changed = 0;

		slot->flags |= SC_SLOT_CARD_PRESENT;
		slot->atr_len = pslot->readerState.cbAtr;
		if (slot->atr_len > SC_MAX_ATR_SIZE)
			slot->atr_len = SC_MAX_ATR_SIZE;
		memcpy(slot->atr, pslot->readerState.rgbAtr, slot->atr_len);

#ifndef _WIN32
		/* On Linux, SCARD_STATE_CHANGED always means there was an
		 * insert or removal. But we may miss events that way. */ 
		if (pslot->readerState.dwEventState & SCARD_STATE_CHANGED) {
			slot->flags |= SC_SLOT_CARD_CHANGED; 
		} else { 
			maybe_changed = 1; 
		} 
#else
		/* On windows, SCARD_STATE_CHANGED is turned on by lots of 
		 * other events, so it gives us a lot of false positives. 
		 * But if it's off, there really no change */ 
		if (pslot->readerState.dwEventState & SCARD_STATE_CHANGED) { 
			maybe_changed = 1; 
		} 
#endif
		/* If we aren't sure if the card state changed, check if 
		 * the card handle is still valid. If the card changed, 
		 * the handle will be invalid. */
		slot->flags &= ~SC_SLOT_CARD_CHANGED;
		if (maybe_changed && (old_flags & SC_SLOT_CARD_PRESENT)) {
			DWORD readers_len = 0, state, prot, atr_len = 32;
			unsigned char atr[32];
			int rv = SCardStatus(pslot->pcsc_card, NULL, &readers_len,
				&state,	&prot, atr, &atr_len);
			if (rv == SCARD_W_REMOVED_CARD)
				slot->flags |= SC_SLOT_CARD_CHANGED;
		}
	} else {
		slot->flags &= ~(SC_SLOT_CARD_PRESENT|SC_SLOT_CARD_CHANGED);
	}
	return 0;
}

static int pcsc_detect_card_presence(struct sc_reader *reader, struct sc_slot_info *slot)
{
	int rv;

	if ((rv = refresh_slot_attributes(reader, slot)) < 0)
		return rv;
	return slot->flags;
}

/* Wait for an event to occur.
 * This function ignores the list of slots, because with
 * pcsc we have a 1:1 mapping of readers and slots anyway
 */
static int pcsc_wait_for_event(struct sc_reader **readers,
			       struct sc_slot_info **slots,
			       size_t nslots,
                               unsigned int event_mask,
                               int *reader,
			       unsigned int *event, int timeout)
{
	struct sc_context *ctx;
	SCARDCONTEXT pcsc_ctx;
	LONG ret;
	SCARD_READERSTATE_A rgReaderStates[SC_MAX_READERS];
	unsigned long on_bits, off_bits;
	time_t end_time, now, delta;
	int i;

	/* Prevent buffer overflow */
	if (nslots >= SC_MAX_READERS)
	       return SC_ERROR_INVALID_ARGUMENTS;

	on_bits = off_bits = 0;
	if (event_mask & SC_EVENT_CARD_INSERTED) {
		event_mask &= ~SC_EVENT_CARD_INSERTED;
		on_bits |= SCARD_STATE_PRESENT;
	}
	if (event_mask & SC_EVENT_CARD_REMOVED) {
		event_mask &= ~SC_EVENT_CARD_REMOVED;
		off_bits |= SCARD_STATE_PRESENT;
	}
	if (event_mask != 0)
	       return SC_ERROR_INVALID_ARGUMENTS;

	/* Find out the current status */
	ctx = readers[0]->ctx;
	pcsc_ctx = GET_PRIV_DATA(readers[0])->pcsc_ctx;
	for (i = 0; i < nslots; i++) {
		struct pcsc_private_data *priv = GET_PRIV_DATA(readers[i]);

		rgReaderStates[i].szReader = priv->reader_name;
		rgReaderStates[i].dwCurrentState = SCARD_STATE_UNAWARE;
		rgReaderStates[i].dwEventState = SCARD_STATE_UNAWARE;

		/* Can we handle readers from different PCSC contexts? */
		if (priv->pcsc_ctx != pcsc_ctx)
			return SC_ERROR_INVALID_ARGUMENTS;
	}

	ret = SCardGetStatusChange(pcsc_ctx, 0, rgReaderStates, nslots);
	if (ret != 0) {
		PCSC_ERROR(ctx, "SCardGetStatusChange(1) failed", ret);
		return pcsc_ret_to_error(ret);
	}

	time(&now);
	end_time = now + (timeout + 999) / 1000;

	/* Wait for a status change and return if it's a card insert/removal
	 */
	for( ; ; ) {
		SCARD_READERSTATE_A *rsp;

		/* Scan the current state of all readers to see if they
		 * match any of the events we're polling for */
		*event = 0;
	       	for (i = 0, rsp = rgReaderStates; i < nslots; i++, rsp++) {
			unsigned long state, prev_state;

			prev_state = rsp->dwCurrentState;
			state = rsp->dwEventState;
			if ((state & on_bits & SCARD_STATE_PRESENT) &&
			    (prev_state & SCARD_STATE_EMPTY))
				*event |= SC_EVENT_CARD_INSERTED;
			if ((~state & off_bits & SCARD_STATE_PRESENT) &&
			    (prev_state & SCARD_STATE_PRESENT))
				*event |= SC_EVENT_CARD_REMOVED;
			if (*event) {
				*reader = i;
			       	return 0;
		       	}

			/* No match - copy the state so pcscd knows
			 * what to watch out for */
			rsp->dwCurrentState = rsp->dwEventState;
	       	}

		/* Set the timeout if caller wants to time out */
		if (timeout == 0)
			return SC_ERROR_EVENT_TIMEOUT;
		if (timeout > 0) {
			time(&now);
			if (now >= end_time)
				return SC_ERROR_EVENT_TIMEOUT;
			delta = end_time - now;
		} else {
			delta = 3600;
		}

		ret = SCardGetStatusChange(pcsc_ctx, 1000 * delta,
				rgReaderStates, nslots);
	       	if (ret == SCARD_E_TIMEOUT) {
			if (timeout < 0)
				continue;
		       	return SC_ERROR_EVENT_TIMEOUT;
		}
	       	if (ret != 0) {
		       	PCSC_ERROR(ctx, "SCardGetStatusChange(2) failed", ret);
		       	return pcsc_ret_to_error(ret);
	       	}

	}
}

static int pcsc_connect(struct sc_reader *reader, struct sc_slot_info *slot)
{
	DWORD proto, active_proto;
	SCARDHANDLE card_handle;
	LONG rv;
	struct pcsc_private_data *priv = GET_PRIV_DATA(reader);
	struct pcsc_slot_data *pslot = GET_SLOT_DATA(slot);
	int r;
#ifdef PINPAD_ENABLED
	u8 feature_buf[256];
	DWORD i;
	size_t feature_len;
	PCSC_TLV_STRUCTURE *pcsc_tlv;
#endif

	r = refresh_slot_attributes(reader, slot);
	if (r)
		return r;
	if (!(slot->flags & SC_SLOT_CARD_PRESENT))
		return SC_ERROR_CARD_NOT_PRESENT;

	/* Xiring pin pad readers on Win98/NT need the SCARD_PROTOCOL_DEFAULT flag */
	proto = SCARD_PROTOCOL_PCSC_ANY;

#ifdef _WIN32
if (strstr(priv->reader_name, "Xiring") != NULL)
	proto |= SCARD_PROTOCOL_DEFAULT;
#endif

        rv = SCardConnect(priv->pcsc_ctx, priv->reader_name,
			  SCARD_SHARE_SHARED, proto,
                          &card_handle, &active_proto);
	if (rv != 0) {
		PCSC_ERROR(reader->ctx, "SCardConnect failed", rv);
		return pcsc_ret_to_error(rv);
	}
	slot->active_protocol = pcsc_proto_to_opensc(active_proto);
	pslot->pcsc_card = card_handle;

	/* check for pinpad support */
#ifdef PINPAD_ENABLED
	memset(&priv->pinpad, 0, sizeof(priv->pinpad));
	r = pinpad_load_module(reader);
	sc_debug(reader->ctx, "pinpad_load_module() returned 0x%0x (%d)\n", r, r);

	sc_debug(reader->ctx, "Requesting reader features ... \n");
	feature_len = sizeof(feature_buf);
	rv = pinpad_transmit_gui(reader, slot, NULL, 0, feature_buf, &feature_len, CM_IOCTL_GET_FEATURE_REQUEST, NULL);
	if (rv == SCARD_S_SUCCESS) {
		
		if (!(feature_len % sizeof(PCSC_TLV_STRUCTURE))) {
			/* get the number of elements instead of the complete size */
			feature_len /= sizeof(PCSC_TLV_STRUCTURE);

			pcsc_tlv = (PCSC_TLV_STRUCTURE *)feature_buf;
			for (i = 0; i < feature_len; i++) {
				if (pcsc_tlv[i].tag == FEATURE_VERIFY_PIN_DIRECT) {
					pslot->verify_ioctl = ntohl(pcsc_tlv[i].value);
				} else if (pcsc_tlv[i].tag == FEATURE_VERIFY_PIN_START) {
					pslot->verify_ioctl_start = ntohl(pcsc_tlv[i].value);
				} else if (pcsc_tlv[i].tag == FEATURE_VERIFY_PIN_FINISH) {
					pslot->verify_ioctl_finish = ntohl(pcsc_tlv[i].value);
				} else if (pcsc_tlv[i].tag == FEATURE_MODIFY_PIN_DIRECT) {
					pslot->modify_ioctl = ntohl(pcsc_tlv[i].value);
				} else if (pcsc_tlv[i].tag == FEATURE_MODIFY_PIN_START) {
					pslot->modify_ioctl_start = ntohl(pcsc_tlv[i].value);
				} else if (pcsc_tlv[i].tag == FEATURE_MODIFY_PIN_FINISH) {
					pslot->modify_ioctl_finish = ntohl(pcsc_tlv[i].value);
				} else {
					sc_debug(reader->ctx, "Reader pinpad feature: %02x not supported\n", pcsc_tlv[i].tag);
				}
			}
			
			/* Set slot capabilities based on detected IOCTLs */
			if (pslot->verify_ioctl || (pslot->verify_ioctl_start && pslot->verify_ioctl_finish)) {
				sc_debug(reader->ctx, "Reader supports pinpad PIN verification\n");
				slot->capabilities |= SC_SLOT_CAP_PIN_PAD;
			}
			if (pslot->modify_ioctl || (pslot->modify_ioctl_start && pslot->modify_ioctl_finish)) {
				sc_debug(reader->ctx, "Reader supports pinpad PIN modification\n");
				slot->capabilities |= SC_SLOT_CAP_PIN_PAD;
				}
		} else
			sc_debug(reader->ctx, "Inconsistent TLV from reader!\n");
	} else {
		sc_debug(reader->ctx, "SCardControl failed, err = 0x%0x (%d)\n", rv, rv);
	}
#endif /* PINPAD_ENABLED */

	return 0;
}

static int pcsc_disconnect(struct sc_reader *reader, struct sc_slot_info *slot,
			   int action)
{
	DWORD dwDisposition = SCARD_LEAVE_CARD;
	struct pcsc_slot_data *pslot = GET_SLOT_DATA(slot);

	if (action == SC_DISCONNECT_AND_RESET)
		dwDisposition = SCARD_RESET_CARD;
	else if (action == SC_DISCONNECT_AND_UNPOWER)
		dwDisposition = SCARD_UNPOWER_CARD;
	else if (action == SC_DISCONNECT_AND_EJECT)
		dwDisposition = SCARD_EJECT_CARD;

	SCardDisconnect(pslot->pcsc_card, dwDisposition);
	memset(pslot, 0, sizeof(*pslot));
	slot->flags = 0;
	return 0;
}
                                          
static int pcsc_lock(struct sc_reader *reader, struct sc_slot_info *slot)
{
	long rv;
	struct pcsc_slot_data *pslot = GET_SLOT_DATA(slot);

	assert(pslot != NULL);
        rv = SCardBeginTransaction(pslot->pcsc_card);
        if (rv != SCARD_S_SUCCESS) {
		PCSC_ERROR(reader->ctx, "SCardBeginTransaction failed", rv);
                return pcsc_ret_to_error(rv);
        }
	return 0;
}

static int pcsc_unlock(struct sc_reader *reader, struct sc_slot_info *slot)
{
	long rv;
	struct pcsc_slot_data *pslot = GET_SLOT_DATA(slot);

	assert(pslot != NULL);
	rv = SCardEndTransaction(pslot->pcsc_card, SCARD_LEAVE_CARD);
	if (rv != SCARD_S_SUCCESS) {
		PCSC_ERROR(reader->ctx, "SCardEndTransaction failed", rv);
                return pcsc_ret_to_error(rv);
	}
	return 0;
}

static int pcsc_release(struct sc_reader *reader)
{
    int i;
	struct pcsc_private_data *priv = GET_PRIV_DATA(reader);

	free(priv->reader_name);
	free(priv);
	for (i = 0; i < reader->slot_count; i++) {
	    if (reader->slot[i].drv_data == NULL) {
	        free(reader->slot[i].drv_data);
	    	reader->slot[i].drv_data = NULL;
	    }
	}
	return 0;
}

static struct sc_reader_operations pcsc_ops;

static const struct sc_reader_driver pcsc_drv = {
	"PC/SC Lite Resource Manager",
	"pcsc",
	&pcsc_ops
};

static int pcsc_init(struct sc_context *ctx, void **reader_data)
{
	LONG rv;
	DWORD reader_buf_size = 0;
	char *reader_buf = NULL, *p = NULL;
	char *mszGroups = NULL;
	SCARDCONTEXT pcsc_ctx;
	int r, i;
	struct pcsc_global_private_data *gpriv;
	scconf_block **blocks = NULL, *conf_block = NULL;

        rv = SCardEstablishContext(SCARD_SCOPE_GLOBAL,
                                   NULL,
                                   NULL,
				   &pcsc_ctx);
	if (rv != SCARD_S_SUCCESS)
		return pcsc_ret_to_error(rv);
	SCardListReaders(pcsc_ctx, NULL, NULL,
			 (LPDWORD) &reader_buf_size);
	if (reader_buf_size < 2) {
		SCardReleaseContext(pcsc_ctx);
		return 0;	/* No readers configured */
	}
	gpriv = (struct pcsc_global_private_data *) malloc(sizeof(struct pcsc_global_private_data));
	if (gpriv == NULL) {
		SCardReleaseContext(pcsc_ctx);
		return SC_ERROR_OUT_OF_MEMORY;
	}
	gpriv->pcsc_ctx = pcsc_ctx;
	gpriv->apdu_fix = DEF_APDU_FIX;
	*reader_data = gpriv;
	
	reader_buf = (char *) malloc(sizeof(char) * reader_buf_size);
	SCardListReaders(pcsc_ctx, mszGroups, reader_buf,
			 (LPDWORD) &reader_buf_size);
	p = reader_buf;
	do {
		struct sc_reader *reader = (struct sc_reader *) malloc(sizeof(struct sc_reader));
		struct pcsc_private_data *priv = (struct pcsc_private_data *) malloc(sizeof(struct pcsc_private_data));
		struct pcsc_slot_data *pslot = (struct pcsc_slot_data *) malloc(sizeof(struct pcsc_slot_data));
		struct sc_slot_info *slot;
		
		if (reader == NULL || priv == NULL || pslot == NULL) {
			if (reader)
				free(reader);
			if (priv)
				free(priv);
			if (pslot)
				free(pslot);
			break;
		}

		memset(reader, 0, sizeof(*reader));
		reader->drv_data = priv;
		reader->ops = &pcsc_ops;
		reader->driver = &pcsc_drv;
		reader->slot_count = 1;
		reader->name = strdup(p);
		priv->gpriv = gpriv;
		priv->pcsc_ctx = pcsc_ctx;
		priv->reader_name = strdup(p);
		r = _sc_add_reader(ctx, reader);
		if (r) {
			free(priv->reader_name);
			free(priv);
			free(reader->name);
			free(reader);
			free(pslot);
			break;
		}
		slot = &reader->slot[0];
		memset(slot, 0, sizeof(*slot));
		slot->drv_data = pslot;
		memset(pslot, 0, sizeof(*pslot));
		refresh_slot_attributes(reader, slot);

		while (*p++ != 0);
	} while (p < (reader_buf + reader_buf_size - 1));
	free(reader_buf);
	
	for (i = 0; ctx->conf_blocks[i] != NULL; i++) {
		blocks = scconf_find_blocks(ctx->conf, ctx->conf_blocks[i],
					    "reader_driver", "pcsc");
		conf_block = blocks[0];
		free(blocks);
		if (conf_block != NULL)
			break;
	}

	if (conf_block != NULL)
		gpriv->apdu_fix = scconf_get_bool(conf_block, "apdu_fix", DEF_APDU_FIX);
	
	return 0;
}

static int pcsc_finish(struct sc_context *ctx, void *prv_data)
{
	struct pcsc_global_private_data *priv = (struct pcsc_global_private_data *) prv_data;

	if (priv) {
		SCardReleaseContext(priv->pcsc_ctx);
		free(priv);
	}
	
	return 0;
}

const struct sc_reader_driver * sc_get_pcsc_driver(void)
{
	pcsc_ops.init = pcsc_init;
	pcsc_ops.finish = pcsc_finish;
	pcsc_ops.transmit = pcsc_transmit;
	pcsc_ops.detect_card_presence = pcsc_detect_card_presence;
	pcsc_ops.lock = pcsc_lock;
	pcsc_ops.unlock = pcsc_unlock;
	pcsc_ops.release = pcsc_release;
	pcsc_ops.connect = pcsc_connect;
	pcsc_ops.disconnect = pcsc_disconnect;
	pcsc_ops.perform_verify = ctbcs_pin_cmd;
	pcsc_ops.wait_for_event = pcsc_wait_for_event;
	
	return &pcsc_drv;
}

/**************************************************************************/

/*
 * Pinpad support, based on PC/SC v2 Part 10 interface
 * Similar to CCID in spirit.
 */

#ifdef PINPAD_ENABLED

static unsigned long g_language = PP_LANG_PT;

static void pinpad_cleanup_module(pinpad_info_t *pinpad)
{
	tGuiInfo *gui_info = &pinpad->gui_info;

	if (pinpad->module)
		scdl_close(pinpad->module);

	if (gui_info->csVerifyInfo)
		free(gui_info->csVerifyInfo);
	if (gui_info->csChangeInfo)
		free(gui_info->csChangeInfo);
	if (gui_info->csUnblockNochangeInfo)
		free(gui_info->csUnblockNochangeInfo);
	if (gui_info->csUnblockChangeInfo)
		free(gui_info->csUnblockChangeInfo);
	if (gui_info->csUnblockMergeNoChangeInfo)
		free(gui_info->csUnblockMergeNoChangeInfo);
	if (gui_info->csUnblockMergeChangeInfo)
		free(gui_info->csUnblockMergeChangeInfo);

	memset(pinpad, 0, sizeof(pinpad_info_t));
}

static long pinpad_check_module(char *path, struct sc_reader *reader)
{
	long (*ppinit)(unsigned char, SCARDCONTEXT, const char *, unsigned long, tGuiInfo *, unsigned long, void *);
	struct pcsc_private_data *priv = GET_PRIV_DATA(reader);
	pinpad_info_t *pinpad = &priv->pinpad;
	tGuiInfo *gui_info = &pinpad->gui_info;
	long r;

	pinpad->module = (void *) scdl_open(path);
	if (pinpad->module == NULL)
		return SC_ERROR_OBJECT_NOT_FOUND;

	pinpad->init = (void *) scdl_get_address(pinpad->module, "PTEIDPP_Init");
	ppinit = pinpad->init;
	if (pinpad->init == NULL)
		sc_error(reader->ctx, "Function \"PTEIDPP_Init\" not found in pinpad lib \"%\"\n", path);
	pinpad->cmd = (void *) scdl_get_address(pinpad->module, "PTEIDPP_Command");
	if (pinpad->cmd == NULL)
		sc_error(reader->ctx, "Function \"PTEIDPP_Command\" not found in pinpad lib \"%\"\n", path);
	if (pinpad->init == NULL || pinpad->cmd == NULL) {
		pinpad_cleanup_module(pinpad);
		return SC_ERROR_OBJECT_NOT_FOUND;
	}

	gui_info->csVerifyInfo = (unsigned char *) malloc(DLG_INFO_CHARS + 2);
	gui_info->csChangeInfo = (unsigned char *) malloc(DLG_INFO_CHARS + 2);
	gui_info->csUnblockNochangeInfo = (unsigned char *) malloc(DLG_INFO_CHARS + 2);
	gui_info->csUnblockChangeInfo = (unsigned char *) malloc(DLG_INFO_CHARS + 2);
	gui_info->csUnblockMergeNoChangeInfo = (unsigned char *) malloc(DLG_INFO_CHARS + 2);
	gui_info->csUnblockMergeChangeInfo = (unsigned char *) malloc(DLG_INFO_CHARS + 2);
	if (!gui_info->csVerifyInfo || !gui_info->csChangeInfo || !gui_info->csUnblockNochangeInfo ||
		!gui_info->csUnblockChangeInfo || !gui_info->csUnblockMergeNoChangeInfo || !gui_info->csUnblockMergeChangeInfo) {
			pinpad_cleanup_module(pinpad);
			return SC_ERROR_MEMORY_FAILURE;
	}
	gui_info->csVerifyInfo[0] = gui_info->csChangeInfo[0] = gui_info->csUnblockNochangeInfo[0] = '\0';
	gui_info->csUnblockChangeInfo[0] = gui_info->csUnblockMergeNoChangeInfo[0] = gui_info->csUnblockMergeChangeInfo[0] = '\0';

	r = ppinit(PTEID_MINOR_VERSION, priv->pcsc_ctx,
			reader->name, g_language, gui_info, 0, NULL);
	if (r != 0)
		pinpad_cleanup_module(pinpad);

	return r;
}

#ifdef _WIN32
static long pinpad_load_module(struct sc_reader *reader)
{
	char dir[PATH_MAX];
	char searchfor[PATH_MAX];
	char path[PATH_MAX];
    struct _finddata_t c_file;
    long hFile = -1;
    int res = 0;
    int lng;
    long r = SC_ERROR_OBJECT_NOT_FOUND;

	searchfor[0] = '\0';
	GetSystemDirectory(dir, sizeof(dir) - 25);
	sprintf(searchfor, "%s\\pteidpp\\pteidpp1*.*", dir);

	r = reader->ctx->dlg.pGuiGetLang(&lng);
	if (r == SCGUI_OK)
		g_language = lng;

    hFile = _findfirst(searchfor, &c_file);
    if (hFile != -1) {
		do {
			if (strlen(dir) + 20 + strlen(c_file.name) < sizeof(path))
			{
				sprintf(path, "%s\\pteidpp\\\\%s", dir, c_file.name);
				r = pinpad_check_module(path, reader);
				if (r == 0)
					break; /* This pinpad lib is OK */
			}

			res = _findnext(hFile, &c_file);
		}
		while (res == 0);

		_findclose( hFile );
	}

	return r;
}
#else
static long pinpad_load_module(struct sc_reader *reader)
{
	char *dirpath = "/usr/local/lib/pteidpp";
	char *prefix = "libpteidpp1";
#ifdef __APPLE__
	char *ext = ".dylib";
#else
	char *ext = ".so";
#endif
	char path[PATH_MAX];
	DIR *dir = NULL;
	struct dirent *file;
    int lng;
    long r = SC_ERROR_OBJECT_NOT_FOUND;

	r = reader->ctx->dlg.pGuiGetLang(&lng);
	if (r == SCGUI_OK)
		g_language = lng;

	dir = opendir(dirpath);
 	if (dir != NULL) {
		for (file = readdir(dir); file != NULL; file = readdir(dir))
		{
			size_t filenamelen = strlen(file->d_name);
			if (filenamelen > strlen(prefix) + 1 + strlen(ext) &&
				filenamelen < PATH_MAX < 30 &&
			    memcmp(file->d_name, prefix, strlen(prefix)) == 0 &&
			    memcmp(file->d_name + filenamelen - strlen(ext), ext, strlen(ext)) == 0)
			    {
					sprintf(path, "%s/%s", dirpath, file->d_name);
					r = pinpad_check_module(path, reader);
					if (r == 0)
						break; /* This pinpad lib is OK */
			    }
		}
		closedir(dir);
	}

	return r;
}

#endif

static int pinpad_check_result(const u8 *rbuf, int rcount, sc_apdu_t *apdu)
{
	int r = 0;

	/* We expect only two bytes of result data (SW1 and SW2) */
	if (rcount != 2)
		return SC_ERROR_UNKNOWN_DATA_RECEIVED;

	/* Extract the SWs for the result APDU */
	apdu->sw1 = (unsigned int) rbuf[rcount - 2];
	apdu->sw2 = (unsigned int) rbuf[rcount - 1];

	switch (((unsigned int) apdu->sw1 << 8) | apdu->sw2) {
	case 0x6400: /* Input timed out */
		r = SC_ERROR_KEYPAD_TIMEOUT;   
		break;
	case 0x6401: /* Input cancelled */
		r = SC_ERROR_KEYPAD_CANCELLED; 
		break;
	case 0x6402: /* PINs don't match */
		r = SC_ERROR_KEYPAD_PIN_MISMATCH;
		break;
	}

	return r;
}

static int pinpad_transmit(struct sc_reader *reader, struct sc_slot_info *slot,
	const u8 *sendbuf, size_t sendsize,
	u8 *recvbuf, size_t *recvsize, int control,
	unsigned char pintype, unsigned char operation, pinpad_info_t *pp_info)
{
	int r = 0;
	DWORD dwRecvLength = *recvsize;
	struct pcsc_slot_data *pslot = GET_SLOT_DATA(slot);
	long (*pp_cmd)(SCARDHANDLE , int, const unsigned char *, DWORD,
		unsigned char *, DWORD , DWORD *, unsigned char,
		unsigned char,	unsigned long , void *);

	if (pp_info->cmd) {
#ifdef DUMP_APDUS
		dumphex("  PP cmd, via lib", sendbuf, sendsize);
#endif
		/* Send the command to the pinpad lib */
		pp_cmd = pp_info->cmd;
		r = pp_cmd(pslot->pcsc_card, control, sendbuf, (DWORD) sendsize,
				recvbuf, dwRecvLength, &dwRecvLength, pintype, operation, 0, NULL);
		*recvsize = dwRecvLength;
	}
	else {
#ifdef DUMP_APDUS
		dumphex("  PP cmd, not via lib", sendbuf, sendsize);
#endif
		r = pcsc_transmit(reader, slot, sendbuf, sendsize, recvbuf, recvsize, control);
	}

#ifdef DUMP_APDUS
	if (r == 0)
		dumphex("    recv", recvbuf, dwRecvLength);
	else
		printf("    recv: ERR = 0x%0x (%d)\n", r, r);
#endif

	return r;
}

static int pinpad_transmit_2nd_cmd(struct sc_reader *reader, struct sc_slot_info *slot,
	u8 *recvbuf, size_t *recvsize, unsigned char pintype, pinpad_info_t *pp_info,
	struct sc_pin_cmd_data *pin_data)
{
	int r = 0;
	struct pcsc_slot_data *pslot = GET_SLOT_DATA(slot);
	u8 sendbuf[SC_MAX_APDU_BUFFER_SIZE];
	size_t sendsize = 0;
	unsigned int flags_backup = pin_data->flags;
	struct sc_apdu *apdu_backup = pin_data->apdu;
	struct sc_apdu apdu;
	unsigned char apdu_data[8];

	// Hardcoded for the IAS card
	// TODO: implement a more generic way (e.g. by adding a 2nd apdu field in sc_pin_cmd_data)
	memset(apdu_data, 0x2F, 8);
	memset(&apdu, 0, sizeof(apdu));
	apdu.cla = 0x00;
	apdu.cse = SC_APDU_CASE_3_SHORT;
	apdu.ins = pin_data->cmd_gui == SC_PIN_CMD_CHANGE ? 0x24 : 0x2C;
	apdu.p1 = pin_data->cmd_gui == SC_PIN_CMD_CHANGE ? 0x01 : 0x02;
	apdu.p2 = pin_data->cmd_gui == SC_PIN_CMD_CHANGE ? pin_data->pin_reference : pin_data->puk_reference;
	apdu.lc = 8;
	apdu.datalen = 8;
	apdu.data = apdu_data;
	apdu.resplen = 0;
	apdu.sensitive = 1;

	pin_data->flags |= SC_PIN_CMD_2ND;
	pin_data->apdu = &apdu;
	r = part10_build_modifyunblock_pin_block(sendbuf, &sendsize, pin_data);
	pin_data->flags = flags_backup;
	pin_data->apdu = apdu_backup;

	SC_TEST_RET(reader->ctx, r, "Second Part10 PIN block building failed!");

	r = pinpad_transmit(reader, slot, sendbuf, sendsize, recvbuf, recvsize,
		pslot->modify_ioctl, pintype, PTEIDPP_OP_CHANGE, pp_info);	


	return r;
}

static int pinpad_transmit_gui(struct sc_reader *reader, struct sc_slot_info *slot,
			 const u8 *sendbuf, size_t sendsize,
			 u8 *recvbuf, size_t *recvsize,
			 int control, struct sc_pin_cmd_data *pin_data)
{
	int idx;
	char *msg = NULL;
	unsigned char pintype = 0, operation = 0;
	int showgui = 0;
	unsigned long handle;	struct pcsc_private_data *priv = GET_PRIV_DATA(reader);
	pinpad_info_t *pp_info = &priv->pinpad;
	int r = 0;

	if (pin_data != NULL) {
		if (pin_data->cmd_gui == 0)
			pin_data->cmd_gui = pin_data->cmd;
		if (pin_data->cmd_gui == SC_PIN_CMD_UNBLOCK) {
			if (pin_data->flags & SC_PIN_CMD_MERGE_PUK)
				idx = 4;
			else
				idx = 2;
			if (pin_data->flags & SC_PIN_CMD_NEW_PIN)
				idx++;
		}
		else if (pin_data->cmd_gui == SC_PIN_CMD_CHANGE)
			idx = 1;
		else
			idx = 0;

		pintype = (unsigned char) pin_data->icon;
		operation = (unsigned char) (1 + idx);

		if (pp_info->cmd != NULL)
		{
			switch(idx) {
				case 0: msg = pp_info->gui_info.csVerifyInfo; break;
				case 1: msg = pp_info->gui_info.csChangeInfo; break;
				case 2: msg = pp_info->gui_info.csUnblockNochangeInfo; break;
				case 3: msg = pp_info->gui_info.csUnblockChangeInfo; break;
				case 4: msg = pp_info->gui_info.csUnblockMergeNoChangeInfo; break;
				case 5: msg = pp_info->gui_info.csUnblockMergeChangeInfo; break;
			}
		}
		else
			msg = "";

		showgui = strcmp(msg, "r") != 0;
	}

	if (showgui) {
		r = reader->ctx->dlg.pGuiDisplayMsg(reader->name, pintype, operation, msg, &handle);
		if (r != 0) {
			sc_error(reader->ctx, "pteidgui_display_msg() returns %d\n", r);
			r = SC_ERROR_INTERNAL;
		}
	}
	else
		r = 0;

	if (r == 0) {
		r = pinpad_transmit(reader, slot, sendbuf, sendsize, recvbuf, recvsize, control,
			pintype, operation, pp_info);

		// If we have to do a second command (e.g. for the IAS card, see ias_pin_cmd())
		if ((r >= 0) && (pin_data != NULL) && (pin_data->flags & SC_PIN_CMD_DOUBLE)) {
			r = pinpad_check_result(recvbuf, *recvsize, pin_data->apdu);
			if (r < 0)
				sc_debug(reader->ctx, "First pinpad cmd failed: %d\n", r);
			r = pinpad_transmit_2nd_cmd(reader, slot,
				recvbuf, recvsize, pintype, pp_info, pin_data);
		}

		if (showgui) {
			/* Send the command directly via SCardControl() */
			reader->ctx->dlg.pGuiCloseMsg(handle);
		}
	}

	return r;	
}


/* Local definitions */
#define SC_CCID_PIN_TIMEOUT        30

/* CCID definitions */
#define SC_CCID_PIN_ENCODING_BIN   0x00
#define SC_CCID_PIN_ENCODING_BCD   0x01
#define SC_CCID_PIN_ENCODING_ASCII 0x02

#define SC_CCID_PIN_UNITS_BYTES    0x80


/* Build a pin verification block + APDU */
static int part10_build_verify_pin_block(u8 * buf, size_t * size, struct sc_pin_cmd_data *data)
{
	int offset = 0, count = 0;
	sc_apdu_t *apdu = data->apdu;
	u8 tmp;
	unsigned int tmp16;
	PIN_VERIFY_STRUCTURE *pin_verify  = (PIN_VERIFY_STRUCTURE *)buf; 
	
	/* PIN verification control message */
	pin_verify->bTimerOut = SC_CCID_PIN_TIMEOUT;
	pin_verify->bTimerOut2 = SC_CCID_PIN_TIMEOUT;
	
	/* bmFormatString */
	tmp = 0x80;
	if (data->pin1.encoding == SC_PIN_ENCODING_ASCII) {
		tmp |= SC_CCID_PIN_ENCODING_ASCII;

		/* if the effective pin length offset is specified, use it */
		if (data->pin1.length_offset > 4) {
			tmp |= SC_CCID_PIN_UNITS_BYTES;
			tmp |= (data->pin1.length_offset - 5) << 3;
		}
	} else if (data->pin1.encoding == SC_PIN_ENCODING_BCD) {
		tmp |= SC_CCID_PIN_ENCODING_BCD;
		tmp |= SC_CCID_PIN_UNITS_BYTES;
	} else if (data->pin1.encoding == SC_PIN_ENCODING_GLP) {
		/* see comment about GLP pins in sec.c */
		tmp |= SC_CCID_PIN_ENCODING_BCD;
		tmp |= 0x04 << 3;
	} else
		return SC_ERROR_NOT_SUPPORTED;

	pin_verify->bmFormatString = tmp;

	/* bmPINBlockString */
	tmp = 0x00;
	if (data->pin1.encoding == SC_PIN_ENCODING_GLP) {
		/* GLP pin length is encoded in 4 bits and block size is always 8 bytes */
		tmp |= 0x40 | 0x08;
	} else if (data->pin1.encoding == SC_PIN_ENCODING_ASCII && data->pin1.pad_length) {
		tmp |= data->pin1.pad_length;
	}
	pin_verify->bmPINBlockString = tmp;

	/* bmPINLengthFormat */
	tmp = 0x00;
	if (data->pin1.encoding == SC_PIN_ENCODING_GLP) {
		/* GLP pins expect the effective pin length from bit 4 */
		tmp |= 0x04;
	}
	pin_verify->bmPINLengthFormat = tmp;	/* bmPINLengthFormat */

	if (!data->pin1.min_length || !data->pin1.max_length)
		return SC_ERROR_INVALID_ARGUMENTS;

	tmp16 = (data->pin1.min_length << 8 ) + data->pin1.max_length;
	pin_verify->wPINMaxExtraDigit = HOST_TO_CCID_16(tmp16); /* Min Max */
	
	pin_verify->bEntryValidationCondition = 0x02; /* Keypress only */
	
	/* Ignore language and T=1 parameters. */
	pin_verify->bNumberMessage = 0x01;
	pin_verify->wLangId = HOST_TO_CCID_16(g_language);
	pin_verify->bMsgIndex = 0x00;
	pin_verify->bTeoPrologue[0] = 0x00;
	pin_verify->bTeoPrologue[1] = 0x00;
	pin_verify->bTeoPrologue[2] = 0x00;
	                
	/* APDU itself */
	pin_verify->abData[offset++] = apdu->cla;
	pin_verify->abData[offset++] = apdu->ins;
	pin_verify->abData[offset++] = apdu->p1;
	pin_verify->abData[offset++] = apdu->p2;

	/* Copy data if not Case 1 */
	if (apdu->lc != 0) {
		pin_verify->abData[offset++] = apdu->lc;
		memcpy(&pin_verify->abData[offset], apdu->data, apdu->datalen);
		offset += apdu->datalen;
	}

	pin_verify->ulDataLength = HOST_TO_CCID_32(offset); /* APDU size */
	
	count = sizeof(PIN_VERIFY_STRUCTURE) + offset -1;
	*size = count;
	return SC_SUCCESS;
}



/* Build a pin modification block + APDU */
static int part10_build_modifyunblock_pin_block(u8 * buf, size_t * size, struct sc_pin_cmd_data *data)
{
	int offset = 0, count = 0;
	sc_apdu_t *apdu = data->apdu;
	u8 tmp;
	unsigned int tmp16;
	PIN_MODIFY_STRUCTURE *pin_modify  = (PIN_MODIFY_STRUCTURE *)buf;

	/* PIN verification control message */
	pin_modify->bTimerOut = SC_CCID_PIN_TIMEOUT;	/* bTimeOut */
	pin_modify->bTimerOut2 = SC_CCID_PIN_TIMEOUT;	/* bTimeOut2 */

	/* bmFormatString */
	tmp = 0x00;
	if (data->pin1.encoding == SC_PIN_ENCODING_ASCII) {
		tmp |= SC_CCID_PIN_ENCODING_ASCII;

		/* if the effective pin length offset is specified, use it */
		if (data->pin1.length_offset > 4) {
			tmp |= SC_CCID_PIN_UNITS_BYTES;
			tmp |= (data->pin1.length_offset - 5) << 3;
		}
	} else if (data->pin1.encoding == SC_PIN_ENCODING_BCD) {
		tmp |= SC_CCID_PIN_ENCODING_BCD;
		tmp |= SC_CCID_PIN_UNITS_BYTES;
	} else if (data->pin1.encoding == SC_PIN_ENCODING_GLP) {
		/* see comment about GLP pins in sec.c */
		tmp |= SC_CCID_PIN_ENCODING_BCD;
		tmp |= 0x04 << 3;
	} else
		return SC_ERROR_NOT_SUPPORTED;

	pin_modify->bmFormatString = tmp;	/* bmFormatString */

	/* bmPINBlockString */
	tmp = 0x00;
	if (data->pin1.encoding == SC_PIN_ENCODING_GLP) {
		/* GLP pin length is encoded in 4 bits and block size is always 8 bytes */
		tmp |= 0x40 | 0x08;
	} else if (data->pin1.encoding == SC_PIN_ENCODING_ASCII && data->pin1.pad_length) {
		tmp |= data->pin1.pad_length;
	}
	pin_modify->bmPINBlockString = tmp; /* bmPINBlockString */

	/* bmPINLengthFormat */
	tmp = 0x00;
	if (data->pin1.encoding == SC_PIN_ENCODING_GLP) {
		/* GLP pins expect the effective pin length from bit 4 */
		tmp |= 0x04;
	}
	pin_modify->bmPINLengthFormat = tmp;	/* bmPINLengthFormat */

	pin_modify->bInsertionOffsetOld = 0x00;  /* bOffsetOld */
	if (data->flags & SC_PIN_CMD_2ND) {
		pin_modify->bInsertionOffsetNew = 0;
		pin_modify->bConfirmPIN = 0x01; /* Ask confirmation but not the current PIN */
		pin_modify->bNumberMessage = 0x02;
	}
	else if ((data->cmd == SC_PIN_CMD_CHANGE) || (data->flags & SC_PIN_CMD_NEW_PIN)) {
		pin_modify->bInsertionOffsetNew = data->pin1.max_length;  /* bOffsetNew */
		pin_modify->bConfirmPIN = 0x03;	/* PUK/old PIN + confirm new PIN */
		pin_modify->bNumberMessage = 0x03;
	}
	else {
		pin_modify->bInsertionOffsetNew = 0;
		pin_modify->bConfirmPIN = 0x00; /* Reader thinks you will only enter your PIN */
		pin_modify->bNumberMessage = 0x01;
	}

	if (!data->pin1.min_length || !data->pin1.max_length)
		return SC_ERROR_INVALID_ARGUMENTS;
		
	tmp16 = (data->pin1.min_length << 8 ) + data->pin1.max_length;
	pin_modify->wPINMaxExtraDigit = HOST_TO_CCID_16(tmp16); /* Min Max */

	pin_modify->bEntryValidationCondition = 0x02;	/* bEntryValidationCondition, keypress only */

	pin_modify->wLangId = HOST_TO_CCID_16(g_language);
	pin_modify->bMsgIndex1 = 0x00;
	pin_modify->bMsgIndex2 = 0x00;
	pin_modify->bMsgIndex3 = 0x00;
	pin_modify->bTeoPrologue[0] = 0x00;
	pin_modify->bTeoPrologue[1] = 0x00;
	pin_modify->bTeoPrologue[2] = 0x00;
	                
	/* APDU itself */
	pin_modify->abData[offset++] = apdu->cla;
	pin_modify->abData[offset++] = apdu->ins;
	pin_modify->abData[offset++] = apdu->p1;

	pin_modify->abData[offset++] = apdu->p2;

	/* Copy data if not Case 1 */
	if (apdu->lc != 0) {
		pin_modify->abData[offset++] = apdu->lc;
		memcpy(&pin_modify->abData[offset], apdu->data, apdu->datalen);
		offset += apdu->datalen;
	}

	pin_modify->ulDataLength = HOST_TO_CCID_32(offset); /* APDU size */
	
	count = sizeof(PIN_MODIFY_STRUCTURE) + offset -1;
	*size = count;
	return SC_SUCCESS;
}

#endif
/* Do the PIN command */
static int part10_pin_cmd(sc_reader_t *reader, sc_slot_info_t *slot,
	     struct sc_pin_cmd_data *data)
{
#ifdef PINPAD_ENABLED
	u8 rbuf[SC_MAX_APDU_BUFFER_SIZE], sbuf[SC_MAX_APDU_BUFFER_SIZE];
	char dbuf[SC_MAX_APDU_BUFFER_SIZE * 3];
	size_t rcount = sizeof(rbuf), scount = 0;
	int r;
	DWORD ioctl = 0;
	sc_apdu_t *apdu;
	struct pcsc_slot_data *pslot = (struct pcsc_slot_data *) slot->drv_data;

	SC_FUNC_CALLED(reader->ctx, 3);
	assert(pslot != NULL);

	/* The APDU must be provided by the card driver */
	if (!data->apdu) {
		sc_error(reader->ctx, "No APDU provided for Part 10 pinpad verification!");
		return SC_ERROR_NOT_SUPPORTED;
	}

	apdu = data->apdu;

	switch (data->cmd) {
	case SC_PIN_CMD_VERIFY:
		if (!(pslot->verify_ioctl || (pslot->verify_ioctl_start && pslot->verify_ioctl_finish))) {
			sc_error(reader->ctx, "Pinpad reader does not support verification!");
			return SC_ERROR_NOT_SUPPORTED;
		}
		r = part10_build_verify_pin_block(sbuf, &scount, data);
		ioctl = pslot->verify_ioctl ? pslot->verify_ioctl : pslot->verify_ioctl_start;
		break;
	case SC_PIN_CMD_CHANGE:
	case SC_PIN_CMD_UNBLOCK:
		if (!(pslot->modify_ioctl || (pslot->modify_ioctl_start && pslot->modify_ioctl_finish))) {
			sc_error(reader->ctx, "Pinpad reader does not support modification!");
			return SC_ERROR_NOT_SUPPORTED;
		}
		r = part10_build_modifyunblock_pin_block(sbuf, &scount, data);
		ioctl = pslot->modify_ioctl ? pslot->modify_ioctl : pslot->modify_ioctl_start;
		break;
	default:
		sc_error(reader->ctx, "Unknown PIN command %d", data->cmd);
		return SC_ERROR_NOT_SUPPORTED;
	}

	/* If PIN block building failed, we fail too */
	SC_TEST_RET(reader->ctx, r, "Part10 PIN block building failed!");

	r = pinpad_transmit_gui(reader, slot, sbuf, scount, rbuf, &rcount, ioctl, data);
	SC_TEST_RET(reader->ctx, r, "Part 10: block transmit failed!");

	// finish the call if it was a two-phase operation
	if ((ioctl == pslot->verify_ioctl_start)
	    || (ioctl == pslot->modify_ioctl_start)) {
		if (rcount != 0) {
			SC_FUNC_RETURN(reader->ctx, 2, SC_ERROR_UNKNOWN_DATA_RECEIVED);
		}
		ioctl = (ioctl == pslot->verify_ioctl_start) ? pslot->verify_ioctl_finish : pslot->modify_ioctl_finish;

		rcount = sizeof(rbuf);
		r = pinpad_transmit_gui(reader, slot, sbuf, 0, rbuf, &rcount, ioctl, data);
		SC_TEST_RET(reader->ctx, r, "Part 10: finish operation failed!");
	}

	r = pinpad_check_result(rbuf, rcount, apdu);
	SC_TEST_RET(reader->ctx, r, "PIN command failed");

	/* PIN command completed, all is good */
	return SC_SUCCESS;
#else
	return SC_ERROR_NOT_SUPPORTED;
#endif /* PINPAD_ENABLED */
}

int sc_pinpad_cmd(sc_reader_t *reader, sc_slot_info_t *slot,
	struct sc_pin_cmd_data *data)
{
	/* (For now) only CCID pinpad support */
	return part10_pin_cmd(reader, slot, data);
}

#endif   /* HAVE_PCSC */
