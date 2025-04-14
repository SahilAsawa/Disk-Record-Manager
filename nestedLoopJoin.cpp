#include <fstream>
#include <iostream>
#include <ranges>
#include <vector>
#include <algorithm>
#include <queue>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <sys/stat.h> 
#include <unistd.h>
#include <cstdio>
#include <Storage/Disk.hpp>
#include <Storage/BufferManager.hpp>
#include <Utilities/Utils.hpp>

#define EMPLOYEE 0
#define COMPANY 1

int BLOCK_SIZE = 4096;
int BLOCK_COUNT_DISK = 1024 * 1024;
int BLOCK_COUNT_BUFFER = 16;

auto getNextFreeFrame(int readBytes) -> int
{
    int usedFrameCnt = (readBytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
    return usedFrameCnt * BLOCK_SIZE;
}

template <typename T>
T extractData(const std::vector<std::byte> &data)
{
    T result;
    std::memcpy(&result, data.data(), sizeof(T));
    return result;
}

template <typename T>
auto storeResult(BufferManager &buffer, int start, int end, std::string fileName) -> void
{
    T storeData;
    auto size = storeData.size;
    std::ofstream file(fileName);
    if (!file.is_open())
    {
        std::cerr << "Error opening file" << '\n';
        return;
    }
    for (int i = start; i < end; i += size)
    {
        auto data = buffer.readAddress(i, size);
        storeData = extractData<T>(data);
        file << storeData.toString() << std::endl;
    }
    file.close();
    return;
}

auto loadData() -> std::tuple<address_id_t, address_id_t, address_id_t, address_id_t>
{
    Disk disk(RANDOM, BLOCK_SIZE, BLOCK_COUNT_DISK);
    BufferManager buffer(&disk, MRU, BLOCK_COUNT_BUFFER);

    auto locationEmployee = loadFileInDisk(buffer, "./bin/employee.bin", 0);
    if (!locationEmployee.has_value())
    {
        std::cerr << "Error loading Employee data" << std::endl;
        exit(1);
    }
    auto [StartAddressEmployee, EndAddressEmployee] = locationEmployee.value();

    auto locationCompany = loadFileInDisk(buffer, "./bin/company.bin", EndAddressEmployee);
    if (!locationCompany.has_value())
    {
        std::cerr << "Error loading Company data" << std::endl;
        exit(1);
    }
    auto [StartAddressCompany, EndAddressCompany] = locationCompany.value();
    return {StartAddressEmployee, EndAddressEmployee, StartAddressCompany, EndAddressCompany};
}

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
    Disk disk(DiskAccessStrategy, BLOCK_SIZE, BLOCK_COUNT_DISK);
    BufferManager buffer(&disk, BufferReplacementStategy, BLOCK_COUNT_BUFFER);

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
    storeResult<JoinEmployeeCompany>(buffer, StartJoin, EndJoin, "./NestedJoin/joined_result.csv");

    return;
}

int main()
{
    struct stat st;
    if(stat("NestedJoin", &st) == -1) 
    { 
        if(mkdir("NestedJoin", 0755) != 0) perror("mkdir failed");
    }
    else if(S_ISDIR(st.st_mode));
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