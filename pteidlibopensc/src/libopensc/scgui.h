#ifndef SCGUI_H
#define SCGUI_H

#ifdef SCGUI_DLL

#ifdef SCGUI_EXPORTS
#define SCGUI_API __declspec(dllexport)
#else
#define SCGUI_API __declspec(dllimport)
#endif

#else
#define SCGUI_API
#endif

typedef int (*TimerFunc)(void);

typedef enum {
	SCGUI_OK,
	SCGUI_CANCEL,
	SCGUI_ERROR,
	SCGUI_NOT_INITED,
	SCGUI_BUFFER_TOO_SMALL,
	SCGUI_BAD_PARAM
} scgui_ret_t;

typedef enum {
	SCGUI_AUTH_ICON,
	SCGUI_SIGN_ICON,
    SCGUI_ADDR_ICON,
    SCGUI_ACTIV_ICON,
	SCGUI_SIGN_ACTIV_ICON,
	SCGUI_SIGN_NOALERT_ICON,
} scgui_param_t;


#define PTEIDPP_OP_VERIFY                    0x01    /* PIN verification */
#define PTEIDPP_OP_CHANGE                    0x02    /* PIN change */
#define PTEIDPP_OP_UNBLOCK_NO_CHANGE         0x03    /* PIN unblock without PIN change */
#define PTEIDPP_OP_UNBLOCK_CHANGE            0x04    /* PIn unblock plus PIN change */
#define PTEIDPP_OP_UNBLOCK_MERGE_NO_CHANGE   0x05    /* PIN unblock using PUK merge, without PIN change*/
#define PTEIDPP_OP_UNBLOCK_MERGE_CHANGE      0x06    /* PIN unblock using PUK merge, plus PIN change */

#define PTEID_PIN_MIN_LENGTH 4
#define PTEID_PIN_MAX_LENGTH 8

typedef struct 
{
	char pin[PTEID_PIN_MAX_LENGTH+1];
	char *msg;
	char *btn_ok;
	char *btn_cancel;
	char *title;
	char *pinTooShort;
	scgui_param_t iconSign;
} VerifyPinData;

typedef struct 
{
	char oldpin[PTEID_PIN_MAX_LENGTH+1];
	char newpin[PTEID_PIN_MAX_LENGTH+1];
	char confirmpin[PTEID_PIN_MAX_LENGTH+1];
	char *msg;
	char *btn_ok;
	char *btn_cancel;
	char *title;
	char *pinTooShort;
	char *confirmPinError;
} ChangePinData;

typedef struct 
{
	char puk[PTEID_PIN_MAX_LENGTH+1];
	char newpin[PTEID_PIN_MAX_LENGTH+1];
	char confirmpin[PTEID_PIN_MAX_LENGTH+1];
	char *msg;
	char *btn_ok;
	char *btn_cancel;
	char *title;
	char *pinTooShort;
	char *confirmPinError;
} UnblockPinData;

typedef struct 
{
	char *short_msg;
	char *long_msg;
	scgui_ret_t ret;
	char *btn_ok;
	char *btn_cancel;
	char *title;
	char *pinpad_reader;
    int cancelbtn;
    int messageid;
    int param;
} AskMessageData;

typedef struct 
{
	char *short_msg;
	char *long_msg;
	scgui_param_t iconSign;
	char *btn_cancel;
	char *title;
	char *pinpad_reader;
} DisplayMessageData;

typedef struct 
{
	char *msg;
	scgui_ret_t ret;
	char *btn_ok;
	char *btn_cancel;
	char *title;
    TimerFunc timer_func;
} InsertCardData;

#ifdef __cplusplus
extern "C"
{
#endif

typedef scgui_ret_t (*PTEIDGUI_INIT)(void);
typedef scgui_ret_t (*PTEIDGUI_ENTERPIN)(char *, int *, scgui_param_t);
typedef scgui_ret_t (*PTEIDGUI_CHANGEPIN)(char *, int *, char *, int *);
typedef scgui_ret_t (*PTEIDGUI_ASK_MESSAGE)(int, int, int, const char *);
typedef scgui_ret_t (*PTEIDGUI_INSERTCARD)(TimerFunc);
typedef scgui_ret_t (*PTEIDGUI_UNBLOCKPIN)(char *, int *, char *, int *);
typedef scgui_ret_t (*PTEIDGUI_GET_LANG)(int *);
typedef scgui_ret_t (*PTEIDGUI_DISPLAY_MSG)(const char *, scgui_param_t, int, const char *, unsigned long*);
typedef scgui_ret_t (*PTEIDGUI_CLOSE_MSG)(unsigned long);

/* New API */
SCGUI_API scgui_ret_t pteidgui_init(void);
SCGUI_API scgui_ret_t pteidgui_enterpin(char *pin, int *len, scgui_param_t signIcon);
SCGUI_API scgui_ret_t pteidgui_changepin(char *oldpin, int *oldpinlen, char *newpin, int *newpinlen);
SCGUI_API scgui_ret_t pteidgui_ask_message(int messageID, int cancelButton, int triesleft, const char *pinpad_reader);
SCGUI_API scgui_ret_t pteidgui_insertcard(TimerFunc pFunc);
SCGUI_API scgui_ret_t pteidgui_unblockpin(char *puk, int *puklen, char *newpin, int *newpinlen);

/* Return: *langcode = 0x0816 for Portugese, 0x0409 for English */
SCGUI_API scgui_ret_t pteidgui_get_lang(unsigned *langcode);

SCGUI_API scgui_ret_t pteidgui_display_msg(const char *reader, scgui_param_t icon, int operation,
	const char *mesg, unsigned long *handle);

SCGUI_API scgui_ret_t pteidgui_close_msg(unsigned long handle);


#ifdef __cplusplus
}
#endif

#endif
