/*
 * (C) Gražvydas "notaz" Ignotas, 2006-2012
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 *  - MAME license.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pspctrl.h>

#include "../input.h"
//#include "soc.h"
#include "../../common/emu.h"
#include "in_psp.h"

#define IN_PSP_PREFIX "psp:"
#define IN_PSP_NBUTTONS 32

/* note: in_psp handles combos (if 2 btns have the same bind,
 * both must be pressed for action to happen) */
static int in_psp_combo_keys = 0;
static int in_psp_combo_acts = 0;

static int (*in_psp_get_bits)(void);

static const char *in_psp_keys[IN_PSP_NBUTTONS] = {
	[0 ... IN_PSP_NBUTTONS-1] = NULL,
	[PSP_BTN_UP]    = "Up",    [PSP_BTN_LEFT]   = "Left",
	[PSP_BTN_DOWN]  = "Down",  [PSP_BTN_RIGHT]  = "Right",
	[PSP_BTN_START] = "Start", [PSP_BTN_SELECT] = "Select",
	[PSP_BTN_L]     = "L",     [PSP_BTN_R]      = "R",
	[PSP_BTN_X]     = "X",     [PSP_BTN_CIRCLE] = "Circle",
	[PSP_BTN_SQUARE]   = "Square",     [PSP_BTN_TRIANGLE]   = "Triangle",
	[PSP_BTN_NUB_UP]    = "Analog Up",    [PSP_BTN_NUB_LEFT]   = "Analog Left",
	[PSP_BTN_NUB_DOWN]  = "Analog Down",  [PSP_BTN_NUB_RIGHT]  = "Analog Right",
	[PSP_BTN_VOL_DOWN] = "VOL DOWN",
	[PSP_BTN_VOL_UP]   = "VOL UP",
	[PSP_BTN_HOME]     = "Home"
};

#if 0
unsigned int mapPSPbuttonsToPicoDrive( unsigned int buttons ) {
	unsigned int pico_buttons;

	pico_buttons = 0;

	if( buttons & PSP_CTRL_UP ) {
		pico_buttons |= PBTN_UP;
	}

	if( buttons & PSP_CTRL_DOWN ) {
		pico_buttons |= PBTN_DOWN;
	}

	if( buttons & PSP_CTRL_LEFT ) {
		pico_buttons |= PBTN_LEFT;
	}

	if( buttons & PSP_CTRL_RIGHT ) {
		pico_buttons |= PBTN_RIGHT;
	}

	if( buttons & PSP_CTRL_CROSS ) {
		pico_buttons |= PBTN_MBACK;
	}

	if( buttons & PSP_CTRL_CIRCLE ) {
		pico_buttons |= PBTN_MOK;
	}

	if( buttons & PSP_CTRL_SQUARE ) {
		pico_buttons |= PBTN_MA2;
	}

	if( buttons & PSP_CTRL_TRIANGLE ) {
		pico_buttons |= PBTN_MA3;
	}

	if( buttons & PSP_CTRL_LTRIGGER ) {
		pico_buttons |= PBTN_L;
	}

	if( buttons & PSP_CTRL_RTRIGGER ) {
		pico_buttons |= PBTN_R;
	}

	if( buttons & PSP_CTRL_START ) {
		pico_buttons |= PBTN_CHAR;
	}

	if( buttons & PSP_CTRL_SELECT ) {
		pico_buttons |= PBTN_MENU;
	}

	return pico_buttons;
}
#endif

static int in_psp_get_bits_(void)
{
	unsigned int buttons;
	SceCtrlData pad;
	sceCtrlPeekBufferPositive(&pad, 1);
	buttons = pad.Buttons;
#if 0
	buttons = mapPSPbuttonsToPicoDrive( buttons );
#endif

	// analog..
	if( currentConfig.EmuOpt & EOPT_EN_ANALOG) {
		if (pad.Lx < 128 - ANALOG_DEADZONE) buttons |= (1<<PSP_BTN_NUB_LEFT);
		if (pad.Lx > 128 + ANALOG_DEADZONE) buttons |= (1<<PSP_BTN_NUB_RIGHT);
		if (pad.Ly < 128 - ANALOG_DEADZONE) buttons |= (1<<PSP_BTN_NUB_UP);
		if (pad.Ly > 128 + ANALOG_DEADZONE) buttons |= (1<<PSP_BTN_NUB_DOWN);
	}

	return buttons;
}

static void in_psp_probe(const in_drv_t *drv)
{
	in_psp_get_bits = in_psp_get_bits_;

	in_register(IN_PSP_PREFIX "PSP pad", -1, NULL,
		IN_PSP_NBUTTONS, in_psp_keys, 1);
}

static void in_psp_free(void *drv_data)
{
	close(0);
}

static const char * const *
in_psp_get_key_names(const in_drv_t *drv, int *count)
{
	*count = IN_PSP_NBUTTONS;
	return in_psp_keys;
}

/* ORs result with pressed buttons */
static int in_psp_update(void *drv_data, const int *binds, int *result)
{
	int type_start = 0;
	int i, t, keys;

	keys = in_psp_get_bits();

	if (keys & in_psp_combo_keys) {
		result[IN_BINDTYPE_EMU] = in_combos_do(keys, binds, PSP_BTN_HOME,
						in_psp_combo_keys, in_psp_combo_acts);
		type_start = IN_BINDTYPE_PLAYER12;
	}

	for (i = 0; keys; i++, keys >>= 1) {
		if (!(keys & 1))
			continue;

		for (t = type_start; t < IN_BINDTYPE_COUNT; t++)
			result[t] |= binds[IN_BIND_OFFS(i, t)];
	}

	return 0;
}

int in_psp_update_keycode(void *data, int *is_down)
{
	static int old_val = 0;
	int val, diff, i;

	val = in_psp_get_bits();

	diff = val ^ old_val;
	if (diff == 0)
		return -1;

	/* take one bit only */
	for (i = 0; i < sizeof(diff)*8; i++)
		if (diff & (1<<i))
			break;

	old_val ^= 1 << i;

	if (is_down)
		*is_down = !!(val & (1<<i));
	return i;
}

static const struct {
	short key;
	short pbtn;
} key_pbtn_map[] =
{
	{ PSP_BTN_UP,		PBTN_UP },
	{ PSP_BTN_DOWN,		PBTN_DOWN },
	{ PSP_BTN_LEFT,		PBTN_LEFT },
	{ PSP_BTN_RIGHT,	PBTN_RIGHT },
	{ PSP_BTN_NUB_UP,	PBTN_UP },
	{ PSP_BTN_NUB_DOWN,	PBTN_DOWN },
	{ PSP_BTN_NUB_LEFT,	PBTN_LEFT },
	{ PSP_BTN_NUB_RIGHT,PBTN_RIGHT },
	{ PSP_BTN_CIRCLE,	PBTN_MOK },
	{ PSP_BTN_X,		PBTN_MBACK },
	{ PSP_BTN_SQUARE,	PBTN_MA2 },
	{ PSP_BTN_TRIANGLE,	PBTN_MA3 },
	{ PSP_BTN_L,		PBTN_L },
	{ PSP_BTN_R,		PBTN_R },
	{ PSP_BTN_SELECT,	PBTN_MENU },
};

#define KEY_PBTN_MAP_SIZE (sizeof(key_pbtn_map) / sizeof(key_pbtn_map[0]))

static int in_psp_menu_translate(void *drv_data, int keycode, char *charcode)
{
	int i;
	if (keycode < 0)
	{
		/* menu -> kc */
		keycode = -keycode;
		for (i = 0; i < KEY_PBTN_MAP_SIZE; i++)
			if (key_pbtn_map[i].pbtn == keycode)
				return key_pbtn_map[i].key;
	}
	else
	{
		for (i = 0; i < KEY_PBTN_MAP_SIZE; i++)
			if (key_pbtn_map[i].key == keycode )
				return key_pbtn_map[i].pbtn;
	}

	return 0;
}

#if 0 // TODO: move to pico
static const struct {
	short code;
	char btype;
	char bit;
} in_psp_defbinds[] =
{
	/* MXYZ SACB RLDU */
	{ BTN_UP,	IN_BINDTYPE_PLAYER12, 0 },
	{ BTN_DOWN,	IN_BINDTYPE_PLAYER12, 1 },
	{ BTN_LEFT,	IN_BINDTYPE_PLAYER12, 2 },
	{ BTN_RIGHT,	IN_BINDTYPE_PLAYER12, 3 },
	{ BTN_X,	IN_BINDTYPE_PLAYER12, 4 },	/* B */
	{ BTN_B,	IN_BINDTYPE_PLAYER12, 5 },	/* C */
	{ BTN_A,	IN_BINDTYPE_PLAYER12, 6 },	/* A */
	{ BTN_START,	IN_BINDTYPE_PLAYER12, 7 },
	{ BTN_SELECT,	IN_BINDTYPE_EMU, PEVB_MENU },
//	{ BTN_Y,	IN_BINDTYPE_EMU, PEVB_SWITCH_RND },
	{ BTN_L,	IN_BINDTYPE_EMU, PEVB_STATE_SAVE },
	{ BTN_R,	IN_BINDTYPE_EMU, PEVB_STATE_LOAD },
	{ BTN_VOL_UP,	IN_BINDTYPE_EMU, PEVB_VOL_UP },
	{ BTN_VOL_DOWN,	IN_BINDTYPE_EMU, PEVB_VOL_DOWN },
	{ 0, 0, 0 },
};
#endif

/* remove binds of missing keys, count remaining ones */
static int in_psp_clean_binds(void *drv_data, int *binds, int *def_binds)
{
	int i, count = 0;
//	int eb, have_vol = 0, have_menu = 0;

	for (i = 0; i < IN_PSP_NBUTTONS; i++) {
		int t, offs;
		for (t = 0; t < IN_BINDTYPE_COUNT; t++) {
			offs = IN_BIND_OFFS(i, t);
			if (in_psp_keys[i] == NULL)
				binds[offs] = def_binds[offs] = 0;
			if (binds[offs])
				count++;
		}
#if 0
		eb = binds[IN_BIND_OFFS(i, IN_BINDTYPE_EMU)];
		if (eb & (PEV_VOL_DOWN|PEV_VOL_UP))
			have_vol = 1;
		if (eb & PEV_MENU)
			have_menu = 1;
#endif
	}

	// TODO: move to pico
#if 0
	/* autobind some important keys, if they are unbound */
	if (!have_vol && binds[PSP_BTN_VOL_UP] == 0 && binds[PSP_BTN_VOL_DOWN] == 0) {
		binds[IN_BIND_OFFS(PSP_BTN_VOL_UP, IN_BINDTYPE_EMU)]   = PEV_VOL_UP;
		binds[IN_BIND_OFFS(PSP_BTN_VOL_DOWN, IN_BINDTYPE_EMU)] = PEV_VOL_DOWN;
		count += 2;
	}

	if (!have_menu) {
		binds[IN_BIND_OFFS(PSP_BTN_SELECT, IN_BINDTYPE_EMU)] = PEV_MENU;
		count++;
	}
#endif

	in_combos_find(binds, PSP_BTN_HOME, &in_psp_combo_keys, &in_psp_combo_acts);

	return count;
}

static const in_drv_t in_psp_drv = {
	.prefix         = IN_PSP_PREFIX,
	.probe          = in_psp_probe,
	.free           = in_psp_free,
	.get_key_names  = in_psp_get_key_names,
	.clean_binds    = in_psp_clean_binds,
	.update         = in_psp_update,
	.update_keycode = in_psp_update_keycode,
	.menu_translate = in_psp_menu_translate,
};

void in_psp_init(const struct in_default_bind *defbinds)
{
	in_psp_combo_keys = in_psp_combo_acts = 0;

	in_register_driver(&in_psp_drv, defbinds, NULL);
}

