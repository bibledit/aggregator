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


#include <filesystem>
#include "sword.h"

#include <iostream>
#include <string>
#include "logger.h"
#include "utilities.h"


[[nodiscard]] static std::string get_path()
{
    return std::filesystem::path{utilities::home_directory()} / ".sword" / "InstallMgr";
}


void Sword::initialize()
{
    logger::plain("Initialize SWORD structures");

    // Initialize SWORD directory structure and configuration.
    const std::string sword_path = get_path();
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
                store::Module module {
                    .id = 0, // Todo
                    .type = store::Type::sword,
                    .source = remote_source,
                    .abbrev = std::string(info.value().id),
                    .version = std::string(info.value().version),
                    .name = std::string(info.value().name)
                };
                m_store.add_or_update(module);
                logger::plain(line);
            }
        }
        logger::plain(remote_source, ":", m_store.count(store::Type::sword, remote_source), "modules");
    }
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


static constexpr std::optional<info> parse_impl (std::string_view line) noexcept
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

    return info {.id = *id, .version = *version, .name = line};
}


// Static unit tests.
namespace {

constexpr std::string_view line1 {"*[ymp2025eb]  	(2.7)  	- Yamap"};
static_assert(parse_impl(line1)->id == "ymp2025eb");
static_assert(parse_impl(line1)->version == "2.7");
static_assert(parse_impl(line1)->name == "Yamap");

constexpr std::string_view line2 {"*[peg2020eb]  	(4.7)  	- ସତ୍‌ ବଚନ୍"};
static_assert(parse_impl(line2)->id == "peg2020eb");
static_assert(parse_impl(line2)->version == "4.7");
static_assert(parse_impl(line2)->name == "ସତ୍‌ ବଚନ୍");

constexpr std::string_view line3 {"*[pan2017eb]  	(21.37)  	- ਇੰਡਿਅਨ ਰਿਵਾਇਜ਼ਡ ਵਰਜ਼ਨ (IRV) - ਪੰਜਾਬੀ"};
static_assert(parse_impl(line3)->id == "pan2017eb");
static_assert(parse_impl(line3)->version == "21.37");
static_assert(parse_impl(line3)->name == "ਇੰਡਿਅਨ ਰਿਵਾਇਜ਼ਡ ਵਰਜ਼ਨ (IRV) - ਪੰਜਾਬੀ");

constexpr std::string_view line4 {"*[ABSMaps]  	(1.071229)  	- Maps by American Bible Society (1888)"};
static_assert(parse_impl(line4)->id == "ABSMaps");
static_assert(parse_impl(line4)->version == "1.071229");
static_assert(parse_impl(line4)->name == "Maps by American Bible Society (1888)");

constexpr std::string_view line5 {"id]  	(1.1)  	- Name"};
static_assert(not parse_impl(line5));
constexpr std::string_view line6 {"[id  	(1.1)  	- Name"};
static_assert(not parse_impl(line6));
constexpr std::string_view line7 {"[id]  	1.1)  	- Name"};
static_assert(not parse_impl(line7));
constexpr std::string_view line8 {"[id]  	(1.1  	- Name"};
static_assert(not parse_impl(line8));

}


std::optional<info> parse (const std::string_view line) noexcept
{
    return parse_impl(line);
}

}



// // Gets the name of the remote source of the $line like this:
// // [CrossWire] *[Shona] (1.1) - Shona Bible
// std::string sword_logic_get_source (std::string line)
// {
//   if (line.length () < 10) return std::string();
//   line.erase (0, 1);
//   size_t pos = line.find ("]");
//   if (pos == std::string::npos) return std::string();
//   line.erase (pos);
//   return line;
// }
//
//
// // Gets the module name of the $line like this:
// // [CrossWire] *[Shona] (1.1) - Shona Bible
// std::string sword_logic_get_remote_module (std::string line)
// {
//   if (line.length () < 10) return std::string();
//   line.erase (0, 2);
//   if (line.length () < 10) return std::string();
//   size_t pos = line.find ("[");
//   if (pos == std::string::npos) return std::string();
//   line.erase (0, pos + 1);
//   pos = line.find ("]");
//   if (pos == std::string::npos) return std::string();
//   line.erase (pos);
//   return line;
// }
//
//
// // Gets the module name of the $line like this:
// // [Shona]  (1.1)  - Shona Bible
// std::string sword_logic_get_installed_module (std::string line)
// {
//   line = filter::string::trim (line);
//   if (line.length () > 10) {
//     line.erase (0, 1);
//     size_t pos = line.find ("]");
//     if (pos != std::string::npos) line.erase (pos);
//   }
//   return line;
// }
//
//
// // Gets the version number of a module of the $line like this:
// // [Shona]  (1.1)  - Shona Bible
// std::string sword_logic_get_version (std::string line)
// {
//   line = filter::string::trim (line);
//   if (line.length () > 10) {
//     line.erase (0, 3);
//   }
//   if (line.length () > 10) {
//     size_t pos = line.find ("(");
//     if (pos != std::string::npos) line.erase (0, pos + 1);
//     pos = line.find (")");
//     if (pos != std::string::npos) line.erase (pos);
//   }
//   return line;
// }
//
//
// // Gets the human-readable name of a $line like this:
// // [CrossWire] *[Shona] (1.1) - Shona Bible
// std::string sword_logic_get_name (std::string line)
// {
//   std::vector <std::string> bits = filter::string::explode (line, '-');
//   if (bits.size () >= 2) {
//     bits.erase (bits.begin ());
//   }
//   line = filter::string::implode (bits, "-");
//   line = filter::string::trim (line);
//   return line;
// }
//
//
// // Schedule SWORD module installation.
// void sword_logic_install_module_schedule (const std::string& source, const std::string& module)
// {
//   // No source: Done.
//   if (source.empty ()) return;
//
//   // No module: Done.
//   // There have been cases with more than 6000 scheduled SWORD module installation tasks,
//   // all trying to install an empty $module.
//   // So it's important to check on that.
//   if (module.empty ()) return;
//
//   // Check whether the module installation has been scheduled already.
//   if (tasks::tasks_logic_queued (tasks::enums::task::install_sword_module, {source, module})) return;
//
//   // Schedule it.
//   tasks::tasks_logic_queue (tasks::enums::task::install_sword_module, {source, module});
// }
//
//
// void sword_logic_install_module (const std::string& source_name, const std::string& module_name)
// {
//   logger::plain ("Install SWORD module", module_name, "from source", source_name);
//   std::string sword_path {sword_logic_get_path ()};
//
//   // Installation through SWORD InstallMgr does not yet work.
//   // When running it from the ~/.sword/InstallMgr directory, it works.
// #ifdef HAVE_SWORD
//
//   sword::SWMgr *mgr = new sword::SWMgr();
//
//   sword::SWBuf baseDir = sword_logic_get_path ().c_str ();
//
//   sword::InstallMgr *installMgr = new sword::InstallMgr (baseDir, NULL);
//   installMgr->setUserDisclaimerConfirmed (true);
//
//   sword::InstallSourceMap::iterator source = installMgr->sources.find(source_name.c_str ());
//   if (source == installMgr->sources.end()) {
//     logger::plain ("Could not find remote source", source_name);
//   } else {
//     sword::InstallSource *is = source->second;
//     sword::SWMgr *rmgr = is->getMgr();
//     sword::SWModule *module;
//     sword::ModMap::iterator it = rmgr->Modules.find(module_name.c_str());
//     if (it == rmgr->Modules.end()) {
//       logger::plain ("Remote source", source_name, "does not make available module", module_name);
//     } else {
//       module = it->second;
//       int error = installMgr->installModule(mgr, 0, module->getName(), is);
//       if (error) {
//         logger::plain ("Error installing module", module_name);
//       } else {
//         logger::plain ("Installed module", module_name);
//       }
//     }
//   }
//
//   delete installMgr;
//   delete mgr;
//
// #else
//
//   std::string out_err {};
//   std::string command = "cd " + sword_path + "; " + std::string(filter::shell::get_executable(filter::shell::Executable::installmgr))+ " --allow-internet-access-and-risk-tracing-and-jail-or-martyrdom --allow-unverified-tls-peer -ri \"" + source_name + "\" \"" + module_name + "\"";
//   logger::plain (command);
//   filter::shell::run (command, out_err);
//   sword_logic_log (out_err);
//
// #endif
//
//   // After the installation is complete, write some temporal some data.
//   // This temporal data indicates the last access time for this SWORD module.
//   {
//     const std::string path = sword_logic_access_tracker (module_name);
//     filter_url_file_put_contents (path, "SWORD");
//   }
// }
//
//
// void sword_logic_uninstall_module (const std::string& module)
// {
//   logger::plain ("Uninstall SWORD module", module);
//   std::string out_err;
//   const std::string sword_path {sword_logic_get_path ()};
//   filter::shell::run ("cd " + sword_path + "; " + std::string(filter::shell::get_executable(filter::shell::Executable::installmgr)) + " -u \"" + module + "\"", out_err);
//   sword_logic_log (out_err);
// }
//
//

//
// // Get installed SWORD modules.
// std::vector <std::string> sword_logic_get_installed ()
// {
//   std::vector <std::string> modules {};
//   std::string out_err {};
//   const std::string sword_path {sword_logic_get_path ()};
//   filter::shell::run ("cd " + sword_path + "; " + std::string(filter::shell::get_executable(filter::shell::Executable::installmgr)) + " -l", out_err);
//   std::vector <std::string> lines = filter::string::explode (out_err, '\n');
//   for (auto line : lines) {
//     line = filter::string::trim (line);
//     if (line.empty ()) continue;
//     if (line.find ("[") == std::string::npos) continue;
//     modules.push_back (line);
//   }
//   return modules;
// }
//
//
// std::string sword_logic_get_text (const std::string& source, const std::string& module, const int book, const int chapter, const int verse)
// {
// #ifdef HAVE_CLIENT
//
//   // The resource name consists of source and module, e.g. [CrossWire][NET].
//   std::string resource = sword_logic_get_resource_name (source, module);
//
//   // Client checks for and optionally creates the cache for this SWORD source/module.
//   if (!database::cache::sql::exists (resource, book)) {
//     database::cache::sql::create (resource, book);
//   }
//
//   // If this module/passage exists in the cache, return it (it updates the access days in the cache).
//   if (database::cache::sql::exists (resource, book, chapter, verse)) {
//     return database::cache::sql::retrieve (resource, book, chapter, verse);
//   }
//
//   // Fetch this SWORD resource from the server.
//   std::string address = database::config::general::get_server_address ();
//   int port = database::config::general::get_server_port ();
//   if (!client_logic_client_enabled ()) {
//     // If the client has not been connected to a cloud instance,
//     // fetch the SWORD content from the Bibledit Cloud demo.
//     address = demo_address ();
//     port = demo_port ();
//   }
//   const std::string url = filter_url_build_http_query (client_logic_url (address, port, sync_resources_url()), {
//     {"r", resource},
//     {"b", std::to_string(book)},
//     {"c", std::to_string(chapter)},
//     {"v", std::to_string(verse)},
//   });
//   std::string error {};
//   std::string html = filter_url_http_get (url, error, true);
//
//   // In case of an error, don't cache that error, but let the user see it.
//   if (!error.empty ()) return error;
//
//   // Client caches this info for later.
//   // Except in case of predefined responses from the Cloud.
//   if (html != sword_logic_installing_module_text ()) {
//     if (html != sword_logic_fetch_failure_text ()) {
//       database::cache::sql::cache (resource, book, chapter, verse, html);
//     }
//   }
//
//   return html;
//
// #else
//
//   std::string module_text;
//   bool module_available {false};
//
//   const std::string osis = database::books::get_osis_from_id (static_cast<book_id>(book));
//   const std::string chapter_verse = std::to_string (chapter) + ":" + std::to_string (verse);
//
//   // See notes on function sword_logic_diatheke
//   // for why it is not currently fetching content via a SWORD library call.
//   // module_text = sword_logic_diatheke (module, osis, chapter, verse, module_available);
//
//   // Running diatheke only works when it runs in the SWORD installation directory.
//   const std::string sword_path = sword_logic_get_path ();
//   // Running several instances of diatheke simultaneously fails.
//   sword_logic_diatheke_run_mutex.lock ();
//   // The server fetches the module text as follows:
//   // diatheke -b KJV -k Jn 3:16
//   // To included, run this instead: $ diatheke -b KJV -o n -k Jn 3:16
//   std::vector <std::string> parameters {"-b", module};
//   parameters.push_back("-o");
//   std::string module_options {};
//   if (database::config::general::get_keep_osis_content_in_sword_resources ()) {
//     module_options.append("n");
//   }
//   module_options.append("cvapr"); // Hebrew cantillation / Hebrew vowels / Greek accents / Arabic vowels / Arabic shaping.
//   parameters.push_back(module_options);
//   parameters.push_back("-k");
//   parameters.push_back(osis);
//   parameters.push_back(chapter_verse);
//   std::string error {};
//   const int result = filter::shell::run (sword_path, std::string(filter::shell::get_executable(filter::shell::Executable::diatheke)), parameters, &module_text, &error);
//   module_text.append (error);
//   sword_logic_diatheke_run_mutex.unlock ();
//   if (result != 0) return sword_logic_fetch_failure_text ();
//
//   // Touch the temporal file
//   // so the server knows that the module has been accessed just now
//   // and won't uninstall it too soon.
//   {
//     const std::string path = sword_logic_access_tracker (module);
//     filter_url_file_put_contents (path, "access");
//   }
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
//
// // Checks the installed modules, whether they need to be updated.
// void sword_logic_update_installed_modules ()
// {
//   logger::plain ("Updating installed SWORD modules");
//
//   std::vector <std::string> available_modules = sword_logic_get_available ();
//
//   std::vector <std::string> installed_modules = sword_logic_get_installed ();
//   for (const auto& installed_module : installed_modules) {
//     const std::string module = sword_logic_get_installed_module (installed_module);
//     const std::string installed_version = sword_logic_get_version (installed_module);
//     for (const auto& available_module : available_modules) {
//       if (sword_logic_get_remote_module (available_module) == module) {
//         if (sword_logic_get_version (available_module) != installed_version) {
//           const std::string source = sword_logic_get_source (available_module);
//           // Uninstall module.
//           sword_logic_uninstall_module (module);
//           // Schedule module installation.
//           sword_logic_install_module_schedule (source, module);
//         }
//         continue;
//       }
//     }
//   }
//
//   logger::plain ("Ready updating installed SWORD modules");
// }
//
//
// // Trims the installed SWORD modules.
// void sword_logic_trim_modules ()
// {
// #ifdef HAVE_CLOUD
//   logger::plain ("Trimming the installed SWORD modules");
//   const std::vector <std::string> modules = sword_logic_get_installed ();
//   for (auto module : modules) {
//     module = sword_logic_get_installed_module (module);
//     const std::string path = sword_logic_access_tracker (module);
//     if (!file_or_dir_exists (path)) {
//       sword_logic_uninstall_module (module);
//     }
//   }
//   logger::plain ("Ready trimming the SWORD caches and modules");
// #endif
// }
//
//
// // Tracker for accessing the SWORD module.
// std::string sword_logic_access_tracker (const std::string& module)
// {
//   const std::string path = filter_url_create_root_path ({filter_url_temp_dir (), "sword_access_tracker_" + module});
//   return path;
// }
//
//
// // The functions runs a scheduled module installation.
// // The purpose of this function is that only one module installation occurs at a time,
// // rather than simultaneously installing modules, which clogs the system.
// void sword_logic_run_scheduled_module_install (const std::string& source, const std::string& module)
// {
//   // If a module is being installed,
//   // and a call is made for another module installation,
//   // re-schedule this module installation to postpone it,
//   // till after this one is ready.
//   sword_logic_installer_mutex.lock ();
//   const bool installing = sword_logic_installing_module;
//   sword_logic_installer_mutex.unlock ();
//   if (installing) {
//     sword_logic_install_module_schedule (source, module);
//     return;
//   }
//
//   // Set flag for current module installation running.
//   sword_logic_installer_mutex.lock ();
//   sword_logic_installing_module = true;
//   sword_logic_installer_mutex.unlock ();
//
//   // Run the installer if the module is not yet installed.
//   const std::vector <std::string> modules {sword_logic_get_installed ()};
//   bool already_installed = false;
//   for (const auto& installed_module : modules) {
//     if (installed_module.find ("[" + module + "]") != std::string::npos) {
//       already_installed = true;
//     }
//   }
//   if (!already_installed) {
//     sword_logic_install_module (source, module);
//   }
//
//   // Clear flag as current module installation is ready.
//   sword_logic_installer_mutex.lock ();
//   sword_logic_installing_module = false;
//   sword_logic_installer_mutex.unlock ();
// }
//
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
//
