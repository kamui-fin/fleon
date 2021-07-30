#include <xcb/xproto.h>

#define ROOT_EVENT_MASK                                                        \
    (XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |                                    \
     XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE |     \
     XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE)

struct client {
    struct client *next;
    xcb_window_t win;
};

typedef void(ev_handler_t)(xcb_generic_event_t *);

/* Client managment */
struct client *find_client(xcb_window_t w);
void client_add(xcb_window_t w);
void client_kill(struct client *c);
void client_resize(struct client *c, int w, int h);
void client_move(struct client *c, int x, int y);
void client_raise(struct client *c);

/* Event handling */
void on_map_request(xcb_generic_event_t *e);
void on_button_pressed(xcb_generic_event_t *e);
void on_button_release(xcb_generic_event_t *e);
void on_key_pressed(xcb_generic_event_t *e);
void on_key_release(xcb_generic_event_t *e);
void on_motion_notify(xcb_generic_event_t *e);

/* Setup and teardown */
void initialize();
bool existing_wm(void);
void quit(int status);
