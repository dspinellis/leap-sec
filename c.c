#include <stdio.h>
#include <string.h>
#include <time.h>

/* Sleep for the specified number of milliseconds */
#if defined(unix)
#include <sys/select.h>

static void
milli_sleep(long msec)
{
	struct timeval tv;

	tv.tv_sec = msec / 1000L;
	tv.tv_usec = (msec % 1000L) * 1000L;
	(void)select(0, 0, 0, 0, &tv);
}
#elif defined(_WIN32)
#include <windows.h>

static void
milli_sleep(long msec)
{
	Sleep(msec);
}
#endif

/* Display the program's name */
static void
show_name(char *s)
{
	char *p;

	/* Remove leading \ path */
	if ((p = strrchr(s, '\\')) != NULL)
		s = p + 1;
	/* Remove leading / path */
	if ((p = strrchr(s, '/')) != NULL)
		s = p + 1;
	/* Remove trailing extension */
	if ((p = strrchr(s, '.')) != NULL && p != s)
		*p = 0;
	printf("%s\n", s);
}

int
main(int argc, char *argv[])
{
	char buff[100];
	time_t now;
	struct tm *t;

	for (;;) {
		time(&now);
		t = gmtime(&now);
		strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S\n", t);
		fputs(buff, stdout);
		fflush(stdout);
		milli_sleep(100);
	}
}
