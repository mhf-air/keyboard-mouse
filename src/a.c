#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <X11/Xlib.h>

/* void keyboard(Display* display) {
	long mask = KeyPressMask | KeyReleaseMask | ButtonPressMask |
		ButtonReleaseMask | FocusChangeMask;
	Window root = DefaultRootWindow(display);

	Window focused;
	int revert_to;
	XGetInputFocus(display, &focused, &revert_to);
	XSelectInput(display, focused, mask);
	printf("start %ld\n", focused);

	XEvent event;
	while (true) {
		printf("before\n");
		XNextEvent(display, &event);
		printf("after\n");
		// printf("%d\n", event.type);

		switch (event.type) {
			case KeyPress:
				printf("KeyPress\n");
				break;
			case KeyRelease:
				printf("KeyRelease\n");
				break;
			case ButtonPress:
				printf("ButtonPress\n");
				break;
			case ButtonRelease:
				printf("ButtonRelease\n");
				break;
			case FocusIn:
				XGetInputFocus(display, &focused, &revert_to);
				printf("FocusIn %ld %ld\n", event.xfocus.window, focused);
				XSelectInput(display, focused, mask);
				// XSelectInput(display, event.xfocus.window, mask);
				break;
			case FocusOut:
				if (focused != root) {
					XSelectInput(display, focused, 0);
				}
				XGetInputFocus(display, &focused, &revert_to);
				printf("FocusOut %ld -> %ld\n", event.xfocus.window, focused);
				if (focused == PointerRoot) {
					focused = root;
				}
				XSelectInput(display, focused, mask);
				break;
		}
	}
} */

#define KEY_TOGGLE_MOUSE_MODE KEY_LEFTMETA

enum _KEY_EVENT {
	KEY_EVENT_RELEASED = 0,
	KEY_EVENT_PRESSED = 1,
	KEY_EVENT_REPEATED = 2,
};
enum _KEY_MOVE {
	// my j, k, h, l keys
	KEY_MOVE_UP = KEY_V,
	KEY_MOVE_DOWN = KEY_C,
	KEY_MOVE_LEFT = KEY_J,
	KEY_MOVE_RIGHT = KEY_P,
};
static const char* const evval[3] = {"RELEASED", "PRESSED ", "REPEATED"};
void printKey(struct input_event ev) {
	printf("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
}

void handleKeyMove(struct input_event ev, bool* pressed, int* delta,
				   bool positive) {
	int step = 20;
	if (ev.value == KEY_EVENT_RELEASED) {
		*pressed = false;
		*delta = 0;
	} else {
		*pressed = true;
		if (positive) {
			*delta += step;
		} else {
			*delta -= step;
		}
	}
}

void moveMouse(Display* display, int dx, int dy) {
	XWarpPointer(display, None, None, 0, 0, 0, 0, dx, dy);
	XFlush(display);
}

int input() {
	const char* dev = "/dev/input/by-id/usb-046a_0011-event-kbd";
	int fd = open(dev, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
		return EXIT_FAILURE;
	}

	int dx = 0;
	int dy = 0;
	bool upPressed = false;
	bool downPressed = false;
	bool leftPressed = false;
	bool rightPressed = false;

	bool inMouseMode = false;
	// bool f1Pressed = false;

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

		if (ev.code == KEY_TOGGLE_MOUSE_MODE &&
			ev.value == KEY_EVENT_RELEASED) {
			inMouseMode = !inMouseMode;
			if (inMouseMode) {
				XGrabKeyboard(display, rootWindow, true, GrabModeAsync,
							  GrabModeAsync, CurrentTime);
			} else {
				XUngrabKeyboard(display, CurrentTime);
				XFlush(display);
			}
		} else if (!inMouseMode) {
			continue;
			/* } else if (ev.code == KEY_F1 &&
					   ev.value == KEY_EVENT_RELEASED) {  // pressed F1

				if (f1Pressed) {
					inMouseMode = true;
					XGrabKeyboard(display, rootWindow, true, GrabModeAsync,
								  GrabModeAsync, CurrentTime);
				} else {
					inMouseMode = false;
					XUngrabKeyboard(display, CurrentTime);
					XFlush(display);
				}
				f1Pressed = !f1Pressed; */
		}

		switch (ev.code) {
			case KEY_MOVE_UP:
				handleKeyMove(ev, &upPressed, &dy, false);
				break;
			case KEY_MOVE_DOWN:
				handleKeyMove(ev, &downPressed, &dy, true);
				break;
			case KEY_MOVE_LEFT:
				handleKeyMove(ev, &leftPressed, &dx, false);
				break;
			case KEY_MOVE_RIGHT:
				handleKeyMove(ev, &rightPressed, &dx, true);
				break;
		}
		if (leftPressed && rightPressed) {
			dx = 0;
		}
		if (upPressed && downPressed) {
			dy = 0;
		}
		if (dx != 0 || dy != 0) {
			moveMouse(display, dx, dy);
		}
	}

	XCloseDisplay(display);
	fflush(stdout);
	fprintf(stderr, "%s.\n", strerror(errno));
	return EXIT_FAILURE;
}

int main(void) {
	input();

	return 0;
}
