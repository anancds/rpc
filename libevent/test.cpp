#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <event2/event.h>
#include <time.h>

void do_timeout(evutil_socket_t fd, short event, void *arg)
{
  printf("do timeout (time: %ld)!\n", time(NULL));
  struct event_base *base = reinterpret_cast<struct event_base*>(arg);
  struct event* ev;
  struct timeval timeout;
  timeout.tv_sec = 3;
  timeout.tv_usec = 0;
//  ev = evtimer_new(base, do_timeout, base);
//  evtimer_add(ev, &timeout);
}

void create_timeout_event(struct event_base *base)
{
  struct event *ev;
  struct timeval timeout{};

//  ev = evtimer_new(base, do_timeout, base);
  ev = event_new(base, -1, EV_PERSIST, do_timeout, base);
  if (ev) {
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    event_add(ev, &timeout);
  }
}


int main(int argc, char *argv[])
{
  struct event_base *base;

  base = event_base_new();

  if (!base) {
    printf("Error: Create an event base error!\n");
    return -1;
  }

  create_timeout_event(base);
  event_base_dispatch(base);

  return 0;
}