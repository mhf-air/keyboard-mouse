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

void moveMouse(int dx, int dy) {
	Display* display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "error: no display\n");
		exit(1);
	}
	XWarpPointer(display, None, None, 0, 0, 0, 0, dx, dy);
	XCloseDisplay(display);
}

enum KEY_EVENT {
	KEY_EVENT_RELEASED = 0,
	KEY_EVENT_PRESSED = 1,
	KEY_EVENT_REPEATED = 2,
};
enum KEY_MOVE {
	KEY_MOVE_UP = KEY_W,
	KEY_MOVE_DOWN = KEY_S,
	KEY_MOVE_LEFT = KEY_A,
	KEY_MOVE_RIGHT = KEY_D,
};
static const char* const evval[3] = {"RELEASED", "PRESSED ", "REPEATED"};
void printKey(struct input_event ev) {
	printf("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
}

void handleKeyMove(struct input_event ev, int* dp, int delta) {
	if (ev.value == KEY_EVENT_PRESSED) {
		(*dp) += delta;
	} else if (ev.value == KEY_EVENT_RELEASED) {
		(*dp) = 0;
		(*dp) -= delta;
	}
}

int input() {
	const char* dev = "/dev/input/by-id/usb-046a_0011-event-kbd";
	struct input_event ev;
	ssize_t n;
	int fd;

	fd = open(dev, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
		return EXIT_FAILURE;
	}

	int dx = 0;
	int dy = 0;
	int totalDx = 0;
	int totalDy = 0;

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

		printKey(ev);
		switch (ev.code) {
			case KEY_MOVE_UP:
				handleKeyMove(ev, &dy, -1);
				break;
			case KEY_MOVE_DOWN:
				handleKeyMove(ev, &dy, 1);
				break;
			case KEY_MOVE_LEFT:
				handleKeyMove(ev, &dx, -1);
				break;
			case KEY_MOVE_RIGHT:
				handleKeyMove(ev, &dx, 1);
				break;
		}
		if (dx != 0 || dy != 0) {
			moveMouse(dx, dy);
		}
	}
	fflush(stdout);
	fprintf(stderr, "%s.\n", strerror(errno));
	return EXIT_FAILURE;
}

int main(void) {
	input();

	return 0;
}
