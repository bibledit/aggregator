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
#include <vector>
#include "logger.h"
#include "utilities.h"
#include "exception.h"

namespace sword {

Store::Store(const std::optional<std::filesystem::path>& alternate_store_path)
{
    // Path may be passed (for unit tests).
    // If omitted, it uses a standard location.
    if (alternate_store_path)
        m_path = *alternate_store_path;
    else
        m_path = std::filesystem::path(SOURCE_DIR) / "sword" / "store.json";

    load_from_file();
}

Store::~Store()
{
    save_to_file();
}

// Generates to_json() and from_json() for struct Module.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Module, source, identifier, version, name)

void Store::load_from_file()
{
    m_modules.clear();

    std::ifstream stream(m_path);
    if (not stream)
        return; // No file: Start with an empty list.

    try {
        // Parse without exceptions first, to be able to report problems.
        const nlohmann::json json = nlohmann::json::parse(stream, nullptr, true);
        m_modules = json.get<std::vector<Module>>();
    }
    catch (const nlohmann::json::exception& e) {
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

void Store::add_or_replace(const Module& module)
{
    // A module is uniquely identified by its source and identifier.
    const auto same_module = [&module](const Module& existing) {
        return existing.source == module.source and existing.identifier == module.identifier;
    };

    if (const auto iter = std::ranges::find_if(m_modules, same_module); iter != m_modules.cend()) {
        *iter = module; // Replace in place, keeping the position.
    } else {
        m_modules.push_back(module); // Not present: add at the end.
    }
}


[[nodiscard]] std::optional<Module> Store::get_module(const std::string& source, const std::string& identifier) const
{
    // A module is uniquely identified by its source and identifier.
    const auto same_module = [&source, &identifier](const Module& existing) {
        return existing.source == source and existing.identifier == identifier;
    };

    if (const auto iter = std::ranges::find_if(m_modules, same_module); iter != m_modules.cend())
        return *iter; // Found.

    return {}; // Not found.
}

}
