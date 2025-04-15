#include <Indexes/HashIndex.hpp>
#include <set>
#include <string>

auto Bucket::isFull ( ) -> bool
{
    return bucketList.size() >= bucketSize;
}

auto Bucket::isEmpty ( ) -> bool
{
    return bucketList.empty();
}

auto Bucket::insert ( KeyType key, ValueType value ) -> bool
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

auto Bucket::search ( KeyType key ) -> std::optional< ValueType >
{
    for (auto &pair : bucketList)
    {
        if (pair.first == key)
            return pair.second;
    }
    return std::nullopt;
}

auto Bucket::deleteKey ( KeyType key ) -> bool
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

auto Bucket::getLocalDepth ( ) -> int
{
    return localDepth;
}

auto Bucket::increaseDepth ( ) -> int
{
    return ++localDepth;
}

auto Bucket::decreaseDepth() -> int
{
    return --localDepth;
}

auto Bucket::copy ( ) -> std::list<std::pair<KeyType, ValueType>>
{
    return bucketList;
}

auto Bucket::clear ( ) -> void
{
    this->bucketList.clear();
    this->listSize=0;
}

auto Bucket::display ( ) -> void
{
    for (auto &[k, v] : bucketList)
        std::cout << "(" << k << ", " << v << ") ";
    std::cout << std::endl;
}

auto Bucket::getMaxElementCount ( ) -> size_t
{
    return bucketSize;
}

auto Bucket::getMaxSize ( ) -> size_t
{
    int keySize = sizeof(KeyType), valueSize = sizeof(ValueType);
    return sizeof(bucketSize) + sizeof(localDepth) + sizeof(listSize) + sizeof(bucket_id) + bucketSize * (keySize + valueSize);
}

// ======================== ExtendableHashIndex ========================

auto ExtendableHashIndex::createBucket ( ) -> bucket_id_t
{
    bucket_id_t id;
    if(free_ids.empty())
    {
        id = lastIDUsed++;
    }
    else{
        id = free_ids.back();
        free_ids.pop_back();
    }
    Bucket *bucket = new Bucket(order, 0); // Create a new bucket with size 2 and local depth 0
    bucket->bucket_id=id;
    saveBucket(id, bucket); // Save the new bucket to the directory
    // directory.push_back(id); // Add the bucket ID to the directory
    return id;
}

auto ExtendableHashIndex::destroyBucket ( bucket_id_t id ) -> void
{
    free_ids.push_back(id); // Add the bucket ID to the free list
    return;
}

 auto ExtendableHashIndex::loadBucket ( bucket_id_t id ) -> Bucket*
 {
    Bucket *bp = new Bucket;
    
    // std::cout << "Trying to read from address: " << base_address + id*bp->getMaxSize() << " of size: " << bp->getMaxSize() << std::endl;
    std::vector< std::byte > data = buffer_manager->readAddress(base_address + id*bp->getMaxSize(), bp->getMaxSize());
    // std::cout << "Read data size: " << data.size() << std::endl;

    size_t curr = 0;
    bp->bucketSize = *reinterpret_cast<size_t*>(data.data());
    curr += sizeof(size_t);
    // std::cout << "Bucket size: " << bp->bucketSize << std::endl;

    bp->localDepth = *reinterpret_cast<int*>(data.data()+curr);
    curr += sizeof(int);
    // std::cout << "Local depth: " << bp->localDepth << std::endl;

    bp->listSize = *reinterpret_cast<int*>(data.data()+curr);
    curr += sizeof(int);
    // std::cout << "List size: " << bp->listSize << std::endl;

    bp->bucket_id = *reinterpret_cast<bucket_id_t*>(data.data()+curr);
    curr += sizeof(bucket_id_t);
    // std::cout << "Bucket ID: " << bp->bucket_id << std::endl;

    for ( size_t i=0; i<bp->bucketSize; i++)
    {
        KeyType key = *reinterpret_cast<KeyType*>(data.data()+curr);
        curr += sizeof(KeyType);
        ValueType value = *reinterpret_cast<ValueType*>(data.data()+curr);
        curr += sizeof(ValueType);
        // std::cout << "Element " << i << ": (" << key << ", " << value << ")" << std::endl;
    
        if ( i < (size_t)bp->listSize )
        {
            bp->bucketList.push_back(std::make_pair(key,value));
        }
    }
    // std::cout << "Loaded bucket: " << id << std::endl;

    return bp;
}

auto ExtendableHashIndex::saveBucket ( bucket_id_t id, Bucket*bp ) -> void
{
    std::vector<std::byte>data(bp->getMaxSize(),std::byte(0));
    size_t curr=0;
    std::copy(reinterpret_cast<std::byte*>(&bp->bucketSize),reinterpret_cast<std::byte*>(&bp->bucketSize)+sizeof(size_t),data.data()+curr);
    curr+=sizeof(size_t);

    std::copy(reinterpret_cast<std::byte*>(&bp->localDepth),reinterpret_cast<std::byte*>(&bp->localDepth)+sizeof(int),data.data()+curr);
    curr+=sizeof(int);

    std::copy(reinterpret_cast<std::byte*>(&bp->listSize),reinterpret_cast<std::byte*>(&bp->listSize)+sizeof(int),data.data()+curr);
    curr+=sizeof(int);

    std::copy(reinterpret_cast<std::byte*>(&bp->bucket_id),reinterpret_cast<std::byte*>(&bp->bucket_id)+sizeof(bucket_id_t),data.data()+curr);
    curr+=sizeof(bucket_id_t);

    for(auto&pair:bp->bucketList){
        std::copy(reinterpret_cast<std::byte*>(&pair.first),reinterpret_cast<std::byte*>(&pair.first)+sizeof(KeyType),data.data()+curr);
        curr+=sizeof(KeyType);
        std::copy(reinterpret_cast<std::byte*>(&pair.second),reinterpret_cast<std::byte*>(&pair.second)+sizeof(ValueType),data.data()+curr);
        curr+=sizeof(ValueType);
    }
    buffer_manager->writeAddress(base_address+id*bp->getMaxSize(),data);
    return;
}

auto ExtendableHashIndex::getBucketNo ( KeyType key ) -> int
{
    return key & ((1ll << globalDepth) - 1);
}

auto ExtendableHashIndex::insert ( KeyType key, ValueType value ) -> bool
{
    int index = getBucketNo(key);

    Bucket *bptr = loadBucket(directory[index]);

    if (bptr->insert(key, value)){
        saveBucket(directory[index],bptr);
        return true;
    }
    // std::cout<<"Bucket is full, need to split"<<std::endl;
    splitBucket(index);
    return insert(key, value); // re-attempt after split
}

auto ExtendableHashIndex::search ( KeyType key ) -> std::optional<ValueType>
{
    Bucket*bptr=loadBucket(directory[getBucketNo(key)]);
    std::cout<<"Searching "<<key<<" in bucket: "<<directory[getBucketNo(key)]<<std::endl;
    return bptr->search(key);
}

auto ExtendableHashIndex::deleteKey ( KeyType key ) -> bool
{
    Bucket*bptr=loadBucket(directory[getBucketNo(key)]);
    bool val= bptr->deleteKey(key);
    if(val){
        saveBucket(directory[getBucketNo(key)],bptr);
    }
    return val;
}

auto ExtendableHashIndex::getGlobalDepth ( ) -> int
{
    return globalDepth;
}

auto ExtendableHashIndex::getDirectorySize ( ) -> size_t
{
    return directory.size();
}

auto ExtendableHashIndex::splitBucket ( int index ) -> void
{
    Bucket*bptr=loadBucket(directory[index]);
    int localDepth, buddy_index, index_diff, dir_size;
    std::list<std::pair<KeyType, ValueType>> items = bptr->copy();
    localDepth = bptr->increaseDepth();
    if (localDepth > (int)globalDepth)
    {
        grow(); // Increase global depth
    }
    buddy_index = (index ^ (1 << (localDepth - 1)));
    // std::cout<<"Split bucket: "<<index<<", new buddy index: "<<buddy_index<<", Local depth="<<localDepth<<std::endl;
    directory[buddy_index] =createBucket();
    Bucket*buddy_ptr=loadBucket(directory[buddy_index]); 
    buddy_ptr->bucketSize=bptr->getMaxElementCount();
    buddy_ptr->localDepth=localDepth;
    // new Bucket(bptr->getMaxElementCount(), localDepth); 
    bptr->clear();
    bptr->listSize=0;
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

auto ExtendableHashIndex::grow ( ) -> void
{
    for (int i = 0; i < (1LL << globalDepth); i++)
    {
        // Bucket tmp(directory[i].getMaxElementCount(), directory[i].getLocalDepth());
        directory.push_back(directory[i]);
    }
    globalDepth++;
}

auto ExtendableHashIndex::mergeBucket ( int index ) -> void
{
    int local_depth, dummy_index, index_diff, dir_size;
    Bucket*bptr=loadBucket(directory[index]);
    local_depth = bptr->getLocalDepth();
    dummy_index = (index ^ (1 << (local_depth - 1)));
    Bucket*dum_ptr=loadBucket(directory[dummy_index]);
    index_diff = (1 << (local_depth));
    dir_size = (1 << ((int)globalDepth));
    if (dum_ptr->getLocalDepth() == local_depth)
    {
        dum_ptr->decreaseDepth();
        // delete (directory[index]);
        // directory[index] = nullptr;
        destroyBucket(directory[index]);
        delete(bptr);
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

auto ExtendableHashIndex::shrink ( ) -> void
{
    for (size_t i = 0; i < directory.size(); i++)
    {
        Bucket*bptr=loadBucket(directory[i]);
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

auto ExtendableHashIndex::bucket_string ( int n ) -> std::string
{
    int d;
    std::string s;
    Bucket*bptr=loadBucket(directory[n]);
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

auto ExtendableHashIndex::display ( ) -> void
{
    int size = 0;
    std::string s;
    std::set<std::string> shown;
    std::cout << "Hash Index\nGlobal Depth: " << globalDepth << std::endl;

    for (size_t i = 0; i < directory.size(); i++)
    {
        s = bucket_string(i);
        if (shown.find(s) == shown.end())
        {
            shown.insert(s);
            Bucket*bptr=loadBucket(directory[i]);
            if (bptr->isEmpty() == 0)
                size++;
        }
    }

    std::cout << "Directory Size: " << (int)directory.size() << std::endl;
    shown.clear();
    std::vector<bucket_id_t> tempo = directory;
    for (size_t i = 0; i < directory.size(); i++)
    {
        s = bucket_string(i);
        if (shown.find(s) == shown.end())
        {
            shown.insert(s);
            Bucket*bptr=loadBucket(tempo[i]);
            if ((bptr->isEmpty()) == 0)
            {
                bptr->display();
            }
        }
    }
    std::cout << std::endl;
}
