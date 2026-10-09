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
#include <optional>
#include <string>
#include <vector>

namespace sword {

struct Module
{
    std::string source{}; // Remote SWORD source, e.g. "Crosswire".
    std::string identifier{}; // Module identifier, e.g. "GNB2026".
    std::string version{}; // Module version, e.g. "1.2".
    std::string name{}; // Module name, e.g. "Great News Bible".
    constexpr std::strong_ordering operator<=>(const Module&) const = default;
};

class Store
{
public:
    explicit Store(const std::optional<std::filesystem::path>& = std::nullopt);
    Store() = delete;
    ~Store();
    void save_to_file() const;
    void add_or_replace(const Module&);
    [[nodiscard]] std::size_t count() const { return m_modules.size(); }
    [[nodiscard]] std::size_t count(const std::string&) const;
    [[nodiscard]] std::optional<Module> get_module(const std::string& source, const std::string& identifier) const;
    constexpr bool operator==(const Store& other) const { return m_modules == other.m_modules; }
private:
    void load_from_file();
    std::vector<Module> m_modules{};
    std::filesystem::path m_path{};
};

}
