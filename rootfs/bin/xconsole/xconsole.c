#include <dev/devserv.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <kstring.h>
#include <console.h>
#include <x/xclient.h>

static int run(int argc, char* argv[]) {
	int pos_x = 0, pos_y = 40;
	int pos_w = 640, pos_h = 480;

	if(argc >= 3) {
		pos_x = atoi(argv[1]);
		pos_y = atoi(argv[2]);
	}
	if(argc >= 5) {
		pos_w = atoi(argv[3]);
		pos_h = atoi(argv[4]);
	}

	x_t x;
	console_t console;
	console_init(&console, "/etc/xconsole.conf");
	if(x_open("/dev/xman0", &x) != 0)
		return -1;

	x_resize_to(&x, pos_w, pos_h);
	console.g = x_get_graph(&x);
	x_set_title(&x, argv[0]);
	x_move_to(&x, pos_x, pos_y);
	console_reset(&console);

	while(true) {
		x_ev_t ev;
		if(x_get_event(&x, &ev) == 0) {
			if(ev.type == X_EV_KEYB) {
				char c = (char)ev.value.keyboard.value;
				write(1, &c, 1);
			}
			else if(ev.type == X_EV_WIN) {
				if(ev.state == X_EV_WIN_CLOSE)
					break;
			}
		}

		char buf[256];
		int32_t size;
		size = read(0, buf, 255);
		if(size <= 0) {
			if(errno != EAGAIN) {
				break;
			}
			else {
				sleep(0);
				continue;
			}
		}

		buf[size] = 0;
		const char* p = (const char*)buf;
		for(int32_t i=0; i<size; i++) {
			char c = p[i];
			console_put_char(&console, c);
		}
		x_flush(&x);
	}
	graph_free(console.g);
	x_close(&x);
	return 0;
}

int main(int argc, char* argv[]) {
	int fds1[2];
	int fds2[2];
	pipe(fds1);
	pipe(fds2);

	int pid = fork();
	if(pid != 0) { //father proc for p2 reader.
		dup2(fds1[0], 0);
		dup2(fds2[1], 1);
		close(fds1[0]);
		close(fds1[1]);
		close(fds2[0]);
		close(fds2[1]);
		return run(argc, argv);
	}
	//child proc for p1 writer
	dup2(fds1[1], 1);
	dup2(fds2[0], 0);
	close(fds1[0]);
	close(fds1[1]);
	close(fds2[0]);
	close(fds2[1]);
	setenv("CONSOLE", "xconsole");
	return exec("/bin/shell");
}