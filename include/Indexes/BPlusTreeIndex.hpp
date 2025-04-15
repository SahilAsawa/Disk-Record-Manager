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
        // one of INTERNAL or LEAF
        NodeType type;

        // id of the node
        node_id_t parent_id;

        // id of the next leaf node
        node_id_t nextLeaf_id;

        // keys in the node
        std::vector< KeyType > keys;

        // For internal nodes
        // std::vector< page_id_t > children;
        std::vector< node_id_t > children;    // The size of this vector should always be keys.size() + 1

        // For leaf nodes
        std::vector< ValueType > values;

        // Child[i] < keys[i] <= Child[i+1]
        // Search keys less that keys[i] are present in Child[i] (left)
        // Search keys greater than or equal to keys[i] are present in Child[i+1] (right)

        // tells if the node is a leaf or internal node
        bool isLeaf () const
        {
            return type == NodeType::LEAF;
        }
    };

    struct BPlusTreeIterator
    {
        friend class BPlusTreeIndex;

        private:

        // id the iterator is currently pointing to
        node_id_t curr_id;
        
        // pointer to the B+ tree
        BPlusTreeIndex *tree;

        // index of the current element in the node
        size_t curr_index;

        // data of the current node
        std::vector< std::pair< KeyType, ValueType > > data;

        // constructor
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

        // overload dereference operator
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

        // overload increment operator
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

    // id of the root node
    node_id_t root_id;

    // order of the B+ tree
    const unsigned int order;

    // id's assigned till now
    node_id_t last_id;

    // free id's
    std::vector< node_id_t > free_ids;

    // buffer manager
    BufferManager *buffer_manager;

    // base address of the B+ tree in the disk
    address_id_t base_address;
    
    public:
    
    //
    BPlusTreeIndex ( BufferManager *_bm, unsigned int _order = 4, address_id_t _base_address = 0 )
        : root_id ( -1 ), order ( std::max( 3U, _order )), last_id ( 0 ),  buffer_manager ( _bm ), base_address ( _base_address )
    {
    }

    /**
	 * @brief insert a key-value pair in the B+ tree, replace the value if the key already exists
	 * @param key the key to be inserted
     * @param value the value to be inserted\
     * @return true if the key-value pair is inserted successfully, false otherwise
	 */
    auto insert ( KeyType key, ValueType value ) -> bool;
    
    /**
	 * @brief search for a key in the B+ tree
	 * @param key the key to be searched
     * @return the value associated with the key if found, std::nullopt otherwise
	 */
    auto search ( KeyType key ) -> std::optional< ValueType >;
    
    /**
	 * @brief search for a key in a range in the B+ tree
	 * @param start the key to be searched
     * @param end the key to be searched
     * @return a vector of pairs of key and value in the range [start, end]
	 */
    auto rangeSearch ( KeyType start, KeyType end ) -> std::vector< std::pair< KeyType, ValueType > >;
    
    /**
	 * @brief remove a key-value pair from the B+ tree
	 * @param key the key to be removed
     * @return true if the key-value pair is removed successfully, false otherwise
	 */
    auto remove ( KeyType key ) -> bool;

    /**
	 * @brief prints the B+ tree by overloading the << operator
	 * @param os the output stream
     * @param tree the B+ tree to be printed
     * @return the output stream
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

    /**
     * @brief get the size of a node in the B+ tree
     * @return the size of a node in bytes
     */
    auto nodeSize () -> size_t;

    /**
     * @brief get the size of a node in the B+ tree
     * @param node the node whose size is to be calculated
     * @return the size of the node in bytes
     */
    auto loadNode ( node_id_t id ) -> BPlusTreeNode *;

    /**
     * @brief load a node from the disk
     * @param id the id of the node to be loaded
     * @return a pointer to the node
     */
    auto saveNode ( node_id_t id, BPlusTreeNode *node ) -> void;

    /**
     * @brief create a new node in the B+ tree
     * @param type the type of the node to be created
     * @return the id of the new node
     */
    auto createNode ( NodeType type ) -> node_id_t;

    /**
     * @brief destroy a node in the B+ tree
     * @param id the id of the node to be destroyed
     * @return true if the node is destroyed successfully, false otherwise
     */
    auto destroyNode ( node_id_t id ) -> void;

    /**
     * @brief split a node in the B+ tree
     * @param id the id of the node to be split
     * @return the id of the new node created after splitting
     */
    auto insertInternal ( node_id_t left_id, KeyType key, node_id_t right_id ) -> bool;
    
    /**
     * @brief find the leaf node where the key should be inserted
     * @param key the key to be inserted
     * @return the id of the leaf node
     */
    auto findLeaf ( KeyType key ) -> node_id_t;

    /**
     * @brief find the index of the key in the node
     * @param node the node to be searched
     * @param key the key to be searched
     * @return the index of the key in the node
     */
    auto removeEntry( node_id_t node_id, KeyType key, node_id_t ptr_id ) -> bool;

    /**
     * @brief remove a key from the B+ tree
     * @param node_id the id of the node to be searched
     * @param key the key to be removed
     * @return true if the key is removed successfully, false otherwise
     */
    auto printBPlusTree ( std::ostream &os, node_id_t node_id, std::string prefix = "", bool last = true ) const -> void;
};

#endif      // _B_PLUS_TREE_INDEX_HPP_