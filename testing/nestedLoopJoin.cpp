#include <fstream>
#include <iostream>
#include <ranges>
#include <vector>
#include <algorithm>
#include <queue>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <Storage/Disk.hpp>
#include <Storage/BufferManager.hpp>
#include <Utilities/Utils.hpp>

#define EMPLOYEE 0
#define COMPANY 1

block_id_t BLOCK_SIZE = (4 KB);
storage_t DISK_SIZE = (4 MB);
storage_t BUFFER_SIZE = (64 KB);

auto join(BufferManager &buffer, address_id_t StartAddressEmployee, address_id_t EndAddressEmployee, address_id_t StartAddressCompany, address_id_t EndAddressCompany, address_id_t NextUsableAddress, bool Outer) -> std::pair<address_id_t, address_id_t>
{
    address_id_t baseAddress = NextUsableAddress;

    if (Outer)
    {
        address_id_t employeePtr = StartAddressEmployee;
        while ( employeePtr < EndAddressEmployee )
        {
            auto employeeData = extractData<Employee>(buffer.readAddress(employeePtr, EmployeeSize));
            address_id_t companyPtr = StartAddressCompany;
            while ( companyPtr < EndAddressCompany )
            {
                auto companyData = extractData<Company>(buffer.readAddress(companyPtr, CompanySize));
                if ( employeeData.company_id == companyData.id )
                {
                    JoinEmployeeCompany joinData(employeeData, companyData);
                    buffer.writeAddress(baseAddress, std::vector<std::byte>(reinterpret_cast<std::byte *>(&joinData), reinterpret_cast<std::byte *>(&joinData) + JoinEmployeeCompany::size));
                    baseAddress += JoinEmployeeCompany::size;
                }
                companyPtr += CompanySize;
            }
            employeePtr += EmployeeSize;
        }
    }
    else 
    {
        address_id_t companyPtr = StartAddressCompany;
        while ( companyPtr < EndAddressCompany )
        {
            auto companyData = extractData<Company>(buffer.readAddress(companyPtr, CompanySize));
            address_id_t employeePtr = StartAddressEmployee;
            while ( employeePtr < EndAddressEmployee )
            {
                auto employeeData = extractData<Employee>(buffer.readAddress(employeePtr, EmployeeSize));
                if ( employeeData.company_id == companyData.id )
                {
                    JoinEmployeeCompany joinData(employeeData, companyData);
                    buffer.writeAddress(baseAddress, std::vector<std::byte>(reinterpret_cast<std::byte *>(&joinData), reinterpret_cast<std::byte *>(&joinData) + JoinEmployeeCompany::size));
                    baseAddress += JoinEmployeeCompany::size;
                }
                employeePtr += EmployeeSize;
            }
            companyPtr += CompanySize;
        }
    }

    return std::make_pair(NextUsableAddress, baseAddress);
}

auto testing(bool DiskAccessStrategy, int BufferReplacementStategy, bool Outer) -> void
{
    auto [StartAddressEmployee, EndAddressEmployee, StartAddressCompany, EndAddressCompany] = loadData();
    Disk disk(DiskAccessStrategy, BLOCK_SIZE, DISK_SIZE);
    BufferManager buffer(&disk, BufferReplacementStategy, BUFFER_SIZE);

    address_id_t NextUsableAddress = getNextFreeFrame(EndAddressCompany);
    auto [StartJoin, EndJoin] = join(buffer, StartAddressEmployee, EndAddressEmployee, StartAddressCompany, EndAddressCompany, NextUsableAddress, Outer);
    
    // print statistics
    std::cout << "\nStatistics for Nested Join with " << (Outer == EMPLOYEE ? "Employee" : "Company") << " as outer relation" << std::endl;
    std::cout << "\t========================================================" << std::endl;
    std::cout << "\t\tDisk IO operations: " << buffer.getNumIO() << std::endl;
    std::cout << "\t\tDisk IO cost: " << buffer.getCostIO() << std::endl;
    std::cout << "\t\tBuffer Manager Size: " << buffer.getNumFrames() << std::endl;
    std::cout << "\t\tBuffer Manager Replacement Strategy: " << (buffer.getReplaceStrategy() == LRU ? "LRU" : "MRU") << std::endl;
    std::cout << "\t\tDisk Access Strategy: " << (DiskAccessStrategy == RANDOM ? "RANDOM" : "SEQUENTIAL") << std::endl;

    // store the result
    storeResult<JoinEmployeeCompany>(buffer, StartJoin, EndJoin, RES_DIR + "nest_join_joined_result.csv");

    return;
}

int main()
{
    testing(RANDOM, LRU, EMPLOYEE);
    testing(RANDOM, MRU, EMPLOYEE);
    testing(SEQUENTIAL, LRU, EMPLOYEE);
    testing(SEQUENTIAL, MRU, EMPLOYEE);
    testing(RANDOM, LRU, COMPANY);
    testing(RANDOM, MRU, COMPANY);
    testing(SEQUENTIAL, LRU, COMPANY);
    testing(SEQUENTIAL, MRU, COMPANY);
    std::cout << "\t========================================================" << std::endl;
    return 0;
}