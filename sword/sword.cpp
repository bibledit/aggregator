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
#include <filesystem>
#include "sword.h"

#include <iostream>
#include <ranges>
#include <string>
#include <__thread/this_thread.h>

#include "data.h"
#include "exception.h"
#include "logger.h"
#include "utilities.h"
#include "books/books.h"


[[nodiscard]] static std::string get_sword_install_mgr_path()
{
    return std::filesystem::path{utilities::home_directory()} / ".sword" / "InstallMgr";
}


void Sword::initialize()
{
    logger::plain("Initialize SWORD structures");

    // Initialize SWORD directory structure and configuration.
    const std::string sword_path = get_sword_install_mgr_path();
    const std::filesystem::path path(sword_path);
    logger::plain ("Initialize SWORD in", path);
    std::filesystem::create_directories(path);
    const std::string sword_conf = "[Install]\n"
        "DataPath=" + sword_path + "/\n";
    utilities::file_put_contents(utilities::create_path(sword_path, "sword.conf"), sword_conf);
    const std::string config_files_path = utilities::create_path(SOURCE_DIR, "sword");
    utilities::shell_run("cp -r " + config_files_path + "/locales.d " + sword_path, out_err);
    logger::plain(out_err);
    utilities::shell_run("cp -r " + config_files_path + "/mods.d " + sword_path, out_err);
    logger::plain(out_err);

    // Initialize basic user configuration.
    utilities::shell_run("installmgr --allow-internet-access-and-risk-tracing-and-jail-or-martyrdom --allow-unverified-tls-peer -init", out_err);
    utilities::trim(out_err);
    logger::plain(out_err);

    // Sync the configuration with the online known remote repository list.
    utilities::shell_run("installmgr --allow-internet-access-and-risk-tracing-and-jail-or-martyrdom --allow-unverified-tls-peer -sc", out_err);
    utilities::trim(out_err);
    logger::plain(out_err);
}


void Sword::fetch_remote_sources()
{
    logger::plain("Fetching list of remote SWORD sources");
    m_remote_sources.clear();
    utilities::shell_run("installmgr -s", out_err);
    utilities::trim(out_err);
    logger::plain(out_err);
    for (auto line : utilities::explode_lines(out_err, '\n'))
    {
        utilities::trim(line);
        if (line.empty() or not line.starts_with('[') or not line.ends_with(']'))
            continue;
        line.erase(0, 1);
        line.pop_back();
        m_remote_sources.emplace_back(line);
        logger::plain(line);
        return; // Todo remove later.
    }
}


void Sword::fetch_modules()
{
    logger::plain("Fetching SWORD modules");
    for (const auto& remote_source : m_remote_sources)
    {
        utilities::shell_run("installmgr --allow-internet-access-and-risk-tracing-and-jail-or-martyrdom --allow-unverified-tls-peer -r \"" + remote_source + "\"", out_err);
        utilities::trim(out_err);
        logger::plain(out_err);
        utilities::shell_run("installmgr -rl \"" + remote_source + "\"", out_err);
        for (const auto& line : utilities::explode_lines(out_err, '\n'))
        {
            if (const std::optional<sword::info> info = sword::parse (line))
            {
                m_sources_modules[remote_source].push_back(*info);
                logger::plain(line);
            }
        }
        logger::plain(remote_source, ":", m_sources_modules[remote_source].size(), "modules");
    }
}


void Sword::leave_or_download_or_update_modules() // Todo
{
    // Iterate over the remote sources and the module information.
    for (const auto& [remote_source, modules_info] : m_sources_modules) {
        for (const sword::info& module_info : modules_info)
        {
            if (not download_required(remote_source, module_info)) {
                logger::plain("No download required for SWORD module", module_info.abbrev, "because it is up-to-date");
                continue;
            }
            install_module(remote_source, module_info);
            import_module(remote_source, module_info);
            uninstall_module(module_info);
        }
    }
}


bool Sword::download_required(const std::string& remote_source, const sword::info& info) const
{
    // If the module is not in the Store: Download required.
    if (not m_store.get_module(store::Type::sword, remote_source, info.abbrev))
        return true;

    // From here on the module is available in the Store.
    const store::Module module = m_store.get_module(store::Type::sword, remote_source, info.abbrev).value();

    // If version number online differs from the one in Store: Download required.
    if (info.version != module.version)
        return true;

    // Get the module identifier from the Store.
    // Check whether the module file is available from disk.
    // If not: Download required.
    data::Data data;
    data.id(module.id);
    if (not std::filesystem::is_regular_file(data.storage_path()))
        return true;

    // If the module file cannot be read: Download required.
    try
    {
        data.load(std::nullopt);
    }
    catch (const std::exception& exception)
    {
        logger::plain("Cannot read the module file:", exception.what());
        return true;
    }

    return false; // No download required.
}


void Sword::install_module(const std::string& remote_source, const sword::info& info)
{
    logger::plain ("Install SWORD module", info.abbrev, "from source", remote_source);

    // Installation through SWORD InstallMgr binary "installmgr" works
    // when running it from the ~/.sword/InstallMgr directory.
    const std::string command = "cd " + get_sword_install_mgr_path() + "; installmgr --allow-internet-access-and-risk-tracing-and-jail-or-martyrdom --allow-unverified-tls-peer -ri \"" + remote_source + "\" \"" + info.abbrev + "\"";
    logger::plain(command);
    utilities::shell_run(command, out_err);
    utilities::trim(out_err);
    logger::plain(out_err);
}


constexpr auto corrupt_data_during_decompression {"corrupt data during decompression"};


void Sword::import_module(const std::string& remote_source, const sword::info& info) // Todo write it.
{
    logger::plain("Importing SWORD module", info.name, "version", info.version);

    // The data for the entire module.
    data::Data data{};

    // Run "diatheke" from the SWORD installation directory, else it fails.
    const std::string sword_path = get_sword_install_mgr_path();

    // The server fetches the module text as follows: diatheke -b KJV -k Jn 3:16
    // This gets the entire book of John: diatheke -b NETfree -o -n -k Jn
    // To include more notes, run this instead: $ diatheke -b KJV -o n -k Jn 3:16

    for (const auto book_type : {books::BookType::OldTestament, books::BookType::NewTestament})
    {
        for (const auto& osis : books::get_all_osis(book_type))
        {
            logger::plain("Extracting from", osis);
            // Build and run the command to fetch text for the entire book.
            std::string parameters;
            parameters.append("-b " + info.abbrev + " -o ");
            parameters.append("n"); // Keep OSIS content.
            // Hebrew cantillation / Hebrew vowels / Greek accents / Arabic vowels / Arabic shaping.
            parameters.append("cvapr");
            parameters.append(" -k ");
            parameters.append(osis);
            if (const int result = utilities::shell_run("cd " + sword_path + "; diatheke " + parameters, out_err); result != 0)
            {
                logger::plain("Error fetching SWORD data from module", info.abbrev, "book", osis, ":", out_err);
                continue; // Try next book.
            }

            // If the module has not been installed, the output of "diatheke" will be empty.
            if (out_err.empty())
            {
                logger::plain("Skipping module", info.abbrev, "because it is not installed");
                return; // Skip this module.
            }

            // If the module was encrypted, the output of "diatheke" displays a decompression error.
            if (out_err.find(corrupt_data_during_decompression) != std::string::npos)
            {
                logger::plain("Skipping module", info.abbrev, "because it is encrypted");
                return; // Skip this module.
            }

            // If the module was installed, but the requested book is out of range,
            // the output of "diatheke" contains the module name.
            if (out_err.size() < info.abbrev.size() + 10)
            {
                logger::plain("The book", osis, "is not available in module", info.abbrev);
                continue; // Try next book.
            }

            // There's a large block of text for one book.
            // This consists of multiple lines like this:
            // Malachi 3:1: <verse osisID="Mal.3.1">Behold, I send forth My messenger, and he shall survey the way before Me: and the Lord, whom you seek, shall suddenly come into His temple, even the Messenger of the covenant, whom you take pleasure in: behold, He is coming, says the Lord Almighty.
            // Split the text block into lines.
            std::vector<std::string> lines = utilities::explode_lines(out_err, '\n');

            // Find the shortest required book chapter:verse size.
            const auto get_shortest_passage_signature = []
            {
                const auto osis_ot = get_all_osis(books::BookType::OldTestament);
                const auto it1 = std::ranges::min_element(osis_ot, {}, &std::string::size);
                const auto osis_nt = get_all_osis(books::BookType::NewTestament);
                const auto it2 = std::ranges::min_element(osis_nt, {}, &std::string::size);
                // Add chapter:verse like "Genesis 1:1: ".
                // Shortest text is "Jesus wept" -> 10 characters.
                return std::min(it1->size(), it2->size()) + 6 + 10;
            };
            const std::size_t shortest_passage_signature = get_shortest_passage_signature();

            for (auto& line : lines)
            {
                // Clean it up.
                utilities::trim(line);

                // If the line is so short that no verse text could have been there, skip it.
                if (line.size() <= shortest_passage_signature)
                    continue;

                // Fetch the passage, if not, record that and skip the line.
                const std::optional<books::Passage> passage = books::get_passage(line);
                if (not passage)
                {
                    logger::plain("No passage found in:", line);
                    continue; // Skip this line.
                }

                // Get the length of the passage signature to skip to get the verse text. Todo fix this.
                const size_t skip = passage->book_name.size() + 1 + std::to_string(passage->chapter).size() + 1 + std::to_string(passage->verse).size() + 1;
                line.erase(0, skip);

                // Process the XML and so on in the line, in the order below.
                line = utilities::replace_xml_elements(line);
                line = utilities::strip_xml_elements(line);
                utilities::remove_double_spaces(line);
                utilities::trim(line);
                data.add_or_replace(
                    data::VerseText{
                        .book = passage->book_id,
                        .chapter = passage->chapter,
                        .verse = passage->verse,
                        .text = line,
                        .case_fold = utilities::unicode_case_fold(line),
                    }
                );
            }
            break; // return; // Todo
        }
    }

    std::optional<store::Module> module = m_store.get_module(store::Type::sword, remote_source, info.abbrev);
    int module_id{0};
    if (module)
        module_id = module->id;
    else
        module_id = utilities::get_next_id();
    data.id(module_id);
    data.save(std::nullopt); // Todo unify paths between this and the one in check download required.

    store::Module new_module {
        .id = module_id,
        .type = store::Type::sword,
        .source = remote_source,
        .abbrev = info.abbrev,
        .version = info.version,
        .name = info.name,
    };
    m_store.add_or_update(new_module);

    m_store.save_to_file();
}


void Sword::uninstall_module(const sword::info& info)
{
    logger::plain ("Uninstall SWORD module", info.abbrev);
    const std::string command = "cd " + get_sword_install_mgr_path() + "; installmgr -u \"" + info.abbrev + "\"";
    utilities::shell_run(command, out_err);
    utilities::trim(out_err);
    logger::plain (out_err);
}


namespace sword {

constexpr std::string_view whitespace{" \t\r\n"};

static constexpr void skip_whitespace(std::string_view& s) noexcept
{
    s.remove_prefix(std::min(s.find_first_not_of(whitespace), s.size()));
}

// Consumes "<open>text<close>" from the front of line and returns text.
static constexpr std::optional<std::string_view>
take_delimited(std::string_view& line, const char open, const char close) noexcept
{
    if (line.empty() or line.front() != open)
        return std::nullopt;
    const auto end = line.find(close, 1);
    if (end == std::string_view::npos)
        return std::nullopt;
    const auto inner = line.substr(1, end - 1);
    line.remove_prefix(end + 1);
    return inner;
}


std::optional<info> parse (std::string_view line) noexcept
{
    skip_whitespace(line);

    if (not line.empty() and line.front() == '*') // the leading '*' is optional
        line.remove_prefix(1);

    skip_whitespace(line);

    const auto id = take_delimited(line, '[', ']');
    if (not id)
        return std::nullopt;

    skip_whitespace(line);

    const auto version = take_delimited(line, '(', ')');
    if (not version)
        return std::nullopt;

    skip_whitespace(line);

    if (line.empty() or line.front() != '-')
        return std::nullopt;
    line.remove_prefix(1);

    skip_whitespace(line);

    const auto last = line.find_last_not_of(whitespace); // trim the right end
    if (last == std::string_view::npos)
        return std::nullopt; // no name after the dash
    line = line.substr(0, last + 1);

    return info {.abbrev = std::string(*id), .version = std::string(*version), .name = std::string(line)};
}


} // Namespace
