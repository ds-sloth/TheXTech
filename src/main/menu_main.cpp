/*
 * TheXTech - A platform game engine ported from old source code for VB6
 *
 * Copyright (c) 2009-2011 Andrew Spinks, original VB6 code
 * Copyright (c) 2020-2022 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_thread.h>
#include <fmt_format_ne.h>
#include <array>

#include <AppPath/app_path.h>
#include <DirManager/dirman.h>
#include <Utils/files.h>
#include <PGE_File_Formats/file_formats.h>

#include "menu_main.h"
#include "game_info.h"
#include "../gfx.h"
#include "screen_connect.h"
#include "menu_controls.h"

#include "speedrunner.h"
#include "../game_main.h"
#include "../sound.h"
#include "../player.h"
#include "../collision.h"
#include "../graphics.h"
#include "../core/render.h"
#include "../core/window.h"
#include "../core/events.h"
#include "../controls.h"
#include "../config.h"
#include "../compat.h"
#include "level_file.h"
#include "world_file.h"
#include "pge_delay.h"
#include "video.h"
#include "change_res.h"
#include "game_globals.h"

#include "screen_textentry.h"
#include "editor/new_editor.h"
#include "editor/write_world.h"

MainMenuContent g_mainMenu;

static SDL_atomic_t         loading;
static SDL_atomic_t         loadingProgrss;
static SDL_atomic_t         loadingProgrssMax;

static SDL_Thread*          loadingThread = nullptr;


int NumSelectWorld = 0;
int NumSelectWorldEditable = 0;
int NumSelectBattle = 0;
std::vector<SelectWorld_t> SelectWorld;
std::vector<SelectWorld_t> SelectWorldEditable;
std::vector<SelectWorld_t> SelectBattle;

void initMainMenu()
{
    SDL_AtomicSet(&loading, 0);
    SDL_AtomicSet(&loadingProgrss, 0);
    SDL_AtomicSet(&loadingProgrssMax, 0);

    g_mainMenu.introPressStart = "Press Start";

    g_mainMenu.mainStartGame = "Start Game";
    g_mainMenu.main1PlayerGame = "1 Player Game";
    g_mainMenu.mainMultiplayerGame = "2 Player Game";
    g_mainMenu.mainBattleGame = "Battle Game";
    g_mainMenu.mainEditor = "Editor";
    g_mainMenu.mainOptions = "Options";
    g_mainMenu.mainExit = "Exit";

    g_mainMenu.loading = "Loading...";

    for(int i = 1; i <= numCharacters; ++i)
        g_mainMenu.selectPlayer[i] = fmt::format_ne("{0} game", g_gameInfo.characterName[i]);

    g_mainMenu.charSelTitle = "Character Select";
    g_mainMenu.reconnectTitle = "Reconnect";
    g_mainMenu.dropAddTitle = "Drop/Add Players";
    g_mainMenu.wordPlayer = "Player";
    g_mainMenu.wordProfile = "Profile";
    g_mainMenu.playerSelStartGame = "Start Game";
    g_mainMenu.phrasePressAButton = "Press A Button";
    g_mainMenu.phraseTestControls = "Test Controls";
    g_mainMenu.wordDisconnect = "Disconnect";
    g_mainMenu.phraseHoldStartToReturn = "Hold Start";
    g_mainMenu.wordBack = "Back";
    g_mainMenu.wordResume = "Resume";
    g_mainMenu.wordWaiting = "Waiting";
    g_mainMenu.phraseForceResume = "Force Resume";
    g_mainMenu.phraseDropOthers = "Drop Others";
    g_mainMenu.phraseDropSelf = "Drop Self";
    g_mainMenu.phraseChangeChar = "Change Character";

    g_mainMenu.controlsTitle = "Controls";
    g_mainMenu.controlsConnected = "Connected:";
    g_mainMenu.controlsDeleteKey = "(Alt Jump to Delete)";
    g_mainMenu.controlsDeviceTypes = "Device Types";
    g_mainMenu.controlsInUse = "(In Use)";
    g_mainMenu.controlsNotInUse = "(Not In Use)";
    g_mainMenu.wordProfiles = "Profiles";
    g_mainMenu.wordButtons = "Buttons";

    g_mainMenu.controlsReallyDeleteProfile = "Really delete profile?";
    g_mainMenu.controlsNewProfile = "<New Profile>";

    g_mainMenu.wordNo = "No";
    g_mainMenu.wordYes = "Yes";

    MenuMode = MENU_INTRO;
}


static int ScrollDelay = 0;
static int menuPlayersNum = 0;
static int menuBattleMode = false;

static int menuCopySaveSrc = 0;
static int menuCopySaveDst = 0;
static int menuRecentEpisode = -1;

static int listMenuLastScroll = 0;
static int listMenuLastCursor = 0;

static const int TinyScreenH = 400;
static const int SmallScreenH = 500;
static const int TinyScreenW = 600;

void GetMenuPos(int* MenuX, int* MenuY)
{
    *MenuX = ScreenW / 2 - 100;
    *MenuY = ScreenH - 250;
    if(ScreenW < TinyScreenW)
    {
        *MenuX = ScreenW / 2 - 240;
        if(*MenuX < 24)
            *MenuX = 24;
    }
    if(ScreenH < TinyScreenH)
        *MenuY = 100;
    else if(ScreenH < SmallScreenH)
        *MenuY = ScreenH - 180;
    if(MenuMode == MENU_CHARACTER_SELECT_NEW)
    {
        if(ScreenH < TinyScreenH);
        else if(ScreenH < SmallScreenH)
            *MenuY -= 40;
    }
}

static void s_findRecentEpisode()
{
    menuRecentEpisode = -1;

    if((MenuMode == MENU_1PLAYER_GAME && !g_recentWorld1p.empty()) ||
       (MenuMode == MENU_2PLAYER_GAME && !g_recentWorld2p.empty()))
    {
        for(size_t i = 1; i < SelectWorld.size(); ++i)
        {
            auto &w = SelectWorld[i];
            const std::string wPath = w.WorldPath + w.WorldFile;

            if((MenuMode == MENU_1PLAYER_GAME && wPath == g_recentWorld1p) ||
               (MenuMode == MENU_2PLAYER_GAME && wPath == g_recentWorld2p) ||
               (MenuMode == MENU_EDITOR && wPath == g_recentWorldEditor))
            {
                menuRecentEpisode = i - 1;
                w.highlight = true;
            }
        }
    }
    else if(MenuMode == MENU_EDITOR && !g_recentWorldEditor.empty())
    {
        for(size_t i = 1; i < SelectWorldEditable.size(); ++i)
        {
            auto &w = SelectWorldEditable[i];
            const std::string wPath = w.WorldPath + w.WorldFile;

            if(wPath == g_recentWorldEditor)
            {
                menuRecentEpisode = i - 1;
                w.highlight = true;
            }
        }
    }

    if(menuRecentEpisode >= 0)
        worldCurs = menuRecentEpisode - 3;

    MenuCursor = (menuRecentEpisode < 0) ? 0 : menuRecentEpisode;
}


static int FindWorldsThread(void *)
{
    FindWorlds();
    return 0;
}

#if (defined(__APPLE__) && defined(USE_BUNDLED_ASSETS)) || defined(FIXED_ASSETS_PATH)
#   define USER_WORLDS_NEEDED
#   define WORLD_ROOTS_SIZE 2
#   define CAN_WRITE_APPPATH_WORLDS false
#else
#   define WORLD_ROOTS_SIZE 1
#   define CAN_WRITE_APPPATH_WORLDS true
#endif

struct WorldRoot_t
{
    std::string path;
    bool editable;
};

void FindWorlds()
{
    bool compatModern = (CompatGetLevel() == COMPAT_MODERN);
    NumSelectWorld = 0;

    std::vector<WorldRoot_t> worldRoots =
    {
        {AppPath + "worlds/", CAN_WRITE_APPPATH_WORLDS}
    };

    if(AppPathManager::userDirIsAvailable())
        worldRoots.push_back({AppPathManager::userWorldsRootDir(), true});

#ifdef __3DS__
    for(const std::string& root : AppPathManager.worldPackages())
        worldRoots.push_back({root, false});
#endif

    SelectWorld.clear();
    SelectWorld.emplace_back(SelectWorld_t()); // Dummy entry
    SelectWorldEditable.clear();
    SelectWorldEditable.push_back(SelectWorld_t()); // Dummy entry

#ifndef PGE_NO_THREADING
    SDL_AtomicSet(&loadingProgrss, 0);
    SDL_AtomicSet(&loadingProgrssMax, 0);

    for(const auto &worldsRoot : worldRoots)
    {
        std::vector<std::string> dirs;
        DirMan episodes(worldsRoot.path);
        episodes.getListOfFolders(dirs);
        SDL_AtomicAdd(&loadingProgrssMax, (int)dirs.size());
    }
#endif

    for(const auto &worldsRoot : worldRoots)
    {
        DirMan episodes(worldsRoot.path);

        std::vector<std::string> dirs;
        std::vector<std::string> files;
        episodes.getListOfFolders(dirs);
        WorldData head;

        for(auto &dir : dirs)
        {
            std::string epDir = worldsRoot.path + dir + "/";
            DirMan episode(epDir);
            episode.getListOfFiles(files, {".wld", ".wldx"});

            for(std::string &fName : files)
            {
                std::string wPath = epDir + fName;
                if(FileFormats::OpenWorldFileHeader(wPath, head))
                {
                    SelectWorld_t w;
                    w.WorldName = head.EpisodeTitle;
                    head.charactersToS64();
                    w.WorldPath = epDir;
                    w.WorldFile = fName;
                    if(w.WorldName.empty())
                        w.WorldName = fName;

                    w.blockChar[1] = head.nocharacter1;
                    w.blockChar[2] = head.nocharacter2;

                    if(head.meta.RecentFormat != LevelData::SMBX64 || head.meta.RecentFormatVersion >= 30 || !compatModern)
                    {
                        w.blockChar[3] = head.nocharacter3;
                        w.blockChar[4] = head.nocharacter4;
                        w.blockChar[5] = head.nocharacter5;
                    }
                    else
                    {
                        w.blockChar[3] = true;
                        w.blockChar[4] = true;
                        w.blockChar[5] = true;
                    }

                    SelectWorld.push_back(w);
                    if(worldsRoot.editable)
                        SelectWorldEditable.push_back(w);
                }
            }

#ifndef PGE_NO_THREADING
            SDL_AtomicAdd(&loadingProgrss, 1);
#endif
        }
    }

    if(SelectWorld.size() <= 1) // No available worlds in the list
    {
        SelectWorld.clear();
        SelectWorld.emplace_back(SelectWorld_t()); // Dummy entry
        SelectWorld.emplace_back(SelectWorld_t()); // "no battle levels" entry
        SelectWorld[1].WorldName = "<No episodes to play>";
        SelectWorld[1].disabled = true;
    }

    // Sort all worlds by alphabetical order
    std::sort(SelectWorld.begin(), SelectWorld.end(),
              [](const SelectWorld_t& a, const SelectWorld_t& b)
    {
        return a.WorldName < b.WorldName;
    });

    std::sort(SelectWorldEditable.begin(), SelectWorldEditable.end(),
              [](const SelectWorld_t& a, const SelectWorld_t& b)
    {
        return a.WorldName < b.WorldName;
    });

    NumSelectWorld = (int)(SelectWorld.size() - 1);

    SelectWorld_t createWorld = SelectWorld_t();
    createWorld.WorldName = "<New World>";
    SelectWorldEditable.push_back(createWorld);
    NumSelectWorldEditable = (SelectWorldEditable.size() - 1);

    s_findRecentEpisode();

    SDL_AtomicSet(&loading, 0);
}

static int FindLevelsThread(void *)
{
    FindLevels();
    return 0;
}

void FindLevels()
{
    std::vector<std::string> battleRoots =
    {
        AppPath + "battle/"
    };

    if(AppPathManager::userDirIsAvailable())
        battleRoots.push_back(AppPathManager::userBattleRootDir());

    SelectBattle.clear();
    SelectBattle.emplace_back(SelectWorld_t()); // Dummy entry

    NumSelectBattle = 1;
    SelectBattle.emplace_back(SelectWorld_t()); // "random level" entry
    SelectBattle[1].WorldName = "Random Level";
    LevelData head;

#ifndef PGE_NO_THREADING
    SDL_AtomicSet(&loadingProgrss, 0);
    SDL_AtomicSet(&loadingProgrssMax, 0);

    for(const auto &battleRoot : battleRoots)
    {
        std::vector<std::string> files;
        DirMan battleLvls(battleRoot);
        battleLvls.getListOfFiles(files, {".lvl", ".lvlx"});
        SDL_AtomicAdd(&loadingProgrssMax, (int)files.size());
    }
#endif

    for(const auto &battleRoot : battleRoots)
    {
        std::vector<std::string> files;
        DirMan battleLvls(battleRoot);
        battleLvls.getListOfFiles(files, {".lvl", ".lvlx"});
        for(std::string &fName : files)
        {
            std::string wPath = battleRoot + fName;
            if(FileFormats::OpenLevelFileHeader(wPath, head))
            {
                SelectWorld_t w;
                w.WorldPath = battleRoot;
                w.WorldFile = fName;
                w.WorldName = head.LevelName;
                if(w.WorldName.empty())
                    w.WorldName = fName;
                SelectBattle.push_back(w);
            }
#ifndef PGE_NO_THREADING
            SDL_AtomicAdd(&loadingProgrss, 1);
#endif
        }
    }

    NumSelectBattle = (SelectBattle.size() - 1);
#ifndef PGE_NO_THREADING
    SDL_AtomicSet(&loading, 0);
#endif

    if(SelectBattle.size() <= 2) // No available levels in the list
    {
        SelectBattle.clear();
        SelectBattle.emplace_back(SelectWorld_t()); // Dummy entry

        NumSelectBattle = 1;
        SelectBattle.emplace_back(SelectWorld_t()); // "no battle levels" entry
        SelectBattle[1].WorldName = "<No battle levels>";
        SelectBattle[1].disabled = true;
    }
}


static void s_handleMouseMove(int items, int x, int y, int maxWidth, int itemHeight)
{
    For(A, 0, items)
    {
        if(SharedCursor.Y >= y + A * itemHeight && SharedCursor.Y <= y + 16 + A * itemHeight)
        {
            if(SharedCursor.X >= x && SharedCursor.X <= x + maxWidth)
            {
                if(MenuMouseRelease && SharedCursor.Primary)
                    MenuMouseClick = true;
                if(MenuCursor != A)
                {
                    PlaySoundMenu(SFX_Slide);
                    MenuCursor = A;
                    break;
                }
            }
        }
    }
}



bool mainMenuUpdate()
{
    int MenuX, MenuY;
    GetMenuPos(&MenuX, &MenuY);

    int B;
    // Location_t tempLocation;
    int menuLen;
    // Player_t blankPlayer;

    bool upPressed = SharedControls.MenuUp;
    bool downPressed = SharedControls.MenuDown;
    bool leftPressed = SharedControls.MenuLeft;
    bool rightPressed = SharedControls.MenuRight;
    bool homePressed = SharedCursor.Tertiary;

    bool menuDoPress = SharedControls.MenuDo || SharedControls.Pause;
    bool menuBackPress = SharedControls.MenuBack;

    for(int i = 0; i < maxLocalPlayers; i++)
    {
        Controls_t &c = Player[i+1].Controls;

        menuDoPress |= c.Start || c.Jump;
        menuBackPress |= c.Run;

        upPressed |= c.Up;
        downPressed |= c.Down;
        leftPressed |= c.Left;
        rightPressed |= c.Right;

        homePressed |= c.Drop;
    }

    menuBackPress |= SharedCursor.Secondary && MenuMouseRelease;

    if(menuBackPress && menuDoPress)
        menuDoPress = false;

    {
        if(XWindow::getCursor() != AbstractWindow_t::CURSOR_NONE)
        {
            XWindow::setCursor(AbstractWindow_t::CURSOR_NONE);
            XWindow::showCursor(0);
        }

        {
            bool k = false;
            k |= menuBackPress;
            k |= menuDoPress;
            k |= upPressed;
            k |= downPressed;
            k |= leftPressed;
            k |= rightPressed;
            k |= homePressed;

            if(!k)
                MenuCursorCanMove = true;
        }

        if(!g_pollingInput && (MenuMode != MENU_CHARACTER_SELECT_NEW && MenuMode != MENU_CHARACTER_SELECT_NEW_BM && MenuMode != MENU_INTRO))
        {
            int cursorDelta = 0;

            if(upPressed)
            {
                if(MenuCursorCanMove)
                {
                    MenuCursor -= 1;
                    cursorDelta = -1;
                }

                MenuCursorCanMove = false;
            }
            else if(downPressed)
            {
                if(MenuCursorCanMove)
                {
                    MenuCursor += 1;
                    cursorDelta = +1;
                }
                MenuCursorCanMove = false;
            }

            if(cursorDelta != 0)
            {
                if(MenuMode >= MENU_CHARACTER_SELECT_BASE && MenuMode <= MENU_CHARACTER_SELECT_BASE_END)
                {
                    while((MenuCursor == (PlayerCharacter - 1) &&
                          (MenuMode == MENU_CHARACTER_SELECT_2P_S2 || MenuMode == MENU_CHARACTER_SELECT_BM_S2)) ||
                           blockCharacter[MenuCursor + 1])
                    {
                        MenuCursor += cursorDelta;
                        if(MenuCursor < 0)
                            MenuCursor = numCharacters - 1;
                        else if(MenuCursor >= numCharacters)
                            MenuCursor = 0;
                    }
                }
                PlaySoundMenu(SFX_Slide);
            }

        } // No keyboard/Joystick grabbing active

        if(MenuMode == MENU_INTRO && ScreenH >= TinyScreenH)
            MenuMode = MENU_MAIN;

        if(SDL_AtomicGet(&loading))
        {
            if((menuDoPress && MenuCursorCanMove) || MenuMouseClick)
                PlaySoundMenu(SFX_BlockHit);
            if(MenuCursor != 0)
                MenuCursor = 0;
        }
        else

        // Menu Intro
        if(MenuMode == MENU_INTRO)
        {
            if(MenuMouseRelease && SharedCursor.Primary)
                MenuMouseClick = true;
            if(menuBackPress && MenuCursorCanMove)
            {
                int quitKeyPos = 2;
                if(!g_gameInfo.disableTwoPlayer)
                    quitKeyPos ++;
                if(!g_gameInfo.disableBattleMode)
                    quitKeyPos ++;
                if(g_config.enable_editor)
                    quitKeyPos ++;

                MenuMode = MENU_MAIN;
                MenuCursor = quitKeyPos;
                MenuCursorCanMove = false;
                PlaySoundMenu(SFX_Slide);
            }
            if((menuDoPress && MenuCursorCanMove) || MenuMouseClick)
            {
                MenuCursorCanMove = false;
                MenuMode = MENU_MAIN;
                PlaySoundMenu(SFX_Do);
            }
        }
        // Main Menu
        else if(MenuMode == MENU_MAIN)
        {
            if(SharedCursor.Move)
            {
                For(A, 0, 10)
                {
                    if(SharedCursor.Y >= MenuY + A * 30 && SharedCursor.Y <= MenuY + A * 30 + 16)
                    {
                        int i = 0;
                        if(A == i++)
                            menuLen = 18 * (g_gameInfo.disableTwoPlayer ? g_mainMenu.main1PlayerGame.size() : g_mainMenu.mainStartGame.size()) - 2;
                        else if(!g_gameInfo.disableTwoPlayer && A == i++)
                            menuLen = 18 * g_mainMenu.mainMultiplayerGame.size() - 2;
                        else if(!g_gameInfo.disableBattleMode && A == i++)
                            menuLen = 18 * g_mainMenu.mainBattleGame.size();
                        else if(g_config.enable_editor && A == i++)
                            menuLen = 18 * g_mainMenu.mainEditor.size();
                        else if(A == i++)
                            menuLen = 18 * g_mainMenu.mainOptions.size();
                        else if(A == i++)
                            menuLen = 18 * g_mainMenu.mainExit.size();
                        else
                            break;

                        if(SharedCursor.X >= MenuX && SharedCursor.X <= MenuX + menuLen)
                        {
                            if(MenuMouseRelease && SharedCursor.Primary)
                                MenuMouseClick = true;

                            if(MenuCursor != A)
                            {
                                PlaySoundMenu(SFX_Slide);
                                MenuCursor = A;
                            }
                        }
                    }
                }
            }

            if(menuBackPress && MenuCursorCanMove)
            {
                int quitKeyPos = 2;
                if(!g_gameInfo.disableTwoPlayer)
                    quitKeyPos ++;
                if(!g_gameInfo.disableBattleMode)
                    quitKeyPos ++;
                if(g_config.enable_editor)
                    quitKeyPos ++;

                if(ScreenH < TinyScreenH)
                {
                    MenuCursorCanMove = false;
                    MenuMode = MENU_INTRO;
                    PlaySoundMenu(SFX_Slide);
                }
                else if(MenuCursor != quitKeyPos)
                {
                    MenuCursor = quitKeyPos;
                    PlaySoundMenu(SFX_Slide);
                }
            }
            else if((menuDoPress && MenuCursorCanMove) || MenuMouseClick)
            {
                MenuCursorCanMove = false;
                PlayerCharacter = 0;
                PlayerCharacter2 = 0;

                int i = 0;
                if(MenuCursor == i++)
                {
                    PlaySoundMenu(SFX_Do);
                    MenuMode = MENU_1PLAYER_GAME;
                    menuPlayersNum = 1;
                    menuBattleMode = false;
                    MenuCursor = 0;
#if !defined(THEXTECH_PRELOAD_LEVELS) && defined(PGE_NO_THREADING)
                    FindWorlds();
#elif !defined(THEXTECH_PRELOAD_LEVELS)
                    SDL_AtomicSet(&loading, 1);
                    loadingThread = SDL_CreateThread(FindWorldsThread, "FindWorlds", nullptr);
                    SDL_DetachThread(loadingThread);
#else
                    s_findRecentEpisode();
#endif
                }
                else if(!g_gameInfo.disableTwoPlayer && MenuCursor == i++)
                {
                    PlaySoundMenu(SFX_Do);
                    MenuMode = MENU_2PLAYER_GAME;
                    menuPlayersNum = 2;
                    menuBattleMode = false;
                    MenuCursor = 0;
#if !defined(THEXTECH_PRELOAD_LEVELS) && defined(PGE_NO_THREADING)
                    FindWorlds();
#elif !defined(THEXTECH_PRELOAD_LEVELS)
                    SDL_AtomicSet(&loading, 1);
                    loadingThread = SDL_CreateThread(FindWorldsThread, "FindWorlds", nullptr);
                    SDL_DetachThread(loadingThread);
#else
                    s_findRecentEpisode();
#endif
                }
                else if(!g_gameInfo.disableBattleMode && MenuCursor == i++)
                {
                    PlaySoundMenu(SFX_Do);
                    MenuMode = MENU_BATTLE_MODE;
                    menuPlayersNum = 2;
                    menuBattleMode = true;
#if !defined(THEXTECH_PRELOAD_LEVELS) && defined(PGE_NO_THREADING)
                    FindLevels();
#elif !defined(THEXTECH_PRELOAD_LEVELS)
                    SDL_AtomicSet(&loading, 1);
                    loadingThread = SDL_CreateThread(FindLevelsThread, "FindLevels", nullptr);
                    SDL_DetachThread(loadingThread);
#endif
                    MenuCursor = 0;
                }
                else if(g_config.enable_editor && MenuCursor == i++)
                {
                    if(ScreenW < 640 || ScreenH < 480)
                    {
                        PlaySoundMenu(SFX_BlockHit);
                        MessageText = "Sorry! The in-game editor is not supported at your current resolution.";
                        PauseGame(PauseCode::Message);
                    }
                    else if(!GFX.EIcons.inited)
                    {
                        PlaySoundMenu(SFX_BlockHit);
                        MessageText = "Sorry! You are missing EditorIcons.png, the icons for the in-game editor.";
                        PauseGame(PauseCode::Message);
                    }
                    else
                    {
                        PlaySoundMenu(SFX_Do);
                        MenuMode = MENU_EDITOR;
                        MenuCursor = 0;

#if !defined(THEXTECH_PRELOAD_LEVELS) && defined(PGE_NO_THREADING)
                        FindWorlds();
#elif !defined(THEXTECH_PRELOAD_LEVELS)
                        SDL_AtomicSet(&loading, 1);
                        loadingThread = SDL_CreateThread(FindWorldsThread, "FindWorlds", nullptr);
                        SDL_DetachThread(loadingThread);
#else
                        s_findRecentEpisode();
#endif
                    }
                }
                else if(MenuCursor == i++)
                {
                    PlaySoundMenu(SFX_Do);
                    MenuMode = MENU_OPTIONS;
                    MenuCursor = 0;
                }
                else if(MenuCursor == i++)
                {
                    PlaySoundMenu(SFX_Do);
                    XRender::setTargetTexture();
                    XRender::clearBuffer();
                    StopMusic();
                    XRender::repaint();
                    XEvents::doEvents();
                    PGE_Delay(500);
                    KillIt();
                    return true;
                }

            }


            int quitKeyPos = 2;
            if(!g_gameInfo.disableTwoPlayer)
                quitKeyPos ++;
            if(!g_gameInfo.disableBattleMode)
                quitKeyPos ++;
            if(g_config.enable_editor)
                quitKeyPos ++;

            if(MenuCursor > quitKeyPos)
                MenuCursor = 0;
            if(MenuCursor < 0)
                MenuCursor = quitKeyPos;
        } // Main Menu

        // Character Select
        else if(MenuMode == MENU_CHARACTER_SELECT_NEW ||
                MenuMode == MENU_CHARACTER_SELECT_NEW_BM)
        {
            int ret = ConnectScreen::Logic();
            if(ret == -1)
            {
                if(MenuMode == MENU_CHARACTER_SELECT_NEW_BM)
                {
                    MenuCursor = selWorld - 1;
                    MenuMode = MENU_BATTLE_MODE;
                }
                else
                {
                    MenuCursor = selSave - 1;
                    if(menuPlayersNum == 1)
                        MenuMode = MENU_SELECT_SLOT_1P;
                    else
                        MenuMode = MENU_SELECT_SLOT_2P;
                }
                MenuCursorCanMove = false;
            }
            else if(ret == 1)
            {
                if(MenuMode == MENU_CHARACTER_SELECT_NEW)
                {
                    MenuCursor = 0;
                    StartEpisode();
                    return true;
                }
                else if(MenuMode == MENU_CHARACTER_SELECT_NEW_BM)
                {
                    MenuCursor = 0;
                    StartBattleMode();
                    return true;
                }
            }
        }
#if 0 // old code, no longer used
        // Character Select
        else if(MenuMode == MENU_CHARACTER_SELECT_1P ||
                MenuMode == MENU_CHARACTER_SELECT_2P_S1 ||
                MenuMode == MENU_CHARACTER_SELECT_2P_S2 ||
                MenuMode == MENU_CHARACTER_SELECT_BM_S1 ||
                MenuMode == MENU_CHARACTER_SELECT_BM_S2)
        {
            if(SharedCursor.Move)
            {
                B = 0;
                For(A, 0, 4)
                {
                    if(blockCharacter[A + 1])
                    {
                        B -= 30;
                    }
                    else
                    {
                        if(SharedCursor.Y >= MenuY + A * 30 + B && SharedCursor.Y <= 366 + A * 30 + B)
                        {
                            if(A >= 0 && A < numCharacters)
                            {
                                menuLen = 18 * g_mainMenu.selectPlayer[A + 1].size();
                                if(A == 0)
                                    menuLen += 2;
                            }
                            else
                            {
                                menuLen = 180;
                            }

                            if(SharedCursor.X >= MenuX && SharedCursor.X <= MenuX + menuLen)
                            {
                                if(MenuMouseRelease && SharedCursor.Primary)
                                    MenuMouseClick = true;
                                if(MenuCursor != A)
                                {
                                    if(
                                        ((MenuMode == MENU_CHARACTER_SELECT_2P_S2 || MenuMode == MENU_CHARACTER_SELECT_BM_S2) && PlayerCharacter - 1 == A) ||
                                        ((blockCharacter[A + 1]) && MenuMouseClick)
                                    )
                                    {
                                        MenuMouseClick = false;
                                    }
                                    else
                                    {
                                        PlaySoundMenu(SFX_Slide);
                                        MenuCursor = A;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if(MenuCursorCanMove || MenuMouseClick)
            {
                if(menuBackPress)
                {
                    if(MenuMode == MENU_CHARACTER_SELECT_2P_S2)
                    {
                        MenuMode = MENU_CHARACTER_SELECT_2P_S1;
                        MenuCursor = PlayerCharacter - 1;
                    }
                    else if(MenuMode == MENU_CHARACTER_SELECT_BM_S2)
                    {
                        MenuMode = MENU_CHARACTER_SELECT_BM_S1;
                        MenuCursor = PlayerCharacter - 1;
                    }
                    else
                    {
                        // MenuCursor = selWorld - 1;
                        MenuMode /= MENU_CHARACTER_SELECT_BASE;
                        // Restore menu state
                        worldCurs = listMenuLastScroll;
                        MenuCursor = listMenuLastCursor;
                    }

                    MenuCursorCanMove = false;
                    PlaySoundMenu(SFX_Slide);
                }
                else if(menuDoPress || MenuMouseClick)
                {
                    PlaySoundMenu(SFX_Do);

                    if(MenuMode == MENU_CHARACTER_SELECT_1P)
                    {
                        PlayerCharacter = MenuCursor + 1;
                        MenuMode = MENU_SELECT_SLOT_1P;
                        MenuCursor = 0;
                    }
                    else if(MenuMode == MENU_CHARACTER_SELECT_2P_S1)
                    {
                        PlayerCharacter = MenuCursor + 1;
                        MenuMode = MENU_CHARACTER_SELECT_2P_S2;
                        MenuCursor = PlayerCharacter2;
                    }
                    else if(MenuMode == MENU_CHARACTER_SELECT_2P_S2)
                    {
                        PlayerCharacter2 = MenuCursor + 1;
                        MenuMode = MENU_SELECT_SLOT_2P;
                        MenuCursor = 0;
                    }
                    else if(MenuMode == MENU_CHARACTER_SELECT_BM_S1)
                    {
                        PlayerCharacter = MenuCursor + 1;
                        MenuMode = MENU_CHARACTER_SELECT_BM_S2;
                        MenuCursor = PlayerCharacter2 - 1;
                        if(MenuCursor < 0)
                            MenuCursor = 0;
                    }
                    else if(MenuMode == MENU_CHARACTER_SELECT_BM_S2)
                    {
                        PlayerCharacter2 = MenuCursor + 1;
                        MenuCursor = 0;
                        StartBattleMode();
                        return true;
                    }

                    MenuCursorCanMove = false;
                }
            }

            bool isListMenu = (MenuMode == MENU_1PLAYER_GAME || MenuMode == MENU_2PLAYER_GAME || MenuMode == MENU_BATTLE_MODE);

            if(MenuMode > MENU_MAIN && !isListMenu)
            {
                if(MenuCursor > numCharacters - 1)
                {
                    MenuCursor = 0;

                    while((MenuCursor == PlayerCharacter - 1 && (MenuMode == MENU_CHARACTER_SELECT_2P_S2 || MenuMode == MENU_CHARACTER_SELECT_BM_S2)) ||
                          blockCharacter[MenuCursor + 1])
                    {
                        MenuCursor += 1;
                    }

                }

                if(MenuCursor < 0)
                {
                    MenuCursor = numCharacters - 1;

                    while((MenuCursor == PlayerCharacter - 1 && (MenuMode == MENU_CHARACTER_SELECT_2P_S2 || MenuMode == MENU_CHARACTER_SELECT_BM_S2)) ||
                          blockCharacter[MenuCursor + 1])
                    {
                        MenuCursor -= 1;
                    }
                }
            }

            if(!isListMenu)
            {
                while(((MenuMode == MENU_CHARACTER_SELECT_2P_S2 || MenuMode == MENU_CHARACTER_SELECT_BM_S2) && MenuCursor == PlayerCharacter - 1) ||
                       blockCharacter[MenuCursor + 1])
                {
                    MenuCursor += 1;
                }
            }

            if(MenuMode >= MENU_CHARACTER_SELECT_BASE && MenuMode <= MENU_CHARACTER_SELECT_BASE_END)
            {
                if(MenuCursor >= numCharacters)
                {
                    MenuCursor = 0;
                }
                else
                {
                    For(A, 1, numPlayers)
                    {
                        Player[A].Character = MenuCursor + 1;
                        SizeCheck(A);
                    }

                    For(A, 1, numNPCs)
                    {
                        if(NPC[A].Type == 13)
                            NPC[A].Special = MenuCursor + 1;
                    }
                }
            }
        } // Character Select
#endif

        // World Select
        else if(MenuMode == MENU_1PLAYER_GAME || MenuMode == MENU_2PLAYER_GAME
            || MenuMode == MENU_BATTLE_MODE || MenuMode == MENU_EDITOR)
        {
            const std::vector<SelectWorld_t>& SelectorList
                = (MenuMode == MENU_BATTLE_MODE) ? SelectBattle :
                    ((MenuMode == MENU_EDITOR) ? SelectWorldEditable :
                        SelectWorld);

            if(ScrollDelay > 0)
            {
                SharedCursor.Move = true;
                ScrollDelay -= 1;
            }

            if(SharedCursor.Move)
            {
                B = 0;

                For(A, minShow - 1, maxShow - 1)
                {
                    if(SharedCursor.Y >= MenuY + B * 30 && SharedCursor.Y <= MenuY + B * 30 + 16)
                    {
                        menuLen = 19 * static_cast<int>(SelectorList[A + 1].WorldName.size());

                        if(SharedCursor.X >= MenuX && SharedCursor.X <= MenuX + menuLen)
                        {
                            if(MenuMouseRelease && SharedCursor.Primary)
                                MenuMouseClick = true;

                            if(MenuCursor != A && ScrollDelay == 0)
                            {
                                ScrollDelay = 10;
                                PlaySoundMenu(SFX_Slide);
                                MenuCursor = A;
                            }
                        }
                    }

                    B += 1;
                }
            }

            if(MenuCursorCanMove || MenuMouseClick)
            {
                if(menuBackPress)
                {
                    MenuCursor = MenuMode - 1;

                    if(MenuMode == MENU_BATTLE_MODE)
                    {
                        MenuCursor = g_gameInfo.disableTwoPlayer ? 1 : 2;
                    }
                    else if(MenuMode == MENU_EDITOR)
                    {
                        MenuCursor = 3;
                        if(g_gameInfo.disableTwoPlayer)
                            MenuCursor--;
                        if(g_gameInfo.disableBattleMode)
                            MenuCursor--;
                    }

                    MenuMode = MENU_MAIN;
//'world select back

                    PlaySoundMenu(SFX_Slide);
                    MenuCursorCanMove = false;
                }
                else if(menuDoPress || MenuMouseClick)
                {
                    bool disabled = false;
                    // Save menu state
                    listMenuLastScroll = worldCurs;
                    listMenuLastCursor = MenuCursor;

                    selWorld = MenuCursor + 1;

                    if((MenuMode == MENU_BATTLE_MODE && SelectBattle[selWorld].disabled) ||
                       ((MenuMode == MENU_1PLAYER_GAME || MenuMode == MENU_2PLAYER_GAME) && SelectWorld[selWorld].disabled))
                        disabled = true;

                    if(!disabled)
                        PlaySoundMenu(SFX_Do);

                    if(disabled)
                    {
                        PlaySoundMenu(SFX_BlockHit);
                        // Do nothing. stay at menu
                    }
                    // level editor
                    else if(MenuMode == MENU_EDITOR)
                    {
                        if(selWorld == NumSelectWorldEditable)
                        {
                            ClearWorld(true);
                            WorldName = TextEntryScreen::Run("New world name");
                            if(!WorldName.empty())
                            {
                                std::string fn = WorldName;
                                // eliminate bad characters
                                std::replace(fn.begin(), fn.end(), '/', '_');
                                std::replace(fn.begin(), fn.end(), '\\', '_');
                                std::replace(fn.begin(), fn.end(), '.', '_');
                                std::replace(fn.begin(), fn.end(), ':', '_');
                                std::replace(fn.begin(), fn.end(), '<', '_');
                                std::replace(fn.begin(), fn.end(), '>', '_');
                                std::replace(fn.begin(), fn.end(), '"', '_');
                                std::replace(fn.begin(), fn.end(), '|', '_');
                                std::replace(fn.begin(), fn.end(), '?', '_');
                                std::replace(fn.begin(), fn.end(), '*', '_');
                                // ensure uniqueness (but still case-sensitive for now)
                                while(DirMan::exists(AppPathManager::userWorldsRootDir() + fn))
                                    fn += "2";
                                DirMan::mkAbsPath(AppPathManager::userWorldsRootDir() + fn);

                                std::string wPath = AppPathManager::userWorldsRootDir() + fn + "/world.wld";
                                if(g_config.editor_preferred_file_format != FileFormats::WLD_SMBX64 && g_config.editor_preferred_file_format != FileFormats::WLD_SMBX38A)
                                    wPath += "x";
                                g_recentWorldEditor = wPath;

                                SaveWorld(wPath, g_config.editor_preferred_file_format);

#ifdef PGE_NO_THREADING
                                FindWorlds();
#else
                                SDL_AtomicSet(&loading, 1);
                                loadingThread = SDL_CreateThread(FindWorldsThread, "FindWorlds", NULL);
#endif
                            }
                        }
                        else
                        {
                            GameMenu = false;
                            LevelSelect = false;
                            BattleMode = false;
                            LevelEditor = true;
                            WorldEditor = true;
                            ClearLevel();
                            ClearGame();
                            std::string wPath = SelectWorldEditable[selWorld].WorldPath
                                + SelectWorldEditable[selWorld].WorldFile;
                            OpenWorld(wPath);
                            if(g_recentWorldEditor != wPath)
                            {
                                g_recentWorldEditor = wPath;
                                SaveConfig();
                            }
                            editorScreen.ResetCursor();
                            editorScreen.active = false;
                            MouseRelease = false;
                            return true;
                        }
                    }
                    // game mode
                    else
                    {
                        if(MenuMode != MENU_BATTLE_MODE)
                            FindSaves();

                        For(A, 1, numCharacters)
                        {
                            if(MenuMode == MENU_BATTLE_MODE)
                                blockCharacter[A] = false;
                            else
                                blockCharacter[A] = SelectWorld[selWorld].blockChar[A];
                        }

                        if(MenuMode == MENU_BATTLE_MODE)
                        {
                            MenuMode = MENU_CHARACTER_SELECT_NEW_BM;
                            ConnectScreen::MainMenu_Start(2);
                        }
                        else
                        {
                            MenuMode *= MENU_SELECT_SLOT_BASE;
                            MenuCursor = 0;
                        }
                    }

                    MenuCursorCanMove = false;
                }

            }

            // New world select scroll options!
            // Based on Wohlstand's but somewhat simpler and less keyboard-specific.
            // Left and right are -/+ 3 (repeatable, so they also provide a quick-first/quick-last function).
            // DropItem / Tertiary cursor button is return to last episode.
            bool dontWrap = false;

            if(leftPressed && (MenuCursorCanMove || ScrollDelay == 0))
            {
                PlaySoundMenu(SFX_Saw);
                MenuCursor -= 3;
                MenuCursorCanMove = false;
                ScrollDelay = 15;
                dontWrap = true;
            }
            else if(rightPressed && (MenuCursorCanMove || ScrollDelay == 0))
            {
                PlaySoundMenu(SFX_Saw);
                MenuCursor += 3;
                MenuCursorCanMove = false;
                ScrollDelay = 15;
                dontWrap = true;
            }
            else if((leftPressed || rightPressed) && !SharedCursor.Move)
            {
                ScrollDelay -= 1;
            }
            else if(SharedCursor.ScrollUp)
            {
                PlaySoundMenu(SFX_Saw);
                MenuCursor -= 1;
                dontWrap = true;
            }
            else if(SharedCursor.ScrollDown)
            {
                PlaySoundMenu(SFX_Saw);
                MenuCursor += 1;
                dontWrap = true;
            }

            if(homePressed && MenuCursorCanMove && menuRecentEpisode >= 0)
            {
                PlaySoundMenu(SFX_Camera);
                MenuCursor = menuRecentEpisode;
                MenuCursorCanMove = false;
                dontWrap = true;
            }
            
            if(MenuMode == MENU_1PLAYER_GAME || MenuMode == MENU_2PLAYER_GAME
                || MenuMode == MENU_BATTLE_MODE || MenuMode == MENU_EDITOR)
            {
                maxShow = (MenuMode == MENU_BATTLE_MODE) ? NumSelectBattle :
                    ((MenuMode == MENU_EDITOR) ? NumSelectWorldEditable :
                        NumSelectWorld);

                if(dontWrap)
                {
                    if(MenuCursor >= maxShow)
                        MenuCursor = maxShow - 1;
                    if(MenuCursor < 0)
                        MenuCursor = 0;
                }
                else
                {
                    if(MenuCursor >= maxShow)
                        MenuCursor = 0;
                    if(MenuCursor < 0)
                        MenuCursor = maxShow - 1;
                }
            }
        } // World select

        // Save Select
        else if(MenuMode == MENU_SELECT_SLOT_1P || MenuMode == MENU_SELECT_SLOT_2P)
        {
            if(SharedCursor.Move)
                s_handleMouseMove(4, MenuX, MenuY, MenuX, 30);

            if(MenuCursorCanMove || MenuMouseClick)
            {
                if(menuBackPress)
                {
//'save select back
                    MenuMode /= MENU_SELECT_SLOT_BASE;
                    // Restore menu state
                    worldCurs = listMenuLastScroll;
                    MenuCursor = listMenuLastCursor;

                    MenuCursorCanMove = false;
                    PlaySoundMenu(SFX_Slide);
                }
                else if(menuDoPress || MenuMouseClick)
                {
                    PlaySoundMenu(SFX_Do);

                    if(MenuCursor >= 0 && MenuCursor <= 2) // Select the save slot, but still need to select players
                    {
                        selSave = MenuCursor + 1;
                        if(MenuMode == MENU_SELECT_SLOT_2P)
                            ConnectScreen::MainMenu_Start(2);
                        else
                            ConnectScreen::MainMenu_Start(1);
                        MenuMode = MENU_CHARACTER_SELECT_NEW;
                        MenuCursorCanMove = false;
                    }
                    else if(MenuCursor == 3) // Copy the gamesave
                    {
                        MenuCursor = 0;
                        MenuMode += MENU_SELECT_SLOT_COPY_S1_ADD;
                        MenuCursorCanMove = false;
                    }
                    else if(MenuCursor == 4) // Delete the gamesave
                    {
                        MenuCursor = 0;
                        MenuMode += MENU_SELECT_SLOT_DELETE_ADD;
                        MenuCursorCanMove = false;
                    }
                }
            }

            if(MenuMode == MENU_SELECT_SLOT_1P || MenuMode == MENU_SELECT_SLOT_2P)
            {
                if(MenuCursor > 4) MenuCursor = 0;
                if(MenuCursor < 0) MenuCursor = 4;
            }
        } // Save Slot Select

        // Save Select
        else if(MenuMode == MENU_SELECT_SLOT_1P_COPY_S1 || MenuMode == MENU_SELECT_SLOT_2P_COPY_S1 ||
                MenuMode == MENU_SELECT_SLOT_1P_COPY_S2 || MenuMode == MENU_SELECT_SLOT_2P_COPY_S2)
        {
            if(SharedCursor.Move)
                s_handleMouseMove(2, MenuX, MenuY, MenuX, 30);

            if(MenuCursorCanMove || MenuMouseClick)
            {
                if(menuBackPress)
                {
//'save select back
                    if(MenuMode == MENU_SELECT_SLOT_1P_COPY_S2 || MenuMode == MENU_SELECT_SLOT_2P_COPY_S2)
                    {
                        MenuMode -= MENU_SELECT_SLOT_COPY_S1_ADD;
                    }
                    else
                    {
                        MenuMode -= MENU_SELECT_SLOT_COPY_S1_ADD;
                        MenuCursor = 3;
                    }

                    MenuCursorCanMove = false;
                    PlaySoundMenu(SFX_Do);
                }
                else if(menuDoPress || MenuMouseClick)
                {
                    SDL_assert_release(IF_INRANGE(MenuCursor, 0, maxSaveSlots - 1));
                    int slot = MenuCursor + 1;

                    if(MenuMode == MENU_SELECT_SLOT_1P_COPY_S1 || MenuMode == MENU_SELECT_SLOT_2P_COPY_S1)
                    {
                        if(SaveSlot[slot] < 0)
                            PlaySoundMenu(SFX_BlockHit);
                        else
                        {
                            PlaySoundMenu(SFX_Do);
                            menuCopySaveSrc = slot;
                            MenuMode += MENU_SELECT_SLOT_COPY_S1_ADD;
                        }
                        MenuCursorCanMove = false;
                    }
                    else if(menuCopySaveSrc == slot)
                    {
                        PlaySoundMenu(SFX_BlockHit);
                        MenuCursorCanMove = false;
                    }
                    else
                    {
                        PlaySoundMenu(SFX_Raccoon);
                        menuCopySaveDst = slot;
                        CopySave(selWorld, menuCopySaveSrc, menuCopySaveDst);
                        FindSaves();
                        MenuMode -= MENU_SELECT_SLOT_COPY_S2_ADD;
                        MenuCursor = 3;
                        MenuCursorCanMove = false;
                    }
                }
            }

            if(MenuMode == MENU_SELECT_SLOT_1P_COPY_S1 || MenuMode == MENU_SELECT_SLOT_2P_COPY_S1 ||
               MenuMode == MENU_SELECT_SLOT_1P_COPY_S2 || MenuMode == MENU_SELECT_SLOT_2P_COPY_S2)
            {
                if(MenuCursor > 2) MenuCursor = 0;
                if(MenuCursor < 0) MenuCursor = 2;
            }
        }

        // Delete gamesave
        else if(MenuMode == MENU_SELECT_SLOT_1P_DELETE || MenuMode == MENU_SELECT_SLOT_2P_DELETE)
        {
            if(SharedCursor.Move)
                s_handleMouseMove(2, MenuX, MenuY, MenuX, 30);

            if(MenuCursorCanMove || MenuMouseClick)
            {
                if(menuBackPress)
                {
//'save select back
                    MenuMode -= MENU_SELECT_SLOT_DELETE_ADD;
                    MenuCursor = 4;
                    PlaySoundMenu(SFX_Do);
                    MenuCursorCanMove = false;
                }
                else if(menuDoPress || MenuMouseClick)
                {
                    MenuMode -= MENU_SELECT_SLOT_DELETE_ADD;
                    DeleteSave(selWorld, (MenuCursor + 1));
                    FindSaves();
                    MenuCursor = 4;
                    PlaySoundMenu(SFX_Blaarg);
                    MenuCursorCanMove = false;
                }
            }

            if(MenuMode == MENU_SELECT_SLOT_1P_DELETE || MenuMode == MENU_SELECT_SLOT_2P_DELETE)
            {
                if(MenuCursor > 2) MenuCursor = 0;
                if(MenuCursor < 0) MenuCursor = 2;
            }
        }

        // Options
        else if(MenuMode == MENU_OPTIONS)
        {
            int optionsMenuLength = 1; // controls, credits
#ifndef RENDER_FULLSCREEN_ALWAYS
            optionsMenuLength++;
#endif
#if !defined(__3DS__) && !defined(VITA)
            optionsMenuLength ++; // ScaleMode
#endif
#ifndef FIXED_RES
            optionsMenuLength ++; // resolution
#endif

            if(SharedCursor.Move)
            {
                For(A, 0, optionsMenuLength)
                {
                    if(SharedCursor.Y >= MenuY + A * 30 && SharedCursor.Y <= MenuY + A * 30 + 16)
                    {
                        int i = 0;
                        if(A == i++)
                            menuLen = 18 * g_mainMenu.controlsTitle.size();
#ifndef RENDER_FULLSCREEN_ALWAYS
                        else if(A == i++)
                        {
                            if(resChanged)
                                menuLen = 18 * 13; // std::strlen("windowed mode")
                            else
                                menuLen = 18 * 15; // std::strlen("fullscreen mode")
                        }
#endif
#if !defined(__3DS__) && !defined(VITA)
                        else if(A == i++)
                            menuLen = 18 * (7 + ScaleMode_strings.at(g_videoSettings.scaleMode).length());
#endif
#if !defined(FIXED_RES)
                        else if(A == i++)
                            menuLen = 18 * std::strlen("res: WWWxHHH (word)");
#endif
                        else
                            menuLen = 18 * 12 - 2; // std::strlen("view credits")

                        if(SharedCursor.X >= MenuX && SharedCursor.X <= MenuX + menuLen)
                        {
                            if(MenuMouseRelease && SharedCursor.Primary)
                                MenuMouseClick = true;

                            if(MenuCursor != A)
                            {
                                PlaySoundMenu(SFX_Slide);
                                MenuCursor = A;
                            }
                        }
                    }
                }
            }

            if(MenuCursorCanMove || MenuMouseClick)
            {
                if(menuBackPress)
                {
                    SaveConfig();

                    int optionsIndex = 1;
                    if(!g_gameInfo.disableTwoPlayer)
                        optionsIndex++;
                    if(!g_gameInfo.disableBattleMode)
                        optionsIndex++;
                    if(g_config.enable_editor)
                        optionsIndex++;
                    MenuMode = MENU_MAIN;
                    MenuCursor = optionsIndex;
                    MenuCursorCanMove = false;
                    PlaySoundMenu(SFX_Slide);
                }
                else if(menuDoPress || MenuMouseClick || leftPressed || rightPressed)
                {
                    MenuCursorCanMove = false;
                    int i = 0;
                    if(MenuCursor == i++)
                    {
                        MenuCursor = 0;
                        MenuMode = MENU_INPUT_SETTINGS;
                        PlaySoundMenu(SFX_Do);
                    }
#ifndef RENDER_FULLSCREEN_ALWAYS // on Android and some other platforms, run the always full-screen
                    else if(MenuCursor == i++)
                    {
                        PlaySoundMenu(SFX_Do);
                        ChangeScreen();
                    }
#endif
#if !defined(__3DS__) && !defined(VITA)
                    else if(MenuCursor == i++)
                    {
                        PlaySoundMenu(SFX_Do);
                        if(!leftPressed)
                            g_videoSettings.scaleMode = g_videoSettings.scaleMode + 1;
                        else
                            g_videoSettings.scaleMode = g_videoSettings.scaleMode - 1;
                        if(g_videoSettings.scaleMode > SCALE_FIXED_2X)
                            g_videoSettings.scaleMode = SCALE_DYNAMIC_INTEGER;
                        if(g_videoSettings.scaleMode < SCALE_DYNAMIC_INTEGER)
                            g_videoSettings.scaleMode = SCALE_FIXED_2X;
                        UpdateWindowRes();
                        UpdateInternalRes();
                    }
#endif
#ifndef FIXED_RES
                    else if(MenuCursor == i++)
                    {
                        PlaySoundMenu(SFX_Do);
                        if(!leftPressed)
                        {
                            if (g_config.InternalW == 0 && g_config.InternalH == 0)
                                { g_config.InternalW = 480; g_config.InternalH = 320; }
                            else if (g_config.InternalW == 480 && g_config.InternalH == 320)
                                { g_config.InternalW = 512; g_config.InternalH = 384; }
                            else if (g_config.InternalW == 512 && g_config.InternalH == 384)
                                { g_config.InternalW = 512; g_config.InternalH = 448; }
                            else if (g_config.InternalW == 512 && g_config.InternalH == 448)
                                { g_config.InternalW = 768; g_config.InternalH = 432; }
                            else if (g_config.InternalW == 768 && g_config.InternalH == 432)
                                { g_config.InternalW = 640; g_config.InternalH = 480; }
                            else if (g_config.InternalW == 640 && g_config.InternalH == 480)
                                { g_config.InternalW = 800; g_config.InternalH = 480; }
                            else if (g_config.InternalW == 800 && g_config.InternalH == 480)
                                { g_config.InternalW = 800; g_config.InternalH = 600; }
                            else if (g_config.InternalW == 800 && g_config.InternalH == 600)
                                { g_config.InternalW = 1280; g_config.InternalH = 720; }
                            else if (g_config.InternalW == 1280 && g_config.InternalH == 720)
                                { g_config.InternalW = 0; g_config.InternalH = 600; }
                            else if (g_config.InternalW == 0 && g_config.InternalH == 600)
                                { g_config.InternalW = 0; g_config.InternalH = 0; }
                            else
                                { g_config.InternalW = 0; g_config.InternalH = 0; }
                        }
                        else
                        {
                            if (g_config.InternalW == 0 && g_config.InternalH == 0)
                                { g_config.InternalW = 0; g_config.InternalH = 600; }
                            else if (g_config.InternalW == 480 && g_config.InternalH == 320)
                                { g_config.InternalW = 0; g_config.InternalH = 0; }
                            else if (g_config.InternalW == 512 && g_config.InternalH == 384)
                                { g_config.InternalW = 480; g_config.InternalH = 320; }
                            else if (g_config.InternalW == 512 && g_config.InternalH == 448)
                                { g_config.InternalW = 512; g_config.InternalH = 384; }
                            else if (g_config.InternalW == 640 && g_config.InternalH == 480)
                                { g_config.InternalW = 768; g_config.InternalH = 432; }
                            else if (g_config.InternalW == 768 && g_config.InternalH == 432)
                                { g_config.InternalW = 512; g_config.InternalH = 448; }
                            else if (g_config.InternalW == 800 && g_config.InternalH == 480)
                                { g_config.InternalW = 640; g_config.InternalH = 480; }
                            else if (g_config.InternalW == 800 && g_config.InternalH == 600)
                                { g_config.InternalW = 800; g_config.InternalH = 480; }
                            else if (g_config.InternalW == 1280 && g_config.InternalH == 720)
                                { g_config.InternalW = 800; g_config.InternalH = 600; }
                            else if (g_config.InternalW == 0 && g_config.InternalH == 600)
                                { g_config.InternalW = 1280; g_config.InternalH = 720; }
                            else
                                { g_config.InternalW = 0; g_config.InternalH = 0; }
                        }
                        UpdateWindowRes();
                        UpdateInternalRes();
                    }
#endif
                    else if(MenuCursor == i++)
                    {
                        PlaySoundMenu(SFX_Do);
                        GameMenu = false;
                        GameOutro = true;
                        CreditChop = MenuX;
                        EndCredits = 0;
                        SetupCredits();
                    }

                }

            }

            if(MenuMode == MENU_OPTIONS)
            {
                if(MenuCursor > optionsMenuLength)
                    MenuCursor = 0;
                if(MenuCursor < 0)
                    MenuCursor = optionsMenuLength;
            }
        }

        // Input Settings
        else if(MenuMode == MENU_INPUT_SETTINGS)
        {
            int ret = menuControls_Logic();
            if(ret == -1)
            {
                SaveConfig();
                MenuCursor = 0; // index of controls within options
                MenuMode = MENU_OPTIONS;
                MenuCursorCanMove = false;
            }
        }
    }

#if 0 // unused now
//'check for all characters blocked
    if(MenuMode == MENU_CHARACTER_SELECT_1P || MenuMode == MENU_CHARACTER_SELECT_2P_S1 || MenuMode == MENU_CHARACTER_SELECT_2P_S2)
    {
        AllCharBlock = 0;

        For(A, 1, numCharacters)
        {
            if(!blockCharacter[A])
            {
                if(AllCharBlock == 0)
                {
                    AllCharBlock = A;
                }
                else
                {
                    AllCharBlock = 0;
                    break;
                }
            }
        }

        if(AllCharBlock > 0)
        {
            PlayerCharacter = AllCharBlock;
            PlayerCharacter2 = AllCharBlock;

            if(MenuMode == MENU_CHARACTER_SELECT_1P)
            {
                MenuMode = MENU_SELECT_SLOT_1P;
                MenuCursor = 0;
            }
            else if(MenuMode == MENU_CHARACTER_SELECT_2P_S1)
            {
                MenuMode = MENU_CHARACTER_SELECT_2P_S2;
                MenuCursor = PlayerCharacter2;
            }
            else
            {
                MenuMode = MENU_SELECT_SLOT_2P;
                MenuCursor = 0;
            }
        }
    }
#endif

    return false;
}

static void s_drawGameTypeTitle(int x, int y)
{
    if(MenuMode == MENU_EDITOR)
        SuperPrint(g_mainMenu.mainEditor, 3, x, y, 0.8f, 0.8f, 0.3f);
    else if(menuBattleMode)
        SuperPrint(g_mainMenu.mainBattleGame, 3, x, y, 0.3f, 0.3f, 1.0f);
    else
    {
        float r = menuPlayersNum == 1 ? 1.f : 0.3f;
        float g = menuPlayersNum == 2 ? 1.f : 0.3f;
        if(menuPlayersNum == 1)
            SuperPrint(g_mainMenu.main1PlayerGame, 3, x, y, r, g, 0.3f);
        else
            SuperPrint(g_mainMenu.mainMultiplayerGame, 3, x, y, r, g, 0.3f);
    }
}

static void s_drawGameSaves(int MenuX, int MenuY)
{
    int A;

    for(A = 1; A <= maxSaveSlots; A++)
    {
        if(SaveSlot[A] >= 0)
        {
            SuperPrint(fmt::format_ne("SLOT {0} ... {1}%", A, SaveSlot[A]), 3, MenuX, MenuY - 30 + (A * 30));
            if(SaveStars[A] > 0)
            {
                XRender::renderTexture(MenuX + 260, MenuY - 30 + (A * 30) + 1,
                                      GFX.Interface[5].w, GFX.Interface[5].h,
                                      GFX.Interface[5], 0, 0);
                XRender::renderTexture(MenuX + 260 + 24, MenuY - 30 + (A * 30) + 2,
                                      GFX.Interface[1].w, GFX.Interface[1].h,
                                      GFX.Interface[1], 0, 0);
                SuperPrint(fmt::format_ne(" {0}", SaveStars[A]), 3, MenuX + 288, MenuY - 30 + (A * 30));
            }
        }
        else
        {
            SuperPrint(fmt::format_ne("SLOT {0} ... NEW GAME", A), 3, MenuX, MenuY - 30 + (A * 30));
        }
    }

    if(MenuMode == MENU_SELECT_SLOT_1P || MenuMode == MENU_SELECT_SLOT_2P)
    {
        SuperPrint("COPY SAVE", 3, MenuX, MenuY - 30 + (A * 30));
        A++;
        SuperPrint("ERASE SAVE", 3, MenuX, MenuY - 30 + (A * 30));
    }
}

void mainMenuDraw()
{
    int MenuX, MenuY;
    GetMenuPos(&MenuX, &MenuY);

    int B = 0;

    // just don't call this during an offset!
    // XRender::offsetViewportIgnore(true);

    if(MenuMode != MENU_1PLAYER_GAME && MenuMode != MENU_2PLAYER_GAME && MenuMode != MENU_BATTLE_MODE && MenuMode != MENU_EDITOR)
        worldCurs = 0;

    // Render the permanent menu graphics (curtain, URL, logo)

    // Curtain
    // correction to loop the original asset properly
    int A = GFX.MenuGFX[1].w;
    if(A == 800)
        A = 768;
    // horizReps
    B = ScreenW / A + 2;

    double x = 0;

    // can add parallax scrolling to curtain by uncommenting following line
    // x = -A + std::fmod(1.25 * vScreenX[1], A);

    for(int C = 0; C < B; C++)
        XRender::renderTexture(x + A * C, 0, A, GFX.MenuGFX[1].h, GFX.MenuGFX[1], 0, 0);

    // can animate curtain falling by replacing y coordinate with -GFX.MenuGFX[1].h * g_levelScreenFader.m_scale

    // URL
    if(ScreenH >= SmallScreenH)
        XRender::renderTexture(ScreenW / 2 - GFX.MenuGFX[3].w / 2, ScreenH - 24,
                GFX.MenuGFX[3].w, GFX.MenuGFX[3].h, GFX.MenuGFX[3], 0, 0);

    // game logo
    int LogoMode = 0;
    if(ScreenH < SmallScreenH &&
        (MenuMode == MENU_INPUT_SETTINGS))
    {
        LogoMode = 2;
    }
    else if(ScreenH >= TinyScreenH || MenuMode == MENU_INTRO)
    {
        LogoMode = 1;
    }
    else if(MenuMode == MENU_MAIN || MenuMode == MENU_OPTIONS)
        LogoMode = 2;

    if(LogoMode == 1)
    {
        if(ScreenH < SmallScreenH)
            XRender::renderTexture(ScreenW / 2 - GFX.MenuGFX[2].w / 2, 30,
                    GFX.MenuGFX[2].w, GFX.MenuGFX[2].h, GFX.MenuGFX[2], 0, 0);
        else if(ScreenH <= 600)
            XRender::renderTexture(ScreenW / 2 - GFX.MenuGFX[2].w / 2, 70,
                    GFX.MenuGFX[2].w, GFX.MenuGFX[2].h, GFX.MenuGFX[2], 0, 0);
        else
            XRender::renderTexture(ScreenW / 2 - GFX.MenuGFX[2].w / 2, ScreenH / 2 - 230,
                    GFX.MenuGFX[2].w, GFX.MenuGFX[2].h, GFX.MenuGFX[2], 0, 0);
    }
    else if(LogoMode == 2)
    {
        SuperPrint(g_gameInfo.title, 3, ScreenW/2 - g_gameInfo.title.length()*9, 30);
    }


    if(SDL_AtomicGet(&loading))
    {
        if(SDL_AtomicGet(&loadingProgrssMax) <= 0)
            SuperPrint(g_mainMenu.loading, 3, MenuX, MenuY);
        else
        {
            int progress = (SDL_AtomicGet(&loadingProgrss) * 100) / SDL_AtomicGet(&loadingProgrssMax);
            SuperPrint(fmt::format_ne("{0} {1}%", g_mainMenu.loading, progress), 3, MenuX, MenuY);
        }
    }
    else

    // Menu Intro
    if(MenuMode == MENU_INTRO)
    {
        BlockFlash += 1;

        if(BlockFlash >= 90)
            BlockFlash = 0;

        if(BlockFlash < 45)
            SuperPrint(g_mainMenu.introPressStart, 3, ScreenW/2 - g_mainMenu.introPressStart.length()*9, ScreenH - 40);
    }
    // Main menu
    else if(MenuMode == MENU_MAIN)
    {
        int i = 0;
        SuperPrint(g_gameInfo.disableTwoPlayer ? g_mainMenu.mainStartGame : g_mainMenu.main1PlayerGame, 3, MenuX, MenuY+30*(i++));
        if(!g_gameInfo.disableTwoPlayer)
            SuperPrint(g_mainMenu.mainMultiplayerGame, 3, MenuX, MenuY+30*(i++));
        if(!g_gameInfo.disableBattleMode)
            SuperPrint(g_mainMenu.mainBattleGame, 3, MenuX, MenuY+30*(i++));
        if(g_config.enable_editor)
            SuperPrint(g_mainMenu.mainEditor, 3, MenuX, MenuY+30*(i++));
        SuperPrint(g_mainMenu.mainOptions, 3, MenuX, MenuY+30*(i++));
        SuperPrint(g_mainMenu.mainExit, 3, MenuX, MenuY+30*(i++));
        XRender::renderTexture(MenuX - 20, MenuY + (MenuCursor * 30), 16, 16, GFX.MCursor[0], 0, 0);
    }

    // Character select
    else if(MenuMode == MENU_CHARACTER_SELECT_NEW ||
            MenuMode == MENU_CHARACTER_SELECT_NEW_BM)
    {
        ConnectScreen::Render();
    }
#if 0 // dead now
    else if(MenuMode == MENU_CHARACTER_SELECT_1P ||
            MenuMode == MENU_CHARACTER_SELECT_2P_S1 ||
            MenuMode == MENU_CHARACTER_SELECT_2P_S2 ||
            MenuMode == MENU_CHARACTER_SELECT_BM_S1 ||
            MenuMode == MENU_CHARACTER_SELECT_BM_S2)
    {
        A = 0;
        B = 0;
        C = 0;

        s_drawGameTypeTitle(MenuX, 280);
        SuperPrint(SelectWorld[selWorld].WorldName, 3, MenuX, 310, 0.6f, 1.f, 1.f);

        // TODO: Make a custom playable character names print here
        if(!blockCharacter[1])
            SuperPrint(g_mainMenu.selectPlayer[1], 3, MenuX, MenuY);
        else
        {
            A -= 30;
            if(MenuCursor + 1 >= 1)
                B -= 30;
            if(PlayerCharacter >= 1)
                C -= 30;
        }

        if(!blockCharacter[2])
            SuperPrint(g_mainMenu.selectPlayer[2], 3, MenuX, 380 + A);
        else
        {
            A -= 30;
            if(MenuCursor + 1 >= 2)
                B -= 30;
            if(PlayerCharacter >= 2)
                C -= 30;
        }

        if(!blockCharacter[3])
            SuperPrint(g_mainMenu.selectPlayer[3], 3, MenuX, 410 + A);
        else
        {
            A -= 30;
            if(MenuCursor + 1 >= 3)
                B -= 30;
            if(PlayerCharacter >= 3)
                C -= 30;
        }

        if(!blockCharacter[4])
            SuperPrint(g_mainMenu.selectPlayer[4], 3, MenuX, 440 + A);
        else
        {
            A -= 30;
            if(MenuCursor + 1 >= 4)
                B -= 30;
            if(PlayerCharacter >= 4)
                C -= 30;
        }

        if(!blockCharacter[5])
            SuperPrint(g_mainMenu.selectPlayer[5], 3, MenuX, 470 + A);
        else
        {
            A -= 30;
            if(MenuCursor + 1 >= 5)
                B -= 30;
            if(PlayerCharacter >= 5)
                C -= 30;
        }

        if(MenuMode == MENU_CHARACTER_SELECT_2P_S2 || MenuMode == MENU_CHARACTER_SELECT_BM_S2)
        {
            XRender::renderTexture(MenuX - 20, B + MenuY + (MenuCursor * 30), GFX.MCursor[3]);
            XRender::renderTexture(MenuX - 20, B + MenuY + ((PlayerCharacter - 1) * 30), GFX.MCursor[0]);
        }
        else
        {
            XRender::renderTexture(MenuX - 20, B + MenuY + (MenuCursor * 30), GFX.MCursor[0]);
        }
    }
#endif

    // Episode / Level selection
    else if(MenuMode == MENU_1PLAYER_GAME || MenuMode == MENU_2PLAYER_GAME || MenuMode == MENU_BATTLE_MODE || MenuMode == MENU_EDITOR)
    {
        s_drawGameTypeTitle(MenuX, MenuY - 70);
        // std::string tempStr;

        minShow = 1;
        maxShow = (MenuMode == MENU_BATTLE_MODE) ? NumSelectBattle :
            ((MenuMode == MENU_EDITOR) ? NumSelectWorldEditable :
                NumSelectWorld);
        const std::vector<SelectWorld_t>& SelectorList
            = (MenuMode == MENU_BATTLE_MODE) ? SelectBattle :
                ((MenuMode == MENU_EDITOR) ? SelectWorldEditable :
                    SelectWorld);

        int original_maxShow = maxShow;
        if(maxShow > 5)
        {
            minShow = worldCurs;
            maxShow = minShow + 4;

            if(MenuCursor <= minShow - 1)
                worldCurs -= 1;

            if(MenuCursor >= maxShow - 1)
                worldCurs += 1;

            if(worldCurs < 1)
                worldCurs = 1;

            if(worldCurs > original_maxShow - 4)
                worldCurs = original_maxShow - 4;

            minShow = worldCurs;
            maxShow = minShow + 4;
        }

        for(auto A = minShow; A <= maxShow; A++)
        {
            auto w = SelectorList[A];
            float r = w.highlight ? 0.f : 1.f;
            float g = 1.0f;
            float b = 1.0f;

            B = A - minShow + 1;

            if(w.disabled)
            {
                r = 0.5f;
                g = 0.5f;
                b = 0.5f;
            }

            SuperPrint(w.WorldName, 3, MenuX, MenuY - 30 + (B * 30), r, g, b, 1.f);
        }

        // render the scroll indicators
        if(minShow > 1)
            XRender::renderTexture(ScreenW/2 - 8, MenuY - 20, GFX.MCursor[1]);

        if(maxShow < original_maxShow)
            XRender::renderTexture(ScreenW/2 - 8, MenuY + 140, GFX.MCursor[2]);

        B = MenuCursor - minShow + 1;

        if(B >= 0 && B < 5)
            XRender::renderTexture(MenuX - 20, MenuY + (B * 30), GFX.MCursor[0].w, GFX.MCursor[0].h, GFX.MCursor[0], 0, 0);
    }

    else if(MenuMode == MENU_SELECT_SLOT_1P || MenuMode == MENU_SELECT_SLOT_2P) // Save Select
    {
        s_drawGameTypeTitle(MenuX, MenuY - 70);
        SuperPrint(SelectWorld[selWorld].WorldName, 3, MenuX, MenuY - 40, 0.6f, 1.f, 1.f);
        s_drawGameSaves(MenuX, MenuY);
        XRender::renderTexture(MenuX - 20, MenuY + (MenuCursor * 30), GFX.MCursor[0]);
    }

    else if(MenuMode == MENU_SELECT_SLOT_1P_COPY_S1 || MenuMode == MENU_SELECT_SLOT_2P_COPY_S1 ||
            MenuMode == MENU_SELECT_SLOT_1P_COPY_S2 || MenuMode == MENU_SELECT_SLOT_2P_COPY_S2) // Copy save
    {
        s_drawGameTypeTitle(MenuX, MenuY - 70);
        SuperPrint(SelectWorld[selWorld].WorldName, 3, MenuX, MenuY - 40, 0.6f, 1.f, 1.f);
        s_drawGameSaves(MenuX, MenuY);

        if(MenuMode == MENU_SELECT_SLOT_1P_COPY_S1 || MenuMode == MENU_SELECT_SLOT_2P_COPY_S1)
            SuperPrint("Select the source slot", 3, MenuX, MenuY - 30 + (5 * 30), 0.7f, 0.7f, 1.0f);
        else if(MenuMode == MENU_SELECT_SLOT_1P_COPY_S2 || MenuMode == MENU_SELECT_SLOT_2P_COPY_S2)
            SuperPrint("Now select the target", 3, MenuX, MenuY - 30 + (5 * 30), 0.7f, 1.0f, 0.7f);

        if(MenuMode == MENU_SELECT_SLOT_1P_COPY_S2 || MenuMode == MENU_SELECT_SLOT_2P_COPY_S2)
        {
            XRender::renderTexture(MenuX - 20, MenuY + ((menuCopySaveSrc - 1) * 30), GFX.MCursor[0]);
            XRender::renderTexture(MenuX - 20, MenuY + (MenuCursor * 30), GFX.MCursor[3]);
        }
        else
            XRender::renderTexture(MenuX - 20, MenuY + (MenuCursor * 30), GFX.MCursor[0]);
    }

    else if(MenuMode == MENU_SELECT_SLOT_1P_DELETE || MenuMode == MENU_SELECT_SLOT_2P_DELETE) // Copy save
    {
        s_drawGameTypeTitle(MenuX, MenuY - 70);
        SuperPrint(SelectWorld[selWorld].WorldName, 3, MenuX, MenuY - 40, 0.6f, 1.f, 1.f);
        s_drawGameSaves(MenuX, MenuY);

        SuperPrint("Select the slot to erase", 3, MenuX, MenuY - 30 + (5 * 30), 1.0f, 0.7f, 0.7f);

        XRender::renderTexture(MenuX - 20, MenuY + (MenuCursor * 30), GFX.MCursor[0]);
    }

    // Options Menu
    else if(MenuMode == MENU_OPTIONS)
    {
        int i = 0;
        SuperPrint(g_mainMenu.controlsTitle, 3, MenuX, MenuY + 30*i++);
#ifndef __ANDROID__
        if(resChanged)
            SuperPrint("WINDOWED MODE", 3, MenuX, MenuY + 30*i++);
        else
            SuperPrint("FULLSCREEN MODE", 3, MenuX, MenuY + 30*i++);
#endif
#if !defined(__3DS__) && !defined(VITA)
        SuperPrint("SCALE: "+ScaleMode_strings.at(g_videoSettings.scaleMode), 3, MenuX, MenuY + 30*i++);
#endif
#ifndef FIXED_RES
        std::string resString = fmt::format_ne("RES: {0}x{1}", g_config.InternalW, g_config.InternalH);
        if (g_config.InternalW == 480 && g_config.InternalH == 320)
            resString += " (GBA)";
        else if (g_config.InternalW == 512 && g_config.InternalH == 384)
            resString += " (NDS)";
        else if (g_config.InternalW == 512 && g_config.InternalH == 448)
            resString += " (SNES)";
        else if (g_config.InternalW == 640 && g_config.InternalH == 480)
            resString += " (VGA)";
        else if (g_config.InternalW == 768 && g_config.InternalH == 432)
            resString += " (HELLO)";
        else if (g_config.InternalW == 800 && g_config.InternalH == 480)
            resString += " (3DS)";
        else if (g_config.InternalW == 800 && g_config.InternalH == 600)
            resString += " (SMBX)";
        else if (g_config.InternalW == 1280 && g_config.InternalH == 720)
            resString += " (HD)";
        else if (g_config.InternalW == 0 && g_config.InternalH == 600)
            resString = "RES: 600P DYNAMIC";
        else if (g_config.InternalW == 0 && g_config.InternalH == 0)
            resString = "RES: DYNAMIC";
        else
            resString += " (CUSTOM)";
        SuperPrint(resString, 3, MenuX, MenuY + 30*i++);
#endif
        SuperPrint("VIEW CREDITS", 3, MenuX, MenuY + 30*i++);
        XRender::renderTexture(MenuX - 20, MenuY + (MenuCursor * 30),
                              GFX.MCursor[0].w, GFX.MCursor[0].h, GFX.MCursor[0], 0, 0);
    }

    // Player controls setup
    else if(MenuMode == MENU_INPUT_SETTINGS)
    {
        menuControls_Render();
    }

    // Mouse cursor
    XRender::renderTexture(int(SharedCursor.X), int(SharedCursor.Y), GFX.ECursor[2]);
}
