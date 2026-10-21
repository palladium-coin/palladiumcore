// Copyright (c) 2020 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpserver.h>
#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/strencodings.h>

#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_struct.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {
std::string ConsumeToken(FuzzedDataProvider& provider, size_t max_len)
{
    // Header names and values must not contain NUL, libevent treats them as C strings
    std::string s = provider.ConsumeRandomLengthString(max_len);
    for (char& c : s) {
        if (c == '\0') c = '_';
    }
    return s;
}
} // namespace

/**
 * Build an evhttp request by hand (the part of libevent the node trusts) and run every accessor of
 * HTTPRequest on it: URI, method, peer, headers, body.
 */
void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    evhttp_request* evreq = evhttp_request_new(nullptr, nullptr);
    if (evreq == nullptr) return;

    evreq->type = provider.PickValueInArray({EVHTTP_REQ_GET, EVHTTP_REQ_POST, EVHTTP_REQ_HEAD, EVHTTP_REQ_PUT,
                                             EVHTTP_REQ_DELETE, EVHTTP_REQ_OPTIONS, EVHTTP_REQ_TRACE,
                                             EVHTTP_REQ_CONNECT, EVHTTP_REQ_PATCH});
    const std::string uri = ConsumeToken(provider, 200);
    evreq->uri = strdup(uri.c_str());
    evreq->kind = EVHTTP_REQUEST;

    const std::string header_name = ConsumeToken(provider, 30);
    const std::string header_value = ConsumeToken(provider, 100);
    if (!header_name.empty()) {
        evhttp_add_header(evhttp_request_get_input_headers(evreq), header_name.c_str(), header_value.c_str());
    }
    evhttp_add_header(evhttp_request_get_input_headers(evreq), "Authorization", ConsumeToken(provider, 100).c_str());

    const std::vector<uint8_t> body = provider.ConsumeBytes<uint8_t>(provider.ConsumeIntegralInRange<size_t>(0, 500));
    if (!body.empty()) {
        evbuffer_add(evhttp_request_get_input_buffer(evreq), body.data(), body.size());
    }

    // replySent=true: the destructor and the accessors must not need a connection
    HTTPRequest request{evreq, /* replySent */ true};
    (void)request.GetRequestMethod();
    (void)request.GetURI();
    (void)request.GetPeer();
    const auto header = request.GetHeader(header_name);
    if (header.first) {
        (void)header.second.size();
    }
    (void)request.GetHeader("authorization");
    const std::string read_body = request.ReadBody();
    assert(read_body.size() == body.size());
    // The buffer is consumed by the first read
    assert(request.ReadBody().empty());
    // evhttpd frees the request after a reply; there is no connection here
    evhttp_request_free(evreq);
}
