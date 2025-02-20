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

#include <memory>
#include <algorithm>
#include "trees.h"
#include "QuadTree/LooseQuadtree.h"

std::vector<void*> treeresult_vec[4] = {std::vector<void*>(400), std::vector<void*>(400), std::vector<void*>(50), std::vector<void*>(50)};
ptrdiff_t cur_treeresult_vec = 0;

template<class ItemT>
class Tree_Extractor
{
public:
    static void ExtractBoundingBox(const ItemT *object, loose_quadtree::BoundingBox<double> *bbox)
    {
        bbox->left      = object->Location.X;
        bbox->top       = object->Location.Y;
        bbox->width     = object->Location.Width;
        bbox->height    = object->Location.Height;
    }
};


template<>
class Tree_Extractor<Block_t>
{
public:
    static void ExtractBoundingBox(const Block_t *object, loose_quadtree::BoundingBox<double> *bbox)
    {
        bbox->left      = object->LocationInLayer.X;
        bbox->top       = object->LocationInLayer.Y;
        bbox->width     = object->LocationInLayer.Width;
        bbox->height    = object->LocationInLayer.Height;
    }
};

template<class ItemT>
struct Tree_private
{
    typedef loose_quadtree::LooseQuadtree<double, ItemT, Tree_Extractor<ItemT>> IndexTreeQ;
    IndexTreeQ tree;
};

const double s_gridSize = 4;

static std::unique_ptr<Tree_private<Tile_t>> s_worldTilesTree;
static std::unique_ptr<Tree_private<Scene_t>> s_worldSceneTree;
static std::unique_ptr<Tree_private<WorldPath_t>> s_worldPathTree;
static std::unique_ptr<Tree_private<WorldLevel_t>> s_worldLevelTree;
static std::unique_ptr<Tree_private<WorldMusic_t>> s_worldMusicTree;
static std::unique_ptr<Tree_private<Block_t>> s_levelBlockTrees[maxLayers+2];

template<class Q>
void clearTree(Q &tree)
{
    if(tree.get())
    {
        tree->tree.Clear();
        tree.reset();
    }
}

void treeWorldCleanAll()
{
    clearTree(s_worldTilesTree);
    clearTree(s_worldSceneTree);
    clearTree(s_worldPathTree);
    clearTree(s_worldLevelTree);
    clearTree(s_worldMusicTree);
}

void treeLevelCleanBlockLayers()
{
    for(int i = 0; i < maxLayers+2; i++)
        clearTree(s_levelBlockTrees[i]);
}

void treeLevelCleanAll()
{
    treeLevelCleanBlockLayers();
}

template<class Obj, class Arr>
void treeInsert(Arr &p, Obj*obj)
{
    if(!p.get())
        p.reset(new Tree_private<Obj>());
    p->tree.Insert(obj);
}

template<class Obj, class Arr>
void treeUpdate(Arr &p, Obj*obj)
{
    if(p.get())
        p->tree.Update(obj);
}

template<class Obj, class Arr>
void treeRemove(Arr &p, Obj*obj)
{
    if(p.get())
        p->tree.Remove(obj);
}

template<class Obj>
TreeResult_Sentinel<Obj> treeWorldQuery(std::unique_ptr<Tree_private<Obj>> &p,
    double Left, double Top, double Right, double Bottom, bool z_sort)
{
    TreeResult_Sentinel<Obj> result;

    if(!p.get())
        return result;

    auto q = p->tree.QueryIntersectsRegion(loose_quadtree::BoundingBox<double>(Left - s_gridSize,
                                                                               Top - s_gridSize,
                                                                               (Right - Left) + s_gridSize * 2,
                                                                               (Bottom - Top) + s_gridSize * 2));

    while(!q.EndOfQuery())
    {
        auto *item = q.GetCurrent();
        if(item)
            result.i_vec->push_back(item);
        q.Next();
    }

    if(z_sort)
    {
        std::sort(result.i_vec->begin(), result.i_vec->end(),
            [](void* a, void* b) {
                return a < b;
            });
    }

    return result;
}


/* ================= Terrain Tile ================= */

void treeWorldTileAdd(Tile_t *obj)
{
    treeInsert(s_worldTilesTree, obj);
}

void treeWorldTileUpdate(Tile_t *obj)
{
    treeUpdate(s_worldTilesTree, obj);
}

void treeWorldTileRemove(Tile_t *obj)
{
    treeRemove(s_worldTilesTree, obj);
}

TreeResult_Sentinel<Tile_t> treeWorldTileQuery(double Left, double Top, double Right, double Bottom, bool z_sort, double margin)
{
    return treeWorldQuery(s_worldTilesTree,
                   Left - margin,
                   Top - margin,
                   Right + margin,
                   Bottom + margin,
                   z_sort);
}

TreeResult_Sentinel<Tile_t> treeWorldTileQuery(const Location_t &loc, bool z_sort, double margin)
{
    return treeWorldQuery(s_worldTilesTree,
                   loc.X - margin,
                   loc.Y - margin,
                   loc.X + loc.Width + margin,
                   loc.Y + loc.Height + margin,
                   z_sort);
}


/* ================= Scenery ================= */

void treeWorldSceneAdd(Scene_t *obj)
{
    treeInsert(s_worldSceneTree, obj);
}

void treeWorldSceneUpdate(Scene_t *obj)
{
    treeUpdate(s_worldSceneTree, obj);
}

void treeWorldSceneRemove(Scene_t *obj)
{
    treeRemove(s_worldSceneTree, obj);
}

TreeResult_Sentinel<Scene_t> treeWorldSceneQuery(double Left, double Top, double Right, double Bottom, bool z_sort, double margin)
{
    return treeWorldQuery(s_worldSceneTree,
                   Left - margin,
                   Top - margin,
                   Right + margin,
                   Bottom + margin,
                   z_sort);
}

TreeResult_Sentinel<Scene_t> treeWorldSceneQuery(const Location_t &loc, bool z_sort, double margin)
{
    return treeWorldQuery(s_worldSceneTree,
                   loc.X - margin,
                   loc.Y - margin,
                   loc.X + loc.Width + margin,
                   loc.Y + loc.Height + margin,
                   z_sort);
}


/* ================= Paths ================= */

void treeWorldPathAdd(WorldPath_t *obj)
{
    treeInsert(s_worldPathTree, obj);
}

void treeWorldPathUpdate(WorldPath_t *obj)
{
    treeUpdate(s_worldPathTree, obj);
}

void treeWorldPathRemove(WorldPath_t *obj)
{
    treeRemove(s_worldPathTree, obj);
}

TreeResult_Sentinel<WorldPath_t> treeWorldPathQuery(double Left, double Top, double Right, double Bottom,
                        bool z_sort, double margin)
{
    return treeWorldQuery(s_worldPathTree,
                   Left - margin,
                   Top - margin,
                   Right + margin,
                   Bottom + margin,
                   z_sort);
}

TreeResult_Sentinel<WorldPath_t> treeWorldPathQuery(const Location_t &loc, bool z_sort, double margin)
{
    return treeWorldQuery(s_worldPathTree,
                   loc.X - margin,
                   loc.Y - margin,
                   loc.X + loc.Width + margin,
                   loc.Y + loc.Height + margin,
                   z_sort);
}

/* ================= Levels ================= */

void treeWorldLevelAdd(WorldLevel_t *obj)
{
    treeInsert(s_worldLevelTree, obj);
}

void treeWorldLevelUpdate(WorldLevel_t *obj)
{
    treeUpdate(s_worldLevelTree, obj);
}

void treeWorldLevelRemove(WorldLevel_t *obj)
{
    treeRemove(s_worldLevelTree, obj);
}

TreeResult_Sentinel<WorldLevel_t> treeWorldLevelQuery(double Left, double Top, double Right, double Bottom,
                         bool z_sort, double margin)
{
    return treeWorldQuery(s_worldLevelTree,
                   Left - margin,
                   Top - margin,
                   Right + margin,
                   Bottom + margin,
                   z_sort);
}

TreeResult_Sentinel<WorldLevel_t> treeWorldLevelQuery(const Location_t &loc, bool z_sort, double margin)
{
    return treeWorldQuery(s_worldLevelTree,
                   loc.X - margin,
                   loc.Y - margin,
                   loc.X + loc.Width + margin,
                   loc.Y + loc.Height + margin,
                   z_sort);
}


/* ================= Music ================= */

void treeWorldMusicAdd(WorldMusic_t *obj)
{
    treeInsert(s_worldMusicTree, obj);
}

void treeWorldMusicUpdate(WorldMusic_t *obj)
{
    treeUpdate(s_worldMusicTree, obj);
}

void treeWorldMusicRemove(WorldMusic_t *obj)
{
    treeRemove(s_worldMusicTree, obj);
}

TreeResult_Sentinel<WorldMusic_t> treeWorldMusicQuery(double Left, double Top, double Right, double Bottom,
                         bool z_sort,
                         double margin)
{
    return treeWorldQuery(s_worldMusicTree,
                   Left - margin,
                   Top - margin,
                   Right + margin,
                   Bottom + margin,
                   z_sort);
}

TreeResult_Sentinel<WorldMusic_t> treeWorldMusicQuery(const Location_t &loc,
                         bool z_sort,
                         double margin)
{
    return treeWorldQuery(s_worldMusicTree,
                   loc.X - margin,
                   loc.Y - margin,
                   loc.X + loc.Width + margin,
                   loc.Y + loc.Height + margin, z_sort);
}


/* ================= Level blocks ================= */

void treeBlockAddLayer(int layer, Block_t *obj)
{
    if(layer < 0)
        layer = maxLayers + 1;
    treeInsert(s_levelBlockTrees[layer], obj);
}

void treeBlockUpdateLayer(int layer, Block_t *obj)
{
    if(layer < 0)
        layer = maxLayers + 1;
    treeUpdate(s_levelBlockTrees[layer], obj);
}

void treeBlockRemoveLayer(int layer, Block_t *obj)
{
    if(layer < 0)
        layer = maxLayers + 1;
    treeRemove(s_levelBlockTrees[layer], obj);
}

TreeResult_Sentinel<Block_t> treeBlockQuery(double Left, double Top, double Right, double Bottom,
                         int sort_mode,
                         double margin)
{
    TreeResult_Sentinel<Block_t> result;

    for(int layer = 0; layer < maxLayers+2; layer++)
    {
        double OffsetX, OffsetY;
        if (layer == maxLayers+1)
            OffsetX = OffsetY = 0.0;
        else
        {
            OffsetX = Layer[layer].OffsetX;
            OffsetY = Layer[layer].OffsetY;
        }
        std::unique_ptr<Tree_private<Block_t>>& p = s_levelBlockTrees[layer];
        if(!p.get())
            continue;

        auto q = p->tree.QueryIntersectsRegion(loose_quadtree::BoundingBox<double>(Left - OffsetX - margin - s_gridSize,
                                                                                   Top - OffsetY - margin - s_gridSize,
                                                                                   (Right - Left) + (margin + s_gridSize) * 2,
                                                                                   (Bottom - Top) + (margin + s_gridSize) * 2));
        while(!q.EndOfQuery())
        {
            auto *item = q.GetCurrent();
            if(item)
                result.i_vec->push_back(item);
            q.Next();
        }
    }

    if(sort_mode == SORTMODE_LOC)
    {
        std::sort(result.i_vec->begin(), result.i_vec->end(),
            [](void* a, void* b) {
                return (((Block_t*)a)->Location.X < ((Block_t*)b)->Location.X
                    || (((Block_t*)a)->Location.X == ((Block_t*)b)->Location.X
                        && ((Block_t*)a)->Location.Y < ((Block_t*)b)->Location.Y));
            });
    }
    else if(sort_mode == SORTMODE_ID)
    {
        std::sort(result.i_vec->begin(), result.i_vec->end(),
            [](void* a, void* b) {
                return a < b;
            });
    }

    return result;
}

TreeResult_Sentinel<Block_t> treeBlockQuery(const Location_t &loc,
                         int sort_mode,
                         double margin)
{
    return treeBlockQuery(loc.X,
                   loc.Y,
                   loc.X + loc.Width,
                   loc.Y + loc.Height, sort_mode, margin);
}
