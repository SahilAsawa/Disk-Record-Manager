#pragma once

#ifndef _HASH_INDEX_HPP_
    #define _HASH_INDEX_HPP_

    #include <vector> 
    #include <string>
    #include <optional>
    #include <list>
    // #include <iostream>

    #include <Utilities/Utils.hpp>
    #include <Storage/BufferManager.hpp>

template<typename KeyType, typename ValueType>
class ExtendableHashIndex;

// using KeyType = int;
// using ValueType = int;

template<typename KeyType, typename ValueType>
class Bucket
{
    private:

    // Denotes the maximum number of elements in a bucket
    size_t bucketSize;

    // Denotes the local depth of the bucket
    int localDepth;

    // Denotes the number of elements in the bucket
    int listSize;

    // Denotes the ID of the bucket
    bucket_id_t bucket_id;

    // Denotes the list of key-value pairs in the bucket
    std::list<std::pair<KeyType, ValueType>> bucketList;
    
    public:

    Bucket ( ): bucketSize(2), localDepth(0) 
    {
    }

    Bucket ( size_t _bucketSize, int _localDepth = 0 )
        : bucketSize(_bucketSize), localDepth(_localDepth), listSize(0)
    {
    }

    
    // overloaded assignment operator
    Bucket<KeyType, ValueType>& operator= ( const Bucket<KeyType, ValueType>& other )
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

    // overloaded copy constructor
    Bucket ( const Bucket<KeyType, ValueType>& other )
    {
        this->bucketSize = other.bucketSize;
        this->localDepth = other.localDepth;
        this->bucketList = other.bucketList;
        this->listSize = other.listSize;
        this->bucket_id = other.bucket_id;
    }
    
    /**
     * @brief Check if the bucket is full
     * @returns true if the bucket is full, false otherwise
     */
    auto isFull ( ) -> bool;


    /**
     * @brief Check if the bucket is empty
     * @returns true if the bucket is empty, false otherwise
     */
    auto isEmpty ( ) -> bool;

    /**
     * @brief Insert a key-value pair into the bucket
     * @param key The key to insert
     * @param value The value to insert
     * @return true if the insertion was successful, false if the bucket is full
     */
    auto insert ( KeyType key, ValueType value ) -> bool;

    /**
     * @brief Search for a key in the bucket
     * @param key The key to search for
     * @return An optional containing the value if found, or std::nullopt if not found
     */
    auto search ( KeyType key ) -> std::optional<ValueType>;

    /**
     * @brief Delete a key from the bucket
     * @param key The key to delete
     * @return true if the deletion was successful, false if the key was not found
     */
    auto deleteKey ( KeyType key ) -> bool;
    
    /**
     * @brief Get the local depth of the bucket
     * @return The local depth of the bucket
     */
    auto getLocalDepth ( ) -> int;

    /**
     * @brief Increase the local depth of the bucket by 1
     * @return The new local depth of the bucket
     */
    auto increaseDepth ( ) -> int;

    /**
     * @brief Decrease the local depth of the bucket by 1
     * @return The new local depth of the bucket
     */
    auto decreaseDepth ( ) -> int;

    /**
     * @brief Copies the bucket's contents to a new list
     * @return A list of key-value pairs in the bucket
     */
    auto copy ( ) -> std::list< std::pair< KeyType, ValueType > >;

    /**
     * @brief Clear the contents of the bucket
     */
    auto clear ( ) -> void;

    /**
     * @brief Prints the contents of the bucket
     */
    auto display ( std::ostream &os ) const -> void;

    /**
     * @brief Get the maximum number of elements in the bucket
     * @return The maximum number of elements in the bucket
     */
    auto getMaxElementCount ( ) -> size_t;

    /**
     * @brief Get the maximum bucket size
     * @return The maximum possible size of the bucket
     */
    auto getMaxSize ( ) -> size_t;
    friend class ExtendableHashIndex<KeyType, ValueType>;
    template<typename K, typename V>
    friend std::ostream& operator<<(std::ostream& os, const ExtendableHashIndex<K, V>& index);
};

template<typename KeyType, typename ValueType>
class ExtendableHashIndex
{
    // Denotes the global depth of the hash index
    size_t globalDepth;

    // Denotes the maximum number of elements in a bucket
    const unsigned int order;

    // Denotes the list of buckets in the hash index. Each bucket is represented by its ID
    std::vector<bucket_id_t> directory;

    // Denotes the list of free bucket IDs. This is used to reuse bucket IDs when buckets are deleted
    std::vector<bucket_id_t> free_ids;

    // Denotes the base address of the hash index in the disk
    address_id_t base_address;

    // Denotes the last bucket ID used. This is used to keep track of the next available bucket ID
    bucket_id_t lastIDUsed;

    // Denotes the buffer manager used to manage the disk
    BufferManager* buffer_manager;

    public:

    // Constructor
    ExtendableHashIndex ( BufferManager* _bm, unsigned int _order = 2, size_t _globalDepth = 0, address_id_t _base_addr = 0 )
    : globalDepth ( _globalDepth ), order ( _order ), base_address ( _base_addr ), lastIDUsed ( 0 ), buffer_manager ( _bm )
    {
        for ( int i = 0; i < (1<<globalDepth) ;i++ )
        {
            // Initialize buckets with size 2 and local depth 0
            directory.push_back(createBucket()); 
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
    auto display ( std::ostream &os) const -> void;

    /**
     * @brief Get the address range of the hash index occupied in the disk
     * @return A pair of address_id_t representing the start and end address of the hash index in the disk
     */
    auto getAddressRange  ( ) -> std::pair< address_id_t, address_id_t >
    {
        Bucket<KeyType, ValueType> b(order, globalDepth);
        address_id_t end_address = base_address + b.getMaxSize() * lastIDUsed;
        return std::make_pair( base_address, end_address );
    }

    template<typename K, typename V>
    friend std::ostream& operator<<(std::ostream& os, const ExtendableHashIndex<K, V>& index);
    
    private:

    /**
     * @brief Returns a 0-1 string representing the bucket according to current local depth of the bucket
     * @param n The bucket number
     * @return A string of length equal to the local depth of the bucket, representing the bucket number in binary
     */
    auto bucket_string ( int n ) const -> std::string;

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
    auto loadBucket ( bucket_id_t id ) const -> Bucket<KeyType,ValueType> *;

    /**
     * @brief Save a bucket to the disk
     * @param id The ID of the bucket to save
     * @param bp A pointer to the bucket to save
     */
    void saveBucket ( bucket_id_t id, Bucket<KeyType, ValueType> *bp );

    /**
     * @brief Destroy a bucket
     * @param id The ID of the bucket to destroy
     */
    auto destroyBucket ( bucket_id_t id ) -> void;

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

    auto keyToInt( KeyType key ) -> unsigned long long int;
};

#endif      // _HASH_INDEX_HPP_
