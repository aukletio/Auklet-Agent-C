/* Module server provides a server to handle profile tree emission requests. */
/* needs pthread.h */

typedef struct Server Server;

Server *newServer(int fd, void (*handler)());
int start(Server *s);
int wait(Server *s, int kill);
