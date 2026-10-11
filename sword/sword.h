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

#include <map>
#include <vector>
#include "store.h"
#include "sword.h"

namespace sword {
struct info;
}

class Sword
{
public:
    explicit Sword(store::Store& store) : m_store(store) {}
    Sword() = delete;
    void initialize();
    void fetch_remote_sources();
    void fetch_modules();
    void download_or_update_modules();
    [[nodiscard]] bool download_required(const std::string& remote_source, const sword::info& info) const;
private:
    std::string out_err{};
    std::vector<std::string> m_remote_sources{}; // The remote sources.
    std::map<std::string, std::vector<sword::info>> m_sources_modules{}; // The remote modules.
    store::Store& m_store; // The general store.
};

namespace sword {

struct info
{
    std::string abbrev{};
    std::string version{};
    std::string name{};
};


std::optional<info> parse (std::string_view line) noexcept;

}