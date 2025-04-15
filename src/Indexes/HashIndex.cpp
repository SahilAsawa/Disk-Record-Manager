#include <Indexes/HashIndex.hpp>

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

// ======================== ExtendableHashIndex ========================

int ExtendableHashIndex::getBucketNo(KeyType key)
{
    return key & ((1 << globalDepth) - 1);
}

bool ExtendableHashIndex::insert(KeyType key, ValueType value)
{
    int index = getBucketNo(key);

    if (directory[index]->insert(key, value))
        return true;
    // std::cout<<"Bucket is full, need to split\n";
    splitBucket(index);
    return insert(key, value); // re-attempt after split
}

std::optional<ValueType> ExtendableHashIndex::search(KeyType key)
{
    return directory[getBucketNo(key)]->search(key);
}

bool ExtendableHashIndex::deleteKey(KeyType key)
{
    return directory[getBucketNo(key)]->deleteKey(key);
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
    int localDepth,buddy_index,index_diff,dir_size;
    std::list<std::pair<KeyType, ValueType>> items = directory[index]->copy();
    localDepth = directory[index]->increaseDepth();
    if(localDepth > (int)globalDepth)
    {
        grow(); // Increase global depth
    }
    buddy_index = (index ^ (1 << (localDepth - 1)));
    // std::cout<<"Split bucket: "<<index<<", new buddy index: "<<buddy_index<<", Local depth="<<localDepth<<std::endl;
    directory[buddy_index] = new Bucket(directory[index]->getBucketSize(), localDepth); // Create new buddy bucket
    directory[index]->clear();
    index_diff = (1 << (localDepth));
    dir_size = (1<<((int)globalDepth));
    for(int i=buddy_index-index_diff;i>=0;i-=index_diff)
    {
        directory[i] = directory[buddy_index];
    }
    for(int i=buddy_index+index_diff;i<dir_size;i+=index_diff)
    {
        directory[i] = directory[buddy_index];
    }
    for(auto &pair : items)
    {
        insert(pair.first, pair.second); // Reinsert items into the split buckets
    }
}

void ExtendableHashIndex::grow()
{
    for(int i=0;i<(1LL<<globalDepth);i++)
    {
        // Bucket tmp(directory[i].getBucketSize(), directory[i].getLocalDepth());
        directory.push_back(directory[i]);
    }
    globalDepth++;
   
}

void ExtendableHashIndex::mergeBucket(int index)
{
    int local_depth,dummy_index,index_diff,dir_size;
    local_depth = directory[index]->getLocalDepth();
    dummy_index = (index ^ (1 << (local_depth - 1)));
    index_diff = (1 << (local_depth));
    dir_size = (1<<((int)globalDepth));
    if(directory[dummy_index]->getLocalDepth()==local_depth)
    {
        directory[dummy_index]->decreaseDepth();
        delete(directory[index]);
        directory[index]=nullptr;
        directory[index]=directory[dummy_index];
        for(int i=index+index_diff;i<dir_size;i+=index_diff)
        {
            directory[i] = directory[dummy_index];
        }
        for(int i=index-index_diff;i>=0;i-=index_diff)
        {
            directory[i] = directory[dummy_index];
        }
    }
}

void ExtendableHashIndex::shrink()
{
    for(size_t i=0;i<directory.size();i++)
    {
        if(directory[i]->getLocalDepth() == (int)globalDepth)
        {
            return; // Cannot shrink if any bucket has max depth
        }
    }
    globalDepth--;
    for(int i=0;i<(1<<globalDepth);i++){
        directory.pop_back();
    }
}

std::string ExtendableHashIndex::bucket_id(int n){
    int d;
    std::string s;
    d = (directory[n])->getLocalDepth();
    s = "";
    while(n>0 && d>0){
        s = (n%2==0?"0":"1")+s;
        n/=2;
        d--;
    }
    while(d>0){
        s = "0"+s;
        d--;
    }
    return s;
}