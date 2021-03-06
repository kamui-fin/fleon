#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <wait.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <X11/keysym.h>
#include <xcb/xcb_keysyms.h>
#include "utils.h"
#include "fleon.h"

/* Global state */
int sigcode = 0;
xcb_connection_t* conn;
const xcb_setup_t* setup;
xcb_screen_t* screen;
struct client* clients;
struct client* focused;
int current_workspace = 0;

/* For resizing and moving with pointer */
xcb_get_geometry_reply_t* geometry;
xcb_query_pointer_reply_t* pointer;
struct client* mouse_on;

xcb_key_symbols_t* keysyms;
static struct keybind keybinds[] = {
    {MOD_KEY, XK_Return, spawn, {.v = TERMINAL}},
    {MOD_KEY, XK_w, close_focused},
    {MOD_KEY, XK_f, change_layout, {.i = 2}}, // fullscreen
    {MOD_KEY, XK_s, change_layout, {.i = 1}}, // floating

    {MOD_KEY, XK_1, change_workspace, {.i = 0}},
    {MOD_KEY, XK_2, change_workspace, {.i = 1}},
    {MOD_KEY, XK_3, change_workspace, {.i = 2}},
    {MOD_KEY, XK_4, change_workspace, {.i = 3}},
    {MOD_KEY, XK_5, change_workspace, {.i = 4}},
    {MOD_KEY, XK_6, change_workspace, {.i = 5}},
    {MOD_KEY, XK_7, change_workspace, {.i = 6}},
    {MOD_KEY, XK_8, change_workspace, {.i = 7}},
    {MOD_KEY, XK_9, change_workspace, {.i = 8}},
    {MOD_KEY, XK_0, change_workspace, {.i = 9}},

    {MOD_KEY | XCB_MOD_MASK_SHIFT, XK_1, move_focused_to_workspace, {.i = 0}},
    {MOD_KEY | XCB_MOD_MASK_SHIFT, XK_2, move_focused_to_workspace, {.i = 1}},
    {MOD_KEY | XCB_MOD_MASK_SHIFT, XK_3, move_focused_to_workspace, {.i = 2}},
    {MOD_KEY | XCB_MOD_MASK_SHIFT, XK_4, move_focused_to_workspace, {.i = 3}},
    {MOD_KEY | XCB_MOD_MASK_SHIFT, XK_5, move_focused_to_workspace, {.i = 4}},
    {MOD_KEY | XCB_MOD_MASK_SHIFT, XK_6, move_focused_to_workspace, {.i = 5}},
    {MOD_KEY | XCB_MOD_MASK_SHIFT, XK_7, move_focused_to_workspace, {.i = 6}},
    {MOD_KEY | XCB_MOD_MASK_SHIFT, XK_8, move_focused_to_workspace, {.i = 7}},
    {MOD_KEY | XCB_MOD_MASK_SHIFT, XK_9, move_focused_to_workspace, {.i = 8}},
    {MOD_KEY | XCB_MOD_MASK_SHIFT, XK_0, move_focused_to_workspace, {.i = 9}}};

void (*handlers[30])(xcb_generic_event_t*) = {
    [XCB_MAP_REQUEST] = &on_map_request,
    [XCB_MAP_NOTIFY] = &on_map_notify,
    [XCB_DESTROY_NOTIFY] = &on_destroy_notify,
    [XCB_CONFIGURE_NOTIFY] = &on_configure_notify,
    [XCB_BUTTON_PRESS] = &on_button_pressed,
    [XCB_BUTTON_RELEASE] = &on_button_release,
    [XCB_KEY_PRESS] = &on_key_pressed,
    [XCB_KEY_RELEASE] = &on_key_release,
    [XCB_MOTION_NOTIFY] = &on_motion_notify,
};

struct window_mgr wm = {.current_layout = FLOATING};

struct client* find_client(xcb_window_t w) {
    struct client* head = clients;
    while (head != NULL) {
        if (head->win == w) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

void del_client(xcb_window_t w) {
    struct client* tmp = clients;
    struct client* prev;
    if (tmp != NULL && tmp->win == w) {
        clients = tmp->next;
        free(tmp);
        return;
    }

    while (tmp != NULL) {
        if (tmp->win == w) {
            prev->next = tmp->next;
            free(tmp);
            return;
        }
        prev = tmp;
        tmp = tmp->next;
    }
}

void client_add(xcb_window_t w) {
    struct client* new_client = malloc(sizeof(struct client));
    xcb_get_geometry_reply_t* geom =
        xcb_get_geometry_reply(conn, xcb_get_geometry(conn, w), NULL);

    new_client->geom.x = geom->x;
    new_client->geom.y = geom->y;
    new_client->geom.w = geom->width;
    new_client->geom.h = geom->height;
    new_client->win = w;
    new_client->border_size = BORDER_SIZE;
    new_client->next = clients;
    new_client->workspace = current_workspace;

    if (wm.current_layout == FLOATING) {
        new_client->isFloating = true;
    }
    clients = new_client;
}

void client_kill(struct client* c) { xcb_kill_client(conn, c->win); }

void client_resize(struct client* c, int w, int h) {
    uint32_t pos[2] = {w, h};
    xcb_configure_window(
        conn, c->win, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, pos);
}

void client_move(struct client* c, int x, int y) {
    uint32_t pos[2] = {x, y};
    xcb_configure_window(conn, c->win,
                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, pos);
}

void client_move_resize(struct client* c, int x, int y, int w, int h) {
    uint32_t pos[4] = {x, y, w, h};
    xcb_configure_window(conn, c->win,
                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
                             XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                         pos);
}

void client_raise(struct client* c) {
    uint32_t arg[1] = {XCB_STACK_MODE_ABOVE};
    xcb_configure_window(conn, c->win, XCB_CONFIG_WINDOW_STACK_MODE, arg);
}

void on_map_request(xcb_generic_event_t* e) {
    xcb_map_request_event_t* ev = (xcb_map_request_event_t*)e;
    client_add(ev->window);
    xcb_map_window(conn, ev->window);
    client_move_resize(clients, clients->geom.x, clients->geom.y,
                       clients->geom.w, clients->geom.h);
}

void on_button_pressed(xcb_generic_event_t* e) {
    xcb_button_press_event_t* ev = (xcb_button_press_event_t*)e;
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

void on_button_release(xcb_generic_event_t* e) {
    xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
}

void on_key_pressed(xcb_generic_event_t* e) {
    xcb_key_press_event_t* ev = (xcb_key_press_event_t*)e;

    for (int i = 0; i < LENGTH(keybinds); i++) {
        struct keybind k = keybinds[i];
        if (ev->detail == get_keycode(k.keysym) && ev->state == k.mod) {
            k.key_ev_handler(k.arg);
            return;
        }
    }
}

void on_key_release(xcb_generic_event_t* e) {
    xcb_key_release_event_t* ev = (xcb_key_release_event_t*)e;
}

void on_motion_notify(xcb_generic_event_t* e) {
    xcb_motion_notify_event_t* ev = (xcb_motion_notify_event_t*)e;
    struct client* c = find_client(ev->child);
    if (c && c->isFloating) {
        int xdiff = ev->root_x - pointer->root_x;
        int ydiff = ev->root_y - pointer->root_y;
        if (ev->state == (XCB_MOD_MASK_1 | XCB_BUTTON_MASK_1)) {
            client_move(c, geometry->x + xdiff, geometry->y + ydiff);
        } else if (ev->state == (XCB_MOD_MASK_1 | XCB_BUTTON_MASK_3)) {
            client_resize(c, geometry->width + xdiff, geometry->height + ydiff);
        }
    }
}

void on_map_notify(xcb_generic_event_t* e) {
    xcb_map_notify_event_t* ev = (xcb_map_notify_event_t*)e;
    focused = find_client(ev->window);
    set_border(ev->window, BORDER_SIZE, 0xff0000);
}

void on_destroy_notify(xcb_generic_event_t* e) {
    xcb_destroy_notify_event_t* ev = (xcb_destroy_notify_event_t*)e;
    del_client(ev->window);
}

void on_configure_notify(xcb_generic_event_t* e) {
    xcb_configure_notify_event_t* ev = (xcb_configure_notify_event_t*)e;
    struct client* c = find_client(ev->window);
    struct geometry new_geom = {ev->x, ev->y, ev->width, ev->height};
    c->prev_geom.x = c->geom.x;
    c->prev_geom.y = c->geom.y;
    c->prev_geom.w = c->geom.w;
    c->prev_geom.h = c->geom.h;
    c->geom = new_geom;
    c->border_size = ev->border_width;
}

void spawn(arg arg) {
    int pid = fork();
    if (pid == -1) {
        WLOG("Error forking")
    } else if (pid == 0) {
        (void)setsid();
        execvp(arg.v[0], (char* const*)arg.v);
        exit(1);
    }
}

void maximize(struct client* c) {
    int full_w = screen->width_in_pixels - 2 * c->border_size;
    int full_h = screen->height_in_pixels - 2 * c->border_size;
    client_move_resize(c, 0, 0, full_w, full_h);
}

void set_border(xcb_window_t win, int width, int color) {
    xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_BORDER_WIDTH, &width);
    xcb_change_window_attributes(conn, win, XCB_CW_BORDER_PIXEL, &color);
}

xcb_keycode_t get_keycode(xcb_keysym_t keysym) {
    return *xcb_key_symbols_get_keycode(keysyms, keysym);
}

void close_focused() { client_kill(focused); }

void change_layout(arg arg) {
    switch (arg.i) {
    case 1:
        change_floating();
        break;
    case 2:
        change_fullscreen();
        break;
    }
}

void change_fullscreen() {
    focused->isFloating = false;
    focused->isFullscreen = true;
    maximize(clients);
}

void change_floating() {
    focused->isFullscreen = false;
    focused->isFloating = true;
    client_move_resize(focused, focused->prev_geom.x, focused->prev_geom.y,
                       focused->prev_geom.w, focused->prev_geom.h);
}

void change_workspace(arg arg) {
    if (arg.i == current_workspace)
        return;
    struct client* current = clients;

    while (current != NULL) {
        if (current->workspace == arg.i) {
            xcb_map_window(conn, current->win);
        } else if (current->workspace == current_workspace) {
            xcb_unmap_window(conn, current->win);
        }
        current = current->next;
    }

    current_workspace = arg.i;
}

void move_focused_to_workspace(arg arg) {
    focused->workspace = arg.i;
    xcb_unmap_window(conn, focused->win);
}

bool existing_wm(void) {
    xcb_generic_error_t* error;
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

    for (int x = 0; x < LENGTH(keybinds); x++) {
        struct keybind k = keybinds[x];
        xcb_grab_key(conn, 0, screen->root, k.mod, get_keycode(k.keysym),
                     XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }
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

    keysyms = xcb_key_symbols_alloc(conn);
    setup_bindings();
    xcb_flush(conn);
}

void quit(int status) {
    struct client* head = clients;
    while (head != NULL) {
        struct client* temp = head->next;
        free(head);
        head = temp;
    }
    xcb_set_input_focus(conn, XCB_NONE, XCB_INPUT_FOCUS_POINTER_ROOT,
                        XCB_CURRENT_TIME);
    xcb_ungrab_key(conn, XCB_GRAB_ANY, screen->root, XCB_BUTTON_MASK_ANY);
    xcb_ungrab_button(conn, XCB_BUTTON_INDEX_ANY, screen->root,
                      XCB_MOD_MASK_ANY);
    xcb_flush(conn);
    xcb_disconnect(conn);
    exit(status);
}

void sig_handler(int sig) {
    pid_t pid;
    int status;

    switch (sig) {
    case SIGCHLD:
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
            ;
        break;
    case SIGINT:
    case SIGTERM:
        sigcode = sig;
        quit(sig);
        break;
    }
}

void run() {
    signal(SIGCHLD, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    xcb_generic_event_t* e;

    while (true) {
        e = xcb_wait_for_event(conn);
        if (e) {
            xcb_ev_handler_t* handler = handlers[e->response_type & ~0x80];
            if (handler) {
                handler(e);
            } else {
                TLOG("Ignoring event")
            }
            free(e);
            if (xcb_connection_has_error(conn)) {
                FLOG("An unexpected error has occured");
                quit(1);
            }
        }
        xcb_flush(conn);
    }
}

int main(int argc, char* argv[]) {
    initialize();
    run();
}
