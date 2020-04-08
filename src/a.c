#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
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
static void print_key(struct input_event ev) {
	printf("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
}

static void handle_key_move(struct input_event ev, bool* pressed, int* delta,
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

static void move_mouse(Display* display, int dx, int dy) {
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

	int step_x = 1;
	if (dx < 0) {
		step_x = -1;
	} else if (dx == 0) {
		step_x = 0;
	}
	int step_y = 1;
	if (dy < 0) {
		step_y = -1;
	} else if (dy == 0) {
		step_y = 0;
	}

	for (int i = 0; i < len; i++) {
		XWarpPointer(display, None, None, 0, 0, 0, 0, step_x, step_y);
	}
	XFlush(display);
}

// requires libxtst-dev for <X11/extensions/XTest.h>
static void click_mouse(Display* display, struct input_event ev, int button) {
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

static const char* PID_FILE = "/var/run/keyboard-mouse.pid";
static void sig_handler(int sig, siginfo_t* siginfo, void* context) {
	if (sig == SIGINT) {
		int r = remove(PID_FILE);
		if (r != 0) {
			fprintf(stderr, "Cannot remove file: %s\n", PID_FILE);
		}
		exit(EXIT_FAILURE);
	}
}

static void one_instance(bool skip_old_pid) {
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = &sig_handler;
	act.sa_flags = SA_SIGINFO;
	if (sigaction(SIGINT, &act, NULL) == -1) {
		fprintf(stderr, "Cannot catch SIGINT\n");
		exit(EXIT_FAILURE);
	}

	FILE* fp;

	if (!skip_old_pid) {
		fp = fopen(PID_FILE, "r");
		if (fp != NULL) {
			pid_t old_pid;
			int r = fscanf(fp, "%d", &old_pid);
			if (r != EOF) {
				r = kill(old_pid, SIGTERM);
				if (r == -1) {
					fprintf(stderr, "Cannot kill process %d: %s.\n", old_pid,
							strerror(errno));
					exit(EXIT_FAILURE);
				}
			}
			fclose(fp);
		}
	}

	fp = fopen(PID_FILE, "w+");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open %s: %s.\n", PID_FILE, strerror(errno));
		exit(EXIT_FAILURE);
	}

	pid_t pid = getpid();
	int r = fprintf(fp, "%d", pid);
	if (r < 0) {
		fprintf(stderr, "Cannot write to %s: %s.\n", PID_FILE, strerror(errno));
	}

	fclose(fp);
}

int main(int argc, char* argv[]) {
	bool skip_old_pid = false;
	char* dev = "";

	// process command line flags
	int opt;
	while ((opt = getopt(argc, argv, ":sf:")) != -1) {
		switch (opt) {
			case 's':
				skip_old_pid = true;
				break;
			case 'f':
				dev = optarg;
				break;
			case ':':
				printf("option needs a value\n");
				break;
			case '?':
				printf("unknown option: %c\n", optopt);
				break;
		}
	}
	if (strlen(dev) == 0) {
		dev = DEFAULT_KEYBOARD_INPUT_FILE;
	}
	one_instance(skip_old_pid);

	// open keyboard device
	int fd = open(dev, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
		return EXIT_FAILURE;
	}

	int acc = 0;
	int dx = 0;
	int dy = 0;
	bool up_pressed = false;
	bool down_pressed = false;
	bool left_pressed = false;
	bool right_pressed = false;

	bool in_mouse_mode = false;

	Display* display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "error: no display\n");
		exit(EXIT_FAILURE);
	}
	Window root_window = DefaultRootWindow(display);

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
		// print_key(ev);

		if (ev.code == KEY_TOGGLE_MOUSE_MODE) {
			if (ev.value == KEY_EVENT_PRESSED) {
				in_mouse_mode = true;
				XGrabKeyboard(display, root_window, true, GrabModeAsync,
							  GrabModeAsync, CurrentTime);
			} else if (ev.value == KEY_EVENT_RELEASED) {
				in_mouse_mode = false;
				XUngrabKeyboard(display, CurrentTime);
				XFlush(display);
			}
			continue;
		}
		if (!in_mouse_mode) {
			continue;
		}

		switch (ev.code) {
			case KEY_MOVE_UP:
				handle_key_move(ev, &up_pressed, &dy, false, &acc);
				break;
			case KEY_MOVE_DOWN:
				handle_key_move(ev, &down_pressed, &dy, true, &acc);
				break;
			case KEY_MOVE_LEFT:
				handle_key_move(ev, &left_pressed, &dx, false, &acc);
				break;
			case KEY_MOVE_RIGHT:
				handle_key_move(ev, &right_pressed, &dx, true, &acc);
				break;

			case KEY_MOUSE_LEFT:
				click_mouse(display, ev, 1);
				break;
			case KEY_MOUSE_MIDDLE:
				click_mouse(display, ev, 2);
				break;
			case KEY_MOUSE_RIGHT:
				click_mouse(display, ev, 3);
				break;
			case KEY_MOUSE_SCROLL_FORWARD:
				click_mouse(display, ev, 4);
				break;
			case KEY_MOUSE_SCROLL_BACKWARD:
				click_mouse(display, ev, 5);
				break;
		}
		if (left_pressed && right_pressed) {
			dx = 0;
		}
		if (up_pressed && down_pressed) {
			dy = 0;
		}
		move_mouse(display, dx, dy);
	}

	XCloseDisplay(display);
	fflush(stdout);
	fprintf(stderr, "%s.\n", strerror(errno));
	return EXIT_FAILURE;
}
