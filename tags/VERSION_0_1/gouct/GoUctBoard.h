//----------------------------------------------------------------------------
/** @file GoUctBoard.h
    Go board optimized for Monte-Carlo.
*/
//----------------------------------------------------------------------------

#ifndef GOUCT_BOARD_H
#define GOUCT_BOARD_H

#include <bitset>
#include <cstring>
#include <stdint.h>
#include <boost/static_assert.hpp>
#include "GoBoard.h"
#include "GoBoardUtil.h"
#include "GoPlayerMove.h"
#include "SgArray.h"
#include "SgBoardConst.h"
#include "SgBoardColor.h"
#include "SgMarker.h"
#include "SgBWArray.h"
#include "SgNbIterator.h"
#include "SgPoint.h"
#include "SgPointArray.h"
#include "SgPointIterator.h"
#include "SgSList.h"

//----------------------------------------------------------------------------

/** Go board optimized for Monte-Carlo.
    In contrast to class GoBoard, this board makes certain assumptions
    that are usually true for Monte-Carlo simulations for better efficiency:
    - No undo (not yet done, some client code still uses undo)
    - Simple-Ko rule
    - suicide not allowed

    Otherwise, the member functions are named like in class GoBoard to allow
    writing utility functions that use the board class as a template parameter
    (as long as they use only the functionality shared by both board classes)
*/
class GoUctBoard
{
public:
    /** Marker that can be used in client code.
        This marker is never used by this class, it is intended for external
        functions that operate on the board and can profit from the fast clear
        operation of SgMarker (if reused), but cannot store its own
        marker (or don't want to use a global variable for thread-safety).
        Since only one function can use this marker at a time, you should
        assert with SgReserveMarker that the marker is not used in a
        conflicting way.
    */
    mutable SgMarker m_userMarker;

    explicit GoUctBoard(const GoBoard& bd);

    ~GoUctBoard();

    const SgBoardConst& BoardConst() const;

    /** Re-initializes the board from GoBoard position. */
    void Init(const GoBoard& bd);

    /** Return the size of this board. */
    SgGrid Size() const;

    /** Check if point is occupied by a stone.
        Can be called with border points.
    */
    bool Occupied(SgPoint p) const;

    bool IsEmpty(SgPoint p) const;

    bool IsBorder(SgPoint p) const;

    bool IsColor(SgPoint p, int c) const;

    SgBoardColor GetColor(SgPoint p) const;

    SgBlackWhite GetStone(SgPoint p) const;

    /** %Player whose turn it is to play. */
    SgBlackWhite ToPlay() const;

    /** Opponent of player whose turn it is to play. */
    SgBlackWhite Opponent() const;

    /** Set the current player. */
    void SetToPlay(SgBlackWhite player);

    /** See SgBoardConst::Line */
    SgGrid Line(SgPoint p) const;

    /** See SgBoardConst::Pos */
    SgGrid Pos(SgPoint p) const;

    /** Returns the offset to the point on the line above this point.
        Returns zero for points outside the board, and for the center
        point(s).
    */
    int Up(SgPoint p) const;

    /** Returns the offset along left side of the board.
        Left and right are as seen from the edge toward the center of the
        board.
        Returns zero for the same points as Up does.
    */
    int Left(SgPoint p) const;

    /** Returns the offset along right side of the board.
        @see Left for more info.
    */
    int Right(SgPoint p) const;

    /** Same as Left/Right, but the side is passed in as an index (0 or 1). */
    int Side(SgPoint p, int index) const;

    bool IsSuicide(SgPoint p, SgBlackWhite toPlay) const;

    bool IsValidPoint(SgPoint p) const;

    bool HasEmptyNeighbors(SgPoint p) const;

    int NumEmptyNeighbors(SgPoint p) const;

    /** Includes diagonals. */
    int Num8EmptyNeighbors(SgPoint p) const;

    bool HasNeighbors(SgPoint p, int c) const;

    int NumNeighbors(SgPoint p, int c) const;

    /** Includes diagonals. */
    int Num8Neighbors(SgPoint p, int c) const;

    bool HasDiagonals(SgPoint p, SgBoardColor c) const;

    int NumDiagonals(SgPoint p, int c) const;

    bool HasEmptyDiagonals(SgPoint p) const;

    int NumEmptyDiagonals(SgPoint p) const;

    bool HasNeighborsOrDiags(SgPoint p, SgBoardColor c) const;

    bool InCorner(SgPoint p) const;

    bool OnEdge(SgPoint p) const;

    bool InCenter(SgPoint p) const;

    /** See SgBoardConst::FirstBoardPoint */
    int FirstBoardPoint() const;

    /** See SgBoardConst::FirstBoardPoint */
    int LastBoardPoint() const;

    /** Play a move.
        Move needs to be SG_PASS or on-board empty point.
        If move is not legal, the move flag isIllegal will be set.
        After playing the color to play ys the opposite color of the color
        of the move.
    */
    void Play(SgPoint p, SgBlackWhite player);

    /** Play a move for the current player.
        @see Play(SgPoint,SgBlackWhite);
    */
    void Play(SgPoint p);

    /** Play a move.
        @see Play(SgPoint,SgBlackWhite);
    */
    void Play(GoPlayerMove move);

    /** Check whether the move at 'p' is legal.
        Since it's not clear how 'p' was arrived at, any value of 'p' is
        admissible, even out of point range and on border points; just return
        false on such input.
    */
    bool IsLegal(int p, SgBlackWhite player) const;

    /** Check whether the move at 'p' is legal for color to play.
        @see IsLegal(int, SgBlackWhite).
    */
    bool IsLegal(int p) const;

    bool IsSuicide(SgPoint p) const;

    /** Whether the most recent move captured any stones. */
    bool CapturingMove() const;

    /** The stones removed from the board by the most recent move.
        Can be used for incremental update of other data structures.
        Only valid directly after a GoUctBoard::Play, otherwise undefined.
    */
    const SgPointSList& CapturedStones() const;

    /** The stones captured by the most recent move.
        @see CapturedStones
    */
    int NuCapturedStones() const;

    /** The total number of stones of 'color' that have been
        captured by the opponent throughout the game. */
    int NumPrisoners(SgBlackWhite color) const;

    /** Return last move played.
        @return The last move played or SG_NULLMOVE, if
        - No move was played yet
        - The last move was not by the opposite color of the current player
    */
    SgPoint GetLastMove() const;

    /** 2nd Last move = last move by ToPlay().
        Conditions similar to GetLastMove().
    */
    SgPoint Get2ndLastMove() const;

    /** Return the number of stones in the block at 'p'.
        Not defined for empty or border points.
    */
    int NumStones(SgPoint p) const;

    /** Return NumStones(p) == 1. */
    bool IsSingleStone(SgPoint p) const;

    /** Return whether the two stones are located in the same block.
        Return false if one of the stones is an empty or border point.
    */
    bool AreInSameBlock(SgPoint stone1, SgPoint stone2) const;

    /** Return a reference point in the block at a point.
        @note In contrast to GoBoard, the anchor point is not guaranteed
        to be the smallest point (this functionality is not neede in
        Monte-Carlo)
        Requires: Occupied(p).
    */
    SgPoint Anchor(SgPoint p) const;

    /** See GoBoard::IsInBlock */
    bool IsInBlock(SgPoint p, SgPoint anchor) const;

    /** See GoBoard::IsLibertyOfBlock */
    bool IsLibertyOfBlock(SgPoint p, SgPoint anchor) const;

    /** Get adjacent opponent blocks with a maximum number of liberties for a
        given block.
        Not defined for empty points.
        @param p The block to check.
        @param maxLib The maximum number of liberties of the neighbors.
        @param anchors Resulting neighbor anchors and an additional END_POINT.
        @param maxAnchors Array size of anchors (for detecting overflow in
        debug mode)
        @return Number of anchors (without the END_POINT)
    */
    int AdjacentBlocks(SgPoint p, int maxLib, SgPoint anchors[],
                       int maxAnchors) const;

    /** %List anchor of each block of color 'c' adjacent to the
        empty point 'p'.
        Assert if 'p' is not empty.
        Fill an array of points, terminated by END_POINT.
    */
    void NeighborBlocks(SgPoint p, SgBlackWhite c, SgPoint anchors[]) const;

    /** %List anchor of each block of color 'c' with at most 'maxLib'
        liberties adjacent to the empty point 'p'.
        Assert if 'p' is not empty.
        Fill an array of points, terminated by END_POINT.
    */
    void NeighborBlocks(SgPoint p, SgBlackWhite c, int maxLib,
                        SgPoint anchors[]) const;

    /** Return the liberty of 'blockInAtari' which must have exactly
        one liberty.
    */
    SgPoint TheLiberty(SgPoint blockInAtari) const;

    /** Return the number of liberties of the block at 'p'.
        Not defined for empty or border points.
    */
    int NumLiberties(SgPoint p) const;

    /** Return whether block has at most n liberties. */
    bool AtMostNumLibs(SgPoint block, int n) const;

    /** Return whether block has at least n liberties. */
    bool AtLeastNumLibs(SgPoint block, int n) const;

    /** Return whether the number of liberties of the block at 'p' is one.
        Requires: Occupied(p)
    */
    bool InAtari(SgPoint p) const;

    /** Check if point is occupied and in atari.
        Faster than Occupied(p) || InAtari(p).
        May be called for border points.
    */
    bool OccupiedInAtari(SgPoint p) const;

    /** Return whether playing colour c at p can capture anything,
        ignoring any possible repetition. */
    bool CanCapture(SgPoint p, SgBlackWhite c) const;

    /** Checks whether all the board data structures are in a consistent
        state.
    */
    void CheckConsistency() const;

private:
    /** Data related to a block of stones on the board. */
    class Block
    {
    public:
        /** Upper limit for liberties.
            Proof?
        */
        static const int MAX_LIBERTIES = (SG_MAX_SIZE / 3) * 2 * SG_MAX_SIZE;

        typedef SgSList<SgPoint,MAX_LIBERTIES> LibertyList;

        typedef LibertyList::Iterator LibertyIterator;

        typedef SgPointSList::Iterator StoneIterator;

        SgPoint Anchor() const { return m_anchor; }

        void AppendLiberty(SgPoint p) { m_liberties.Append(p); }

        void AppendStone(SgPoint p) { m_stones.Append(p); }

        SgBlackWhite Color() const { return m_color; }

        void ExcludeLiberty(SgPoint p) { m_liberties.Exclude(p); }

        void InitSingleStoneBlock(SgBlackWhite c, SgPoint anchor)
        {
            SG_ASSERT(IsBlackWhite(c));
            m_color = c;
            m_anchor = anchor;
            m_stones.SetTo(anchor);
            m_liberties.Clear();
        }

        void InitNewBlock(SgBlackWhite c, SgPoint anchor)
        {
            SG_ASSERT(IsBlackWhite(c));
            m_color = c;
            m_anchor = anchor;
            m_stones.Clear();
            m_liberties.Clear();
        }

        const LibertyList& Liberties() const { return m_liberties; }

        int NumLiberties() const { return m_liberties.Length(); }

        int NumStones() const { return m_stones.Length(); }

        const SgPointSList& Stones() const { return m_stones; }

    private:
        SgPoint m_anchor;

        SgBlackWhite m_color;

        LibertyList m_liberties;

        SgPointSList m_stones;
    };

    SgPoint m_lastMove;

    SgPoint m_secondLastMove;

    /** Point which is currently illegal for simple Ko rule. */
    SgPoint m_koPoint;

    /** Whose turn it is to play. */
    SgBlackWhite m_toPlay;

    SgArray<Block*,SG_MAXPOINT> m_block;

    /** Number of prisoners of each color */
    SgBWArray<int> m_prisoners;

    /** The current board position. */
    SgArray<int,SG_MAXPOINT> m_color;

    /** Number of black and white neighbors. */
    SgArray<int,SG_MAXPOINT> m_nuNeighborsEmpty;

    /** Number of black and white neighbors. */
    SgBWArray<SgArray<int,SG_MAXPOINT> > m_nuNeighbors;

    /** Data that's constant for this board size. */
    SgBoardConst m_const;

    /** The current board size. */
    SgGrid m_size;

    SgPointArray<Block> m_blockArray;

    mutable SgMarker m_marker;

    SgPointSList m_capturedStones;

    SgArray<bool,SG_MAXPOINT> m_isBorder;

    /** Not implemented. */
    GoUctBoard(const GoUctBoard&);

    /** Not implemented. */
    GoUctBoard& operator=(const GoUctBoard&);

    void AddLibToAdjBlocks(SgPoint p, SgBlackWhite c);

    void AddStoneToBlock(SgPoint p, Block* block);

    void CreateSingleStoneBlock(SgPoint p, SgBlackWhite c);

    SgSList<Block*,4> GetAdjacentBlocks(SgPoint p) const;

    SgSList<Block*,4> GetAdjacentBlocks(SgPoint p, SgBlackWhite c) const;

    void InitSize(const GoBoard& bd);

    bool IsAdjacentTo(SgPoint p, const Block* block) const;

    void KillAdjacentOpponentBlocks(SgPoint p, SgBlackWhite opp);

    void MergeBlocks(SgPoint p, const SgSList<Block*,4>& adjBlocks);

    void RemoveLibFromAdjBlocks(SgPoint p);

    void RemoveLibFromAdjBlocks(SgPoint p, SgBlackWhite c);

    void UpdateBlocksAfterAddStone(SgPoint p, SgBlackWhite c);

    void CheckConsistencyBlock(SgPoint p) const;

    bool FullBoardRepetition() const;

    void AddStone(SgPoint p, SgBlackWhite c);

    void RemoveStone(SgPoint p);

    void KillBlock(SgPoint p);

    bool HasLiberties(SgPoint p) const;

public:
    friend class LibertyIterator;
    friend class StoneIterator;

    /** Iterate through all points on the given board. */
    class Iterator
        : public SgPointIterator
    {
    public:
        Iterator(const GoUctBoard& board);
    };

    /** Iterate through all the liberties of a block.
        Point 'p' must be occupied.
        Liberties should only be accessed for the current board position.
        No moves are allowed to be executed during the iteration.
    */
    class LibertyIterator
    {
    public:
        LibertyIterator(const GoUctBoard& bd, SgPoint p);

        /** Advance the state of the iteration to the next liberty. */
        void operator++();

        /** Return the current liberty. */
        int operator*() const;

        /** Return true if iteration is valid, otherwise false. */
        operator bool() const;

    private:
        GoUctBoard::Block::LibertyList::Iterator m_it;

        const GoUctBoard& m_board;

        /** Not implemented. */
        LibertyIterator(const LibertyIterator&);

        /** Not implemented. */
        LibertyIterator& operator=(const LibertyIterator&);
    };

    /** Iterate through all the stones of a block.
        Point 'p' must be occupied.
        Also, the stones can only be accessed for the current board position.
    */
    class StoneIterator
    {
    public:
        StoneIterator(const GoUctBoard& bd, SgPoint p);

        /** Advance the state of the iteration to the next liberty. */
        void operator++();

        /** Return the current liberty. */
        int operator*() const;

        /** Return true if iteration is valid, otherwise false. */
        operator bool() const;

    private:
        GoUctBoard::Block::StoneIterator m_it;

        const GoUctBoard& m_board;

        /** Not implemented. */
        StoneIterator(const StoneIterator&);

        /** Not implemented. */
        StoneIterator& operator=(const StoneIterator&);
    };
};

inline std::ostream& operator<<(std::ostream& out, const GoUctBoard& bd)
{
    return GoWriteBoard(out, bd);
}

inline GoUctBoard::Iterator::Iterator(const GoUctBoard& board)
    : SgPointIterator(board.BoardConst().BoardIterAddress())
{
}

inline GoUctBoard::LibertyIterator::LibertyIterator(const GoUctBoard& bd,
                                                    SgPoint p)
    : m_it(bd.m_block[p]->Liberties()),
      m_board(bd)
{
    SG_ASSERT(m_board.Occupied(p));
}

inline void GoUctBoard::LibertyIterator::operator++()
{
    ++m_it;
}

inline int GoUctBoard::LibertyIterator::operator*() const
{
    return *m_it;
}

inline GoUctBoard::LibertyIterator::operator bool() const
{
    return m_it;
}

inline GoUctBoard::StoneIterator::StoneIterator(const GoUctBoard& bd,
                                                SgPoint p)
    : m_it(bd.m_block[p]->Stones()),
      m_board(bd)
{
    SG_ASSERT(m_board.Occupied(p));
}

inline void GoUctBoard::StoneIterator::operator++()
{
    ++m_it;
}

inline int GoUctBoard::StoneIterator::operator*() const
{
    return *m_it;
}

inline GoUctBoard::StoneIterator::operator bool() const
{
    return m_it;
}

inline SgPoint GoUctBoard::Anchor(SgPoint p) const
{
    SG_ASSERT(Occupied(p));
    return m_block[p]->Anchor();
}

inline bool GoUctBoard::AreInSameBlock(SgPoint p1, SgPoint p2) const
{
    return Occupied(p1) && Occupied(p2) && Anchor(p1) == Anchor(p2);
}

inline bool GoUctBoard::AtLeastNumLibs(SgPoint block, int n) const
{
    return NumLiberties(block) >= n;
}

inline bool GoUctBoard::AtMostNumLibs(SgPoint block, int n) const
{
    return NumLiberties(block) <= n;
}

inline const SgPointSList& GoUctBoard::CapturedStones() const
{
    return m_capturedStones;
}

inline bool GoUctBoard::CapturingMove() const
{
    return ! m_capturedStones.IsEmpty();
}

inline int GoUctBoard::FirstBoardPoint() const
{
    return m_const.FirstBoardPoint();
}

inline const SgBoardConst& GoUctBoard::BoardConst() const
{
    return m_const;
}

inline SgPoint GoUctBoard::Get2ndLastMove() const
{
    return m_secondLastMove;
}

inline SgBoardColor GoUctBoard::GetColor(SgPoint p) const
{
    return m_color[p];
}

inline SgPoint GoUctBoard::GetLastMove() const
{
    return m_lastMove;
}

inline SgBlackWhite GoUctBoard::GetStone(SgPoint p) const
{
    SG_ASSERT(Occupied(p));
    return m_color[p];
}

inline bool GoUctBoard::HasDiagonals(SgPoint p, SgBoardColor c) const
{
    return (IsColor(p - SG_NS - SG_WE, c)
            || IsColor(p - SG_NS + SG_WE, c)
            || IsColor(p + SG_NS - SG_WE, c)
            || IsColor(p + SG_NS + SG_WE, c));
}

inline bool GoUctBoard::HasEmptyDiagonals(SgPoint p) const
{
    return HasDiagonals(p, SG_EMPTY);
}

inline bool GoUctBoard::HasEmptyNeighbors(SgPoint p) const
{
    return m_nuNeighborsEmpty[p] != 0;
}

inline bool GoUctBoard::HasLiberties(SgPoint p) const
{
    return NumLiberties(p) > 0;
}

inline bool GoUctBoard::HasNeighbors(SgPoint p, int c) const
{
    return (m_nuNeighbors[c][p] > 0);
}

inline bool GoUctBoard::HasNeighborsOrDiags(SgPoint p, SgBoardColor c) const
{
    return HasNeighbors(p, c) || HasDiagonals(p, c);
}

inline bool GoUctBoard::InAtari(SgPoint p) const
{
    SG_ASSERT(Occupied(p));
    return AtMostNumLibs(p, 1);
}

inline bool GoUctBoard::IsInBlock(SgPoint p, SgPoint anchor) const
{
    SG_ASSERT(Occupied(anchor));
    const Block* b = m_block[p];
    return (b != 0 && b->Anchor() == anchor);
}

inline bool GoUctBoard::IsLibertyOfBlock(SgPoint p, SgPoint anchor) const
{
    SG_ASSERT(IsEmpty(p));
    SG_ASSERT(Occupied(anchor));
    SG_ASSERT(Anchor(anchor) == anchor);
    const Block* b = m_block[anchor];
    if (m_nuNeighbors[b->Color()][p] == 0)
        return false;
    return (   m_block[p - SG_NS] == b
            || m_block[p - SG_WE] == b
            || m_block[p + SG_WE] == b
            || m_block[p + SG_NS] == b);
}

inline bool GoUctBoard::CanCapture(SgPoint p, SgBlackWhite c) const
{
    SgBlackWhite opp = OppBW(c);
    for (SgNb4Iterator nb(p); nb; ++nb)
        if (IsColor(*nb, opp) && AtMostNumLibs(*nb, 1))
            return true;
    return false;
}

inline bool GoUctBoard::IsSuicide(SgPoint p, SgBlackWhite toPlay) const
{
    if (HasEmptyNeighbors(p))
        return false;
    SgBlackWhite opp = OppBW(toPlay);
    for (SgNb4Iterator it(p); it; ++it)
    {
        if (IsBorder(*it))
            continue;
        SgEmptyBlackWhite c = GetColor(*it);
        if (c == toPlay && NumLiberties(*it) > 1)
            return false;
        if (c == opp && NumLiberties(*it) == 1)
            return false;
    }
    return true;
}

inline bool GoUctBoard::IsBorder(SgPoint p) const
{
    SG_ASSERT(p != SG_PASS);
    return m_isBorder[p];
}

inline bool GoUctBoard::IsColor(SgPoint p, int c) const
{
    SG_ASSERT(p != SG_PASS);
    SG_ASSERT(IsEmptyBlackWhite(c));
    return m_color[p] == c;
}

inline bool GoUctBoard::IsEmpty(SgPoint p) const
{
    SG_ASSERT(p != SG_PASS);
    return m_color[p] == SG_EMPTY;
}

inline bool GoUctBoard::IsLegal(int p, SgBlackWhite player) const
{
    SG_ASSERT_BW(player);
    if (p == SG_PASS)
        return true;
    SG_ASSERT(SgPointUtil::InBoardRange(p));
    if (! IsEmpty(p))
        return false;
    // Suicide
    if (IsSuicide(p, player))
        return false;
    // Repetition
    if (p == m_koPoint && m_toPlay == player)
        return false;
    return true;
}

inline bool GoUctBoard::IsLegal(int p) const
{
    return IsLegal(p, ToPlay());
}

inline bool GoUctBoard::IsSingleStone(SgPoint p) const
{
    return (Occupied(p) && NumNeighbors(p, GetColor(p)) == 0);
}

inline bool GoUctBoard::IsSuicide(SgPoint p) const
{
    return IsSuicide(p, ToPlay());
}

inline bool GoUctBoard::IsValidPoint(SgPoint p) const
{
    return SgPointUtil::InBoardRange(p) && ! IsBorder(p);
}

inline int GoUctBoard::LastBoardPoint() const
{
    return m_const.LastBoardPoint();
}

inline int GoUctBoard::Left(SgPoint p) const
{
    return m_const.Left(p);
}

inline SgGrid GoUctBoard::Line(SgPoint p) const
{
    return m_const.Line(p);
}

inline int GoUctBoard::Num8Neighbors(SgPoint p, int c) const
{
    return NumNeighbors(p, c) + NumDiagonals(p, c);
}

inline int GoUctBoard::Num8EmptyNeighbors(SgPoint p) const
{
    return NumEmptyNeighbors(p) + NumEmptyDiagonals(p);
}

inline int GoUctBoard::NuCapturedStones() const
{
    return m_capturedStones.Length();
}

inline int GoUctBoard::NumDiagonals(SgPoint p, int c) const
{
    int n = 0;
    if (IsColor(p - SG_NS - SG_WE, c))
        ++n;
    if (IsColor(p - SG_NS + SG_WE, c))
        ++n;
    if (IsColor(p + SG_NS - SG_WE, c))
        ++n;
    if (IsColor(p + SG_NS + SG_WE, c))
        ++n;
    return n;
}

inline int GoUctBoard::NumEmptyDiagonals(SgPoint p) const
{
    return NumDiagonals(p, SG_EMPTY);
}

inline int GoUctBoard::NumEmptyNeighbors(SgPoint p) const
{
    return m_nuNeighborsEmpty[p];
}

inline int GoUctBoard::NumLiberties(SgPoint p) const
{
    SG_ASSERT(IsValidPoint(p));
    SG_ASSERT(Occupied(p));
    return m_block[p]->NumLiberties();
}

inline int GoUctBoard::NumNeighbors(SgPoint p, int c) const
{
    return m_nuNeighbors[c][p];
}

inline int GoUctBoard::NumPrisoners(SgBlackWhite color) const
{
    return m_prisoners[color];
}

inline int GoUctBoard::NumStones(SgPoint block) const
{
    SG_ASSERT(Occupied(block));
    return m_block[block]->NumStones();
}

inline bool GoUctBoard::Occupied(SgPoint p) const
{
    return (m_block[p] != 0);
}

inline bool GoUctBoard::OccupiedInAtari(SgPoint p) const
{
    const Block* b = m_block[p];
    return (b != 0 && b->NumLiberties() <= 1);
}

inline SgBlackWhite GoUctBoard::Opponent() const
{
    return OppBW(m_toPlay);
}

inline void GoUctBoard::Play(GoPlayerMove move)
{
    Play(move.Point(), move.Color());
}

inline void GoUctBoard::Play(SgPoint p)
{
    Play(p, ToPlay());
}

inline SgGrid GoUctBoard::Pos(SgPoint p) const
{
    return m_const.Pos(p);
}

inline int GoUctBoard::Right(SgPoint p) const
{
    return m_const.Right(p);
}

inline void GoUctBoard::SetToPlay(SgBlackWhite player)
{
    SG_ASSERT_BW(player);
    m_toPlay = player;
}

inline int GoUctBoard::Side(SgPoint p, int index) const
{
    return m_const.Side(p, index);
}

inline SgGrid GoUctBoard::Size() const
{
    return m_size;
}

inline SgPoint GoUctBoard::TheLiberty(SgPoint p) const
{
    SG_ASSERT(Occupied(p));
    SG_ASSERT(NumLiberties(p) == 1);
    return m_block[p]->Liberties()[0];
}

inline SgBlackWhite GoUctBoard::ToPlay() const
{
    return m_toPlay;
}

inline int GoUctBoard::Up(SgPoint p) const
{
    return m_const.Up(p);
}

//----------------------------------------------------------------------------

#endif // GOUCT_BOARD_H
