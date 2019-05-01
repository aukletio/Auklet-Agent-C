/* needs unistd.h */

/* auklet_send sends the given data to Auklet.
 * data must point to a valid JSON message.
 * If the data could not be sent to the Client,
 * -1 is returned and errno is set; otherwise,
 * the number of bytes written is returned. */
int auklet_send(const char *data, size_t size);