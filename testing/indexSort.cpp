#include <Utilities/Utils.hpp>
#include <Storage/BufferManager.hpp>
#include <Storage/Disk.hpp>
#include <Indexes/BPlusTreeIndex.hpp>

#include <iostream>
#include <vector>
#include <string>

address_id_t employeeStartAddress = 0;
address_id_t employeeEndAddress = 0;
address_id_t companyStartAddress = 0;
address_id_t companyEndAddress = 0;

std::ofstream outFile(STAT_DIR + "bplus_tree_stats.txt", std::ios::out | std::ios::trunc);

int help(storage_t blockSize, storage_t diskSize, storage_t bufferSize, int replaceStrategy, int accessType)
{
    Disk disk(accessType, blockSize, diskSize);
    BufferManager bm(&disk, replaceStrategy, bufferSize);

    auto stat = bm.getStats();

    BPlusTreeIndex< unsigned long long, address_id_t > emp_index(&bm, 10, companyEndAddress);
    for ( int i = 0; i < EMP_SIZE; ++i )
    {
        address_id_t addr = employeeStartAddress + i * sizeof(Employee);
        Employee emp = extractData<Employee>(bm.readAddress(addr, sizeof(Employee)));
        emp_index.insert( emp.company_id * 1e5 + emp.id, addr );
    }
    auto [empStartIndex, empEndIndex] = emp_index.getAddressRange();

    BPlusTreeIndex< unsigned long long, address_id_t > comp_index(&bm, 10, empEndIndex);
    for( int i = 0; i < COMP_SIZE; ++i )
    {
        address_id_t addr = companyStartAddress + i * sizeof(Company);
        Company comp = extractData<Company>(bm.readAddress(addr, sizeof(Company)));
        comp_index.insert( comp.id, addr );
    }
    auto [compStartIndex, compEndIndex] = comp_index.getAddressRange();

    bm.printStats(outFile, stat, "Statistics for the creation of B+ Tree Index");
    stat = bm.getStats();

    auto empBegin = emp_index.begin();
    auto compBegin = comp_index.begin();

    address_id_t joinAddr = compEndIndex;
    while( empBegin != emp_index.end() && compBegin != comp_index.end() )
    {
        auto [empKey, empValue] = *empBegin;
        auto [compKey, compValue] = *compBegin;

        if( empKey / (int) 1e5 == compKey )
        {
            Employee emp = extractData<Employee>(bm.readAddress(empValue, sizeof(Employee)));
            Company comp = extractData<Company>(bm.readAddress(compValue, sizeof(Company)));
            JoinEmployeeCompany joinData(emp, comp);
            bm.writeAddress(joinAddr, std::vector<std::byte>(reinterpret_cast<std::byte*>(&joinData), reinterpret_cast<std::byte*>(&joinData) + sizeof(JoinEmployeeCompany)));
            joinAddr += sizeof(JoinEmployeeCompany);
            ++empBegin;
        }
        else if( empKey / (int) 1e5 < compKey ) ++empBegin;
        else ++compBegin;
    }

    bm.printStats(outFile, stat, "Statistics for the Join using Index operation");

    // Store the result in a file
    storeResult<JoinEmployeeCompany>(bm, compEndIndex, joinAddr, RES_DIR + "bplus_index_joined_data_" + (replaceStrategy == LRU ? "lru" : "mru") + "_" + (accessType == RANDOM ? "rand" : "seq") + ".csv");

    return 0;
}

int main()
{
    auto [a,b,c,d] = loadData();
    employeeStartAddress = a;
    employeeEndAddress = b;
    companyStartAddress = c;
    companyEndAddress = d;

    if(!outFile.is_open())
    {
        std::cerr << "Error opening file for writing statistics." << std::endl;
        return 1;
    }

    help(BLOCK_SIZE, DISK_SIZE, BUFFER_SIZE, LRU, RANDOM);
    help(BLOCK_SIZE, DISK_SIZE, BUFFER_SIZE, LRU, SEQUENTIAL);
    help(BLOCK_SIZE, DISK_SIZE, BUFFER_SIZE, MRU, RANDOM);
    help(BLOCK_SIZE, DISK_SIZE, BUFFER_SIZE, MRU, SEQUENTIAL);

    outFile.close();
    outFile.clear();
    std::cout << "Statistics saved to " << STAT_DIR + "bplus_tree_stats.txt" << std::endl;

    return 0;
}