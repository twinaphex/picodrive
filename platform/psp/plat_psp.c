/*
 * plat.c
 *
 *  Created on: 11/08/2016
 *      Author: Robson
 */
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspsdk.h>

#include "psp.h"
#include "../libpicofe/menu.h"
#include "../libpicofe/plat.h"
#include "../libpicofe/input.h"
#include "../libpicofe/psp/in_psp.h"
#include "../libpicofe/linux/in_evdev.h"
#include "../common/input_pico.h"
#include "../common/emu.h"

#define menu_screen psp_screen
static unsigned short bg_buffer[480*272] __attribute__((aligned(16)));

const char *renderer_names[] = { " accurate", " fast", NULL };
const char *renderer_names32x[] = { " faster", " fastest", " accurate", NULL };

int plat_get_skin_dir(char *dst, int len)
{
	memcpy(dst, "skin/", sizeof("skin/"));
	return ( sizeof("skin/") - 1 );
}

void plat_video_menu_enter(int is_rom_loaded)
{
}

void plat_video_menu_leave(void)
{
}

void clear_vram(void) {
	memset32((unsigned short *)VRAM_CACHED_STUFF, 0, 512*240/2);
}

void plat_video_menu_begin(void)
{
//	clear_vram();
//	sceGuSync(0,0); // sync with prev
//	sceGuStart(GU_DIRECT, guCmdList);
//	sceGuCopyImage(GU_PSM_5650, 0, 0, 480, 272, 480, g_menubg_ptr, 0, 0, 512, menu_screen);
//	sceGuFinish();
//	sceGuSync(0,0);
}

void plat_video_menu_end(void)
{
	plat_video_flip(0);
}

static struct in_default_bind in_psp_defbinds[] =
{
	{ PSP_BTN_UP,    IN_BINDTYPE_PLAYER12, GBTN_UP },
	{ PSP_BTN_DOWN,  IN_BINDTYPE_PLAYER12, GBTN_DOWN },
	{ PSP_BTN_LEFT,  IN_BINDTYPE_PLAYER12, GBTN_LEFT },
	{ PSP_BTN_RIGHT, IN_BINDTYPE_PLAYER12, GBTN_RIGHT },
	{ PSP_BTN_SQUARE,     IN_BINDTYPE_PLAYER12, GBTN_A },
	{ PSP_BTN_X,     IN_BINDTYPE_PLAYER12, GBTN_B },
	{ PSP_BTN_CIRCLE,     IN_BINDTYPE_PLAYER12, GBTN_C },
	{ PSP_BTN_START, IN_BINDTYPE_PLAYER12, GBTN_START },
	{ PSP_BTN_TRIANGLE,     IN_BINDTYPE_EMU, GBTN_Y },
	{ PSP_BTN_L,     IN_BINDTYPE_EMU, GBTN_X },
	{ PSP_BTN_R,     IN_BINDTYPE_EMU, GBTN_Z },
	{ PSP_BTN_VOL_DOWN, IN_BINDTYPE_EMU, PEVB_VOL_DOWN },
	{ PSP_BTN_VOL_UP,   IN_BINDTYPE_EMU, PEVB_VOL_UP },
	{ PSP_BTN_SELECT,   IN_BINDTYPE_EMU, PEVB_MENU },
	{ 0, 0, 0 }
};

static const struct menu_keymap key_pbtn_map[] =
{
	{ PSP_BTN_UP,	PBTN_UP },
	{ PSP_BTN_DOWN,	PBTN_DOWN },
	{ PSP_BTN_LEFT,	PBTN_LEFT },
	{ PSP_BTN_RIGHT,	PBTN_RIGHT },
	{ PSP_BTN_CIRCLE,	PBTN_MOK },
	{ PSP_BTN_X,	PBTN_MBACK },
	{ PSP_BTN_SQUARE,	PBTN_MA2 },
	{ PSP_BTN_TRIANGLE,	PBTN_MA3 },
	{ PSP_BTN_SELECT,	PBTN_MENU },
	{ PSP_BTN_L,	PBTN_L },
	{ PSP_BTN_R,	PBTN_R },
};

static const struct in_pdata psp_evdev_pdata = {
	.defbinds = in_psp_defbinds,
//	.key_map = key_pbtn_map,
//	.kmap_size = sizeof(key_pbtn_map) / sizeof(key_pbtn_map[0]),
};

void plat_init(void)
{
	psp_init();

	//in_evdev_init(&psp_evdev_pdata);
	in_psp_init(in_psp_defbinds);
	in_probe();
	plat_target_setup_input();
}

void plat_set_scale_unscaled_centered(void) {
	currentConfig.scale = currentConfig.hscale40 = currentConfig.hscale32 = 1.0;
	currentConfig.scale_int = currentConfig.hscale40_int = currentConfig.hscale32_int = 100;
}

void plat_set_scale_4_3(void) {
	currentConfig.scale = 1.20;
	currentConfig.hscale40 = 1.00;
	currentConfig.hscale32 = 1.25;
	currentConfig.scale_int = (int)(currentConfig.scale * 100.0f);
	currentConfig.hscale40_int = (int)(currentConfig.hscale40 * 100.0f);
	currentConfig.hscale32_int = (int)(currentConfig.hscale32 * 100.0f);
}

void plat_set_scale_fullscreen(void) {
	currentConfig.scale = 1.20;
	currentConfig.hscale40 = 1.25;
	currentConfig.hscale32 = 1.56;
	currentConfig.scale_int = (int)(currentConfig.scale * 100.0f);
	currentConfig.hscale40_int = (int)(currentConfig.hscale40 * 100.0f);
	currentConfig.hscale32_int = (int)(currentConfig.hscale32 * 100.0f);
}

void plat_set_scale_fullscreen_zoomed(void) {
	currentConfig.scale = 1.50;
	currentConfig.hscale40 = 1.00;
	currentConfig.hscale32 = 1.25;
	currentConfig.scale_int = (int)(currentConfig.scale * 100.0f);
	currentConfig.hscale40_int = (int)(currentConfig.hscale40 * 100.0f);
	currentConfig.hscale32_int = (int)(currentConfig.hscale32 * 100.0f);
}

void plat_set_scale_cut_borders(void) {
	currentConfig.scale = 1.32;
	currentConfig.hscale40 = 1.0;
	currentConfig.hscale32 = 1.25;
	currentConfig.scale_int = (int)(currentConfig.scale * 100.0f);
	currentConfig.hscale40_int = (int)(currentConfig.hscale40 * 100.0f);
	currentConfig.hscale32_int = (int)(currentConfig.hscale32 * 100.0f);
}

int plat_mem_set_exec(void *ptr, size_t size)
{
#if 0
	int ret = mprotect(ptr, size, PROT_READ | PROT_WRITE | PROT_EXEC);
	if (ret != 0)
		fprintf(stderr, "mprotect(%p, %zd) failed: %d\n",
			ptr, size, errno);

	return ret;
#else
	return 0;
#endif
}

int scandir(const char *dir, struct my_dirent **namelist_out,
		int(*filter)(const struct my_dirent *),
		int(*compar)(const void *, const void *))
{
	int ret = -1, dir_uid = -1, name_alloc = 4, name_count = 0;
	struct my_dirent **namelist = NULL, *ent;
	SceIoDirent sce_ent;

	namelist = malloc(sizeof(*namelist) * name_alloc);
	if (namelist == NULL) { lprintf("%s:%i: OOM\n", __FILE__, __LINE__); goto fail; }

	// try to read first..
	dir_uid = sceIoDopen(dir);
	if (dir_uid >= 0)
	{
		/* it is very important to clear SceIoDirent to be passed to sceIoDread(), */
		/* or else it may crash, probably misinterpreting something in it. */
		memset(&sce_ent, 0, sizeof(sce_ent));
		ret = sceIoDread(dir_uid, &sce_ent);
		if (ret < 0)
		{
			lprintf("sceIoDread(\"%s\") failed with %i\n", dir, ret);
			goto fail;
		}
	}
	else
		lprintf("sceIoDopen(\"%s\") failed with %i\n", dir, dir_uid);

	while (ret > 0)
	{
		ent = malloc(sizeof(*ent));
		if (ent == NULL) { lprintf("%s:%i: OOM\n", __FILE__, __LINE__); goto fail; }
		ent->d_type = sce_ent.d_stat.st_mode;
		strncpy(ent->d_name, sce_ent.d_name, sizeof(ent->d_name));
		ent->d_name[sizeof(ent->d_name)-1] = 0;
		if (filter == NULL || filter(ent))
		     namelist[name_count++] = ent;
		else free(ent);

		if (name_count >= name_alloc)
		{
			void *tmp;
			name_alloc *= 2;
			tmp = realloc(namelist, sizeof(*namelist) * name_alloc);
			if (tmp == NULL) { lprintf("%s:%i: OOM\n", __FILE__, __LINE__); goto fail; }
			namelist = tmp;
		}

		memset(&sce_ent, 0, sizeof(sce_ent));
		ret = sceIoDread(dir_uid, &sce_ent);
	}

	// sort
	if (compar != NULL && name_count > 3) qsort(&namelist[2], name_count - 2, sizeof(namelist[0]), compar);

	// all done.
	ret = name_count;
	*namelist_out = namelist;
	goto end;

fail:
	if (namelist != NULL)
	{
		while (name_count--)
			free(namelist[name_count]);
		free(namelist);
	}
end:
	if (dir_uid >= 0) sceIoDclose(dir_uid);
	return ret;
}



