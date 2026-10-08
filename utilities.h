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

#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

namespace utilities {

void file_put_contents(const std::filesystem::path& filename, std::string_view contents);
std::string file_get_contents(const std::filesystem::path& filename);

template <typename... Parts>
    requires (std::constructible_from<std::filesystem::path, const Parts&> && ...)
[[nodiscard]] std::string create_path(const Parts&... parts)
{
    std::filesystem::path path;
    // On an empty path, operator/= does not add a separator.
    ((path /= parts), ...);
    return path.string();
}

[[nodiscard]] std::string home_directory();
[[nodiscard]] std::string root_path();
[[nodiscard]] std::filesystem::path tmp_file();
int shell_run (std::string command, std::string& out_err);
void trim(std::string& s);
[[nodiscard]] std::vector<std::string> explode_lines(std::string_view text, const char delimiter);



}