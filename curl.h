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

#pragma once

#include <string>
#include <vector>
#include <curl/curl.h>

// Sends a http GET or POST request to the given URL.
// If post data is given, it posts that data as is.
// It returns the response from the server.
class Curl {
  public:
    Curl(const std::string& url,
         std::string&& post_data,
         bool verify_peer,
         const std::string& ca_cert,
         const std::vector<std::pair<std::string, std::string>>& headers,
         bool debug,
         unsigned int max_bytes_per_second);
    ~Curl() noexcept;
    Curl(const Curl&) = delete;
    Curl(Curl&&) = delete;
    Curl& operator=(const Curl&) = delete;
    Curl& operator=(Curl&&) = delete;
    void perform_request();
    const std::string& get_response_header() { return m_response_header; }
    const std::string& get_response_body() { return m_response_body; }
  private:
    CURL* m_curl; // The C URL handle.
    curl_slist* m_headers {nullptr}; // The headers for the request.
    std::string m_post_data {}; // The data to be POSTed.
    std::string m_response_header {}; // The response headers from the server.
    std::string m_response_body {}; // The response body from the server.
};
