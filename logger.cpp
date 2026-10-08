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


#include <fstream>
#include <iostream>
#include <mutex>
#include "logger.h"

namespace logger {

static std::mutex mutex;

// Log text to file.
// Open log file once during the app's lifetime.
void text (const std::string& message)
{
    if (message.empty())
        return;
    static std::ofstream file("/tmp/aggregator.log", std::ios::binary | std::ios::trunc);
    const std::lock_guard lock(mutex);
    std::cout << message << std::endl;
    time_t tt;
    time(&tt);
    const tm* time_struct = localtime(&tt);
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", time_struct);
    using namespace std::chrono;
    const auto ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() % 1000;
    file << buffer << "." << ms << " " << message << std::endl;
}

}
