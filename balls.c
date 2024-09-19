#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <cursor.h>
#include <keyboard.h>

#define NELEMS(arr) (sizeof(arr) / sizeof(arr[0]))

enum {
	BG = DWhite,
	WALLS = DBlack,

	PAD = 0,
	WIDTH = 640,
	HEIGHT=480,

	KEY_QUIT = 'q' ,

	NBALLS = 2,
	RADIUS = 20,
	VMAX = 5,
	G = 1,

	FPS = 60,
	NS_PER_SEC = 1000000000,
	FRAME_TIME = NS_PER_SEC / FPS,
	TICK_BUFSIZE = 4,

	POS_BUFSIZE = 4,
};

typedef struct {
	Point pos;
	int vx, vy;
	int col;

	Channel *frametick;

	Channel **posin; /* receive positions of other balls */
	Channel **posout; /* send position to other balls */
	int nothers; /* number of other balls */
} BallArg;

int ballcolors[] = { DRed, DGreen, DBlue };
Rectangle bounds = {{PAD, PAD}, {PAD+WIDTH, PAD+HEIGHT}};

int init(char *label, Mousectl **mctl, Keyboardctl **kctl);
void spawnball(void);
void drawbg(Image *walls, Image *bg);
void spawnballs(int n);
Channel **allocchans(int nchans, int elsize, int nel);
void mcopycolskip(Channel *vec[], Channel **matrix[], int n, int col, int skip);
void vcopyskip(Channel *dst[], Channel *src[], int n, int skip);
void nooverlapcircles(Point centers[], int n);
Point randptinrect(Rectangle r);
int randint(int lo, int hi);
int isoverlapcircle(Point p, Point q);
int dist(Point p, Point q);
void ball(void *arg);
Image *alloccircle(int fg, int bg);
void drawcircle(Image *m, Point pos);
int clamp(int v, int lo, int hi);
int min(int a, int b);
int max(int a, int b);
void broadcast(Point p, Channel *cs[], int n);
void frametick(void *arg);

void
threadmain(int argc, char *argv[]) {
	Image *bg, *walls;
	Mousectl *mctl;
	Keyboardctl *kctl;
	int resize[2];
	Rune key;

	if (init(argv[0], &mctl, &kctl))
		sysfatal("%s: %r", argv[0]);

	printf("screen: (%d,%d) (%d,%d)\n", screen->r.min.x, screen->r.min.y,
		screen->r.max.x, screen->r.max.y);
	printf("bounds: (%d,%d) (%d,%d)\n", bounds.min.x, bounds.min.y,
		bounds.max.x, bounds.max.y);

	walls = allocimage(display, Rect(0, 0, 1, 1), RGBA32, 1, WALLS);
	bg = allocimage(display, bounds, RGBA32, 0, BG);
	if (bg == nil || walls == nil)
		sysfatal("failed to allocate images");
	
	drawbg(walls, bg);

	spawnballs(NBALLS);

	enum { RESIZE = 0, KEYBD = 1 };
	Alt alts[3] = {
		{mctl->resizec, &resize, CHANRCV},
		{kctl->c, &key, CHANRCV},
		{nil, nil, CHANEND},
	};
	for (;;) {
		switch (alt(alts)) {
		case RESIZE:
			if (getwindow(display, Refnone) < 0)
				sysfatal("%s: %r", argv[0]);
			drawbg(walls, bg);
			break;
		case KEYBD:
			if (key == KEY_QUIT)
				threadexitsall(0);
			break;
		case -1: /* interrupted */
			break;
		}
	}
}

int
init(char *label, Mousectl **mctl, Keyboardctl **kctl) {
	if (initdraw(nil, nil, label) < 0)
		return 1;
	if ((*mctl = initmouse(nil, screen)) == nil)
		return 1;
	if ((*kctl = initkeyboard(nil)) == nil)
		return 1;
	return 0;
}

void
drawbg(Image *walls, Image *bg) {
	draw(screen, screen->r, walls, nil, ZP);
	draw(screen, screen->r, bg, nil, ZP);
	flushimage(display, Refnone);
}

void
spawnballs(int n) {
	Channel **ticks;
	Point *pos;
	int i, j;
	BallArg *b;
	Channel ***poschans;

	if ((ticks = allocchans(n, sizeof(int), TICK_BUFSIZE)) == nil)
		sysfatal("failed to allocate frame ticker channels");

	threadcreate(frametick, ticks, mainstacksize);

	if ((pos = malloc(n*sizeof(Point))) == nil)
		sysfatal("failed to allocate position array");
	nooverlapcircles(pos, n);

	if ((poschans = malloc(n*sizeof(Channel **))) == nil)
		sysfatal("failed to allocate position channel matrix");
	for (i = 0; i < n; i++) {
		if ((poschans[i] = malloc(n*sizeof(Channel *))) == nil)
			sysfatal("failed to allocate row of position channel matrix");
		for (j = 0; j < n; j++) {
			if (j == i)
				continue;
			if ((poschans[i][j] = chancreate(sizeof(Point), POS_BUFSIZE)) == nil)
				sysfatal("failed to create position channel");
		}
	}

	for (i = 0; i < n; i++) {
		if ((b = malloc(sizeof(BallArg))) == nil)
			sysfatal("failed to allocate ball");

		b->pos = pos[i];
		b->vx = randint(-VMAX, VMAX+1);
		b->vy = randint(-VMAX, VMAX+1);
		b->col = ballcolors[randint(0, NELEMS(ballcolors))];

		b->frametick = ticks[i];

		if ((b->posin = malloc((n-1)*sizeof(Channel *))) == nil)
			sysfatal("failed to allocate array of incoming position channels");
		mcopycolskip(b->posin, poschans, n, i, i);

		if ((b->posout = malloc((n-1)*sizeof(Channel *))) == nil)
			sysfatal("failed to allocate array of outgoing position channels");
		vcopyskip(b->posout, poschans[i], n, i);

		b->nothers = n-1;

		threadcreate(ball, b, mainstacksize);
	}

	free(pos);
	for (i = 0; i < n; i++)
		free(poschans[i]);
	free(poschans);
}

Channel **
allocchans(int nchans, int elsize, int nel) {
	Channel **cs;
	int i;

	if (nchans < 0)
		return nil;

	if ((cs = malloc((nchans+1)*sizeof(Channel *))) == nil)
		return nil;
	for (i = 0; i < nchans; i++) {
		if ((cs[i] = chancreate(elsize, nel)) == nil) {
			while (i-- > 0)
				chanfree(cs[i]);
			free(cs);
			return nil;
		}
	}
	cs[nchans] = nil;
	return cs;
}

/* copy column col of nxn matrix, except for element skip, into n-1 length vec */
void
mcopycolskip(Channel *vec[], Channel **matrix[], int n, int col, int skip) {
	int i;

	for (i = 0; i < skip; i++)
		vec[i] = matrix[i][col];
	for (i = skip+1; i < n; i++)
		vec[i-1] = matrix[i][col];
}

/* copy each element in n-length src, except for element skip, into n-1 length dst */
void
vcopyskip(Channel *dst[], Channel *src[], int n,  int skip) {
	memmove(dst, src, skip*sizeof(Channel *));
	memmove(dst, src+skip+1, (n-skip-1)*sizeof(Channel *));
}

void
nooverlapcircles(Point centers[], int n) {
	int i, j;

	srand(time(0));
	for (i = 0; i < n; i++) {
		centers[i] = randptinrect(insetrect(bounds, RADIUS));
		for (j = 0; j < i; j++)
			if (isoverlapcircle(centers[j], centers[i]))
				break;
		if (j < i) { /* overlapping */
			i--;
			continue;
		}
	}
}

Point
randptinrect(Rectangle r) {
	return Pt(randint(r.min.x, r.max.x), randint(r.min.y, r.max.y));
}

int
randint(int lo, int hi) {
	return (rand() % (hi-lo)) + lo;
}

int
isoverlapcircle(Point p, Point q) {
	return dist(p, q) < 2*RADIUS;
}

int
dist(Point p, Point q) {
	Rectangle r;

	r = Rpt(p, q);
	return sqrt(Dx(r)*Dx(r) + Dy(r)*Dy(r));
}

void
ball(void *arg) {
	BallArg *b;
	Point p1, p2;
	Image *fill, *erase;
	int t, i, vsum;
	float rx, ry;
	Point otherpos, vdir;

	b = (BallArg *) arg;
	p1 = p2 = b->pos;

	fill = alloccircle(b->col, DTransparent);
	erase = alloccircle(BG, DTransparent);
	if (fill == nil ||erase == nil)
		sysfatal("failed to allocate image");

	for (;;) {
		b->vy += G;

		p2.x += b->vx;
		p2.y += b->vy;

		printf("(%d,%d) %d %d\n", b->pos.x, b->pos.y, b->vx, b->vy);

		/* check for wall collision */
		if (p2.x < bounds.min.x+RADIUS || p2.x > bounds.max.x-RADIUS) {
			p2.x = clamp(p2.x, bounds.min.x+RADIUS, bounds.max.x-RADIUS);
			printf("clamped to %d\n", b->pos.x);
			b->vx = -b->vx;
		}
		if (p2.y < bounds.min.y+RADIUS || p2.y > bounds.max.y-RADIUS) {
			p2.y = clamp(p2.y, bounds.min.y+RADIUS, bounds.max.y-RADIUS);
			b->vy = -b->vy;
		}

		broadcast(p2, b->posout, b->nothers);

		/* check for ball collision */
		for (i = 0; i < b->nothers; i++) {
			recv(b->posin[i], &otherpos);
			if (isoverlapcircle(p2, otherpos)) {
				vdir = subpt(p2, otherpos);
				rx = (float) vdir.x / (float) (vdir.x + vdir.y);
				ry = (float) vdir.y / (float) (vdir.x + vdir.y);
				vsum = b->vx + b->vy;
				b->vx = rx * (float) vsum;
				b->vy = ry * (float) vsum;
			}
		}		

		recv(b->frametick, &t);
		drawcircle(erase, p1);
		drawcircle(fill, p2);
		p1 = p2;

	}	
}

Image *
alloccircle(int fg, int bg) {
	Image *m, *fill
;
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
	draw(screen, rectaddpt(bounds, subpt(pos, Pt(RADIUS, RADIUS))), m, nil, ZP);
}

int
clamp(int v, int lo, int hi) {
	return min(hi, max(v, lo));
}

int
min(int a, int b) {
	return (a < b) ? a : b;
}

int
max(int a, int b) {
	return (a > b) ? a : b;
}

void
broadcast(Point p, Channel *cs[], int n) {
	while (n-- > 0)
		send(cs[n], &p);
}

void
frametick(void *arg) {
	Channel **cs, **c;
	vlong t1, t2;
	int s;

	cs = (Channel **) arg;

	t1 = t2 = nsec();
	for (;;) {
		while (t2 - t1 < FRAME_TIME)
			t2 = nsec();
		t1 = t2;
		for (c = cs; *c != nil; c++)
			send(*c, &s);
		flushimage(display, Refnone);
	}
}