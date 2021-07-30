#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include "utils.h"
#include "wm.h"

/* Global state */
xcb_connection_t *conn;
const xcb_setup_t *setup;
xcb_screen_t *screen;
struct client *clients;


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

void client_raise(xcb_window_t win) {
    uint32_t arg[1] = {XCB_STACK_MODE_ABOVE};
    xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_STACK_MODE, arg);
}

void on_map_request(xcb_generic_event_t *e) {
    xcb_map_request_event_t *ev = (xcb_map_request_event_t *)e;
    client_add(ev->window);
    xcb_map_window(conn, ev->window);
}

void quit(int status) {
    xcb_disconnect(conn);
    exit(status);
}

bool existing_wm(void) {
    xcb_generic_error_t *error;
    uint32_t values[] = {ROOT_EVENT_MASK};
    // error and quit if found
    error = xcb_request_check(
        conn, xcb_change_window_attributes_checked(conn, screen->root,
                                                   XCB_CW_EVENT_MASK, values));
    return error != NULL;
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
    xcb_grab_button(conn, 0, screen->root,
                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root,
                    XCB_NONE, XCB_BUTTON_INDEX_1, XCB_MOD_MASK_1);
    xcb_grab_button(conn, 0, screen->root,
                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root,
                    XCB_NONE, XCB_BUTTON_INDEX_3, XCB_MOD_MASK_1);
    xcb_flush(conn);
}

int main(int argc, char *argv[]) {
    initialize();
    xcb_generic_event_t *e;

    void (*handlers[30])(xcb_generic_event_t *) = {
        [XCB_MAP_REQUEST] = &on_map_request,
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
