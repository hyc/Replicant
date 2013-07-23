// Copyright (c) 2012, Robert Escriva
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Replicant nor the names of its contributors may be
//       used to endorse or promote products derived from this software without
//       specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Google Log
#include <glog/logging.h>

// e
#include <e/endian.h>

// Replicant
#include "daemon/replicant_state_machine.h"
#include "daemon/replicant_state_machine_context.h"

#define COND_STR2NUM(STR, NUM) \
    do \
    { \
        if (strlen(STR) > sizeof(uint64_t)) \
        { \
            return -1; \
        } \
        char cond_buf[sizeof(uint64_t)]; \
        memset(cond_buf, 0, sizeof(cond_buf)); \
        memmove(cond_buf, STR, strlen(STR)); \
        e::unpack64be(cond_buf, &NUM); \
    } while (0)

replicant_state_machine_context :: replicant_state_machine_context()
    : object(0)
    , client(0)
    , output()
    , conditions()
    , response(NULL)
    , response_sz(0)
{
}

replicant_state_machine_context :: ~replicant_state_machine_context() throw ()
{
}

extern "C"
{

uint64_t
replicant_state_machine_get_client(struct replicant_state_machine_context* ctx)
{
    return ctx->client;
}

FILE*
replicant_state_machine_log_stream(struct replicant_state_machine_context* ctx)
{
    return ctx->output;
}

void
replicant_state_machine_set_response(struct replicant_state_machine_context* ctx,
                                     const char* data, size_t data_sz)
{
    ctx->response = data;
    ctx->response_sz = data_sz;
}

int
replicant_state_machine_condition_create(struct replicant_state_machine_context* ctx,
                                         const char* cond)
{
    uint64_t _cond;
    COND_STR2NUM(cond, _cond);
    return ctx->conditions.create(_cond);
}

int
replicant_state_machine_condition_destroy(struct replicant_state_machine_context* ctx,
                                          const char* cond)
{
    uint64_t _cond;
    COND_STR2NUM(cond, _cond);
    return ctx->conditions.destroy(_cond);
}

int
replicant_state_machine_condition_broadcast(struct replicant_state_machine_context* ctx,
                                            const char* cond,
                                            uint64_t* state)
{
    uint64_t _cond;
    COND_STR2NUM(cond, _cond);
    return ctx->conditions.broadcast(_cond, state);
}

} // extern "C"
