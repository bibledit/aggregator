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

#include <exception>
#include <string>
#include <string_view>

template <typename Arg>
concept one_parameter_stream_writable = requires (std::ostream& os, const Arg& arg)
{
    { os << arg } -> std::convertible_to<std::ostream&>;
};

template <typename ...Args>
concept stream_writable = (one_parameter_stream_writable<Args> and ...);

class Base : public std::exception
{
    std::string m_what;
public:
    template <stream_writable ...Args>
    explicit Base(Args&&...args) noexcept
    {
        std::ostringstream oss;
        bool first {true};
        (void((first ? void(first = false) : void(oss << ' '), oss << std::forward<Args>(args))), ...);
        m_what.assign(std::move(oss).str());
    }
    [[nodiscard]] const char* what() const noexcept override { return m_what.c_str(); }
};

namespace base {

struct Derived1 : Base { using Base::Base; };

}

