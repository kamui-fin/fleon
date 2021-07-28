#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include "featherbox.h"

xcb_connection_t *conn;
const xcb_setup_t *setup;
xcb_screen_t *screen;
xcb_window_t root_window;

void _log(FILE *fd, char *color, char *level, char *msg) {
    fprintf(fd, "%s:%d %s%s: " RESET "%s\n", __FILE__, __LINE__, color, level,
            msg);
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
    root_window = screen->root;

    const static uint32_t values[] = {
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
        XCB_EVENT_MASK_BUTTON_PRESS,
        XCB_EVENT_MASK_BUTTON_RELEASE,
        XCB_EVENT_MASK_KEY_PRESS,
        XCB_EVENT_MASK_KEY_RELEASE,
    };
    xcb_change_window_attributes(conn, root_window, XCB_CW_EVENT_MASK, values);
    xcb_flush(conn);
}

int main(int argc, char *argv[]) {
    initialize();

    xcb_generic_event_t *e;
    while (1) {
        e = xcb_wait_for_event(conn);
        switch (e->response_type & ~0x80) {
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
    }
}
