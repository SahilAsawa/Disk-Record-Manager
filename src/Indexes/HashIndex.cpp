#include <Indexes/HashIndex.hpp>
#include <set>
#include<iostream>
#include <string>
#define MOD 1000000007

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::isFull() -> bool
{
    // std::cout<<"Bucket Size: "<<bucketSize<<std::endl;
    return bucketList.size() >= bucketSize;
}

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::isEmpty() -> bool
{
    return bucketList.empty();
}

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::insert(KeyType key, ValueType value) -> bool
{
    for (auto &pair : bucketList)
    {
        if (pair.first == key)
        {
            // Key already exists
            pair.second = value; // Update value
            return true;
        }
    }
    if (isFull())
        return false;
    bucketList.emplace_back(key, value);
    listSize++;
    return true;
}

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::search(KeyType key) -> std::optional<ValueType>
{
    // std::cout << "Search Order->: ";
    for (auto &pair : bucketList)
    {
        // std::cout << pair.first << " ";
        if (pair.first == key)
            return pair.second;
    }
    // std::cout << std::endl;
    return std::nullopt;
}

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::deleteKey(KeyType key) -> bool
{
    for (auto it = bucketList.begin(); it != bucketList.end(); ++it)
    {
        if (it->first == key)
        {
            bucketList.erase(it);
            listSize--;
            return true;
        }
    }
    return false;
}

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::getLocalDepth() -> int
{
    return localDepth;
}

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::increaseDepth() -> int
{
    return ++localDepth;
}

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::decreaseDepth() -> int
{
    return --localDepth;
}

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::copy() -> std::list<std::pair<KeyType, ValueType>>
{
    return bucketList;
}

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::clear() -> void
{
    this->bucketList.clear();
    this->listSize = 0;
}

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::display(std::ostream &os) const -> void
{
    for (auto &[k, v] : bucketList)
        os << "(" << k << ", " << v << ") ";
    os << std::endl;
}

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::getMaxElementCount() -> size_t
{
    return bucketSize;
}

template <typename KeyType, typename ValueType>
auto Bucket<KeyType, ValueType>::getMaxSize() -> size_t
{
    int keySize = sizeof(KeyType), valueSize = sizeof(ValueType);
    return sizeof(size_t) + sizeof(int) + sizeof(int) + sizeof(bucket_id_t) + bucketSize * (keySize + valueSize);
}

// ======================== ExtendableHashIndex ========================

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::createBucket() -> bucket_id_t
{
    bucket_id_t id;
    if (free_ids.empty())
    {
        id = lastIDUsed++;
    }
    else
    {
        id = free_ids.back();
        free_ids.pop_back();
    }
    Bucket<KeyType, ValueType> *bucket = new Bucket<KeyType,ValueType>(order, 0); // Create a new bucket with size 'order'(=2 by default) and local depth 0
    bucket->bucket_id = id;
    // std::cout<<"Create Bucket, Bucket Size = "<<bucket->bucketSize<<std::endl;
    saveBucket(id, bucket); // Save the new bucket to the directory
    // directory.push_back(id); // Add the bucket ID to the directory
    return id;
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::destroyBucket(bucket_id_t id) -> void
{
    free_ids.push_back(id); // Add the bucket ID to the free list
    return;
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::loadBucket(bucket_id_t id) const -> Bucket<KeyType, ValueType> *
{
    Bucket<KeyType, ValueType> *bp = new Bucket<KeyType,ValueType>;

    // std::cout << "Trying to read from address: " << base_address + id*bp->getMaxSize() << " of size: " << bp->getMaxSize() << std::endl;
    std::vector<std::byte> data = buffer_manager->readAddress(base_address + id * bp->getMaxSize(), bp->getMaxSize());
    // std::cout << "Read data size: " << data.size() << std::endl;

    size_t curr = 0;
    bp->bucketSize = *reinterpret_cast<size_t *>(data.data());
    curr += sizeof(size_t);
    // std::cout << "Bucket size: " << bp->bucketSize << std::endl;

    bp->localDepth = *reinterpret_cast<int *>(data.data() + curr);
    curr += sizeof(int);
    // std::cout << "Local depth: " << bp->localDepth << std::endl;

    bp->listSize = *reinterpret_cast<int *>(data.data() + curr);
    curr += sizeof(int);
    // std::cout << "List size: " << bp->listSize << std::endl;

    bp->bucket_id = *reinterpret_cast<bucket_id_t *>(data.data() + curr);
    curr += sizeof(bucket_id_t);
    // std::cout << "Bucket ID: " << bp->bucket_id << std::endl;

    for (size_t i = 0; i < bp->bucketSize; i++)
    {
        KeyType key = *reinterpret_cast<KeyType *>(data.data() + curr);
        curr += sizeof(KeyType);
        ValueType value = *reinterpret_cast<ValueType *>(data.data() + curr);
        curr += sizeof(ValueType);
        // std::cout << "Element " << i << ": (" << key << ", " << value << ")" << std::endl;

        if (i < (size_t)bp->listSize)
        {
            bp->bucketList.push_back(std::make_pair(key, value));
        }
    }
    // std::cout << "Loaded bucket: " << id << std::endl;

    return bp;
}

template <typename KeyType, typename ValueType>
void ExtendableHashIndex<KeyType, ValueType>::saveBucket(bucket_id_t id, Bucket<KeyType, ValueType> *bp)
{
    std::vector<std::byte> data(bp->getMaxSize(), std::byte(0));
    size_t curr = 0;
    // if (bp->bucketSize != bp->getMaxElementCount())
    //     std::cerr << "Bucket size mismatch: " << bp->bucketSize << " for bucket id: " << bp->bucket_id << std::endl;
    std::copy(reinterpret_cast<std::byte *>(&bp->bucketSize), reinterpret_cast<std::byte *>(&bp->bucketSize) + sizeof(size_t), data.data() + curr);
    curr += sizeof(size_t);

    std::copy(reinterpret_cast<std::byte *>(&bp->localDepth), reinterpret_cast<std::byte *>(&bp->localDepth) + sizeof(int), data.data() + curr);
    curr += sizeof(int);

    std::copy(reinterpret_cast<std::byte *>(&bp->listSize), reinterpret_cast<std::byte *>(&bp->listSize) + sizeof(int), data.data() + curr);
    curr += sizeof(int);

    std::copy(reinterpret_cast<std::byte *>(&bp->bucket_id), reinterpret_cast<std::byte *>(&bp->bucket_id) + sizeof(bucket_id_t), data.data() + curr);
    curr += sizeof(bucket_id_t);

    for (auto &pair : bp->bucketList)
    {
        std::copy(reinterpret_cast<std::byte *>(&pair.first), reinterpret_cast<std::byte *>(&pair.first) + sizeof(KeyType), data.data() + curr);
        curr += sizeof(KeyType);
        std::copy(reinterpret_cast<std::byte *>(&pair.second), reinterpret_cast<std::byte *>(&pair.second) + sizeof(ValueType), data.data() + curr);
        curr += sizeof(ValueType);
    }
    buffer_manager->writeAddress(base_address + id * bp->getMaxSize(), data);
    return;
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::keyToInt(KeyType key) -> unsigned long long int
{   
    unsigned long long int  hash = 0;

    if constexpr (std::is_same<KeyType, std::string>::value)
    {
        for(unsigned int i=0;i<key.length();i++)
        {
            char c=key[i];
            if(c<='z' && c>='a')
            {
                hash = ((hash*26)%MOD + (c-'a'+1))%MOD;
            }
            else if(c<='Z' && c>='A')
            {
                hash = ((hash*26)%MOD + (c-'A'+1))%MOD;
            }
            else if(c<='9' && c>='0')
            {
                hash = ((hash*10)%MOD + (c-'0'))%MOD;
            }
            else
            {
                hash = (hash*26)%MOD;
            }
        }
    }
    else if constexpr (std::is_same<KeyType, int>::value)
    {
        hash = static_cast<unsigned long long int>(key);
    }
    else if constexpr (std::is_same<KeyType, unsigned long long int>::value)
    {
        hash = key;
    }
    else
    {
        // std::cerr << "Key type not supported" << std::endl;
        hash = 0LL;
    }
    // std::cout << "Key: " << key << ", Hash: " << hash << std::endl;
    return hash;
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::getBucketNo(KeyType key) -> int
{
    // unsigned long long int hash = keyToInt(key);
    if constexpr (std::is_same<KeyType, std::string>::value)
    {
        unsigned long long int hash = keyToInt(key);
        // std::cout << "Hash value: " << hash << std::endl;
        return hash & ((1ll << globalDepth) - 1);
    }
    else if constexpr (std::is_same<KeyType, unsigned long long int>::value)
    {
        unsigned long long int hash = keyToInt(key);
        // std::cout << "Hash value: " << hash << std::endl;
        return hash & ((1ll << globalDepth) - 1);
    }
    else return key & ((1ll << globalDepth) - 1);
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::insert(KeyType key, ValueType value) -> bool
{
    int index = getBucketNo(key);

    Bucket<KeyType, ValueType> *bptr = loadBucket(directory[index]);
    // std::cout<<"Trying to insert " << key << " in bucket: " << index << ", of size: " << bptr->bucketSize <<", Address = " << directory[index] << std::endl;
    if (bptr->insert(key, value))
    {
        saveBucket(directory[index], bptr);
        // std::cout<<"Inserted " << key << " in bucket: " << index << std::endl;
        return true;
    }
    // std::cout<<"Bucket is full, need to split"<<std::endl;
    splitBucket(index);
    return insert(key, value); // re-attempt after split
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::search(KeyType key) -> std::optional<ValueType>
{
    // std::cout<<"Search Key :"<<key<<std::endl;
    Bucket<KeyType, ValueType> *bptr = loadBucket(directory[getBucketNo(key)]);
    // std::cout << "Searching " << key << " in bucket: " << directory[getBucketNo(key)] << " of size: " << bptr->bucketList.size() << std::endl;
    return bptr->search(key);
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::deleteKey(KeyType key) -> bool
{
    Bucket<KeyType, ValueType> *bptr = loadBucket(directory[getBucketNo(key)]);
    bool val = bptr->deleteKey(key);
    if (val)
    {
        saveBucket(directory[getBucketNo(key)], bptr);
    }
    return val;
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::getGlobalDepth() -> int
{
    return globalDepth;
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::getDirectorySize() -> size_t
{
    return directory.size();
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::splitBucket(int index) -> void
{
    Bucket<KeyType, ValueType> *bptr = loadBucket(directory[index]);
    int localDepth, buddy_index, index_diff, dir_size;
    std::list<std::pair<KeyType, ValueType>> items = bptr->copy();
    localDepth = bptr->increaseDepth();
    if (localDepth > (int)globalDepth)
    {
        grow(); // Increase global depth
    }
    buddy_index = (index ^ (1 << (localDepth - 1)));
    // std::cout<<"Split bucket: "<<index<<", new buddy index: "<<buddy_index<<", Local depth="<<localDepth<<std::endl;
    directory[buddy_index] = createBucket();
    Bucket<KeyType, ValueType> *buddy_ptr = loadBucket(directory[buddy_index]);
    buddy_ptr->bucketSize = bptr->getMaxElementCount();
    buddy_ptr->localDepth = localDepth;
    // new Bucket(bptr->getMaxElementCount(), localDepth);
    bptr->clear();
    bptr->listSize = 0;
    index_diff = (1 << (localDepth));
    dir_size = (1 << ((int)globalDepth));
    for (int i = buddy_index - index_diff; i >= 0; i -= index_diff)
    {
        directory[i] = directory[buddy_index];
        saveBucket(directory[i], buddy_ptr);
    }
    for (int i = buddy_index + index_diff; i < dir_size; i += index_diff)
    {
        directory[i] = directory[buddy_index];
        saveBucket(directory[i], buddy_ptr);
    }
    saveBucket(directory[index], bptr);
    saveBucket(directory[buddy_index], buddy_ptr);
    for (auto &pair : items)
    {
        insert(pair.first, pair.second); // Reinsert items into the split buckets
    }
    // saveBucket(directory[index], bptr);
    // saveBucket(directory[buddy_index], buddy_ptr);
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::grow() -> void
{
    for (int i = 0; i < (1LL << globalDepth); i++)
    {
        // Bucket tmp(directory[i].getMaxElementCount(), directory[i].getLocalDepth());
        directory.push_back(directory[i]);
    }
    globalDepth++;
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::mergeBucket(int index) -> void
{
    int local_depth, dummy_index, index_diff, dir_size;
    Bucket<KeyType, ValueType> *bptr = loadBucket(directory[index]);
    local_depth = bptr->getLocalDepth();
    dummy_index = (index ^ (1 << (local_depth - 1)));
    Bucket<KeyType, ValueType> *dum_ptr = loadBucket(directory[dummy_index]);
    index_diff = (1 << (local_depth));
    dir_size = (1 << ((int)globalDepth));
    if (dum_ptr->getLocalDepth() == local_depth)
    {
        dum_ptr->decreaseDepth();
        // delete (directory[index]);
        // directory[index] = nullptr;
        destroyBucket(directory[index]);
        delete (bptr);
        directory[index] = directory[dummy_index];
        for (int i = index + index_diff; i < dir_size; i += index_diff)
        {
            directory[i] = directory[dummy_index];
        }
        for (int i = index - index_diff; i >= 0; i -= index_diff)
        {
            directory[i] = directory[dummy_index];
        }
    }
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::shrink() -> void
{
    for (size_t i = 0; i < directory.size(); i++)
    {
        Bucket<KeyType, ValueType> *bptr = loadBucket(directory[i]);
        if (bptr->getLocalDepth() == (int)globalDepth)
        {
            return; // Cannot shrink if any bucket has max depth
        }
    }
    globalDepth--;
    for (int i = 0; i < (1 << globalDepth); i++)
    {
        destroyBucket(directory.back());
        directory.pop_back();
    }
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::bucket_string(int n) const -> std::string
{
    int d;
    std::string s;
    Bucket<KeyType, ValueType> *bptr = loadBucket(directory[n]);
    d = bptr->getLocalDepth();
    s = "";
    while (n > 0 && d > 0)
    {
        s = (n % 2 == 0 ? "0" : "1") + s;
        n /= 2;
        d--;
    }
    while (d > 0)
    {
        s = "0" + s;
        d--;
    }
    return s;
}

template <typename KeyType, typename ValueType>
auto ExtendableHashIndex<KeyType, ValueType>::display(std::ostream &os) const -> void
{
    int size = 0;
    std::string s;
    std::set<std::string> shown;
    os << "Hash Index\nGlobal Depth: " << globalDepth << std::endl;

    for (size_t i = 0; i < directory.size(); i++)
    {
        s = bucket_string(i);
        if (shown.find(s) == shown.end())
        {
            shown.insert(s);
            Bucket<KeyType, ValueType> *bptr = loadBucket(directory[i]);
            if (bptr->isEmpty() == 0)
                size++;
        }
    }

    os << "Directory Size: " << (int)directory.size() << std::endl;
    shown.clear();
    std::vector<bucket_id_t> tempo = directory;
    for (size_t i = 0; i < directory.size(); i++)
    {
        s = bucket_string(i);
        if (shown.find(s) == shown.end())
        {
            shown.insert(s);
            Bucket<KeyType, ValueType> *bptr = loadBucket(tempo[i]);
            if ((bptr->isEmpty()) == 0)
            {
                os<<*(bptr)<<std::endl;
            }
        }
    }
    os << std::endl;
}

template<typename KeyType, typename ValueType>
std::ostream &operator<< (std::ostream &os, const ExtendableHashIndex<KeyType, ValueType> &hashIndex)
{
    hashIndex.display(os);
    return os;
}

template<typename KeyType, typename ValueType>
std::ostream &operator<< (std::ostream &os, const Bucket<KeyType, ValueType> &bucket)
{  
    bucket.display(os);
    return os;
}

template class ExtendableHashIndex<int, int>;
template class ExtendableHashIndex<int, std::string>;
template class ExtendableHashIndex<std::string, int>;
template class ExtendableHashIndex<std::string, std::string>;
template class ExtendableHashIndex<unsigned long long, unsigned long long>;
template class Bucket<int, int>;
template class Bucket<int, std::string>;
template class Bucket<std::string, int>;
template class Bucket<std::string, std::string>;
template class Bucket<unsigned long long, unsigned long long>;

template std::ostream &operator<< (std::ostream &os, const ExtendableHashIndex<int, int> &hashIndex);
template std::ostream &operator<< (std::ostream &os, const ExtendableHashIndex<int, std::string> &hashIndex);
template std::ostream &operator<< (std::ostream &os, const ExtendableHashIndex<std::string, int> &hashIndex);
template std::ostream &operator<< (std::ostream &os, const ExtendableHashIndex<std::string, std::string> &hashIndex);
template std::ostream &operator<< (std::ostream &os, const ExtendableHashIndex<unsigned long long, unsigned long long> &hashIndex);
template std::ostream &operator<< (std::ostream &os, const Bucket<int, int> &bucket);
template std::ostream &operator<< (std::ostream &os, const Bucket<int, std::string> &bucket);
template std::ostream &operator<< (std::ostream &os, const Bucket<std::string, int> &bucket);
template std::ostream &operator<< (std::ostream &os, const Bucket<std::string, std::string> &bucket);
template std::ostream &operator<< (std::ostream &os, const Bucket<unsigned long long, unsigned long long> &bucket);
