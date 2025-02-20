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

#include "../globals.h"
#include "../frame_timer.h"
#include "../graphics.h"
#include "../collision.h"
#include "../player.h"
#include "../main/speedrunner.h"
#include "../main/trees.h"


// draws GFX to screen when on the world map/world map editor
void UpdateGraphics2(bool skipRepaint)
{
    if(!GameIsActive)
        return;

#ifdef __ANDROID__
    if(frmMain.renderBlocked())
        return;
#endif

    float c = ShadowMode ? 0.f : 1.f;
    cycleNextInc();

    if(FrameSkip && frameSkipNeeded())
        return;

    frmMain.setTargetTexture();

    frameNextInc();
    frameRenderStart();

    g_stats.reset();

    int A = 0;
    int B = 0;
    int Z = 0;
    int WPHeight = 0;
//    Location_t tempLocation;
    Z = 1;

    vScreen[Z].Left = 0;
    vScreen[Z].Top = 0;
    vScreen[Z].Width = ScreenW;
    vScreen[Z].Height = ScreenH;
    CenterScreens();

    SpecialFrames();
    SceneFrame2[1] = SceneFrame2[1] + 1;
    if(SceneFrame2[1] >= 12)
    {
        SceneFrame2[1] = 0;
        SceneFrame[1] = SceneFrame[1] + 1;
        if(SceneFrame[1] >= 4)
            SceneFrame[1] = 0;
        SceneFrame[4] = SceneFrame[1];
        SceneFrame[5] = SceneFrame[1];
        SceneFrame[6] = SceneFrame[1];
        SceneFrame[9] = SceneFrame[1];
        SceneFrame[10] = SceneFrame[1];
        SceneFrame[12] = SceneFrame[1];
        SceneFrame[51] = SceneFrame[1];
        SceneFrame[52] = SceneFrame[1];
        SceneFrame[53] = SceneFrame[1];
        SceneFrame[54] = SceneFrame[1];
        SceneFrame[55] = SceneFrame[1];
    }
    SceneFrame2[27] = SceneFrame2[27] + 1;
    if(SceneFrame2[27] >= 8)
    {
        SceneFrame2[27] = 0;
        SceneFrame[27] = SceneFrame[27] + 1;
        if(SceneFrame[27] >= 12)
            SceneFrame[27] = 0;
        SceneFrame[28] = SceneFrame[27];
        SceneFrame[29] = SceneFrame[27];
        SceneFrame[30] = SceneFrame[27];
    }
    SceneFrame2[33] = SceneFrame2[33] + 1;
    if(SceneFrame2[33] >= 4)
    {
        SceneFrame2[33] = 0;
        SceneFrame[33] = SceneFrame[27] + 1;
        if(SceneFrame[33] >= 14)
            SceneFrame[33] = 0;
        SceneFrame[34] = SceneFrame[33];
    }
    SceneFrame2[62] = SceneFrame2[62] + 1;
    if(SceneFrame2[62] >= 8)
    {
        SceneFrame2[62] = 0;
        SceneFrame[62] = SceneFrame[62] + 1;
        if(SceneFrame[62] >= 8)
            SceneFrame[62] = 0;
        SceneFrame[63] = SceneFrame[62];
    }
    LevelFrame2[2] = LevelFrame2[2] + 1;
    if(LevelFrame2[2] >= 6)
    {
        LevelFrame2[2] = 0;
        LevelFrame[2] = LevelFrame[2] + 1;
        if(LevelFrame[2] >= 6)
            LevelFrame[2] = 0;
        LevelFrame[9] = LevelFrame[2];
        LevelFrame[13] = LevelFrame[2];
        LevelFrame[14] = LevelFrame[2];
        LevelFrame[15] = LevelFrame[2];
        LevelFrame[31] = LevelFrame[2];
        LevelFrame[32] = LevelFrame[2];
    }
    LevelFrame2[8] = LevelFrame2[8] + 1;
    if(LevelFrame2[8] >= 12)
    {
        LevelFrame2[8] = 0;
        LevelFrame[8] = LevelFrame[8] + 1;
        if(LevelFrame[8] >= 4)
            LevelFrame[8] = 0;
    }
    LevelFrame2[12] = LevelFrame2[12] + 1;
    if(LevelFrame2[12] >= 8)
    {
        LevelFrame2[12] = 0;
        LevelFrame[12] = LevelFrame[12] + 1;
        if(LevelFrame[12] >= 2)
            LevelFrame[12] = 0;
    }
    LevelFrame2[25] = LevelFrame2[25] + 1;
    if(LevelFrame2[25] >= 8)
    {
        LevelFrame2[25] = 0;
        LevelFrame[25] = LevelFrame[25] + 1;
        if(LevelFrame[25] >= 4)
            LevelFrame[25] = 0;
        LevelFrame[26] = LevelFrame[25];
    }
    TileFrame2[14] = TileFrame2[14] + 1;
    if(TileFrame2[14] >= 14)
    {
        TileFrame2[14] = 0;
        TileFrame[14] = TileFrame[14] + 1;
        if(TileFrame[14] >= 4)
            TileFrame[14] = 0;
        TileFrame[27] = TileFrame[14];
        TileFrame[241] = TileFrame[14];
    }

#ifdef __3DS__
    frmMain.initDraw();
#else
    frmMain.clearBuffer();
#endif

    frmMain.setViewport(vScreen[Z].ScreenLeft, vScreen[Z].ScreenTop,
        vScreen[Z].Width, vScreen[Z].Height);

//    if(TakeScreen == true)
//    {
//        if(LevelEditor == true || MagicHand == true)
//            frmLevelWindow::vScreen[1].AutoRedraw = true;
//        else
//            frmMain.AutoRedraw = true;
//    }

//    If LevelEditor = True Then
//        For A = 1 To numTiles
//            With Tile(A)
//                If vScreenCollision(1, .Location) = True Then
//                    BitBlt myBackBuffer, vScreenX(Z) + .Location.X, vScreenY(Z) + .Location.Y, .Location.Width, .Location.Height, GFXTile(.Type), 0, TileHeight(.Type) * TileFrame(.Type), vbSrcCopy
//                End If
//            End With
//        Next A
//        For A = 1 To numScenes
//            With Scene(A)
//                If vScreenCollision(1, .Location) = True Then
//                    BitBlt myBackBuffer, vScreenX(Z) + .Location.X, vScreenY(Z) + .Location.Y, .Location.Width, .Location.Height, GFXSceneMask(.Type), 0, SceneHeight(.Type) * SceneFrame(.Type), vbSrcAnd
//                    BitBlt myBackBuffer, vScreenX(Z) + .Location.X, vScreenY(Z) + .Location.Y, .Location.Width, .Location.Height, GFXScene(.Type), 0, SceneHeight(.Type) * SceneFrame(.Type), vbSrcPaint
//                End If
//            End With
//        Next A
//        For A = 1 To numWorldPaths
//            With WorldPath(A)
//                If vScreenCollision(1, .Location) = True Then
//                    BitBlt myBackBuffer, vScreenX(Z) + .Location.X, vScreenY(Z) + .Location.Y, .Location.Width, .Location.Height, GFXPathMask(.Type), 0, 0, vbSrcAnd
//                    BitBlt myBackBuffer, vScreenX(Z) + .Location.X, vScreenY(Z) + .Location.Y, .Location.Width, .Location.Height, GFXPath(.Type), 0, 0, vbSrcPaint
//                End If
//            End With
//        Next A
//        For A = 1 To numWorldLevels
//            With WorldLevel(A)
//                If vScreenCollision(1, .Location) = True Then
//                    If .Path = True Then
//                        BitBlt myBackBuffer, vScreenX(Z) + .Location.X, vScreenY(Z) + .Location.Y, .Location.Width, .Location.Height, GFXLevelMask(0), 0, 0, vbSrcAnd
//                        BitBlt myBackBuffer, vScreenX(Z) + .Location.X, vScreenY(Z) + .Location.Y, .Location.Width, .Location.Height, GFXLevel(0), 0, 0, vbSrcPaint
//                    End If
//                    If .Path2 = True Then
//                        BitBlt myBackBuffer, vScreenX(Z) + .Location.X - 16, vScreenY(Z) + 8 + .Location.Y, 64, 32, GFXLevelMask(29), 0, 0, vbSrcAnd
//                        BitBlt myBackBuffer, vScreenX(Z) + .Location.X - 16, vScreenY(Z) + 8 + .Location.Y, 64, 32, GFXLevel(29), 0, 0, vbSrcPaint
//                    End If
//                    If GFXLevelBig(.Type) = True Then
//                        BitBlt myBackBuffer, vScreenX(Z) + .Location.X - (GFXLevelWidth(.Type) - 32) / 2, vScreenY(Z) + .Location.Y - GFXLevelHeight(.Type) + 32, GFXLevelWidth(.Type), GFXLevelHeight(.Type), GFXLevelMask(.Type), 0, 32 * LevelFrame(.Type), vbSrcAnd
//                        BitBlt myBackBuffer, vScreenX(Z) + .Location.X - (GFXLevelWidth(.Type) - 32) / 2, vScreenY(Z) + .Location.Y - GFXLevelHeight(.Type) + 32, GFXLevelWidth(.Type), GFXLevelHeight(.Type), GFXLevel(.Type), 0, 32 * LevelFrame(.Type), vbSrcPaint
//                    Else
//                        BitBlt myBackBuffer, vScreenX(Z) + .Location.X, vScreenY(Z) + .Location.Y, .Location.Width, .Location.Height, GFXLevelMask(.Type), 0, 32 * LevelFrame(.Type), vbSrcAnd
//                        BitBlt myBackBuffer, vScreenX(Z) + .Location.X, vScreenY(Z) + .Location.Y, .Location.Width, .Location.Height, GFXLevel(.Type), 0, 32 * LevelFrame(.Type), vbSrcPaint
//                    End If
//                End If
//            End With
//        Next A
//    Else
    double sLeft, sTop, sRight, sBottom, sW, sH;
    double margin, marginTop;
    {
#ifdef __3DS__
        frmMain.setLayer(0);
#endif

        sW = vScreen[Z].Width;
        sH = vScreen[Z].Height;
        
        margin = 66;
        marginTop = 130;

        if(sH < 400)
        {
            margin = 24;
            marginTop = 72;
        }
        else if(sH < 500)
        {
            margin = 32;
            marginTop = 96;
        }
        else if(sW < 500)
            margin = 32;

        if (WorldEditor)
        {
            sLeft = -vScreenX[1];
            sTop = -vScreenY[1];
            sRight = -vScreenX[1] + vScreen[1].Width;
            sBottom = -vScreenY[1] + vScreen[1].Height;
        }
        else
        {
            sLeft = -vScreenX[1] + margin - 2;
            sTop = -vScreenY[1] + (marginTop - 34);
            sRight = -vScreenX[1] + vScreen[1].Width - (margin - 2);
            sBottom = -vScreenY[1] + vScreen[1].Height - (margin - 2);
        }

        Location_t sView;
        sView.X = sLeft;
        sView.Y = sTop;
        sView.Width = sRight - sLeft;
        sView.Height = sBottom - sTop;

        for(Tile_t* t : treeWorldTileQuery(sLeft, sTop, sRight, sBottom, true))
        {
            Tile_t &tile = *t;
            SDL_assert(IF_INRANGE(tile.Type, 1, maxTileType));

            g_stats.checkedTiles++;
            if(CheckCollision(sView, tile.Location))
            {
                g_stats.renderedTiles++;
//                frmMain.renderTexture(vScreenX[Z] + Tile[A].Location.X, vScreenY[Z] + Tile[A].Location.Y, Tile[A].Location.Width, Tile[A].Location.Height, GFXTile[Tile[A].Type], 0, TileHeight[Tile[A].Type] * TileFrame[Tile[A].Type]);
                frmMain.renderTexture(vScreenX[Z] + tile.Location.X,
                                      vScreenY[Z] + tile.Location.Y,
                                      tile.Location.Width,
                                      tile.Location.Height,
                                      GFXTileBMP[tile.Type], 0, TileHeight[tile.Type] * TileFrame[tile.Type]);
            }
        }

        for(Scene_t* t : treeWorldSceneQuery(sLeft, sTop, sRight, sBottom, true))
        {
            Scene_t &scene = *t;
            SDL_assert(IF_INRANGE(scene.Type, 1, maxSceneType));

            g_stats.checkedScenes++;
            if(CheckCollision(sView, scene.Location) && (WorldEditor || scene.Active))
            {
                g_stats.renderedScenes++;
//                frmMain.renderTexture(vScreenX[Z] + scene.Location.X, vScreenY[Z] + scene.Location.Y, scene.Location.Width, scene.Location.Height, GFXSceneMask[scene.Type], 0, SceneHeight[scene.Type] * SceneFrame[scene.Type]);
//                frmMain.renderTexture(vScreenX[Z] + scene.Location.X, vScreenY[Z] + scene.Location.Y, scene.Location.Width, scene.Location.Height, GFXScene[scene.Type], 0, SceneHeight[scene.Type] * SceneFrame[scene.Type]);
                frmMain.renderTexture(vScreenX[Z] + scene.Location.X,
                                      vScreenY[Z] + scene.Location.Y,
                                      scene.Location.Width, scene.Location.Height,
                                      GFXSceneBMP[scene.Type], 0, SceneHeight[scene.Type] * SceneFrame[scene.Type]);
            }
        }

        for(WorldPath_t* t : treeWorldPathQuery(sLeft, sTop, sRight, sBottom, true))
        {
            WorldPath_t &path = *t;
            SDL_assert(IF_INRANGE(path.Type, 1, maxPathType));

            g_stats.checkedPaths++;
            if(CheckCollision(sView, path.Location) && (WorldEditor || path.Active))
            {
                g_stats.renderedPaths++;
//                frmMain.renderTexture(vScreenX[Z] + path.Location.X, vScreenY[Z] + path.Location.Y, path.Location.Width, path.Location.Height, GFXPathMask[path.Type], 0, 0);
//                frmMain.renderTexture(vScreenX[Z] + path.Location.X, vScreenY[Z] + path.Location.Y, path.Location.Width, path.Location.Height, GFXPath[path.Type], 0, 0);
                frmMain.renderTexture(vScreenX[Z] + path.Location.X,
                                      vScreenY[Z] + path.Location.Y,
                                      path.Location.Width, path.Location.Height,
                                      GFXPathBMP[path.Type], 0, 0);
            }
        }

        for(WorldLevel_t* t : treeWorldLevelQuery(sLeft, sTop, sRight, sBottom, true))
        {
            WorldLevel_t &level = *t;
            SDL_assert(IF_INRANGE(level.Type, 0, maxLevelType));

            g_stats.checkedLevels++;
            if(CheckCollision(sView, level.Location) && (WorldEditor || level.Active))
            {
                g_stats.renderedLevels++;
                if(level.Path)
                {
                    frmMain.renderTexture(vScreenX[Z] + level.Location.X,
                                          vScreenY[Z] + level.Location.Y,
                                          level.Location.Width,
                                          level.Location.Height,
                                          GFXLevelBMP[0], 0, 0);
                }
                if(level.Path2)
                {
                    frmMain.renderTexture(vScreenX[Z] + level.Location.X - 16,
                                          vScreenY[Z] + 8 + level.Location.Y,
                                          64, 32,
                                          GFXLevelBMP[29], 0, 0);
                }
                if(GFXLevelBig[level.Type])
                {
                    frmMain.renderTexture(vScreenX[Z] + level.Location.X - (GFXLevelWidth[level.Type] - 32) / 2.0,
                                          vScreenY[Z] + level.Location.Y - GFXLevelHeight[level.Type] + 32,
                                          GFXLevelWidth[level.Type], GFXLevelHeight[level.Type],
                                          GFXLevelBMP[level.Type], 0, 32 * LevelFrame[level.Type]);
                }
                else
                {
                    frmMain.renderTexture(vScreenX[Z] + level.Location.X,
                                          vScreenY[Z] + level.Location.Y,
                                          level.Location.Width, level.Location.Height,
                                          GFXLevelBMP[level.Type], 0, 32 * LevelFrame[level.Type]);
                }
            }
        }
    }
    if (WorldEditor)
    {
        for (A = 1; A <= numEffects; A++)
        {
            if (vScreenCollision(Z, Effect[A].Location))
            {
                frmMain.renderTexture(vScreenX[Z] + Effect[A].Location.X,
                    vScreenY[Z] + Effect[A].Location.Y,
                    Effect[A].Location.Width, Effect[A].Location.Height,
                    GFXEffect[Effect[A].Type], 0, Effect[A].Frame * EffectHeight[Effect[A].Type]);
            }
        }
        //for(A = 1; A <= numWorldLevels; A++)
        for(WorldMusic_t* t : treeWorldMusicQuery(sLeft, sTop, sRight, sBottom, true))
        {
            WorldMusic_t &music = *t;
            if (vScreenCollision(Z, music.Location))
            {
                frmMain.renderRect(vScreenX[Z] + music.Location.X, vScreenY[Z] + music.Location.Y, 32, 32,
                    1.f, 0.f, 1.f, 1.f, false);
                SuperPrint(std::to_string(music.Type), 1, vScreenX[Z] + music.Location.X + 2, vScreenY[Z] + music.Location.Y + 2);
            }
        }
#ifdef __3DS__
        frmMain.setLayer(3);
#endif
        DrawEditorWorld();
    }
    else
    { // NOT AN EDITOR!!!
        if(WorldPlayer[1].Type == 0)
            WorldPlayer[1].Type = 1;
        if(Player[1].Character == 1)
            WorldPlayer[1].Type = 1;
        if(Player[1].Character == 2)
            WorldPlayer[1].Type = 2;
        if(Player[1].Character == 3)
            WorldPlayer[1].Type = 3;
        if(Player[1].Character == 4)
            WorldPlayer[1].Type = 4;
        if(Player[1].Character == 5)
            WorldPlayer[1].Type = 5;
        if(WorldPlayer[1].Type == 3)
            WPHeight = 44;
        else if(WorldPlayer[1].Type == 4)
            WPHeight = 40;
        else
            WPHeight = 32;
//        frmMain.renderTexture(vScreenX[Z] + WorldPlayer[1].Location.X, vScreenY[Z] + WorldPlayer[1].Location.Y - 10 + WorldPlayer[1].Location.Height - WPHeight, WorldPlayer[1].Location.Width, WPHeight, GFXPlayerMask[WorldPlayer[1].Type], 0, WPHeight * WorldPlayer[1].Frame);
//        frmMain.renderTexture(vScreenX[Z] + WorldPlayer[1].Location.X, vScreenY[Z] + WorldPlayer[1].Location.Y - 10 + WorldPlayer[1].Location.Height - WPHeight, WorldPlayer[1].Location.Width, WPHeight, GFXPlayer[WorldPlayer[1].Type], 0, WPHeight * WorldPlayer[1].Frame);
        frmMain.renderTexture(vScreenX[Z] + WorldPlayer[1].Location.X,
                              vScreenY[Z] + WorldPlayer[1].Location.Y - 10 + WorldPlayer[1].Location.Height - WPHeight,
                              WorldPlayer[1].Location.Width, WPHeight,
                              GFXPlayerBMP[WorldPlayer[1].Type], 0, WPHeight * WorldPlayer[1].Frame);

//        frmMain.renderTexture(0, 0, 800, 130, GFX.Interface[4], 0, 0);
#ifdef __3DS__
        frmMain.setLayer(2);
#endif
        // render background, in MANY careful segments...

        // top-left
        frmMain.renderTexture(0, 0, margin, marginTop, GFX.Interface[4], 66-margin, 130-marginTop);
        // top
        A = GFX.Interface[4].w-66-66;
        for (B = 0; B < (sW-margin*2)/A+1; B++)
            frmMain.renderTexture(margin+B*A, 0, A, marginTop, GFX.Interface[4], 66, 130-marginTop);
        // top-right
        frmMain.renderTexture(sW-margin, 0, margin, marginTop+20, GFX.Interface[4], GFX.Interface[4].w-66, 130-marginTop);
        // left
        A = GFX.Interface[4].h-130-66;
        for (B = 0; B < (sH-marginTop-margin)/A+1; B++)
            frmMain.renderTexture(0, marginTop+B*A, margin, A, GFX.Interface[4], 66-margin, 130);
        // right
        A = GFX.Interface[4].h-(130+20)-66;
        for (B = 0; B < (sH-(marginTop+20)-margin)/A+1; B++)
            frmMain.renderTexture(sW-margin, (marginTop+20)+B*A, margin, A, GFX.Interface[4], GFX.Interface[4].w-66, 150);
        // bottom-left
        frmMain.renderTexture(0, sH-margin, margin+34, margin, GFX.Interface[4], 66-margin, GFX.Interface[4].h-66);
        // bottom
        A = GFX.Interface[4].w-100-66;
        for (B = 0; B < (sW-(margin+34)-margin)/A+1; B++)
            frmMain.renderTexture((margin+34)+B*A, sH-margin, A, margin, GFX.Interface[4], 100, GFX.Interface[4].h-66);
        // bottom-right
        frmMain.renderTexture(sW-margin, sH-margin, margin, margin, GFX.Interface[4], GFX.Interface[4].w-66, GFX.Interface[4].h-66);

#ifdef __3DS__
        frmMain.setLayer(3);
#endif

        int pX, pY;
        pY = marginTop - 6;

        for(A = 1; A <= numPlayers; A++)
        {
            pX = margin * 1.5 - 64 + 48 * A;

            Player[A].Direction = -1;
            Player[A].Location.SpeedY = 0;
            Player[A].Location.SpeedX = -1;
            Player[A].Controls.Left = false;
            Player[A].Controls.Right = false;
            if(Player[A].Duck)
                UnDuck(A);
            PlayerFrame(A);

            if(Player[A].Mount == 3)
            {
                if(Player[A].MountType == 0)
                    Player[A].MountType = 1;
                B = Player[A].MountType;
                // Yoshi's Body
                frmMain.renderTexture(pX + Player[A].YoshiBX, pY - Player[A].Location.Height + Player[A].YoshiBY,
                                      32, 32, GFXYoshiBBMP[B], 0, 32 * Player[A].YoshiBFrame, c, c, c);

                // Yoshi's Head
                frmMain.renderTexture(pX + Player[A].YoshiTX,
                                      pY - Player[A].Location.Height + Player[A].YoshiTY,
                                      32, 32, GFXYoshiTBMP[B], 0, 32 * Player[A].YoshiTFrame, c, c, c);
            }

            if(Player[A].Character == 1)
            {
                if(Player[A].Mount == 0 || Player[A].Mount == 3)
                {
                    frmMain.renderTexture(pX + MarioFrameX[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)],
                                          pY - Player[A].Location.Height + MarioFrameY[(Player[A].State * 100) +
                                            (Player[A].Frame * Player[A].Direction)] + Player[A].MountOffsetY,
                                          99, 99, GFXMarioBMP[Player[A].State], pfrX(100 + Player[A].Frame * Player[A].Direction),
                                          pfrY(100 + Player[A].Frame * Player[A].Direction), c, c, c);
                }
                else if(Player[A].Mount == 1)
                {
                    int height;
                    if(Player[A].State == 1)
                        height = Player[A].Location.Height - MarioFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)];
                    else
                        height = Player[A].Location.Height - MarioFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)] - 30;
                    frmMain.renderTexture(pX + MarioFrameX[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)],
                                          pY - Player[A].Location.Height + MarioFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)],
                                          99,
                                          height,
                                          GFXMarioBMP[Player[A].State],
                                          pfrX(100 + Player[A].Frame * Player[A].Direction),
                                          pfrY(100 + Player[A].Frame * Player[A].Direction), c, c, c);

                    frmMain.renderTexture(pX + Player[A].Location.Width / 2.0 - 16, pY - 30, 32, 32,
                                          GFX.Boot[Player[A].MountType], 0, 32 * Player[A].MountFrame, c, c, c);

                    if(Player[A].MountType == 3)
                    {
                        Player[A].YoshiWingsFrameCount = Player[A].YoshiWingsFrameCount + 1;
                        Player[A].YoshiWingsFrame = 0;
                        if(Player[A].YoshiWingsFrameCount <= 12)
                            Player[A].YoshiWingsFrame = 1;
                        else if(Player[A].YoshiWingsFrameCount >= 24)
                            Player[A].YoshiWingsFrameCount = 0;

                        frmMain.renderTexture(pX + Player[A].Location.Width / 2.0 - 16 + 20, pY - 30 - 10, 32, 32,
                                              GFX.YoshiWings, 0, 0 + 32 * Player[A].YoshiWingsFrame);
                    }
                }
            }
            else if(Player[A].Character == 2)
            {
                if(Player[A].Mount == 0 || Player[A].Mount == 3)
                {
                    frmMain.renderTexture(pX + LuigiFrameX[(Player[A].State * 100) +
                                            (Player[A].Frame * Player[A].Direction)],
                                          pY - Player[A].Location.Height + LuigiFrameY[(Player[A].State * 100) +
                                            (Player[A].Frame * Player[A].Direction)] + Player[A].MountOffsetY, 99, 99,
                                          GFXLuigiBMP[Player[A].State], pfrX(100 + Player[A].Frame * Player[A].Direction),
                                          pfrY(100 + Player[A].Frame * Player[A].Direction), c, c, c);
                }
                else if(Player[A].Mount == 1)
                {
                    int height;
                    if(Player[A].State == 1)
                        height = Player[A].Location.Height - LuigiFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)];
                    else
                        height = Player[A].Location.Height - LuigiFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)] - 30;
                    frmMain.renderTexture(pX + LuigiFrameX[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)],
                                          pY - Player[A].Location.Height + LuigiFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)],
                                          99,
                                          height,
                                          GFXLuigiBMP[Player[A].State],
                                          pfrX(100 + Player[A].Frame * Player[A].Direction),
                                          pfrY(100 + Player[A].Frame * Player[A].Direction), c, c, c);

                    frmMain.renderTexture(pX + Player[A].Location.Width / 2.0 - 16, pY - 30, 32, 32, GFX.Boot[Player[A].MountType], 0, 32 * Player[A].MountFrame, c, c, c);

                    if(Player[A].MountType == 3)
                    {
                        Player[A].YoshiWingsFrameCount = Player[A].YoshiWingsFrameCount + 1;
                        Player[A].YoshiWingsFrame = 0;
                        if(Player[A].YoshiWingsFrameCount <= 12)
                            Player[A].YoshiWingsFrame = 1;
                        else if(Player[A].YoshiWingsFrameCount >= 24)
                            Player[A].YoshiWingsFrameCount = 0;

                        frmMain.renderTexture(pX + Player[A].Location.Width / 2.0 - 16 + 20, pY - 30 - 10, 32, 32, GFX.YoshiWings, 0, 0 + 32 * Player[A].YoshiWingsFrame, c, c, c);
                    }
                }
            }
            else if(Player[A].Character == 3)
            {
                if(Player[A].Mount == 0 || Player[A].Mount == 3)
                {
                    frmMain.renderTexture(pX + PeachFrameX[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)],
                                          pY - Player[A].Location.Height + PeachFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)] + Player[A].MountOffsetY,
                                          99, 99,
                                          GFXPeachBMP[Player[A].State],
                                          pfrX(100 + Player[A].Frame * Player[A].Direction),
                                          pfrY(100 + Player[A].Frame * Player[A].Direction), c, c, c);
                }
                else if(Player[A].Mount == 1)
                {
                    int height;
                    if(Player[A].State == 1)
                        height = Player[A].Location.Height - PeachFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)];
                    else
                        height = Player[A].Location.Height - PeachFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)] - 30;
                    frmMain.renderTexture(pX + PeachFrameX[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)],
                                          pY - Player[A].Location.Height + PeachFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)],
                                          99,
                                          height,
                                          GFXPeachBMP[Player[A].State],
                                          pfrX(100 + Player[A].Frame * Player[A].Direction),
                                          pfrY(100 + Player[A].Frame * Player[A].Direction), c, c, c);

                    frmMain.renderTexture(pX + Player[A].Location.Width / 2.0 - 16, pY - 30, 32, 32, GFX.Boot[Player[A].MountType], 0, 32 * Player[A].MountFrame, c, c, c);

                    if(Player[A].MountType == 3)
                    {
                        Player[A].YoshiWingsFrameCount = Player[A].YoshiWingsFrameCount + 1;
                        Player[A].YoshiWingsFrame = 0;
                        if(Player[A].YoshiWingsFrameCount <= 12)
                            Player[A].YoshiWingsFrame = 1;
                        else if(Player[A].YoshiWingsFrameCount >= 24)
                            Player[A].YoshiWingsFrameCount = 0;

                        frmMain.renderTexture(pX + Player[A].Location.Width / 2.0 - 16 + 20, pY - 30 - 10, 32, 32, GFX.YoshiWings, 0, 0 + 32 * Player[A].YoshiWingsFrame, c, c, c);
                    }
                }
            }
            else if(Player[A].Character == 4)
            {
                if(Player[A].Mount == 0 || Player[A].Mount == 3)
                {
                    frmMain.renderTexture(pX + ToadFrameX[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)], pY - Player[A].Location.Height + ToadFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)] + Player[A].MountOffsetY, 99, 99, GFXToadBMP[Player[A].State], pfrX(100 + Player[A].Frame * Player[A].Direction), pfrY(100 + Player[A].Frame * Player[A].Direction), c, c, c);
                }
                else if(Player[A].Mount == 1)
                {
                    if(Player[A].State == 1)
                    {
                        frmMain.renderTexture(pX + ToadFrameX[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)], 6 + pY - Player[A].Location.Height + ToadFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)], 99, Player[A].Location.Height - 24, GFXToadBMP[Player[A].State], pfrX(100 + Player[A].Frame * Player[A].Direction), pfrY(100 + Player[A].Frame * Player[A].Direction), c, c, c);
                    }
                    else
                    {
                        int height;
                        if(Player[A].State == 1)
                            height = Player[A].Location.Height - ToadFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)];
                        else
                            height = Player[A].Location.Height - ToadFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)] - 26;
                        frmMain.renderTexture(pX + ToadFrameX[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)],
                                              pY - Player[A].Location.Height + ToadFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)],
                                              99,
                                              height,
                                              GFXToadBMP[Player[A].State], pfrX(100 + Player[A].Frame * Player[A].Direction), pfrY(100 + Player[A].Frame * Player[A].Direction), c, c, c);
                    }
                    frmMain.renderTexture(pX + Player[A].Location.Width / 2.0 - 16, pY - 30, 32, 32, GFX.Boot[Player[A].MountType], 0, 32 * Player[A].MountFrame, c, c, c);

                    if(Player[A].MountType == 3)
                    {
                        Player[A].YoshiWingsFrameCount = Player[A].YoshiWingsFrameCount + 1;
                        Player[A].YoshiWingsFrame = 0;
                        if(Player[A].YoshiWingsFrameCount <= 12)
                            Player[A].YoshiWingsFrame = 1;
                        else if(Player[A].YoshiWingsFrameCount >= 24)
                            Player[A].YoshiWingsFrameCount = 0;
                        frmMain.renderTexture(pX + Player[A].Location.Width / 2.0 - 16 + 20, pY - 30 - 10, 32, 32, GFX.YoshiWings, 0, 0 + 32 * Player[A].YoshiWingsFrame, c, c, c);
                    }
                }
            }
            else if(Player[A].Character == 5)
            {
                frmMain.renderTexture(pX + LinkFrameX[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)], pY - Player[A].Location.Height + LinkFrameY[(Player[A].State * 100) + (Player[A].Frame * Player[A].Direction)], 99, 99, GFXLinkBMP[Player[A].State], pfrX(100 + Player[A].Frame * Player[A].Direction), pfrY(100 + Player[A].Frame * Player[A].Direction), c, c, c);
            }
        }
        A = numPlayers + 1;
        pX = margin * 1.5 - 64 + 48 * A;
        // Print lives on the screen
        frmMain.renderTexture(pX, marginTop - 4 - GFX.Interface[3].h, GFX.Interface[3].w, GFX.Interface[3].h, GFX.Interface[3], 0, 0);
        frmMain.renderTexture(pX + 40, marginTop - 2 - GFX.Interface[3].h, GFX.Interface[1].w, GFX.Interface[1].h, GFX.Interface[1], 0, 0);

        SuperPrint(std::to_string(int(Lives)), 1, pX + 62, marginTop-18);
        // Print coins on the screen
        if(Player[1].Character == 5)
        {
            frmMain.renderTexture(pX + 16, marginTop - 42, GFX.Interface[2].w, GFX.Interface[2].h, GFX.Interface[6], 0, 0);
        }
        else
        {
            frmMain.renderTexture(pX + 16, marginTop - 42, GFX.Interface[2].w, GFX.Interface[2].h, GFX.Interface[2], 0, 0);
        }
        frmMain.renderTexture(pX + 40, marginTop - 40, GFX.Interface[1].w, GFX.Interface[1].h, GFX.Interface[1], 0, 0);

        SuperPrint(std::to_string(Coins), 1, pX + 62, marginTop - 40);
        // Print stars on the screen
        if(numStars > 0)
        {
            frmMain.renderTexture(pX + 16, marginTop - 64, GFX.Interface[5].w, GFX.Interface[5].h, GFX.Interface[5], 0, 0);
            frmMain.renderTexture(pX + 40, marginTop - 62, GFX.Interface[1].w, GFX.Interface[1].h, GFX.Interface[1], 0, 0);
            SuperPrint(std::to_string(numStars), 1, pX + 62, marginTop - 62);
        }
        // Print the level's name
        if(WorldPlayer[1].LevelName != "")
        {
            size_t availChars = (size_t)((sW - margin - (pX + 116))/16) + 1;
            if(WorldPlayer[1].LevelName.length() > availChars*2)
            {
                SuperPrint(WorldPlayer[1].LevelName.substr(0, availChars), 2,
                    pX + 116, marginTop - 21 - 40);
                SuperPrint(WorldPlayer[1].LevelName.substr(availChars, availChars), 2,
                    pX + 116 + 16, marginTop - 21 - 20);
                SuperPrint(WorldPlayer[1].LevelName.substr(availChars*2), 2,
                    pX + 116 + 16, marginTop - 21);
            }
            else if(WorldPlayer[1].LevelName.length() > availChars)
            {
                SuperPrint(WorldPlayer[1].LevelName.substr(0, availChars), 2,
                    pX + 116, marginTop - 21 - 20);
                SuperPrint(WorldPlayer[1].LevelName.substr(availChars), 2,
                    pX + 116 + 16, marginTop - 21);
            }
            else
                SuperPrint(WorldPlayer[1].LevelName, 2, pX + 116, marginTop - 21);
        }

        frmMain.setViewport(0, 0, ScreenW, ScreenH);

        if(GamePaused == true)
        {
            frmMain.renderRect(ScreenW/2 - 190, ScreenH/2 - 100, 380, 200, 0, 0, 0);
            if(Cheater == false)
            {
                SuperPrint("CONTINUE", 3, ScreenW/2 - 190 + 62, ScreenH/2 - 100 + 57);
                SuperPrint("SAVE & CONTINUE", 3, ScreenW/2 - 190 + 62, ScreenH/2 - 100 + 92);
                SuperPrint("SAVE & QUIT", 3, ScreenW/2 - 190 + 62, ScreenH/2 - 100 + 127);
                frmMain.renderTexture(ScreenW/2 - 190 + 42, ScreenH/2 - 100 + 57 + (MenuCursor * 35), 16, 16, GFX.MCursor[0], 0, 0);
            }
            else
            {
                SuperPrint("CONTINUE", 3, ScreenW/2 - 190 + 62, ScreenH/2 - 100 + 75);
                SuperPrint("QUIT", 3, ScreenW/2 - 190 + 62, ScreenH/2 - 100 + 110);
                frmMain.renderTexture(ScreenW/2 - 190 + 42, ScreenH/2 - 100 + 75 + (MenuCursor * 35), 16, 16, GFX.MCursor[0], 0, 0);
            }
        }
    }

    if(PrintFPS > 0)
        SuperPrint(std::to_string(int(PrintFPS)), 1, 8, 8, 0.f, 1.f, 0.f);

    g_stats.print();

    speedRun_render();

    if(!skipRepaint)
        frmMain.repaint();

    frmMain.setTargetScreen();

    if(TakeScreen)
        ScreenShot();

    frameRenderEnd();

//    if(frmMain.lazyLoadedBytes() > 200000) // Reset timer while loading many pictures at the same time
//        resetFrameTimer();
    frmMain.lazyLoadedBytesReset();
}
