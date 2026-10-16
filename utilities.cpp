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
#include <charconv>
#include <fstream>
#include <ranges>
#include <sstream>
#include <vector>
#include "utilities.h"
#include "exception.h"
#include <unicode/ustdio.h>
#include <unicode/normlzr.h>
#include <unicode/utypes.h>
#include <unicode/unistr.h>
#include <unicode/translit.h>
#include <uuid/uuid.h>



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
    return parts;
}


[[nodiscard]] int get_next_id(const std::optional<std::filesystem::path>& test_path)
{
    // Path may be passed (for unit tests), otherwise use production location.
    const auto path = test_path.value_or(std::filesystem::path(SOURCE_DIR) / "data" / "id.txt");

    std::string text = file_get_contents(path);
    trim(text);
    if (text.empty()) // Handle initial empty file.
        text = "0";

    decltype(get_next_id()) id{};
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), id);
    if (ec != std::errc())
        throw Base("Invalid id file", path.string());
    ++id; // Next ID.

    file_put_contents(path, std::to_string(id)); // Store for next ID fetch.

    return id;
}


// The Levenshtein distance is a fundamental string metric
// to measure the difference between two sequences.
// It  calculates the minimum number of single-character edits required
// to transform one string into another.
static size_t levenshtein_distance(const std::string& a, const std::string& b)
{
    std::vector<size_t> prev(b.size() + 1), curr(b.size() + 1);
    for (size_t j = 0; j <= b.size(); j++) prev[j] = j;
    for (size_t i = 1; i <= a.size(); i++) {
        curr[0] = i;
        for (size_t j = 1; j <= b.size(); j++) {
            const size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = std::min({prev[j] + 1,           // deletion
                                curr[j - 1] + 1,       // insertion
                                prev[j - 1] + cost }); // substitution
        }
        std::swap(prev, curr);
    }
    return prev[b.size()];
}

// Similarity as a percentage: 100 = identical, 0 = completely different.
int similarity_percentage(const std::string& a, const std::string& b)
{
    const size_t max_len = std::max(a.size(), b.size());
    if (max_len == 0) return 100;
    return static_cast<int>(100.0 * (1.0f - static_cast<float>(levenshtein_distance(a, b)) / static_cast<float>(max_len)));
}


[[nodiscard]] std::string unicode_case_fold(const std::string& s)
{
    // UTF-8 string -> UTF-16 UnicodeString
    icu::UnicodeString source = icu::UnicodeString::fromUTF8 (icu::StringPiece (s));
    // Case folding.
    source.foldCase ();
    // UTF-16 UnicodeString -> UTF-8 std::string
    std::string result;
    source.toUTF8String (result);
    // Ready.
    return result;
}


[[nodiscard]] std::string replace(std::string_view s, const std::string_view original, const std::string_view replacement)
{
    std::string out;
    out.reserve(s.size());

    size_t pos = 0;
    while (true) {
        const size_t found = s.find(original, pos);
        if (found == std::string::npos) {
            out.append(s, pos, std::string::npos); // Copy the remainder.
            break;
        }
        out.append(s, pos, found - pos); // Text before the match.
        out += replacement;
        pos = found + original.size(); // Continue after the match.
    }
    return out;
}


[[nodiscard]] std::string replace_xml_elements(const std::string& s)
{
    constexpr std::string_view original = R"(<milestone type="line" />)";
    constexpr std::string_view replacement = " ";
    return replace(s, original, replacement);
}


[[nodiscard]] std::string strip_xml_elements(std::string_view s)
{
    std::string out;
    out.reserve(s.size());

    while (!s.empty()) {
        const size_t open = s.find('<');
        if (open == std::string_view::npos) { // No more tags: keep the rest.
            out += s;
            break;
        }
        out += s.substr(0, open); // Text before the tag.

        const size_t close = s.find('>', open);
        if (close == std::string_view::npos) // unterminated tag: drop the rest.
            break;
        s.remove_prefix(close + 1); // Continue after '>'
    }
    return out;
}


void remove_double_spaces(std::string& str)
{
    // The std::unique compacts the string in place in one pass.
    // It drops each element for which the predicate says it equals the last kept element,
    // so a run of spaces shrinks to one.
    // The erase then trims the leftover tail.
    // This is O(n) and so fast.
    constexpr auto predicate = [](const char a, const char b) { return a == ' ' and b == ' '; };
    str.erase(std::ranges::unique(str, predicate).begin(), str.end());
}


// Returns the seconds since the Unix epoch, which is 1 January 1970 UTC.
int seconds_since_unix_epoch()
{
    const auto now = std::chrono::system_clock::now();
    const auto duration = now.time_since_epoch();
    return static_cast<decltype(seconds_since_unix_epoch())>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
}


// Only version 4 of Leach-Salz UUID is used for the UUID in the RSMP specs.
// https://datatracker.ietf.org/doc/html/rfc4122.
// The library "libuuid" has mechanisms to adhere to the above RFC.
[[nodiscard]] std::string get_uuid() noexcept
{
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[UUID_STR_LEN]; // eg. "1b4e28ba-2fa1-11d2-883f-0016d3cca427" + "\0"
    uuid_unparse_lower(uuid, uuid_str);
    return uuid_str;
}


}
