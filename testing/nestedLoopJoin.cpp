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

address_id_t StartAddressEmployee = 0;
address_id_t EndAddressEmployee = 0;
address_id_t StartAddressCompany = 0;
address_id_t EndAddressCompany = 0;

std::ofstream outFile(STAT_DIR + "nested_join_stats.txt", std::ios::out | std::ios::trunc);

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
    Disk disk(DiskAccessStrategy, BLOCK_SIZE, DISK_SIZE);
    BufferManager buffer(&disk, BufferReplacementStategy, BUFFER_SIZE);

    auto stat = buffer.getStats();

    address_id_t NextUsableAddress = getNextFreeFrame(EndAddressCompany);
    auto [StartJoin, EndJoin] = join(buffer, StartAddressEmployee, EndAddressEmployee, StartAddressCompany, EndAddressCompany, NextUsableAddress, Outer);
    
    // print statistics
    std::string s = "Statistics for Nested Join with ";
    s += (Outer == EMPLOYEE ? "Employee" : "Company");
    s += " as outer relation";
    buffer.printStats(outFile, stat, s);


    // store the result
    storeResult<JoinEmployeeCompany>(buffer, StartJoin, EndJoin, RES_DIR + "nest_join_joined__data_" + (BufferReplacementStategy == LRU ? "lru_" : "mru_") + (DiskAccessStrategy == RANDOM ? "rand_" : "seq_") + (Outer == EMPLOYEE ? "emp" : "comp") + ".csv");

    return;
}

int main()
{
    auto [a,b,c,d] = loadData();
    StartAddressEmployee = a;
    EndAddressEmployee = b;
    StartAddressCompany = c;
    EndAddressCompany = d;

    if(!outFile.is_open())
    {
        std::cerr << "Error opening file for writing statistics." << std::endl;
        return 1;
    }

    testing(RANDOM, LRU, EMPLOYEE);
    testing(RANDOM, MRU, EMPLOYEE);
    testing(SEQUENTIAL, LRU, EMPLOYEE);
    testing(SEQUENTIAL, MRU, EMPLOYEE);
    testing(RANDOM, LRU, COMPANY);
    testing(RANDOM, MRU, COMPANY);
    testing(SEQUENTIAL, LRU, COMPANY);
    testing(SEQUENTIAL, MRU, COMPANY);

    std::cout << "Statistics saved to " << RES_DIR + "nested_join_stats.txt" << std::endl;
    return 0;
}