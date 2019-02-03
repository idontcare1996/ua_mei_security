/*
 * This API should be implemented by libraries (also called
 * plugins) that want to support a pinpad reader for use
 * with the Portugese eID middleware.
 *
 * The middleware has support for CCID readers; see the PCSC V2
 * part 10 standard and the USB CCID specifications.
 *
 * However these standards don't specify how to display messages
 * on the reader's display. Because the middleware will show a
 * some explanation on the PC whenever a pinpad operation needs
 * to be done, this restriction is not important for pinpad
 * readers without display.
 *
 * But for readers with display, it is important that the correct
 * message be displayed, as to avoid confusion between the different
 * PINs on the eID card and the fact that the "PIN change command"
 * is used for both PIN change and PIN unblock.
 *
 * In this case, as well as for non-CCID pinpad readers or readers
 * that offer extra functionality, a library (a DLL on Windows, an
 * .so file on Linux and a .dylib file on Mac) can be made that
 * exports the 2 functions below.
 * The middleware will first try ot load this library and check if
 * it can be used by mean of the PTEIDPP_Init() function. If so, then
 * all pinpad functionality will be requested  through this library.
 *
 * To allow the middleware to find the library, it must be placed in
 * the following directory:
 *   - On Linux and Mac OS X: /usr/local/lib/pteidpp
 *   - On Windows: %windir%\System32\pteidpp
 *     (%windir% is the Windows installation folder, usually this
 *      C:\WINDOWS on WinXP and C:\WINNT on Win2000)
 * And the name of the library should start with "pteidpp1" on
 * Windows, and with "libpteidpp1" on Linux and Mac.
 *
 * Future, incompatible versions of this library will start with
 * "pteidpp2" etc.
 */

#ifndef __PTEIDPINPAD_H__
#define __PTEIDPINPAD_H__

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(WIN32)
#ifdef PTEIDPP_IMPORT
#define PTEIDPP_API __declspec(dllimport)
#else
#define PTEIDPP_API __declspec(dllexport)
#endif
#else
#define PTEIDPP_API
#endif

#define PTEID_MINOR_VERSION       0

#define PTEIDPP_TYPE_AUTH         0x00    /* The Authentication PIN or PUK */
#define PTEIDPP_TYPE_SIGN         0x01    /* The Signature PIN or PUK */
#define PTEIDPP_TYPE_ADDR         0x02    /* The Address PIN or PUK */
#define PTEIDPP_TYPE_ACTIV        0x03    /* The Activation PIN */

#define PTEIDPP_OP_VERIFY                    0x01    /* PIN verification */
#define PTEIDPP_OP_CHANGE                    0x02    /* PIN change */
#define PTEIDPP_OP_UNBLOCK_NO_CHANGE         0x03    /* PIN unblock without PIN change */
#define PTEIDPP_OP_UNBLOCK_CHANGE            0x04    /* PIn unblock plus PIN change */
#define PTEIDPP_OP_UNBLOCK_MERGE_NO_CHANGE   0x05    /* PIN unblock using PUK merge, without PIN change*/
#define PTEIDPP_OP_UNBLOCK_MERGE_CHANGE      0x06    /* PIN unblock using PUK merge, plus PIN change */

#define DLG_INFO_CHARS       2000

#define PP_LANG_PT   0x0816
#define PP_LANG_EN   0x0409

/**
 * This allows libraries to display their own messages in the dialogs that
 * appear before each pinpad operation, in order to instruct users what to do.
 * If a message is left to "" then the default message will be used.
 * If a message is set to "r" then the middleware won't show a dialog,
 * this allows libraries to implement their own dialogs.
 * The messages should contain plain text or html code and can be at most
 * 2000 (DLG_INFO_CHARS) bytes in length; this memory has already been allocated.
 * LIMITATION: double quote signs " or \" are not allowed inside the strings!
 */
typedef struct {
	char *csVerifyInfo;
	char *csChangeInfo;
	char *csUnblockNochangeInfo;
	char *csUnblockChangeInfo;
	char *csUnblockMergeNoChangeInfo;
	char *csUnblockMergeChangeInfo;
	char *csRfu1;                      /* NULL */
	char *csRfu2;                      /* NULL */
	char *csRfu3;                      /* NULL */
	char *csRfu4;                      /* NULL */
} tGuiInfo;

/**
 * This function is called after loading the pinpad library, it is used by the middleware
 * to see if the pinpad library supports this reader; and for the pinpad library itself
 * to make any intialisations.
 *
 * - ucMinorVersion: (IN) indicates a change in the API that is compatible with other
 *     ucMinorVersions of this library, current value = PTEID_MINOR_VERSION.
 * - hCtx: (IN) the SCARDCONTEXT handle that was obtained by a call to SCardEstablishContext()
 * - csReader: (IN) the PCSC reader name as obtained by a call to SCardListReaders()
 * - ulLanguage: (IN) USB LANGID code (http://www.usb.org/developers/docs) for the language
 *      of the messages to be displayed on the pinpad reader
 *      Current values are 0x0816 for Portugese, 0x0409 for English
 * - pGuiInfo: (OUT) strings to be displayed on PC before each pinpad operation,
 *      leave them unchanged (NULL) to use the default strings. The size that has
 *      been allocated for each string is 2000 (DLG_INFO_CHARS) bytes.
 * - ulRfu: reserved for future use, set to 0 for this ucMinorVersion
 * - pRfu: reserved for future use, set to NULL for this ucMinorVersion
 *
 * Returns SCARD_ERROR_SUCCESS upon success, or another (preferably PCSC) error if this pinpad
 * library doesn't support this reader or something else went wrong.
 * In the case SCARD_ERROR_SUCCESS is returned, this pinpad library will be used
 * by the middleware for pinpad operation. Otherwise, the middleware will continue
 * searching for other pinpad libraries that may support this reader.
 */
PTEIDPP_API long PTEIDPP_Init(
	unsigned char ucMinorVersion,
	SCARDCONTEXT 	hCtx, const char *csReader,
	unsigned long ulLanguage,
	tGuiInfo *pGuiInfo,
	unsigned long ulRfu, void *pRfu);

/**
 * This function is called for a "Get Feature Request" with control = CM_IOCTL_GET_FEATURE_REQUEST
 * (right after a successfull call to PTEIDPP_Init())
 * and when a pinpad operation (verify, change, unblock) is needed.
 *
 * The following ioctl codes are recognized and used by the middleware
 *  - For PIN verification:
 *       FEATURE_VERIFY_PIN_DIRECT
 *       FEATURE_VERIFY_PIN_START and FEATURE_VERIFY_PIN_FINISH
 *  - For PIN change and unblock:
 *       FEATURE_MODIFY_PIN_DIRECT
 *       FEATURE_MODIFY_PIN_START and FEATURE_MODIFY_PIN_FINISH
 *
 * - The first 7 parameters are identical to the ones given in an SCardControl()
 *     command, as specified in part 10 of the PCSC standard.
 *   In case of a standard CCID pinpad reader without display, this function can
 *     directly 'forward' these parameters to an SCardControl() function.
 * - ucPintype: one of PTEIDPP_TYPE_AUTH, ..., PTEIDPP_TYPE_ACTIV; is ignored for
 *     a "Get Feature Request"
 * - ucOperation: one of PTEIDPP_OP_VERIFY, ..., PTEIDPP_OP_UNBLOCK_MERGE; is
 *     ignored for a "Get Feature Request"
 * - ulRfu: reserved for future use, set to 0 for this ucMinorVersion
 * - pRfu: reserved for future use, set to NULL for this ucMinorVersion
 *
 * Returns SC_ERROR_SUCCESS upon success, or another (preferably) PCSC error code otherwise.
 */
PTEIDPP_API long PTEIDPP_Command(
	SCARDHANDLE hCard, int ioctl,
	const unsigned char *pucSendbuf, DWORD dwSendlen,
	unsigned char *pucRecvbuf, DWORD dwRrecvlen, DWORD *pdwRrecvlen,
	unsigned char ucPintype, unsigned char ucOperation,
	unsigned long ulRfu, void *pRfu);

#ifdef  __cplusplus
}
#endif

#endif
