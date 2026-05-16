#include "LoadReplay.hpp"
#include "../gdr/Parser.hpp"
#include "../store/ReplayStore.hpp"

#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/file.hpp>
#include <fstream>

using namespace geode::prelude;

namespace hooks {

void openReplayFilePicker() {
    async::spawn(
        file::pick(
            file::PickMode::OpenFile,
            file::FilePickOptions{
                .filters = {file::FilePickOptions::Filter{
                    .description = "GDR Replay",
                    .files = {"*.gdr", "*.gdr2"},
                }},
            }
        ),
        [](Result<std::optional<std::filesystem::path>> res) {
            if (!res.isOk()) {
                FLAlertLayer::create("GDR Pattern", res.unwrapErr().c_str(), "OK")->show();
                return;
            }

            auto const& pathOpt = res.unwrap();
            if (!pathOpt.has_value()) {
                return;
            }

            auto path = pathOpt.value();
            std::ifstream in(path, std::ios::binary);
            if (!in) {
                FLAlertLayer::create("GDR Pattern", "Could not read file.", "OK")->show();
                return;
            }

            std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                                       std::istreambuf_iterator<char>());

            try {
                auto replay = gdr::parseReplay(bytes);
                store::ReplayStore::get().setReplay(std::move(replay), path.filename().string());
                store::ReplayStore::get().setLastPath(path.string());

                std::string msg = "Loaded " + store::ReplayStore::get().fileName();
                if (!store::ReplayStore::get().replay().levelName.empty()) {
                    msg += "\nLevel: " + store::ReplayStore::get().replay().levelName;
                }
                msg += "\n\nPause in a level and tap Pattern.";
                FLAlertLayer::create("GDR Pattern", msg.c_str(), "OK")->show();
            } catch (std::exception const& e) {
                FLAlertLayer::create("GDR Pattern", e.what(), "OK")->show();
            }
        }
    );
}

} // namespace hooks
