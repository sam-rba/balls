#include "balls.h"

#include <stdio.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>

#define NELEMS(arr) (sizeof(arr) / sizeof(arr[0]))

#define G 0.25

enum {
	BG = DWhite,
	WALLS = DBlack,

	PAD = 0,
	WIDTH = 960,
	HEIGHT=640,

	KEY_QUIT = 'q' ,

	DEFAULT_NBALLS = 3,

	VMAX = 3,
	MASS = 10,

	FPS = 60,
	NS_PER_SEC = 1000000000,
	FRAME_TIME = NS_PER_SEC / FPS,
	TICK_BUFSIZE = 4,

	POS_BUFSIZE = 4,
};

typedef struct {
	Ball b;

	int color;

	Channel *frametick;

	Channel **in; /* <-chan Ball */
	Channel **out; /* chan<- Ball */
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
	int nballs;
	Image *bg, *walls;
	Mousectl *mctl;
	Keyboardctl *kctl;
	int resize[2];
	Rune key;

	nballs = DEFAULT_NBALLS;
	if (argc > 1) {
		if (sscanf(argv[1], "%d", &nballs) != 1 || nballs < 1) {
			printf("usage: balls [number of balls]\n");
			threadexitsall(0);
		}
	}
	printf("nballs: %ud\n", nballs);

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

	spawnballs(nballs);

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
	Point *ps;
	int i, j;
	BallArg *arg;
	Channel ***cs;

	if ((ticks = allocchans(n, sizeof(int), TICK_BUFSIZE)) == nil)
		sysfatal("failed to allocate frame ticker channels");

	threadcreate(frametick, ticks, mainstacksize);

	if ((ps = malloc(n*sizeof(Point))) == nil)
		sysfatal("failed to allocate position array");
	nooverlapcircles(ps, n);

	if ((cs = malloc(n*sizeof(Channel **))) == nil)
		sysfatal("failed to allocate channel matrix");
	for (i = 0; i < n; i++) {
		if ((cs[i] = malloc(n*sizeof(Channel *))) == nil)
			sysfatal("failed to allocate row of channel matrix");
		for (j = 0; j < n; j++) {
			if (j == i)
				continue;
			if ((cs[i][j] = chancreate(sizeof(Ball), POS_BUFSIZE)) == nil)
				sysfatal("failed to create channel");
		}
	}

	for (i = 0; i < n; i++) {
		if ((arg = malloc(sizeof(BallArg))) == nil)
			sysfatal("failed to allocate ball");

		arg->b.p = ps[i];
		arg->b.v = V((double) randint(-VMAX, VMAX+1), (double) randint(-VMAX, VMAX+1));
		arg->b.m = MASS;

		arg->color = ballcolors[randint(0, NELEMS(ballcolors))];

		arg->frametick = ticks[i];

		if ((arg->in = malloc((n-1)*sizeof(Channel *))) == nil)
			sysfatal("failed to allocate array of incoming channels");
		mcopycolskip(arg->in, cs, n, i, i);

		if ((arg->out = malloc((n-1)*sizeof(Channel *))) == nil)
			sysfatal("failed to allocate array of outgoing channels");
		vcopyskip(arg->out, cs[i], n, i);

		arg->nothers = n-1;

		threadcreate(ball, arg, mainstacksize);
	}

	free(ps);
	for (i = 0; i < n; i++)
		free(cs[i]);
	free(cs);
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
	int i;

	for (i = 0; i < skip; i++)
		dst[i] = src[i];
	for (i = skip+1; i < n; i++)
		dst[i-1] = src[i];
}

void
nooverlapcircles(Point centers[], int n) {
	int i, j;

	srand(time(0));
	for (i = 0; i < n; i++) {
		centers[i] = randptinrect(insetrect(bounds, RADIUS));
		for (j = 0; j < i; j++)
			if (iscollision(centers[j], centers[i]))
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

void
ball(void *arg) {
	BallArg *barg;
	Point p, oldp;
	Vec v;
	int m;
	Image *fill, *erase;
	int i;
	Ball other;
	Point midpoint;
	Vec d, n;
	double magnitude;
	int t;

	barg = (BallArg *) arg;
	p = oldp = barg->b.p;
	v = barg->b.v;
	m = barg->b.m;

	fill = alloccircle(barg->color, DTransparent);
	erase = alloccircle(BG, DTransparent);
	if (fill == nil ||erase == nil)
		sysfatal("failed to allocate image");

	for (;;) {
		v.y += G;

		p = ptaddv(p, v);

		printf("(%d,%d) %f %f\n", p.x, p.y, v.x, v.y);

		broadcast(p, barg->out, barg->nothers);

		/* check for ball collision */
		for (i = 0; i < barg->nothers; i++) {
			recv(barg->in[i], &other);
			if (iscollision(p, other.p)) {
				midpoint = divpt(addpt(p, other.p), 2);
				d = Vpt(other.p, p);
				p = ptaddv(midpoint, vmuls(unitnorm(d), RADIUS));

				printf("collision (%d,%d), (%d,%d)\n", p.x, p.y, other.p.x, other.p.y);

				printf("oldv: (%2.2f,%2.2f\n", v.x, v.y);

				n = unitnorm(Vpt(other.p, p));
				magnitude = 2*(vdot(v, n) - vdot(other.v, n)) / (m + other.m);

				printf("n: (%2.2f,%2.2f), magnitude: %2.2f\n", n.x, n.y, magnitude);

				v = vsub(v, vmuls(n, magnitude * m));

				printf("newv: (%2.2f,%2.2f)\n", v.x, v.y);
			}
		}		

		/* check for wall collision */
		if (p.x < bounds.min.x+RADIUS || p.x > bounds.max.x-RADIUS) {
			p.x = clamp(p.x, bounds.min.x+RADIUS, bounds.max.x-RADIUS);
			printf("clamped to %d\n", p.x);
			v.x = -v.x;
		}
		if (p.y < bounds.min.y+RADIUS || p.y > bounds.max.y-RADIUS) {
			p.y = clamp(p.y, bounds.min.y+RADIUS, bounds.max.y-RADIUS);
			v.y = -v.y;
		}		
		recv(barg->frametick, &t);
		drawcircle(erase, oldp);
		drawcircle(fill, p);
		oldp = p;

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