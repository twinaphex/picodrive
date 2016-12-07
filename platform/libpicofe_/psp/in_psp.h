
struct in_default_bind;

void in_PSP_init(const struct in_default_bind *defbinds);

enum  { PSP_BTN_UP = 4,      PSP_BTN_LEFT = 7,      PSP_BTN_DOWN = 6,	PSP_BTN_RIGHT = 5,
        PSP_BTN_START = 3,   PSP_BTN_SELECT = 0,    PSP_BTN_L = 8,		PSP_BTN_R = 9,
        PSP_BTN_X = 14,      PSP_BTN_CIRCLE = 13,   PSP_BTN_SQUARE = 15,PSP_BTN_TRIANGLE = 12,
        PSP_BTN_VOL_UP = 20, PSP_BTN_VOL_DOWN = 21, PSP_BTN_HOME = 16,	PSP_BTN_NUB_UP = 28,
        PSP_BTN_NUB_RIGHT = 29, PSP_BTN_NUB_DOWN = 30, PSP_BTN_NUB_LEFT = 27 };

#define ANALOG_DEADZONE 80

/* FIXME */
#ifndef PSP_DEV
extern int PSP_dev_id;
#define PSP_DEV 1
#endif
