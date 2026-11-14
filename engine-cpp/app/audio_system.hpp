#ifndef KORAMA_AUDIO_SYSTEM_HPP
#define KORAMA_AUDIO_SYSTEM_HPP

#include "sound_cues.hpp"
#include <SFML/Audio.hpp>
#include <array>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace KoRama {

enum class MusicTrack {
    None,
    Menu,
    Fight,
    Boss,
    Jack,
    KingFrog,
    Joana,
    Cremoso,
    Valeria,
    Vivaldi,
    Espinha,
    Marcov,
    BossPhase1,
    BossPhase2,
    BossPhase3,
    Victory,
    Defeat
};

class AudioSystem {
public:
    AudioSystem();
    explicit AudioSystem(const std::filesystem::path& assetRoot);

    void init(const std::filesystem::path& assetRoot);
    void play(Sfx id, int param = 0);
    void play(const std::vector<SoundCue>& cues);
    void playReplay(Sfx id, int param = 0, float pitch = 0.78f);
    void playReplay(const std::vector<SoundCue>& cues, float pitch = 0.78f);
    void setMusic(MusicTrack track);
    void setReplayMusic(MusicTrack track, float pitch = 0.82f, sf::Time startOffset = sf::Time::Zero);
    void setMusicPitch(float pitch);
    float musicPitch() const { return musicPitch_; }
    bool isReplayMusic() const { return replayMusic_; }
    sf::Time musicPlayingOffset() const;

    void setFightPaused(bool paused);
    bool fightPaused() const { return fightPaused_; }

    void stop(Sfx id);
    void stopVersusSounds();
    void stopFightSounds();
    void stopAllSfx();

    void setSfxVolume(float percent);
    void setMusicVolume(float percent);
    void setMusicDuck(float factor);
    float musicDuck() const { return duckFactor_; }
    void toggleMute();
    bool muted() const { return muted_; }
    MusicTrack currentTrack() const { return track_; }

    // Buffers sintetizados (também usados pela ferramenta tools/sound_dump).
    const sf::SoundBuffer& buffer(Sfx id, int param);
    const sf::SoundBuffer& musicBuffer(MusicTrack track);

private:
    struct ActiveVoice {
        std::optional<sf::Sound> sound;
        Sfx id = Sfx::Count;
        int param = 0;
        bool isFightSound = true;
        bool pausedByState = false;
    };

    std::filesystem::path assetRoot_;
    std::map<std::pair<int, int>, sf::SoundBuffer> cache_;
    std::map<Sfx, sf::SoundBuffer> fileSfxBuffers_;
    std::array<ActiveVoice, 32> voices_;
    std::array<sf::SoundBuffer, 3> musicBuffers_;
    std::array<bool, 3> musicReady_{};
    std::unique_ptr<sf::Music> musicStream_;
    std::optional<sf::Sound> synthMusic_;
    MusicTrack track_ = MusicTrack::None;
    float sfxVolume_ = 100.0f;
    float musicVolume_ = 65.0f;
    float musicPitch_ = 1.0f;
    float duckFactor_ = 1.0f;
    bool replayMusic_ = false;
    bool muted_ = false;
    bool fightPaused_ = false;
    bool musicPausedByState_ = false;
    bool synthMusicPausedByState_ = false;

    void applyMusicVolume();
    std::filesystem::path resolveMusicPath(MusicTrack track) const;
    void loadSfxFiles();
};

} // namespace KoRama

#endif // KORAMA_AUDIO_SYSTEM_HPP
