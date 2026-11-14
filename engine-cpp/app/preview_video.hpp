#ifndef KORAMA_PREVIEW_VIDEO_HPP
#define KORAMA_PREVIEW_VIDEO_HPP

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace KoRama {

// Small, seekable, looped preview: JPEG frames in one KPV file. Only the
// currently selected frame occupies a GPU texture; no fight runs in the build UI.
class PreviewVideo {
public:
    bool open(const std::filesystem::path& path);
    bool showFrame(std::size_t index);
    void clear();
    const sf::Texture* texture() const { return texture_.get(); }
    std::size_t frameCount() const { return frames_.size(); }
    unsigned fps() const { return fps_; }

private:
    std::vector<std::vector<std::uint8_t>> frames_;
    std::unique_ptr<sf::Texture> texture_;
    std::size_t shown_ = static_cast<std::size_t>(-1);
    unsigned width_ = 0, height_ = 0, fps_ = 0;
};

} // namespace KoRama
#endif
