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


#include "store.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <list>
#include "logger.h"
#include "utilities.h"
#include "exception.h"

namespace store {

Store::Store(const std::optional<std::filesystem::path>& alternate_store_path)
{
    // Path may be passed (for unit tests).
    // If omitted, it uses a standard location.
    if (alternate_store_path)
        m_path = *alternate_store_path;
    else
        m_path = std::filesystem::path(SOURCE_DIR) / "data" / "modules.json";

    load_from_file();
}

Store::~Store()
{
    save_to_file();
}

// Generates to_json() and from_json() for struct Module.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Module, id, type, source, abbrev, version, name)

void Store::load_from_file()
{
    m_modules.clear();

    std::ifstream stream(m_path);
    if (not stream)
        return; // No file: Start with an empty list.

    try {
        // Parse without exceptions first, to be able to report problems.
        const nlohmann::json json = nlohmann::json::parse(stream, nullptr, true);
        m_modules = json.get<std::list<Module>>();
    }
    catch (const nlohmann::json::exception&) {
        logger::plain("Cannot load SWORD modules from", m_path, ":", "Start with empty list of modules");
        m_modules.clear();
    }
}

void Store::save_to_file() const
{
    try {
        const nlohmann::json json = m_modules; // Uses the generated to_json().
        // Write to a temporary file first, then rename it over the real one,
        // so a crash mid-write cannot corrupt the existing file.
        const std::filesystem::path temporary = utilities::tmp_file();
        {
            std::ofstream stream(temporary, std::ios::trunc);
            if (not stream)
                throw Base("Cannot open", temporary.string());
            // Plain "dump" throws on invalid UTF-8.
            // SWORD module names are not assumed to be valid UTF-8 always.
            // The extended "dump" replaces invalid UTF-8 with this: � .
            stream << json.dump(2, ' ', false, nlohmann::json::error_handler_t::replace);
            stream << std::endl; // Flushes.
            if (not stream)
                throw Base("Cannot write", temporary.string());
        }
        std::filesystem::rename(temporary, m_path);
    }
    catch (const std::exception& e) {
        logger::plain("Cannot save SWORD store to", m_path, ":", e.what());
    }
}

void Store::add_or_update(const Module& module)
{
    // A module is uniquely identified by its type, source and abbreviation.
    const auto same_module = [&module](const Module& existing) {
        return existing.type == module.type and existing.source == module.source and existing.abbrev == module.abbrev;
    };

    if (const auto iter = std::ranges::find_if(m_modules, same_module); iter != m_modules.cend()) {
        // Update: Check that the id of the incoming module is the same as the existing module.
        if (iter->id != module.id)
            throw Base("Failed to update the store with a module with id", module.id, "whereas the store has id", iter->id);
        // Update in place, iterator remains valid.
        *iter = module;
    } else {
        // Add: Check that the incoming id does not yet exist in the store.
        if (get_module(module.id))
            throw Base("Failed to add module with id", module.id, "because this id already exists in the store");
        // Not present: add at the end.
        m_modules.push_back(module);
    }
}


[[nodiscard]] std::optional<Module> Store::get_module(const Type type, const std::string& source, const std::string& abbrev) const
{
    // A module is uniquely identified by its type, source and identifier.
    const auto match = [type, &source, &abbrev](const Module& existing) {
        return existing.type == type and existing.source == source and existing.abbrev == abbrev;
    };

    if (const auto iter = std::ranges::find_if(m_modules, match); iter != m_modules.cend())
        return *iter; // Found.

    return {}; // Not found.
}


[[nodiscard]] std::optional<Module> Store::get_module(const int id) const
{
    if (const auto iter = std::ranges::find(m_modules, id, &Module::id); iter != m_modules.cend())
        return *iter;
    return {};
}


[[nodiscard]] std::size_t Store::count(const Type type) const
{
    return static_cast<decltype(count(type))>(std::ranges::count(m_modules, type, &Module::type));
}


[[nodiscard]] std::size_t Store::count(const Type type, const std::string& source) const
{
    const auto compare = [&](const Module& module) {
        return module.type == type and module.source == source;
    };
    return static_cast<decltype(count(type,source))>(std::ranges::count_if(m_modules, compare));
}


}
