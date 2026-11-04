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
#include "data.h"
#include "exception.h"
#include "logger.h"
#include "utilities.h"
#include "books/books.h"
#include <swmgr.h>
#include <installmgr.h>
#include <filemgr.h>
#include <remotetrans.h>
#include <swmodule.h>
#include <versekey.h>



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
    // Todo via SWORD API the same as above.
    // sword::SWBuf baseDir = sword_logic_get_path ().c_str ();
    // sword::InstallMgr *installMgr = new sword::InstallMgr (baseDir, NULL);
    // installMgr->setUserDisclaimerConfirmed (true);
    // if (installMgr->refreshRemoteSourceConfiguration()) {
    //     success = false;
    // }
    // delete installMgr;
}


// List the remote sources.
void Sword::list_remote_sources()
{
    logger::plain("List of the remote SWORD sources:");
    const sword::SWBuf base_dir = get_sword_install_mgr_path().c_str ();
    sword::InstallMgr install_manager (base_dir, nullptr);
    install_manager.setUserDisclaimerConfirmed (true);
    for (const sword::InstallSource* install_source : install_manager.sources | std::views::values) {
        logger::plain(install_source->caption);
        // logger::plain("Type", install_source->type);
        // logger::plain("Source", install_source->source);
        // logger::plain("Directory", install_source->directory);
    }
}


void Sword::remote_source(const std::string_view remote_source)
{
    m_remote_source = remote_source;
}


void Sword::fetch_modules()
{
    logger::plain("Fetching SWORD modules");
    {
        std::ostringstream command {};
        command << "installmgr --allow-internet-access-and-risk-tracing-and-jail-or-martyrdom --allow-unverified-tls-peer -r " << std::quoted(m_remote_source);
        utilities::shell_run(std::move(command).str(), out_err);
    }
    utilities::trim(out_err);
    logger::plain(out_err);

    sword::SWBuf base_dir = get_sword_install_mgr_path().c_str();
    sword::InstallMgr install_manager(base_dir, nullptr);
    install_manager.setUserDisclaimerConfirmed (true);
    auto source_iter = install_manager.sources.find(m_remote_source.c_str ());
    if (source_iter == install_manager.sources.cend())
        throw Base ("Could not find remote source", m_remote_source);
    sword::SWMgr sword_manager;
    sword::SWMgr* other_manager = source_iter->second->getMgr();
    if (not other_manager)
        other_manager = std::addressof(sword_manager);
    const std::map<sword::SWModule*, int> mods = sword::InstallMgr::getModuleStatus(sword_manager, *other_manager);
    for (const auto& [module_ptr, module_status] : mods)
    {
        const sword::SWModule* module = module_ptr;

        std::string name = module->getName();
        std::string version = module->getConfigEntry("Version");
        std::string description = module->getDescription();

        // No CipherKey entry: the module is not encrypted.
        // Skip encrypted module (no CipherKey to decrypt it).
        if (module->getConfigEntry("CipherKey"))
        {
            logger::plain("Skipping encrypted module", name, version, description);
            continue;
        }

        // Skip module if it is not verse-based (Bible or commentary).
        const auto verse_based = [module]
        {
            const std::string module_type = module->getType();
            if (module_type == "Biblical Texts")
                return true;
            if (module_type == "Commentaries")
                return true;
            return false;
        };
        if (not verse_based())
        {
            logger::plain("Skipping non-verse based module", name, version, description);
            continue;
        }

        logger::plain("Consider module", name, version, description);
        sword::info info {
            .abbrev = std::move(name),
            .version = std::move(version),
            .name = std::move(description)
        };
        m_modules.push_back(std::move(info));
    }

    logger::plain(m_remote_source, ":", m_modules.size(), "modules");
}


void Sword::leave_or_download_or_update_modules()
{
    // Iterate over the module information.
    for (const sword::info& module_info : m_modules) {
        if (not download_required(module_info)) {
            logger::plain("No download required for module", module_info.abbrev, "because it is up-to-date");
            continue;
        }
        install_module(module_info);
        import_module(module_info);
        uninstall_module(module_info);
    }
}


bool Sword::download_required(const sword::info& info) const
{
    // If the module is not in the Store: Download required.
    if (not m_store.get_module(store::Type::sword, m_remote_source, info.abbrev))
        return true;

    // From here on the module is available in the Store.
    const store::Module module = m_store.get_module(store::Type::sword, m_remote_source, info.abbrev).value();

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


namespace {
class MyStatusReporter : public sword::StatusReporter {
public:
    // Called repeatedly while a file is being downloaded.
    void update(const unsigned long total_bytes, const unsigned long completed_bytes) override {
        const int percent = total_bytes ? static_cast<int>(completed_bytes * 100 / total_bytes) : 0;
        std::cout << "\rProgress: " << percent << "% (" << completed_bytes << "/" << total_bytes << ")" << std::flush;
    }
    // Called before each file download starts.
    void preStatus(const long total_bytes, [[maybe_unused]] const long completed_bytes, const char *message) override {
        std::cout << "\n" << (message ? message : "") << " " << total_bytes << " bytes" << std::endl;
    }
};
}


void Sword::install_module(const sword::info& info)
{
    logger::plain ("Install SWORD module", info.abbrev, "from source", m_remote_source);
    // Installation through SWORD InstallMgr binary "installmgr" works
    // when running it from the ~/.sword/InstallMgr directory.
    // std::ostringstream command;
    // command << "cd " << get_sword_install_mgr_path() << "; ";
    // command << "installmgr --allow-internet-access-and-risk-tracing-and-jail-or-martyrdom --allow-unverified-tls-peer -ri " << std::quoted(m_remote_source) << " " << std::quoted(info.abbrev);
    // logger::plain(command.str());
    // system(command.str().c_str());

    const sword::SWBuf base_dir = get_sword_install_mgr_path().c_str ();
    sword::SWMgr sword_mgr (base_dir); // Works if if path set.
    MyStatusReporter reporter;
    sword::InstallMgr installMgr (base_dir, &reporter);
    installMgr.setUserDisclaimerConfirmed (true);
    const auto source = installMgr.sources.find(m_remote_source.c_str());
    if (source == installMgr.sources.end())
    {
        logger::plain("Could not find remote source", m_remote_source);
        return;
    }
    sword::InstallSource *install_source = source->second;
    sword::SWMgr *remote_manager = install_source->getMgr();
    const auto module_iter = remote_manager->Modules.find(info.abbrev.c_str());
    if (module_iter == remote_manager->Modules.end())
    {
        logger::plain ("Remote source", m_remote_source, "does not make available module", info.abbrev);
        return;
    }
    const sword::SWModule *module = module_iter->second;
    if (const int error = installMgr.installModule(&sword_mgr, nullptr, module->getName(), install_source);
        error != 0)
    {
        logger::plain ("Error installing module", info.abbrev);
        return;
    }
    logger::plain ("Installed module", info.abbrev);
}


constexpr auto corrupt_data_during_decompression {"corrupt data during decompression"};


void Sword::import_module(const sword::info& info) const
{
    logger::plain("Importing module", info.name, "version", info.version);

    // The data for the entire module.
    data::Data data{};

    // The SWORD manager should be pointed to the path of the library, else it fails.
    sword::SWMgr sword_manager (get_sword_install_mgr_path().c_str ());

    // Set options that influence the text content.
    sword_manager.setGlobalOption("Footnotes", "Off");
    sword_manager.setGlobalOption("Headings", "Off");
    sword_manager.setGlobalOption("Strong's Numbers", "Off");
    sword_manager.setGlobalOption("Morphological Tags", "Off");
    sword_manager.setGlobalOption("Hebrew Cantillation", "On");
    sword_manager.setGlobalOption("Hebrew Vowel Points", "On");
    sword_manager.setGlobalOption("Greek Accents", "On");
    sword_manager.setGlobalOption("Lemmas", "Off");
    sword_manager.setGlobalOption("Cross-references", "Off");
    sword_manager.setGlobalOption("Words of Christ in Red", "Off");
    sword_manager.setGlobalOption("Arabic Vowel Points", "On");
    sword_manager.setGlobalOption("Glosses", "Off");
    sword_manager.setGlobalOption("Transliterated Forms", "Off");
    sword_manager.setGlobalOption("Enumerations", "Off");
    sword_manager.setGlobalOption("Transliteration", "Off");
    sword_manager.setGlobalOption("Textual Variants", "All Readings");
    //manager.setGlobalOption("Textual Variants", "Secondary Reading");
    //manager.setGlobalOption("Textual Variants", "Primary Reading");

    sword::SWModule* module = sword_manager.getModule(info.abbrev.c_str());
    if (not module)
    {
        logger::plain("Module", info.abbrev, "is not available");
        return;
    }

    // Use the module's own key, so the versification matches the module.
    auto *verse_key = dynamic_cast<sword::VerseKey *>(module->getKey());
    if (not verse_key)
    {
        logger::plain("Module", info.abbrev, "does not use a verse key");
        return;
    }

    // Don't let setters jump to neighbouring books.
    verse_key->setAutoNormalize(false);

    // Iterate over both Testaments.
    // The Old Testament may also contain Apocrypha for some verse systems.
    for (char testament : std::initializer_list<char>{1, 2})
    {
        verse_key->setTestament(testament);
        // Number of books in this testament.
        const int max_book = verse_key->getBookMax();
        for (int book = 1; book <= max_book; ++book)
        {
            verse_key->setBook(static_cast<char>(book));

            // Info about book: name and id.
            //const std::string name = verse_key->getBookName();
            const std::string module_osis_book_name = verse_key->getOSISBookName();
            const int book_id = books::get_id_fuzzy(module_osis_book_name);
            logger::plain("Extracting from", module_osis_book_name);

            // Number of chapters in this book.
            const int max_chapter = verse_key->getChapterMax();
            for (int chapter = 1; chapter <= max_chapter; ++chapter)
            {
                verse_key->setChapter(chapter);

                // Number of verses in this chapter.
                const int max_verse = verse_key->getVerseMax();
                for (int verse = 1; verse <= max_verse; ++verse)
                {
                    verse_key->setVerse(verse);
                    // The versification may define books the module does not contain, e.g. an NT-only module.
                    if (not module->hasEntry(verse_key))
                        continue;

                    // Get verse text.
                    std::string rendering = module->renderText().c_str();

                    // Process the XML and so on in the line, in the order below.
                    rendering = utilities::replace_xml_elements(rendering);
                    rendering = utilities::strip_xml_elements(rendering);
                    utilities::remove_double_spaces(rendering);
                    utilities::trim(rendering);
                    data.add_or_replace(
                        data::VerseText{
                            .book = book_id,
                            .chapter = chapter,
                            .verse = verse,
                            .text = rendering,
                        }
                    );
                }
            }
        }
    }

    // If a module was downloaded or re-downloaded, it always gets a new UUID.
    const std::string uuid = utilities::get_uuid();

    const auto get_module_id = [&]
    {
        const std::optional<store::Module> module = m_store.get_module(store::Type::sword, m_remote_source, info.abbrev);
        if (module)
            return module->id;
        return utilities::get_next_id();
    };
    const int module_id {get_module_id()};
    data.id(module_id);
    data.uuid(uuid);
    data.save(std::nullopt); // Todo unify paths between this and the one in check download required.

    store::Module new_module {
        .id = module_id,
        .type = store::Type::sword,
        .source = m_remote_source,
        .abbrev = info.abbrev,
        .version = info.version,
        .name = info.name,
        .uuid = uuid,
    };
    m_store.add_or_update(new_module);

    m_store.save_to_file();
}


void Sword::uninstall_module(const sword::info& info)
{
    logger::plain ("Uninstall SWORD module", info.abbrev);
    const sword::SWBuf base_dir = get_sword_install_mgr_path().c_str ();
    sword::SWMgr sword_mgr (base_dir);
    sword::InstallMgr installMgr (base_dir, nullptr);
    const auto module_iter = sword_mgr.Modules.find(info.abbrev.c_str());
    if (module_iter == sword_mgr.Modules.end())
    {
        logger::plain ("Module", info.abbrev, "is not installed");
        return;
    }
    const sword::SWModule *module = module_iter->second;
    if (const int error = installMgr.removeModule(&sword_mgr, module->getName()); error != 0)
    {
        logger::plain ("Error uninstalling module", info.abbrev);
        return;
    }
    logger::plain ("Uninstalled module", info.abbrev);
}


void Sword::develop()
{
    logger::plain("Develop");
    remote_source("CrossWire");
    const sword::info info {
        .abbrev = "AB",
        .version = "1.0",
        .name = "Develop"
    };
//    install_module(info);
//    uninstall_module(info);
    // import_module(info);
}
