#include <algorithm>
#include "sprite_atlas.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

namespace KoRama {

bool SpriteLibrary::loadSheet(const std::filesystem::path& spritesDir, const std::string& sheet) {
    if (loadedSheets_.count(sheet)) return true;
    sf::Image img;
    if (!img.loadFromFile(spritesDir / (sheet + ".png"))) {
        std::cerr << "[K.O.-RAMA] Spritesheet ausente: " << sheet << ".png" << std::endl;
        return false;
    }
    const bool ok = addSheet(sheet, std::move(img), spritesDir / (sheet + ".atlas"));
    if (ok && sheet == "jack" && std::filesystem::exists(spritesDir / "jack_skins.atlas"))
        return loadSheet(spritesDir, "jack_skins");
    if (ok && sheet == "frog" && std::filesystem::exists(spritesDir / "frog_skins.atlas"))
        return loadSheet(spritesDir, "frog_skins");
    if (ok && sheet == "joana" && std::filesystem::exists(spritesDir / "joana_skins.atlas"))
        return loadSheet(spritesDir, "joana_skins");
    if (ok && sheet == "marcov" && std::filesystem::exists(spritesDir / "marcov_skins.atlas"))
        return loadSheet(spritesDir, "marcov_skins");
    if (ok && sheet == "valeria" && std::filesystem::exists(spritesDir / "valeria_skins.atlas"))
        return loadSheet(spritesDir, "valeria_skins");
    if (ok && sheet == "vivaldi")
        for (unsigned page = 0; std::filesystem::exists(spritesDir / ("vivaldi_skins_" + std::to_string(page) + ".atlas")); ++page)
            if (!loadSheet(spritesDir, "vivaldi_skins_" + std::to_string(page))) return false;
    if (ok && sheet == "cremoso")
        for (unsigned page = 0; std::filesystem::exists(spritesDir / ("cremoso_skins_" + std::to_string(page) + ".atlas")); ++page)
            if (!loadSheet(spritesDir, "cremoso_skins_" + std::to_string(page))) return false;
    if (ok && sheet == "espinha")
        for(unsigned page=0;std::filesystem::exists(spritesDir/("espinha_skins_"+std::to_string(page)+".atlas"));++page)
            if(!loadSheet(spritesDir,"espinha_skins_"+std::to_string(page)))return false;
    if (ok && sheet == "boss")
        for (unsigned page = 0; std::filesystem::exists(spritesDir / ("boss_skins_" + std::to_string(page) + ".atlas")); ++page)
            if (!loadSheet(spritesDir, "boss_skins_" + std::to_string(page))) return false;
    if(ok && sheet=="portraits" && std::filesystem::exists(spritesDir/"dummy_skins.atlas")) return loadSheet(spritesDir,"dummy_skins");
    return ok;
}

bool SpriteLibrary::addSheet(const std::string& sheet, sf::Image&& image, const std::filesystem::path& atlasPath) {
    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromImage(image)) {
        std::cerr << "[K.O.-RAMA] Spritesheet ausente: " << sheet << ".png\n";
        return false;
    }
    texture->setSmooth(true);
    const unsigned maxSize = sf::Texture::getMaximumSize();
    if (texture->getSize().x > maxSize || texture->getSize().y > maxSize) {
        std::cerr << "[K.O.-RAMA] Aviso: " << sheet << ".png (" << texture->getSize().x << "x" << texture->getSize().y
                  << ") passa do limite de textura da GPU (" << maxSize << ").\n";
    }

    return registerSheet(sheet, std::move(texture), atlasPath);
}

bool SpriteLibrary::registerSheet(const std::string& sheet, std::unique_ptr<sf::Texture> texture, const std::filesystem::path& atlasPath) {
    std::ifstream atlas(atlasPath);
    if (!atlas) {
        std::cerr << "[K.O.-RAMA] Atlas ausente: " << sheet << ".atlas\n";
        return false;
    }

    std::vector<std::pair<std::string, std::string>> aliases;
    std::string line;
    while (std::getline(atlas, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream in(line);
        std::string name;
        in >> name;
        if (name == "alias") {
            std::string alias, target;
            if (in >> alias >> target) aliases.emplace_back(alias, target);
            continue;
        }
        int x, y, w, h;
        if (in >> x >> y >> w >> h) {
            SpriteFrame frame{texture.get(), sf::IntRect({x, y}, {w, h})};
            int ox, oy, fw, fh;
            if (in >> ox >> oy >> fw >> fh) { frame.trimOffset = {ox, oy}; frame.fullSize = {fw, fh}; }
            else frame.fullSize = {w, h};
            frames_[name] = frame;
        }
    }
    for (const auto& [alias, target] : aliases) {
        auto it = frames_.find(target);
        if (it != frames_.end()) frames_[alias] = it->second;
    }

    loadedSheets_.insert(sheet);
    textureSheets_[texture.get()] = sheet;
    textures_.push_back(std::move(texture));
    return true;
}

void SpriteLibrary::loadSheets(const std::filesystem::path& spritesDir, const std::vector<std::string>& sheets) {
    if(std::find(sheets.begin(),sheets.end(),"portraits")!=sheets.end() && std::filesystem::exists(spritesDir/"dummy_skins.atlas"))loadSheet(spritesDir,"dummy_skins");
    std::vector<std::future<std::optional<sf::Image>>> decoded;
    for (const std::string& sheet : sheets)
        decoded.push_back(std::async(std::launch::async, [path = spritesDir / (sheet + ".png")]() -> std::optional<sf::Image> {
            sf::Image img;
            if (!img.loadFromFile(path)) return std::nullopt;
            return img;
        }));
    for (std::size_t i = 0; i < sheets.size(); ++i) {
        auto img = decoded[i].get();
        if (!img) { std::cerr << "[K.O.-RAMA] Spritesheet ausente: " << sheets[i] << ".png\n"; continue; }
        addSheet(sheets[i], std::move(*img), spritesDir / (sheets[i] + ".atlas"));
    }
    if (std::find(sheets.begin(), sheets.end(), "jack") != sheets.end() && std::filesystem::exists(spritesDir / "jack_skins.atlas"))
        loadSheet(spritesDir, "jack_skins");
    if (std::find(sheets.begin(), sheets.end(), "frog") != sheets.end() && std::filesystem::exists(spritesDir / "frog_skins.atlas"))
        loadSheet(spritesDir, "frog_skins");
    if (std::find(sheets.begin(), sheets.end(), "joana") != sheets.end() && std::filesystem::exists(spritesDir / "joana_skins.atlas"))
        loadSheet(spritesDir, "joana_skins");
    if (std::find(sheets.begin(), sheets.end(), "marcov") != sheets.end() && std::filesystem::exists(spritesDir / "marcov_skins.atlas"))
        loadSheet(spritesDir, "marcov_skins");
    if (std::find(sheets.begin(), sheets.end(), "valeria") != sheets.end() && std::filesystem::exists(spritesDir / "valeria_skins.atlas"))
        loadSheet(spritesDir, "valeria_skins");
    if (std::find(sheets.begin(), sheets.end(), "vivaldi") != sheets.end())
        for (unsigned page = 0; std::filesystem::exists(spritesDir / ("vivaldi_skins_" + std::to_string(page) + ".atlas")); ++page)
            loadSheet(spritesDir, "vivaldi_skins_" + std::to_string(page));
    if (std::find(sheets.begin(), sheets.end(), "cremoso") != sheets.end())
        for (unsigned page = 0; std::filesystem::exists(spritesDir / ("cremoso_skins_" + std::to_string(page) + ".atlas")); ++page)
            loadSheet(spritesDir, "cremoso_skins_" + std::to_string(page));
    if (std::find(sheets.begin(), sheets.end(), "espinha") != sheets.end())
        for (unsigned page=0;std::filesystem::exists(spritesDir/("espinha_skins_"+std::to_string(page)+".atlas"));++page)loadSheet(spritesDir,"espinha_skins_"+std::to_string(page));
    if (std::find(sheets.begin(), sheets.end(), "boss") != sheets.end())
        for (unsigned page = 0; std::filesystem::exists(spritesDir / ("boss_skins_" + std::to_string(page) + ".atlas")); ++page)
            loadSheet(spritesDir, "boss_skins_" + std::to_string(page));
}

std::optional<SpriteFrame> SpriteLibrary::find(const std::string& name) const {
    auto it = frames_.find(name);
    if (it == frames_.end()) return std::nullopt;
    return it->second;
}

void SpriteLibrary::loadRig(const std::filesystem::path& spritesDir, const std::string& key) {
    if (rigs_.count(key)) return;
    auto r = std::make_unique<LiveRig>();
    if (r->load(spritesDir, key)) rigs_[key] = std::move(r);
}

void SpriteLibrary::pumpRequests() const {
    while (pending_.size() < 2 && !queuedRigs_.empty()) {
        const std::string key = queuedRigs_.front();
        queuedRigs_.erase(queuedRigs_.begin());
        if (!wanted(key) || rigs_.count(key) || uploading_.count(key) || pending_.count(key)) continue;
        const auto dir = rigDir_;
        pending_[key] = std::async(std::launch::async, [dir, key]() {
            auto r = std::make_unique<LiveRig>();
            if (!r->parse(dir, key)) r.reset();
            return r;
        });
    }
}

void SpriteLibrary::requestRig(const std::string& key) const {
    if (rigDir_.empty() || !wanted(key) || rigs_.count(key) || pending_.count(key) || uploading_.count(key) || rigMissing_.count(key)) return;
    if (std::find(queuedRigs_.begin(), queuedRigs_.end(), key) == queuedRigs_.end()) queuedRigs_.push_back(key);
    pumpRequests();
}

const LiveRig* SpriteLibrary::rig(const std::string& key) const {
    auto it = rigs_.find(key);
    if (it != rigs_.end()) return it->second.get();
    requestRig(key);
    // Production drawing never uploads textures or waits for a worker.
    if (!waitForRigs || !wanted(key)) return nullptr;
    while (!rigMissing_.count(key)) {
        const_cast<SpriteLibrary*>(this)->pollUpload();
        it = rigs_.find(key);
        if (it != rigs_.end()) return it->second.get();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return nullptr;
}

void SpriteLibrary::keepRigs(const std::vector<std::string>& keys) {
    limitedRigs_ = true;
    wantedRigs_ = std::unordered_set<std::string>(keys.begin(), keys.end());
    for (auto it = rigs_.begin(); it != rigs_.end();) { if (!wanted(it->first)) it = rigs_.erase(it); else ++it; }
    for (auto it = uploading_.begin(); it != uploading_.end();) { if (!wanted(it->first)) it = uploading_.erase(it); else ++it; }
    queuedRigs_.erase(std::remove_if(queuedRigs_.begin(), queuedRigs_.end(), [this](const auto& key) { return !wanted(key); }), queuedRigs_.end());
    // Never destroy an unfinished async future here: that would block the UI.
}

bool SpriteLibrary::pollUpload() {
    const bool sheetUploaded = pollSheets();
    bool changed = false;
    for (auto it = pending_.begin(); it != pending_.end();) {
        if (it->second.wait_for(std::chrono::seconds(0)) != std::future_status::ready) { ++it; continue; }
        const auto key = it->first;
        auto r = it->second.get();
        it = pending_.erase(it);
        if (!wanted(key)) { changed = true; continue; }
        if (!r) rigMissing_[key] = true;
        else uploading_[key] = std::move(r);
        changed = true;
    }
    pumpRequests();
    if (!uploading_.empty()) {
        auto it = uploading_.begin();
        const int status = it->second->uploadStep();
        if (status < 0) { rigMissing_[it->first] = true; uploading_.erase(it); }
        else if (status > 0) { rigs_[it->first] = std::move(it->second); uploading_.erase(it); }
        changed = true;
    }
    return changed || sheetUploaded;
}

bool SpriteLibrary::rigsReady(const std::vector<std::string>& keys) const {
    return std::all_of(keys.begin(), keys.end(), [this](const auto& key) { return rigs_.count(key) != 0; });
}

void SpriteLibrary::loadMenuSheets(const std::filesystem::path& dir) {
    loadSheets(dir, {"crowd", "portraits"});
    for (const char* key : {"jack","frog","joana","cremoso","valeria","vivaldi","espinha","marcov","boss","dummy"})
        if (std::filesystem::exists(dir / (std::string("portraits_hd_") + key + ".atlas"))) loadSheet(dir, std::string("portraits_hd_") + key);
}

std::vector<std::string> SpriteLibrary::selectFighterSheets(const std::filesystem::path& dir, const std::vector<uint32_t>& characters) {
    static const char* keys[] = {"jack","frog","joana","cremoso","valeria","vivaldi","espinha","marcov","boss","dummy"};
    std::unordered_set<std::string> keep = {"crowd","portraits"};
    for (const char* key : keys) keep.insert(std::string("portraits_hd_") + key);
    for (uint32_t c : characters) if (c < 10) {
        const std::string key = keys[c];
        if (c < 9) keep.insert(key);
        const std::string single = key + "_skins";
        if (std::filesystem::exists(dir / (single + ".atlas"))) keep.insert(single);
        for (unsigned p = 0; std::filesystem::exists(dir / (single + "_" + std::to_string(p) + ".atlas")); ++p) keep.insert(single + "_" + std::to_string(p));
    }
    wantedSheets_ = keep;
    requiredSheets_.clear();
    for (const auto& key : keep) if (key != "crowd" && key != "portraits" && key.rfind("portraits_hd_", 0) != 0) requiredSheets_.push_back(key);
    std::sort(requiredSheets_.begin(), requiredSheets_.end());
    queuedSheets_.erase(std::remove_if(queuedSheets_.begin(), queuedSheets_.end(), [&](const auto& k) { return !keep.count(k); }), queuedSheets_.end());
    for (auto it = uploadingSheets_.begin(); it != uploadingSheets_.end();) { if (!keep.count(it->first)) it = uploadingSheets_.erase(it); else ++it; }
    for (auto it = sheetMissing_.begin(); it != sheetMissing_.end();) { if (!keep.count(*it)) it = sheetMissing_.erase(it); else ++it; }
    for (auto it = textures_.begin(); it != textures_.end();) {
        const auto* texture = it->get();const auto name = textureSheets_.at(texture);
        if (keep.count(name)) { ++it; continue; }
        for (auto f = frames_.begin(); f != frames_.end();) { if (f->second.texture == texture) f = frames_.erase(f); else ++f; }
        loadedSheets_.erase(name);textureSheets_.erase(texture);it = textures_.erase(it);
    }
    return requiredSheets_;
}

void SpriteLibrary::keepFighterSheets(const std::filesystem::path& dir, const std::vector<uint32_t>& characters) {
    selectFighterSheets(dir, characters);
    // Synchronous path retained for snapshot/export tools, never used to enter a fight.
    static const char* keys[] = {"jack","frog","joana","cremoso","valeria","vivaldi","espinha","marcov","boss","dummy"};
    for (uint32_t c : characters) if (c < 10) loadSheet(dir, c == 9 ? "dummy_skins" : keys[c]);
}

void SpriteLibrary::requestFighterSheets(const std::filesystem::path& dir, const std::vector<uint32_t>& characters) {
    sheetDir_ = dir;
    for (const auto& key : selectFighterSheets(dir, characters))
        if (!loadedSheets_.count(key) && !pendingSheets_.count(key) && !uploadingSheets_.count(key) && !sheetMissing_.count(key) &&
            std::find(queuedSheets_.begin(), queuedSheets_.end(), key) == queuedSheets_.end()) queuedSheets_.push_back(key);
    pumpSheets();
}

bool SpriteLibrary::fighterSheetsReady() const {
    return std::all_of(requiredSheets_.begin(), requiredSheets_.end(), [&](const auto& key) { return loadedSheets_.count(key) != 0; });
}

void SpriteLibrary::pumpSheets() {
    // One decoder in addition to the two rig workers. Never cancel an unfinished future on the UI thread.
    while (pendingSheets_.size() < 1 && !queuedSheets_.empty()) {
        const auto key = queuedSheets_.front(); queuedSheets_.erase(queuedSheets_.begin());
        if (!wantedSheets_.count(key) || loadedSheets_.count(key)) continue;
        pendingSheets_[key] = std::async(std::launch::async, [file = sheetDir_ / (key + ".png")]() -> std::optional<sf::Image> {
            sf::Image image;
            if (!image.loadFromFile(file)) return std::nullopt;
            return image;
        });
    }
}

bool SpriteLibrary::pollSheets() {
    for (auto it = pendingSheets_.begin(); it != pendingSheets_.end();) {
        if (it->second.wait_for(std::chrono::seconds(0)) != std::future_status::ready) { ++it; continue; }
        const auto key = it->first; auto image = it->second.get(); it = pendingSheets_.erase(it);
        if (!wantedSheets_.count(key) || loadedSheets_.count(key)) continue;
        if (!image) sheetMissing_.insert(key);
        else uploadingSheets_[key].image = std::move(*image);
    }
    pumpSheets();
    if (uploadingSheets_.empty()) return false;
    auto it = uploadingSheets_.begin(); auto& upload = it->second;
    const auto size = upload.image.getSize();
    if (!upload.texture) {
        upload.texture = std::make_unique<sf::Texture>();
        if (!upload.texture->resize(size)) { sheetMissing_.insert(it->first); uploadingSheets_.erase(it); return true; }
        upload.texture->setSmooth(true);
    }
    const unsigned rows = std::min(size.y - upload.row, std::max(1u, unsigned((4u * 1024u * 1024u) / (std::size_t(size.x) * 4))));
    upload.texture->update(upload.image.getPixelsPtr() + std::size_t(upload.row) * size.x * 4, {size.x, rows}, {0, upload.row});
    upload.row += rows;
    if (upload.row == size.y) {
        if (!registerSheet(it->first, std::move(upload.texture), sheetDir_ / (it->first + ".atlas"))) sheetMissing_.insert(it->first);
        uploadingSheets_.erase(it);
    }
    return true;
}

} // namespace KoRama
