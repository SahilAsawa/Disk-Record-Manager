#include <Indexes/BPlusTreeIndex.hpp>
#include <algorithm>

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::nodeSize () -> size_t
{
    int keySize = sizeof( KeyType ), valueSize = sizeof( ValueType );
    if constexpr (std::is_same_v<KeyType, std::string>) keySize = sizeof( std::byte ) * 32; // Assuming max length of string is 32
    if constexpr (std::is_same_v<ValueType, std::string>) valueSize = sizeof( std::byte ) * 32; // Assuming max length of string is 32

    return sizeof( NodeType ) +
        sizeof( node_id_t ) * 2 +
        sizeof( size_t ) * 3 +
        keySize * order +
        valueSize * order +
        sizeof( node_id_t ) * ( order + 1 );
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::loadNode ( node_id_t id ) -> BPlusTreeNode *
{
    BPlusTreeNode *node = new BPlusTreeNode;
    std::vector< std::byte > data = buffer_manager->readAddress( base_address + id * nodeSize(), nodeSize() );

    size_t curr = 0;
    node->type = *reinterpret_cast< NodeType * >( data.data() );
    curr += sizeof( NodeType );

    node->parent_id = *reinterpret_cast< node_id_t * >( data.data() + curr );
    curr += sizeof( node_id_t );

    node->nextLeaf_id = *reinterpret_cast< node_id_t * >( data.data() + curr );
    curr += sizeof( node_id_t );

    size_t size = *reinterpret_cast< size_t * >( data.data() + curr );
    curr += sizeof( size_t );

    if constexpr (std::is_same_v<KeyType, std::string>)
    {
        for (size_t i = 0; i < size; ++i)
        {
            size_t strSize = *reinterpret_cast< size_t * >( data.data() + curr );
            curr += sizeof( size_t );

            std::string str( reinterpret_cast< char * >( data.data() + curr ), strSize );
            node->keys.push_back( str );
            curr += strSize;
        }
    }
    else
    {
        node->keys.resize( size );
        std::copy( data.data() + curr, data.data() + curr + size * sizeof( KeyType ), reinterpret_cast< std::byte * >( node->keys.data() ) );
        curr += size * sizeof( KeyType );
    }

    size = *reinterpret_cast< size_t * >( data.data() + curr );
    curr += sizeof( size_t );

    node->children.resize( size );
    std::copy( data.data() + curr, data.data() + curr + size * sizeof( node_id_t ), reinterpret_cast< std::byte * >( node->children.data() ) );
    curr += size * sizeof( node_id_t );

    size = *reinterpret_cast< size_t * >( data.data() + curr );
    curr += sizeof( size_t );

    if constexpr (std::is_same_v<ValueType, std::string>)
    {
        for (size_t i = 0; i < size; ++i)
        {
            size_t strSize = *reinterpret_cast< size_t * >( data.data() + curr );
            curr += sizeof( size_t );

            std::string str( reinterpret_cast< char * >( data.data() + curr ), strSize );
            node->values.push_back( str );
            curr += strSize;
        }
    }
    else
    {
        node->values.resize( size );
        std::copy( data.data() + curr, data.data() + curr + size * sizeof( ValueType ), reinterpret_cast< std::byte * >( node->values.data() ) );
        curr += size * sizeof( ValueType );
    }

    return node;
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::saveNode ( node_id_t id, BPlusTreeNode *node ) -> void
{
    std::vector< std::byte > data( nodeSize(), std::byte( 0 ) );

    size_t curr = 0;
    std::copy( reinterpret_cast< std::byte * >( &node->type ), reinterpret_cast< std::byte * >( &node->type ) + sizeof( NodeType ), data.data() + curr );
    curr += sizeof( NodeType );

    std::copy( reinterpret_cast< std::byte * >( &node->parent_id ), reinterpret_cast< std::byte * >( &node->parent_id ) + sizeof( node_id_t ), data.data() + curr );
    curr += sizeof( node_id_t );

    std::copy( reinterpret_cast< std::byte * >( &node->nextLeaf_id ), reinterpret_cast< std::byte * >( &node->nextLeaf_id ) + sizeof( node_id_t ), data.data() + curr );
    curr += sizeof( node_id_t );

    size_t size = node->keys.size();
    std::copy( reinterpret_cast< std::byte * >( &size ), reinterpret_cast< std::byte * >( &size ) + sizeof( size_t ), data.data() + curr );
    curr += sizeof( size_t );

    if constexpr (std::is_same_v<KeyType, std::string>)
    {
        for (size_t i = 0; i < size; ++i)
        {
            size_t strSize = node->keys[i].size();
            std::copy( reinterpret_cast< std::byte * >( &strSize ), reinterpret_cast< std::byte * >( &strSize ) + sizeof( size_t ), data.data() + curr );
            curr += sizeof( size_t );

            std::string str = node->keys[i];
            std::copy( reinterpret_cast< std::byte * >( str.data() ), reinterpret_cast< std::byte * >( str.data() ) + str.size(), data.data() + curr );
            curr += str.size();
        }
    }
    else
    {
        std::copy( reinterpret_cast< std::byte * >( node->keys.data() ), reinterpret_cast< std::byte * >( node->keys.data() ) + size * sizeof( KeyType ), data.data() + curr );
        curr += size * sizeof( KeyType );
    }

    size = node->children.size();
    std::copy( reinterpret_cast< std::byte * >( &size ), reinterpret_cast< std::byte * >( &size ) + sizeof( size_t ), data.data() + curr );
    curr += sizeof( size_t );

    std::copy( reinterpret_cast< std::byte * >( node->children.data() ), reinterpret_cast< std::byte * >( node->children.data() ) + size * sizeof( node_id_t ), data.data() + curr );
    curr += size * sizeof( node_id_t );

    size = node->values.size();
    std::copy( reinterpret_cast< std::byte * >( &size ), reinterpret_cast< std::byte * >( &size ) + sizeof( size_t ), data.data() + curr );
    curr += sizeof( size_t );

    if constexpr (std::is_same_v<ValueType, std::string>)
    {
        for (size_t i = 0; i < size; ++i)
        {
            size_t strSize = node->values[i].size();
            std::copy( reinterpret_cast< std::byte * >( &strSize ), reinterpret_cast< std::byte * >( &strSize ) + sizeof( size_t ), data.data() + curr );
            curr += sizeof( size_t );

            std::string str = node->values[i];
            std::copy( reinterpret_cast< std::byte * >( str.data() ), reinterpret_cast< std::byte * >( str.data() ) + str.size(), data.data() + curr );
            curr += str.size();
        }
    }
    else
    {
        std::copy( reinterpret_cast< std::byte * >( node->values.data() ), reinterpret_cast< std::byte * >( node->values.data() ) + size * sizeof( ValueType ), data.data() + curr );
        curr += size * sizeof( ValueType );
    }

    buffer_manager->writeAddress( base_address + id * nodeSize(), data );
    
    return;
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::createNode ( NodeType type ) -> node_id_t
{
    node_id_t id;
    if( free_ids.empty() )
    {
        id = last_id++;
    }
    else
    {
        id = free_ids.back();
        free_ids.pop_back();
    }
    BPlusTreeNode *node = new BPlusTreeNode;
    node->type = type;
    node->parent_id = -1;
    node->nextLeaf_id = -1;
    if( type == NodeType::INTERNAL ) node->children.push_back( -1 );

    saveNode( id, node );
    delete node;
    return id;
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::destroyNode ( node_id_t id ) -> void
{
    free_ids.push_back( id );
    return;
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::search( KeyType key ) -> std::optional< ValueType >
{
    if(root_id == -1) return std::nullopt;

    node_id_t curr_id = root_id;
    BPlusTreeNode *curr = loadNode( curr_id );
    while( curr->type != NodeType::LEAF )
    {
        size_t i = std::upper_bound( curr->keys.begin(), curr->keys.end(), key ) - curr->keys.begin();
        curr_id = curr->children[i];
        delete curr;
        curr = loadNode( curr_id );
    }
    if ( curr->keys.empty() )
    {
        delete curr;
        return std::nullopt;
    }
    size_t i = std::lower_bound( curr->keys.begin(), curr->keys.end(), key ) - curr->keys.begin();
    if( i < curr->keys.size() && curr->keys[i] == key )
    {
        ValueType value = curr->values[i];
        delete curr;
        return value;
    }
    delete curr;
    return std::nullopt;
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::rangeSearch ( KeyType start, KeyType end ) -> std::vector< std::pair< KeyType, ValueType > >
{
    std::vector< std::pair< KeyType, ValueType > > result;

    node_id_t curr_id = root_id;
    if ( curr_id == -1 ) return result;
    BPlusTreeNode *curr = loadNode( curr_id );
    while( curr->type != NodeType::LEAF )
    {
        size_t i = std::upper_bound( curr->keys.begin(), curr->keys.end(), start ) - curr->keys.begin();
        curr_id = curr->children[i];
        delete curr;
        curr = loadNode( curr_id );
    }
    while (curr->keys[0] <= end)
    {
        for ( size_t i = 0; i < curr->keys.size(); ++i )
        {
            if ( curr->keys[i] >= start && curr->keys[i] <= end )
            {
                result.push_back( std::make_pair( curr->keys[i], curr->values[i] ) );
            }
        }
        curr_id = curr->nextLeaf_id;
        if( curr_id == -1 ) break;
        delete curr;
        curr = loadNode( curr_id );
    }
    delete curr;
    return result;
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::findLeaf ( KeyType key ) -> node_id_t
{
    if(root_id == -1) return -1;
    BPlusTreeNode *root = loadNode( root_id );
    if(root->type == NodeType::LEAF)
    {
        delete root;
        return root_id;
    }
    node_id_t curr_id = root_id;
    BPlusTreeNode *curr = loadNode( curr_id );
    while( curr->type != NodeType::LEAF )
    {
        int i = std::upper_bound( curr->keys.begin(), curr->keys.end(), key ) - curr->keys.begin();
        curr_id = curr->children[i];
        delete curr;
        curr = loadNode( curr_id );
    }
    delete root;
    delete curr;
    return curr_id;
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::insert ( KeyType key, ValueType value ) -> bool
{
    if(root_id == -1)
    {
        root_id = createNode( NodeType::LEAF );
        BPlusTreeNode *root = loadNode( root_id );
        root->keys.push_back( key );
        root->values.push_back( value );
        saveNode( root_id, root );
        delete root;
        return true;
    }

    node_id_t leaf_id = findLeaf( key );
    BPlusTreeNode *leaf = loadNode( leaf_id );
    
    size_t i = std::lower_bound( leaf->keys.begin(), leaf->keys.end(), key ) - leaf->keys.begin();
    if( i < leaf->keys.size() && leaf->keys[i] == key )
    {
        // TODO: Can change this part to throw duplicate key error instead of updating value
        leaf->values[i] = value; // Update existing value
        saveNode( leaf_id, leaf );
        delete leaf;
        return true;
    }

    leaf->keys.insert( leaf->keys.begin() + i, key );
    leaf->values.insert( leaf->values.begin() + i, value );

    // Need to split the leaf if it is full
    if( leaf->keys.size() == order )
    {
        node_id_t newLeaf_id = createNode( NodeType::LEAF );
        BPlusTreeNode *newLeaf = loadNode( newLeaf_id );

        // Link the leaves
        newLeaf->nextLeaf_id = leaf->nextLeaf_id;
        leaf->nextLeaf_id = newLeaf_id;

        // Split the keys and values: First half in original leaf, second half in new leaf
        size_t mid = order / 2;
        newLeaf->keys.assign( leaf->keys.begin() + mid, leaf->keys.end() );
        newLeaf->values.assign( leaf->values.begin() + mid, leaf->values.end() );
        leaf->keys.resize( mid );
        leaf->values.resize( mid );

        KeyType newKey = newLeaf->keys[0];
        saveNode( newLeaf_id, newLeaf );
        delete newLeaf;
        saveNode( leaf_id, leaf );
        delete leaf;
        return insertInternal( leaf_id, newKey, newLeaf_id );
    }

    saveNode( leaf_id, leaf );
    delete leaf;
    return true;
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::insertInternal ( node_id_t left_id, KeyType key, node_id_t right_id ) -> bool
{
    if(left_id == -1 || right_id == -1) return false;

    BPlusTreeNode *left = loadNode( left_id );
    BPlusTreeNode *right = loadNode( right_id );

    if(left_id == root_id)
    {
        // Create a new root
        node_id_t newRoot_id = createNode( NodeType::INTERNAL );
        BPlusTreeNode *newRoot = loadNode( newRoot_id );
        
        // Link the new root to the left and right nodes
        newRoot->keys.push_back( key );
        newRoot->children[0] = left_id;
        newRoot->children.push_back( right_id );
        
        // Set the parent of left and right nodes to the new root
        left->parent_id = newRoot_id;
        right->parent_id = newRoot_id;
        
        root_id = newRoot_id;

        saveNode( newRoot_id, newRoot );
        delete newRoot;
        saveNode( left_id, left );
        delete left;
        saveNode( right_id, right );
        delete right;
        return true;
    }
    
    node_id_t parent_id = left->parent_id;
    BPlusTreeNode *parent = loadNode( parent_id );

    size_t i = std::upper_bound( parent->keys.begin(), parent->keys.end(), key ) - parent->keys.begin();
    parent->keys.insert( parent->keys.begin() + i, key );
    parent->children.insert( parent->children.begin() + i + 1, right_id );
    right->parent_id = parent_id;

    saveNode( right_id, right );
    delete right;
    delete left;
    
    if(parent->keys.size() == order)
    {
        // create new internal node
        node_id_t newInternal_id = createNode( NodeType::INTERNAL );
        BPlusTreeNode *newInternal = loadNode( newInternal_id );

        size_t mid = order / 2;
        newInternal->keys.assign( parent->keys.begin() + mid + 1, parent->keys.end() );
        newInternal->children.assign( parent->children.begin() + mid + 1, parent->children.end() );
        KeyType newKey = parent->keys[mid];
        parent->keys.resize( mid );
        parent->children.resize( mid + 1 );
        for(size_t i = 0; i < newInternal->children.size(); ++i)
        {
            BPlusTreeNode *child = loadNode( newInternal->children[i] );
            child->parent_id = newInternal_id;
            saveNode( newInternal->children[i], child );
            delete child;
        }

        saveNode( parent_id, parent );
        delete parent;
        saveNode( newInternal_id, newInternal );
        delete newInternal;
        return insertInternal( parent_id, newKey, newInternal_id );
    }

    saveNode( parent_id, parent );
    delete parent;
    return true;
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::remove ( KeyType key ) -> bool
{
    node_id_t leaf_id = search(key).has_value() ? findLeaf(key) : -1;
    if(leaf_id == -1) return false;
    return removeEntry( leaf_id, key, -1 );
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::removeEntry ( node_id_t node_id, KeyType key, node_id_t ptr_id ) -> bool
{
    BPlusTreeNode *node = loadNode( node_id );
    if(ptr_id == -1) // meaning deletion
    {
        size_t i = std::lower_bound( node->keys.begin(), node->keys.end(), key ) - node->keys.begin();
        if( i < node->keys.size() && node->keys[i] == key )
        {
            node->keys.erase( node->keys.begin() + i );
            node->values.erase( node->values.begin() + i );
        }
        else
        {
            delete node;
            return false;
        }
    }
    else
    {
        size_t i = std::lower_bound( node->keys.begin(), node->keys.end(), key ) - node->keys.begin();
        if( i < node->keys.size() && node->keys[i] == key )
        {
            node->children.erase( node->children.begin() + i + 1 );
            node->keys.erase( node->keys.begin() + i );
        }
        else
        {
            delete node;
            return false;
        }
    }
    saveNode( node_id, node );

    if(node_id == root_id)
    {
        if(node->isLeaf() && node->keys.empty())
        {
            delete node;
            destroyNode( node_id );
            root_id = -1;
        }
        else if(node->children.size() == 1)
        {
            root_id = node->children[0];

            delete node;
            destroyNode( node_id );
        }
        else delete node;
    }
    else if((!node->isLeaf() && node->children.size() <= order / 2) || (node->isLeaf() && node->values.size() < order / 2)) // if node has too few pointers
    {
        node_id_t parent_id = node->parent_id;
        BPlusTreeNode *parent = loadNode( parent_id );
        size_t i = std::find( parent->children.begin(), parent->children.end(), node_id ) - parent->children.begin();
        KeyType betKey;
        int isPred = false;
        BPlusTreeNode *prev = nullptr;
        node_id_t prev_id = -1;
        if(i + 1 < parent->children.size()) // take next one
        {
            prev_id = parent->children[i + 1];
            prev = loadNode( prev_id );
            betKey = parent->keys[i];
            isPred = true;
        }
        else if(i - 1 >= 0) // take previous one
        {
            prev_id = parent->children[i - 1];
            prev = loadNode( prev_id );
            betKey = parent->keys[i - 1];
        }
        else
        {
            delete node;
            delete parent;
            return true;
        }

        if((!node->isLeaf() && (node->children.size() + prev->children.size() <= order)) || (node->isLeaf() && (node->keys.size() + prev->keys.size() < order)))
        {
            if(isPred)
            {
                std::swap( node, prev );
                std::swap( node_id, prev_id );
            }

            if(!node->isLeaf())
            {
                prev->keys.push_back(betKey);
                prev->keys.insert( prev->keys.end(), node->keys.begin(), node->keys.end() );
                prev->children.insert( prev->children.end(), node->children.begin(), node->children.end() );
                for(size_t i = 0; i < node->children.size(); ++i)
                {
                    BPlusTreeNode *child = loadNode( node->children[i] );
                    child->parent_id = prev_id;
                    saveNode( node->children[i], child );
                    delete child;
                }
            }
            else
            {
                prev->keys.insert( prev->keys.end(), node->keys.begin(), node->keys.end() );
                prev->values.insert( prev->values.end(), node->values.begin(), node->values.end() );
                prev->nextLeaf_id = node->nextLeaf_id;
            }
            saveNode( prev_id, prev );
            delete prev;
            bool status = removeEntry( parent_id, betKey, node_id );    
            delete node;
            destroyNode( node_id );
            delete parent;
            return status;
        }
        else
        {
            if(!isPred)
            {
                if(!node->isLeaf())
                {
                    node->keys.insert( node->keys.begin(), betKey );
                    node->children.insert( node->children.begin(), prev->children.back() );
                    parent->keys[i - 1] = prev->keys.back();
                    prev->keys.pop_back();
                    prev->children.pop_back();

                    BPlusTreeNode *child = loadNode( node->children[0] );
                    child->parent_id = node_id; // update parent
                    saveNode( node->children[0], child );
                    delete child;
                }
                else
                {
                    node->keys.insert( node->keys.begin(), prev->keys.back() );
                    node->values.insert( node->values.begin(), prev->values.back() );
                    parent->keys[i - 1] = prev->keys.back();
                    prev->keys.pop_back();
                    prev->values.pop_back();
                }
            }
            else
            {
                if(!node->isLeaf())
                {
                    node->keys.push_back( betKey );
                    node->children.push_back( prev->children[0] );
                    parent->keys[i] = prev->keys[0];
                    prev->keys.erase( prev->keys.begin() );
                    prev->children.erase( prev->children.begin() );

                    BPlusTreeNode *child = loadNode( node->children[node->children.size() - 1] );
                    child->parent_id = node_id; // update parent
                    saveNode( node->children[node->children.size() - 1], child );
                    delete child;
                }
                else
                {
                    node->keys.push_back( prev->keys[0] );
                    node->values.push_back( prev->values[0] );
                    parent->keys[i] = prev->keys[0];
                    prev->keys.erase( prev->keys.begin() );
                    prev->values.erase( prev->values.begin() );
                }
            }
            saveNode( parent_id, parent );
            delete parent;
            saveNode( node_id, node );
            delete node;
            saveNode( prev_id, prev );
            delete prev;
        }
    }
    else delete node;
    return true;
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::printBPlusTree ( std::ostream &os, node_id_t node_id, std::string prefix, bool last ) const -> void
{
    if(node_id == -1) return;
    BPlusTreeNode *node = const_cast<BPlusTreeIndex *>(this)->loadNode( node_id );

    os << prefix << "├─ [";
    for(size_t i = 0; i < node->keys.size(); ++i)
    {
        os << node->keys[i];
        if(i != node->keys.size() - 1) os << ", ";
    }
    os << "]" << std::endl;

    prefix += last ? "   " : "│  ";

    if(!node->isLeaf())
    {
        for(size_t i = 0; i < node->children.size(); ++i)
        {
            printBPlusTree( os, node->children[i], prefix, i == node->children.size() - 1 );
        }
    }
    else
    {
        os << prefix << "└─ [";
        for(size_t i = 0; i < node->values.size(); ++i)
        {
            os << node->values[i];
            if(i != node->values.size() - 1) os << ", ";
        }
        os << "]" << std::endl;
    }
    delete node;
    return;
}

template<typename KeyType, typename ValueType>
std::ostream &operator<< ( std::ostream &os, const BPlusTreeIndex<KeyType, ValueType> &tree )
{
    if(tree.root_id == -1)
    {
        os << "Empty B+ Tree" << std::endl;
        return os;
    }
    tree.printBPlusTree( os, tree.root_id );
    return os;
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::begin ( ) -> BPlusTreeIterator
{
    if(root_id == -1) return BPlusTreeIterator( this, -1 );
    node_id_t curr_id = root_id;
    BPlusTreeNode *curr = loadNode( root_id );
    while( curr->type != NodeType::LEAF )
    {
        curr_id = curr->children[0];
        delete curr;
        curr = loadNode( curr_id );
    }
    BPlusTreeIterator it( this, curr_id );
    delete curr;
    return it;
}

template<typename KeyType, typename ValueType>
auto BPlusTreeIndex<KeyType, ValueType>::data ( node_id_t id ) -> std::vector< std::pair< KeyType, ValueType > >
{
    BPlusTreeNode *curr = loadNode( id );
    std::vector< std::pair< KeyType, ValueType > > result;
    for(size_t i = 0; i < curr->keys.size(); ++i)
    {
        result.push_back( std::make_pair( curr->keys[i], curr->values[i] ) );
    }
    delete curr;
    return result;
}

template class BPlusTreeIndex<int, int>;
template class BPlusTreeIndex<int, std::string>;
template class BPlusTreeIndex<std::string, int>;
template class BPlusTreeIndex<std::string, std::string>;
template class BPlusTreeIndex<unsigned long long, unsigned long long>;

template std::ostream &operator<< ( std::ostream &os, const BPlusTreeIndex<int, int> &tree );
template std::ostream &operator<< ( std::ostream &os, const BPlusTreeIndex<int, std::string> &tree );
template std::ostream &operator<< ( std::ostream &os, const BPlusTreeIndex<std::string, int> &tree );
template std::ostream &operator<< ( std::ostream &os, const BPlusTreeIndex<std::string, std::string> &tree );
template std::ostream &operator<< ( std::ostream &os, const BPlusTreeIndex<unsigned long long, unsigned long long> &tree );

