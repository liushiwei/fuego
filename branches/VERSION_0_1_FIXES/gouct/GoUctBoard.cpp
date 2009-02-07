//----------------------------------------------------------------------------
/** @file GoBoard.cpp
    @see GoBoard.h
*/
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "GoUctBoard.h"

#include <boost/static_assert.hpp>
#include <algorithm>
#include "GoBoardUtil.h"
#include "SgNbIterator.h"
#include "SgStack.h"

using namespace std;

//----------------------------------------------------------------------------

namespace {

/** Do a consistency check.
    Check some data structures for consistency after and before each play
    (and at some other places).
    This is an expensive check and therefore has to be enabled at compile
    time.
*/
const bool CONSISTENCY = false;

} // namespace

//----------------------------------------------------------------------------

GoUctBoard::GoUctBoard(const GoBoard& bd)
    : m_const(bd.Size())
{
    m_size = -1;
    Init(bd);
}

GoUctBoard::~GoUctBoard()
{
}

void GoUctBoard::CheckConsistency() const
{
    if (! CONSISTENCY)
        return;
    for (SgPoint p = 0; p < SG_MAXPOINT; ++p)
    {
        if (IsBorder(p))
            continue;
        int c = m_color[p];
        SG_ASSERT(IsEmptyBlackWhite(c));
        int n = 0;
        for (SgNb4Iterator it(p); it; ++it)
            if (m_color[*it] == SG_EMPTY)
                ++n;
        SG_ASSERT(n == NumEmptyNeighbors(p));
        n = 0;
        for (SgNb4Iterator it(p); it; ++it)
            if (m_color[*it] == SG_BLACK)
                ++n;
        SG_ASSERT(n == NumNeighbors(p, SG_BLACK));
        n = 0;
        for (SgNb4Iterator it(p); it; ++it)
            if (m_color[*it] == SG_WHITE)
                ++n;
        SG_ASSERT(n == NumNeighbors(p, SG_WHITE));
        if (c == SG_BLACK || c == SG_WHITE)
            CheckConsistencyBlock(p);
        if (c == SG_EMPTY)
            SG_ASSERT(m_block[p] == 0);
    }
}

void GoUctBoard::CheckConsistencyBlock(SgPoint point) const
{
    SG_ASSERT(Occupied(point));
    SgBlackWhite color = GetColor(point);
    SgPointSList stones;
    Block::LibertyList liberties;
    SgMarker mark;
    SgStack<SgPoint,SG_MAXPOINT> stack;
    stack.Push(point);
    bool anchorFound = false;
    const Block* block = m_block[point];
    while (! stack.IsEmpty())
    {
        SgPoint p = stack.Pop();
        if (IsBorder(p) || ! mark.NewMark(p))
            continue;
        if (GetColor(p) == color)
        {
            stones.Append(p);
            if (p == block->Anchor())
                anchorFound = true;
            stack.Push(p - SG_NS);
            stack.Push(p - SG_WE);
            stack.Push(p + SG_WE);
            stack.Push(p + SG_NS);
        }
        else if (GetColor(p) == SG_EMPTY)
            liberties.Append(p);
    }
    SG_ASSERT(anchorFound);
    SG_ASSERT(color == block->Color());
    SG_ASSERT(stones.SameElements(block->Stones()));
    SG_ASSERT(liberties.SameElements(block->Liberties()));
    SG_ASSERT(stones.Length() == NumStones(point));
}

void GoUctBoard::AddLibToAdjBlocks(SgPoint p, SgBlackWhite c)
{
    if (NumNeighbors(p, c) == 0)
        return;
    SgSList<Block*,4> blocks = GetAdjacentBlocks(p, c);
    for (SgSList<Block*,4>::Iterator it(blocks); it; ++it)
    {
        Block* block = *it;
        if (block != 0)
            block->AppendLiberty(p);
    }
}

void GoUctBoard::AddStoneToBlock(SgPoint p, Block* block)
{
    // Stone already placed
    SG_ASSERT(IsColor(p, block->Color()));
    block->AppendStone(p);
    if (IsEmpty(p - SG_NS) && ! IsAdjacentTo(p - SG_NS, block))
        block->AppendLiberty(p - SG_NS);
    if (IsEmpty(p - SG_WE) && ! IsAdjacentTo(p - SG_WE, block))
        block->AppendLiberty(p - SG_WE);
    if (IsEmpty(p + SG_WE) && ! IsAdjacentTo(p + SG_WE, block))
        block->AppendLiberty(p + SG_WE);
    if (IsEmpty(p + SG_NS) && ! IsAdjacentTo(p + SG_NS, block))
        block->AppendLiberty(p + SG_NS);
    m_block[p] = block;
}

void GoUctBoard::CreateSingleStoneBlock(SgPoint p, SgBlackWhite c)
{
    // Stone already placed
    SG_ASSERT(IsColor(p, c));
    SG_ASSERT(NumNeighbors(p, c) == 0);
    Block& block = m_blockArray[p];
    block.InitSingleStoneBlock(c, p);
    if (IsEmpty(p - SG_NS))
        block.AppendLiberty(p - SG_NS);
    if (IsEmpty(p - SG_WE))
        block.AppendLiberty(p - SG_WE);
    if (IsEmpty(p + SG_WE))
        block.AppendLiberty(p + SG_WE);
    if (IsEmpty(p + SG_NS))
        block.AppendLiberty(p + SG_NS);
    m_block[p] = &block;
}

SgSList<GoUctBoard::Block*,4> GoUctBoard::GetAdjacentBlocks(SgPoint p) const
{
    SgSList<Block*,4> result;
    if (NumNeighbors(p, SG_BLACK) > 0 || NumNeighbors(p, SG_WHITE) > 0)
    {
        Block* block;
        if ((block = m_block[p - SG_NS]) != 0)
            result.Append(block);
        if ((block = m_block[p - SG_WE]) != 0
            && ! result.Contains(block))
            result.Append(block);
        if ((block = m_block[p + SG_WE]) != 0
            && ! result.Contains(block))
            result.Append(block);
        if ((block = m_block[p + SG_NS]) != 0
            && ! result.Contains(block))
            result.Append(block);
    }
    return result;
}

SgSList<GoUctBoard::Block*,4>
GoUctBoard::GetAdjacentBlocks(SgPoint p, SgBlackWhite c) const
{
    SgSList<Block*,4> result;
    if (NumNeighbors(p, c) > 0)
    {
        Block* block;
        if (IsColor(p - SG_NS, c))
            result.Append(m_block[p - SG_NS]);
        if (IsColor(p - SG_WE, c)
            && ! result.Contains((block = m_block[p - SG_WE])))
            result.Append(block);
        if (IsColor(p + SG_WE, c)
            && ! result.Contains((block = m_block[p + SG_WE])))
            result.Append(block);
        if (IsColor(p + SG_NS, c)
            && ! result.Contains((block = m_block[p + SG_NS])))
            result.Append(block);
    }
    return result;
}

bool GoUctBoard::IsAdjacentTo(SgPoint p,
                              const GoUctBoard::Block* block) const
{
    return (   m_block[p - SG_NS] == block
            || m_block[p - SG_WE] == block
            || m_block[p + SG_WE] == block
            || m_block[p + SG_NS] == block);
}

void GoUctBoard::KillAdjacentOpponentBlocks(SgPoint p, SgBlackWhite opp)
{
    if (NumNeighbors(p, opp) == 0)
        return;
    if (IsColor(p - SG_NS, opp) && ! HasLiberties(p - SG_NS))
        KillBlock(p - SG_NS);
    if (IsColor(p - SG_WE, opp) && ! HasLiberties(p - SG_WE))
        KillBlock(p - SG_WE);
    if (IsColor(p + SG_WE, opp) && ! HasLiberties(p + SG_WE))
        KillBlock(p + SG_WE);
    if (IsColor(p + SG_NS, opp) && ! HasLiberties(p + SG_NS))
        KillBlock(p + SG_NS);
}

void GoUctBoard::MergeBlocks(SgPoint p, const SgSList<Block*,4>& adjBlocks)
{
    // Stone already placed
    SG_ASSERT(IsColor(p, adjBlocks[0]->Color()));
    SG_ASSERT(NumNeighbors(p, adjBlocks[0]->Color()) > 1);
    Block* largestBlock = 0;
    int largestBlockStones = 0;
    for (SgSList<Block*,4>::Iterator it(adjBlocks); it; ++it)
    {
        Block* adjBlock = *it;
        int numStones = adjBlock->NumStones();
        if (numStones > largestBlockStones)
        {
            largestBlockStones = numStones;
            largestBlock = adjBlock;
        }
    }
    largestBlock->AppendStone(p);
    SgReserveMarker reserve(m_marker);
    m_marker.Clear();
    for (Block::LibertyIterator lib(largestBlock->Liberties()); lib; ++lib)
        m_marker.Include(*lib);
    for (SgSList<Block*,4>::Iterator it(adjBlocks); it; ++it)
    {
        Block* adjBlock = *it;
        if (adjBlock == largestBlock)
            continue;
        for (Block::StoneIterator stn(adjBlock->Stones()); stn; ++stn)
        {
            largestBlock->AppendStone(*stn);
            m_block[*stn] = largestBlock;
        }
        for (Block::LibertyIterator lib(adjBlock->Liberties()); lib; ++lib)
            if (m_marker.NewMark(*lib))
                largestBlock->AppendLiberty(*lib);
    }
    m_block[p] = largestBlock;
    if (IsEmpty(p - SG_NS) && m_marker.NewMark(p - SG_NS))
        largestBlock->AppendLiberty(p - SG_NS);
    if (IsEmpty(p - SG_WE) && m_marker.NewMark(p - SG_WE))
        largestBlock->AppendLiberty(p - SG_WE);
    if (IsEmpty(p + SG_WE) && m_marker.NewMark(p + SG_WE))
        largestBlock->AppendLiberty(p + SG_WE);
    if (IsEmpty(p + SG_NS) && m_marker.NewMark(p + SG_NS))
        largestBlock->AppendLiberty(p + SG_NS);
}

void GoUctBoard::RemoveLibFromAdjBlocks(SgPoint p)
{
    if (NumNeighbors(p, SG_BLACK) == 0 && NumNeighbors(p, SG_WHITE) == 0)
        return;
    SgSList<Block*,4> blocks = GetAdjacentBlocks(p);
    for (SgSList<Block*,4>::Iterator it(blocks); it; ++it)
        (*it)->ExcludeLiberty(p);
}

void GoUctBoard::RemoveLibFromAdjBlocks(SgPoint p, SgBlackWhite c)
{
    if (NumNeighbors(p, c) == 0)
        return;
    SgSList<Block*,4> blocks = GetAdjacentBlocks(p, c);
    for (SgSList<Block*,4>::Iterator it(blocks); it; ++it)
        (*it)->ExcludeLiberty(p);
}

void GoUctBoard::UpdateBlocksAfterAddStone(SgPoint p, SgBlackWhite c)
{
    // Stone already placed
    SG_ASSERT(IsColor(p, c));
    if (NumNeighbors(p, c) == 0)
        CreateSingleStoneBlock(p, c);
    else
    {
        SgSList<Block*,4> adjBlocks = GetAdjacentBlocks(p, c);
        if (adjBlocks.Length() == 1)
            AddStoneToBlock(p, adjBlocks[0]);
        else
            MergeBlocks(p, adjBlocks);
    }
}

void GoUctBoard::Init(const GoBoard& bd)
{
    if (bd.Size() != m_size)
        InitSize(bd);
    m_prisoners[SG_BLACK] = bd.NumPrisoners(SG_BLACK);
    m_prisoners[SG_WHITE] = bd.NumPrisoners(SG_WHITE);
    m_koPoint = bd.KoPoint();
    m_lastMove = bd.GetLastMove();
    m_secondLastMove = bd.Get2ndLastMove();
    m_toPlay = bd.ToPlay();
    for (GoBoard::Iterator it(bd); it; ++it)
    {
        SgPoint p = *it;
        SgBoardColor c = bd.GetColor(p);
        m_color[p] = c;
        m_nuNeighbors[SG_BLACK][p] = bd.NumNeighbors(p, SG_BLACK);
        m_nuNeighbors[SG_WHITE][p] = bd.NumNeighbors(p, SG_WHITE);
        m_nuNeighborsEmpty[p] = bd.NumEmptyNeighbors(p);
        if (bd.IsEmpty(p))
            m_block[p] = 0;
        else if (bd.Anchor(p) == p)
        {
            SgBoardColor c = m_color[p];
            Block& block = m_blockArray[p];
            block.InitNewBlock(c, p);
            for (GoBoard::StoneIterator it2(bd, p); it2; ++it2)
            {
                block.AppendStone(*it2);
                m_block[*it2] = &block;
            }
            for (GoBoard::LibertyIterator it2(bd, p); it2; ++it2)
                block.AppendLiberty(*it2);
        }
    }
    CheckConsistency();
}

void GoUctBoard::InitSize(const GoBoard& bd)
{
    m_size = bd.Size();
    m_nuNeighbors[SG_BLACK].Fill(0);
    m_nuNeighbors[SG_WHITE].Fill(0);
    m_nuNeighborsEmpty.Fill(0);
    m_block.Fill(0);
    for (SgPoint p = 0; p < SG_MAXPOINT; ++p)
    {
        if (bd.IsBorder(p))
        {
            m_color[p] = 0;
            m_isBorder[p] = true;
        }
        else
            m_isBorder[p] = false;
    }
    m_const.ChangeSize(m_size);
}

int GoUctBoard::AdjacentBlocks(SgPoint point, int maxLib, SgPoint anchors[],
                               int maxAnchors) const
{
    SG_DEBUG_ONLY(maxAnchors);
    SG_ASSERT(Occupied(point));
    const SgBlackWhite other = OppBW(GetStone(point));
    int n = 0;
    SgReserveMarker reserve(m_marker);
    SG_UNUSED(reserve);
    m_marker.Clear();
    for (StoneIterator it(*this, point); it; ++it)
    {
        if (NumNeighbors(*it, other) > 0)
        {
            SgPoint p = *it;
            if (IsColor(p - SG_NS, other)
                && m_marker.NewMark(Anchor(p - SG_NS))
                && AtMostNumLibs(p - SG_NS, maxLib))
                anchors[n++] = Anchor(p - SG_NS);
            if (IsColor(p - SG_WE, other)
                && m_marker.NewMark(Anchor(p - SG_WE))
                && AtMostNumLibs(p - SG_WE, maxLib))
                anchors[n++] = Anchor(p - SG_WE);
            if (IsColor(p + SG_WE, other)
                && m_marker.NewMark(Anchor(p + SG_WE))
                && AtMostNumLibs(p + SG_WE, maxLib))
                anchors[n++] = Anchor(p + SG_WE);
            if (IsColor(p + SG_NS, other)
                && m_marker.NewMark(Anchor(p + SG_NS))
                && AtMostNumLibs(p + SG_NS, maxLib))
                anchors[n++] = Anchor(p + SG_NS);
        }
    };
    // Detect array overflow.
    SG_ASSERT(n < maxAnchors);
    anchors[n] = SG_ENDPOINT;
    return n;
}

void GoUctBoard::NeighborBlocks(SgPoint p, SgBlackWhite c,
                                SgPoint anchors[]) const
{
    SG_ASSERT(IsEmpty(p));
    SgReserveMarker reserve(m_marker);
    SG_UNUSED(reserve);
    m_marker.Clear();
    int i = 0;
    if (NumNeighbors(p, c) > 0)
    {
        if (IsColor(p - SG_NS, c) && m_marker.NewMark(Anchor(p - SG_NS)))
            anchors[i++] = Anchor(p - SG_NS);
        if (IsColor(p - SG_WE, c) && m_marker.NewMark(Anchor(p - SG_WE)))
            anchors[i++] = Anchor(p - SG_WE);
        if (IsColor(p + SG_WE, c) && m_marker.NewMark(Anchor(p + SG_WE)))
            anchors[i++] = Anchor(p + SG_WE);
        if (IsColor(p + SG_NS, c) && m_marker.NewMark(Anchor(p + SG_NS)))
            anchors[i++] = Anchor(p + SG_NS);
    }
    anchors[i] = SG_ENDPOINT;
}

void GoUctBoard::NeighborBlocks(SgPoint p, SgBlackWhite c, int maxLib,
                                SgPoint anchors[]) const
{
    SG_ASSERT(IsEmpty(p));
    SgReserveMarker reserve(m_marker);
    SG_UNUSED(reserve);
    m_marker.Clear();
    int i = 0;
    if (NumNeighbors(p, c) > 0)
    {
        if (IsColor(p - SG_NS, c) && m_marker.NewMark(Anchor(p - SG_NS))
            && AtMostNumLibs(p - SG_NS, maxLib))
            anchors[i++] = Anchor(p - SG_NS);
        if (IsColor(p - SG_WE, c) && m_marker.NewMark(Anchor(p - SG_WE))
            && AtMostNumLibs(p - SG_WE, maxLib))
            anchors[i++] = Anchor(p - SG_WE);
        if (IsColor(p + SG_WE, c) && m_marker.NewMark(Anchor(p + SG_WE))
            && AtMostNumLibs(p + SG_WE, maxLib))
            anchors[i++] = Anchor(p + SG_WE);
        if (IsColor(p + SG_NS, c) && m_marker.NewMark(Anchor(p + SG_NS))
            && AtMostNumLibs(p + SG_NS, maxLib))
            anchors[i++] = Anchor(p + SG_NS);
    }
    anchors[i] = SG_ENDPOINT;
}

void GoUctBoard::AddStone(SgPoint p, SgBlackWhite c)
{
    SG_ASSERT(IsEmpty(p));
    SG_ASSERT_BW(c);
    m_color[p] = c;
    --m_nuNeighborsEmpty[p - SG_NS];
    --m_nuNeighborsEmpty[p - SG_WE];
    --m_nuNeighborsEmpty[p + SG_WE];
    --m_nuNeighborsEmpty[p + SG_NS];
    SgArray<int,SG_MAXPOINT>& nuNeighbors = m_nuNeighbors[c];
    ++nuNeighbors[p - SG_NS];
    ++nuNeighbors[p - SG_WE];
    ++nuNeighbors[p + SG_WE];
    ++nuNeighbors[p + SG_NS];
}

void GoUctBoard::RemoveStone(SgPoint p)
{
    SgBlackWhite c = GetStone(p);
    SG_ASSERT_BW(c);
    m_color[p] = SG_EMPTY;
    ++m_nuNeighborsEmpty[p - SG_NS];
    ++m_nuNeighborsEmpty[p - SG_WE];
    ++m_nuNeighborsEmpty[p + SG_WE];
    ++m_nuNeighborsEmpty[p + SG_NS];
    SgArray<int,SG_MAXPOINT>& nuNeighbors = m_nuNeighbors[c];
    --nuNeighbors[p - SG_NS];
    --nuNeighbors[p - SG_WE];
    --nuNeighbors[p + SG_WE];
    --nuNeighbors[p + SG_NS];
}

void GoUctBoard::KillBlock(SgPoint p)
{
    Block* block = m_block[p];
    SgBlackWhite c = GetColor(p);
    SgBlackWhite opp = OppBW(c);
    for (Block::StoneIterator it(block->Stones()); it; ++it)
    {
        SgPoint stn = *it;
        AddLibToAdjBlocks(stn, opp);
        RemoveStone(stn);
        m_capturedStones.Append(stn);
        m_block[stn] = 0;
    }
    int nuStones = block->Stones().Length();
    m_prisoners[c] += nuStones;
    if (nuStones == 1)
        // Remember that single stone was captured, check conditions on
        // capturing block later
        m_koPoint = p;
}

void GoUctBoard::Play(SgPoint p, SgBlackWhite player)
{
    SG_ASSERT(p >= 0); // No special move, see SgMove
    SG_ASSERT_BW(player);
    SG_ASSERT(p == SG_PASS || (IsValidPoint(p) && IsEmpty(p)));
    CheckConsistency();
    m_koPoint = SG_NULLPOINT;
    m_capturedStones.Clear();
    SgBlackWhite opp = OppBW(player);
    if (p == SG_PASS)
    {
        m_toPlay = opp;
        return;
    }
    AddStone(p, player);
    RemoveLibFromAdjBlocks(p);
    KillAdjacentOpponentBlocks(p, opp);
    UpdateBlocksAfterAddStone(p, player);
    if (m_koPoint != SG_NULLPOINT)
        if (NumStones(p) > 1 || NumLiberties(p) > 1)
            m_koPoint = SG_NULLPOINT;
    if (player == m_toPlay)
    {
        m_secondLastMove = m_lastMove;
        m_lastMove = p;
    }
    else
    {
        m_secondLastMove = SG_NULLPOINT;
        m_lastMove = SG_NULLPOINT;
    }
    SG_ASSERT(HasLiberties(p)); // Suicide not supported by GoUctBoard
    m_toPlay = opp;
    CheckConsistency();
}

//----------------------------------------------------------------------------