#include "server.c"

#include <sys/socket.h>
#include <stdio.h>

#define len(x) (sizeof(x)/sizeof(x[0]))

int count = 0;

void handler() { count++; }

int
test_server()
{
	int ret = 1;
	int fd[2];
	char buf = '@';

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == -1) {
		perror("socketpair");
		return 0;
	}
	
	Server *server = newServer(fd[0], handler, malloc);
	start(server);
	for (int i = 0; i < 42; i++) {
		if (write(fd[1], &buf, sizeof(buf)) == -1) {
			perror("write");
			ret = 0;
			goto cleanup;
		}
	}
	close(fd[1]);
	wait(server, 0);
	if (count != 42) {
		printf("request count: expected %d, got %d\n", 42, count);
		ret = 0;
		goto cleanup;
	}

cleanup:
	free(server);
	return ret;
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
