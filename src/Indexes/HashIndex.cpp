#include <Indexes/HashIndex.hpp>

using KeyType=int;
using ValueType=int;

bool Bucket::isFull()
{
    return bucketList.size() >= bucketSize;
}

bool Bucket::isEmpty()
{
    return bucketList.empty();
}

bool Bucket::insert(KeyType key, ValueType value)
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

std::optional<ValueType> Bucket::search(KeyType key)
{
    for (auto &pair : bucketList)
    {
        if (pair.first == key)
            return pair.second;
    }
    return std::nullopt;
}

bool Bucket::deleteKey(KeyType key)
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

int Bucket::getLocalDepth()
{
    return localDepth;
}

int Bucket::increaseDepth()
{
    return ++localDepth;
}

int Bucket::decreaseDepth()
{
    return --localDepth;
}

std::list<std::pair<KeyType, ValueType>> Bucket::copy(void)
{
    return bucketList;
}

void Bucket::clear(void)
{
    this->bucketList.clear();
}

void Bucket::display(void)
{
    for (auto &[k, v] : bucketList)
        std::cout << "(" << k << ", " << v << ") ";
    std::cout << std::endl;
}

size_t Bucket::getBucketSize()
{
    return bucketSize;
}

auto Bucket::getActualSize() -> size_t
{
    int keySize = sizeof(KeyType),valueSize = sizeof(ValueType);
    return sizeof(bucketSize)+sizeof(localDepth)+sizeof(listSize)+sizeof(bucket_id)+bucketSize*(keySize+valueSize);
}

// ======================== ExtendableHashIndex ========================

auto ExtendableHashIndex::createBucket() -> bucket_id_t
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
    Bucket *bucket = new Bucket(2, 0); // Create a new bucket with size 2 and local depth 0
    saveBucket(id, bucket); // Save the new bucket to the directory
    directory.push_back(id); // Add the bucket ID to the directory
    return id;
}

auto ExtendableHashIndex::destroyBucket(bucket_id_t id) -> void
{
    free_ids.push_back(id); // Add the bucket ID to the free list
    return;
}

Bucket* ExtendableHashIndex::loadBucket(bucket_id_t id){
    Bucket*bp=new Bucket;
    std::vector<std::byte>data=buffer_manager->readAddress(base_address+id*bp->getActualSize(),bp->getActualSize());

    size_t curr=0;
    bp->bucketSize=*reinterpret_cast<size_t*>(data.data());
    curr+=sizeof(size_t);

    bp->localDepth=*reinterpret_cast<int*>(data.data()+curr);
    curr+=sizeof(int);

    bp->listSize=*reinterpret_cast<int*>(data.data()+curr);
    curr+=sizeof(int);

    bp->bucket_id=*reinterpret_cast<bucket_id_t*>(data.data()+curr);
    curr+=sizeof(bucket_id_t);

    for(int i=0;i<bp->bucketSize;i++){
        KeyType key=*reinterpret_cast<KeyType*>(data.data()+curr);
        curr+=sizeof(KeyType);
        ValueType value=*reinterpret_cast<ValueType*>(data.data()+curr);
        curr+=sizeof(ValueType);
    
        if(i<bp->listSize){
            bp->bucketList.push_back(std::make_pair(key,value));
        }
    }

    return bp;
}

auto ExtendableHashIndex::saveBucket(bucket_id_t id, Bucket*bp) -> void
{
    std::vector<std::byte>data(bp->getActualSize(),std::byte(0));
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
    buffer_manager->writeAddress(base_address+id*bp->getActualSize(),data);
    return;
}

int ExtendableHashIndex::getBucketNo(KeyType key)
{
    return key & ((1 << globalDepth) - 1);
}

bool ExtendableHashIndex::insert(KeyType key, ValueType value)
{
    int index = getBucketNo(key);

    Bucket*bptr=loadBucket(directory[index]);

    if (bptr->insert(key, value)){
        saveBucket(directory[index],bptr);
        return true;
    }
    // std::cout<<"Bucket is full, need to split\n";
    splitBucket(index);
    return insert(key, value); // re-attempt after split
}

std::optional<ValueType> ExtendableHashIndex::search(KeyType key)
{
    Bucket*bptr=loadBucket(directory[getBucketNo(key)]);
    return bptr->search(key);
}

bool ExtendableHashIndex::deleteKey(KeyType key)
{
    Bucket*bptr=loadBucket(directory[getBucketNo(key)]);
    return bptr->deleteKey(key);
}

int ExtendableHashIndex::getGlobalDepth()
{
    return globalDepth;
}

size_t ExtendableHashIndex::getDirectorySize()
{
    return directory.size();
}

void ExtendableHashIndex::splitBucket(int index)
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
    buddy_ptr->bucketSize=bptr->getBucketSize();
    buddy_ptr->localDepth=localDepth;
    // new Bucket(bptr->getBucketSize(), localDepth); 
    bptr->clear();
    index_diff = (1 << (localDepth));
    dir_size = (1 << ((int)globalDepth));
    for (int i = buddy_index - index_diff; i >= 0; i -= index_diff)
    {
        directory[i] = directory[buddy_index];
    }
    for (int i = buddy_index + index_diff; i < dir_size; i += index_diff)
    {
        directory[i] = directory[buddy_index];
    }
    for (auto &pair : items)
    {
        insert(pair.first, pair.second); // Reinsert items into the split buckets
    }
}

void ExtendableHashIndex::grow()
{
    for (int i = 0; i < (1LL << globalDepth); i++)
    {
        // Bucket tmp(directory[i].getBucketSize(), directory[i].getLocalDepth());
        directory.push_back(directory[i]);
    }
    globalDepth++;
}

void ExtendableHashIndex::mergeBucket(int index)
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
        delete(bptr)
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

void ExtendableHashIndex::shrink()
{
    for (size_t i = 0; i < directory.size(); i++)
    {
        if (directory[i]->getLocalDepth() == (int)globalDepth)
        {
            return; // Cannot shrink if any bucket has max depth
        }
    }
    globalDepth--;
    for (int i = 0; i < (1 << globalDepth); i++)
    {
        directory.pop_back();
    }
}

std::string ExtendableHashIndex::bucket_id(int n)
{
    int d;
    std::string s;
    d = (directory[n])->getLocalDepth();
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

void ExtendableHashIndex::display()
{
    int size = 0;
    std::string s;
    std::set<std::string> shown;
    std::cout << "Global Depth: " << globalDepth << std::endl;

    for (size_t i = 0; i < directory.size(); i++)
    {
        s = bucket_id(i);
        if (shown.find(s) == shown.end())
        {
            shown.insert(s);
            if (directory[i]->isEmpty() == 0)
                size++;
        }
    }

    std::cout << "Directory Size: " << size << std::endl;
    shown.clear();
    std::vector<Bucket *> tempo = directory;
    for (size_t i = 0; i < directory.size(); i++)
    {
        s = bucket_id(i);
        if (shown.find(s) == shown.end())
        {
            shown.insert(s);
            if (tempo[i]->isEmpty() == 0)
            {
                tempo[i]->display();
            }
        }
    }
}

// using namespace std;

void testInsertSearch(ExtendableHashIndex &index)
{
    std::cout << "\n=== Insert & Search Test ===\n";
    index.insert(1, 100);
    index.insert(5, 500);
    index.insert(9, 900);
    index.display(); // View current directory

    auto res1 = index.search(5);
    std::cout << "Search key 5: " << (res1.has_value() ? std::to_string(res1.value()) : "Not Found") << std::endl;

    auto res2 = index.search(99);
    std::cout << "Search key 99: " << (res2.has_value() ? std::to_string(res2.value()) : "Not Found") << std::endl;
}

void testDelete(ExtendableHashIndex &index)
{
    std::cout << "\n=== Delete Test ===\n";
    index.deleteKey(5);
    index.display(); // View structure after deletion

    auto res = index.search(5);
    std::cout << "Search key 5 after delete: " << (res.has_value() ? std::to_string(res.value()) : "Not Found") << std::endl;
}

void testSplitAndMerge(ExtendableHashIndex &index)
{
    std::cout << "\n=== Split & Merge Test ===\n";

    // Insert keys to cause splits
    for (int i = 0; i < 20; ++i)
    {
        index.insert(i, i * 10);
        std::cout << "\nInserted " << i << std::endl;
        index.display();
    }
    // index.display(); // After multiple inserts

    // Now delete keys to cause merges/shrink
    for (int i = 0; i < 20; ++i)
    {
        index.deleteKey(i);
    }
    index.display(); // After deletes

    std::cout << "Global Depth After Deletes: " << index.getGlobalDepth() << std::endl;
}

int main()
{
    ExtendableHashIndex index(0); // Start with global depth 2

    // testInsertSearch(index);
    // testDelete(index);
    testSplitAndMerge(index);

    return 0;
}
