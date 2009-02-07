//----------------------------------------------------------------------------
/** @file GoEyeUtil.h
    GoBoard eye-related utility classes.
*/
//----------------------------------------------------------------------------

#ifndef GO_EYEUTIL_H
#define GO_EYEUTIL_H

#include "GoBoard.h"
#include "SgPoint.h"

//----------------------------------------------------------------------------

namespace GoEyeUtil
{
    /** Code for how many points of each degree there are.
        The degree measures how many of the (up to 4) neighbors
        are also in the set of points.

        code =     1 * # degree 0
             +    10 * # degree 1
             +   100 * # degree 2
             +  1000 * # degree 3
             + 10000 * # degree 4

        This is a different format, but has the same information
        as the Cazenave/Vila "neighbour classification".
        E.g. their code 112224 means 2x degree 1, 3x degree 2, 1x degree 4,
        so the DegreeCode is 10320.

        The DegreeCode is not strong enough for graph isomorphism testing -
        there are nonisomorphic graphs with the same code -
        but it is good for distinguishing small graphs.

        For example, it can not distinguish between a "straight" and a "bent"
        line of three.
    */
    int DegreeCode(const SgPointSet& points);

    /** Like DegreeCode, but also count diagonal neighbors */
    long DegreeCode8(const SgPointSet& points);

    /** Return an empty neighbor of p. Precondition: one must exist. */
    template<class BOARD>
    SgPoint EmptyNeighbor(const BOARD& bd, SgPoint p);

    /** Check if point is a single point eye with one or two adjacent blocks.
        This is a fast eye detection routine, which can be used instead of
        Benson's static life detection, when end-of-game detection is a
        performance bottle-neck (e.g. for machine-learning or monte-carlo).
        It detects single-point eyes surrounded by a single block or by two
        blocks that share another single point eye.
        Larger eyes can be reduced to simple eyes (assuming chinese rules,
        so that playing on does not change the score).
        @todo Add example to documentation where this method fails
    */
    bool IsSimpleEye(const GoBoard& bd, SgPoint p, SgBlackWhite c);

    /**  */
    bool IsSinglePointEye(const GoBoard& bd, SgPoint p, SgBlackWhite c);

    /** Return true if a point can become an eye by adding one more
    defender's move */
    bool IsPossibleEye(const GoBoard& board, SgBlackWhite color, SgPoint p);

    /** Given opponent's safe stones, can p ever become an eye?
        Checks direct and diagonal neighbors.
    */
    bool CanBecomeSinglePointEye(const GoBoard& board, SgPoint p,
                             const SgPointSet& oppSafe);

    /** Return true if a point can become an eye by adding number of
        defender's move.
    */
    bool NumberOfMoveToEye(const GoBoard& bd, SgBlackWhite c, SgPoint p,
                           int& number);

    /** As IsSinglePointEye, but allows diagonal points to be eyes.
        Slightly slower, but identifies more single point eyes.
        E.g:
        @verbatim
        # X X X . .
        # O O X X X
        # O . O O X
        # . O . O X
        ###########
        @endverbatim
    */
    bool IsSinglePointEye2(const GoBoard& bd, SgPoint p, SgBlackWhite c);

    /** As IsSinglePointEye2, but specifying points assumed to be eyes. */
    bool IsSinglePointEye2(const GoBoard& bd, SgPoint p,
                           SgBlackWhite c, SgList<SgPoint>& eyes);

    /** p is in a 2 point eye surrounded by a single chain */
    bool IsTwoPointEye(const GoBoard& bd, SgPoint p,
                       SgBlackWhite c);

    /** As NumberOfMoveToEye2, but includes existing diagonal eyes,
        and allows opponent stones to be captured.
    */
    bool NumberOfMoveToEye2(const GoBoard& bd, SgBlackWhite c,
                            SgPoint p, int& nummoves);

    /** Count number of single point eyes for block p */
    int CountSinglePointEyes2(const GoBoard& bd, SgPoint p);

    /** Does block at p have two or more single point eyes? */
    bool SinglePointSafe2(const GoBoard& bd, SgPoint p);

    /** Does removing p split s into two or more parts? */
    bool IsSplitPt(SgPoint p, const SgPointSet& s);

    /** Does p locally,within a 3x3 region, split its neighbors in s?
        Even if the reply is 'yes', s might still be connected outside
        the region.
    */
    bool IsLocalSplitPt(SgPoint p, const SgPointSet& set);

    /** Area is tree shape if it does not contain a 2x2 square. */
    bool IsTreeShape(const SgPointSet& area);

    /** Vital point in small shape - usually has most liberties
        See implementation for details.
    */
    bool IsVitalPt(const SgPointSet& points, SgPoint p, SgBlackWhite opp,
               const GoBoard& bd);

    /** Analyze small region locally for number of eyes.
        color: the player surrounding the area.
        isNakade: only one eye
        makeNakade: attacker can reduce to one eye, defender can live locally.
        makeFalse: attacker can make the area into a false eye.
        maybeSeki, sureSeki: can there be, or is there, a seki between
            boundary stones and interior opponent stones?
        @todo: seki support is primitive only.
        vital is set iff makeNakade or makeFalse
    */
    void TestNakade(const SgPointSet& points, const GoBoard& bd,
                    SgBlackWhite color, bool isFullyEnclosed, bool& isNakade,
                    bool& makeNakade, bool& makeFalse, bool& maybeSeki,
                    bool& sureSeki, SgPoint* vital);

    bool CheckInterior(const GoBoard& bd, const SgPointSet& area,
                       SgBlackWhite opp, bool checkBlocks);
}

inline bool GoEyeUtil::IsSimpleEye(const GoBoard& bd, SgPoint p,
                                   SgBlackWhite c)
{
    // Function is inline despite its large size, because it returns quickly
    // on average, which makes the function call an overhead

    SgBlackWhite opp = OppBW(c);
    if (bd.HasEmptyNeighbors(p) || bd.HasNeighbors(p, opp))
        return false;
    SgSList<SgPoint,2> anchors;
    for (SgNb4Iterator it(p); it; ++it)
    {
        SgPoint nbPoint = *it;
        if (bd.IsBorder(nbPoint))
            continue;
        SG_ASSERT(bd.GetColor(nbPoint) == c);
        SgPoint nbAnchor = bd.Anchor(nbPoint);
        if (! anchors.Contains(nbAnchor))
        {
            if (anchors.Length() > 1)
                return false;
            anchors.Append(nbAnchor);
        }
    }
    if (anchors.Length() == 1)
        return true;
    for (GoBoard::LibertyIterator it(bd, anchors[0]); it; ++it)
    {
        SgPoint lib = *it;
        if (lib == p)
            continue;
        bool isSecondSharedEye = true;
        SgSList<SgPoint,2> foundAnchors;
        for (SgNb4Iterator it2(lib); it2; ++it2)
        {
            SgPoint nbPoint = *it2;
            if (bd.IsBorder(nbPoint))
                continue;
            if (bd.GetColor(nbPoint) != c)
            {
                isSecondSharedEye = false;
                break;
            }
            SgPoint nbAnchor = bd.Anchor(nbPoint);
            if (! anchors.Contains(nbAnchor))
            {
                isSecondSharedEye = false;
                break;
            }
            if (! foundAnchors.Contains(nbAnchor))
                foundAnchors.Append(nbAnchor);
        }
        if (isSecondSharedEye && foundAnchors.Length() == 2)
            return true;
    }
    return false;
}

template<class BOARD>
SgPoint GoEyeUtil::EmptyNeighbor(const BOARD& bd, SgPoint p)
{
    if (bd.IsEmpty(p + SG_NS))
        return p + SG_NS;
    if (bd.IsEmpty(p - SG_NS))
        return p - SG_NS;
    if (bd.IsEmpty(p + SG_WE))
        return p + SG_WE;
    if (bd.IsEmpty(p - SG_WE))
        return p - SG_WE;
    SG_ASSERT(false);
    return SG_NULLPOINT;
}

//----------------------------------------------------------------------------

#endif // GO_EYEUTIL_H
