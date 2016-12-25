#include <stdio.h>
#include <string.h>
#include <time.h>

#define SAMPLES_PER_SECOND 25
#define SECONDS_TO_RECORD 120

#define SAMPLES_TO_RECORD (SECONDS_TO_RECORD * SAMPLES_PER_SECOND)

/* Compatbility functions */
#if defined(unix)
#include <sys/select.h>
#include <sys/time.h>

/* Sleep for the specified number of milliseconds */
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

/* Sleep for the specified number of milliseconds */
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
	char *name;
	int i;

	name = short_name(argv[0]);
	for (i = 0; i < SAMPLES_TO_RECORD; i++) {
		char human[100], normalized[100];
		struct timeval now;
		struct tm *tm;
		time_t tnow, treverse;

		gettimeofday(&now, NULL);

		tnow = now.tv_sec;
		if ((tm = gmtime(&tnow)) != NULL)
			strftime(human, sizeof(human), "%Y-%m-%d %H:%M:%S", tm);
		else
			strcpy(human, "ERROR");

		if ((time_t)(treverse = mktime(tm)) != -1)
			strftime(normalized, sizeof(normalized),
					"%Y-%m-%d %H:%M:%S", tm);
		else
			strcpy(normalized, "ERROR");
		printf("%llu.%llu\t%s\t%s\t%llu\t%s\n",
				(unsigned long long)now.tv_sec,
				(unsigned long long)now.tv_usec, name, human,
				(unsigned long long)treverse, normalized);
		fflush(stdout);
		milli_sleep(1000 / SAMPLES_PER_SECOND);
	}
}
