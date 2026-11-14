#include "preview_video.hpp"
#include <fstream>
#include <string_view>

namespace KoRama {
namespace {
std::uint32_t readU32(std::istream& in) {
    unsigned char b[4]{};
    if (!in.read(reinterpret_cast<char*>(b), 4)) return 0;
    return static_cast<std::uint32_t>(b[0]) | (static_cast<std::uint32_t>(b[1]) << 8) |
           (static_cast<std::uint32_t>(b[2]) << 16) | (static_cast<std::uint32_t>(b[3]) << 24);
}
}

void PreviewVideo::clear() {
    frames_.clear();
    texture_.reset();
    shown_ = static_cast<std::size_t>(-1);
    width_ = height_ = fps_ = 0;
}

bool PreviewVideo::open(const std::filesystem::path& path) {
    clear();
    std::error_code ec;
    const auto bytes = std::filesystem::file_size(path, ec);
    if (ec || bytes < 24 || bytes > 128u * 1024u * 1024u) return false;
    std::ifstream in(path, std::ios::binary);
    char magic[8]{};
    if (!in.read(magic, 8) || std::string_view(magic, 7) != "KORAPV1") return false;
    const unsigned width = readU32(in), height = readU32(in), fps = readU32(in), count = readU32(in);
    if (!in || width == 0 || height == 0 || width > 1024 || height > 1024 ||
        fps < 12 || fps > 60 || count == 0 || count > 180) return false;
    std::vector<std::vector<std::uint8_t>> loaded;
    loaded.reserve(count);
    for (unsigned i = 0; i < count; ++i) {
        const auto size = readU32(in);
        if (!in || size < 32 || size > 4u * 1024u * 1024u) return false;
        std::vector<std::uint8_t> frame(size);
        if (!in.read(reinterpret_cast<char*>(frame.data()), size)) return false;
        loaded.push_back(std::move(frame));
    }
    frames_ = std::move(loaded);
    width_ = width; height_ = height; fps_ = fps;
    return showFrame(0);
}

bool PreviewVideo::showFrame(std::size_t index) {
    if (index >= frames_.size()) return false;
    if (shown_ == index && texture_) return true;
    sf::Image image;
    const auto& data = frames_[index];
    if (!image.loadFromMemory(data.data(), data.size()) ||
        image.getSize() != sf::Vector2u(width_, height_)) return false;
    if (!texture_) {
        texture_ = std::make_unique<sf::Texture>();
        if (!texture_->loadFromImage(image)) { texture_.reset(); return false; }
        texture_->setSmooth(true);
    } else {
        texture_->update(image);
    }
    shown_ = index;
    return true;
}

} // namespace KoRama
