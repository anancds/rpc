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

#include <iostream>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "proto_client.h"
#include "proto_utils.h"

namespace proto {

client::client()
    :mBase(nullptr), mTimeoutEvent(nullptr), mBufferEvent(nullptr), mBufferEvent1(nullptr)
{
}

client::~client()
{
    stop();
}

void client::set_target(const std::string& target)
{
    mTarget = target;
}

std::string client::get_target() const
{
    return mTarget;
}

void client::set_callbacks(on_connected conn, on_disconnected disconn,
                           on_read read, on_timeout timeout)
{
    mConnectedCb =      conn;
    mDisconnectedCb =   disconn;
    mReadCb =           read;
    mTimeoutCb =        timeout;
}


void client::start()
{
    if (mBufferEvent)
        return;

    int retcode = 0;

    mBase = event_base_new();

    // Find IP address
    uint16_t port = static_cast<uint16_t>(DEFAULT_PORT);
    std::string::size_type p = mTarget.find(':');
    if (p != std::string::npos)
        port = static_cast<uint16_t>(std::atoi(mTarget.c_str() + p + 1));

    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    if (evutil_inet_pton(AF_INET, mTarget.c_str(), &sin.sin_addr.s_addr) != 0)
        throw error(errno);
    sin.sin_port = htons(port);

    // Prepare bufferevent
    mBufferEvent = bufferevent_socket_new(mBase, -1, BEV_OPT_CLOSE_ON_FREE);

    // No write callback for now
    bufferevent_setcb(mBufferEvent, readcb, nullptr, eventcb, this);
    bufferevent_enable(mBufferEvent, EV_READ | EV_WRITE);

    // evbuffer_add(bufferevent_get_output(bev), message, block_size);

    retcode = bufferevent_socket_connect(mBufferEvent, reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin));
    if (retcode < 0)
        throw error(retcode);
}

void client::start1()
{
  if (mBufferEvent1)
    return;

  int retcode = 0;

  // Find IP address
  uint16_t port = static_cast<uint16_t>(DEFAULT_PORT);
  std::string::size_type p = mTarget.find(':');
  if (p != std::string::npos)
    port = static_cast<uint16_t>(std::atoi(mTarget.c_str() + p + 1));

  sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  if (evutil_inet_pton(AF_INET, mTarget.c_str(), &sin.sin_addr.s_addr) != 0)
    throw error(errno);
  sin.sin_port = htons(port);

  // Prepare bufferevent
  mBufferEvent1 = bufferevent_socket_new(mBase, -1, BEV_OPT_CLOSE_ON_FREE);

  // No write callback for now
  bufferevent_setcb(mBufferEvent1, readcb, nullptr, eventcb, this);
  bufferevent_enable(mBufferEvent1, EV_READ | EV_WRITE);

  // evbuffer_add(bufferevent_get_output(bev), message, block_size);

  retcode = bufferevent_socket_connect(mBufferEvent1, reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin));
  if (retcode < 0)
    throw error(retcode);
}

void client::start_with_delay(int seconds)
{
    if (mBufferEvent)
        return;

    mBase = event_base_new();

    timeval timeout_value;
    timeout_value.tv_sec = seconds;
    timeout_value.tv_usec = 0;

    mTimeoutEvent = evtimer_new(mBase, timeoutcb, this);
    evtimer_add(mTimeoutEvent, &timeout_value);
}

void client::stop()
{
    if (!mBase)
        return;

    if (mBufferEvent)
    {
        bufferevent_free(mBufferEvent);
        mBufferEvent = nullptr;
    }

    if (mTimeoutEvent)
    {
        event_free(mTimeoutEvent);
        mTimeoutEvent = nullptr;
    }

    if (mBase)
    {
        event_base_free(mBase);
        mBase = nullptr;
    }
}

void client::write(const void* buffer, size_t num)
{
    if (mBufferEvent)
    {
        evbuffer_add(bufferevent_get_output(mBufferEvent), buffer, num);
    }
}

/*
static void set_tcp_no_delay(evutil_socket_t fd)
{
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,   &one, sizeof one);
}
*/


void client::timeoutcb(evutil_socket_t fd, short what, void *arg)
{
    // Time to start connecting
    client* c = reinterpret_cast<client*>(arg);
    try
    {
        c->start();
    }
    catch (const error& e)
    {
    }
}

void client::readcb(struct bufferevent *bev, void *ctx)
{
    client* c = reinterpret_cast<client*>(ctx);

    /* This callback is invoked when there is data to read on bev. */
    struct evbuffer *input = bufferevent_get_input(bev);
    // struct evbuffer *output = bufferevent_get_output(bev);

    char readbuf[1024];
    size_t read = 0;

    while( (read = static_cast<size_t>(evbuffer_remove(input, &readbuf, sizeof(readbuf)))) > 0)
    {
        c->on_read_handler(readbuf, read);
    }
}

void client::on_read_handler(const void *buf, size_t num)
{
    if (mReadCb)
        mReadCb(*this, buf, num);
}

void client::eventcb(struct bufferevent *bev, short events, void *ptr)
{
    client* c = reinterpret_cast<client*>(ptr);
    if (events & BEV_EVENT_CONNECTED)
    {
        // Connected
        if (c->mConnectedCb)
            c->mConnectedCb(*c);
        //evutil_socket_t fd = bufferevent_getfd(bev);
        //set_tcp_no_delay(fd);
    } else if (events & BEV_EVENT_ERROR) {
        //printf("NOT Connected\n");
        if (c->mDisconnectedCb)
            c->mDisconnectedCb(*c, errno);
    } else if (events & BEV_EVENT_EOF) {
        if (c->mDisconnectedCb)
            c->mDisconnectedCb(*c, 0);
    }
}

void client::update()
{
    if (mBase)
      event_base_dispatch(mBase);
}

// ------------ msgclient --------------
msgclient::msgclient()
{
    mParser.set_callback([this](const void* buf, size_t num)
    {
       if (buf == nullptr && num == 0xFFFFFFFF)
       {
           // Format error, disconnect
           if (mDisconnectedCb)
               mDisconnectedCb(*this, 200);
           stop();
       }
       if (mMessageCb)
           mMessageCb(*this, buf, num);
    });
}

msgclient::~msgclient()
{}

void msgclient::set_message_callback(on_message cb)
{
    mMessageCb = cb;
}

void msgclient::on_read_handler(const void *buf, size_t num)
{
    mParser.parse(buf, num);
}

void msgclient::send_msg(const void* buf, size_t num)
{
    msgparser::header hdr;
    hdr.mMagic = htonl(msgparser::MAGIC);
    hdr.mLength = htonl(static_cast<uint32_t>(num));

    write(&hdr, sizeof(hdr));
    write(buf, num);
}

} // end of namespace
