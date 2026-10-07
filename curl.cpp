/*
Copyright (©) 2026-2026 Teus Benschop.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <algorithm>
#include <chrono>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <curl/curl.h>
#include "curl.h"
#include "exception.h"

// Helper function for libcurl to write data to a server.
static size_t curl_write_function(void* ptr, const size_t size, const size_t count, void* stream) noexcept
{
    static_cast<std::string*>(stream)->append(static_cast<char*>(ptr), 0, size * count);
    return size * count;
}


// Helper function for libcurl to save the response headers.
static size_t header_callback(const char* buffer, const size_t size,
                              const size_t num_items, void* userdata)
{
    auto* headers = static_cast<std::string*>(userdata);
    headers->append(buffer, num_items * size);
    return num_items * size;
}


// The debug function for libcurl, it dumps the data as specified.
static void curl_debug_dump(const char* const text, FILE* const stream, const unsigned char* const ptr, const size_t size)
{
    size_t c;
    constexpr unsigned int width = 0x10;

    fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n", text, static_cast<long>(size), size);

    for (size_t i = 0; i < size; i += width) {
        fprintf(stream, "%4.4lx: ", i);

        // Show hex to the left.
        for (c = 0; c < width; c++) {
            if (i + c < size)
                fprintf(stream, "%02x ", ptr[i + c]);
            else
                fputs("   ", stream);
        }

        // Show data on the right.
        for (c = 0; c < width and i + c < size; ++c) {
            const unsigned char x = ptr[i + c] >= 0x20 && ptr[i + c] < 0x80 ? ptr[i + c] : '.';
            fputc(x, stream);
        }

        // Newline.
        fputc('\n', stream);
    }
}


// The trace function for libcurl.
static int curl_trace(CURL*, const curl_infotype type, char* data, const size_t size, void*)
{
    const char* text { nullptr };

    switch (type) {
        case CURLINFO_TEXT:
            fprintf(stderr, "== Info: %s", data);
            return 0;
        case CURLINFO_HEADER_OUT:
            text = "=> Send header";
            break;
        case CURLINFO_DATA_OUT:
            text = "=> Send data";
            break;
        case CURLINFO_SSL_DATA_OUT:
            text = "=> Send SSL data";
            break;
        case CURLINFO_HEADER_IN:
            text = "<= Recv header";
            break;
        case CURLINFO_DATA_IN:
            text = "<= Recv data";
            break;
        case CURLINFO_SSL_DATA_IN:
            text = "<= Recv SSL data";
            break;
        case CURLINFO_END:
        default:
            return 0;
    }

    curl_debug_dump(text, stderr, reinterpret_cast<unsigned char*>(data), size);
    return 0;
}


// Constructor.
Curl::Curl(const std::string& url,
           std::string&& post_data,
           const bool verify_peer,
           const std::string& ca_cert,
           const std::vector<std::pair<std::string, std::string>>& headers,
           const bool debug,
           const unsigned int max_bytes_per_second)
    : m_curl(curl_easy_init()), m_post_data(std::move(post_data))
{
    // Handle error on curl handle.
    if (!m_curl)
        throw Base("Could not get a curl handle");

    // Set the URL that is about to receive the GET or POST.
    // This can be http or https.
    if (curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str()) != CURLE_OK)
        throw Base("Could not set curl URL");

    // Specify the POST data to curl, e.g.: "name=foo&project=bar".
    // Support posting null bytes by setting the size of the data.
    if (!m_post_data.empty()) {
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, m_post_data.data());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, m_post_data.size());
    }

    // Further options.
    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);

    if (debug) {
        // Enable the trace function.
        curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, curl_trace);
        // The DEBUGFUNCTION has no effect until we enable VERBOSE.
        curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
    }

    // Set the optional extra headers.
    for (const auto& [header_key, header_value] : headers) {
        std::string line;
        line.append(header_key);
        line.append(": ");
        line.append(header_value);
        m_headers = curl_slist_append(m_headers, line.c_str());
    }
    if (m_headers)
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);

    // There is a timeout on establishing a connection.
    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 10);

    // There is a also a transfer timeout for normal speeds.
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 600);

    // There is also a shorter transfer timeout for low speeds,
    // because low speeds indicate a stalled connection.
    curl_easy_setopt(m_curl, CURLOPT_LOW_SPEED_LIMIT, 100);
    curl_easy_setopt(m_curl, CURLOPT_LOW_SPEED_TIME, 10);

    // Timing out may use signals, which is not what we want.
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);

    // Whether to check the secure certificate.
    // If the configuration of the site is not right, the certificate cannot be verified.
    // That would result in resources not being fetched anymore.
    if (curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, verify_peer ? 1L : 0L) != CURLE_OK)
        throw Base("Could not set Verify Peer option");

    // If specified, override the system default certificate store and use this CA instead:
    if (!ca_cert.empty())
        if (curl_easy_setopt(m_curl, CURLOPT_CAINFO, ca_cert.c_str()) != CURLE_OK)
            throw Base("Could not set CA Certificate option");

    // Setting the upload and download speed throttle in bytes per second.
    // This requires a relatively small buffer to work well.
    if (curl_easy_setopt(m_curl, CURLOPT_UPLOAD_BUFFERSIZE, 16000) != CURLE_OK)
        throw Base("Could not set the upload buffer size");
    if (curl_easy_setopt(m_curl, CURLOPT_MAX_SEND_SPEED_LARGE, static_cast<curl_off_t>(max_bytes_per_second)) != CURLE_OK)
        throw Base("Could not set the maximum send speed in bytes per second");
    if (curl_easy_setopt(m_curl, CURLOPT_MAX_RECV_SPEED_LARGE, static_cast<curl_off_t>(max_bytes_per_second)) != CURLE_OK)
        throw Base("Could not set the maximum receive speed in bytes per second");

    // Callbacks for the server response headers and body.
    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &m_response_header);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl_write_function);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_response_body);
}


Curl::~Curl() noexcept
{
    if (m_curl)
        curl_easy_cleanup(m_curl);

    if (m_headers) {
        curl_slist_free_all(m_headers);
        m_headers = nullptr;
    }
}


void Curl::perform_request()
{
    // Empty a possible previous response.
    m_response_header.clear();
    m_response_body.clear();

    // Perform / check the request.
    if (const CURLcode res = curl_easy_perform(m_curl); res == CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != 200)
            throw Base("Server response code " + std::to_string(http_code));
    }
    else
        throw Base("Error fetching content: " + std::string(curl_easy_strerror(res)));
}
