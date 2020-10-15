/*
Copyright (c) 2020, SEVANA OÜ
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. All advertising materials mentioning features or use of this software
   must display the following acknowledgement:
   This product includes software developed by the <organization>.
4. Neither the name of the <organization> nor the
   names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

extern "C" {
    #include <sys/socket.h>
    #include <event2/bufferevent.h>
    #include <event2/buffer.h>
    #include <event2/listener.h>
    #include <event2/util.h>
    #include <event2/event.h>
}

#include "proto_server.h"
#include "proto_utils.h"
#include <memory.h>
#include <signal.h>
#include <assert.h>

using namespace proto;

connection::connection()
{

}

connection::~connection()
{}

void connection::setup(int fd, struct bufferevent *bev, server *srv)
{
    mBufferEvent = bev;
    mFd = fd;
    mServer = srv;
}

void connection::send(const void* data, size_t numBytes)
{
    if(bufferevent_write(mBufferEvent, data, numBytes) == -1)
        throw error(errno);
}

void connection::on_read_handler(const void *buffer, size_t numBytes)
{
    if (mServer->client_recv)
        mServer->client_recv(mServer, this, buffer, numBytes);
}

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

// ------------ msgconnection ------------

msgconnection::msgconnection()
{
}

msgconnection::~msgconnection()
{}

void msgconnection::setup(evutil_socket_t fd, struct bufferevent* bev, server* srv)
{
    connection::setup(fd, bev, srv);

    msgserver* ms = dynamic_cast<msgserver*>(srv);
    if (ms)
    {
        mParser.set_callback([this, ms](const void* buf, size_t num)
        {
            if (ms->mMessageCb)
                ms->mMessageCb(*ms, *this, buf, num);
        });
    }
}

void msgconnection::on_read_handler(const void* buffer, size_t num)
{
    mParser.parse(buffer, num);
}

void msgconnection::send_msg(const void* buffer, size_t num)
{
    assert(sizeof(msgparser::header) == 8);
    msgparser::header hdr;

    hdr.mMagic = htonl(msgparser::MAGIC);
    hdr.mLength = htonl(static_cast<uint32_t>(num));
    //printf("Num of bytes: %d\n", static_cast<uint32_t>(num));
    send(&hdr, sizeof(hdr));
    send(buffer, num);
}

// ------------ msgserver ----------------
msgserver::msgserver()
{}

msgserver::~msgserver()
{}

void msgserver::set_msg_callback(on_message cb)
{
    mMessageCb = cb;
}

void msgserver::send_msg(connection& conn, const void* data, size_t num)
{
    msgconnection& mc = dynamic_cast<msgconnection&>(conn);
    mc.send_msg(data, num);
}

void msgserver::send_msg(const void* data, size_t num)
{
    std::unique_lock<std::recursive_mutex> l(mConnectionsMutex);

    typename std::map<evutil_socket_t, connection*>::iterator it = connections.begin();
    while(it != connections.end())
    {
        send_msg(*it->second, data, num);
        ++it;
    }
}

connection* msgserver::on_create_conn()
{
    connection* result = nullptr;
    if (client_accept)
        result = client_accept(this);
    else
        result = new msgconnection();

    return result;
}
