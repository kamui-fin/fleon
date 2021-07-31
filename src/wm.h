#include <xcb/xproto.h>
#define MOD_KEY XCB_MOD_MASK_1

#define ROOT_EVENT_MASK                                                        \
    (XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |                                    \
     XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE |     \
     XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE)

enum layout { FLOATING, FULL_SCREEN };

struct window_mgr {
    enum layout current_layout;
};

struct keybind {
    xcb_mod_mask_t mod;
    xcb_keysym_t keysym;
    void (*key_ev_handler)();
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
};

typedef void(xcb_ev_handler_t)(xcb_generic_event_t*);

/* Client managment */
struct client* find_client(xcb_window_t w);
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
void on_configure_notify(xcb_generic_event_t* e);

void close_focused();
void change_fullscreen();
void change_floating();
void change_workspace(int workspace_idx);
void move_window_to_workspace(struct client* c, int workspace_idx);
void set_border(xcb_window_t win, int width, int color);
xcb_keycode_t get_keycode(xcb_keysym_t keysym);

/* Setup and teardown */
void setup_bindings();
void initialize();
bool existing_wm(void);
void quit(int status);
void sigint_quit();
void run();
