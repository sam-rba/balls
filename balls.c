#include "balls.h"

#include <thread.h>
#include <mouse.h>
#include <keyboard.h>

#define NELEMS(arr) (sizeof(arr) / sizeof(arr[0]))

#define G_FACTOR 15.0
#define G (9.81/G_FACTOR)

enum {
	BG = DWhite,
	WALLS = DBlack,

	WIDTH = 1280,
	HEIGHT=720,

	KEY_QUIT = 'q' ,

	DEFAULT_NBALLS = 3,

	VMAX = 3,
	MASS_FACTOR = 75,
	G_PER_KG = 1000,

	FPS = 60,
	NS_PER_SEC = 1000000000,
	FRAME_TIME = NS_PER_SEC / FPS,
	TICK_BUFSIZE = 1,

	BALL_BUFSIZE = 1,
};

typedef struct {
	Ball b;

	int color;

	Channel *frametick;

	/* threads communicate in a ring */
	Channel *lneighbor; /* <-chan Ball */
	Channel *rneighbor; /* chan<- Ball */
	int nothers; /* number of other balls */
} BallArg;

static int ballcolors[] = { DRed, DGreen, DBlue };
static uint radii[] = { 24, 32, 48 };
static Rectangle bounds = {{0, 0}, {WIDTH, HEIGHT}};

void spawnballs(int n);
void eventloop(Image *bg, Image *walls);
Channel **allocchans(int nchans, int elsize, int nel);
void nooverlapcircles(Point centers[], int n, uint radius);
Point randptinrect(Rectangle r);
int randint(int lo, int hi);
uint maxelem(uint arr[], uint n);
double mass(uint radius);
double area(double radius);
void ball(void *arg);
void frametick(void *arg);

void
threadmain(int argc, char *argv[]) {
	int nballs;
	Image *bg, *walls;

	nballs = DEFAULT_NBALLS;
	if (argc > 1) {
		if (sscanf(argv[1], "%d", &nballs) != 1 || nballs < 1) {
			printf("usage: balls [number of balls]\n");
			threadexitsall(0);
		}
	}
	printf("nballs: %ud\n", nballs);

	if (initdraw(nil, nil, "Balls") < 0)
		sysfatal("initdraw() failed\n");

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

	eventloop(bg, walls);
}

void
spawnballs(int n) {
	Channel **ticks;
	Point *ps;
	int i;
	BallArg *arg;
	Channel **cs;

	if ((ticks = allocchans(n, sizeof(int), TICK_BUFSIZE)) == nil)
		sysfatal("failed to allocate frame ticker channels");

	threadcreate(frametick, ticks, mainstacksize);

	if ((ps = malloc(n*sizeof(Point))) == nil)
		sysfatal("failed to allocate position array");
	nooverlapcircles(ps, n, maxelem(radii, NELEMS(radii)));

	if ((cs = malloc(n*sizeof(Channel *))) == nil)
		sysfatal("failed to allocate channel matrix");
	for (i = 0; i < n; i++) {
		if ((cs[i] = chancreate(sizeof(Ball), BALL_BUFSIZE)) == nil)
			sysfatal("failed to create channel");
	}

	for (i = 0; i < n; i++) {
		if ((arg = malloc(sizeof(BallArg))) == nil)
			sysfatal("failed to allocate ball");

		arg->b.p = ps[i];
		arg->b.v = V((double) randint(-VMAX, VMAX+1), (double) randint(-VMAX, VMAX+1));
		arg->b.r = radii[randint(0, NELEMS(radii))];
		arg->b.m = mass(arg->b.r);
		arg->color = ballcolors[randint(0, NELEMS(ballcolors))];
		arg->frametick = ticks[i];
		arg->lneighbor = cs[i];
		arg->rneighbor = cs[(i+1) % n];
		arg->nothers = n-1;

		printf("create ball p(%d,%d) v(%f,%f) r(%u) m(%f)\n", arg->b.p.x, arg->b.p.y, arg->b.v.x, arg->b.v.y, arg->b.r, arg->b.m);
		threadcreate(ball, arg, mainstacksize);
	}

	free(ps);
	free(cs);
}

void
eventloop(Image *bg, Image *walls) {
	Mousectl *mctl;
	Keyboardctl *kctl;
	int resize[2];
	Rune key;

	if ((mctl = initmouse(nil, screen)) == nil)
		sysfatal("failed to initialize mouse\n");
	if ((kctl = initkeyboard(nil)) == nil)
		sysfatal("failed to initialize keyboard\n");

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
				sysfatal("getwindow() failed\n");
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

/* allocate nil-terminated array of channels */
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

/* populate array of circle coordinates within bounds so that circles don't overlap */
void
nooverlapcircles(Point centers[], int n, uint radius) {
	int i, j;

	srand(time(0));
	for (i = 0; i < n; i++) {
		centers[i] = randptinrect(insetrect(bounds, radius));
		for (j = 0; j < i; j++)
			if (iscollision(centers[j], radius, centers[i], radius))
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

uint
maxelem(uint arr[], uint n) {
	uint max;

	if (n == 0)
		return 0;
	max = arr[--n];
	while (n-- > 0)
		if (arr[n] > max)
			max = arr[n];
	return max;
}

double
mass(uint radius) {
	return area(radius) / MASS_FACTOR / G_PER_KG;
}

double
area(double radius) {
	return M_PI * radius*radius;
}

void
ball(void *arg) {
	BallArg *barg;
	Ball b, other;
	Point oldpos;
	Image *fill, *erase;
	int i, t;

	barg = (BallArg *) arg;
	b = (Ball) barg->b;

	fill = alloccircle(barg->color, DTransparent, b.r);
	erase = alloccircle(BG, DTransparent, b.r);
	if (fill == nil ||erase == nil)
		sysfatal("failed to allocate image");

	oldpos = b.p; /* keep track of previous position so it can be erased */
	for (;;) {
		b.v.y += G;
		b.p = ptaddv(b.p, b.v);

		printf("(%d,%d) %f %f\n", b.p.x, b.p.y, b.v.x, b.v.y);

		/* check for ball collision */
		other = b; /* send our own ball first */
		for (i = 0; i < barg->nothers; i++) {
			send(barg->rneighbor, &other); /* pass balls around the ring */
			recv(barg->lneighbor, &other);
			collideball(&b, &other);
		}		

		collidewall(&b, bounds);

		recv(barg->frametick, &t); /* wait for next frame */
		drawcircle(erase, oldpos);
		drawcircle(fill, b.p);
		oldpos = b.p;
	}	
}

/* arg is nil-terminated array of chan int */
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