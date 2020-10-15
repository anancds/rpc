//
// Created by cds on 2020/10/15.
//

namespace mindspore {
namespace ps {
namespace comm {

server::server()
  :base(nullptr)
  ,signal_event(nullptr)
  ,listener(nullptr)
{
}


server::~server()
{
  if(signal_event != nullptr)
  {
    event_free(signal_event);
    signal_event = nullptr;
  }

  if(listener != nullptr)
  {
    evconnlistener_free(listener);
    listener = nullptr;
  }

  if(base != nullptr)
  {
    event_base_free(base);
    base = nullptr;
  }
}

//void server::set_callbacks(on_connected client_conn, on_disconnected client_disconn, on_accepted client_accept, on_received client_recv)
//{
//    this->client_conn = client_conn;
//    this->client_disconn = client_disconn;
//    this->client_accept = client_accept;
//    this->client_recv = client_recv;
//}

void server::setup(const unsigned short& port)
{
  base = event_base_new();
  if(!base)
    throw error(error_base_failed);

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);

  listener = evconnlistener_new_bind(
    base
    ,server::listenerCallback
    ,reinterpret_cast<void*>(this)
    ,LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE
    ,-1
    ,reinterpret_cast<struct sockaddr*>(&sin)
    ,sizeof(sin)
  );

  if(!listener)
    throw error(errno);

  /*signal_event = evsignal_new(base, SIGINT, signalCallback, reinterpret_cast<void*>(this));
  if(!signal_event || event_add(signal_event, nullptr) < 0) {

      printf("Cannog create signal event.\n");
      return false;
  }*/
}

void server::update()
{
  std::unique_lock<std::recursive_mutex> l(mConnectionsMutex);
  if(base != nullptr)
  {
    event_base_loop(base, EVLOOP_NONBLOCK);
  }
}

void server::addConnection(evutil_socket_t fd, connection* connection)
{
  std::unique_lock<std::recursive_mutex> l(mConnectionsMutex);
  connections.insert(std::pair<evutil_socket_t, class connection*>(fd, connection));
}

void server::removeConnection(evutil_socket_t fd)
{
  std::unique_lock<std::recursive_mutex> l(mConnectionsMutex);
  connections.erase(fd);
}

void server::sendToAllClients(const char* data, size_t len)
{
  std::unique_lock<std::recursive_mutex> l(mConnectionsMutex);

  typename std::map<evutil_socket_t, connection*>::iterator it = connections.begin();
  while(it != connections.end())
  {
    it->second->send(data, len);
    ++it;
  }
}

// ------------------------------------

void server::listenerCallback(
  struct evconnlistener* listener
  ,evutil_socket_t fd
  ,struct sockaddr* saddr
  ,int socklen
  ,void* data
)
{
  server* server = reinterpret_cast<class server*>(data);
  struct event_base* base = reinterpret_cast<struct event_base*>(server->base);
  struct bufferevent* bev;

  bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  if(!bev) {
    event_base_loopbreak(base);
    printf("Error constructing bufferevent!\n");
    return;
  }

  connection* conn = server->on_create_conn();
  if (!conn)
  {
    printf("Error creation of connection object.");
    return;
  }

  conn->setup(fd, bev, server);

  server->addConnection(fd, conn);

  bufferevent_setcb(bev, server::readCallback, server::writeCallback, server::eventCallback,
                    reinterpret_cast<void*>(conn));
  bufferevent_enable(bev, EV_WRITE);
  bufferevent_enable(bev, EV_READ);
}

connection* server::on_create_conn()
{
  connection* conn = nullptr;
  if (client_accept)
    conn = client_accept(this);
  else
    conn = new connection();

  return conn;
}

void server::signalCallback(evutil_socket_t sig, short events, void* data)
{
  server* server = reinterpret_cast<class server*>(data);
  struct event_base* base = server->base;
  struct timeval delay = {0,0};
  // Caught an interrupt signal; exiting cleanly
  event_base_loopexit(base, &delay);
  // Exited
}

void server::writeCallback(struct bufferevent* bev, void* data)
{
  struct evbuffer* output = bufferevent_get_output(bev);
  if(evbuffer_get_length(output) == 0)
  {

  }
}

void server::readCallback(struct bufferevent* bev, void* connection)
{
  class connection* conn = static_cast<class connection*>(connection);
  struct evbuffer* buf = bufferevent_get_input(bev);
  char readbuf[1024];
  int read = 0;

  while( (read = evbuffer_remove(buf, &readbuf, sizeof(readbuf))) > 0)
  {
    conn->on_read_handler(readbuf, static_cast<size_t>(read));
  }
}

void server::eventCallback(struct bufferevent* bev, short events, void* data)
{
  connection* conn = reinterpret_cast<connection*>(data);
  server* srv = conn->mServer;

  if(events & BEV_EVENT_EOF)
  {
    // Notify about disconnection
    if (srv->client_disconn)
      srv->client_disconn(conn->mServer, conn);

    // Free connection structures
    conn->mServer->removeConnection(conn->mFd);
    bufferevent_free(bev);

  }
  else if(events & BEV_EVENT_ERROR)
  {
    // Free connection structures
    conn->mServer->removeConnection(conn->mFd);
    bufferevent_free(bev);

    // Notify about disconnection
    if (srv->client_disconn)
      srv->client_disconn(conn->mServer, conn);

  }
  else
  {
    printf("unhandled.\n");
  }
}
}
}  // namespace ps
}  // namespace mindspore
