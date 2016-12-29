/*-
 * Create a time advancement log
 *
 * Copyright 2016 Diomidis Spinellis
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#define FRAC       4294967296.             /* NTP fraction: 2^32 as a double */

/* Record two minutes in real time */
#define SAMPLES_PER_SECOND 25
#define MILLISECONDS_TO_RECORD (2 * 60 * 1000)

/* Compatbility functions */
#if defined(unix) || defined(__MACH__)
#include <err.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

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

static void
err(int ret, const char *msg)
{
	fprintf(stderr, "%s: %s\n", strerror(errno));
	exit(1);
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

/* Display the current timestamp on standard output */
static void
time_stamp(const char *name, double si_seconds)
{
	char human[100];
	struct tm *tm;
	struct timeval now;
	time_t tnow;
	unsigned long long mono_now;

	gettimeofday(&now, NULL);
	tnow = now.tv_sec;
	if ((tm = gmtime(&tnow)) != NULL)
		strftime(human, sizeof(human), "%Y-%m-%d %H:%M:%S", tm);
	else
		strcpy(human, "ERROR");

	printf("%.3f\t%llu.%03u\t%s\t%s\n", si_seconds,
			(unsigned long long)now.tv_sec,
			(unsigned int)floor((double)now.tv_usec / 1000),
			name, human);
}

static void
fast_log(const char *name)
{
	int i;
	time_t tnow;
	struct tm *tm;
	unsigned long long mono_start;
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
	/* Fast recording of many hours */
	do {
		time_stamp(name, (milli_counter() - mono_start) / 1000);
		milli_sleep(1000 / SAMPLES_PER_SECOND);
	} while (milli_counter() - mono_start < MILLISECONDS_TO_RECORD);
}

/*
 * This code will query an NTP server for the local time and display
 * it.
 * It is derived from public domain code written
 * by Tim Hogard (thogard@abnormal.com)  in Perl Thu Sep 26 13:35:41 EAST 2002
 * and converted to C Fri Feb 21 21:42:49 EAST 2003
 * The original code can be found at http://www.abnormal.com/~thogard/ntp/
 */
ntp_log(const char *name, const char *hostname)
{
	int portno = 123;	//NTP is port 123
	int i;			// misc var i
	unsigned char msg[48] = { 010, 0, 0, 0, 0, 0, 0, 0, 0 };	// the packet we send
	uint32_t buf[1024];	// the buffer we get back
	struct protoent *proto;	//
	struct sockaddr_in server_addr;
	int s;			// socket
	uint32_t tmit;		// the time -- This is a time_t sort of
	uint32_t tfrac;
	struct hostent *h;
	struct timeval now;

	proto = getprotobyname("udp");
	if ((s = socket(PF_INET, SOCK_DGRAM, proto->p_proto)) < 0)
		err(1, "socket");

	if ((h = gethostbyname(hostname)) == NULL)
		err(1, "gethostbyname");
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr, h->h_addr_list[0],
	       sizeof(server_addr.sin_addr));
	server_addr.sin_port = htons(portno);

	/*
	 * build a message.  Our message is all zeros except for a one in the
	 * protocol version field
	 * msg[] in binary is 00 001 000 00000000 
	 * it should be a total of 48 bytes long
	 */

	// send the data
	i = sendto(s, msg, sizeof(msg), 0, (struct sockaddr *)&server_addr,
		   sizeof(server_addr));

	// get the data back
	i = recv(s, (char *)buf, sizeof(buf), 0);

	/*
	 * The high word of transmit time is the 10th word we get back
	 * tmit is the time in seconds not accounting for network delays which
	 * should be way less than a second if this is a local NTP server
	 */

	tmit = ntohl((uint32_t) buf[10]);	//# get transmit time
	tfrac = ntohl((uint32_t) buf[11]);	//# get transmit time
	//printf("tmit=%d\n",tmit);

	/*
	 * Convert time to unix standard time NTP is number of seconds since 0000
	 * UT on 1 January 1900 unix time is seconds since 0000 UT on 1 January
	 * 1970 There has been a trend to add a 2 leap seconds every 3 years.
	 * Leap seconds are only an issue the last second of the month in June and
	 * December if you don't try to set the clock then it can be ignored but
	 * this is importaint to people who coordinate times with GPS clock sources.
	 */

	tmit -= 2208988800U;
	time_stamp(name, tmit + (double)tfrac / FRAC);
}

int
main(int argc, char *argv[])
{
	if (argc == 1)
		fast_log(short_name(argv[0]));
	else if (argc == 2)
		ntp_log(short_name(argv[0]), argv[1]);
	else {
		fprintf(stderr, "Usage: %s [ntp_server]\n", argv[0]);
		exit(1);
	}
}
