#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <X11/keysym.h>
#include <xcb/xcb_keysyms.h>
#include "utils.h"
#include "wm.h"

/* Global state */
xcb_connection_t *conn;
const xcb_setup_t *setup;
xcb_screen_t *screen;
struct client *clients;
struct client *focused;

/* For resizing and moving with pointer */
xcb_get_geometry_reply_t *geometry;
xcb_query_pointer_reply_t *pointer;
struct client *mouse_on;

struct client *find_client(xcb_window_t w) {
    struct client *head = clients;
    while (head != NULL) {
        if (head->win == w) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

void client_add(xcb_window_t w) {
    struct client *new_client = malloc(sizeof(struct client));
    new_client->win = w;
    new_client->next = clients;
    clients = new_client;
}

void client_kill(struct client *c) { xcb_kill_client(conn, c->win); }

void client_resize(struct client *c, int w, int h) {
    uint32_t pos[2] = {w, h};
    xcb_configure_window(
        conn, c->win, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, pos);
}

void client_move(struct client *c, int x, int y) {
    uint32_t pos[2] = {x, y};
    xcb_configure_window(conn, c->win,
                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, pos);
}

void client_raise(struct client *c) {
    uint32_t arg[1] = {XCB_STACK_MODE_ABOVE};
    xcb_configure_window(conn, c->win, XCB_CONFIG_WINDOW_STACK_MODE, arg);
}

void client_set_border(struct client *c, int width, int color) {
    xcb_configure_window(conn, c->win, XCB_CONFIG_WINDOW_BORDER_WIDTH, &width);
    xcb_change_window_attributes(conn, c->win, XCB_CW_BORDER_PIXEL, &color);
}

void on_map_request(xcb_generic_event_t *e) {
    xcb_map_request_event_t *ev = (xcb_map_request_event_t *)e;
    client_add(ev->window);
    xcb_map_window(conn, ev->window);
}

void on_button_pressed(xcb_generic_event_t *e) {
    xcb_button_press_event_t *ev = (xcb_button_press_event_t *)e;
    mouse_on = find_client(ev->child);
    if (mouse_on) {
        geometry = xcb_get_geometry_reply(
            conn, xcb_get_geometry(conn, mouse_on->win), NULL);
        pointer = xcb_query_pointer_reply(
            conn, xcb_query_pointer(conn, screen->root), 0);
        if (ev->state == XCB_MOD_MASK_1) {
            xcb_grab_pointer(conn, 0, screen->root,
                             XCB_EVENT_MASK_BUTTON_RELEASE |
                                 XCB_EVENT_MASK_BUTTON_MOTION,
                             XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                             screen->root, XCB_NONE, XCB_CURRENT_TIME);
        } else {
            focused = mouse_on;
            xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
                                focused->win, XCB_CURRENT_TIME);
            client_raise(focused);
            xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
        }
    }
}

void on_button_release(xcb_generic_event_t *e) {
    xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
}

void on_key_pressed(xcb_generic_event_t *e) {
    xcb_key_press_event_t *ev = (xcb_key_press_event_t *)e;
    struct client *c = find_client(ev->child);
    client_kill(c);
}

void on_key_release(xcb_generic_event_t *e) {
    xcb_key_release_event_t *ev = (xcb_key_release_event_t *)e;
}

void on_motion_notify(xcb_generic_event_t *e) {
    xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)e;
    struct client *c = find_client(ev->child);
    if (c) {
        int xdiff = ev->root_x - pointer->root_x;
        int ydiff = ev->root_y - pointer->root_y;
        if (ev->state == (XCB_MOD_MASK_1 | XCB_BUTTON_MASK_1)) {
            client_move(c, geometry->x + xdiff, geometry->y + ydiff);
        } else if (ev->state == (XCB_MOD_MASK_1 | XCB_BUTTON_MASK_3)) {
            client_resize(c, geometry->width + xdiff, geometry->height + ydiff);
        }
    }
}

void on_map_notify(xcb_generic_event_t *e) {
    xcb_map_notify_event_t *ev = (xcb_map_notify_event_t *)e;
    struct client *c = find_client(ev->window);
    client_set_border(c, 10, 0xff0000);
}
void quit(int status) {
    xcb_disconnect(conn);
    exit(status);
}

bool existing_wm(void) {
    xcb_generic_error_t *error;
    uint32_t values[] = {ROOT_EVENT_MASK};
    error = xcb_request_check(
        conn, xcb_change_window_attributes_checked(conn, screen->root,
                                                   XCB_CW_EVENT_MASK, values));
    return error != NULL;
}

void setup_bindings() {
    xcb_grab_button(conn, 0, screen->root,
                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root,
                    XCB_NONE, XCB_BUTTON_INDEX_1, XCB_MOD_MASK_1);
    xcb_grab_button(conn, 0, screen->root,
                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root,
                    XCB_NONE, XCB_BUTTON_INDEX_3, XCB_MOD_MASK_1);
    xcb_grab_button(conn, 0, screen->root, XCB_EVENT_MASK_BUTTON_PRESS,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root,
                    XCB_NONE, 1, XCB_NONE);

    xcb_key_symbols_t *keysyms = xcb_key_symbols_alloc(conn);
    xcb_grab_key(conn, 0, screen->root, XCB_MOD_MASK_1,
                 *xcb_key_symbols_get_keycode(keysyms, XK_w),
                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
}

void initialize() {
    int screen_num;
    conn = xcb_connect(NULL, &screen_num);
    if (xcb_connection_has_error(conn)) {
        FLOG("Error opening display");
        exit(1);
    }
    setup = xcb_get_setup(conn);
    screen = xcb_setup_roots_iterator(setup).data;
    if (existing_wm()) {
        FLOG("A window manager is already running");
        quit(1);
    }

    setup_bindings();
    xcb_flush(conn);
}

int main(int argc, char *argv[]) {
    initialize();
    xcb_generic_event_t *e;

    void (*handlers[30])(xcb_generic_event_t *) = {
        [XCB_MAP_REQUEST] = &on_map_request,
        [XCB_BUTTON_PRESS] = &on_button_pressed,
        [XCB_MAP_NOTIFY] = &on_map_notify,
        [XCB_BUTTON_RELEASE] = &on_button_release,
        [XCB_KEY_PRESS] = &on_key_pressed,
        [XCB_KEY_RELEASE] = &on_key_release,
        [XCB_MOTION_NOTIFY] = &on_motion_notify,
    };

    while (1) {
        e = xcb_wait_for_event(conn);
        ev_handler_t *handler = handlers[e->response_type & ~0x80];
        if (handler) {
            handler(e);
        } else {
            TLOG("Ignoring event")
        }
        free(e);
        if (xcb_connection_has_error(conn)) {
            FLOG("An unexpected error has occured");
            quit(1);
            break;
        }
        xcb_flush(conn);
    }
}
