#ifndef GRAPH_H
#define GRAPH_H

#include <graph/font.h>

typedef struct Graph {
	uint32_t *buffer;
	uint32_t w;
	uint32_t h;

	int32_t fd;
	int32_t shmID;
} GraphT;

uint32_t rgb(uint32_t r, uint32_t g, uint32_t b);

GraphT* graphOpen(const char* fname);

void graphFlush(GraphT* g);

void graphClose(GraphT* g);

void pixel(GraphT* g, int32_t x, int32_t y, uint32_t color);

void clear(GraphT* g, uint32_t color);

void box(GraphT* g, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color);

void fill(GraphT* g, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color);

void line(GraphT* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);

void drawChar(GraphT* g, int32_t x, int32_t y, char c, FontT* font, uint32_t color);

void drawText(GraphT* g, int32_t x, int32_t y, const char* str, FontT* font, uint32_t color);

#endif
