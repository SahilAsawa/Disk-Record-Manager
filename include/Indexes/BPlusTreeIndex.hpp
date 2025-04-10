#pragma once

#ifndef _B_PLUS_TREE_INDEX_HPP_
    #define _B_PLUS_TREE_INDEX_HPP_

    #include <vector>
    #include <optional>
    #include <string>
    #include <cstdint>

using page_id_t = long long;
constexpr int ORDER = 4;        // Maximum number of children of a node
// using KeyType = uint32_t;       // Type of keys in the B+ tree
// using ValueType = page_id_t;    // Type of values in the B+ tree
using KeyType = std::string;
using ValueType = std::string;


class BPlusTreeIndex
{
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
        std::vector< KeyType > keys;

        // For internal nodes
        // std::vector< page_id_t > children;
        std::vector< BPlusTreeNode * > children;    // The size of this vector should always be keys.size() + 1

        // For leaf nodes
        std::vector< ValueType > values;

        // Child[i] < keys[i] <= Child[i+1]
        // Search keys less that keys[i] are present in Child[i] (left)
        // Search keys greater than or equal to keys[i] are present in Child[i+1] (right)

        //
        BPlusTreeNode *parent;

        //
        BPlusTreeNode *nextLeaf;

        //
        bool isLeaf () const
        {
            return type == NodeType::LEAF;
        }
    };

    //
    BPlusTreeNode *root;
    
    public:
    
    //
    BPlusTreeIndex ( )
        : root ( nullptr )
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

    private:

    //
    auto loadNode ( page_id_t id ) -> BPlusTreeNode;

    //
    auto saveNode ( const BPlusTreeNode &node ) -> void;

    //
    auto createNode ( NodeType type ) -> BPlusTreeNode *;

    //
    auto insertInternal ( BPlusTreeNode *left, KeyType key, BPlusTreeNode *right ) -> bool;

    //
    auto findLeaf ( KeyType key ) -> BPlusTreeNode *;

    //
    auto removeEntry( BPlusTreeNode *node, KeyType key, BPlusTreeNode *ptr ) -> bool;

    //
    auto printBPlusTree ( std::ostream &os, BPlusTreeNode *node, std::string prefix = "", bool last = true ) const -> void;
};

using PBPlusTreeIndex   =  BPlusTreeIndex *;
using CPBPlusTreeIndex  = const BPlusTreeIndex *;
using RBPlusTreeIndex   = BPlusTreeIndex &;
using CRBlusTreeIndex   = const BPlusTreeIndex &;

#endif      // _B_PLUS_TREE_INDEX_HPP_