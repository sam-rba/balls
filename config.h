/* #define WINDOWS 1 */

#define CL_TARGET_OPENCL_VERSION 110

#define WINDOW_TITLE "Balls"

#define RMIN 0.05 /* Minimum radius. */
#define RMAX 0.15 /* Maximum radius. */
#define VMAX_INIT 5.0 /* Maximum initial velocity. */

enum { FPS = 60 }; /* Frames per second. */
enum window {
	WIDTH = 640,
	HEIGHT = 640,
};
enum { KEY_QUIT = 'q' };

enum { NBALLS_DEFAULT = 3 };
enum { CIRCLE_POINTS = 24 }; /* Number of vertices per circle. */
