#include <Utilities/Utils.hpp>
#include <Storage/BufferManager.hpp>
#include <Storage/Disk.hpp>
#include <Indexes/HashIndex.hpp>

#include <iostream>
#include <vector>
#include <optional>

address_id_t employeeStartAddress = 0;
address_id_t employeeEndAddress = 0;
address_id_t companyStartAddress = 0;
address_id_t companyEndAddress = 0;

const storage_t blockSize = (4 KB);
const storage_t bufferSize = (64 KB);
const storage_t diskSize = (4 MB);
const int replaceStrategy = LRU;
const int accessType = RANDOM;

std::ofstream outFile(STAT_DIR + "hash_index_stats.txt", std::ios::out | std::ios::trunc);

int help(storage_t blockSize, storage_t diskSize, storage_t bufferSize, int replaceStrategy, int accessType){
    Disk disk(accessType, blockSize, diskSize);
    BufferManager bm(&disk, replaceStrategy, bufferSize);

    auto stat = bm.getStats();

    ExtendableHashIndex<int, address_id_t> comp_index(&bm, 2, 0, companyEndAddress);
    for( int i = 0; i < 200; ++i )
    {
        address_id_t addr = companyStartAddress + i * sizeof(Company);
        Company comp = extractData<Company>(bm.readAddress(addr, sizeof(Company)));
        comp_index.insert( comp.id, addr );
        // std::cout << "Inserted company ID: " << comp.id << " with record at address: " << addr << std::endl;
    }
    // std::cout<<comp_index<<std::endl;
    auto [compStartIndex, compEndIndex] = comp_index.getAddressRange();

    bm.printStats(outFile, stat, "Statistics for the creation of Hash Index");
    stat = bm.getStats();

    address_id_t joinAddress = compEndIndex;
    for(address_id_t addr = employeeStartAddress; addr < employeeEndAddress; addr += sizeof(Employee))
    {
        Employee emp = extractData<Employee>(bm.readAddress(addr, sizeof(Employee)));
        auto compAddr = comp_index.search(emp.company_id);
        if(!compAddr.has_value())
        {
            std::cerr << "Company ID " << emp.company_id << " not found in index for employee ID " << emp.id << std::endl;
            continue;
        }
        // std::cout << "Joining Employee ID: " << emp.id << " with Company ID: " << emp.company_id << std::endl;
        Company comp = extractData<Company>(bm.readAddress(compAddr.value(), sizeof(Company)));
        JoinEmployeeCompany joinData(emp, comp);
        bm.writeAddress(joinAddress, std::vector<std::byte>(reinterpret_cast<std::byte *>(&joinData), reinterpret_cast<std::byte *>(&joinData) + sizeof(JoinEmployeeCompany)));
        joinAddress += sizeof(JoinEmployeeCompany);
    }

    storeResult<JoinEmployeeCompany>(bm, compEndIndex, joinAddress, RES_DIR + "hash_index_join_data_" + (replaceStrategy == LRU ? "lru" : "mru") + "_" + (accessType == RANDOM ? "rand" : "seq") + ".csv");

    return 0;

}

int main() {
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

    help((4 KB), (4 MB), (64 KB), LRU, RANDOM);
    help((4 KB), (4 MB), (64 KB), LRU, SEQUENTIAL);
    help((4 KB), (4 MB), (64 KB), MRU, RANDOM);
    help((4 KB), (4 MB), (64 KB), MRU, SEQUENTIAL);

    outFile.close();
    outFile.clear();
    std::cout << "Statistics saved to " << STAT_DIR + "hash_join_stats.txt" << std::endl;

    return 0;

    // std::cout << "Employee Start Address: " << employeeStartAddress << std::endl;
    // std::cout << "Employee End Address: " << employeeEndAddress << std::endl;
    // std::cout << "Company Start Address: " << companyStartAddress << std::endl;
    // std::cout << "Company End Address: " << companyEndAddress << std::endl;

    // ExtendableHashIndex<int,int> comp_index(&bm, 2, 0, companyEndAddress);
    // for( int i = 0; i < 200; ++i )
    // {
    //     address_id_t addr = companyStartAddress + i * sizeof(Company);
    //     Company comp = extractData<Company>(bm.readAddress(addr, sizeof(Company)));
    //     comp_index.insert( comp.id, addr );
    //     // std::cout << "Inserted company ID: " << comp.id << " with record at address: " << addr << std::endl;
    // }
    // // std::cout<<comp_index<<std::endl;
    // auto [compStartIndex, compEndIndex] = comp_index.getAddressRange();

    // std::cout << "Company Index Start Address: " << compStartIndex << std::endl;
    // std::cout << "Company Index End Address: " << compEndIndex << std::endl;

    // std::cout << "========================================================" << std::endl;
    // std::cout << "Statistics of the creation of Hash Indices (Before Join)" << std::endl;
    // std::cout << "\t\tDisk IO operations: " << bm.getNumIO() << std::endl;
    // std::cout << "\t\tDisk IO cost: " << bm.getCostIO() << std::endl;
    // std::cout << "\t\t(FrameSize: " << blockSize << ", FrameCount: " << bm.getNumFrames() << ")" << std::endl;
    // std::cout << "\t\t(ReplacementStrategy: " << (replaceStrategy == LRU ? "LRU" : "MRU") << ", DiskAccessStrategy: " << (accessType == RANDOM ? "RANDOM" : "SEQUENTIAL") << ")" << std::endl;

    // address_id_t joinAddress = compEndIndex;
    // for(address_id_t addr = employeeStartAddress; addr < employeeEndAddress; addr += sizeof(Employee))
    // {
    //     Employee emp = extractData<Employee>(bm.readAddress(addr, sizeof(Employee)));
    //     auto compAddr = comp_index.search(emp.company_id);
    //     if(!compAddr.has_value())
    //     {
    //         std::cerr << "Company ID " << emp.company_id << " not found in index for employee ID " << emp.id << std::endl;
    //         continue;
    //     }
    //     std::cout << "Joining Employee ID: " << emp.id << " with Company ID: " << emp.company_id << std::endl;
    //     Company comp = extractData<Company>(bm.readAddress(compAddr.value(), sizeof(Company)));
    //     JoinEmployeeCompany joinData(emp, comp);
    //     bm.writeAddress(joinAddress, std::vector<std::byte>(reinterpret_cast<std::byte *>(&joinData), reinterpret_cast<std::byte *>(&joinData) + sizeof(JoinEmployeeCompany)));
    //     joinAddress += sizeof(JoinEmployeeCompany);
    // }

    // storeResult<JoinEmployeeCompany>(bm, compEndIndex, joinAddress, RES_DIR + "hash_index_join_result.csv");

    // std::cout << "========================================================" << std::endl;
    // std::cout << "Statistics of the creation of Hash Indices (Before Join)" << std::endl;
    // std::cout << "\t\tDisk IO operations: " << bm.getNumIO() << std::endl;
    // std::cout << "\t\tDisk IO cost: " << bm.getCostIO() << std::endl;
    // std::cout << "\t\t(FrameSize: " << blockSize << ", FrameCount: " << bm.getNumFrames() << ")" << std::endl;
    // std::cout << "\t\t(ReplacementStrategy: " << (replaceStrategy == LRU ? "LRU" : "MRU") << ", DiskAccessStrategy: " << (accessType == RANDOM ? "RANDOM" : "SEQUENTIAL") << ")" << std::endl;

    // return 0;
}