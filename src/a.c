#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <X11/Xlib.h>

#include "a.h"

int main(void) {
	Display* display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "error: no display\n");
		exit(1);
	}

	keyboard(display);

	for (int i = 0; i < 10; i++) {
		moveMouse(display, 1, 1);
	}

	XCloseDisplay(display);
	return 0;
}

void moveMouse(Display* display, int dx, int dy) {
	XWarpPointer(display, None, None, 0, 0, 0, 0, dx, dy);
}

void keyboard(Display* display) {
	Window root = DefaultRootWindow(display);

	// XGrabKeyboard(display, root, true, GrabModeAsync, GrabModeAsync,
	// CurrentTime);

	XSelectInput(
		display, root,
		KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask);

	XEvent ev;
	while (true) {
		XNextEvent(display, &ev);
		printf("%d\n", ev.type);
		switch (ev.type) {
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
		}
		// break;
	}
}
