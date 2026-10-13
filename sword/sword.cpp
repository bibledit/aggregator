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
            if (not download_required(remote_source, module_info))
                continue;
            install_module(remote_source, module_info);
            import_module(module_info);
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

    // If version number online differs from one in Store: Download required.
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


void Sword::import_module(const sword::info& info) // Todo write it.
{
    logger::plain("Importing SWORD module", info.name, "version", info.version);

    // Run "diatheke" from the SWORD installation directory, else it fails.
    const std::string sword_path = get_sword_install_mgr_path();

    // The server fetches the module text as follows: diatheke -b KJV -k Jn 3:16
    // This gets the entire book of John: diatheke -b NETfree -o -n -k Jn
    // To include more notes, run this instead: $ diatheke -b KJV -o n -k Jn 3:16

    constexpr std::array book_types {books::BookType::OldTestament, books::BookType::NewTestament};
    for (const auto book_type : book_types)
    {
        for (const auto& osis : books::get_all_osis(book_type))
        {
            std::string parameters;
            parameters.append("-b " + info.abbrev + " -o ");
            parameters.append("n"); // Keep OSIS content.
            // Hebrew cantillation / Hebrew vowels / Greek accents / Arabic vowels / Arabic shaping.
            parameters.append("cvapr");
            parameters.append(" -k ");
            parameters.append(osis);
            if (const int result = utilities::shell_run("cd " + sword_path + "; diatheke " + parameters, out_err); result != 0)
            {
                logger::plain(out_err);
                throw Base("Error fetching SWORD data from module", info.abbrev, "book", osis);
            }
        }
    }
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


}


//
//   // If the module has not been installed, the output of "diatheke" will be empty.
//   // If the module was installed, but the requested passage is out of range,
//   // the output of "diatheke" contains the module name, so it won't be empty.
//   module_available = !module_text.empty ();
//
//   if (!module_available) {
//
//     // Check whether the SWORD module exists.
//     std::vector <std::string> modules {sword_logic_get_available ()};
//     const std::string smodules = filter::string::implode (modules, std::string());
//     if (smodules.find ("[" + module + "]") != std::string::npos) {
//       // Schedule SWORD module installation.
//       // (It used to be the case that this function, to get the text,
//       // would wait till the SWORD module was installed, and then after installation,
//       // return the text from that module.
//       // But due to long waiting on Bibledit demo, while it would install multiple modules,
//       // the Bibledit demo would become unresponsive.
//       // So, it's better to return immediately with an informative text.)
//       sword_logic_install_module_schedule (source, module);
//       // Return standard 'installing' information. Client knows not to cache this.
//       return sword_logic_installing_module_text ();
//     } else {
//       return "Cannot find SWORD module " + module;
//     }
//   }
//
//   // Clean it up.
//   module_text = sword_logic_clean_verse (module, chapter, verse, module_text);
//
//   return module_text;
//
// #endif
// }
//
//
// std::map <int, std::string> sword_logic_get_bulk_text (const std::string& module, const int book, const int chapter, const std::vector <int>& verses)
// {
//   // Touch the cache so the server knows that the module has been accessed and won't uninstall it too soon.
//   {
//     const std::string path = sword_logic_access_tracker (module);
//     filter_url_file_put_contents (path, "bulk");
//   }
//
//   // The name of the book to pass to diatheke.
//   const std::string osis = database::books::get_osis_from_id (static_cast<book_id>(book));
//
//   // Cannot run more than one "diatheke" per user, so use a mutex for that.
//   sword_logic_diatheke_run_mutex.lock ();
//
//   // Here is how to speed up SWORD text retrieval.
//   // The main point is to not pass just one verse,
//   // but to pass the chapter number without the verse.
//   // So it returns the entire chapter in one go.
//   // One would even be able to pass the book only, so it returns the entire book.
//   // But that would not add much to increasing the speed.
//   // cd ~/.sword/InstallMgr
//   // diatheke -b AB -k Ezra 5:1
//   // diatheke -b AB -k Ezra 5
//   // diatheke -b AB -k Ezra
//   std::string error {};
//   std::string bulk_text {};
//   const int result = filter::shell::run (sword_logic_get_path (), std::string(filter::shell::get_executable(filter::shell::Executable::diatheke)), { "-b", module, "-k", osis, std::to_string (chapter) }, &bulk_text, &error);
//   bulk_text.append (error);
//   if (result != 0) logger::plain (error);
//   // This is how the output would look.
//   // Malachi 3:1: <verse osisID="Mal.3.1">Behold, I send forth My messenger, and he shall survey the way before Me: and the Lord, whom you seek, shall suddenly come into His temple, even the Messenger of the covenant, whom you take pleasure in: behold, He is coming, says the Lord Almighty.
//
//   sword_logic_diatheke_run_mutex.unlock ();
//
//   // Resulting verse text.
//   std::map <int, std::string> output {};
//
//   // Iterate over all requested verses to extract the correct content from the chapter.
//   // This works well in general.
//   // It has been seen in a sample module, the "AB", that some verses in the SWORD module were empty.
//   // In case of such verses, there's no content to extract from the chapter.
//   // The cause in such verses is in the module builder.
//   for (const auto verse : verses) {
//     const std::string starter = " " + std::to_string(chapter) + ":" + std::to_string(verse) + ":";
//     size_t pos1 = bulk_text.find (starter);
//     if (pos1 == std::string::npos) {
//       //logger::plain("Cannot find starter: |" + starter + "|");
//       continue;
//     }
//     const std::string finisher = "\n";
//     size_t pos2 = bulk_text.find (finisher, pos1);
//     if (pos2 == std::string::npos) pos2 = bulk_text.length() + 1;
//     pos1 += starter.length ();
//     std::string text = bulk_text.substr (pos1, pos2 - pos1);
//     text = sword_logic_clean_verse (module, chapter, verse, text);
//     output [verse] = text;
//   }
//
//   // Done.
//   return output;
// }
//

// std::string sword_logic_clean_verse (const std::string& module, int chapter, int verse, std::string text)
// {
//   // Remove any OSIS elements or make those elements displayable.
//   if (database::config::general::get_keep_osis_content_in_sword_resources ()) {
//     text = filter::string::escape_special_xml_characters (text);
//   } else {
//     filter::string::replace_between (text, "<", ">", "");
//   }
//
//   // Remove the passage name that diatheke adds.
//   // A reliable signature for this is the chapter and verse plus subsequent colon.
//   const std::string chapter_verse = std::to_string (chapter) + ":" + std::to_string (verse);
//   size_t pos = text.find (" " + chapter_verse + ":");
//   if (pos != std::string::npos) {
//     pos += 2;
//     pos += chapter_verse.size ();
//     text.erase (0, pos);
//   }
//
//   // Remove the module name that diatheke adds.
//   text = filter::string::replace ("(" + module + ")", "", text);
//
//   // Clean whitespace away.
//   text = filter::string::trim (text);
//
//   // Done.
//   return text;
// }
