//----------------------------------------------------------------------------
/** @file GoSafetyUtil.cpp
    See GoSafetyUtil.h.
*/
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "GoSafetyUtil.h"

#include "GoBoard.h"
#include "GoBoardUtil.h"
#include "GoEyeUtil.h"
#include "GoRegionBoard.h"
#include "SgBWSet.h"
#include "SgList.h"
#include "SgPointSet.h"
#include "SgPointSetUtil.h"
#include "SgWrite.h"

using GoSafetyUtil::Find2Libs;
using GoSafetyUtil::MightMakeLife;
using GoSafetyUtil::ReduceToAnchors;

//----------------------------------------------------------------------------

namespace {

const bool DEBUG_SAFETY = false;
const bool DEBUG_EXTENDED_MIGHT_MAKE_LIFE = false;

/** find 2 libs which would connect block to safe.
    if found, update libs and safe to indicate that the block is safe now:
    add block to safe, add block libs to libs, remove the two libs.
*/
bool Find2Conn(const GoBoard& bd, SgPoint block, SgPointSet* libs,
                    SgPointSet* usedLibs, SgPointSet* safe)
{
    SG_ASSERT(libs->Disjoint(*usedLibs));

    int nuLibs(0);
    SgList<SgPoint> blockLibs;
    for (GoBoard::LibertyIterator it(bd, block); it; ++it)
    {
        if (libs->Contains(*it))
        {
            blockLibs.Append(*it);
            ++nuLibs;
            if (nuLibs >= 2)
                break;
        }
    }
    if (nuLibs >= 2) // found it!
    {
        for (GoBoard::StoneIterator it(bd, block); it; ++it)
            safe->Include(*it);
        for (GoBoard::LibertyIterator it(bd, block); it; ++it)
            libs->Include(*it);
        for (SgListIterator<SgPoint> it(blockLibs); it; ++it)
            usedLibs->Include(*it);
        *libs -= *usedLibs; // can never re-use such a lib.
    }

    return nuLibs >= 2;
}

/** Find connections for unsafe boundary and interior stones,
    and interior empty points.

    Can omit maxNuOmissions (0 or 1) empty points from checking.
    maxNuOmissions = 1 if testing whether opponent can make 2 eyes here,
    0 otherwise. Returns bool whether connections were found.
*/
bool Find2ConnForAll(const GoBoard& bd, const SgPointSet& pts,
                const SgPointSet& inSafe, SgBlackWhite color,
                int maxNuOmissions = 0)
{
    if (DEBUG_SAFETY)
        SgDebug() << "Find2ConnForAll " << pts
                  << "safe points - input: " << inSafe;
    SgPointSet safe(inSafe);
    SgList<SgPoint> unsafe;
    const int size = bd.Size();
    ReduceToAnchors(bd, pts.Border(size) - safe, &unsafe);
    // AR sort by # empty nbs in pts

    if (DEBUG_SAFETY)
        SgDebug() << SgWritePointList(unsafe, "unsafe anchors: ");

    SgPointSet libs = pts & bd.AllEmpty() & safe.Border(size);
    SgPointSet interior = pts - libs;
    interior -= bd.All(SgOppBW(color)); // remove opp. stones.
    SgList<SgPoint> unsafeInterior;
    ReduceToAnchors(bd, interior & bd.All(color), &unsafeInterior);
    unsafe.Concat(&unsafeInterior);

    SgPointSet usedLibs;
    bool change = true;
    while (change && unsafe.NonEmpty() && libs.MinSetSize(2))
    {
        SgList<SgPoint> newSafe;
        for (SgListIterator<SgPoint> it(unsafe); it; ++it)
            if (Find2Conn(bd, *it, &libs, &usedLibs, &safe))
            {
                newSafe.Append(*it);
            }

        unsafe.Exclude(newSafe);
        change = newSafe.NonEmpty();
    }

    if (unsafe.NonEmpty()) // could not connect everything.
    {
        if (DEBUG_SAFETY)
            SgDebug()
                << SgWritePointList(unsafe, "could not connect unsafe: ");
        return false;
    }
    // now we know all blocks are safe. try to prove territory safe, too.
    // AR if terr. safety fails, still declare blocks safe, but put
    // miai strategy commitment on region.

    interior = (pts & bd.AllEmpty()) - safe.Border(size);
    // new safe set after Find2Conn.

    // try to prove opp. can't live inside.
    if (maxNuOmissions == 1)
    {
        SgBlackWhite opp(SgOppBW(color));
        if (! MightMakeLife(bd, interior, safe, opp))
            /* */ return true; /* */
    }

    // now try to find miai-paths to remaining interior empty points
    // AR shortcut failure if maxNuOmissions = 0 and opp has eye:
    // then this will never find anything.

    for (SgSetIterator it(interior); it; ++it)
    {
        if (! Find2Libs(*it, &libs))
        {
            if (--maxNuOmissions < 0)
                return false;
        }
    }

    return true;
}

void TestLiberty(SgPoint lib, const SgPointSet& libs,
                 SgList<SgPoint>* foundLibs,
                 int* nuLibs)
{
    if (libs.Contains(lib))
    {
        foundLibs->Append(lib);
        ++(*nuLibs);
    }
}

} // namespace

//----------------------------------------------------------------------------

void GoSafetyUtil::AddToSafe(const GoBoard& board, const SgPointSet& pts,
                             SgBlackWhite color, SgBWSet* safe,
                             const char* reason, int depth, bool addBoundary)
{
    SG_UNUSED(reason);
    SG_UNUSED(depth);

    (*safe)[color] |= pts;
    safe->AssertDisjoint();
    SgPointSet empty = board.AllEmpty();

    const int size = board.Size();
    if (addBoundary)
    {
        SgPointSet dep(pts.Border(size) - empty); // pts can be zone (open!)
        GoBoardUtil::ExpandToBlocks(board, dep);
        SG_ASSERT(dep.SubsetOf(board.All(color)));
        (*safe)[color] |= dep;
        safe->AssertDisjoint();
    }

    if (DEBUG_SAFETY)
        SgDebug() << "AddToSafe " << reason
                  << " depth = " << depth << " points = "
                  << SgWritePointSetID(pts) << '\n';
}

bool GoSafetyUtil::ExtendedMightMakeLife(const GoBoard& board,
                                         GoRegionBoard* regions,
                                         const SgPointSet& area,
                                         const SgPointSet& safe,
                                         SgBlackWhite color)
{
    const GoRegion* r = 0;

    if (DEBUG_EXTENDED_MIGHT_MAKE_LIFE)
        SgDebug() << "ExtendedMightMakeLife for " << SgBW(color)
                  << " area " << area << '\n';

    // Check if region is a nakade shape that fills all potential eye space
    for (SgListIteratorOf<GoRegion> it(regions->AllRegions(color)); it; ++it)
    {
        if (    area.SupersetOf((*it)->Points())
            &&  area.SupersetOf((*it)->BlocksPoints())
           )
        {
            if (DEBUG_EXTENDED_MIGHT_MAKE_LIFE)
            {
                SgDebug() << "contains region ";
                (*it)->WriteID(SgDebug());
                SgDebug() << '\n';
            }

            if (! (*it)->ComputedFlag(computedNakade))
                (*it)->DoComputeFlag(computedNakade); // sets MaxPotEyes()
            if ((*it)->MaxPotEyes() > 1)
                return true;
            else if (r == 0)
                r = *it;
            else // at least 2 regions - might make eyes
                return true;
        }
    }

    if (DEBUG_EXTENDED_MIGHT_MAKE_LIFE)
        SgDebug() << "case 2\n";
    SgPointSet rest = area;
    if (r == 0) // classical case. Call previous function
        return MightMakeLife(board, area, safe, color);
    else
    {
        if (DEBUG_EXTENDED_MIGHT_MAKE_LIFE)
            SgDebug() << "ExtendedMightMakeLife for " << area
                      << ": inside opp region " << *r << '\n';
        if (r->MaxPotEyes() <= 1)
        // what if 0 eyes??? Can allow 1 eye elsewhere?
        {
            rest -= r->Points();
            rest -= r->BlocksPoints();
        }
    }

    const int size = board.Size();
    rest -= safe.Border(size);
    rest -= board.All(color);

    if (DEBUG_EXTENDED_MIGHT_MAKE_LIFE)
        SgDebug() << "rest = " << rest << "\n";
    for (SgSetIterator it(rest); it; ++it)
    {
        SgPoint p(*it);
        if (GoEyeUtil::CanBecomeSinglePointEye(board, p, safe))
            return true;
    }

    return false;
}

SgPointSet GoSafetyUtil::FindDamePoints(const GoBoard& bd,
                                        const SgPointSet& empty,
                                        const SgBWSet& safe)
{
    SgPointSet dame, unsurroundable;
    FindDameAndUnsurroundablePoints(bd, empty, safe, &dame, &unsurroundable);
    return dame;
}

void GoSafetyUtil::FindDameAndUnsurroundablePoints(const GoBoard& bd,
                                                   const SgPointSet& empty,
                                                   const SgBWSet& safe,
                                                   SgPointSet* dame,
                                                   SgPointSet* unsurroundable)
{
    SG_ASSERT(dame->IsEmpty());
    SG_ASSERT(unsurroundable->IsEmpty());
    const int size = bd.Size();
    *unsurroundable =   safe[SG_BLACK].Border(size)
                      & safe[SG_WHITE].Border(size)
                      & empty;
    SgPointSet maybeDame(*unsurroundable);
    SgBWSet unsafe; // must exclude these
    unsafe[SG_BLACK] = bd.All(SG_BLACK) - safe[SG_BLACK];
    unsafe[SG_WHITE] = bd.All(SG_WHITE) - safe[SG_WHITE];
    maybeDame -= unsafe[SG_BLACK].Border(size);
    maybeDame -= unsafe[SG_WHITE].Border(size);
    for (SgSetIterator it(maybeDame); it; ++it)
    {
        SgPoint p(*it);
        bool isDame = true;
        for (SgNb4Iterator it(p); it; ++it)
        {
            SgPoint nb(*it);
            if (empty[nb] && ! unsurroundable->Contains(nb))
            {
            // can use unsurroundable instead of smaller set maybeDame
                isDame = false;
                break;
            }
        }
        if (isDame)
            dame->Include(p);
    }
}

bool GoSafetyUtil::MightMakeLife(const GoBoard& board,
                                 const SgPointSet& area,
                                 const SgPointSet& safe, SgBlackWhite color)
{
    const int size = board.Size();
    SgPointSet eyePts = (area - safe.Border(size)) - board.All(color);
    if (eyePts.MaxSetSize(1))
        return false;

    SgPoint eye(SG_NULLPOINT), adjToEye(SG_NULLPOINT);

    for (SgSetIterator it(eyePts); it; ++it)
    {
        SgPoint p(*it);
        if (GoEyeUtil::CanBecomeSinglePointEye(board, p, safe))
        {
            if (eye == SG_NULLPOINT)
                eye = p;
            else if (   adjToEye == SG_NULLPOINT
                     && SgPointUtil::AreAdjacent(eye, p)
                    )
                adjToEye = p;
            else
                /* */ return true; /* */
        }
    }

    return false;
}

bool GoSafetyUtil::Find2Libs(SgPoint p, SgPointSet* libs)
{
    int nuLibs = 0;
    SgList<SgPoint> foundLibs;
    TestLiberty(p + SG_NS, *libs, &foundLibs, &nuLibs);
    TestLiberty(p + SG_WE, *libs, &foundLibs, &nuLibs);
    if (nuLibs < 2)
    {
        TestLiberty(p - SG_NS, *libs, &foundLibs, &nuLibs);
        if (nuLibs < 2)
            TestLiberty(p - SG_WE, *libs, &foundLibs, &nuLibs);
    }
    if (nuLibs >= 2)
    {
        SG_ASSERT(nuLibs == 2 && foundLibs.IsLength(2));
        libs->Exclude(foundLibs.Top());
        libs->Exclude(foundLibs.Tail());
    }

    return nuLibs >= 2;
}

bool GoSafetyUtil::Find2BestLibs(SgPoint p, const SgPointSet& libs,
                                 SgPointSet interior, SgMiaiPair* miaiPair)
{
    int nuLibs = 0;
    SgList<SgPoint> allLibs; // liberties of point p

    TestLiberty(p + SG_NS, libs, &allLibs, &nuLibs);
    TestLiberty(p + SG_WE, libs, &allLibs, &nuLibs);
    TestLiberty(p - SG_NS, libs, &allLibs, &nuLibs);
    TestLiberty(p - SG_WE, libs, &allLibs, &nuLibs);

    if (allLibs.MaxLength(1))
        return false;
    else if ( allLibs.IsLength(2))
    {
        SG_ASSERT(nuLibs == 2 && allLibs.IsLength(2));
        miaiPair->first = allLibs.Top();
        miaiPair->second = allLibs.Tail();
        /* */ return true; /* */
    }
    else
    {
        SgList<SgPoint> shared, not_shared;
        SgPointSet others = interior;
        others.Exclude(p);

        for (SgListIterator<SgPoint> it(allLibs); it; ++it)
        {
            bool share = false;
            for (SgSetIterator it2(others); it2; ++it2)
            {
                if (SgPointUtil::AreAdjacent(*it, *it2))
                {
                    share = true;
                    break;
                }
            }
            if (share)
                shared.Append(*it);
                // this lib is shared with other interior points
            else
                not_shared.Append(*it);
        }
        // if has >= 3 not_shared libs, then try to find 2 original libs (not
        // new-created libs (because the new one might be ip points)
        if (not_shared.MinLength(2))
        {
            miaiPair->first = not_shared.Pop();
            miaiPair->second = not_shared.Pop();
        }
        /*
        else if (not_shared.IsLength(2))
        {

        }
        */
        // if only 1 not_shared lib, use this first, then another shared lib
        else if (not_shared.IsLength(1))
        {
            miaiPair->first = not_shared.Pop();
            miaiPair->second = shared.Pop();
        }
        // zero not_shared libs, then use two shared libs
        else
        {
            miaiPair->first = shared.Pop();
            miaiPair->second = shared.Pop();
        }
        return true;
    }
}

bool GoSafetyUtil::ExtendedIsTerritory(const GoBoard& board,
                                       GoRegionBoard* regions,
                                       const SgPointSet& pts,
                                       const SgPointSet& safe,
                                       SgBlackWhite color)
{
    const int size = board.Size();
    SgPointSet boundary(pts.Border(size));
    if (boundary.SubsetOf(safe))
    {
        SgBlackWhite opp = SgOppBW(color);
        if (! ExtendedMightMakeLife(board, regions, pts, safe, opp))
            /* */ return true; /* */
    }

    return IsTerritory(board, pts, safe, color);
}

bool GoSafetyUtil::IsTerritory(const GoBoard& board, const SgPointSet& pts,
                               const SgPointSet& safe, SgBlackWhite color)
{
    const int size = board.Size();
    SgPointSet boundary(pts.Border(size));
    if (boundary.SubsetOf(safe))
    {
        SgBlackWhite opp = SgOppBW(color);
        if (! MightMakeLife(board, pts, safe, opp))
            /* */ return true; /* */
    }

    if (boundary.SubsetOf(board.All(color))
         && Find2ConnForAll(board, pts, safe, color, 1))
        /* */ return true; /* */
    return false;
}

void GoSafetyUtil::ReduceToAnchors(const GoBoard& board,
                                   const SgPointSet& stones,
                                   SgList<SgPoint>* anchors)
{
    SG_ASSERT(anchors->IsEmpty());
    for (SgSetIterator it(stones); it; ++it)
    {
        SG_ASSERT(board.Occupied(*it));
        anchors->Insert(board.Anchor(*it));
    }
}

//----------------------------------------------------------------------------