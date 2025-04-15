#pragma once

#ifndef _HASH_INDEX_HPP_
#define _HASH_INDEX_HPP_

#include <vector>
#include <string>
#include <optional>
#include <iostream>
#include <list>
#include <map>
#include <Utilities/Utils.hpp>
#include <Storage/BufferManager.hpp>

using KeyType = int;
using ValueType = int;

class Bucket
{
    private:
    size_t bucketSize;
    int localDepth;
    std::list<std::pair<KeyType, ValueType>> bucketList;
    
    public:
    Bucket(): bucketSize(2), localDepth(0) {}
    Bucket(size_t _bucketSize, int _localDepth = 0)
    : bucketSize(_bucketSize), localDepth(_localDepth) {}

    Bucket& operator=(const Bucket& other){
        if(this!=&other){
            this->bucketSize = other.bucketSize;
            this->localDepth = other.localDepth;
            this->bucketList = other.bucketList;
        }
        return *this;
    }

    Bucket(const Bucket& other) : bucketSize(other.bucketSize), localDepth(other.localDepth), bucketList(other.bucketList) {}
    
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
};

class ExtendableHashIndex
{
    public:
    ExtendableHashIndex(size_t _globalDepth = 0)
    : globalDepth(_globalDepth) {
        for(int i=0;i<(1<<globalDepth);i++)
        {
            directory.push_back(new Bucket(2, 0)); // Initialize buckets with size 2 and local depth 0
        }
    }
    
    auto insert(KeyType key, ValueType value) -> bool;
    auto search(KeyType key) -> std::optional<ValueType>;
    auto deleteKey(KeyType key) -> bool;
    auto getGlobalDepth() -> int;
    auto getDirectorySize() -> size_t;
    auto display() -> void;
    auto bucket_id(int n) -> std::string;
    
    private:
    size_t globalDepth;
    std::vector<Bucket *> directory;
    auto getBucketNo(KeyType key) -> int;
    auto splitBucket(int index) -> void;
    auto grow(void) -> void;
    auto shrink(void) -> void;
    auto mergeBucket(int index) -> void;
};
#endif

// Compile using: g++ -std=c++20 -Wall -Iinclude -o hash src/Indexes/HashIndex.cpp -Llib -lstorage -lindexes