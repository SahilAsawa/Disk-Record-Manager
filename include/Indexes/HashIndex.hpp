#pragma once

#ifndef _HASH_INDEX_HPP_
#define _HASH_INDEX_HPP_

#include <vector>
#include <string>
#include <optional>
#include <list>
#include <Utilities/Utils.hpp>
#include <Storage/BufferManager.hpp>

using KeyType = int;
using ValueType = int;

class ExtendableHashIndex;

class Bucket
{
    private:
    size_t bucketSize;
    int localDepth;
    int listSize;
    bucket_id_t bucket_id;
    std::list<std::pair<KeyType, ValueType>> bucketList;
    
    public:
    Bucket(): bucketSize(2), localDepth(0) {}
    Bucket(size_t _bucketSize, int _localDepth = 0)
    : bucketSize(_bucketSize), localDepth(_localDepth),listSize(0) {}

    

    Bucket& operator=(const Bucket& other){
        if(this!=&other){
            this->bucketSize = other.bucketSize;
            this->localDepth = other.localDepth;
            this->bucketList = other.bucketList;
            this->listSize=other.listSize;
            this->bucket_id=other.bucket_id;
        }
        return *this;
    }

    Bucket(const Bucket& other) : bucketSize(other.bucketSize), localDepth(other.localDepth), listSize(other.listSize), bucketList(other.bucketList), bucket_id(other.bucket_id){}
    
    auto isFull() -> bool;
    auto isEmpty() -> bool;
    auto insert(KeyType key, ValueType value) -> bool;
    auto search(KeyType key) -> std::optional<ValueType>;
    auto deleteKey(KeyType key) -> bool;
    
    auto getLocalDepth() -> int;
    auto increaseDepth() -> int;
    auto decreaseDepth() -> int;
    std::list<std::pair<KeyType, ValueType>> copy(void);
    void clear(void);
    void display(void);
    auto getBucketSize() -> size_t;
    auto getActualSize() -> size_t;

    friend class ExtendableHashIndex;
};

class ExtendableHashIndex
{
    public:
    ExtendableHashIndex(BufferManager* _bm,size_t _globalDepth = 0)
    : globalDepth(_globalDepth),buffer_manager(_bm) {
        for(int i=0;i<(1<<globalDepth);i++)
        {
            directory.push_back(i); // Initialize buckets with size 2 and local depth 0
        }
    }
    
    auto insert(KeyType key, ValueType value) -> bool;
    auto search(KeyType key) -> std::optional<ValueType>;
    auto deleteKey(KeyType key) -> bool;
    auto getGlobalDepth() -> int;
    auto getDirectorySize() -> size_t;
    auto display() -> void;
    auto bucket_id(int n) -> std::string;
    auto createBucket()->bucket_id_t;
    auto loadBucket(bucket_id_t id) ->Bucket*;
    auto saveBucket(bucket_id_t,Bucket*bp)->void;
    auto destroyBucket(bucket_id_t id) -> void;
    
    private:
    size_t globalDepth;
    std::vector<bucket_id_t> directory;
    std::vector<bucket_id_t> free_ids;
    address_id_t base_address;
    bucket_id_t lastIDUsed;
    BufferManager* buffer_manager;

    auto getBucketNo(KeyType key) -> int;
    auto splitBucket(int index) -> void;
    auto grow(void) -> void;
    auto shrink(void) -> void;
    auto mergeBucket(int index) -> void;
};
#endif

// Compile using: g++ -std=c++20 -Wall -Iinclude -o hash src/Indexes/HashIndex.cpp -Llib -lstorage -lindexes