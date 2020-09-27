//
// Created by cds on 2020/9/27.
//

#include <sys/types.h>

#include <event2/event-config.h>

#include <sys/stat.h>
#ifndef _WIN32
#include <sys/queue.h>
#include <sys/time.h>
#include <unistd.h>
#else
#include <windows.h>
#include <winsock2.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <event2/event.h>

int called = 0;

static void signal_cb(evutil_socket_t fd, short event, void *arg) {
  struct event *signal = (struct event*)arg;

  printf("signal_cb: got signal %d\n", event_get_signal(signal));

  if (called >= 2) event_del(signal);

  called++;
}

int main(int argc, char **argv) {
  struct event *signal_int = NULL;
  struct event_base *base;
  int ret = 0;
#ifdef _WIN32
  WORD wVersionRequested;
  WSADATA wsaData;

  wVersionRequested = MAKEWORD(2, 2);

  (void)WSAStartup(wVersionRequested, &wsaData);
#endif

  /* Initialize the event library */
  base = event_base_new();
  if (!base) {
    ret = 1;
    goto out;
  }

  /* Initialize one event */
  signal_int = evsignal_new(base, SIGINT, signal_cb, event_self_cbarg());
  if (!signal_int) {
    ret = 2;
    goto out;
  }
  event_add(signal_int, NULL);

  event_base_dispatch(base);

out:
  if (signal_int) event_free(signal_int);
  if (base) event_base_free(base);
  return ret;
}