#include "balls.h"

void
drawbg(Image *walls, Image *bg) {
	draw(screen, screen->r, walls, nil, ZP);
	draw(screen, screen->r, bg, nil, ZP);
	flushimage(display, Refnone);
}

Image *
alloccircle(int fg, int bg, uint radius) {
	Image *m, *fill;
	uint d;

	d = 2*radius; /* diameter */
	printf("alloccircle: d=%u\n", d);
	m = allocimage(display, Rect(0, 0, d, d), RGBA32, 0, bg);
	if (m == nil)
		return nil;

	fill = allocimage(display, Rect(0, 0, 1, 1), RGBA32, 1, fg);
	if (fill == nil) {
		free(m);
		return nil;
	}

	fillellipse(m, Pt(radius, radius), radius, radius, fill, ZP);
	freeimage(fill);
	return m;
}

void
drawcircle(Image *m, Point pos) {
	uint radius;
	Rectangle r;

	radius = Dx(m->r)/2;
	r = Rpt(subpt(pos, Pt(radius, radius)), addpt(pos, Pt(radius, radius)));
	draw(screen, r, m, nil, ZP);
}
