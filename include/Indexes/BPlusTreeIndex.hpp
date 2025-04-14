#pragma once

#ifndef _B_PLUS_TREE_INDEX_HPP_
    #define _B_PLUS_TREE_INDEX_HPP_

    #include <vector>
    #include <optional>
    #include <string>

    #include <Utilities/Utils.hpp>
    #include <Storage/BufferManager.hpp>


// using KeyType = int;
// using ValueType = int;

template<typename KeyType, typename ValueType>
class BPlusTreeIndex
{
    private:

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
    };

    //
    node_id_t root_id;

    //
    const unsigned int order;

    // id's assigned till now
    node_id_t last_id;

    //
    std::vector< node_id_t > free_ids;

    //
    BufferManager *buffer_manager;
    
    public:
    
    //
    BPlusTreeIndex ( BufferManager *_bm, int _order = 4 )
        : root_id ( -1 ), order ( std::max( 3, _order )), last_id ( 0 ),  buffer_manager ( _bm )
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
    template<typename K, typename V>
    friend std::ostream &operator<< ( std::ostream &os, const BPlusTreeIndex<K, V> &tree );

    private:

    //
    auto nodeSize () -> size_t;

    //
    auto loadNode ( node_id_t id ) -> BPlusTreeNode *;

    //
    auto saveNode ( node_id_t id, BPlusTreeNode *node ) -> void;

    //
    auto createNode ( NodeType type ) -> node_id_t;

    //
    auto destroyNode ( node_id_t id ) -> void;

    //
    auto insertInternal ( node_id_t left_id, KeyType key, node_id_t right_id ) -> bool;

    //
    auto findLeaf ( KeyType key ) -> node_id_t;

    //
    auto removeEntry( node_id_t node_id, KeyType key, node_id_t ptr_id ) -> bool;

    //
    auto printBPlusTree ( std::ostream &os, node_id_t node_id, std::string prefix = "", bool last = true ) const -> void;
};

#endif      // _B_PLUS_TREE_INDEX_HPP_