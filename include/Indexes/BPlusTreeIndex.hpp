#pragma once

#ifndef _B_PLUS_TREE_INDEX_HPP_
    #define _B_PLUS_TREE_INDEX_HPP_

    #include <vector>
    #include <optional>
    #include <string>

    #include <Utilities/Utils.hpp>
    #include <Storage/BufferManager.hpp>

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

    struct BPlusTreeIterator
    {
        friend class BPlusTreeIndex;

        private:

        //
        node_id_t curr_id;
        
        //
        BPlusTreeIndex *tree;

        //
        size_t curr_index;

        //
        std::vector< std::pair< KeyType, ValueType > > data;

        //
        BPlusTreeIterator ( BPlusTreeIndex *tree, node_id_t id )
            : curr_id ( id ), tree ( tree )
        {
            if( id == -1 )
            {
                curr_index = 0;
                return;
            }
            data = tree->data( curr_id );
            curr_index = 0;
        }

        public:

        //
        std::pair< KeyType, ValueType > operator* ()
        {
            if( curr_index >= 0 && curr_index < data.size() )
            {
                return data[ curr_index ];
            }
            else
            {
                throw std::out_of_range( "Iterator out of range" );
            }
        }

        //
        auto operator++ () -> BPlusTreeIterator &
        {
            if( curr_index < data.size() )
            {
                ++curr_index;
            }
            if( curr_index >= data.size() )
            {
                BPlusTreeNode *curr = tree->loadNode( curr_id );
                curr_id = curr->nextLeaf_id;
                delete curr;
                if( curr_id != -1 )
                {
                    data = tree->data( curr_id );
                }
                curr_index = 0;
            }
            return *this;
        }

        //overload equality operator
        auto operator== ( const BPlusTreeIterator &other ) const -> bool
        {
            return ((curr_id == other.curr_id) && (curr_index == other.curr_index));
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

    //
    address_id_t base_address;
    
    public:
    
    //
    BPlusTreeIndex ( BufferManager *_bm, unsigned int _order = 4, address_id_t _base_address = 0 )
        : root_id ( -1 ), order ( std::max( 3U, _order )), last_id ( 0 ),  buffer_manager ( _bm ), base_address ( _base_address )
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
    auto rangeSearch ( KeyType start, KeyType end ) -> std::vector< std::pair< KeyType, ValueType > >;
    
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

    /**
     * @brief get the range of addresses occupied by the B+ tree in the disk
     * @return a pair of address_id_t representing the start and end address of the B+ tree in the disk
     */
    auto getAddressRange ( ) -> std::pair< address_id_t, address_id_t >
    {
        return std::make_pair( base_address, base_address + nodeSize() * last_id );
    }

    /**
     * @brief get the first leaf node of the B+ tree
     * @return id of the first leaf node
     */
    auto begin( ) -> BPlusTreeIterator;

    /**
     * @brief get the end leaf node of the B+ tree
     * @return id of the end leaf node that is -1
     */
    auto end( ) -> BPlusTreeIterator
    {
        return BPlusTreeIterator( this, -1 );
    }

    
    private:

    /**
     * @brief get the data of a leaf node
     * @param id the id of the leaf node
     * @return a vector of pairs of key and value
     */
    auto data ( node_id_t id ) -> std::vector< std::pair< KeyType, ValueType > >;

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