#include "balls.h"

void
drawbg(Image *walls, Image *bg) {
	draw(screen, screen->r, walls, nil, ZP);
	draw(screen, screen->r, bg, nil, ZP);
	flushimage(display, Refnone);
}

Image *
alloccircle(int fg, int bg) {
	Image *m, *fill;

	m = allocimage(display, Rect(0, 0, 2*RADIUS, 2*RADIUS), RGBA32, 0, bg);
	if (m == nil)
		return nil;

	fill = allocimage(display, Rect(0, 0, 1, 1), RGBA32, 1, fg);
	if (fill == nil) {
		free(m);
		return nil;
	}

	fillellipse(m, Pt(RADIUS, RADIUS), RADIUS, RADIUS, fill, ZP);
	freeimage(fill);
	return m;
}

void
drawcircle(Image *m, Point pos) {
	Rectangle r;

	r = Rpt(subpt(pos, Pt(RADIUS, RADIUS)), addpt(pos, Pt(RADIUS, RADIUS)));
	draw(screen, r, m, nil, ZP);
}
