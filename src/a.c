#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <X11/Xlib.h>

void moveMouse(Display* display, int dx, int dy) {
	XWarpPointer(display, None, None, 0, 0, 0, 0, dx, dy);
}

void keyboard(Display* display) {
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
}

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
