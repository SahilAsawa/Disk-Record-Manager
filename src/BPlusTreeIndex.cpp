#include "bplustree.hpp"
#include <algorithm>
#include <iostream>


auto BPlusTreeIndex::createNode ( NodeType type ) -> BPlusTreeNode *
{
    BPlusTreeNode *node = new BPlusTreeNode;
    node->type = type;
    if(type  == NodeType::INTERNAL) node->children.push_back(nullptr);
    node->parent = nullptr;
    node->nextLeaf = nullptr;
    return node;
}

auto BPlusTreeIndex::search( KeyType key ) -> std::optional< ValueType >
{
    if(root == nullptr) return std::nullopt;

    BPlusTreeNode *curr = root;
    while( curr->type != NodeType::LEAF )
    {
        size_t i = std::upper_bound( curr->keys.begin(), curr->keys.end(), key ) - curr->keys.begin();
        curr = curr->children[i];
    }
    if ( curr->keys.empty() ) return std::nullopt;
    size_t i = std::lower_bound( curr->keys.begin(), curr->keys.end(), key ) - curr->keys.begin();
    if( i < curr->keys.size() && curr->keys[i] == key )
    {
        return curr->values[i];
    }
    return std::nullopt;
}

auto BPlusTreeIndex::rangeSearch ( KeyType start, KeyType end ) -> std::vector< ValueType >
{
    std::vector< ValueType > result;
    BPlusTreeNode *curr = root;
    if ( curr == nullptr ) return result;

    while( curr->type != NodeType::LEAF )
    {
        size_t i = std::upper_bound( curr->keys.begin(), curr->keys.end(), start ) - curr->keys.begin();
        curr = curr->children[i];
    }
    while ( curr != nullptr && curr->keys[0] <= end)
    {
        for ( size_t i = 0; i < curr->keys.size(); ++i )
        {
            if ( curr->keys[i] >= start && curr->keys[i] <= end )
            {
                result.push_back( curr->values[i] );
            }
        }

        curr = curr->nextLeaf;
    }
    return result;
}

auto BPlusTreeIndex::findLeaf ( KeyType key ) -> BPlusTreeNode *
{
    if(root == nullptr) return nullptr;
    if(root->type == NodeType::LEAF) return root;
    BPlusTreeNode *curr = root;
    while( curr->type != NodeType::LEAF )
    {
        int i = std::upper_bound( curr->keys.begin(), curr->keys.end(), key ) - curr->keys.begin();
        curr = curr->children[i];
    }
    return curr;
}

auto BPlusTreeIndex::insert ( KeyType key, ValueType value ) -> bool
{
    if(root == nullptr)
    {
        root = createNode( NodeType::LEAF );
        root->keys.push_back( key );
        root->values.push_back( value );
        return true;
    }

    BPlusTreeNode *leaf = findLeaf( key );
    
    size_t i = std::lower_bound( leaf->keys.begin(), leaf->keys.end(), key ) - leaf->keys.begin();
    if( i < leaf->keys.size() && leaf->keys[i] == key )
    {
        // TODO: Can change this part to throw duplicate key error instead of updating value
        leaf->values[i] = value; // Update existing value
        return true;
    }

    leaf->keys.insert( leaf->keys.begin() + i, key );
    leaf->values.insert( leaf->values.begin() + i, value );

    // Need to split the leaf if it is full
    if( leaf->keys.size() == ORDER )
    {
        BPlusTreeNode *newLeaf = createNode( NodeType::LEAF );

        // Link the leaves
        newLeaf->nextLeaf = leaf->nextLeaf;
        leaf->nextLeaf = newLeaf;

        // Split the keys and values: First half in original leaf, second half in new leaf
        size_t mid = ORDER / 2;
        newLeaf->keys.assign( leaf->keys.begin() + mid, leaf->keys.end() );
        newLeaf->values.assign( leaf->values.begin() + mid, leaf->values.end() );
        leaf->keys.resize( mid );
        leaf->values.resize( mid );
        return insertInternal( leaf, newLeaf->keys[0], newLeaf );
    }
    return true;
}

auto BPlusTreeIndex::insertInternal ( BPlusTreeNode *left, KeyType key, BPlusTreeNode *right ) -> bool
{
    if(left == NULL || right == NULL) return false;

    if(left == root)
    {
        // Create a new root
        BPlusTreeNode *newRoot = createNode(NodeType::INTERNAL);
        
        // Link the new root to the left and right nodes
        root = newRoot;
        root->keys.push_back( key );
        root->children[0] = left;
        root->children.push_back( right );
        
        // Set the parent of left and right nodes to the new root
        left->parent = root;
        right->parent = root;
        
        return true;
    }
    
    BPlusTreeNode *parent = left->parent;

    size_t i = std::upper_bound( parent->keys.begin(), parent->keys.end(), key ) - parent->keys.begin();
    parent->keys.insert( parent->keys.begin() + i, key );
    parent->children.insert( parent->children.begin() + i + 1, right );
    right->parent = parent;
    
    if(parent->keys.size() == ORDER)
    {
        // create new internal node
        BPlusTreeNode *newInternal = createNode(NodeType::INTERNAL);

        size_t mid = ORDER / 2;
        newInternal->keys.assign( parent->keys.begin() + mid + 1, parent->keys.end() );
        newInternal->children.assign( parent->children.begin() + mid + 1, parent->children.end() );
        KeyType newKey = parent->keys[mid];
        parent->keys.resize( mid );
        parent->children.resize( mid + 1 );
        for(size_t i = 0; i < newInternal->children.size(); ++i)
        {
            newInternal->children[i]->parent = newInternal;
        }

        return insertInternal( parent, newKey, newInternal );
    }

    return true;
}

auto BPlusTreeIndex::remove ( KeyType key ) -> bool
{
    BPlusTreeNode *leaf = search(key).has_value() ? findLeaf(key) : nullptr;
    if(leaf == nullptr) return false;
    return removeEntry( leaf, key, nullptr );
}

auto BPlusTreeIndex::removeEntry ( BPlusTreeNode *node, KeyType key, BPlusTreeNode *ptr ) -> bool
{
    if(ptr == nullptr) // meaning deletion
    {
        size_t i = std::lower_bound( node->keys.begin(), node->keys.end(), key ) - node->keys.begin();
        if( i < node->keys.size() && node->keys[i] == key )
        {
            node->keys.erase( node->keys.begin() + i );
            node->values.erase( node->values.begin() + i );
        }   
        else return false;
    }
    else
    {
        size_t i = std::lower_bound( node->keys.begin(), node->keys.end(), key ) - node->keys.begin();
        if( i < node->keys.size() && node->keys[i] == key )
        {
            node->children.erase( node->children.begin() + i + 1 );
            node->keys.erase( node->keys.begin() + i );
        }
        else return false;
    }

    if(node == root)
    {
        if(node->isLeaf() && node->keys.empty())
        {
            delete node;
            root = nullptr;
        }
        else if(node->children.size() == 1)
        {
            root = node->children[0];
            delete node;
        }
    }
    else if((!node->isLeaf() && node->children.size() <= ORDER / 2) || (node->isLeaf() && node->values.size() < ORDER / 2)) // if node has too few pointers
    {
        BPlusTreeNode *parent = node->parent;
        size_t i = std::find( parent->children.begin(), parent->children.end(), node ) - parent->children.begin();
        KeyType betKey;
        int isPred = false;
        BPlusTreeNode *prev = nullptr;
        if(i + 1 < parent->children.size()) // take next one
        {
            prev = parent->children[i + 1];
            betKey = parent->keys[i];
            isPred = true;
        }
        else if(i - 1 >= 0) // take previous one
        {
            prev = parent->children[i - 1];
            betKey = parent->keys[i - 1];
        }
        else return true;

        if((!node->isLeaf() && (node->children.size() + prev->children.size() <= ORDER)) || (node->isLeaf() && (node->keys.size() + prev->keys.size() < ORDER)))
        {
            if(isPred) std::swap( node, prev );

            if(!node->isLeaf())
            {
                prev->keys.push_back(betKey);
                prev->keys.insert( prev->keys.end(), node->keys.begin(), node->keys.end() );
                prev->children.insert( prev->children.end(), node->children.begin(), node->children.end() );
                for(size_t i = 0; i < node->children.size(); ++i)
                {
                    node->children[i]->parent = prev;
                }
            }
            else
            {
                prev->keys.insert( prev->keys.end(), node->keys.begin(), node->keys.end() );
                prev->values.insert( prev->values.end(), node->values.begin(), node->values.end() );
                prev->nextLeaf = node->nextLeaf;
            }
            removeEntry( parent, betKey, node );    
            delete node;
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
                    node->children[0]->parent = node; // update parent
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
                    node->children[node->children.size() - 1]->parent = node; // update parent
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
        }
    }
    return true;
}

auto BPlusTreeIndex::printBPlusTree ( std::ostream &os, BPlusTreeNode *node, std::string prefix, bool last ) const -> void
{
    if(node == nullptr) return;

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
    return;
}

std::ostream &operator<< ( std::ostream &os, const BPlusTreeIndex &tree )
{
    if(tree.root == nullptr)
    {
        os << "Empty B+ Tree" << std::endl;
        return os;
    }
    tree.printBPlusTree( os, tree.root );
    return os;
}



int main()
{
    BPlusTreeIndex bptree;
    
    std::vector<std::pair<KeyType, ValueType>> data;
    data.push_back( {"Srinivasan", "10101"} );
    data.push_back( {"Wu", "12121"} );
    data.push_back( {"Mozart", "15151"} );
    data.push_back( {"Einstein", "22222"} );
    data.push_back( {"El Said", "32343"} );
    data.push_back( {"Gold", "33456"} );
    data.push_back( {"Katz", "45565"} );
    data.push_back( {"Caliﬁeri", "58583"} );
    data.push_back( {"Singh", "76543"} );
    data.push_back( {"Crick", "76766"} );
    data.push_back( {"Brandt", "83821"} );
    data.push_back( {"Kim", "98345"} );
    data.push_back({"Adams", "12345"});
    data.push_back({"Lamport", "23456"});
    data.push_back({"Knuth", "34567"});
    data.push_back({"Hopper", "45678"});
    data.push_back({"Turing", "56789"});
    data.push_back({"Hawking", "67890"});
    data.push_back({"Newton", "78901"});
    data.push_back({"Curie", "89012"});
    data.push_back({"Darwin", "90123"});
    data.push_back({"Tesla", "01234"});
    data.push_back({"Hubble", "12345"});
    data.push_back({"Feynman", "23456"});
    data.push_back({"Bohr", "34567"});
    data.push_back({"Heisenberg", "45678"});
    data.push_back({"Faraday", "56789"});
    data.push_back({"Maxwell", "67890"});
    data.push_back({"Planck", "78901"});
    data.push_back({"Galileo", "90123"});
    data.push_back({"Copernicus", "01234"});
    data.push_back({"Kepler", "12345"});


    for(const auto& [key, value] : data)
    {
        bptree.insert( key, value );
        std::cout << "Inserted: " << key << " -> " << value << std::endl;
        std::cout << bptree << std::endl;
    }
    std::cout << "Final B+ Tree:" << std::endl;
    std::cout << bptree << std::endl;

    std::vector<KeyType> toremove{
        "Srinivasan",
        "Singh",
        "Wu",
        "Gold",
        "Adams"
    };

    for(const auto &it: data)
    {
        bptree.remove(it.first);
        std::cout << "Removed: " << it.first << std::endl;
        std::cout << bptree << std::endl;
        std::cerr << "Removing Done" << std::endl;
    }


    // for(const auto &it: toremove)
    // {
    //     bptree.remove(it);
    //     std::cout << "Removed: " << it << std::endl;
    //     std::cout << bptree << std::endl;
    //     std::cerr << "Removing Done" << std::endl;
    // }

    // std::cout << "\nSearch Results:" << std::endl;
    // for(const auto& [key, value] : data)
    // {
    //     auto result = bptree.search( key );
    //     if(result)
    //     {
    //         std::cout << "Found: " << key << " -> " << *result << std::endl;
    //     }
    //     else
    //     {
    //         std::cout << "Not Found: " << key << std::endl;
    //     }
    // }
    // auto result = bptree.search( "Random" );
    // if(result)
    // {
    //     std::cout << "Found: Random -> " << *result << std::endl;
    // }
    // else
    // {
    //     std::cout << "Not Found: Random" << std::endl;
    // }

    // std::vector<std::pair<KeyType, ValueType>> searchData {
    //     {"Einstein", "Einstein"},
    //     {"A", "Z"},
    //     {"A", "Caliﬁeri"},
    //     {"Einstein", "Hawking"},
    //     {"Hawking", "Einstein"},
    //     {"A", "A"},
    //     {"Z", "Z"}
    // };
    // std::cout << "\nRange Search Results:" << std::endl;
    // for(const auto& [start, end] : searchData)
    // {
    //     auto rangeResult = bptree.rangeSearch( start, end );
    //     std::cout << "Range [" << start << ", " << end << "] -> ";
    //     if(rangeResult.empty())
    //     {
    //         std::cout << "No results" << std::endl;
    //     }
    //     else
    //     {
    //         for(const auto& val : rangeResult)
    //         {
    //             std::cout << val << " ";
    //         }
    //         std::cout << std::endl;
    //     }
    // }
    
    return 0;
}