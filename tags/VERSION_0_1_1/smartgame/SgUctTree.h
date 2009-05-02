//----------------------------------------------------------------------------
/** @file SgUctTree.h
    Class SgUctTree and strongly related classes.
*/
//----------------------------------------------------------------------------

#ifndef SG_UCTTREE_H
#define SG_UCTTREE_H

#include <stack>
#include <boost/shared_ptr.hpp>
#include "SgMove.h"
#include "SgStatistics.h"

//----------------------------------------------------------------------------

typedef SgStatisticsBase<float,std::size_t> SgUctStatisticsBase;

typedef SgStatisticsBase<volatile float,volatile std::size_t>
    SgUctStatisticsBaseVolatile;

//----------------------------------------------------------------------------

/** Node used in SgUctTree. */
class SgUctNode
{
public:
    SgUctNode(SgMove move);

    /** Add game result.
        @param eval The game result (e.g. score or 0/1 for win loss)
    */
    void AddGameResult(float eval);

    /** Number of times this node was visited.
        This corresponds to the sum of MoveCount() of all children.
        It can be different from MoveCount() of this position, if prior
        knowledge initialization of the children is used.
    */
    std::size_t PosCount() const;

    /** Number of times the move leading to this position was chosen.
        This count will be different from PosCount(), if prior knowledge
        initialization is used.
    */
    std::size_t MoveCount() const;

    /** Get first child.
        @note This information is an implementation detail of how SgUctTree
        manages nodes. Use SgUctChildIterator to access children nodes.
    */
    const SgUctNode* FirstChild() const;

    /** Does the node have at least one child? */
    bool HasChildren() const;

    /** Average game result. */
    float Mean() const;

    /** Get number of children.
        @note This information is an implementation detail of how SgUctTree
        manages nodes. Use SgUctChildIterator to access children nodes.
    */
    int NuChildren() const;

    /** See FirstChild() */
    void SetFirstChild(const SgUctNode* child);

    /** See NuChildren() */
    void SetNuChildren(int nuChildren);

    /** Increment the position count.
        See PosCount()
    */
    void IncPosCount();

    /** Increment the position count.
        See PosCount()
    */
    void IncPosCount(std::size_t count);

    void SetPosCount(std::size_t value);

    /** Initialize value with prior knowledge. */
    void InitializeValue(float value, std::size_t count);

    /** Copy data from other node.
        Copies all data, apart from the children information (first child
        and number of children).
    */
    void CopyDataFrom(const SgUctNode& node);

    /** Get move.
        Requires: Node has a move (is not root node)
    */
    SgMove Move() const;

    /** Get RAVE count.
        @see SgUctSearch::Rave().
    */
    std::size_t RaveCount() const;

    /** Get RAVE mean value.
        @see SgUctSearch::Rave().
    */
    float RaveValue() const;

    /** Add a game result value to the RAVE value.
        @see SgUctSearch::Rave().
    */
    void AddRaveValue(float value);

    /** Initialize RAVE value with prior knowledge. */
    void InitializeRaveValue(float value, std::size_t count);

    /** See SgUctSearch::UseSignatures() */
    std::size_t Signature() const;

    /** See SgUctSearch::UseSignatures() */
    void SetSignature(std::size_t sig);

private:
    SgUctStatisticsBaseVolatile m_statistics;

    const SgUctNode* volatile m_firstChild;

    volatile int m_nuChildren;

    volatile SgMove m_move;

    SgUctStatisticsBaseVolatile m_raveValue;

    volatile std::size_t m_posCount;

    volatile std::size_t m_signature;
};

inline SgUctNode::SgUctNode(SgMove move)
    : m_nuChildren(0),
      m_move(move),
      m_posCount(0),
      m_signature(std::numeric_limits<size_t>::max())
{
    // m_firstChild is not initialized, only defined in m_nuChildren > 0
}

inline void SgUctNode::AddGameResult(float eval)
{
    m_statistics.Add(eval);
}

inline void SgUctNode::AddRaveValue(float value)
{
    m_raveValue.Add(value);
}

inline void SgUctNode::CopyDataFrom(const SgUctNode& node)
{
    m_statistics = node.m_statistics;
    m_move = node.m_move;
    m_raveValue = node.m_raveValue;
    m_posCount = node.m_posCount;
}

inline const SgUctNode* SgUctNode::FirstChild() const
{
    SG_ASSERT(HasChildren()); // Otherwise m_firstChild is undefined
    return m_firstChild;
}

inline bool SgUctNode::HasChildren() const
{
    return (m_nuChildren > 0);
}

inline void SgUctNode::IncPosCount()
{
    ++m_posCount;
}

inline void SgUctNode::IncPosCount(std::size_t count)
{
    m_posCount += count;
}

inline void SgUctNode::InitializeValue(float value, std::size_t count)
{
    m_statistics.Initialize(value, count);
}

inline void SgUctNode::InitializeRaveValue(float value, std::size_t count)
{
    SG_ASSERT(count > 0);
    m_raveValue.Initialize(value, count);
}

inline float SgUctNode::Mean() const
{
    SG_ASSERT(m_statistics.Count() > 0);
    return m_statistics.Mean();
}

inline SgMove SgUctNode::Move() const
{
    SG_ASSERT(m_move != SG_NULLMOVE);
    return m_move;
}

inline std::size_t SgUctNode::MoveCount() const
{
    return m_statistics.Count();
}

inline int SgUctNode::NuChildren() const
{
    return m_nuChildren;
}

inline std::size_t SgUctNode::PosCount() const
{
    return m_posCount;
}

inline std::size_t SgUctNode::RaveCount() const
{
    return m_raveValue.Count();
}

inline float SgUctNode::RaveValue() const
{
    SG_ASSERT(RaveCount() > 0);
    return m_raveValue.Mean();
}

inline void SgUctNode::SetFirstChild(const SgUctNode* child)
{
    m_firstChild = child;
}

inline void SgUctNode::SetNuChildren(int nuChildren)
{
    SG_ASSERT(nuChildren >= 0);
    m_nuChildren = nuChildren;
}

inline void SgUctNode::SetPosCount(std::size_t value)
{
    m_posCount = value;
}

inline void SgUctNode::SetSignature(std::size_t sig)
{
    m_signature = sig;
}

inline std::size_t SgUctNode::Signature() const
{
    return m_signature;
}

//----------------------------------------------------------------------------

/** Allocater for nodes used in the implementation of SgUctTree.
    Each thread has its own node allocator to allow lock-free usage of
    SgUctTree.
*/
class SgUctAllocator
{
public:
    /** The array to allocate nodes from.
        This vector changes its capacity only when SetMaxNodes() is called.
        Therefore, pointers and references to its elements (as used in
        members of SgUctNode) are never invalidated.
    */
    std::vector<SgUctNode> m_nodes;

    void Clear();

    /** Does the allocator have the capacity for n more nodes? */
    bool HasCapacity(std::size_t n) const;

    std::size_t NuNodes() const;

    std::size_t MaxNodes() const;

    void SetMaxNodes(std::size_t maxNodes);

    /** Check if allocator contains node.
        Only used for assertions. May not be available in future
        implementations. Also not portable, because the result of comparisons
        of pointers belonginf to different variables is undefined.
    */
    bool Contains(const SgUctNode& node) const;

    void Swap(SgUctAllocator& allocator);

private:
    /** Not implemented.
        Cannot be copied because vector contains pointers to elements.
        Use Swap() instead.
    */
    SgUctAllocator& operator=(const SgUctAllocator& tree);
};

inline void SgUctAllocator::Clear()
{
    m_nodes.clear();
}

inline bool SgUctAllocator::HasCapacity(std::size_t n) const
{
    return (m_nodes.size() + n <= m_nodes.capacity());
}

inline std::size_t SgUctAllocator::MaxNodes() const
{
    return m_nodes.capacity();
}

inline std::size_t SgUctAllocator::NuNodes() const
{
    return m_nodes.size();
}

inline void SgUctAllocator::SetMaxNodes(std::size_t maxNodes)
{
    // Swap with a new vector to ensure that size shrinks if new maxNodes is
    // smaller than before
    std::vector<SgUctNode> nodes;
    nodes.reserve(maxNodes);
    m_nodes.swap(nodes);
}

//----------------------------------------------------------------------------

/** Tree used in SgUctSearch.
    The nodes can be accessed only by getting non-const references or modified
    through accessor functions of SgUctTree, therefore SgUctTree can guarantee
    the integrity of the tree structure.
    The tree can be used in a lock-free way during a search (see
    @ref sguctsearchlockfree).
*/
class SgUctTree
{
public:
    friend class SgUctChildIterator;

    /** Constructor.
        Construct a tree. Before using the tree, CreateAllocators() and
        SetMaxNodes() must be called (in this order).
    */
    SgUctTree();

    /** Create node allocators for threads. */
    void CreateAllocators(std::size_t nuThreads);

    /** Add a game result.
        @param node The node.
        @param father The father (if not root) to update the position count.
        @param eval
    */
    void AddGameResult(const SgUctNode& node, const SgUctNode* father,
                       float eval);

    void Clear();

    /** Return the current maximum number of nodes.
        This returns the maximum number of nodes as set by SetMaxNodes().
        See SetMaxNodes() why the real maximum number of nodes can be higher
        or lower.
    */
    std::size_t MaxNodes() const;

    /** Change maximum number of nodes.
        Also clears the tree. This will call SetMaxNodes() at each registered
        allocator with maxNodes / numberAllocators as an argument. The real
        maximum number of nodes can be higher (because the root node is
        owned by this class, not an allocator) or lower (if maxNodes is not
        a multiple of the number of allocators).
        @param maxNodes Maximum number of nodes
    */
    void SetMaxNodes(std::size_t maxNodes);

    /** Swap content with another tree.
        The other tree must have the same number of allocators and
        the same maximum number of nodes.
    */
    void Swap(SgUctTree& tree);

    bool HasCapacity(std::size_t allocatorId, std::size_t n) const;

    /** Create children nodes.
        Requires: Allocator(allocatorId).HasCapacity(moves.size())
    */
    void CreateChildren(std::size_t allocatorId, const SgUctNode& node,
                        const std::vector<SgMove>& moves);

    /** Extract subtree to a different tree.
        @param[out] target The resulting subtree. Must have the same maximum
        number of nodes. Will be cleared before using.
        @param node The start node of the subtree.
    */
    void ExtractSubtree(SgUctTree& target, const SgUctNode& node) const;

    const SgUctNode& Root() const;

    std::size_t NuAllocators() const;

    /** Total number of nodes.
        Includes the sum of nodes in all allocators plus the root node.
    */
    std::size_t NuNodes() const;

    /** Number of nodes in one of the allocators. */
    std::size_t NuNodes(std::size_t allocatorId) const;

    /** Add a game result value to the RAVE value of a node.
        @param node The node with the move
        @param value
        @see SgUctSearch::Rave().
    */
    void AddRaveValue(const SgUctNode& node, float value);

    void InitializeValue(const SgUctNode& node, const SgUctNode& child,
                         float value, std::size_t count);

    /** Initialize the rave value and count of a move node with prior
        knowledge.
    */
    void InitializeRaveValue(const SgUctNode& node, float value,
                             std::size_t count);

    void SetSignature(const SgUctNode& node, std::size_t sig);

    /** Remove some children of a node according to a list of filtered moves.
        Requires: Allocator(allocatorId).HasCapacity(node.NuChildren()) <br>
        For efficiency, no reorganization of the tree is done to remove
        the dead subtrees (and NuNodes() will not report the real number of
        nodes in the tree).
    */
    void ApplyFilter(std::size_t allocatorId, const SgUctNode& node,
                     const std::vector<SgMove>& rootFilter);

private:
    std::size_t m_maxNodes;

    SgUctNode m_root;

    /** Allocators.
        The elements are owned by the vector (shared_ptr is only used because
        auto_ptr should not be used with standard containers)
    */
    std::vector<boost::shared_ptr<SgUctAllocator> > m_allocators;

    /** Not implemented.
        Cannot be copied because allocators contain pointers to elements.
        Use SgUctTree::Swap instead.
    */
    SgUctTree& operator=(const SgUctTree& tree);

    SgUctAllocator& Allocator(std::size_t i);

    const SgUctAllocator& Allocator(std::size_t i) const;

    bool Contains(const SgUctNode& node) const;

    void CopySubtree(SgUctTree& target, SgUctNode& targetNode,
                     const SgUctNode& node,
                     std::size_t& currentAllocatorId) const;
};

inline void SgUctTree::AddGameResult(const SgUctNode& node,
                                     const SgUctNode* father, float eval)
{
    SG_ASSERT(Contains(node));
    // Parameters are const-references, because only the tree is allowed
    // to modify nodes
    if (father != 0)
        const_cast<SgUctNode*>(father)->IncPosCount();
    const_cast<SgUctNode&>(node).AddGameResult(eval);
}

inline void SgUctTree::AddRaveValue(const SgUctNode& node, float value)
{
    SG_ASSERT(Contains(node));
    // Parameters are const-references, because only the tree is allowed
    // to modify nodes
    const_cast<SgUctNode&>(node).AddRaveValue(value);
}

inline SgUctAllocator& SgUctTree::Allocator(std::size_t i)
{
    SG_ASSERT(i < m_allocators.size());
    return *m_allocators[i];
}

inline const SgUctAllocator& SgUctTree::Allocator(std::size_t i) const
{
    SG_ASSERT(i < m_allocators.size());
    return *m_allocators[i];
}

inline bool SgUctTree::HasCapacity(std::size_t allocatorId,
                                   std::size_t n) const
{
    return Allocator(allocatorId).HasCapacity(n);
}

inline void SgUctTree::InitializeValue(const SgUctNode& node,
                                       const SgUctNode& child,
                                       float value, std::size_t count)
{
    SG_ASSERT(Contains(node));
    SG_ASSERT(Contains(child));
    // Parameters are const-references, because only the tree is allowed
    // to modify nodes
    const_cast<SgUctNode&>(node).IncPosCount(count);
    const_cast<SgUctNode&>(child).InitializeValue(value, count);
}

inline void SgUctTree::InitializeRaveValue(const SgUctNode& node,
                                           float value, std::size_t count)
{
    SG_ASSERT(Contains(node));
    // Parameters are const-references, because only the tree is allowed
    // to modify nodes
    const_cast<SgUctNode&>(node).InitializeRaveValue(value, count);
}

inline std::size_t SgUctTree::MaxNodes() const
{
    return m_maxNodes;
}

inline std::size_t SgUctTree::NuAllocators() const
{
    return m_allocators.size();
}

inline std::size_t SgUctTree::NuNodes(std::size_t allocatorId) const
{
    return Allocator(allocatorId).NuNodes();
}

inline const SgUctNode& SgUctTree::Root() const
{
    return m_root;
}

inline void SgUctTree::SetSignature(const SgUctNode& node, std::size_t sig)
{
    SG_ASSERT(Contains(node));
    // Parameters are const-references, because only the tree is allowed
    // to modify nodes
    const_cast<SgUctNode&>(node).SetSignature(sig);
}

//----------------------------------------------------------------------------

/** Iterator over all children of a node. */
class SgUctChildIterator
{
public:
    /** Constructor.
        Requires: node.HasChildren()
    */
    SgUctChildIterator(const SgUctTree& tree, const SgUctNode& node);

    const SgUctNode& operator*() const;

    void operator++();

    operator bool() const;

private:
    const SgUctNode* m_current;

    const SgUctNode* m_last;
};

inline SgUctChildIterator::SgUctChildIterator(const SgUctTree& tree,
                                              const SgUctNode& node)
{
    SG_DEBUG_ONLY(tree);
    SG_ASSERT(tree.Contains(node));
    SG_ASSERT(node.HasChildren());
    m_current = node.FirstChild();
    m_last = m_current + node.NuChildren();
}

inline const SgUctNode& SgUctChildIterator::operator*() const
{
    return *m_current;
}

inline void SgUctChildIterator::operator++()
{
    ++m_current;
}

inline SgUctChildIterator::operator bool() const
{
    return (m_current < m_last);
}

//----------------------------------------------------------------------------

/** Iterator for traversing a tree depth-first. */
class SgUctTreeIterator
{
public:
    SgUctTreeIterator(const SgUctTree& tree);

    const SgUctNode& operator*() const;

    void operator++();

    operator bool() const;

private:
    const SgUctTree& m_tree;

    const SgUctNode* m_current;

    /** Stack of child iterators.
        The elements are owned by the stack (shared_ptr is only used because
        auto_ptr should not be used with standard containers)
    */
    std::stack<boost::shared_ptr<SgUctChildIterator> > m_stack;
};

//----------------------------------------------------------------------------

#endif // SG_UCTTREE_H