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

#include "proto_utils.h"
#include <arpa/inet.h>
#include <assert.h>
#include <event2/thread.h>

using namespace proto;

error::error(int code)
    :mCode(code)
{
}

error::~error()
{

}

int error::code() const
{
    return mCode;
}

msgparser::msgparser()
{}

msgparser::~msgparser()
{}

void msgparser::set_callback(msg_arrived cb)
{
    mCallback = cb;
}

void msgparser::parse(const void* buffer, size_t num)
{
    // Add to buffer
    mBuffer.append(reinterpret_cast<const char*>(buffer), num);

    while (mBuffer.size() > mHeader.mLength || (mHeader.mLength == 0xFFFFFFFF && mBuffer.size() >= sizeof(mHeader)))
    {
        if (mHeader.mLength == 0xFFFFFFFF)
        {
            // Length header is not fetched yet
            assert(sizeof(mHeader) == 8);

            if (mBuffer.size() >= sizeof(mHeader))
            {
                mHeader.mMagic = *reinterpret_cast<const uint32_t*>(mBuffer.c_str());
                mHeader.mMagic = ntohl(mHeader.mMagic);

                mHeader.mLength = *reinterpret_cast<const uint32_t*>(mBuffer.c_str() + sizeof(uint32_t));
                mHeader.mLength = ntohl(mHeader.mLength);

                mBuffer.erase(0, sizeof(mHeader));

                // Check magic
                if (mHeader.mMagic != msgparser::MAGIC)
                {
                    // Send zero buffer to signal about error
                    if (mCallback)
                        mCallback(nullptr, 0xFFFFFFFF);
                }
            }
        }
        else
        if (mBuffer.size() >= mHeader.mLength)
        {
            if (mCallback)
                mCallback(mBuffer.c_str(), mHeader.mLength);
            mBuffer.erase(0, mHeader.mLength);
            mHeader.mLength = 0xFFFFFFFF;
            mHeader.mMagic = 0;
        }
    }
}

// --------------- initializer --------------------
class initializer
{
public:
    initializer()
    {
        evthread_use_pthreads();
    }
    ~initializer()
    {}
};

static initializer thread_initializer;
