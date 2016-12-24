#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>

/* Output the collected records every so many microseconds */
static struct timeval interval = {0, 100000};

int
main(int argc, char *argv[])
{
	int nfiles = argc - 1;
	struct file_data {
		char *name;
		int fd;
		char buff[80];
	} *files;
	struct iovec *iov;
	int i;

	files = malloc(nfiles * sizeof(struct file_data));
	iov = malloc(nfiles * 2 * sizeof(struct iovec));
	for (i = 0; i < nfiles; i++) {
		files[i].name = argv[i + 1];
		if ((files[i].fd = open(argv[i + 1], O_RDONLY)) < 0)
			err(1, files[i].name);
		iov[i * 2].iov_base = files[i].buff;
		iov[i * 2].iov_len = 0;
		iov[i * 2 + 1].iov_base = "\t";
		iov[i * 2 + 1].iov_len = 1;
	}
	iov[nfiles * 2 - 1].iov_base = "\n";


	/* Loop reading values and outputting snapshot. */
	for (;;) {
		struct timeval start, now, elapsed;

		fd_set source_fds;
		FD_ZERO(&source_fds);
		for (i = 0; i < nfiles; i++)
			FD_SET(files[i].fd, &source_fds);

		gettimeofday(&start, NULL);

		/*
		 * Read all available data until the end of the specified
		 * interval.
		 * The required select(2) behavior depends on select
		 * subtracting the time not slept.
		 * This is supported on Linux.
		 */
		do {
			if (select(files[nfiles - 1].fd + 1, &source_fds,
						0, 0, 0) < 0)
				err(1, "select");
			for (i = 0; i < nfiles; i++)
				if (FD_ISSET(files[i].fd, &source_fds)) {
					if ((iov[i * 2].iov_len =
					    read(files[i].fd, files[i].buff,
					    sizeof(files[i].buff))) < 0)
						err(1, "read %s", files[i].name);
					/* Remove terminating newline */
					iov[i * 2].iov_len--;
				}
			gettimeofday(&now, NULL);
			timersub(&now, &start, &elapsed);
		} while (timercmp(&elapsed, &interval, <));

		/* Output the available records. */
		writev(1, iov, nfiles * 2);
	}
	/* NOTREACHED */
}
