#include "server.c"

#include <sys/socket.h>
#include <stdio.h>

#define len(x) (sizeof(x)/sizeof(x[0]))

int count = 0;

void handler() { count++; }

int
test_server()
{
	int fd[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == -1) {
		perror("socketpair");
		return 0;
	}
	
	Server server = {
		.fd = fd[0],
		.handler = handler,
	};
	start(&server);
	for (int i = 0; i < 42; i++) {
		char buf;
		if (write(fd[1], &buf, sizeof(buf)) == -1) {
			perror("write");
			return 0;
		}
	}
	/* This is a hack to make sure all of the bytes are received by the 
	 * time this thread tries to check the count. The correct way would
	 * involve some kind of inter-thread synchronization. */
	sleep(1);
	stop(&server);
	if (count != 42) {
		printf("request count: expected %d, got %d\n", 42, count);
		return 0;
	}
	return 1;
}

int
main()
{
	int (*tests[])() = {
		test_server,
	};

	int fails = 0;
	for (int i = 0; i < len(tests); i++)
		if (!tests[i]())
			fails++;

	return fails;
}
