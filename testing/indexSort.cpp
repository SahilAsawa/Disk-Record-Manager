#include <Utilities/Utils.hpp>
#include <Storage/BufferManager.hpp>
#include <Storage/Disk.hpp>
#include <Indexes/BPlusTreeIndex.hpp>

#include <iostream>
#include <vector>
#include <optional>
#include <string>

std::string to_string(const std::array<char, 58>& arr)
{
    for(size_t i = 0; i < arr.size(); ++i)
    {
        if (arr[i] == '\0')
        {
            return std::string(arr.data(), i);
        }
    }
    return std::string(arr.data(), arr.size());
}

int help(int blockSize, int frameCount, int replaceStrategy, int accessType)
{
    Disk disk(accessType, blockSize, 1024);
    BufferManager bm(&disk, replaceStrategy, frameCount);

    auto p = loadFileInDisk( bm, BIN_DIR + "employee.bin", 0);
    if ( !p.has_value() )
    {
        std::cerr << "Error loading employee.bin" << std::endl;
        return 1;
    }
    auto [employeeStartAddress, employeeEndAddress] = p.value();

    p = loadFileInDisk( bm, BIN_DIR + "company.bin", employeeEndAddress );
    if ( !p.has_value() )
    {
        std::cerr << "Error loading company.bin" << std::endl;
        return 1;
    }
    auto [companyStartAddress, companyEndAddress] = p.value();

    BPlusTreeIndex< unsigned long long, address_id_t > emp_index(&bm, 10, companyEndAddress);
    for ( int i = 0; i < 1000; ++i )
    {
        address_id_t addr = employeeStartAddress + i * sizeof(Employee);
        Employee emp = extractData<Employee>(bm.readAddress(addr, sizeof(Employee)));
        emp_index.insert( emp.company_id * 1e5 + emp.id, addr );
    }
    auto [empStartIndex, empEndIndex] = emp_index.getAddressRange();

    BPlusTreeIndex< unsigned long long, address_id_t > comp_index(&bm, 10, empEndIndex);
    for( int i = 0; i < 200; ++i )
    {
        address_id_t addr = companyStartAddress + i * sizeof(Company);
        Company comp = extractData<Company>(bm.readAddress(addr, sizeof(Company)));
        comp_index.insert( comp.id, addr );
    }
    auto [compStartIndex, compEndIndex] = comp_index.getAddressRange();

    std::cout << "========================================================" << std::endl;
    std::cout << "Statistics of the creation of Index (Before Join)" << std::endl;
    std::cout << "\t\tDisk IO operations: " << bm.getNumIO() << std::endl;
    std::cout << "\t\tDisk IO cost: " << bm.getCostIO() << std::endl;
    std::cout << "\t\t(FrameSize: " << blockSize << ", FrameCount: " << frameCount << ")" << std::endl;
    std::cout << "\t\t(ReplacementStrategy: " << (replaceStrategy == LRU ? "LRU" : "MRU") << ", DiskAccessStrategy: " << (accessType == RANDOM ? "RANDOM" : "SEQUENTIAL") << ")" << std::endl;

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

    // Store the result in a file
    storeResult<JoinEmployeeCompany>(bm, compEndIndex, joinAddr, RES_DIR + "bplus_index_joined_data.csv");

    std::cout << "\nStatistics of the Join using Index operation" << std::endl;
    std::cout << "\t\tDisk IO operations: " << bm.getNumIO() << std::endl;
    std::cout << "\t\tDisk IO cost: " << bm.getCostIO() << std::endl;
    std::cout << "\t\t(FrameSize: " << blockSize << ", FrameCount: " << frameCount << ")" << std::endl;
    std::cout << "\t\t(ReplacementStrategy: " << (replaceStrategy == LRU ? "LRU" : "MRU") << ", DiskAccessStrategy: " << (accessType == RANDOM ? "RANDOM" : "SEQUENTIAL") << ")" << std::endl;

    return 0;
}

int main()
{
    help( 4096, 1024, LRU, RANDOM );
    help( 4096, 5, LRU, RANDOM );
    help( 4096, 1024, MRU, RANDOM );
    help( 4096, 5, MRU, RANDOM );
    return 0;
}