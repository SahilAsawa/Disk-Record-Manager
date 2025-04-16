#include <Indexes/HashIndex.hpp>
#include <Indexes/BPlusTreeIndex.hpp>
#include <Storage/BufferManager.hpp>
#include <Storage/Disk.hpp>
#include <Utilities/Utils.hpp>

#define BLOCK_SIZE 4 KB
#define DISK_SIZE 4 MB
#define BUFFER_SIZE 8 KB


address_id_t empStartAddr, empEndAddr, compStartAddr, compEndAddr;

std::ofstream iterRes(RES_DIR + "queryiter_results.txt", std::ios::out | std::ios::trunc);
std::ofstream bptRes(RES_DIR + "querybpt_results.txt", std::ios::out | std::ios::trunc);
std::ofstream iterStats(STAT_DIR + "queryiter_stats.txt", std::ios::out | std::ios::trunc);
std::ofstream bptStats(STAT_DIR + "querybpt_stats.txt", std::ios::out | std::ios::trunc);

void usingBPT(int accessType, int replaceStrat, BPlusTreeIndex<int, int> &empIndex)
{
    Disk disk(accessType, BLOCK_SIZE, DISK_SIZE);
    BufferManager bm(&disk, replaceStrat, BUFFER_SIZE);

    auto stat = bm.getStats();

    // print all employee id whose salary is between 40000 and 70000
    int low = 40000 * 1000, high = 70001 * 1000;
    auto result = empIndex.rangeSearch(low, high);

    // print results in a file depending on the access type and replace strategy
    bptRes.clear();
    bptRes.seekp(0, std::ios::beg);
    for (const auto &entry : result)
    {
        auto [key, addr] = entry;
        Employee emp = extractData<Employee>(bm.readAddress(addr, sizeof(Employee)));
        bptRes << emp.toString() << std::endl;
    }

    bm.printStats(bptStats, stat, "Statistics for query using B+ Tree Index");
}

void usingIterating(int accessType, int replaceStrat)
{
    Disk disk(accessType, BLOCK_SIZE, DISK_SIZE);
    BufferManager bm(&disk, replaceStrat, BUFFER_SIZE);

    auto stat = bm.getStats();

    // print all employee id whose salary is between 40000 and 70000
    int low = 40000, high = 70000;

    iterRes.clear();
    iterRes.seekp(0, std::ios::beg);
    for (int i = 0; i < 1000; ++i)
    {
        address_id_t addr = empStartAddr + i * sizeof(Employee);
        Employee emp = extractData<Employee>(bm.readAddress(addr, sizeof(Employee)));
        if (emp.salary >= low && emp.salary <= high)
        {
            iterRes << emp.toString() << std::endl;
        }
    }

    bm.printStats(iterStats, stat, "Statistics for the query using Iterating Method");
}

int main()
{
    auto [a, b, c, d] = loadData(4 KB, 4 MB, 64 KB);
    empStartAddr = a;
    empEndAddr = b;
    compStartAddr = c;
    compEndAddr = d;

    // create BPlusTree index
    Disk disk(RANDOM, BLOCK_SIZE, DISK_SIZE);
    BufferManager bm(&disk, LRU, BUFFER_SIZE);
    BPlusTreeIndex<int, int> empIndex(&bm, 7, compEndAddr);
    // create index on salary of employee
    for (int i = 0; i < 1000; ++i)
    {
        address_id_t addr = empStartAddr + i * sizeof(Employee);
        Employee emp = extractData<Employee>(bm.readAddress(addr, sizeof(Employee)));
        empIndex.insert(emp.salary * 1000 + emp.id, addr);
    }

    Stats stat = {0, 0, 0};
    bm.printStats(bptStats, stat, "Statistics for the creation of B+ Tree Index");

    usingBPT(RANDOM, LRU, empIndex);
    usingBPT(SEQUENTIAL, LRU, empIndex);
    usingBPT(RANDOM, MRU, empIndex);
    usingBPT(SEQUENTIAL, MRU, empIndex);
    usingIterating(RANDOM, LRU);
    usingIterating(SEQUENTIAL, LRU);
    usingIterating(RANDOM, MRU);
    usingIterating(SEQUENTIAL, MRU);
}