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

#include <vector>
#include "store.h"
#include "sword.h"

namespace sword {

constexpr auto list {"list"};
constexpr auto develop {"develop"};

struct info
{
    std::string abbrev{};
    std::string version{};
    std::string name{};
};

}

class Sword
{
public:
    explicit Sword(store::Store& store) : m_store(store) {}
    void initialize();
    static void list_remote_sources();
    void fetch_modules();
    void leave_or_download_or_update_modules();
    void remote_source(std::string_view);
    void develop();
protected:
    [[nodiscard]] bool download_required(const sword::info& info) const;
private:
    std::string out_err{};
    std::string m_remote_source{}; // The remote source.
    std::vector<sword::info> m_modules{}; // The remote modules.
    store::Store& m_store; // The general store.
    void install_module(const sword::info& info);
    void import_module(const sword::info& info);
    static void uninstall_module(const sword::info& info);
};
