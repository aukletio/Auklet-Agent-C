#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>

#define len(x) (sizeof(x)/sizeof(x[0]))

void f() { for (int i = 0; i < 1000000; i++); }
void g() { f(); }
void h() { f(); g(); }
void i() { f(); h(); h(); }

void *
thread(void *p)
{
	void (*action[])() = { f, g, h, i };
	for (int i = 0; i < 50; i++)
		action[i%len(action)]();
	return NULL;
}

int
main()
{
	pthread_t t[2];
	for (int i = 0; i < len(t); i++)
		pthread_create(&t[i], NULL, thread, NULL);

	for (int i = 0; i < len(t); i++)
		pthread_join(t[i], NULL);
	return 0;
}
