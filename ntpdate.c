/*
 * This code will query a ntp server for the local time and display
 * it.  it is intended to show how to use a NTP server as a time
 * source for a simple network connected device.
 * This is the C version.  The orignal was in Perl
 *
 * For better clock management see the offical NTP info at:
 * http://www.eecis.udel.edu/~ntp/
 *
 * written by Tim Hogard (thogard@abnormal.com)
 * Thu Sep 26 13:35:41 EAST 2002
 * Converted to C Fri Feb 21 21:42:49 EAST 2003
 * this code is in the public domain.
 * it can be found here http://www.abnormal.com/~thogard/ntp/
 *
 */

#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
	char *hostname = argv[1];
	int portno = 123;	//NTP is port 123
	int maxlen = 1024;	//check our buffers
	int i;			// misc var i
	unsigned char msg[48] = { 010, 0, 0, 0, 0, 0, 0, 0, 0 };	// the packet we send
	uint32_t buf[maxlen];	// the buffer we get back
	//struct in_addr ipaddr;                //      
	struct protoent *proto;	//
	struct sockaddr_in server_addr;
	int s;			// socket
	int tmit;		// the time -- This is a time_t sort of
	struct hostent *h;

	//use Socket;
	//
	//#we use the system call to open a UDP socket
	//socket(SOCKET, PF_INET, SOCK_DGRAM, getprotobyname("udp")) or die "socket: $!";
	proto = getprotobyname("udp");
	if ((s = socket(PF_INET, SOCK_DGRAM, proto->p_proto)) < 0)
		err(1, "socket");

	if ((h = gethostbyname(argv[1])) == NULL)
		err(1, "gethostbyname %s", argv[1]);
	//#convert hostname to ipaddress if needed
	//$ipaddr   = inet_aton($HOSTNAME);
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr, h->h_addr_list[0],
	       sizeof(server_addr.sin_addr));
	server_addr.sin_port = htons(portno);
	//printf("ipaddr (in hex): %x\n",server_addr.sin_addr);

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
	i = recv(s, buf, sizeof(buf), 0);
	//printf("recvfr: %d\n",i);
	//perror("recvfr:");

	//We get 12 long words back in Network order
	/*
	   for(i=0;i<12;i++)
	   printf("%d\t%-8x\n",i,ntohl(buf[i]));
	 */

	/*
	 * The high word of transmit time is the 10th word we get back
	 * tmit is the time in seconds not accounting for network delays which
	 * should be way less than a second if this is a local NTP server
	 */

	tmit = ntohl((time_t) buf[10]);	//# get transmit time
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
	//printf("tmit=%d\n",tmit);
	/* use unix library function to show me the local time (it takes care
	 * of timezone issues for both north and south of the equator and places
	 * that do Summer time/ Daylight savings time.
	 */

	//#compare to system time
	printf("Time: %s", ctime((time_t *) & tmit));
	i = time(0);
	//printf("%d-%d=%d\n",i,tmit,i-tmit);
	printf("System time is %d seconds off\n", i - tmit);
}
