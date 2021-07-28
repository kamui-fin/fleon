#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include "featherbox.h"

xcb_connection_t *conn;
const xcb_setup_t *setup;
xcb_screen_t *screen;

bool existing_wm(void) {
    xcb_generic_error_t *error;
    unsigned int values[] = {EVENT_MASK};

    // error and quit if found
    error = xcb_request_check(
        conn, xcb_change_window_attributes_checked(conn, screen->root,
                                                   XCB_CW_EVENT_MASK, values));
    xcb_flush(conn);
    return error != NULL;
}

void initialize() {
    int screenNum;
    conn = xcb_connect(NULL, &screenNum);
    if (xcb_connection_has_error(conn)) {
        FLOG("Error opening display");
        exit(1);
    }
    setup = xcb_get_setup(conn);
    screen = xcb_setup_roots_iterator(setup).data;
    if (existing_wm()) {
        FLOG("A window manager is already running");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    initialize();
    xcb_generic_event_t *e;
    while (1) {
        e = xcb_wait_for_event(conn);
        switch (e->response_type & ~0x80) {
        case XCB_MAP_REQUEST: {
            xcb_map_request_event_t *ev = (xcb_map_request_event_t *)e;
            xcb_map_window(conn, ev->window);
            break;
        }
        case XCB_KEY_PRESS: {
            xcb_key_press_event_t *ev = (xcb_key_press_event_t *)e;
            ILOG("Pressed key")
            break;
        }
        case XCB_KEY_RELEASE: {
            xcb_key_release_event_t *ev = (xcb_key_release_event_t *)e;
            ILOG("Released key")
            break;
        }
        }
        free(e);
        if (xcb_connection_has_error(conn)) {
            FLOG("An unexpected error has occured");
            exit(1);
            break;
        }
        xcb_flush(conn);
    }
}
