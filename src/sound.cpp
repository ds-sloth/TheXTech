/*
 * TheXTech - A platform game engine ported from old source code for VB6
 *
 * Copyright (c) 2009-2011 Andrew Spinks, original VB6 code
 * Copyright (c) 2020-2021 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef NO_SDL
#include <SDL2/SDL_messagebox.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_mixer_ext.h>
#endif

#include "globals.h"
#include "load_gfx.h"
#include "pge_delay.h"

#include "sound.h"
#include "sound_internal.h"

#include <Logger/logger.h>
#include <IniProcessor/ini_processing.h>
#include <Utils/files.h>
#include <Utils/strings.h>
#include <unordered_map>
#include <fmt_format_ne.h>

#include "pseudo_vb.h"

// Public musicPlaying As Boolean
bool musicPlaying = false;
// Public musicLoop As Integer
int musicLoop = 0;
// Public musicName As String
std::string musicName;

int playerHammerSFX = SFX_Fireball;

AudioSetup_t g_audioSetup;

static bool g_mixerLoaded = false;

static int g_customLvlMusicId = 24;
static int g_customWldMusicId = 17;
static int g_reservedChannels = 0;

//! Total count of loaded default sounds
static unsigned int g_totalSounds = 0;
//! Are custom sounds was loaded from the level/world data folder?
static bool g_customSoundsInDataFolder = false;
//! Are custom music files was loaded from the level/world data folder?
static bool g_customMusicInDataFolder = false;
//! Total count of level music
static unsigned int g_totalMusicLevel = 0;
//! Total count of world map music
static unsigned int g_totalMusicWorld = 0;
//! Total count of special music
static unsigned int g_totalMusicSpecial = 0;
//! Enable using the unique iceball SFX when available
static bool s_useIceBallSfx = false;
//! Enable using of the new ice SFX: NPC freeze and breaking of the frozen NPC
static bool s_useNewIceSfx = false;

static int g_errorsSfx = 0;
// static int g_errorsMusic = 0; // Unued yet

static std::string MusicRoot;
static std::string SfxRoot;

static std::string musicIni = "music.ini";
static std::string sfxIni = "sounds.ini";
static const uint32_t INVALID_ID = -1;

struct Music_t
{
    std::string path;
    int volume = 52;
};

struct SFX_t
{
    std::string path;
    std::string customPath;
    SI_Chunk* chunk = nullptr;
    SI_Chunk* chunkOrig = nullptr;
    bool isCustom = false;
    bool isSilent = false;
    bool isSilentOrig = false;
    int volume = 128;
    int channel = -1;
    uint32_t playingSoundId = INVALID_ID;
};

static std::unordered_map<std::string, Music_t> music;
static std::unordered_map<std::string, SFX_t>   sound;

int CustomWorldMusicId()
{
    return g_customWldMusicId;
}

void InitMixerX()
{
    MusicRoot = AppPath + "music/";
    SfxRoot = AppPath + "sound/";

    if(g_mixerLoaded)
        return;

    if(!SI_Init())
        noSound = true;
    else
        g_mixerLoaded = true;
}

void QuitMixerX()
{
    if(!g_mixerLoaded)
        return;

    noSound = true;
    SI_KillMusic();

    for(auto it = sound.begin(); it != sound.end(); ++it)
    {
        auto &s = it->second;
        if(s.chunk)
            SI_FreeChunk(s.chunk);
        if(s.chunkOrig)
            SI_FreeChunk(s.chunkOrig);
    }
    sound.clear();
    music.clear();
    SI_Quit();
    g_mixerLoaded = false;
}

static void AddMusic(const std::string &root,
                     IniProcessing &ini,
                     std::string alias,
                     std::string group,
                     int volume)
{
    std::string f;
    ini.beginGroup(group);
    ini.read("file", f, std::string());
    if(!f.empty())
    {
        Music_t m;
        m.path = root + f;
        m.volume = volume;
        pLogDebug("Adding music [%s] '%s'", alias.c_str(), m.path.c_str());
        auto a = music.find(alias);
        if(a == music.end())
            music.insert({alias, m});
        else
            a->second = m;
    }
    ini.endGroup();
}

static void RestoreSfx(SFX_t &u)
{
    if(u.isCustom)
    {
        if((u.chunk || u.isSilent) && (u.chunkOrig || u.isSilentOrig))
        {
            if(u.chunk)
                SI_FreeChunk(u.chunk);
            u.chunk = u.chunkOrig;
            u.isSilent = u.isSilentOrig;
            u.chunkOrig = nullptr;
        }
        u.isCustom = false;
    }
}

static void AddSfx(const std::string &root,
                   IniProcessing &ini,
                   std::string alias,
                   std::string group,
                   bool isCustom = false)
{
    std::string f;
    bool isSilent;
    ini.beginGroup(group);
    ini.read("file", f, std::string());
    ini.read("silent", isSilent, false);
    if(!f.empty() || isSilent)
    {
        if(isCustom)
        {
            auto s = sound.find(alias);
            if(s != sound.end())
            {
                auto &m = s->second;
                std::string newPath = root + f;
                if(!isSilent && m.isCustom && newPath == m.customPath)
                {
                    ini.endGroup();
                    return;  // Don't load the same file twice!
                }

                SI_Chunk* backup = m.chunk;
                bool backup_isSilent = m.isSilent;
                m.customPath = newPath;
                bool no_preload = SI_NoPreload((root + f).c_str());
                if(!no_preload && !isSilent)
                    m.chunk = SI_LoadWAV((root + f).c_str());
                if(no_preload || m.chunk || isSilent)
                {
                    if(!m.isCustom && !m.chunkOrig)
                    {
                        m.chunkOrig = backup;
                        m.isSilentOrig = backup_isSilent;
                    }
                    else if(backup)
                        SI_FreeChunk(backup);
                    m.isCustom = true;
                    m.isSilent = isSilent;
                }
                else
                {
                    m.chunk = backup;
                    m.isSilent = backup_isSilent;
                }
            }
        }
        else
        {
            SFX_t m;
            m.path = root + f;
            m.volume = 128;
            m.isSilent = isSilent;
            pLogDebug("Adding SFX [%s] '%s'", alias.c_str(), isSilent ? "<silence>" : m.path.c_str());
            bool no_preload = SI_NoPreload(m.path.c_str());
            if(!isSilent && !no_preload)
                m.chunk = SI_LoadWAV(m.path.c_str());
            m.channel = -1;
            if(no_preload || m.chunk || isSilent)
            {
                bool isSingleChannel = false;
                ini.read("single-channel", isSingleChannel, false);
                if(isSingleChannel)
                    m.channel = g_reservedChannels++;
                sound.insert({alias, m});
            }
            else
            {
                if(!isCustom)
                    g_errorsSfx++;
            }
        }
    }
    ini.endGroup();
}

void SetMusicVolume(std::string Alias, long Volume)
{
    auto mus = music.find(Alias);
    if(mus != music.end())
    {
        mus->second.volume = int(Volume);
    }
}

void SoundPauseAll()
{
    if(noSound)
        return;
    pLogDebug("Pause all sound");
    SI_PauseAll();
}

void SoundResumeAll()
{
    if(noSound)
        return;
    pLogDebug("Resume all sound");
    SI_ResumeAll();
}

static void processPathArgs(std::string &path,
                            const std::string &episodeRoot,
                            const std::string &dataDirName)
{
    if(path.find('|') == std::string::npos)
        return; // Nothing to do
    Strings::List p;
    Strings::split(p, path, '|');
    Strings::replaceInAll(p[1], "{e}", episodeRoot);
    Strings::replaceInAll(p[1], "{d}", episodeRoot + dataDirName);
    Strings::replaceInAll(p[1], "{r}", MusicRoot);
    path = p[0] + "|" + p[1];
}

void PlayMusic(std::string Alias, int fadeInMs)
{
    if(noSound)
        return;
    SI_KillMusic();

    auto mus = music.find(Alias);
    if(mus != music.end())
    {
        auto &m = mus->second;
        std::string p = m.path;
        processPathArgs(p, FileNamePath + "/", FileName + "/");
        SI_PlayMusic(p.c_str(), m.volume, fadeInMs);
    }
}

void PlaySfx(std::string Alias, int loops)
{
    auto sfx = sound.find(Alias);
    if(sfx != sound.end())
    {
        auto &s = sfx->second;
        if(s.isSilent)
            return;

        const char* backup_path;
        if(s.isCustom)
            backup_path = s.customPath.c_str();
        else
            backup_path = s.path.c_str();
        s.playingSoundId = SI_PlaySound(
            s.chunk, backup_path, loops, s.channel, s.playingSoundId);
    }
}

void StopSfx(std::string Alias)
{
    auto sfx = sound.find(Alias);
    if(sfx != sound.end())
    {
        auto &s = sfx->second;
        if(!s.isSilent)
        {
            SI_KillSound(s.channel, s.playingSoundId);
        }
    }
}

void StartMusic(int A, int fadeInMs)
{
    if(noSound)
        return;

    SI_KillMusic();

    if((LevelSelect || WorldEditor) && !GameMenu && !GameOutro) // music on the world map
    {
        curWorldMusic = A;
        std::string mus = fmt::format_ne("wmusic{0}", A);
        if(curWorldMusic == g_customWldMusicId)
        {
            pLogDebug("Starting custom music [%s]", curWorldMusicFile.c_str());
            std::string p = FileNamePath + "/" + curWorldMusicFile;
            processPathArgs(p, FileNamePath + "/", FileName + "/");
            SI_PlayMusic(p.c_str(), 64, fadeInMs);
        }
        else
        {
            pLogDebug("Starting world music [%s]", mus.c_str());
            PlayMusic(mus, fadeInMs);
        }
        musicName = mus;
    }
    else if(A == -1) // P switch music
    {
        if(FreezeNPCs) {
            pLogDebug("Starting special music [stmusic]");
            PlayMusic("stmusic", fadeInMs);
            musicName = "stmusic";
        } else {
            pLogDebug("Starting special music [smusic]");
            PlayMusic("smusic", fadeInMs);
            musicName = "smusic";
        }
        curMusic = -1;
    }
    else if(PSwitchTime == 0 && PSwitchStop == 0) // level music
    {
        curMusic = bgMusic[A];
        std::string mus = fmt::format_ne("music{0}", curMusic);
        if(curMusic == g_customLvlMusicId)
        {
            pLogDebug("Starting custom music [%s]", CustomMusic[A].c_str());
            std::string p = FileNamePath + "/" + CustomMusic[A];
            processPathArgs(p, FileNamePath + "/", FileName + "/");
            SI_PlayMusic(p.c_str(), 52, fadeInMs);
        }
        else
        {
            pLogDebug("Starting level music [%s]", mus.c_str());
            PlayMusic(mus, fadeInMs);
        }
        musicName = mus;
    }

    musicPlaying = true;
}

void StopMusic()
{
    if(!musicPlaying || noSound)
        return;

    pLogDebug("Stopping music");

    SI_KillMusic();
    musicPlaying = false;
}

void FadeOutMusic(int ms)
{
    if(!musicPlaying || noSound)
        return;
    pLogDebug("Fading out music");
    SI_FadeOutMusic(ms);
    musicPlaying = false;
}


void PlayInitSound()
{
    std::string doSound = AppPath + "sound/";
    IniProcessing sounds(AppPath + "sounds.ini");
    unsigned int totalSounds;
    sounds.beginGroup("sound-main");
    sounds.read("total", totalSounds, 0);
    sounds.endGroup();

    if(totalSounds >= 29)
    {
        std::string p;
        sounds.beginGroup("sound-29");
        sounds.read("file", p, std::string());
        sounds.endGroup();

        if(!p.empty())
        {
            SI_PlayLoadSound((SfxRoot + p).c_str());
            do // Synchroniously play the loading sound to don't distort it during the SFX loading
            {
                PGE_Delay(15);
                UpdateLoadREAL();
            } while(SI_PlayingLoadSound());
        }
    }
}

static void loadMusicIni(const std::string &root, const std::string &path, bool isLoadingCustom)
{
    IniProcessing musicSetup(path);
    if(!isLoadingCustom)
    {
        music.clear();
        g_totalMusicLevel = 0;
        g_totalMusicWorld = 0;
        g_totalMusicSpecial = 0;
        musicSetup.beginGroup("music-main");
        musicSetup.read("total-level", g_totalMusicLevel, 0);
        musicSetup.read("total-world", g_totalMusicWorld, 0);
        musicSetup.read("total-special", g_totalMusicSpecial, 0);
        musicSetup.read("level-custom-music-id", g_customLvlMusicId, 0);
        musicSetup.read("world-custom-music-id", g_customWldMusicId, 0);
        musicSetup.endGroup();
    }

    if(!isLoadingCustom)
        UpdateLoad();
    for(unsigned int i = 1; i <= g_totalMusicLevel; ++i)
    {
        std::string alias = fmt::format_ne("music{0}", i);
        std::string group = fmt::format_ne("level-music-{0}", i);
        AddMusic(root, musicSetup, alias, group, 52);
    }

    if(!isLoadingCustom)
        UpdateLoad();
    for(unsigned int i = 1; i <= g_totalMusicWorld; ++i)
    {
        std::string alias = fmt::format_ne("wmusic{0}", i);
        std::string group = fmt::format_ne("world-music-{0}", i);
        AddMusic(root, musicSetup, alias, group, 64);
    }

    if(!isLoadingCustom)
        UpdateLoad();
    for(unsigned int i = 1; i <= g_totalMusicSpecial; ++i)
    {
        std::string alias = fmt::format_ne("smusic{0}", i);
        if(i == 1)
            alias = "smusic";
        else if(i == 2)
            alias = "stmusic";
        else if(i == 3)
            alias = "tmusic";
        std::string group = fmt::format_ne("special-music-{0}", i);
        AddMusic(root, musicSetup, alias, group, 64);
    }
}

static void loadCustomSfxIni(const std::string &root, const std::string &path)
{
    IniProcessing sounds(path);
    for(unsigned int i = 1; i <= g_totalSounds; ++i)
    {
        std::string alias = fmt::format_ne("sound{0}", i);
        std::string group = fmt::format_ne("sound-{0}", i);
        AddSfx(root, sounds, alias, group, true);
    }
}

static void restoreDefaultSfx()
{
    for(auto &s : sound)
    {
        auto &u = s.second;
        RestoreSfx(u);
    }
}

void InitSound()
{
    if(noSound)
        return;

    musicIni = AppPath + "music.ini";
    sfxIni = AppPath + "sounds.ini";

    UpdateLoad();
    if(!Files::fileExists(musicIni) && !Files::fileExists(sfxIni))
    {
        pLogWarning("music.ini and sounds.ini are missing");
#ifndef NO_SDL
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "music.ini and sounds.ini are missing",
                                 "Files music.ini and sounds.ini are not exist, game will work without default music and SFX.",
                                 frmMain.getWindow());
#endif
        g_customLvlMusicId = 24;
        g_customWldMusicId = 17;
        return;
    }
    else if(!Files::fileExists(musicIni))
    {
        pLogWarning("music.ini is missing");
#ifndef NO_SDL
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "music.ini is missing",
                                 "File music.ini is not exist, game will work without default music.",
                                 frmMain.getWindow());
#endif
    }
    else if(!Files::fileExists(sfxIni))
    {
        pLogWarning("sounds.ini is missing");
#ifndef NO_SDL
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "sounds.ini is missing",
                                 "File sounds.ini is not exist, game will work without SFX.",
                                 frmMain.getWindow());
#endif
    }

    loadMusicIni(MusicRoot, musicIni, false);

    UpdateLoad();
    IniProcessing sounds(sfxIni);
    sounds.beginGroup("sound-main");
    sounds.read("total", g_totalSounds, 0);
    sounds.read("use-iceball-sfx", s_useIceBallSfx, false);
    sounds.read("use-new-ice-sfx", s_useNewIceSfx, false);
    bool playerUseNPCHammer;
    bool playerUseOwnHammer;
    sounds.read("player-use-npc-hammer-sfx", playerUseNPCHammer, false);
    sounds.read("player-use-own-hammer-sfx", playerUseOwnHammer, false);
    sounds.endGroup();

    if(playerUseOwnHammer)
        playerHammerSFX = SFX_PlayerHammer;
    else if(playerUseNPCHammer)
        playerHammerSFX = SFX_Throw;
    else
        playerHammerSFX = SFX_Fireball;

    UpdateLoad();
    for(unsigned int i = 1; i <= g_totalSounds; ++i)
    {
        std::string alias = fmt::format_ne("sound{0}", i);
        std::string group = fmt::format_ne("sound-{0}", i);
        AddSfx(SfxRoot, sounds, alias, group);
    }
    UpdateLoad();
    SI_ReserveChannels(g_reservedChannels);

    if(g_errorsSfx > 0)
    {
        std::string msg = fmt::format_ne("Failed to load some SFX assets. Loo a log file to get more details:\n{0}", getLogFilePath());
#ifndef NO_SDL
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Sounds loading error", msg.c_str(), frmMain.getWindow());
#endif
    }
}

static const std::unordered_map<int, int> s_soundDelays =
{
    {2, 12}, {3, 12},  {4, 12},  {5, 30}, {8, 10},  {9, 4},
    {10, 8}, {12, 10}, {17, 10}, {26, 8}, {31, 20}, {37, 10},
    {42, 16},{50, 8},  {54, 8},  {71, 9}, {74, 8},  {81, 5},
    {86, 8}, {SFX_Icebreak, 4}
};

static void s_resetSoundDelay(int A)
{
    // set the delay before a sound can be played again
    auto i = s_soundDelays.find(A);
    if(i == s_soundDelays.end())
        SoundPause[A] = 4;
    else
        SoundPause[A] = i->second;
#if 0
    switch(A)
    {
    case 2: SoundPause[A] = 12; break;
    case 3: SoundPause[A] = 12; break;
    case 4: SoundPause[A] = 12; break;
    case 5: SoundPause[A] = 30; break;
    case 8: SoundPause[A] = 10; break;
    case 9: SoundPause[A] = 4; break;
    case 10: SoundPause[A] = 8; break;
    case 12: SoundPause[A] = 10; break;
    case 17: SoundPause[A] = 10; break;
    case 26: SoundPause[A] = 8; break;
    case 31: SoundPause[A] = 20; break;
    case 37: SoundPause[A] = 10; break;
    case 42: SoundPause[A] = 16; break;
    case 50: SoundPause[A] = 8; break;
    case 54: SoundPause[A] = 8; break;
    case 71: SoundPause[A] = 9; break;
    case 74: SoundPause[A] = 8; break;
    case 81: SoundPause[A] = 5; break;
    case 86: SoundPause[A] = 8; break;
    default: SoundPause[A] = 4; break;
    }
#endif
}

static const std::unordered_map<int, int> s_soundFallback =
{
    {SFX_Iceball, SFX_Fireball},
    {SFX_Freeze, SFX_ShellHit},
    {SFX_Icebreak, SFX_ShellHit},
    {SFX_SproutVine, SFX_Mushroom},
};

static int getFallbackSfx(int A)
{
    auto fb = s_soundFallback.find(A);
    if(fb != s_soundFallback.end())
        A = fb->second;
    return A;
}

void PlaySound(int A, int loops)
{
    if(noSound)
        return;

    if(GameMenu || GameOutro) // || A == 26 || A == 27 || A == 29)
        return;

    if(A > (int)g_totalSounds) // Play fallback sound for the missing SFX
        A = getFallbackSfx(A);
    else if(!s_useIceBallSfx && A == SFX_Iceball)
        A = SFX_Fireball; // Fell back into fireball when iceball sound isn't preferred
    else if(!s_useNewIceSfx && (A == SFX_Freeze || A == SFX_Icebreak))
        A = SFX_ShellHit; // Restore the old behavior

    if(numPlayers > 2)
        SoundPause[10] = 1;

    if(SoundPause[A] == 0) // if the sound wasn't just played
    {
        std::string alias = fmt::format_ne("sound{0}", A);
        PlaySfx(alias, loops);
        s_resetSoundDelay(A);
    }
}

void PlaySoundMenu(int A, int loops)
{
    if(SoundPause[A] == 0) // if the sound wasn't just played
    {
        std::string alias = fmt::format_ne("sound{0}", A);
        PlaySfx(alias, loops);
        s_resetSoundDelay(A);
    }
}

// stops all sound from being played for 10 cycles
void BlockSound()
{
    For(A, 1, numSounds)
    {
        SoundPause[A] = 10;
    }
}

void UpdateSound()
{
    if(noSound)
        return;
    For(A, 1, numSounds)
    {
        if(SoundPause[A] > 0)
            SoundPause[A] -= 1;
    }
}

void LoadCustomSound()
{
    if(noSound)
        return;

    if(GameMenu)
        return; // Don't load custom music in menu mode

    std::string mIni = FileNamePath + "music.ini";
    std::string sIni = FileNamePath + "sounds.ini";
    std::string mIniC = FileNamePath + FileName + "/music.ini";
    std::string sIniC = FileNamePath + FileName + "/sounds.ini";

    // To avoid bugs like custom local sounds was transferred into another level, it's need to clean-up old one if that was
    if(g_customMusicInDataFolder)
    {
        loadMusicIni(MusicRoot, musicIni, true);
        g_customMusicInDataFolder = false;
    }

    if(g_customSoundsInDataFolder)
    {
        restoreDefaultSfx();
        g_customSoundsInDataFolder = false;
    }

    if(FileNamePath == AppPath)
        return; // Don't treat default music/sounds ini as custom

    if(Files::fileExists(mIni)) // Load music.ini from an episode folder
        loadMusicIni(FileNamePath, mIni, true);

    if(Files::fileExists(mIniC)) // Load music.ini from a level/world custom folder
    {
        loadMusicIni(FileNamePath + FileName + "/", mIniC, true);
        g_customMusicInDataFolder = true;
    }

    if(Files::fileExists(sIni)) // Load sounds.ini from an episode folder
        loadCustomSfxIni(FileNamePath, sIni);

    if(Files::fileExists(sIniC)) // Load sounds.ini from a level/world custom folder
    {
        loadCustomSfxIni(FileNamePath + FileName + "/", sIniC);
        g_customSoundsInDataFolder = true;
    }
}

void UnloadCustomSound()
{
    if(noSound)
        return;
    loadMusicIni(MusicRoot, musicIni, true);
    restoreDefaultSfx();
    g_customMusicInDataFolder = false;
    g_customSoundsInDataFolder = false;
}
