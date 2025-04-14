#pragma once

#ifndef _B_PLUS_TREE_INDEX_HPP_
    #define _B_PLUS_TREE_INDEX_HPP_

    #include <vector>
    #include <optional>
    #include <string>

    #include <Utils.hpp>
    #include <Storage/BufferManager.hpp>


using KeyType = std::string;
using ValueType = std::string;


class BPlusTreeIndex
{
    public:

    enum class NodeType
    {
        INTERNAL,
        LEAF
    };

    struct BPlusTreeNode
    {
        //
        NodeType type;

        //
        node_id_t parent_id;

        //
        node_id_t nextLeaf_id;

        //
        std::vector< KeyType > keys;

        // For internal nodes
        // std::vector< page_id_t > children;
        std::vector< node_id_t > children;    // The size of this vector should always be keys.size() + 1

        // For leaf nodes
        std::vector< ValueType > values;

        // Child[i] < keys[i] <= Child[i+1]
        // Search keys less that keys[i] are present in Child[i] (left)
        // Search keys greater than or equal to keys[i] are present in Child[i+1] (right)

        //
        bool isLeaf () const
        {
            return type == NodeType::LEAF;
        }

        //
        BPlusTreeNode ( int order )
        {
            keys.reserve( order );
            children.reserve( order + 1 );
            values.reserve( order );
        }
    };

    //
    node_id_t root_id;

    //
    const unsigned int order;

    //
    node_id_t last_id;

    //
    std::vector< node_id_t > free_ids;

    //
    BufferManager *buffer_manager;
    
    public:
    
    //
    BPlusTreeIndex ( BufferManager *_bm, int _order = 4 )
        : root_id ( 0 ), order ( std::max( 3, _order )), last_id ( 0 ),  buffer_manager ( _bm )
    {
    }

    /**
	 * @brief 
	 * @param
	 */
    auto insert ( KeyType key, ValueType value ) -> bool;
    
    /**
	 * @brief 
	 * @param
	 */
    auto search ( KeyType key ) -> std::optional< ValueType >;
    
    /**
	 * @brief 
	 * @param
	 */
    auto rangeSearch ( KeyType start, KeyType end ) -> std::vector< ValueType >;
    
    /**
	 * @brief 
	 * @param
	 */
    auto remove ( KeyType key ) -> bool;

    /**
	 * @brief prints the B+ tree by overloading the << operator
	 * @param
	 */
    friend std::ostream &operator<< ( std::ostream &os, const BPlusTreeIndex &tree );

    public:

    //
    auto nodeSize () -> size_t
    {
        return sizeof( NodeType ) +
               sizeof( node_id_t ) * 2 +
               sizeof( size_t ) * 3 +
               sizeof( KeyType ) * order +
               sizeof( ValueType ) * order +
               sizeof( node_id_t ) * ( order + 1 );
    }

    //
    auto loadNode ( node_id_t id ) -> BPlusTreeNode *;

    //
    auto saveNode ( node_id_t id, BPlusTreeNode *node ) -> void;

    //
    auto createNode ( NodeType type ) -> node_id_t;

    //
    auto insertInternal ( node_id_t left_id, KeyType key, node_id_t right_id ) -> bool;

    //
    auto findLeaf ( KeyType key ) -> node_id_t;

    //
    auto removeEntry( node_id_t node_id, KeyType key, node_id_t ptr_id ) -> bool;

    //
    auto printBPlusTree ( std::ostream &os, node_id_t node_id, std::string prefix = "", bool last = true ) const -> void;
};

using PBPlusTreeIndex   =  BPlusTreeIndex *;
using CPBPlusTreeIndex  = const BPlusTreeIndex *;
using RBPlusTreeIndex   = BPlusTreeIndex &;
using CRBlusTreeIndex   = const BPlusTreeIndex &;

#endif      // _B_PLUS_TREE_INDEX_HPP_