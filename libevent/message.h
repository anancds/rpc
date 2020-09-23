//
// Created by cds on 2020/9/23.
//

#ifndef RPC_MESSAGE_H
#define RPC_MESSAGE_H

#include <string>
using namespace std;

typedef struct msg_header {
  unsigned int targetID;
  unsigned int sourceID;
  unsigned int length;
} Header;

void listener_cb(struct evconnlistener *, evutil_socket_t,
                 struct sockaddr *, int socklen, void *);
void conn_writecb(struct bufferevent *, void *);
void conn_readcb(struct bufferevent *, void *);
void conn_eventcb(struct bufferevent *, short, void *);
unsigned int get_client_id(struct bufferevent*);
string inttostr(int);
void write_buffer(string&, struct bufferevent*, Header&);

#endif  // RPC_MESSAGE_H
