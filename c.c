#include <stdio.h>
#include <string.h>
#include <time.h>

#define SAMPLES_PER_SECOND 25
#define SECONDS_TO_RECORD 120

#define SAMPLES_TO_RECORD (SECONDS_TO_RECORD * SAMPLES_PER_SECOND)

/* Compatbility functions */
#if defined(unix) || defined(__MACH__)
#include <err.h>
#include <sys/select.h>
#include <sys/time.h>

/* Sleep for the specified number of milliseconds */
static void
milli_sleep(unsigned long msec)
{
	struct timeval tv;

	tv.tv_sec = msec / 1000L;
	tv.tv_usec = (msec % 1000L) * 1000L;
	(void)select(0, 0, 0, 0, &tv);
}
#endif

#if defined(__MACH__)
/* Based on https://developer.apple.com/library/content/qa/qa1398/_index.html */
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>

static unsigned long long
milli_counter(void)
{
	uint64_t t;
	static mach_timebase_info_data_t sTimebaseInfo;
	Nanoseconds nano;

	t = mach_absolute_time();
	// If this is the first time we've run, get the timebase.
	// We can use denom == 0 to indicate that sTimebaseInfo is
	// uninitialised because it makes no sense to have a zero
	// denominator is a fraction.

	if (sTimebaseInfo.denom == 0)
		(void)mach_timebase_info(&sTimebaseInfo);

	// Do the maths. We hope that the multiplication doesn't
	// overflow; the price you pay for working in fixed point.
	return t / 1000000LLU * sTimebaseInfo.numer / sTimebaseInfo.denom;
}
#endif

#if defined(unix)
static unsigned long long
milli_counter(void)
{
	struct timespec ts;

#if defined(CLOCK_MONOTONIC_RAW)
	if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts))
#elif defined(CLOCK_MONOTONIC_PRECISE)
	if (clock_gettime(CLOCK_MONOTONIC_PRECISE, &ts))
#else
	if (clock_gettime(CLOCK_MONOTONIC, &ts))
#endif
		err(1, "clock_kettime");
	return (unsigned long long)ts.tv_sec * 1000 +
		(unsigned long long)ts.tv_nsec / 1000000ULL;
}
#endif

#if defined(_WIN32)
#include <windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64

/* Sleep for the specified number of milliseconds */
static void
milli_sleep(long msec)
{
	Sleep(msec);
}

static unsigned long long
milli_counter(void)
{
	return GetTickCount64();
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
	struct timeval now;
	time_t tnow;
	struct tm *tm;
	unsigned long long mono_start, mono_now;
	unsigned long to_sleep;

	/* Wait for an integral minute to start */
	/* First sleep until close to a second before the minute */
	time(&tnow);
	tm = gmtime(&tnow);
	to_sleep = (60 - tm->tm_sec - 1) * 1000;
	fprintf(stderr, "Sleeping for %lu seconds\n", to_sleep / 1000);
	fflush(stderr);
	milli_sleep(to_sleep);
	/* Then busy loop until the seconds become zero */
	do {
		time(&tnow);
		tm = gmtime(&tnow);
	} while (tm->tm_sec != 0);
	mono_start = milli_counter();

	name = short_name(argv[0]);
	for (i = 0; i < SAMPLES_TO_RECORD; i++) {
		char human[100];

		gettimeofday(&now, NULL);
		tnow = now.tv_sec;
		if ((tm = gmtime(&tnow)) != NULL)
			strftime(human, sizeof(human), "%Y-%m-%d %H:%M:%S", tm);
		else
			strcpy(human, "ERROR");

		printf("%.3f %llu.%llu\t%s\t%s\n",
				(milli_counter() - mono_start) / 1000.,
				(unsigned long long)now.tv_sec,
				(unsigned long long)now.tv_usec, name, human);
		fflush(stdout);
		milli_sleep(1000 / SAMPLES_PER_SECOND);
	}
}
