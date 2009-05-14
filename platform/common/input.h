#define IN_MAX_DEVS 10

/* unified menu keys */
#define PBTN_UP    (1 <<  0)
#define PBTN_DOWN  (1 <<  1)
#define PBTN_LEFT  (1 <<  2)
#define PBTN_RIGHT (1 <<  3)

#define PBTN_MOK   (1 <<  4)
#define PBTN_MBACK (1 <<  5)
#define PBTN_MA2   (1 <<  6)	/* menu action 2 */
#define PBTN_MA3   (1 <<  7)

#define PBTN_L     (1 <<  8)
#define PBTN_R     (1 <<  9)

#define PBTN_MENU  (1 << 10)

enum {
	IN_DRVID_UNKNOWN = 0,
	IN_DRVID_GP2X,
	IN_DRVID_EVDEV,
	IN_DRVID_COUNT,
};

enum {
	IN_INFO_BIND_COUNT = 0,
	IN_INFO_DOES_COMBOS,
};

typedef struct {
	const char *prefix;
	void (*probe)(void);
	void (*free)(void *drv_data);
	int  (*get_bind_count)(void);
	void (*get_def_binds)(int *binds);
	int  (*clean_binds)(void *drv_data, int *binds);
	void (*set_blocking)(void *data, int y);
	int  (*update_keycode)(void *drv_data, int *is_down);
	int  (*menu_translate)(int keycode);
	int  (*get_key_code)(const char *key_name);
	const char * (*get_key_name)(int keycode);
} in_drv_t;


/* to be called by drivers */
void in_register(const char *nname, int drv_id, int fd_hnd, void *drv_data, int combos);
void in_combos_find(int *binds, int last_key, int *combo_keys, int *combo_acts);
int  in_combos_do(int keys, int *binds, int last_key, int combo_keys, int combo_acts);

void in_init(void);
void in_probe(void);
int  in_update(void);
void in_set_blocking(int is_blocking);
int  in_update_keycode(int *dev_id, int *is_down, int timeout_ms);
int  in_menu_wait_any(int timeout_ms);
int  in_menu_wait(int interesting, int autorep_delay_ms);
int  in_get_dev_info(int dev_id, int what);
void in_config_start(void);
int  in_config_parse_dev(const char *dev_name);
int  in_config_bind_key(int dev_id, const char *key, int mask);
void in_config_end(void);
int  in_bind_key(int dev_id, int keycode, int mask, int force_unbind);
void in_debug_dump(void);

const int  *in_get_dev_binds(int dev_id);
const int  *in_get_dev_def_binds(int dev_id);
const char *in_get_dev_name(int dev_id, int must_be_active, int skip_pfix);
const char *in_get_key_name(int dev_id, int keycode);
