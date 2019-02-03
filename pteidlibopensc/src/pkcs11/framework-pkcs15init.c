/*
 * framework-pkcs15.c: PKCS#15 framework and related objects
 *
 * Copyright (C) 2002  Timo Ter�s <timo.teras@iki.fi>
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

#include <stdlib.h>
#include <string.h>
#include "sc-pkcs11.h"
#ifdef USE_PKCS15_INIT
#include "opensc/pkcs15-init.h"

/*
 * Deal with uninitialized cards
 */
static CK_RV pkcs15init_bind(struct sc_pkcs11_card *p11card)
{
	struct sc_card	*card = p11card->card;
	struct sc_profile *profile;
	int		rc;

	card->ctx->log_errors = 0;
	rc = sc_pkcs15init_bind(card, "pkcs15", NULL, &profile);
	card->ctx->log_errors = 1;
	if (rc == 0)
		p11card->fw_data = profile;
	return sc_to_cryptoki_error(rc, p11card->reader);
}

static CK_RV pkcs15init_unbind(struct sc_pkcs11_card *p11card)
{
	struct sc_profile *profile;

	profile = (struct sc_profile *) p11card->fw_data;
	sc_pkcs15init_unbind(profile);
	return CKR_OK;
}

static CK_RV pkcs15init_create_tokens(struct sc_pkcs11_card *p11card)
{
	struct sc_profile	*profile;
	struct sc_pkcs11_slot	*slot;

	profile = (struct sc_profile *) p11card->fw_data;
	while (slot_allocate(&slot, p11card) == CKR_OK) {
		CK_TOKEN_INFO_PTR pToken = &slot->token_info;
		const char	*string;

		if (sc_pkcs11_conf.hide_empty_tokens)
			continue;

		slot->slot_info.flags |= CKF_TOKEN_PRESENT;

		strcpy_bp(pToken->model, "PKCS #15 SCard", 16);
		sc_pkcs15init_get_manufacturer(profile, &string);
		if (!string)
			string = "Unknown";
		strcpy_bp(pToken->manufacturerID, string, 32);
		sc_pkcs15init_get_serial(profile, &string);
		if (!string)
			string = "";
		strcpy_bp(pToken->serialNumber, string, 16);
		pToken->ulMaxSessionCount = CK_EFFECTIVELY_INFINITE;
		pToken->ulSessionCount = 0; /* FIXME */
		pToken->ulMaxRwSessionCount = CK_EFFECTIVELY_INFINITE;
		pToken->ulRwSessionCount = 0; /* FIXME */
		pToken->ulTotalPublicMemory = CK_UNAVAILABLE_INFORMATION;
		pToken->ulFreePublicMemory = CK_UNAVAILABLE_INFORMATION;
		pToken->ulTotalPrivateMemory = CK_UNAVAILABLE_INFORMATION;
		pToken->ulFreePrivateMemory = CK_UNAVAILABLE_INFORMATION;
		pToken->hardwareVersion.major = 1;
		pToken->hardwareVersion.minor = 0;
		pToken->firmwareVersion.major = 1;
		pToken->firmwareVersion.minor = 0;
	}

	return CKR_OK;
}

static CK_RV
pkcs15init_release_token(struct sc_pkcs11_card *p11card, void *ptr)
{
	return CKR_OK;
}

static CK_RV
pkcs15init_login(struct sc_pkcs11_card *p11card, void *ptr,
		CK_USER_TYPE user, CK_CHAR_PTR pin, CK_ULONG pinLength)
{
	return CKR_CRYPTOKI_NOT_INITIALIZED;
}

static CK_RV
pkcs15init_logout(struct sc_pkcs11_card *p11card, void *ptr)
{
	return CKR_CRYPTOKI_NOT_INITIALIZED;
}

static CK_RV
pkcs15init_change_pin(struct sc_pkcs11_card *p11card, void *ptr,
			CK_CHAR_PTR oldPin, CK_ULONG oldPinLength,
			CK_CHAR_PTR newPin, CK_ULONG newPinLength)
{
	return CKR_CRYPTOKI_NOT_INITIALIZED;
}

static CK_RV
pkcs15init_initialize(struct sc_pkcs11_card *p11card, void *ptr,
		CK_UTF8CHAR_PTR pPin, CK_ULONG ulPinLen,
		CK_UTF8CHAR_PTR pLabel)
{
	struct sc_profile *profile = (struct sc_profile *) p11card->fw_data;
	struct sc_pkcs15init_initargs args;
	struct sc_pkcs11_slot *slot;
	int		rc, rv, id;

	memset(&args, 0, sizeof(args));
	args.so_pin = pPin;
	args.so_pin_len = ulPinLen;
	args.so_puk = pPin;
	args.so_puk_len = ulPinLen;
	args.label = (const char *) pLabel;
	rc = sc_pkcs15init_add_app(p11card->card, profile, &args);
	if (rc < 0)
		return sc_to_cryptoki_error(rc, p11card->reader);

	/* Change the binding from the pkcs15init framework
	 * to the pkcs15 framework on the fly.
	 * First, try to bind pkcs15 framework */
	if ((rv = framework_pkcs15.bind(p11card)) != CKR_OK) {
		/* whoops, bad */
		p11card->fw_data = profile;
		return rv;
	}

	/* Change the function vector to the standard pkcs15 ops */
	p11card->framework = &framework_pkcs15;

	/* Loop over all slots belonging to this card, and fix up
	 * the flags.
	 */
	for (id = 0; slot_get_slot(id, &slot) == CKR_OK; id++) {
		if (slot->card == p11card)
			slot->token_info.flags |= CKF_TOKEN_INITIALIZED;
		if (slot->card->card->caps & SC_CARD_CAP_RNG)
			slot->token_info.flags |= CKF_RNG;
	}

	sc_pkcs15init_unbind(profile);
	return CKR_OK;
}

struct sc_pkcs11_framework_ops framework_pkcs15init = {
	pkcs15init_bind,
	pkcs15init_unbind,
	pkcs15init_create_tokens,
	pkcs15init_release_token,
	pkcs15init_login,
	pkcs15init_logout,
	pkcs15init_change_pin,
	pkcs15init_initialize,
};

#else /* ifdef USE_PKCS15_INIT */

struct sc_pkcs11_framework_ops framework_pkcs15init = {
	NULL,
	NULL,
};

#endif
