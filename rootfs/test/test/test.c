#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <vprintf.h>
#include <kserv.h>
#include <proto.h>
#include <graph/graph.h>
#include <x/xclient.h>
#include <shm.h>

static font_t* _font = NULL;

static int32_t ipc_call(int32_t pid, int32_t call_id, proto_t* in, proto_t* out, void* p) {
	(void)pid;
	(void)call_id;
	(void)out;
	graph_t* g = (graph_t*)p;

	uint32_t style = (uint32_t)proto_read_int(in);
	const char *title = proto_read_str(in);
	int32_t x = proto_read_int(in);
	int32_t y = proto_read_int(in);
	int32_t w = proto_read_int(in);
	int32_t h = proto_read_int(in);

	uint32_t fill_color = 0xffffff;
	uint32_t fg_color = 0x0;
	uint32_t text_color = 0x0;

	if((style & X_STYLE_NO_FRAME) != 0)
		return 0;

	box(g, x, y, w, h, fg_color);//win box

	fill(g, x, y-20, w, 20, fill_color);//title box
	box(g, x, y-20, w, 20, fg_color);//title box
	box(g, x+w-20, y-20, 20, 20, fg_color);//close box
	draw_text(g, x, y-20, title, _font, text_color);//title
	return 0;
}

int main(int argc, char* argv[]) {
	_font = get_font_by_name("8x16");
	const char* dev_name = "/dev/xman0";
	proto_t* out = proto_new(NULL, 0);
	if(fs_fctrl(dev_name, X_CMD_REG_WM, NULL, out) != 0) {
		proto_free(out);
		return -1;
	}

	int32_t shm_id = proto_read_int(out);
	int32_t w = proto_read_int(out);
	int32_t h = proto_read_int(out);

	proto_free(out);

	void* p = shm_map(shm_id);
	if(p == NULL) {
		shm_unmap(shm_id);
		return -1;
	}

	const char* kserv_name =  "serv.xwin";
	if(argc >= 2)
		kserv_name = argv[1];

	if(kserv_get_by_name(kserv_name) >= 0) {
    printf("Panic: '%s' process has been running already!\n", kserv_name);
		shm_unmap(shm_id);
		return -1;
	}
	
	if(kserv_register(kserv_name) != 0) {
    printf("Panic: '%s' service register failed!\n", kserv_name);
		shm_unmap(shm_id);
		return -1;
	}

	if(kserv_ready() != 0) {
    printf("Panic: '%s' service can not get ready!\n", kserv_name);
		shm_unmap(shm_id);
		return -1;
	}

	graph_t* g = graph_new(p, w, h);
	kserv_run(ipc_call, g, NULL, NULL);
	shm_unmap(shm_id);
	graph_free(g);
	return 0;
}
