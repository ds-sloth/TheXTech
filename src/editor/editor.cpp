/*
 * TheXTech - A platform game engine ported from old source code for VB6
 *
 * Copyright (c) 2009-2011 Andrew Spinks, original VB6 code
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
#include <SDL2/SDL_timer.h>
#endif

#ifndef NO_INTPROC
#include <InterProcess/intproc.h>
#endif

#include <Logger/logger.h>
#include <Utils/elapsed_timer.h>
#include <pge_delay.h>
#include <fmt_format_ne.h>

#include "../globals.h"
#include "editor.h"
#include "../graphics.h"
#include "../sound.h"
#include "../collision.h"
#include "../layers.h"
#include "../npc.h"
#include "../blocks.h"
#include "../sorting.h"
#include "../player.h"
#include "../effect.h"
#include "../game_main.h"
#include "../main/level_file.h"
#include "../load_gfx.h"
#include "../control/joystick.h"
#include "../main/trees.h"

#include "../pseudo_vb.h"

#ifdef NEW_EDITOR
#include "write_level.h"
#include "write_world.h"
#include "new_editor.h"
#endif

static int ScrollDelay = 0; // slows down the camera movement when scrolling through a level
//Public Declare Function GetCursorPos Lib "user32" (lpPoint As POINTAPI) As long long;

std::string Backup_FullFileName;

Point_t CursorPos;

bool HasCursor = false;
bool NoReallyKillIt = false;
int curSection = 0;

OptCursor_t optCursor;

bool enableAutoAlign = true;

int last_vScreenX[maxSections+1];
int last_vScreenY[maxSections+1];

int last_vScreenX_b[maxSections+1];
int last_vScreenY_b[maxSections+1];

int curSection_b;

void ResetSectionScrolls()
{
    for (int i = 0; i <= maxSections; i++)
    {
        // initialize the section
        if (level[i].Height == level[i].Y)
        {
            level[i].Height = (-200000 + 20000 * i);
            level[i].Y = level[i].Height - 600;
            level[i].X = (-200000 + 20000 * i);
            level[i].Width = level[i].X + 800;
        }

        last_vScreenY[i] = -(level[i].Height - 600);
        last_vScreenX[i] = -(level[i].X);
    }
    curSection = 0;
    vScreenY[1] = last_vScreenY[curSection];
    vScreenX[1] = last_vScreenX[curSection];
}

void SetSection(int i)
{
    if(curMusic != bgMusic[i])
        StartMusic(i);
    else if(bgMusic[i] == 24)
    {
        if(curSection >= 0)
            if(CustomMusic[curSection] != CustomMusic[i])
                StartMusic(i);
    }

    last_vScreenY[curSection] = vScreenY[1];
    last_vScreenX[curSection] = vScreenX[1];
    curSection = i;
    vScreenY[1] = last_vScreenY[curSection];
    vScreenX[1] = last_vScreenX[curSection];
}

void EditorBackup()
{
    last_vScreenY[curSection] = vScreenY[1];
    last_vScreenX[curSection] = vScreenX[1];
    for(int i = 0; i <= maxSections; i++)
    {
        last_vScreenX_b[i] = last_vScreenX[i];
        last_vScreenY_b[i] = last_vScreenY[i];
    }
    curSection_b = curSection;
}

void EditorRestore()
{
    for(int i = 0; i <= maxSections; i++)
    {
        last_vScreenX[i] = last_vScreenX_b[i];
        last_vScreenY[i] = last_vScreenY_b[i];
    }
    curSection = curSection_b;
    vScreenX[1] = last_vScreenX_b[curSection];
    vScreenY[1] = last_vScreenY_b[curSection];
    SetSection(curSection);
    vScreenX[1] = last_vScreenX_b[curSection];
    vScreenY[1] = last_vScreenY_b[curSection];
}

// this sub handles the level editor
// it is still called when the player is testing a level in the editor in windowed mode
void UpdateEditor()
{
    int A = 0;
    int B = 0;
//    int C = 0;
    int qLevel = 0;
    bool CanPlace = false; // Determines if something is in the way
    bool tempBool = false;
//    bool grabBool = false;
    Location_t tempLocation;
//    if(Debugger == true)
//        frmLevelDebugger::UpdateDisplay;
    GameMenu = false;

    GetEditorControls();
    if(!EditorControls.MouseClick && !EditorControls.SwitchScreens)
        MouseRelease = true;

    if(LevelEditor == true)
        numPlayers = 0;

    if(MagicHand)
    {
        MouseMove(EditorCursor.X, EditorCursor.Y, true);
//        frmNPCs::chkMessage.Enabled = false;
    }
    else
    {
//        frmNPCs::chkMessage.Enabled = true;
        FreezeNPCs = false;
        LevelMacro = LEVELMACRO_OFF;
        LevelMacroCounter = 0;
    }

    if(!MagicHand && ScreenType != 0)
    {
        ScreenType = 0;
        SetupScreens();
    }

    if(!MagicHand)
    {
        if(EditorControls.PrevSection && !WorldEditor)
        {
            if(ScrollRelease == true)
            {
                ScrollRelease = false;
//                frmLevelSettings::optSection(curSection).Value = false;
                if(curSection != 0)
                    SetSection(curSection - 1);
//                if(EditorCursor.Mode == 2)
//                    frmLevelSettings::optSection(curSection).Value = true;
            }
        }
        else if(EditorControls.NextSection && !WorldEditor)
        {
            if(ScrollRelease == true)
            {
                ScrollRelease = false;
//                frmLevelSettings::optSection(curSection).Value = false;
                if(curSection != maxSections)
                    SetSection(curSection + 1);
//                if(EditorCursor.Mode == 2)
//                    frmLevelSettings::optSection(curSection).Value = true;
            }
        }
        else
            ScrollRelease = true;

        if(std::fmod((vScreenY[1] + 8), 32) != 0.0)
            vScreenY[1] = static_cast<int>(floor(static_cast<double>(vScreenY[1] / 32))) * 32 - 8;
        if(std::fmod(vScreenX[1], 32) != 0.0)
            vScreenX[1] = static_cast<int>(floor(static_cast<double>(vScreenX[1] / 32))) * 32;
    }
    else
    {
        curSection = Player[1].Section;
    }

    if(LevelEditor || MagicHand)
    {
        if(EditorControls.FastScroll)
            ScrollDelay = 0;

        if(MagicHand)
            ScrollDelay = 10;

        if(ScrollDelay <= 0)
        {
            if(EditorControls.ScrollUp)
            {
                vScreenY[1] = vScreenY[1] + 32;
                EditorCursor.Location.Y -= 32;
                ScrollDelay = 2;
                MouseRelease = true;
            }

            if(EditorControls.ScrollDown)
            {
                vScreenY[1] = vScreenY[1] - 32;
                EditorCursor.Location.Y = EditorCursor.Location.Y + 32;
                ScrollDelay = 2;
                MouseRelease = true;
            }

            if(EditorControls.ScrollLeft)
            {
                vScreenX[1] = vScreenX[1] + 32;
                EditorCursor.Location.X = EditorCursor.Location.X - 32;
                ScrollDelay = 2;
                MouseRelease = true;
            }

            if(EditorControls.ScrollRight)
            {
                vScreenX[1] = vScreenX[1] - 32;
                EditorCursor.Location.X = EditorCursor.Location.X + 32;
                ScrollDelay = 2;
                MouseRelease = true;
            }
        }
        else
            ScrollDelay = ScrollDelay - 1;
        SetCursor();

        // this is where objects are placed/grabbed/deleted

#ifndef NO_INTPROC
        if(IntProc::isEnabled())
            UpdateInterprocess();
#endif

#ifdef NEW_EDITOR
        if(EditorControls.MouseClick && !editorScreen.active && EditorCursor.Y > 40)
#else
        if(EditorControls.MouseClick)
#endif
        {
            CanPlace = true;
            if(EditorCursor.Mode == OptCursor_t::LVL_SELECT || EditorCursor.Mode == 14)
            {
                if(MouseRelease && !MagicHand) // Player start points
                {
                    for(A = 1; A <= 2; A++)
                    {
                        if(CursorCollision(EditorCursor.Location, PlayerStart[A]))
                        {
                            PlaySound(SFX_Grab);
                            EditorCursor.Location = PlayerStart[A];
                            PlayerStart[A].X = 0;
                            PlayerStart[A].Y = 0;
                            optCursor.current = 2;
                            EditorCursor.Mode = 2;
                            EditorCursor.SubMode = 3 + A;
                            MouseMove(EditorCursor.X, EditorCursor.Y);
                            MouseRelease = false;
                            EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
//                            Netplay::sendData "v" + std::to_string(A) + "|" + PlayerStart[A].X + "|" + PlayerStart[A].Y + "|" + std::to_string(PlayerStart[A].Width) + "|" + std::to_string(PlayerStart[A].Height) + LB;
                        }
                    }
                }

                if(MouseRelease) // NPCs
                {
                    for(A = 1; A <= numNPCs; A++)
                    {
                        tempLocation = NPC[A].Location;

                        if(NPC[A].Type == 91) // Herb's container offset
                            tempLocation.Y -= 16;

                        if(CursorCollision(EditorCursor.Location, tempLocation) && !NPC[A].Hidden)
                        {
                            PlaySound(SFX_Grab);
                            B = 0;
//                            frmLevelEditor::optCursor(4).Value = true;
                            optCursor.current = OptCursor_t::LVL_NPCS;
                            OptCursorSync();

                            EditorCursor.Mode = OptCursor_t::LVL_NPCS;
                            ResetNPC(A);
                            EditorCursor.NPC = NPC[A];
                            EditorCursor.NPC.Hidden = false;
                            EditorCursor.Layer = NPC[A].Layer;
                            EditorCursor.Location = NPC[A].Location;
                            EditorCursor.Location.X = NPC[A].Location.X;
                            EditorCursor.Location.Y = NPC[A].Location.Y;
                            printf("Got %d\n", EditorCursor.NPC.Type);
                            SetCursor();
//                            Netplay::sendData Netplay::EraseNPC(A, 1) + "p23" + LB;
                            KillNPC(A, 9);
#ifdef NEW_EDITOR
                            editorScreen.FocusNPC();
#endif
                            MouseRelease = false;
                            EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
                            tempBool = true;
                            UNUSED(tempBool);
#ifndef NO_INTPROC
                            if(IntProc::isEnabled()) // Report the taken block into the Editor
                            {
                                LevelNPC n;
                                n.id = EditorCursor.NPC.Type;
                                n.direct = EditorCursor.NPC.Direction;

                                n.generator = EditorCursor.NPC.Generator;
                                if(n.generator)
                                {
                                    n.generator_direct = EditorCursor.NPC.GeneratorDirection;
                                    n.generator_type = EditorCursor.NPC.GeneratorEffect;
                                    n.generator_period = EditorCursor.NPC.GeneratorTimeMax;
                                }

                                if(n.id == 91 || n.id == 96 || n.id == 283 || n.id == 284)
                                    n.contents = EditorCursor.NPC.Special;

                                if(n.id == 288 || n.id == 289 || (n.id == 91 && int(EditorCursor.NPC.Special) == 288))
                                    n.special_data = EditorCursor.NPC.Special2;

                                if(NPCIsAParaTroopa[n.id] || NPCIsCheep[n.id] || n.id == 260)
                                    n.special_data = EditorCursor.NPC.Special;

                                n.msg = EditorCursor.NPC.Text;
                                n.nomove = EditorCursor.NPC.Stuck;
                                n.is_boss = EditorCursor.NPC.Legacy;

                                n.layer = EditorCursor.NPC.Layer;
                                n.event_activate = EditorCursor.NPC.TriggerActivate;
                                n.event_die = EditorCursor.NPC.TriggerDeath;
                                n.event_emptylayer = EditorCursor.NPC.TriggerLast;
                                n.event_talk = EditorCursor.NPC.TriggerTalk;
                                n.attach_layer = EditorCursor.NPC.AttLayer;
                                IntProc::sendTakenNPC(n);
                            }
#endif
                            break;
                        }
                    }
                }

                if(MouseRelease) // non-sizable blocks
                {
                    for(A = 1; A <= numBlock; A++)
                    {
                        if(!BlockIsSizable[Block[A].Type])
                        {
                            if(CursorCollision(EditorCursor.Location, Block[A].Location) && !Block[A].Hidden)
                            {
                                PlaySound(SFX_Grab);
//                                frmLevelEditor::optCursor(1).Value = true;
                                optCursor.current = OptCursor_t::LVL_BLOCKS;
                                OptCursorSync();

                                EditorCursor.Mode = OptCursor_t::LVL_BLOCKS;
                                EditorCursor.Block = Block[A];
                                EditorCursor.Layer = Block[A].Layer;
                                EditorCursor.Location.X = Block[A].Location.X;
                                EditorCursor.Location.Y = Block[A].Location.Y;
                                EditorCursor.Location.Width = Block[A].Location.Width;
                                EditorCursor.Location.Height = Block[A].Location.Height;
                                SetCursor();
                                KillBlock(A, false);
#ifdef NEW_EDITOR
                                editorScreen.FocusBlock();
#endif
                                MouseRelease = false;
                                EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
                                FindSBlocks();
#ifndef NO_INTPROC
                                if(IntProc::isEnabled()) // Report the taken block into the Editor
                                {
                                    LevelBlock block;
                                    block.id = EditorCursor.Block.Type;
                                    block.w = EditorCursor.Location.Width;
                                    block.h = EditorCursor.Location.Height;
                                    block.invisible = EditorCursor.Block.Invis;
                                    block.slippery = EditorCursor.Block.Slippy;
                                    block.layer = EditorCursor.Block.Layer;
                                    if(EditorCursor.Block.Special >= 1000)
                                        block.npc_id = EditorCursor.Block.Special - 1000;
                                    else if(EditorCursor.Block.Special <= 0)
                                        block.npc_id = 0;
                                    else if(EditorCursor.Block.Special < 1000)
                                        block.npc_id = -EditorCursor.Block.Special;
                                    block.event_hit = EditorCursor.Block.TriggerHit;
                                    block.event_emptylayer = EditorCursor.Block.TriggerLast;
                                    block.event_destroy = EditorCursor.Block.TriggerDeath;
                                    IntProc::sendTakenBlock(block);
                                }
#endif
                                break;
                            }
                        }
                    }
                }

                if(MouseRelease && !MagicHand) // Warps
                {
                    for(A = 1; A <= numWarps; A++)
                    {
                        if(CursorCollision(EditorCursor.Location, Warp[A].Entrance) && Warp[A].Hidden == false)
                        {
                            PlaySound(SFX_Grab);
                            Warp[A].PlacedEnt = false;
                            optCursor.current = OptCursor_t::LVL_WARPS;
                            OptCursorSync();
                            MouseRelease = false;
                            EditorCursor.Mode = OptCursor_t::LVL_WARPS;
                            EditorCursor.SubMode = 1;
                            EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
                            if(Warp[A].LevelEnt || EditorCursor.Warp.MapWarp || EditorCursor.Warp.level != "")
                                Warp[A].PlacedExit = false;
                            EditorCursor.Warp = Warp[A];
                            if (!Warp[A].PlacedEnt && !Warp[A].PlacedExit)
                                KillWarp(A);
                            break;
                        }
                        else if(CursorCollision(EditorCursor.Location, Warp[A].Exit))
                        {
                            PlaySound(SFX_Grab);
                            Warp[A].PlacedExit = false;
                            optCursor.current = OptCursor_t::LVL_WARPS;
                            OptCursorSync();
                            MouseRelease = false;
                            EditorCursor.Mode = OptCursor_t::LVL_WARPS;
                            EditorCursor.SubMode = 2;
                            EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
                            // TODO: additional testing of these situations
                            if(Warp[A].LevelEnt || EditorCursor.Warp.MapWarp || EditorCursor.Warp.level != "")
                                Warp[A].PlacedEnt = false;
                            EditorCursor.Warp = Warp[A];
                            if (!Warp[A].PlacedEnt && !Warp[A].PlacedExit)
                                KillWarp(A);
                            break;
                        }
                    }
                }

                if(MouseRelease) // BGOs
                {
                    for(A = numBackground; A >= 1; A--)
                    {
                        if(CursorCollision(EditorCursor.Location, Background[A].Location) == true && Background[A].Hidden == false)
                        {
                            PlaySound(SFX_Grab);
                            optCursor.current = 3;
                            OptCursorSync();
                            EditorCursor.Layer = Background[A].Layer;
                            EditorCursor.Location.X = Background[A].Location.X;
                            EditorCursor.Location.Y = Background[A].Location.Y;
                            EditorCursor.Background = std::move(Background[A]);
                            SetCursor();
                            Background[A] = std::move(Background[numBackground]);
                            numBackground = numBackground - 1;
#ifdef NEW_EDITOR
                            editorScreen.FocusBGO();
#endif
                            if(MagicHand)
                            {
                                qSortBackgrounds(1, numBackground);
                                UpdateBackgrounds();
                                syncLayers_AllBGOs();
                            }
                            else
                            {
                                syncLayers_BGO(A);
                                syncLayers_BGO(numBackground+1);
                            }
                            MouseRelease = false;
                            EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
#ifndef NO_INTPROC
                            if(IntProc::isEnabled()) // Report the taken block into the Editor
                            {
                                LevelBGO b;
                                b.id = EditorCursor.Background.Type;
                                b.layer = EditorCursor.Background.Layer;
                                b.z_mode = EditorCursor.Background.zMode;
                                b.z_offset = EditorCursor.Background.zOffset;
                                if(EditorCursor.Background.zMode == LevelBGO::ZDefault)
                                    b.smbx64_sp = EditorCursor.Background.SortPriority;
                                IntProc::sendTakenBGO(b);
                            }
#endif
                            break;
                        }
                    }
                }

                if(MouseRelease) // Sizable blocks
                {
                    for(A = 1; A <= numBlock; A++)
                    {
                        if(BlockIsSizable[Block[A].Type])
                        {
                            if(CursorCollision(EditorCursor.Location, Block[A].Location) && !Block[A].Hidden)
                            {
                                PlaySound(SFX_Grab);
//                                frmLevelEditor::optCursor(1).Value = true;
                                optCursor.current = 1;
                                OptCursorSync();

                                EditorCursor.Mode = OptCursor_t::LVL_BLOCKS;
                                EditorCursor.Layer = Block[A].Layer;
                                EditorCursor.Location.X = Block[A].Location.X;
                                EditorCursor.Location.Y = Block[A].Location.Y;
                                EditorCursor.Location.Width = Block[A].Location.Width;
                                EditorCursor.Location.Height = Block[A].Location.Height;
                                EditorCursor.Block = std::move(Block[A]);
                                SetCursor();
//                                Netplay::sendData Netplay::EraseBlock(A, 1);
                                KillBlock(A, false);
#ifdef NEW_EDITOR
                                editorScreen.FocusBlock();
#endif
                                MouseRelease = false;
                                EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
                                FindSBlocks();
#ifndef NO_INTPROC
                                if(IntProc::isEnabled()) // Report the taken block into the Editor
                                {
                                    LevelBlock block;
                                    block.id = EditorCursor.Block.Type;
                                    block.w = EditorCursor.Location.Width;
                                    block.h = EditorCursor.Location.Height;
                                    block.invisible = EditorCursor.Block.Invis;
                                    block.slippery = EditorCursor.Block.Slippy;
                                    block.layer = EditorCursor.Block.Layer;
                                    if(EditorCursor.Block.Special >= 1000)
                                        block.npc_id = EditorCursor.Block.Special - 1000;
                                    else if(EditorCursor.Block.Special <= 0)
                                        block.npc_id = 0;
                                    else if(EditorCursor.Block.Special < 1000)
                                        block.npc_id = -EditorCursor.Block.Special;
                                    block.event_hit = EditorCursor.Block.TriggerHit;
                                    block.event_emptylayer = EditorCursor.Block.TriggerLast;
                                    block.event_destroy = EditorCursor.Block.TriggerDeath;
                                    IntProc::sendTakenBlock(block);
                                }
#endif
                                break;
                            }
                        }
                    }
                }

                if(MouseRelease && !MagicHand) // Water boxes
                {
                    for(int numWaterMax = numWater, A = 1; A <= numWaterMax; A++)
                    {
                        if(CursorCollision(EditorCursor.Location, Water[A].Location) == true)
                        {
                            PlaySound(SFX_Grab);
//                            frmLevelEditor::optCursor(15).Value = true;
                            optCursor.current = OptCursor_t::LVL_WATER;
                            OptCursorSync();
                            EditorCursor.Location = Water[A].Location;
                            EditorCursor.Layer = Water[A].Layer;
                            EditorCursor.Water = std::move(Water[A]);
                            Water[A] = std::move(Water[numWater]);
                            numWater--;
                            syncLayers_Water(A);
                            syncLayers_Water(numWater+1);
                            MouseRelease = false;
                            EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
                            break;
                        }
                    }
                }

                if(MouseRelease && !MagicHand) // World map music
                {
                    for(auto *t : treeWorldMusicQuery(EditorCursor.Location, false))
                    {
                        A = (t - &WorldMusic[1]) + 1;
                        if(CursorCollision(EditorCursor.Location, WorldMusic[A].Location) == true)
                        {
                            PlaySound(SFX_Grab);
                            optCursor.current = 11;
                            OptCursorSync();
                            EditorCursor.Mode = 11;
                            EditorCursor.Location = WorldMusic[A].Location;
                            SetCursor();
//                            frmMusic::optMusic(WorldMusic[A].Type).Value = true;
                            EditorCursor.WorldMusic = std::move(WorldMusic[A]);
                            if(A != numWorldMusic)
                            {
                                WorldMusic[A] = std::move(WorldMusic[numWorldMusic]);
                                treeWorldMusicUpdate(&WorldMusic[A]);
                            }
                            treeWorldMusicRemove(&WorldMusic[numWorldMusic]);
                            numWorldMusic = numWorldMusic - 1;
                            MouseRelease = false;
                            EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
                            break;
                        }
                    }
                }

                if(MouseRelease && !MagicHand) // World paths
                {
                    for(auto *t : treeWorldPathQuery(EditorCursor.Location, false))
                    {
                        A = (t - &WorldPath[1]) + 1;
                        if(CursorCollision(EditorCursor.Location, WorldPath[A].Location) == true)
                        {
                            PlaySound(SFX_Grab);
//                            frmLevelEditor::optCursor(10).Value = true;
                            optCursor.current = 10;
                            OptCursorSync();
//                            frmPaths::WorldPath(WorldPath[A].Type).Value = true;
                            EditorCursor.Mode = OptCursor_t::WLD_PATHS;
                            EditorCursor.Location = WorldPath[A].Location;
                            EditorCursor.WorldPath = std::move(WorldPath[A]);
                            SetCursor();
                            if(A != numWorldPaths)
                            {
                                WorldPath[A] = std::move(WorldPath[numWorldPaths]);
                                treeWorldPathUpdate(&WorldPath[A]);
                            }
                            treeWorldPathRemove(&WorldPath[numWorldPaths]);
                            numWorldPaths = numWorldPaths - 1;
                            MouseRelease = false;
                            EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
                            break;
                        }
                    }
                }

                if(MouseRelease && !MagicHand) // World sceneries
                {
                    auto sentinel = treeWorldSceneQuery(EditorCursor.Location, true);
                    auto i = sentinel.end();
                    --i;
                    for(; i >= sentinel.begin(); i--)
                    {
                        A = (*i - &Scene[1]) + 1;
                        if(CursorCollision(EditorCursor.Location, Scene[A].Location) == true)
                        {
                            PlaySound(SFX_Grab);
//                            frmLevelEditor::optCursor(8).Value = true;
                            optCursor.current = 8;
                            OptCursorSync();
//                            frmScene::Scene(Scene[A].Type).Value = true;
                            EditorCursor.Mode = OptCursor_t::WLD_SCENES;
                            EditorCursor.Location = Scene[A].Location;
                            EditorCursor.Scene = std::move(Scene[A]);
                            SetCursor();
                            MouseMove(EditorCursor.X, EditorCursor.Y);
                            // this maintains the order of the scenes
                            // but makes for a hellish quadtree update
                            for(B = A; B < numScenes; B++)
                            {
                                Scene[B] = std::move(Scene[B + 1]);
                                treeWorldSceneUpdate(&Scene[B]);
                            }
                            treeWorldSceneRemove(&Scene[numScenes]);
                            numScenes = numScenes - 1;
                            MouseRelease = false;
                            EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
                            break;
                        }
                    }
                }

                if(MouseRelease && !MagicHand) // World map level points
                {
                    for(auto *t : treeWorldLevelQuery(EditorCursor.Location, false))
                    {
                        A = (t - &WorldLevel[1]) + 1;
                        if(CursorCollision(EditorCursor.Location, WorldLevel[A].Location) == true)
                        {
                            PlaySound(SFX_Grab);
                            optCursor.current = OptCursor_t::WLD_LEVELS;
                            OptCursorSync();
                            EditorCursor.Mode = OptCursor_t::WLD_LEVELS;
                            EditorCursor.Location = WorldLevel[A].Location;
                            EditorCursor.WorldLevel = std::move(WorldLevel[A]);
                            SetCursor();
                            if(A != numWorldLevels)
                            {
                                WorldLevel[A] = std::move(WorldLevel[numWorldLevels]);
                                treeWorldLevelUpdate(&WorldLevel[A]);
                            }
                            treeWorldLevelRemove(&WorldLevel[numWorldLevels]);
                            numWorldLevels = numWorldLevels - 1;
                            MouseRelease = false;
                            EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
                            break;
                        }
                    }
                }

                if(MouseRelease && !MagicHand) // Terrain tiles
                {
                    for(auto *t : treeWorldTileQuery(EditorCursor.Location, false))
                    {
                        A = (t - &Tile[1]) + 1;
                        if(CursorCollision(EditorCursor.Location, Tile[A].Location) == true)
                        {
                            PlaySound(SFX_Grab);
                            optCursor.current = OptCursor_t::WLD_TILES;
                            OptCursorSync();
                            EditorCursor.Mode = OptCursor_t::WLD_TILES;
                            EditorCursor.Location = Tile[A].Location;
                            EditorCursor.Tile = std::move(Tile[A]);
                            SetCursor();
                            if(A != numTiles)
                            {
                                Tile[A] = std::move(Tile[numTiles]);
                                treeWorldTileUpdate(&Tile[A]);
                            }
                            treeWorldTileRemove(&Tile[numTiles]);
                            numTiles = numTiles - 1;
#ifdef NEW_EDITOR
                            editorScreen.FocusTile();
#endif
                            MouseRelease = false;
                            EditorControls.MouseClick = false; /* Simulate "Focus out" inside of SMBX Editor */
                            break;
                        }
                    }
                }
            }
            else if(EditorCursor.Mode == OptCursor_t::LVL_WATER) // Water
            {
                if(MouseRelease)
                {
                    MouseRelease = false;
                    CanPlace = true;
                    for(A = 1; A <= numWater; A++)
                    {
                        if(fEqual(Water[A].Location.X, EditorCursor.Location.X) &&
                           fEqual(Water[A].Location.Y, EditorCursor.Location.Y) &&
                           fEqual(Water[A].Location.Height, EditorCursor.Location.Height) &&
                           fEqual(Water[A].Location.Width, EditorCursor.Location.Width))
                        {
                            CanPlace = false;
                            break;
                        }
                    }

                    if(CanPlace)
                    {
                        numWater++;
                        Water[numWater] = EditorCursor.Water;
                        syncLayers_Water(numWater);
//                        if(nPlay.Online == true)
//                            Netplay::sendData Netplay::AddWater(numWater);
                    }
                }
            }
            else if(EditorCursor.Mode == OptCursor_t::LVL_ERASER || EditorCursor.Mode == OptCursor_t::LVL_ERASER0) // Eraser
            {
                if(MouseRelease)
                {
                    for(A = 1; A <= numNPCs; A++)
                    {
                        tempLocation = NPC[A].Location;
                        if(NPC[A].Type == 91)
                            tempLocation.Y = tempLocation.Y - 16;
                        if(CursorCollision(EditorCursor.Location, tempLocation) == true && NPC[A].Hidden == false)
                        {
                            if(static_cast<int>(floor(static_cast<double>(std::rand() % 2))) == 0)
                                NPC[A].Location.SpeedX = double(Physics.NPCShellSpeed / 2);
                            else
                                NPC[A].Location.SpeedX = -double(Physics.NPCShellSpeed / 2);
//                            Netplay::sendData Netplay::EraseNPC(A, 0);
                            if(NPCIsABonus[NPC[A].Type] || NPCIsACoin[NPC[A].Type])
                                KillNPC(A, 4); // Kill the bonus/coin
                            else
                                KillNPC(A, 2); // Kill the NPC
                            MouseRelease = false;
                            break;
                        }
                    }
                }

                if(MouseRelease)
                {
                    for(A = 1; A <= numBlock; A++)
                    {
                        if(!BlockIsSizable[Block[A].Type])
                        {
                            if(CursorCollision(EditorCursor.Location, Block[A].Location) && !Block[A].Hidden)
                            {
//                                Netplay::sendData Netplay::EraseBlock[A];
                                KillBlock(A); // Erase the block
                                FindSBlocks();
                                MouseRelease = false;
                                break;
                            }
                        }
                    }
                }

                if(MouseRelease && !MagicHand)
                {
                    for(A = 1; A <= numWarps; A++)
                    {
                        tempLocation = Warp[A].Entrance;
                        tempLocation.Height = 32;
                        tempLocation.Width = 32;
                        if(CursorCollision(EditorCursor.Location, tempLocation))
                        {
                            KillWarp(A);
//                            if(nPlay.Online == true)
//                                Netplay::sendData "B" + std::to_string(A) + LB;
                            MouseRelease = false;
                            break;
                        }
                        tempLocation = Warp[A].Exit;
                        tempLocation.Height = 32;
                        tempLocation.Width = 32;
                        if(CursorCollision(EditorCursor.Location, tempLocation))
                        {
                            KillWarp(A);
//                            if(nPlay.Online == true)
//                                Netplay::sendData "B" + std::to_string(A) + LB;
                            MouseRelease = false;
                            break;
                        }
                    }
                }
                if(MouseRelease)
                {
                    for(A = numBackground; A >= 1; A--)
                    {
                        if(CursorCollision(EditorCursor.Location, Background[A].Location) && !Background[A].Hidden)
                        {
//                            Netplay::sendData Netplay::EraseBackground(A, 0);
                            auto &b = Background[A];
                            b.Location.X = b.Location.X + b.Location.Width / 2.0 - EffectWidth[10] / 2;
                            b.Location.Y = b.Location.Y + b.Location.Height / 2.0 - EffectHeight[10] / 2;
                            NewEffect(10, b.Location);
                            PlaySound(SFX_Smash);
                            Background[A] = std::move(Background[numBackground]);
                            numBackground -= 1;
                            MouseRelease = false;
                            if(MagicHand)
                            {
                                qSortBackgrounds(1, numBackground);
                                UpdateBackgrounds();
                                syncLayers_AllBGOs();
                                syncLayers_BGO(numBackground + 1);
                            }
                            else
                            {
                                syncLayers_BGO(A);
                                syncLayers_BGO(numBackground + 1);
                            }
                            break;
                        }
                    }
                }

                if(MouseRelease)
                {
                    for(A = numBlock; A >= 1; A--)
                    {
                        if(BlockIsSizable[Block[A].Type])
                        {
                            if(CursorCollision(EditorCursor.Location, Block[A].Location) && !Block[A].Hidden)
                            {
//                                Netplay::sendData Netplay::EraseBlock[A];
                                KillBlock(A); // Erase the block
                                FindSBlocks();
                                MouseRelease = false;
                                break;
                            }
                        }
                    }
                }

                if(MouseRelease && LevelEditor)
                {
                    for(int numWaterMax = numWater, A = 1; A <= numWaterMax; A++)
                    {
                        tempLocation = Water[A].Location;
                        if(CursorCollision(EditorCursor.Location, tempLocation) == true && Water[A].Hidden == false)
                        {
                            PlaySound(SFX_Smash);
//                            if(nPlay.Online == true)
//                                Netplay::sendData "y" + std::to_string(A) + LB + "p36" + LB;
                            Water[A] = std::move(Water[numWater]);
                            numWater = numWater - 1;
                            syncLayers_Water(A);
                            syncLayers_Water(numWater + 1);
                            MouseRelease = false;
                            break;
                        }
                    }
                }

                if(MouseRelease)
                {
                    // for(int numWorldMusicMax = numWorldMusic, A = 1; A <= numWorldMusicMax; A++)
                    for(auto *t : treeWorldMusicQuery(EditorCursor.Location, false))
                    {
                        A = (t - &WorldMusic[1]) + 1;
                        if(CursorCollision(EditorCursor.Location, WorldMusic[A].Location) == true)
                        {
                            tempLocation = WorldMusic[A].Location;
                            tempLocation.X = tempLocation.X + tempLocation.Width / 2.0 - EffectWidth[10] / 2;
                            tempLocation.Y = tempLocation.Y + tempLocation.Height / 2.0 - EffectHeight[10] / 2;
                            NewEffect(10, tempLocation);
                            PlaySound(SFX_ShellHit);
                            if(A != numWorldMusic)
                            {
                                WorldMusic[A] = std::move(WorldMusic[numWorldMusic]);
                                treeWorldMusicUpdate(&WorldMusic[A]);
                            }
                            treeWorldMusicRemove(&WorldMusic[numWorldMusic]);
                            numWorldMusic = numWorldMusic - 1;
                            MouseRelease = false;
                            break;
                        }
                    }
                }

                if(MouseRelease)
                {
                    for(auto *t : treeWorldPathQuery(EditorCursor.Location, false))
                    {
                        A = (t - &WorldPath[1]) + 1;

                        if(CursorCollision(EditorCursor.Location, WorldPath[A].Location) == true)
                        {
                            tempLocation = WorldPath[A].Location;
                            tempLocation.X = tempLocation.X + tempLocation.Width / 2.0 - EffectWidth[10] / 2;
                            tempLocation.Y = tempLocation.Y + tempLocation.Height / 2.0 - EffectHeight[10] / 2;
                            NewEffect(10, tempLocation);
                            PlaySound(SFX_ShellHit);
                            if(A != numWorldPaths)
                            {
                                WorldPath[A] = std::move(WorldPath[numWorldPaths]);
                                treeWorldPathUpdate(&WorldPath[A]);
                            }
                            treeWorldPathRemove(&WorldPath[numWorldPaths]);
                            numWorldPaths -= 1;
                            MouseRelease = false;
                            break;
                        }
                    }
                }

                if(MouseRelease)
                {
                    auto sentinel = treeWorldSceneQuery(EditorCursor.Location, true);
                    auto i = sentinel.end();
                    --i;
                    for(; i >= sentinel.begin(); i--)
                    {
                        A = (*i - &Scene[1]) + 1;
                        if(CursorCollision(EditorCursor.Location, Scene[A].Location) == true)
                        {
                            tempLocation = Scene[A].Location;
                            tempLocation.X = tempLocation.X + tempLocation.Width / 2.0 - EffectWidth[10] / 2;
                            tempLocation.Y = tempLocation.Y + tempLocation.Height / 2.0 - EffectHeight[10] / 2;
                            NewEffect(10, tempLocation);
                            PlaySound(SFX_ShellHit);
                            for(B = A; B < numScenes; B++)
                            {
                                Scene[B] = std::move(Scene[B + 1]);
                                treeWorldSceneUpdate(&Scene[B]);
                            }
                            treeWorldSceneRemove(&Scene[numScenes]);
                            numScenes = numScenes - 1;
                            MouseRelease = false;
                            break;
                        }
                    }
                }

                if(MouseRelease)
                {
                    for(auto *t : treeWorldLevelQuery(EditorCursor.Location, false))
                    {
                        A = (t - &WorldLevel[1]) + 1;
                        if(CursorCollision(EditorCursor.Location, WorldLevel[A].Location) == true)
                        {
                            tempLocation = WorldLevel[A].Location;
                            tempLocation.X = tempLocation.X + tempLocation.Width / 2.0 - EffectWidth[10] / 2;
                            tempLocation.Y = tempLocation.Y + tempLocation.Height / 2.0 - EffectHeight[10] / 2;
                            NewEffect(10, tempLocation);
                            PlaySound(SFX_ShellHit);
                            if(A != numWorldLevels)
                            {
                                WorldLevel[A] = std::move(WorldLevel[numWorldLevels]);
                                treeWorldLevelUpdate(&WorldLevel[A]);
                            }
                            treeWorldLevelRemove(&WorldLevel[numWorldLevels]);
                            numWorldLevels = numWorldLevels - 1;
                            MouseRelease = false;
                            break;
                        }
                    }
                }

                if(MouseRelease)
                {
                    for(auto *t : treeWorldTileQuery(EditorCursor.Location, false))
                    {
                        A = (t - &Tile[1]) + 1;
                        if(CursorCollision(EditorCursor.Location, Tile[A].Location) == true)
                        {
                            tempLocation = Tile[A].Location;
                            tempLocation.X = tempLocation.X + tempLocation.Width / 2.0 - EffectWidth[10] / 2;
                            tempLocation.Y = tempLocation.Y + tempLocation.Height / 2.0 - EffectHeight[10] / 2;
                            NewEffect(10, tempLocation);
                            PlaySound(SFX_ShellHit);
                            if(A != numTiles)
                            {
                                Tile[A] = std::move(Tile[numTiles]);
                                treeWorldTileUpdate(&Tile[A]);
                            }
                            treeWorldTileRemove(&Tile[numTiles]);
                            numTiles = numTiles - 1;
                            MouseRelease = false;
                            break;
                        }
                    }
                }
            }
            else if(EditorCursor.Mode == OptCursor_t::LVL_BLOCKS) // Blocks
            {
                for(A = 1; A <= numBlock; A++)
                {
                    if(!BlockIsSizable[Block[A].Type] && !BlockIsSizable[EditorCursor.Block.Type])
                    {
                        if(CursorCollision(EditorCursor.Location, Block[A].Location)  && !Block[A].Hidden)
                            CanPlace = false;
                    }
                    else
                    {
                        if(Block[A].Type == EditorCursor.Block.Type)
                        {
                            if(fEqual(EditorCursor.Location.X, Block[A].Location.X) &&
                               fEqual(EditorCursor.Location.Y, Block[A].Location.Y))
                                CanPlace = false;
                        }
                    }
                }

                if(!BlockIsSizable[EditorCursor.Block.Type] && EditorCursor.Block.Type != 370)
                {
                    for(A = 1; A <= numNPCs; A++)
                    {
                        if(NPC[A].Type != 91 && NPC[A].Type != 259 && NPC[A].Type != 260)
                        {
                            if(CursorCollision(EditorCursor.Location, NPC[A].Location) && !NPC[A].Hidden && NPC[A].Active)
                                CanPlace = false;
                        }
                    }
                    for(A = 1; A <= 2; A++)
                    {
                        if(CursorCollision(EditorCursor.Location, PlayerStart[A]) == true && MagicHand == false)
                            CanPlace = false;
                    }
                }

                if(CanPlace) // Nothing is in the way
                {
//                    if(frmBlocks::chkFill.Value == 1)
//                    {
//                        BlockFill(EditorCursor.Block.Location); // and nuke an application with memory overflow error >:D
//                        if(MagicHand == true)
//                        {
//                            for(A = -FLBlocks; A <= FLBlocks; A++)
//                            {
//                                FirstBlock[A] = 1;
//                                LastBlock[A] = numBlock;
//                            }
//                            BlocksSorted = false;
//                        }
//                        FindSBlocks();
//                    }
//                    else
                    {
                        if(numBlock < maxBlocks) // Not out of blocks
                        {
                            numBlock++;
                            Block[numBlock] = EditorCursor.Block;
                            Block[numBlock].DefaultType = Block[numBlock].Type;
                            Block[numBlock].DefaultSpecial = Block[numBlock].Special;
                            syncLayersTrees_Block(numBlock);
                            // if(MagicHand == true)
                            // {
                            //     for(A = -FLBlocks; A <= FLBlocks; A++)
                            //     {
                            //         FirstBlock[A] = 1;
                            //         LastBlock[A] = numBlock;
                            //     }
                            //     BlocksSorted = false;
                            // }
                        }
                        FindSBlocks();
//                        if(nPlay.Online == true)
//                            Netplay::sendData Netplay::AddBlock(numBlock);
                    }

                }
            }
            else if(EditorCursor.Mode == OptCursor_t::LVL_SETTINGS && !MagicHand) // Level
            {
                if(EditorCursor.SubMode == 0) // Top
                {
                    level[curSection].Y = static_cast<int>(floor(static_cast<double>(EditorCursor.Location.Y / 32))) * 32;
                    if(level[curSection].Height - level[curSection].Y < 600)
                        level[curSection].Y = level[curSection].Height - 600;
                }
                else if(EditorCursor.SubMode == 1) // Left
                {
                    level[curSection].X = static_cast<int>(floor(static_cast<double>(EditorCursor.Location.X / 32))) * 32;
                    if(level[curSection].Width - level[curSection].X < 800)
                        level[curSection].X = level[curSection].Width - 800;
                }
                else if(EditorCursor.SubMode == 2) // Right
                {
                    level[curSection].Width = static_cast<int>(floor(static_cast<double>(EditorCursor.Location.X / 32))) * 32;
                    if(level[curSection].Width - level[curSection].X < 800)
                        level[curSection].Width = level[curSection].X + 800;
                }
                else if(EditorCursor.SubMode == 3) // Bottom
                {
                    level[curSection].Height = static_cast<int>(floor(static_cast<double>(EditorCursor.Location.Y / 32))) * 32;
                    if(level[curSection].Height - level[curSection].Y < 600)
                        level[curSection].Height = level[curSection].Y + 600;
                }
                else if(EditorCursor.SubMode == 4 || EditorCursor.SubMode == 5)
                {
                    // printf("Trying to place player at %f, %f...\n", EditorCursor.Location.X, EditorCursor.Location.Y);
                    if(EditorCursor.SubMode == 4)
                        B = 1;
                    else
                        B = 2;
                    for(A = 1; A <= 2; A++)
                    {
                        if(CursorCollision(EditorCursor.Location, PlayerStart[A]) == true && A != B)
                            CanPlace = false;
                    }
                    for(A = 1; A <= numBlock; A++)
                    {
                        if(CursorCollision(EditorCursor.Location, Block[A].Location) == true && Block[A].Hidden == false && Block[A].Invis == false && BlockIsSizable[Block[A].Type] == false && BlockNoClipping[Block[A].Type] == false && BlockOnlyHitspot1[Block[A].Type] == false && BlockSlope[Block[A].Type] == 0 && BlockSlope2[Block[A].Type] == 0)
                            CanPlace = false;
                    }
                    if(CanPlace == true)
                    {
                        if(EditorCursor.SubMode == 4)
                        {
                            PlayerStart[1] = EditorCursor.Location;
                        }
                        else
                        {
                            PlayerStart[2] = EditorCursor.Location;
                        }
                    }
                }
            }
            else if(EditorCursor.Mode == OptCursor_t::LVL_BGOS) // Backgrounds
            {
                for(A = 1; A <= numBackground; A++)
                {
                    if(EditorCursor.Background.Type == Background[A].Type)
                    {
                        if(CursorCollision(EditorCursor.Location, Background[A].Location) == true && Background[A].Hidden == false)
                            CanPlace = false;
                    }
                }

                if(CanPlace) // Nothing is in the way
                {
                    if(numBackground < maxBackgrounds) // Not out of backgrounds
                    {
                        numBackground++;
                        EditorCursor.Background.uid = numBackground;
                        Background[numBackground] = EditorCursor.Background;
                        syncLayers_BGO(numBackground);
                        if(MagicHand)
                        {
                            qSortBackgrounds(1, numBackground);
                            UpdateBackgrounds();
                            // ugh
                            syncLayers_AllBGOs();
                        }
//                        Netplay::sendData Netplay::AddBackground(numBackground);
                    }
                }
            }
            else if(EditorCursor.Mode == OptCursor_t::LVL_NPCS) // NPCs
            {
                if(EditorCursor.NPC.Type != 91 && EditorCursor.NPC.Type != 259 && EditorCursor.NPC.Type != 260)
                {
                    for(A = 1; A <= numBlock; A++)
                    {
                        if(!BlockIsSizable[Block[A].Type])
                        {
                            if(CursorCollision(EditorCursor.Location, Block[A].Location) &&
                               !Block[A].Hidden && BlockSlope[Block[A].Type] == 0 && BlockSlope2[Block[A].Type] == 0)
                                CanPlace = false;
                        }
                    }
                }

                for(A = 1; A <= numNPCs; A++)
                {
                    if(CursorCollision(EditorCursor.Location, NPC[A].Location) == true && NPC[A].Hidden == false && NPC[A].Active == true && (NPC[A].Type != 159 || EditorCursor.NPC.Type == 159))
                    {
                        if(NPC[A].Generator == false || NPC[A].Type == EditorCursor.NPC.Type)
                        {
                            if((!(EditorCursor.NPC.Type == 208) && !(NPC[A].Type == 208)) || (EditorCursor.NPC.Type == 208 && NPC[A].Type == 208))
                            {
                                if(!NPCIsAVine[NPC[A].Type])
                                    CanPlace = false;
                            }
                        }
                    }
                }

                for(A = 1; A <= 2; A++)
                {
                    if(CursorCollision(EditorCursor.Location, PlayerStart[A]) == true && MagicHand == false)
                        CanPlace = false;
                }

                if(!MouseRelease)
                    CanPlace = false;

                if(CanPlace) // Nothing is in the way
                {
                    if(numNPCs < maxNPCs - 20) // Not out of npcs
                    {
                        MouseRelease = false;
                        numNPCs++;
//                        if(frmNPCs::Bubble.Caption == "Yes" && frmNPCs::optNPCDirection(1).Value == true)
//                        {
//                            EditorCursor.NPC.Direction = 0;
//                            EditorCursor.NPC.DefaultDirection = 0;
//                        }

                        NPC[numNPCs] = EditorCursor.NPC;
                        syncLayers_NPC(numNPCs);
//                        Netplay::sendData Netplay::AddNPC(numNPCs);
                        if(!MagicHand)
                        {
                            // ugh
                            NPCSort();
                            syncLayers_AllNPCs();
                        }

                        if(MagicHand)
                        {
                            auto &n = NPC[numNPCs];
                            n.FrameCount = 0;
                            n.Active = true;
                            n.TimeLeft = 10;
                            n.DefaultDirection = n.Direction;
                            n.DefaultLocation = n.Location;
                            n.DefaultSpecial = int(n.Special);
                            CheckSectionNPC(numNPCs);
                        }
                    }
                }
            }
            else if(EditorCursor.Mode == OptCursor_t::LVL_WARPS) // Warps
            {
                int numWarpsMax = numWarps + 1;
                for(A = 1; A <= numWarpsMax; A++)
                {
                    if(Warp[A].PlacedEnt == false || Warp[A].PlacedExit == false)
                        break;
                }

                if(A > numWarps)
                    numWarps = A;

                if(EditorCursor.SubMode == 1 || EditorCursor.Warp.level != "" || EditorCursor.Warp.LevelEnt == true || EditorCursor.Warp.MapWarp == true)
                {
                    EditorCursor.Warp.Entrance = EditorCursor.Location;
                    EditorControls.MouseClick = false;
                    EditorCursor.Warp.PlacedEnt = true;
                }
                if(EditorCursor.SubMode == 2 || EditorCursor.Warp.level != "" || EditorCursor.Warp.LevelEnt == true || EditorCursor.Warp.MapWarp == true)
                {
                    EditorCursor.Warp.Exit = EditorCursor.Location;
                    EditorControls.MouseClick = false;
                    EditorCursor.Warp.PlacedExit = true;
                }
                Warp[A] = EditorCursor.Warp;
                Warp[A].Layer = EditorCursor.Layer;
                if (Warp[A].PlacedEnt && Warp[A].PlacedExit)
                {
                    EditorCursor.Warp.PlacedEnt = false;
                    EditorCursor.Warp.PlacedExit = false;
                    EditorCursor.SubMode = 1;
                }
                else if (Warp[A].PlacedEnt)
                    EditorCursor.SubMode = 2;
                else
                    EditorCursor.SubMode = 1;
                syncLayers_Warp(A);
//                if(nPlay.Online == true)
//                    Netplay::sendData Netplay::AddWarp[A];
            }
            else if(EditorCursor.Mode == OptCursor_t::WLD_TILES) // Tiles
            {
                for(A = 1; A <= numTiles; A++)
                {
                    if(CursorCollision(EditorCursor.Location, Tile[A].Location) == true)
                        CanPlace = false;
                }

                if(CanPlace) // Nothing is in the way
                {
                    if(numTiles < maxTiles) // Not out of blocks
                    {
                        numTiles++;
                        Tile[numTiles] = EditorCursor.Tile;
                        treeWorldTileAdd(&Tile[numTiles]);
                    }
                }
            }
            else if(EditorCursor.Mode == OptCursor_t::WLD_SCENES) // Scenery
            {
                for(A = 1; A <= numScenes; A++)
                {
                    if(CursorCollision(EditorCursor.Location, Scene[A].Location) == true)
                    {
                        if(EditorCursor.Scene.Type == Scene[A].Type)
                        {
                            if(fEqual(EditorCursor.Scene.Location.X, Scene[A].Location.X) &&
                               fEqual(EditorCursor.Scene.Location.Y, Scene[A].Location.Y))
                                CanPlace = false;
                        }
                    }
                }

                for(A = 1; A <= numWorldLevels; A++)
                {
                    if(CursorCollision(EditorCursor.Location, WorldLevel[A].Location) == true)
                        CanPlace = false;
                }

                if(CanPlace)
                {
                    if(numScenes < maxScenes)
                    {
                        numScenes++;
                        Scene[numScenes] = EditorCursor.Scene;
                        treeWorldSceneAdd(&Scene[numScenes]);
                    }
                }
            }
            else if(EditorCursor.Mode == OptCursor_t::WLD_LEVELS) // Level
            {
                for(A = 1; A <= numWorldPaths; A++)
                {
                    if(CursorCollision(EditorCursor.Location, WorldPath[A].Location) == true)
                        CanPlace = false;
                }

                for(A = 1; A <= numScenes; A++)
                {
                    if(CursorCollision(EditorCursor.Location, Scene[A].Location) == true)
                        CanPlace = false;
                }

                for(A = 1; A <= numWorldLevels; A++)
                {
                    if(CursorCollision(EditorCursor.Location, WorldLevel[A].Location) == true)
                    {
                        CanPlace = false;
                        qLevel = A;
                        UNUSED(qLevel);
                    }
                }

                if(CanPlace)
                {
                    if(numWorldLevels < maxWorldLevels)
                    {
                        numWorldLevels++;
                        WorldLevel[numWorldLevels] = EditorCursor.WorldLevel;
                        treeWorldLevelAdd(&WorldLevel[numWorldLevels]);
                    }
                }
            }
            else if(EditorCursor.Mode == OptCursor_t::WLD_PATHS) // Paths
            {
                for(A = 1; A <= numWorldPaths; A++)
                {
                    if(CursorCollision(EditorCursor.Location, WorldPath[A].Location) == true)
                        CanPlace = false;
                }

                for(A = 1; A <= numWorldLevels; A++)
                {
                    if(CursorCollision(EditorCursor.Location, WorldLevel[A].Location) == true)
                        CanPlace = false;
                }

                if(CanPlace)
                {
                    if(numWorldPaths < maxWorldPaths)
                    {
                        numWorldPaths++;
                        WorldPath[numWorldPaths] = EditorCursor.WorldPath;
                        treeWorldPathAdd(&WorldPath[numWorldPaths]);
                    }
                }
            }
            else if(EditorCursor.Mode == OptCursor_t::WLD_MUSIC) // Music
            {
                for(A = 1; A <= numWorldMusic; A++)
                {
                    if(CursorCollision(EditorCursor.Location, WorldMusic[A].Location) == true)
                        CanPlace = false;
                }

                if(CanPlace)
                {
                    EditorCursor.WorldMusic.Location = EditorCursor.Location;
                    numWorldMusic++;
                    WorldMusic[numWorldMusic] = EditorCursor.WorldMusic;
                    treeWorldMusicAdd(&WorldMusic[numWorldMusic]);
                }
            }
        }

        if(!MagicHand)
        {
            for(A = 1; A <= numNPCs; A++)
            {
                // .Frame = EditorNPCFrame(.Type, .Direction)
                NPCFrames(A);
            }
        }
    }
}

#ifndef NO_INTPROC
void UpdateInterprocess()
{
    if(!IntProc::hasCommand())
        return;

    IntProc::cmdLock();

    //Recive external commands!
    switch(IntProc::commandType())
    {
    case IntProc::MsgBox:
    {
        MessageText = IntProc::getCMD();
        PauseGame(1);
        break;
    }

    case IntProc::Cheat:
    {
        CheatString = IntProc::getCMD();
        CheatCode(CheatString[0]);
        break;
    }

    case IntProc::SetLayer:
    {
        EditorCursor.Layer = IntProc::getCMD();

        EditorCursor.Block.Layer = EditorCursor.Layer;
        EditorCursor.Background.Layer = EditorCursor.Layer;
        EditorCursor.NPC.Layer = EditorCursor.Layer;
        break;
    }

    case IntProc::SetNumStars:
    {
        auto s = IntProc::getCMD();
        numStars = SDL_atoi(s.c_str());
        break;
    }

    case IntProc::PlaceItem:
    {
        std::string raw = IntProc::getCMD();
        pLogDebug(raw.c_str());
        LevelData got;
        PGE_FileFormats_misc::RawTextInput raw_file(&raw);
        FileFormats::ReadExtendedLvlFile(raw_file, got);

        if(!got.meta.ReadFileValid)
        {
            pLogDebug(got.meta.ERROR_info.c_str());
            break;
        }

        if(raw.compare(0, 11, "BLOCK_PLACE") == 0)
        {
            if(got.blocks.empty())
                break;

            optCursor.current = 1;
            OptCursorSync();

            const LevelBlock &b = got.blocks[0];

            if(EditorCursor.Mode != OptCursor_t::LVL_BLOCKS ||
               EditorCursor.Block.Type != int(b.id))
                PlaySound(SFX_Grab);

            EditorCursor.Layer = b.layer;

            EditorCursor.Mode = OptCursor_t::LVL_BLOCKS;
            EditorCursor.Block = Block_t();
            EditorCursor.Block.Type = int(b.id);
            EditorCursor.Location.X = b.x;
            EditorCursor.Location.Y = b.y;
            EditorCursor.Location.Width = b.w;
            EditorCursor.Location.Height = b.h;
            if(EditorCursor.Block.Type <= maxBlockType && BlockIsSizable[EditorCursor.Block.Type])
            {
                EditorCursor.Block.Location.Width = 128;
                EditorCursor.Block.Location.Height = 128;
                EditorCursor.Location.Width = 128;
                EditorCursor.Location.Height = 128;
            }
            EditorCursor.Block.Invis = b.invisible;
            EditorCursor.Block.Slippy = b.slippery;
            EditorCursor.Block.Special = b.npc_id > 0 ? int(b.npc_id + 1000) : int(-b.npc_id);
            EditorCursor.Block.Layer = b.layer;
            EditorCursor.Block.TriggerHit = b.event_hit;
            EditorCursor.Block.TriggerLast = b.event_emptylayer;
            EditorCursor.Block.TriggerDeath = b.event_destroy;

            if(EditorCursor.Block.Type > maxBlockType) // Avoid out of range crash
                EditorCursor.Block.Type = 1;

            SetCursor();
        }
        else if(raw.compare(0, 9, "BGO_PLACE") == 0)
        {
            if(got.bgo.empty())
                break;

            optCursor.current = 3;
            OptCursorSync();

            const LevelBGO &b = got.bgo[0];

            if(EditorCursor.Mode != OptCursor_t::LVL_BGOS ||
               EditorCursor.Background.Type != int(b.id))
                PlaySound(SFX_Grab);

            EditorCursor.Layer = b.layer;

            EditorCursor.Mode = OptCursor_t::LVL_BGOS;
            EditorCursor.Background = Background_t();
            EditorCursor.Background.Type = int(b.id);
            EditorCursor.Location.X = b.x;
            EditorCursor.Location.Y = b.y;
            EditorCursor.Background.Layer = b.layer;
            EditorCursor.Background.SortPriority = -1;
            EditorCursor.Background.uid = (numBackground + 1);
            EditorCursor.Background.zMode = b.z_mode;
            EditorCursor.Background.zOffset = b.z_offset;

            bgoApplyZMode(&EditorCursor.Background, int(b.smbx64_sp));

            if(EditorCursor.Background.Type > maxBackgroundType) // Avoid out of range crash
                EditorCursor.Background.Type = 1;

            SetCursor();
        }
        else if(raw.compare(0, 9, "NPC_PLACE") == 0)
        {
            if(got.npc.empty())
                break;

            optCursor.current = 4;
            OptCursorSync();

            const LevelNPC &n = got.npc[0];

            if(EditorCursor.Mode != OptCursor_t::LVL_NPCS ||
               EditorCursor.NPC.Type != int(n.id))
                PlaySound(SFX_Grab);

            EditorCursor.Layer = n.layer;

            EditorCursor.Mode = OptCursor_t::LVL_NPCS;
            EditorCursor.NPC = NPC_t();
            EditorCursor.NPC.Type = int(n.id);
            EditorCursor.NPC.Hidden = false;
            EditorCursor.Location.X = n.x;
            EditorCursor.Location.Y = n.y;
            EditorCursor.NPC.Direction = n.direct;

            if(EditorCursor.NPC.Type > maxNPCType) // Avoid out of range crash
                EditorCursor.NPC.Type = 1;

            if(EditorCursor.NPC.Type == 91 || EditorCursor.NPC.Type == 96 || EditorCursor.NPC.Type == 283 || EditorCursor.NPC.Type == 284)
            {
                EditorCursor.NPC.Special = n.contents;
                EditorCursor.NPC.DefaultSpecial = int(EditorCursor.NPC.Special);
            }
            if(EditorCursor.NPC.Type == 288 || EditorCursor.NPC.Type == 289 || (EditorCursor.NPC.Type == 91 && int(EditorCursor.NPC.Special) == 288))
            {
                EditorCursor.NPC.Special2 = n.special_data;
                EditorCursor.NPC.DefaultSpecial2 = int(EditorCursor.NPC.Special2);
            }

            if(NPCIsAParaTroopa[EditorCursor.NPC.Type])
            {
                EditorCursor.NPC.Special = n.special_data;
                EditorCursor.NPC.DefaultSpecial = int(EditorCursor.NPC.Special);
            }

            if(NPCIsCheep[EditorCursor.NPC.Type])
            {
                EditorCursor.NPC.Special = n.special_data;
                EditorCursor.NPC.DefaultSpecial = int(EditorCursor.NPC.Special);
            }

            if(EditorCursor.NPC.Type == 260)
            {
                EditorCursor.NPC.Special = n.special_data;
                EditorCursor.NPC.DefaultSpecial = int(EditorCursor.NPC.Special);
            }

            EditorCursor.NPC.Generator = n.generator;
            if(EditorCursor.NPC.Generator)
            {
                EditorCursor.NPC.GeneratorDirection = n.generator_direct;
                EditorCursor.NPC.GeneratorEffect = n.generator_type;
                EditorCursor.NPC.GeneratorTimeMax = n.generator_period;
            }

            EditorCursor.NPC.Text = n.msg;

            EditorCursor.NPC.Inert = n.friendly;
            if(EditorCursor.NPC.Type == 151)
                EditorCursor.NPC.Inert = true;
            EditorCursor.NPC.Stuck = n.nomove;
            EditorCursor.NPC.DefaultStuck = EditorCursor.NPC.Stuck;

            EditorCursor.NPC.Legacy = n.is_boss;

            EditorCursor.NPC.Layer = n.layer;
            EditorCursor.NPC.TriggerActivate = n.event_activate;
            EditorCursor.NPC.TriggerDeath = n.event_die;
            EditorCursor.NPC.TriggerTalk = n.event_talk;
            EditorCursor.NPC.TriggerLast = n.event_emptylayer;
            EditorCursor.NPC.AttLayer = n.attach_layer;

            EditorCursor.NPC.DefaultType = EditorCursor.NPC.Type;
            EditorCursor.NPC.Location.Width = NPCWidth[EditorCursor.NPC.Type];
            EditorCursor.NPC.Location.Height = NPCHeight[EditorCursor.NPC.Type];
            EditorCursor.NPC.DefaultLocation = EditorCursor.NPC.Location;
            EditorCursor.NPC.DefaultDirection = EditorCursor.NPC.Direction;
            EditorCursor.NPC.TimeLeft = 1;
            EditorCursor.NPC.Active = true;
            EditorCursor.NPC.JustActivated = 1;
            SetCursor();
        }
        else
            PlaySound(SFX_Fireworks);

        break;
    }
    }

    IntProc::cmdUnLock();
}
#endif // #ifndef NO_INTPROC

int EditorNPCFrame(int A, int C, int N)
{
    float C_ = C;
    return EditorNPCFrame(A, C_, N);
}

int EditorNPCFrame(int A, float& C, int N)
{
    int ret = 0;
// find the default left/right frames for NPCs

    if(A > maxNPCType)
        return ret;
    float B = 0;
    int D = 0;
    int E = 0;
    B = C;
    while(int(B) == 0)
        B = (iRand(3)) - 1;

    if(LevelEditor == false)
        C = B;
    if(A == 241)
        ret = 4;
    if(A == 195)
        ret = 3;

    // suits
    if(N > 0)
    {
        if(A == 169 || A == 170)
        {
            E = 0;
            for(D = 1; D <= numPlayers; D++)
            {
                if(!Player[D].Dead && Player[D].Section == NPC[N].Section && Player[D].Character != 3 &&
                    Player[D].Character != 4 && Player[D].TimeToLive == 0)
                {
                    if(E == 0 || std::abs(NPC[N].Location.X + NPC[N].Location.Width / 2.0 -
                                          (Player[D].Location.X + Player[D].Location.Width / 2.0)) +
                                 std::abs(NPC[N].Location.Y + NPC[N].Location.Height / 2.0 -
                                          (Player[D].Location.Y + Player[D].Location.Height / 2.0)) < D)
                    {
                        E = CInt(std::abs(NPC[N].Location.X + NPC[N].Location.Width / 2.0 - (Player[D].Location.X + Player[D].Location.Width / 2.0)) +
                                 std::abs(NPC[N].Location.Y + NPC[N].Location.Height / 2.0 - (Player[D].Location.Y + Player[D].Location.Height / 2.0)));
                        if(Player[D].Character == 5)
                            ret = 1;
                        else
                            ret = 0;
                    }
                }
            }
        }
    }


    if(A == 135 || A == 4 || A == 6 || A == 19 || A == 20 || A == 23 || A == 25 || A == 28 || A == 36 || A == 38 ||
       A == 42 || A == 43 || A == 44 || A == 193 || A == 35 || A == 191 || A == 52 || A == 72 || A == 77 || A == 108 ||
       (A >= 109 && A <= 112) || (A >= 121 && A <= 124) || A == 125 || (A >= 129 && A <= 132) || A == 136 || A == 158 ||
        A == 164 || A == 163 || A == 162 || A == 165 || A == 166 || A == 189 || A == 199 || A == 209 || A == 207 ||
        A == 229 || A == 230 || A == 232 || A == 236 || A == 233 || A == 173 || A == 175 || A == 177 ||
        A == 178 || A == 176) // Koopa troopas / Shy guy
    {
        if(int(B) == -1)
            ret = 0;
        else
            ret = 2;
    }

    // Bullet Bills
    if(A == 17 || A == 18 || A == 29 || A == 31 || A == 84 || A == 94 || A == 198 ||
       NPCIsYoshi[A] || A == 101 || A == 102 || A == 181 || A == 81)
    {
        if(int(B) == -1)
            ret = 0;
        else
            ret = 1;
    }

    // Hammer Bros.
    if(A == 29 || A == 55 || A == 75 || A == 78 || A == 168 || A == 234)
    {
        if(int(B) == -1)
            ret = 0;
        else
            ret = 3;
    }

    if(A == 34)
    {
        if(int(B) == -1)
            ret = 1;
        else
            ret = 0;
    }

    if(A == 201)
    {
        if(int(B) == -1)
            ret = 0;
        else
            ret = 8;
    }

    if(A == 137)
    {
        if(int(B) == -1)
            ret = 0;
        else
            ret = 6;
    }

    if(A == 86 || (A >= 117 && A <= 120) || A == 200)
    {
        if(int(B) == -1)
            ret = 0;
        else
            ret = 5;
    }

    // winged koopa / bob-omb buddy
    if(A == 76 || A == 107 || A == 160 || A == 161 || A == 167 || A == 203 || A == 204)
    {
        if(int(B) == -1)
            ret = 0;
        else
            ret = 4;
    }

    // Birdo
    if(A == 39 || A == 208)
    {
        if(int(B) == -1)
            ret = 0;
        else
            ret = 5;
    }

    if(A == 45)
        ret = BlockFrame[4];

    if(A == 56)
    {
        ret = SpecialFrame[2];
        if(int(B) == 1)
            ret = ret + 4;
    }

    if(A == 57) // smb3 belt
    {
        if(int(B) == -1)
            ret = SpecialFrame[4];
        else
            ret = 3 - SpecialFrame[4];
    }

    if(A == 60 || A == 62 || A == 64 || A == 66)
    {
        if(int(B) == -1)
            ret = 1;
    }

    return ret;
}

void GetEditorControls()
{
    if(MagicHand)
        return;
    if(EditorControls.Select)
    {
        optCursor.current = 13;
        SetCursor();
    }
    if(EditorControls.Erase)
    {
        optCursor.current = 0;
        SetCursor();
    }
#ifdef NEW_EDITOR
    if(!WorldEditor && EditorControls.TestPlay)
    {
        EditorBackup();
        Backup_FullFileName = FullFileName;
        FullFileName = FullFileName + "tst";
        SaveLevel(FullFileName);
        HasCursor = false;
        zTestLevel();
    }
    if(EditorControls.SwitchScreens && MouseRelease)
    {
        editorScreen.active = !editorScreen.active;
        HasCursor = false;
        MouseRelease = false;
        MenuMouseRelease = false;
    }
#endif
}

void SetCursor()
{
#ifndef NO_INTPROC
    if(IntProc::isWorking() && EditorCursor.Mode != optCursor.current)
    {
        switch(optCursor.current) // Tell the IPC Editor to close the properties dialog
        {
        case OptCursor_t::LVL_ERASER:
        case OptCursor_t::LVL_SELECT:
            switch(EditorCursor.Mode)
            {
            case OptCursor_t::LVL_BLOCKS:
            case OptCursor_t::LVL_BGOS:
            case OptCursor_t::LVL_NPCS:
            case OptCursor_t::LVL_WARPS:
            case OptCursor_t::LVL_WATER:
                IntProc::sendCloseProperties();
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
#endif
    EditorCursor.Mode = optCursor.current;

    if(EditorCursor.Mode == OptCursor_t::LVL_ERASER && WorldEditor)
        EditorCursor.Mode = OptCursor_t::LVL_SELECT;

//    EditorCursor.Layer = frmLayers::lstLayer::List(frmLayers::lstLayer::ListIndex);
    if(EditorCursor.Mode == OptCursor_t::LVL_ERASER0 || EditorCursor.Mode == OptCursor_t::LVL_ERASER) // Eraser
    {
        EditorCursor.Location.Width = 18;
        EditorCursor.Location.Height = 8;
    }
    else if(EditorCursor.Mode == OptCursor_t::LVL_SELECT || EditorCursor.Mode == 14) // Selection
    {
        EditorCursor.Location.Width = 4;
        EditorCursor.Location.Height = 4;
    }
    else if(EditorCursor.Mode == OptCursor_t::LVL_WATER) // Water
    {
        // some of these seem sort of unbelievable........
//        EditorCursor.Location.Height = frmWater::WaterH * 32;
//        EditorCursor.Location.Width = frmWater::WaterW * 32;
        EditorCursor.Location.Height = EditorCursor.Water.Location.Height;
        EditorCursor.Location.Width = EditorCursor.Water.Location.Width;
        EditorCursor.Water.Location.X = EditorCursor.Location.X;
        EditorCursor.Water.Location.Y = EditorCursor.Location.Y;
        EditorCursor.Water.Buoy = 0; // frmWater.scrBuoy / 100
        EditorCursor.Water.Layer = EditorCursor.Layer;
//        if(frmWater::Quicksand.Caption == "Yes")
            // EditorCursor.Water.Quicksand = false;
//        else
//            EditorCursor.Water.Quicksand = false;
    }
    else if(EditorCursor.Mode == OptCursor_t::LVL_BLOCKS) // Blocks
    {
        if(EditorCursor.Block.Type <= 0)
            EditorCursor.Block.Type = 1;
        if(EditorCursor.Block.Type > maxBlockType)
            EditorCursor.Block.Type = 1;

        EditorCursor.Block.Location.X = EditorCursor.Location.X;
        EditorCursor.Block.Location.Y = EditorCursor.Location.Y;
        EditorCursor.Block.Layer = EditorCursor.Layer;
//        EditorCursor.Block.TriggerHit = frmAdvancedBlock::TriggerHit.Text;
//        EditorCursor.Block.TriggerDeath = frmAdvancedBlock::TriggerDeath.Text;
//        EditorCursor.Block.TriggerLast = frmAdvancedBlock::TriggerLast.Text;
//        for(A = 1; A <= frmBlocks::Block.Count; A++)
//        {
//            if(frmBlocks::Block(A).Value == true && frmBlocks::Block(A).Visible == true)
//            {
//                EditorCursor.Block.Type = A;
//                break;
//            }
//        }
        if(!BlockIsSizable[EditorCursor.Block.Type])
        {
            if(EditorCursor.Block.Location.Width <= 0)
            {
                if(BlockWidth[EditorCursor.Block.Type] > 0)
                    EditorCursor.Block.Location.Width = BlockWidth[EditorCursor.Block.Type];
                else
                    EditorCursor.Block.Location.Width = 32;
            }

            if(EditorCursor.Block.Location.Height <= 0)
            {
                if(BlockHeight[EditorCursor.Block.Type] > 0)
                    EditorCursor.Block.Location.Height = BlockHeight[EditorCursor.Block.Type];
                else
                    EditorCursor.Block.Location.Height = 32;
            }
        }

        EditorCursor.Location.Width = EditorCursor.Block.Location.Width;
        EditorCursor.Location.Height = EditorCursor.Block.Location.Height;
    }
    else if(EditorCursor.Mode == OptCursor_t::LVL_SETTINGS) // Level
    {
        if(EditorCursor.SubMode == 4)
        {
            EditorCursor.Location.Width = Physics.PlayerWidth[1][2]; // Mario
            EditorCursor.Location.Height = Physics.PlayerHeight[1][2];
        }
        else if(EditorCursor.SubMode == 5)
        {
            EditorCursor.Location.Width = Physics.PlayerWidth[2][2]; // Luigi
            EditorCursor.Location.Height = Physics.PlayerHeight[2][2];
        }
        else
        {
            EditorCursor.Location.Width = 32;
            EditorCursor.Location.Height = 32;
        }
    }
    else if(EditorCursor.Mode == OptCursor_t::LVL_BGOS) // Background
    {
        if(EditorCursor.Background.Type <= 0)
            EditorCursor.Background.Type = 1;
        if(EditorCursor.Background.Type > maxBackgroundType)
            EditorCursor.Background.Type = 1;

        EditorCursor.Background.Layer = EditorCursor.Layer;
        EditorCursor.Background.Location = EditorCursor.Location;
        EditorCursor.Background.Location.Width = BackgroundWidth[EditorCursor.Background.Type];
        EditorCursor.Background.Location.Height = BackgroundHeight[EditorCursor.Background.Type];
        EditorCursor.Location.Width = EditorCursor.Background.Location.Width;
        EditorCursor.Location.Height = EditorCursor.Background.Location.Height;
    }
    else if(EditorCursor.Mode == OptCursor_t::LVL_NPCS) // NPCs
    {
        int t = EditorCursor.NPC.Type;
        // handled elsewhere in new editor
        if(MagicHand)
        {
            if(t != 91 && t != 96 && t != 283 && t != 284)
                EditorCursor.NPC.Special = 0;
            if(t != 288 && t != 289 && t != 91 && t != 260)
                EditorCursor.NPC.Special2 = 0.0;
        }
        EditorCursor.NPC.Special3 = 0.0;
        EditorCursor.NPC.Special4 = 0.0;
        EditorCursor.NPC.Special5 = 0.0;
        EditorCursor.NPC.Special6 = 0.0;
        EditorCursor.NPC.Layer = EditorCursor.Layer;
        EditorCursor.NPC.Location = EditorCursor.Location;

        if(NPCWidth[EditorCursor.NPC.Type] > 0)
            EditorCursor.NPC.Location.Width = NPCWidth[EditorCursor.NPC.Type];
        else
            EditorCursor.NPC.Location.Width = 32;
        if(NPCHeight[EditorCursor.NPC.Type] > 0)
            EditorCursor.NPC.Location.Height = NPCHeight[EditorCursor.NPC.Type];
        else
            EditorCursor.NPC.Location.Height = 32;
        EditorCursor.Location.Width = EditorCursor.NPC.Location.Width;
        EditorCursor.Location.Height = EditorCursor.NPC.Location.Height;

        EditorCursor.Location.SpeedX = 0.0;
        EditorCursor.Location.SpeedY = 0.0;

        EditorCursor.NPC.Frame = EditorNPCFrame(EditorCursor.NPC.Type, EditorCursor.NPC.Direction);
        EditorCursor.NPC.Active = true;
    }
    else if(EditorCursor.Mode == OptCursor_t::LVL_WARPS) // Warps
    {
        EditorCursor.Warp.Layer = EditorCursor.Layer;
        EditorCursor.Location.Width = 32;
        EditorCursor.Location.Height = 32;
        // EditorCursor.Warp is now the canonical Warp object.
        // It stores the warp's entrance and exit until the warp is placed,
        // instead of finding and modifying an existing warp.
        // EditorCursor.Warp.Entrance = EditorCursor.Location;
        // EditorCursor.Warp.Exit = EditorCursor.Location;
    }
    else if(EditorCursor.Mode == OptCursor_t::LVL_ERASER) // Eraser
    {
        EditorCursor.Location.Width = 32;
        EditorCursor.Location.Height = 32;
    }
    else if(EditorCursor.Mode == 7) // Tiles
    {
//        for(A = 1; A <= frmTiles::Tile.Count; A++)
//        {
//            if(frmTiles::Tile(A).Value == true)
//            {
//                EditorCursor.Tile.Type = A;
//                break;
//            }
//        }
        if (EditorCursor.Tile.Type == 0)
            EditorCursor.Tile.Type = 1;
        EditorCursor.Location.Width = TileWidth[EditorCursor.Tile.Type];
        EditorCursor.Location.Height = TileHeight[EditorCursor.Tile.Type];
        EditorCursor.Tile.Location = EditorCursor.Location;
    }
    else if(EditorCursor.Mode == 8) // Scene
    {
//        for(A = 1; A <= frmScene::Scene.Count; A++)
//        {
//            if(frmScene::Scene(A).Value == true)
//            {
//                EditorCursor.Scene.Type = A;
//                EditorCursor.Scene.Active = true;
//                break;
//            }
//        }
//        EditorCursor.Location.Width = SceneWidth(EditorCursor.Scene::Type);
//        EditorCursor.Location.Height = SceneHeight(EditorCursor.Scene::Type);
        if (EditorCursor.Scene.Type == 0)
            EditorCursor.Scene.Type = 1;
        EditorCursor.Location.Width = SceneWidth[EditorCursor.Scene.Type];
        EditorCursor.Location.Height = SceneHeight[EditorCursor.Scene.Type];
        EditorCursor.Scene.Location = EditorCursor.Location;
    }
    else if(EditorCursor.Mode == 9) // Levels
    {
//        for(A = 1; A < frmLevels::WorldLevel.Count; A++)
//        {
//            if(frmLevels::WorldLevel(A).Value == true)
//            {
//                EditorCursor.WorldLevel.Type = A;
//                EditorCursor.WorldLevel.Active = true;
//                break;
//            }
//        }
        if (EditorCursor.WorldLevel.Type == 0)
            EditorCursor.WorldLevel.Type = 1;
        EditorCursor.Location.Width = 32;
        EditorCursor.Location.Height = 32;
        EditorCursor.WorldLevel.Location = EditorCursor.Location;
//        EditorCursor.WorldLevel.FileName = frmLevels::txtFilename.Text;
//        if(EditorCursor.WorldLevel::FileName != "" && EditorCursor.WorldLevel::FileName.substr(EditorCursor.WorldLevel::FileName.Length - 4) != StringHelper::toLower(".lvl"))
//            EditorCursor.WorldLevel.FileName = EditorCursor.WorldLevel::FileName + ".lvl";
//        EditorCursor.WorldLevel.StartWarp = SDL_atoi(frmLevels::scrWarp.Value);
//        EditorCursor.WorldLevel.LevelName = frmLevels::txtLevelName.Text;
        // these are set in the new editor screen.
        // EditorCursor.WorldLevel.Visible = false;
        // EditorCursor.WorldLevel.Start = false;
        // EditorCursor.WorldLevel.Path = false;
        // EditorCursor.WorldLevel.Path2 = false;
//        if(frmLevels::chkVisible.Value > 0)
//            EditorCursor.WorldLevel.Visible = true;
//        if(frmLevels::chkStart.Value > 0)
//            EditorCursor.WorldLevel.Start = true;
//        if(frmLevels::chkPath.Value > 0)
//            EditorCursor.WorldLevel.Path = true;
//        if(frmLevels::chkPath2.Value > 0)
//            EditorCursor.WorldLevel.Path2 = true;
//        if(frmLevels::txtX.Text == "")
//            EditorCursor.WorldLevel.WarpX = -1;
//        else
//            EditorCursor.WorldLevel.WarpX = SDL_atoi(frmLevels::txtX.Text);
//        if(frmLevels::txtX.Text == "")
//            EditorCursor.WorldLevel.WarpY = -1;
//        else
//            EditorCursor.WorldLevel.WarpY = SDL_atoi(frmLevels::txtY.Text);

//        for(A = 1; A <= 4; A++)
//        {
//            if(frmLevels::cmbExit(A).ListIndex < 0)
//                frmLevels::cmbExit(A).ListIndex = 0;
//            EditorCursor.WorldLevel::LevelExit(A) = frmLevels::cmbExit(A).ListIndex - 1;
//        }
    }
    else if(EditorCursor.Mode == OptCursor_t::WLD_PATHS) // Paths
    {
//        for(A = 1; A <= frmPaths::WorldPath.Count; A++)
//        {
//            if(frmPaths::WorldPath(A).Value == true)
//            {
//                EditorCursor.WorldPath.Type = A;
//                EditorCursor.WorldPath.Active = true;
//                break;
//            }
//        }
        if (EditorCursor.WorldPath.Type == 0)
            EditorCursor.WorldPath.Type = 1;
        EditorCursor.Location.Width = 32;
        EditorCursor.Location.Height = 32;
        EditorCursor.WorldPath.Location = EditorCursor.Location;
    }
    else if(EditorCursor.Mode == OptCursor_t::WLD_MUSIC) // World Music
    {
        EditorCursor.Location.Height = 32;
        EditorCursor.Location.Width = 32;
        EditorCursor.WorldMusic.Location = EditorCursor.Location;
        // make it play the music
        if(curWorldMusic != EditorCursor.WorldMusic.Type)
        {
            StartMusic(EditorCursor.WorldMusic.Type);
        }
//        for(A = 0; A < frmMusic::optMusic.Count; A++)
//        {
//            if(frmMusic::optMusic(A).Value == true)
//            {
//                EditorCursor.WorldMusic.Type = A;
//                break;
//            }
//        }
    }
}

void PositionCursor()
{
//    if(EditorCursor.Mode == 4 && frmNPCs::Buried.Caption == "Yes")
//        EditorCursor.Location.Y = EditorCursor.Location.Y + 16;

    if(EditorCursor.Mode == 9)
    {
//        frmLevels.lblX = EditorCursor.Location.X;
//        frmLevels.lblY = EditorCursor.Location.Y;
    }

    if(EditorCursor.Mode == OptCursor_t::LVL_SELECT || EditorCursor.Mode == 14 ||
      (EditorCursor.Mode == OptCursor_t::LVL_SETTINGS && EditorCursor.SubMode < 4))
        return;

    if(!enableAutoAlign)
    {
        EditorCursor.Location.X = EditorCursor.Location.X - EditorCursor.Location.Width + 4;
        EditorCursor.Location.Y = EditorCursor.Location.Y - EditorCursor.Location.Height + 12;
        return;
    }

    if(EditorCursor.Mode == OptCursor_t::LVL_SETTINGS && EditorCursor.SubMode >= 4)
        EditorCursor.Location.X = EditorCursor.Location.X - 14;

    if(EditorCursor.Mode == OptCursor_t::LVL_SETTINGS || EditorCursor.Mode == OptCursor_t::LVL_NPCS)
    {
        if(!(EditorCursor.Mode == OptCursor_t::LVL_NPCS && EditorCursor.NPC.Type == 52))
        {
            if(std::fmod(EditorCursor.Location.Width, 32) != 0.0)
            {
                if(EditorCursor.Location.Width > 32)
                    EditorCursor.Location.X = EditorCursor.Location.X - std::fmod(EditorCursor.Location.Width, 32) / 2.0;
                else if(EditorCursor.Location.Width <= 16)
                {
                    EditorCursor.Location.X = EditorCursor.Location.X + std::fmod(32, EditorCursor.Location.Width) / 2.0;
                    EditorCursor.Location.X = EditorCursor.Location.X + (32 - EditorCursor.Location.Width) / 2.0;
                }
                else if(EditorCursor.Location.Width < 32)
                    EditorCursor.Location.X = EditorCursor.Location.X + std::fmod(32, EditorCursor.Location.Width) / 2.0;
                else
                    EditorCursor.Location.X = EditorCursor.Location.X + std::fmod(32, EditorCursor.Location.Width) / 2.0;
            }

            if(std::fmod(EditorCursor.Location.Height, 32) != 0.0)
                EditorCursor.Location.Y = EditorCursor.Location.Y - std::fmod(EditorCursor.Location.Height, 32);
        }
    }

    if(EditorCursor.Mode == OptCursor_t::LVL_BGOS)
    {
        if(EditorCursor.Background.Type == 13) // End level container
        {
            EditorCursor.Location.X = EditorCursor.Location.X - 12;
            EditorCursor.Location.Y = EditorCursor.Location.Y - 12;
        }
        if(EditorCursor.Background.Type == 156 || EditorCursor.Background.Type == 157)
            EditorCursor.Location.Y = EditorCursor.Location.Y + 16;
    }

    else if(EditorCursor.Mode == OptCursor_t::LVL_NPCS)
    {
        if(EditorCursor.NPC.Type == 245 || EditorCursor.NPC.Type == 8 ||
           EditorCursor.NPC.Type == 270 || EditorCursor.NPC.Type == 93 ||
           EditorCursor.NPC.Type == 180 || EditorCursor.NPC.Type ==179 ||
           EditorCursor.NPC.Type == 37 || EditorCursor.NPC.Type ==51) // Piranha Plants
            EditorCursor.Location.X = EditorCursor.Location.X + 16;
        else if(EditorCursor.NPC.Type == 197)
        {
            EditorCursor.Location.X = EditorCursor.Location.X - 8;
            EditorCursor.Location.Y = EditorCursor.Location.Y + 16;
        }

//        if(frmNPCs::Buried.Caption == "Yes")
        if(EditorCursor.NPC.Type == 91)
        {
            if(!enableAutoAlign)
                EditorCursor.Location.Y = EditorCursor.Location.Y + 0/*16*/;
            else
                EditorCursor.Location.Y = EditorCursor.Location.Y + 16/*32*/;
        }
        else if(EditorCursor.NPC.Type == 105)
            EditorCursor.Location.Y = EditorCursor.Location.Y + 22;
        else if(EditorCursor.NPC.Type == 106)
            EditorCursor.Location.Y = EditorCursor.Location.Y + 16;
        else if(EditorCursor.NPC.Type == 260)
            EditorCursor.Location.Y = EditorCursor.Location.Y - 8;
    }

    if(EditorCursor.Mode == OptCursor_t::LVL_NPCS)
    {
        if(NPCHeight[EditorCursor.NPC.Type] < 32)
            EditorCursor.Location.Y = EditorCursor.Location.Y + 32;
    }
}

void HideCursor()
{
    EditorCursor.Location.X = vScreenX[1] - 800;
    EditorCursor.X = float(vScreenX[1] - 800);
    EditorCursor.Location.Y = vScreenY[1] - 600;
    EditorCursor.Y = float(vScreenY[1] - 600);
    HasCursor = false;
    EditorControls.ScrollDown = false;
    EditorControls.ScrollRight = false;
    EditorControls.MouseClick = false;
    EditorControls.ScrollLeft = false;
    EditorControls.ScrollUp = false;
}

void KillWarp(int A)
{
    Warp_t blankWarp;
    Warp[A] = Warp[numWarps];
    Warp[numWarps] = blankWarp;
    numWarps--;
    syncLayers_Warp(A);
    syncLayers_Warp(numWarps+1);
}

void zTestLevel(bool magicHand, bool interProcess)
{
    int A = 0;
    Player_t blankPlayer;
    qScreen = false;

    for(A = 1; A <= numCharacters; A++)
    {
        SavedChar[A] = blankPlayer;
        SavedChar[A].State = 1;
        SavedChar[A].Character = A;
    }

    for(A = 1; A <= numNPCs; A++)
    {
        auto &n = NPC[A];
        // If .Generator = True Then
        n.TimeLeft = 0;
        n.Active = false;
        // End If
    }

    GamePaused = false;
    MessageText.clear();
//  frmNPCs::chkMessage.Value = 0;
    BattleLives[1] = 3;
    BattleLives[2] = 3;
    BattleWinner = 0;
    BattleOutro = 0;
//  frmLevelEditor::mnuOnline.Enabled = false;
    Player[1].Hearts = 1;
    Player[2].Hearts = 1;
    StopMusic();
    Score = 0;
    Coins = 0;
    Lives = 3;

    if(numPlayers == 0)
        numPlayers = 1;

    if(Checkpoint.empty()) // Don't reset players when resume at the checkpoint
    {
        if(numPlayers > 2 /* ||nPlay.Online == true*/)
        {
            for(A = 1; A <= numPlayers; A++)
            {
                Player[A].Hearts = 0;
                Player[A].State = testPlayer[1].State;
                Player[A].HeldBonus = 0;
                Player[A].Dead = false;
                Player[A].Mount = testPlayer[1].Mount;
                Player[A].MountType = testPlayer[1].MountType;
                Player[A].Character = testPlayer[1].Character;
                if(Player[A].Character == 0)
                    Player[1].Character = 1;
            }
        }
        else
        {
            for(A = 2; A >= 1; A--)
            {
                Player[A].State = testPlayer[A].State;
                Player[A].HeldBonus = 0;
                Player[A].Dead = false;
                Player[A].Mount = testPlayer[A].Mount;
                Player[A].MountType = testPlayer[A].MountType;
                Player[A].Character = testPlayer[A].Character;
                if(Player[A].Character == 0)
                    Player[A].Character = A;
                SavedChar[Player[A].Character] = Player[A];
            }
        }
    }

    LevelEditor = false;
    SetupPlayers();
    MagicHand = magicHand;

    if(TestFullscreen)
    {
#ifndef __ANDROID__
        ChangeScreen();
#endif
        DoEvents();
        MagicHand = false;
    }

#ifndef NO_INTPROC
    if(interProcess)
    {
        LevelData data;
        FileFormats::CreateLevelData(data);
        data.meta.ReadFileValid = false;
        pLogDebug("ICP: Requesting editor for a file....");
        IntProc::sendMessage("CMD:CONNECT_TO_ENGINE");
        ElapsedTimer time;
        time.start();
        //wait for accepting of level data
        bool timeOut = false;
        int attempts = 0;

        pLogDebug("ICP: Waiting reply....");
        IntProc::setState("Waiting for input data...");
        while(!IntProc::hasLevelData())
        {
            UpdateLoad();

            //Abort loading process and exit from game if window was closed
            if(!GameIsActive)
                return;

            if(!IntProc::levelReceivingInProcess() && time.elapsed() > 1500)
            {
                pLogDebug("ICP: Waiting #%d....", attempts);
                IntProc::sendMessage("CMD:CONNECT_TO_ENGINE"); // Re-ask again
                time.restart();
                attempts += 1;
            }

            if(attempts > 4)
            {
                pLogWarning("ICP: Wait timeout");
                timeOut = true;
                IntProc::setState("ERROR: Wait time out.");
                UpdateLoad();
                PGE_Delay(1000);
                GameIsActive = false;
                return;
            }
            PGE_Delay(2);
        }

        data = IntProc::editor->m_acceptedLevel;
        if(!timeOut && !data.meta.ReadFileValid)
        {
            pLogWarning("Bad file format!");
            GameIsActive = false;
            return;
        }

        OpenLevelData(data);

        pLogDebug("ICP: Done, starting a game....");
        IntProc::setState("Done. Starting game...");
    }
    else
#endif
    {
        if(!OpenLevel(FullFileName))
        {
            MessageText = fmt::format_ne("ERROR: Can't open \"{0}\": file doesn't exist or corrupted.", FullFileName);
            PauseGame(1);
            ErrorQuit = true;
        }
    }

    if(SingleCoop > 0)
        ScreenType = 6;
    SetupScreens();
    TestLevel = true;
    LevelSelect = false;
    EndLevel = false;
    ReturnWarp = 0;
    StartWarp = 0;

    optCursor.current = 13;
    OptCursorSync();

    if(numPlayers == 1)
        ScreenType = 1;
    else
        ScreenType = 5;
}

void MouseMove(float X, float Y, bool /*nCur*/)
{
    int A = 0;
    HasCursor = true;
    A = 1;
    if(SingleCoop > 0)
        A = SingleCoop;

    else if(ScreenType == 5 && vScreen[2].Visible)
    {
        if(X < float(vScreen[2].Left + vScreen[2].Width))
        {
            if(X > float(vScreen[2].Left))
            {
                if(Y < float(vScreen[2].Top + vScreen[2].Height))
                {
                    if(Y > float(vScreen[2].Top))
                        A = 2;
                }
            }
        }
    }
    else
        A = 1;

    if(EditorCursor.Mode == 0 || EditorCursor.Mode == 6 || EditorCursor.Mode == 13 || EditorCursor.Mode == 14 /*|| frmLevelEditor::chkAlign.Value == 0*/)
    {
        EditorCursor.Location.X = double(X) - vScreenX[A];
        EditorCursor.Location.Y = double(Y) - vScreenY[A];
        PositionCursor();
    }
    else
    {
        if(MagicHand)
        {
            if(std::fmod((vScreenY[A] + 8), 32) != 0.0)
                vScreenY[A] = static_cast<int>(floor(static_cast<double>(vScreenY[A] / 32))) * 32 - 8;
            if(std::fmod(vScreenX[A], 32) != 0.0)
                vScreenX[A] = static_cast<int>(floor(static_cast<double>(vScreenX[A] / 32))) * 32;
        }

        // 16x16 alignment
        if(
            (EditorCursor.Mode == OptCursor_t::LVL_BLOCKS &&
            (EditorCursor.Block.Type == 534 || EditorCursor.Block.Type == 535 ||
             EditorCursor.Block.Type == 536 || EditorCursor.Block.Type == 537)) ||

             EditorCursor.Mode == OptCursor_t::LVL_WARPS ||

            (EditorCursor.Mode == OptCursor_t::LVL_NPCS &&
             EditorCursor.NPC.Type == 52) ||

            (EditorCursor.Mode == OptCursor_t::LVL_BGOS &&
             (EditorCursor.Background.Type == 71 ||
              EditorCursor.Background.Type == 72 ||
              EditorCursor.Background.Type == 73 ||
              EditorCursor.Background.Type == 141 ||
              EditorCursor.Background.Type == 74 ||
              EditorCursor.Background.Type == 70 ||
              EditorCursor.Background.Type == 100)) ||

              (EditorCursor.Mode == OptCursor_t::LVL_NPCS &&
               (EditorCursor.NPC.Generator ||
                EditorCursor.NPC.Type == 209 || EditorCursor.NPC.Type == 256 ||
                EditorCursor.NPC.Type == 257 || EditorCursor.NPC.Type == 260))
        )
        {
            if(!(fEqual(EditorCursor.Location.X, double(std::floor(X / 16)) * 16 - vScreenX[A]) &&
                 fEqual(EditorCursor.Location.Y + 8, double(std::floor(Y / 16)) * 16 - vScreenY[A])) )
            {
                EditorCursor.Location.X = double(std::floor(X / 16)) * 16 - vScreenX[A];
                EditorCursor.Location.Y = double(std::floor(Y / 16)) * 16 - vScreenY[A];
                EditorCursor.Location.Y = EditorCursor.Location.Y - 8;
                PositionCursor();
            }
        }
        else if(EditorCursor.Mode == OptCursor_t::LVL_SETTINGS)
        {
            if(EditorCursor.SubMode < 4)
            {
                EditorCursor.Location.X = X - vScreenX[A];
                EditorCursor.Location.Y = Y - vScreenY[A];
            }
            else if(!(EditorCursor.Location.X == static_cast<float>(floor(X / 8)) * 8 - vScreenX[A] && EditorCursor.Location.Y + 8 == static_cast<float>(floor(Y / 8)) * 8 - vScreenY[A]))
            {
                EditorCursor.Location.X = static_cast<float>(floor(X / 8)) * 8 - vScreenX[A];
                EditorCursor.Location.Y = static_cast<float>(floor(Y / 8)) * 8 - vScreenY[A];
                EditorCursor.Location.Y = EditorCursor.Location.Y - 8;
                PositionCursor();
            }
        }
        else if(EditorCursor.Mode == 8)
        {
            EditorCursor.Location.X = double(std::floor(X / 16)) * 16 - vScreenX[A];
            EditorCursor.Location.Y = double(std::floor(Y / 16)) * 16 - vScreenY[A];
            EditorCursor.Location.Y = EditorCursor.Location.Y - 8;
            PositionCursor();
        }
        else if(EditorCursor.Mode == OptCursor_t::LVL_WATER)
        {
            EditorCursor.Location.X = double(std::floor(X / 16)) * 16 - vScreenX[A];
            EditorCursor.Location.Y = double(std::floor(Y / 16)) * 16 - vScreenY[A];
            EditorCursor.Location.Y = EditorCursor.Location.Y - 8;
            PositionCursor();
        }
        else // Everything also align as 32x32
        {
            EditorCursor.Location.X = double(std::floor(X / 32)) * 32 - vScreenX[A];
            EditorCursor.Location.Y = double(std::floor(Y / 32)) * 32 - vScreenY[A];
            EditorCursor.Location.Y = EditorCursor.Location.Y - 8;
            PositionCursor();
        }
    }
    EditorCursor.Location.X = EditorCursor.Location.X - vScreen[A].Left;
    EditorCursor.Location.Y = EditorCursor.Location.Y - vScreen[A].Top;
//    if(nPlay.Online == true && nCur == true)
//    {
//        if(nPlay.Mode == 0)
//            Netplay::sendData "f" + std::to_string(X) - vScreenX(A) + "|" + std::to_string(Y) - vScreenY(A); // Netplay
//        else
//        {
//            SetCursor();
//            Netplay::sendData std::string("f") + "0|" + std::to_string(X) - vScreenX(A) + "|" + std::to_string(Y) - vScreenY(A); // Netplay
//        }
//    }
}

void ResetNPC(int A)
{
    NPC_t blankNPC;
    NPC[0] = blankNPC;
    NPC[0].Frame = EditorNPCFrame(A, EditorCursor.NPC.Direction);
    EditorCursor.NPC.Frame = NPC[0].Frame;
    EditorCursor.NPC.FrameCount = NPC[0].FrameCount;
    NPC[0].Frame = 0;
}

void BlockFill(Location_t Loc)
{
    bool tempBool = false;
    Location_t tempLoc;
    int A = 0;

    if(Loc.X >= level[curSection].X - 30)
    {
        if(Loc.Y >= level[curSection].Y - 30)
        {
            if(Loc.X + Loc.Width <= level[curSection].Width + 30)
            {
                if(Loc.Y + Loc.Height <= level[curSection].Height + 30)
                {
                    tempBool = true;
                    for(A = 1; A <= numBlock; A++)
                    {
                        if(!Block[A].Hidden)
                        {
                            if(CursorCollision(Loc, Block[A].Location))
                            {
                                tempBool = false;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    if(tempBool)
    {
        numBlock++;
        Block[numBlock] = EditorCursor.Block;
        Block[numBlock].DefaultType = Block[numBlock].Type;
        Block[numBlock].DefaultSpecial = Block[numBlock].Special;
        Block[numBlock].Location = Loc;
        tempLoc = Loc;
        tempLoc.X = tempLoc.X - Loc.Width;
        BlockFill(tempLoc); // left
        tempLoc = Loc;
        tempLoc.X = tempLoc.X + Loc.Width;
        BlockFill(tempLoc); // right
        tempLoc = Loc;
        tempLoc.Y = tempLoc.Y - Loc.Height;
        BlockFill(tempLoc); // top
        tempLoc = Loc;
        tempLoc.Y = tempLoc.Y + Loc.Height;
        BlockFill(tempLoc); // bottom
    }
}

void OptCursorSync()
{
//    On Error Resume Next
//    Dim A As Integer
//    int A = 0;
//    EditorCursor.Selected = 0
    EditorCursor.Selected = 0;
//    EditorCursor.SelectedMode = 0
    EditorCursor.SelectedMode = 0;
//    For A = 1 To optCursor.Count
//        If Index <> A Then optCursor(A).Value = False
//    Next A

//    If Index = 1 Then
    if(optCursor.current == 1)
    {
//        frmBlocks.Show
//        frmBlocks.FocusNinja.SetFocus
//    Else
    }
//        frmBlocks.Hide
//        frmAdvancedBlock.Hide
//    End If


//    If Index = 2 Then
    if(optCursor.current == 2)
    {
//        frmLevelSettings.optLevel(0).Value = False
//        frmLevelSettings.optLevel(1).Value = False
//        frmLevelSettings.optLevel(2).Value = False
//        frmLevelSettings.optLevel(3).Value = False
//        frmLevelSettings.optLevel(4).Value = False
//        frmLevelSettings.optLevel(5).Value = False
//        frmLevelSettings.Show
//        frmLevelSettings.FocusNinja.SetFocus
//    Else
    }
//        frmLevelSettings.Hide
//    End If


//    If Index = 3 Then
    if(optCursor.current == 3)
    {
//        frmBackgrounds.Show
//        frmBackgrounds.FocusNinja.SetFocus
//    Else
    }
//        frmBackgrounds.Hide
//    End If


//    If Index = 4 Then
    if(optCursor.current == 4)
    {
//        frmNPCs.Show
//        frmNPCs.FocusNinja.SetFocus
    }
//    Else
//        frmNPCs.Hide
//        Unload frmGenerator
//        Unload frmAdvanced
//        Unload frmNPCAdvanced
//    End If


//    If Index = 5 Then
//        frmWarp.Show
//        frmWarp.FocusNinja.SetFocus
//    Else
//        frmWarp.Hide
//    End If


//    If Index = 7 Then
//        frmTiles.Show
//        frmTiles.FocusNinja.SetFocus
//    Else
//        frmTiles.Hide
//    End If


//    If Index = 8 Then
//        frmScene.Show
//        frmScene.FocusNinja.SetFocus
//    Else
//        frmScene.Hide
//    End If


//    If Index = 9 Then
//        frmLevels.Show
//        frmLevels.FocusNinja.SetFocus
//    Else
//        frmLevels.Hide
//    End If


//    If Index = 10 Then
//        frmPaths.Show
//        frmPaths.FocusNinja.SetFocus
//    Else
//        frmPaths.Hide
//    End If
//    If Index = 11 Then
//        frmMusic.Show
//        frmMusic.FocusNinja.SetFocus
//    Else
//        frmMusic.Hide
//    End If


//    If Index = 12 Then
//        frmWorld.Show
//        frmWorld.FocusNinja.SetFocus
//    Else
//        frmWorld.Hide
//    End If


//    If Index = 15 Then
//        frmWater.Show
//        'frmWater.FocusNinja.SetFocus
//    Else
//        frmWater.Hide
//    End If


//    If Index = 0 Or Index = 6 Or Index = 13 Or Index = 14 Then
//        frmLevelWindow.SetFocus
//    End If
}
