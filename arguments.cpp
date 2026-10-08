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
#include <sstream>
#include <getopt.h>
#include "arguments.h"
#include <array>
#include <iostream>
#include <ranges>
#include "exception.h"


Arguments::Arguments(const int argc, char* argv[])
{
    // Long-only options start at 256 to not collide with an ASCII character.
    enum long_only : int
    {
        o_help = 256,
        o_sword,
    };

    struct OptionSpec
    {
        std::string_view name;
        int has_arg;  // no_argument / required_argument / optional_argument.
        int val;
        std::string_view arg_name; // shown in usage, e.g. "sword=module"; empty if none.
        std::string_view description;
    };

    static constexpr std::array specs =
    {
        OptionSpec{
            .name = "sword",
            .has_arg = required_argument,
            .val = o_sword,
            .arg_name = "module",
            .description = "Sword to use"
        },
        OptionSpec{
            .name = "help",
            .has_arg = no_argument,
            .val = o_help,
            .arg_name = "",
            .description = "Show usage help"
        },
    };

    // Build the getopt_long table from the specs (plus required terminator).
    constexpr auto make_long_options = []
    {
        std::array<option, specs.size() + 1> opts{};
        for (std::size_t i = 0; i < specs.size(); ++i)
        {
            // name must be NULL-terminated: string literals in specs guarantee that
            opts[i] = option{
                .name = specs[i].name.data(),
                .has_arg = specs[i].has_arg,
                .flag = nullptr,
                .val = specs[i].val
            };
        }
        opts[specs.size()] = option{.name = nullptr, .has_arg = 0, .flag = nullptr, .val = 0};
        return opts;
    };

    constexpr auto long_options = make_long_options();

    // Compute column width so descriptions line up
    constexpr auto option_width = [](const OptionSpec& s) -> std::size_t
    {
        // "--name" plus "=ARG" when there's an argument
        return 2 + s.name.size() + (s.arg_name.empty() ? 0 : 1 + s.arg_name.size());
    };

    constexpr std::size_t width =
        std::ranges::max(specs | std::ranges::views::transform(option_width));

    const auto make_usage = [](const char* program) -> std::string
    {
        std::ostringstream ss;
        ss << "Usage: " << program << " [options]\n\nOptions:\n";
        for (const auto& s : specs)
        {
            std::string left = "--" + std::string(s.name);
            if (not s.arg_name.empty())
                left += "=" + std::string(s.arg_name);
            ss << "  " << left << std::string(width - left.size() + 2, ' ') << s.description << '\n';
        }
        return ss.str();
    };

    int c{};
    while ((c = getopt_long(argc, argv, "", long_options.data(), nullptr)) != -1)
    {
        switch (c)
        {
        case o_sword:
            m_sword = optarg;
            break;
        case o_help:
        default:
            throw Base(make_usage(argv[0]));
        }
    }
}
