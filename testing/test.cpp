#include "Storage/BufferManager.hpp"
#include <iostream>
#include <vector>
#include <optional>
#include <Indexes/BPlusTreeIndex.hpp>
#include <Storage/Disk.hpp>

using KeyType = std::string;
using ValueType = std::string;

Disk disk( SEQUENTIAL, 512, 1024 );
BufferManager bm( &disk, LRU, 1024 );

// void BPlusTreeTest()
// {
    // BPlusTreeIndex<KeyType, ValueType> bptree( &bm, 4 );
    // std::vector<std::pair<KeyType, ValueType>> data;
    // data.push_back( {"Srinivasan", "10101"} );
    // data.push_back( {"Wu", "12121"} );
    // data.push_back( {"Mozart", "15151"} );
    // data.push_back( {"Einstein", "22222"} );
    // data.push_back( {"El Said", "32343"} );
    // data.push_back( {"Gold", "33456"} );
    // data.push_back( {"Katz", "45565"} );
    // data.push_back( {"Caliﬁeri", "58583"} );
    // data.push_back( {"Singh", "76543"} );
    // data.push_back( {"Crick", "76766"} );
    // data.push_back( {"Brandt", "83821"} );
    // data.push_back( {"Kim", "98345"} );
    // data.push_back({"Adams", "12345"});
    // data.push_back({"Lamport", "23456"});
    // data.push_back({"Knuth", "34567"});
    // data.push_back({"Hopper", "45678"});
    // data.push_back({"Turing", "56789"});
    // data.push_back({"Hawking", "67890"});
    // data.push_back({"Newton", "78901"});
    // data.push_back({"Curie", "89012"});
    // data.push_back({"Darwin", "90123"});
    // data.push_back({"Tesla", "01234"});
    // data.push_back({"Hubble", "12345"});
    // data.push_back({"Feynman", "23456"});
    // data.push_back({"Bohr", "34567"});
    // data.push_back({"Heisenberg", "45678"});
    // data.push_back({"Faraday", "56789"});
    // data.push_back({"Maxwell", "67890"});
    // data.push_back({"Planck", "78901"});
    // data.push_back({"Galileo", "90123"});
    // data.push_back({"Copernicus", "01234"});
    // data.push_back({"Kepler", "12345"});


    // for(const auto& [key, value] : data)
    // {
    //     bptree.insert( key, value );
    //     std::cout << "Inserted: " << key << " -> " << value << std::endl;
    //     // std::cout << bptree << std::endl;
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

    // std::vector<KeyType> toremove{
    //     "Srinivasan",
    //     "Singh",
    //     "Wu",
    //     "Gold",
    //     "Adams"
    // };
 
    // for(const auto &it: data)
    // {
    //     bptree.remove(it.first);
    //     std::cout << "Removed: " << it.first << std::endl;
    //     // std::cout << bptree << std::endl;
    // }
    
    // //stats
    // std::cout << "Disk IO's: " << bm.getNumIO() << std::endl;
    // std::cout << "Disk IO cost: " << bm.getCostIO() << std::endl;
    // return;
// }

// void diskTest()
// {
//     Disk disk( RANDOM, 4096, 1024 );
//     std::cout << "Disk created with block size: " << disk.blockSize << " and block count: " << disk.blockCount << std::endl;

//     std::vector<int> data( disk.blockSize / sizeof(int), 0 );
//     for (size_t i = 0; i < data.size(); ++i)
//     {
//         data[i] = i;
//     }
//     disk.writeBlock( 1, std::vector<std::byte>( reinterpret_cast<std::byte*>( data.data() ), reinterpret_cast<std::byte*>( data.data() ) + disk.blockSize ) );
    
//     std::cout << "Written block 0" << std::endl;
//     std::vector<std::byte> readData = disk.readBlock( 0 );
//     std::vector<int> readInts( disk.blockSize / sizeof(int) );
//     std::move( readData.begin(), readData.end(), reinterpret_cast<std::byte*>( readInts.data() ) );
//     std::cout << "Read block 0: ";
//     for (size_t i = 0; i < readInts.size(); ++i)
//     {
//         std::cout << readInts[i] << " ";
//     }
//     std::cout << std::endl;

//     std::cout << "Disk IO operations: " << disk.numIO << std::endl;
//     std::cout << "Disk IO cost: " << disk.costIO << std::endl;
// }

// void BufferManagerTest()
// {
//     int pageSize = 8;
//     Disk disk( RANDOM, pageSize, 1024 );
//     BufferManager bufferManager( &disk, LRU, 10 );
//     int n = 11;
//     std::vector<int> data( n );
//     for(int i = 0; i < n; ++i) data[i] = i;
//     std::vector<std::byte> byteData( reinterpret_cast<std::byte*>( data.data() ), reinterpret_cast<std::byte*>( data.data() ) + n * sizeof(int) );
//     for(int i = 0; i < byteData.size(); ++i)
//     {
//         std::cout << std::to_integer<int>( byteData[i] ) << " ";
//     }
//     std::cout << std::endl;
//     bufferManager.writeAddress( 0, byteData );
//     std::vector<std::byte> readData = bufferManager.readAddress( 8, 100 );
//     for(int i = 0; i < readData.size(); ++i)
//     {
//         std::cout << std::to_integer<int>( readData[i] ) << " ";
//     }

//     std::vector<int> readInts( readData.size() / sizeof(int) );
//     std::move( readData.begin(), readData.end(), reinterpret_cast<std::byte*>( readInts.data() ) );
//     std::cout << "Read data: ";
//     for (size_t i = 0; i < readInts.size(); ++i)
//     {
//         std::cout << readInts[i] << " ";
//     }
// }

// void god()
// {
//     BPlusTreeIndex bptree( &bm );

//     int nodeId = bptree.createNode( BPlusTreeIndex::NodeType::INTERNAL );
//     BPlusTreeIndex::BPlusTreeNode *node = bptree.loadNode( nodeId );
//     node->parent_id = 10;
//     node->nextLeaf_id = 20;
//     node->keys.push_back( "ok" );
//     node->children.push_back( 0 );
//     node->children.push_back( 1 );
//     node->values.push_back( "hi" );
//     bptree.saveNode( nodeId, node );
//     BPlusTreeIndex::BPlusTreeNode *loadedNode = bptree.loadNode( nodeId );
//     std::cout << "Loaded Node Type: " << ( loadedNode->type == BPlusTreeIndex::NodeType::INTERNAL ? "INTERNAL" : "LEAF" ) << std::endl;
//     std::cout << "Loaded Node ID: " << loadedNode->parent_id << std::endl;
//     std::cout << "Loaded Node Next Leaf ID: " << loadedNode->nextLeaf_id << std::endl;
//     std::cout << "Loaded Node Keys: ";
//     for (const auto &key : loadedNode->keys)
//     {
//         std::cout << key << " ";
//     }
//     std::cout << std::endl;
//     std::cout << "Loaded Node Children: ";
//     for (const auto &child : loadedNode->children)
//     {
//         std::cout << child << " ";
//     }
//     std::cout << std::endl;
//     std::cout << "Loaded Node Values: ";
//     for (const auto &value : loadedNode->values)
//     {
//         std::cout << value << " ";
//     }
//     std::cout << std::endl;

// }

int main()
{
    // BufferManagerTest();
    // god();
    // BPlusTreeTest();
}