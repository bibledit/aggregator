/*
Copyright (©) 2006-2026 Teus Benschop.

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
#include "utilities.h"

#include <ranges>
#include <sstream>
#include <vector>

namespace utilities {

void file_put_contents(const std::filesystem::path& filename, const std::string_view contents)
{
    std::ofstream file;
    file.exceptions(std::ios::failbit | std::ios::badbit);
    file.open(filename, std::ios::binary | std::ios::trunc);
    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    file.close();
}


[[nodiscard("the file contents are only returned, never stored elsewhere")]]
std::string file_get_contents(const std::filesystem::path& filename)
{
    std::error_code ec;
    const std::uintmax_t size = std::filesystem::file_size(filename, ec);
    if (ec or size == 0)
        return {};

    std::ifstream ifs(filename, std::ios::binary);
    if (not ifs)
        return {};

    std::string result(size, '\0');
    ifs.read(result.data(), static_cast<std::streamsize>(size));
    result.resize(static_cast<std::size_t>(ifs.gcount())); // handles a file that shrank meanwhile.
    return result;
}



[[nodiscard]] std::string home_directory()
{
    std::string path{"."};
    if (const char* home = getenv("HOME"))
        path = home;
    return path;
}


[[nodiscard]] std::string root_path()
{
    return create_path(home_directory(), "aggregator");
}


// Returns a rotating unique filename from a pool of 256 names in the temp directory.
[[nodiscard]] std::filesystem::path tmp_file()
{
    static std::atomic<std::uint8_t> rotator {0};
    rotator.fetch_add(1, std::memory_order_relaxed); // wraps 255 -> 0.
    std::ostringstream oss{"aggregator", std::ios::ate};
    oss << std::setw(3) << std::setfill('0') << static_cast<unsigned>(rotator);
    return std::filesystem::temp_directory_path() / std::move(oss).str();
}


// Runs the passed command as if it were typed on the command line.
// Does not escape anything in the command.
// Returns the exit code of the process.
// The output of the process, both stdout and stderr, go into variable out_err.
int shell_run (std::string command, std::string& out_err)
{
    const auto pipe = tmp_file();
    command.append (" > " + pipe.string() + " 2>&1");
    const int result = system (command.c_str());
    out_err = file_get_contents(pipe);
    return result;
}


void trim(std::string& s)
{
    constexpr std::string_view whitespace{" \t\n\v\f\r"};
    s.erase(s.find_last_not_of(whitespace) + 1); // npos + 1 wraps to 0, erasing everything
    s.erase(0, s.find_first_not_of(whitespace)); // npos erases from 0 to the end, which is a no-op on an empty string
}


[[nodiscard]] std::vector<std::string> explode_lines(std::string_view text, const char delimiter)
{
    std::vector<std::string> parts;
    for (auto&& part : text | std::views::split(delimiter))
        parts.emplace_back(part.begin(), part.end());
    // std::size_t start = 0;
    // while (true)
    // {
    //     const std::size_t pos = text.find(delimiter, start);
    //     if (pos == std::string_view::npos)
    //     {
    //         parts.emplace_back(text.substr(start));
    //         break;
    //     }
    //     parts.emplace_back(text.substr(start, pos - start));
    //     start = pos + 1;
    // }
    return parts;
}


}
