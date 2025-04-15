#pragma once

#ifndef _HASH_INDEX_HPP_
    #define _HASH_INDEX_HPP_

    #include <vector> 
    #include <string>
    #include <optional>
    #include <list>
    #include <iostream>

    #include <Utilities/Utils.hpp>
    #include <Storage/BufferManager.hpp>

class ExtendableHashIndex;

using KeyType = int;
using ValueType = int;

class Bucket
{
    private:

    //
    size_t bucketSize;

    //
    int localDepth;

    //
    int listSize;

    //
    bucket_id_t bucket_id;

    //
    std::list<std::pair<KeyType, ValueType>> bucketList;
    
    public:

    Bucket(): bucketSize(2), localDepth(0) 
    {
    }

    Bucket( size_t _bucketSize, int _localDepth = 0 )
        : bucketSize(_bucketSize), localDepth(_localDepth), listSize(0)
    {
    }

    

    Bucket& operator=( const Bucket& other )
    {
        if( this != &other )
        {
            this->bucketSize = other.bucketSize;
            this->localDepth = other.localDepth;
            this->bucketList = other.bucketList;
            this->listSize = other.listSize;
            this->bucket_id = other.bucket_id;
        }
        return *this;
    }

    Bucket(const Bucket& other)
        : bucketSize(other.bucketSize), localDepth(other.localDepth), 
        listSize(other.listSize), bucketList(other.bucketList), 
        bucket_id(other.bucket_id)
    {
    }
    
    /**
     * @brief Check if the bucket is full
     * @returns true if the bucket is full, false otherwise
     */
    auto isFull() -> bool;


    /**
     * @brief Check if the bucket is empty
     * @returns true if the bucket is empty, false otherwise
     */
    auto isEmpty() -> bool;

    /**
     * @brief Insert a key-value pair into the bucket
     * @param key The key to insert
     * @param value The value to insert
     * @return true if the insertion was successful, false if the bucket is full
     */
    auto insert(KeyType key, ValueType value) -> bool;

    /**
     * @brief Search for a key in the bucket
     * @param key The key to search for
     * @return An optional containing the value if found, or std::nullopt if not found
     */
    auto search(KeyType key) -> std::optional<ValueType>;

    /**
     * @brief Delete a key from the bucket
     * @param key The key to delete
     * @return true if the deletion was successful, false if the key was not found
     */
    auto deleteKey(KeyType key) -> bool;
    
    /**
     * @brief Get the local depth of the bucket
     * @return The local depth of the bucket
     */
    auto getLocalDepth() -> int;

    /**
     * @brief Increase the local depth of the bucket by 1
     * @return The new local depth of the bucket
     */
    auto increaseDepth() -> int;

    /**
     * @brief Decrease the local depth of the bucket by 1
     * @return The new local depth of the bucket
     */
    auto decreaseDepth() -> int;

    /**
     * @brief Copies the bucket's contents to a new list
     * @return A list of key-value pairs in the bucket
     */
    auto copy() -> std::list< std::pair< KeyType, ValueType > >;

    /**
     * @brief Clear the contents of the bucket
     */
    auto clear() -> void;

    /**
     * @brief Prints the contents of the bucket
     */
    auto display() -> void;

    /**
     * @brief Get the maximum bucket size
     * @return The maximum size of the bucket
     */
    auto getBucketSize() -> size_t;

    /**
     * @brief Get the current bucket size
     * @return The current size of the bucket
     */
    auto getActualSize() -> size_t;

    friend class ExtendableHashIndex;
};


class ExtendableHashIndex
{
    public:

    // Constructor
    ExtendableHashIndex ( BufferManager* _bm,size_t _globalDepth = 0 )
    : globalDepth ( _globalDepth ), buffer_manager ( _bm )
    {
        for ( int i = 0; i < (1<<globalDepth) ;i++ )
        {
            // Initialize buckets with size 2 and local depth 0
            directory.push_back(i); 
        }
    }
    
    /**
     * @brief Insert a key-value pair into the hash index
     * @param key The key to insert
     * @param value The value to insert
     * @return maybe always returns true
     */
    auto insert ( KeyType key, ValueType value ) -> bool;

    /**
     * @brief Search for a key in the hash index
     * @param key The key to search for
     * @return An optional containing the value if found, or std::nullopt if not found
     */
    auto search ( KeyType key ) -> std::optional< ValueType >;

    /**
     * @brief Delete a key from the hash index
     * @param key The key to delete
     * @return true if the deletion was successful, false if the key was not found
     */
    auto deleteKey ( KeyType key ) -> bool;

    /**
     * @brief Get the global depth of the hash index
     * @return The global depth of the hash index
     */
    auto getGlobalDepth ( ) -> int;

    /**
     * @brief Get the number of buckets in the hash index
     * @return The size of the bucket list
     */
    auto getDirectorySize ( ) -> size_t;

    /**
     * @brief Display the contents of the hash index
     */
    auto display ( ) -> void;

    /**
     * @brief Returns a 0-1 string representing the bucket according to current local depth of the bucket
     * @param n The bucket number
     * @return A string of length equal to the local depth of the bucket, representing the bucket number in binary
     */
    auto bucket_id ( int n ) -> std::string;

    /**
     * @brief Create a new bucket
     * @return The ID of the newly created bucket
     */
    auto createBucket ( ) -> bucket_id_t;

    /**
     * @brief Load a bucket from the disk
     * @param id The ID of the bucket to load
     * @return A pointer to the loaded bucket
     */
    auto loadBucket ( bucket_id_t id ) -> Bucket *;

    /**
     * @brief Save a bucket to the disk
     * @param id The ID of the bucket to save
     * @param bp A pointer to the bucket to save
     */
    auto saveBucket ( bucket_id_t id, Bucket *bp ) -> void;

    /**
     * @brief Destroy a bucket
     * @param id The ID of the bucket to destroy
     */
    auto destroyBucket ( bucket_id_t id ) -> void;
    
    private:

    //
    size_t globalDepth;

    //
    std::vector<bucket_id_t> directory;

    //
    std::vector<bucket_id_t> free_ids;

    //
    address_id_t base_address;

    //
    bucket_id_t lastIDUsed;

    //
    BufferManager* buffer_manager;

    /**
     * @brief Get the bucket number for a given key
     * @param key The key to hash
     * @return The bucket number
     */
    auto getBucketNo ( KeyType key ) -> int;

    /**
     * @brief Split a bucket at a given index
     * @param index The index of the bucket to split
     */
    auto splitBucket ( int index ) -> void;

    /**
     * @brief Increase the global depth of the hash index and doubles the directory size
     */
    auto grow ( void ) -> void;

    /**
     * @brief Decrease the global depth of the hash index and halves the directory size
     */
    auto shrink ( void ) -> void;

    /**
     * @brief Merge a bucket at a given index with its buddy
     * @param index The index of the bucket to merge
     */
    auto mergeBucket ( int index ) -> void;
};

#endif      // _HASH_INDEX_HPP_