#include <xcb/xproto.h>
#define MOD_KEY XCB_MOD_MASK_1

#define ROOT_EVENT_MASK                                                        \
    (XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |                                    \
     XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE |     \
     XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE)

#define BORDER_SIZE 10

static const char* TERMINAL[1] = {"alacritty"};

enum layout { FLOATING };

struct window_mgr {
    enum layout current_layout;
};

typedef union {
    int i;
    char* const* v;
} arg;

struct keybind {
    xcb_mod_mask_t mod;
    xcb_keysym_t keysym;
    void (*key_ev_handler)();
    arg arg;
};

struct geometry {
    int x, y;
    int w, h;
};

struct client {
    struct client* next;
    xcb_window_t win;
    struct geometry geom;
    struct geometry prev_geom;
    int border_size;
    int workspace;
    bool isFullscreen, isFloating;
};

typedef void(xcb_ev_handler_t)(xcb_generic_event_t*);

/* Client managment */
struct client* find_client(xcb_window_t w);
void del_client(xcb_window_t w);
void client_add(xcb_window_t w);
void client_kill(struct client* c);
void client_resize(struct client* c, int w, int h);
void client_move(struct client* c, int x, int y);
void client_move_resize(struct client* c, int x, int y, int w, int h);
void client_raise(struct client* c);

/* Event handling */
void on_map_request(xcb_generic_event_t* e);
void on_button_pressed(xcb_generic_event_t* e);
void on_button_release(xcb_generic_event_t* e);
void on_key_pressed(xcb_generic_event_t* e);
void on_key_release(xcb_generic_event_t* e);
void on_motion_notify(xcb_generic_event_t* e);
void on_map_notify(xcb_generic_event_t* e);
void on_destroy_notify(xcb_generic_event_t* e);
void on_configure_notify(xcb_generic_event_t* e);

void spawn(arg arg);
void maximize(struct client* c);
void set_border(xcb_window_t win, int width, int color);
xcb_keycode_t get_keycode(xcb_keysym_t keysym);
void close_focused();
void change_layout(arg arg);
void change_fullscreen();
void change_floating();
void change_workspace(arg arg);
void move_focused_to_workspace(arg arg);

/* Setup and teardown */
bool existing_wm(void);
void setup_bindings();
void initialize();
void quit(int status);
void sig_handler();
void run();
