#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

#include "a.h"

enum _KEY_EVENT {
	KEY_EVENT_RELEASED = 0,
	KEY_EVENT_PRESSED = 1,
	KEY_EVENT_REPEATED = 2,
};

static const char* const evval[3] = {"RELEASED", "PRESSED ", "REPEATED"};
void printKey(struct input_event ev) {
	printf("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
}

void handleKeyMove(struct input_event ev, bool* pressed, int* delta,
				   bool positive, int* acc) {
	int step = 11;
	if (ev.value == KEY_EVENT_RELEASED) {
		*pressed = false;
		*delta = 0;
		*acc = 0;
	} else {
		*acc += 2;
		step += *acc;

		*pressed = true;
		if (positive) {
			*delta += step;
		} else {
			*delta -= step;
		}
	}
}

void moveMouse(Display* display, int dx, int dy) {
	if (dx == 0 && dy == 0) {
		return;
	}
	int len = dx;
	if (dx == 0) {
		len = dy;
	}
	if (len < 0) {
		len = -len;
	}

	int stepX = 1;
	if (dx < 0) {
		stepX = -1;
	} else if (dx == 0) {
		stepX = 0;
	}
	int stepY = 1;
	if (dy < 0) {
		stepY = -1;
	} else if (dy == 0) {
		stepY = 0;
	}

	for (int i = 0; i < len; i++) {
		XWarpPointer(display, None, None, 0, 0, 0, 0, stepX, stepY);
	}
	XFlush(display);
}

// requires libxtst-dev for <X11/extensions/XTest.h>
void clickMouse(Display* display, struct input_event ev, int button) {
	if (ev.value == KEY_EVENT_PRESSED) {
		XTestFakeButtonEvent(display, button, true, CurrentTime);
	} else if (ev.value == KEY_EVENT_RELEASED) {
		XTestFakeButtonEvent(display, button, false, CurrentTime);
	} else if (ev.value == KEY_EVENT_REPEATED) {
		switch (button) {
			case 4:
			case 5:
				XTestFakeButtonEvent(display, button, true, CurrentTime);
				XTestFakeButtonEvent(display, button, false, CurrentTime);
				break;
		}
	}
	XFlush(display);
}

int main(void) {
	const char* dev = "/dev/input/by-id/usb-046a_0011-event-kbd";
	int fd = open(dev, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
		return EXIT_FAILURE;
	}

	int acc = 0;
	int dx = 0;
	int dy = 0;
	bool upPressed = false;
	bool downPressed = false;
	bool leftPressed = false;
	bool rightPressed = false;

	bool inMouseMode = false;

	Display* display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "error: no display\n");
		exit(1);
	}
	Window rootWindow = DefaultRootWindow(display);

	struct input_event ev;
	ssize_t n;
	while (true) {
		n = read(fd, &ev, sizeof(ev));
		if (n == (ssize_t)-1) {
			if (errno == EINTR) {
				continue;
			} else {
				break;
			}
		} else if (n != sizeof(ev)) {
			errno = EIO;
			break;
		}
		if (ev.type != EV_KEY || ev.value < 0 && ev.value > 2) {
			continue;
		}
		// printKey(ev);

		if (ev.code == KEY_TOGGLE_MOUSE_MODE) {
			if (ev.value == KEY_EVENT_PRESSED) {
				inMouseMode = true;
				XGrabKeyboard(display, rootWindow, true, GrabModeAsync,
							  GrabModeAsync, CurrentTime);
			} else if (ev.value == KEY_EVENT_RELEASED) {
				inMouseMode = false;
				XUngrabKeyboard(display, CurrentTime);
				XFlush(display);
			}
			continue;
		}
		if (!inMouseMode) {
			continue;
		}

		switch (ev.code) {
			case KEY_MOVE_UP:
				handleKeyMove(ev, &upPressed, &dy, false, &acc);
				break;
			case KEY_MOVE_DOWN:
				handleKeyMove(ev, &downPressed, &dy, true, &acc);
				break;
			case KEY_MOVE_LEFT:
				handleKeyMove(ev, &leftPressed, &dx, false, &acc);
				break;
			case KEY_MOVE_RIGHT:
				handleKeyMove(ev, &rightPressed, &dx, true, &acc);
				break;

			case KEY_MOUSE_LEFT:
				clickMouse(display, ev, 1);
				break;
			case KEY_MOUSE_MIDDLE:
				clickMouse(display, ev, 2);
				break;
			case KEY_MOUSE_RIGHT:
				clickMouse(display, ev, 3);
				break;
			case KEY_MOUSE_SCROLL_FORWARD:
				clickMouse(display, ev, 4);
				break;
			case KEY_MOUSE_SCROLL_BACKWARD:
				clickMouse(display, ev, 5);
				break;
		}
		if (leftPressed && rightPressed) {
			dx = 0;
		}
		if (upPressed && downPressed) {
			dy = 0;
		}
		moveMouse(display, dx, dy);
	}

	XCloseDisplay(display);
	fflush(stdout);
	fprintf(stderr, "%s.\n", strerror(errno));
	return EXIT_FAILURE;
}
