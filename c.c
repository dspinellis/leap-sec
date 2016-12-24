#include <stdio.h>
#include <string.h>
#include <time.h>

/* Sleep for the specified number of milliseconds */
#if defined(unix)
#include <sys/select.h>
#include <sys/time.h>

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
#include <stdint.h> // portable: uint64_t   MSVC: __int64

static void
milli_sleep(long msec)
{
	Sleep(msec);
}

/* From http://stackoverflow.com/a/26085827/20520 */
static int
gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time =  ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)((time - EPOCH) % 10000000L);
	return 0;
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
	time_t tnow;
	char *name;

	name = short_name(argv[0]);
	for (;;) {
		gettimeofday(&now, NULL);
		tnow = now.tv_sec;
		t = gmtime(&tnow);
		strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", t);
		printf("%llu.%llu\t%s\t%s\n", (unsigned long long)now.tv_sec,
				(unsigned long long)now.tv_usec, name, buff);
		fflush(stdout);
		milli_sleep(100);
	}
}
