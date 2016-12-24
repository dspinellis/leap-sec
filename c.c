#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

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
static char *
short_name(char *s)
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
	return s;
}

int
main(int argc, char *argv[])
{
	char buff[100];
	struct timeval now;
	struct tm *t;
	char *name;

	name = short_name(argv[0]);
	for (;;) {
		gettimeofday(&now, NULL);
		t = gmtime(&(now.tv_sec));
		strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", t);
		printf("%llu.%llu\t%s\t%s\n", (unsigned long long)now.tv_sec,
				(unsigned long long)now.tv_usec, name, buff);
		fflush(stdout);
		milli_sleep(100);
	}
}
