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
auto mergeRuns(BufferManager &buffer, const std::vector<std::pair<int, int>> &Runs, int NextUsableAddress) -> std::pair<int, int>
{
    const bool isEmployee = std::is_same_v<T, Employee>;
    const auto size = (isEmployee) ? EmployeeSize : CompanySize;
    int baseAddress = NextUsableAddress;
    size_t runCount = Runs.size();
    std::priority_queue<std::tuple<T, size_t, int>, std::vector<std::tuple<T, size_t, int>>, std::greater<std::tuple<T, size_t, int>>> minHeap;
    for (size_t i = 0; i < runCount; ++i)
    {
        minHeap.push({extractData<T>(buffer.readAddress(Runs[i].first, size)), i, Runs[i].first});
    }
    while (!minHeap.empty())
    {
        auto [data, runIndex, address] = minHeap.top();
        minHeap.pop();
        buffer.writeAddress(baseAddress, std::vector<std::byte>(reinterpret_cast<std::byte *>(&data), reinterpret_cast<std::byte *>(&data) + size));
        baseAddress += size;
        if (address + size < Runs[runIndex].second)
        {
            auto nextData = extractData<T>(buffer.readAddress(address + size, size));
            minHeap.push({nextData, runIndex, address + size});
        }
    }
    return std::make_pair(NextUsableAddress, baseAddress);
}

template <typename T>
auto externalSort(BufferManager &buffer, int StartAddress, int EndAddress, int NextUsableAddress) -> std::pair<int, int> // Returns the Start and the End index of the final Sorted data
{
    const bool isEmployee = std::is_same_v<T, Employee>;
    const auto size = (isEmployee) ? EmployeeSize : CompanySize;
    const int dataSize = EndAddress - StartAddress;
    // Sorting the Data and storing it in the same Blocks in Disk
    std::vector<T> dataInBlock;
    std::vector<std::pair<int, int>> Runs;
    for (int i = StartAddress; i < EndAddress; i += BLOCK_SIZE)
    {
        Runs.push_back({i, std::min(EndAddress, i + BLOCK_SIZE)});
        auto data = buffer.readAddress(i, std::min(EndAddress, i + BLOCK_SIZE));
        dataInBlock.insert(dataInBlock.end(), reinterpret_cast<T *>(data.data()), reinterpret_cast<T *>(data.data() + std::min(BLOCK_SIZE, EndAddress - i)));
        std::ranges::sort(dataInBlock.begin(), dataInBlock.end()); // Merge Sort each Block
        buffer.writeAddress(i, std::vector<std::byte>(reinterpret_cast<std::byte *>(dataInBlock.data()), reinterpret_cast<std::byte *>(dataInBlock.data() + dataInBlock.size())));
        dataInBlock.clear();
    }

    // Merging the Sorted Blocks
    int usedFrameCnt = (EndAddress - StartAddress + BLOCK_SIZE - 1) / BLOCK_SIZE;
    std::pair<int, int> result;
    if (usedFrameCnt < BLOCK_COUNT_BUFFER - 1)
    {
        //  Merged in a single Run
        result = mergeRuns<T>(buffer, Runs, NextUsableAddress);
    }
    else
    {
        std::vector<std::pair<int, int>> nextRuns;
        bool toUse = false; // if false use NextUsableAddress or use StartAddress replacably
        while (Runs.size() > BLOCK_COUNT_BUFFER - 1)
        {
            int toFillIndex = (toUse) ? StartAddress : NextUsableAddress;
            for (size_t i = 0; i < Runs.size(); i += (BLOCK_COUNT_BUFFER - 1))
            {
                std::vector<std::pair<int, int>> tempRuns;
                for (size_t j = i; j < i + (BLOCK_COUNT_BUFFER - 1) && j < Runs.size(); ++j)
                {
                    tempRuns.push_back(Runs[j]);
                }
                auto mergedRun = mergeRuns<T>(buffer, tempRuns, toFillIndex);
                nextRuns.push_back(mergedRun);
                toFillIndex = mergedRun.second;
            }
            Runs = nextRuns;
            nextRuns.clear();
            toUse = !toUse;
        }
        int toFillIndex = (toUse) ? StartAddress : NextUsableAddress;
        result = mergeRuns<T>(buffer, Runs, toFillIndex);
    }
    if (result.first == NextUsableAddress)
    {
        int readAddr = NextUsableAddress;
        int writeAddr = StartAddress;
        for (int i = readAddr; i < result.second; i += size)
        {
            auto data = buffer.readAddress(i, size);
            buffer.writeAddress(writeAddr, data);
            readAddr += size;
            writeAddr += size;
        }
        result.first = StartAddress;
        result.second = StartAddress + dataSize;
    }
    return result;
}

auto mergeJoin(BufferManager &buffer, int startEmployee, int endEmployee, int startCompany, int endCompany, int NextUsableAddress) -> std::pair<int, int>
{
    // Pinning and Not based implementation
    int baseAddress = NextUsableAddress;
    int employeePtr = startEmployee;
    int companyPtr = startCompany;
    while (employeePtr < endEmployee && companyPtr < endCompany)
    {
        auto employeeData = extractData<Employee>(buffer.readAddress(employeePtr, EmployeeSize));
        auto companyData = extractData<Company>(buffer.readAddress(companyPtr, CompanySize));
        if (employeeData.company_id == companyData.id)
        {
            JoinEmployeeCompany joinData(employeeData, companyData);
            buffer.writeAddress(baseAddress, std::vector<std::byte>(reinterpret_cast<std::byte *>(&joinData), reinterpret_cast<std::byte *>(&joinData) + JoinedSize));
            baseAddress += JoinedSize;
            employeePtr += EmployeeSize;
        }
        else if (employeeData.company_id < companyData.id)
        {
            employeePtr += EmployeeSize;
        }
        else
        {
            companyPtr += CompanySize;
        }
    }
    return std::make_pair(NextUsableAddress, baseAddress);
}

template <typename T>
auto storeResult(BufferManager &buffer, int start, int end, std::string fileName) -> void
{
    T storeData;
    auto size = T::size;
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

auto testing(bool DiskAccessStrategy, int BufferReplacementStategy) -> void
{
    auto [StartAddressEmployee, EndAddressEmployee, StartAddressCompany, EndAddressCompany] = loadData();
    Disk disk(DiskAccessStrategy, BLOCK_SIZE, BLOCK_COUNT_DISK);
    BufferManager buffer(&disk, BufferReplacementStategy, BLOCK_COUNT_BUFFER);

    // External Sort the Employee and Company data
    address_id_t NextUsableAddress = getNextFreeFrame(EndAddressCompany);
    auto [startEmployeeSorted, endEmployeeSorted] = externalSort<Employee>(buffer, StartAddressEmployee, EndAddressEmployee, NextUsableAddress);
    auto [startCompanySorted, endCompanySorted] = externalSort<Company>(buffer, StartAddressCompany, EndAddressCompany, NextUsableAddress);

    std::cout << "\nStatistics of the External Sort" << std::endl;
    std::cout << "\t========================================================" << std::endl;
    std::cout << "\t\tDisk IO operations: " << buffer.getNumIO() << std::endl;
    std::cout << "\t\tDisk IO cost: " << buffer.getCostIO() << std::endl;
    std::cout << "\t\tBuffer Manager Size: " << buffer.getNumFrames() << std::endl;
    std::cout << "\t\tBuffer Manager Replacement Strategy: " << (buffer.getReplaceStrategy() == LRU ? "LRU" : "MRU") << std::endl;
    std::cout << "\t\tDisk Access Strategy: " << (DiskAccessStrategy == RANDOM ? "RANDOM" : "SEQUENTIAL") << std::endl;
    
    // Merge Join the Employee and Company data
    auto [startJoin, endJoin] = mergeJoin(buffer, startEmployeeSorted, endEmployeeSorted, startCompanySorted, endCompanySorted, NextUsableAddress);

    // print statistics
    std::cout << "\nStatistics of the Merge Join (including sorting)" << std::endl;
    std::cout << "\t========================================================" << std::endl;
    std::cout << "\t\tDisk IO operations: " << buffer.getNumIO() << std::endl;
    std::cout << "\t\tDisk IO cost: " << buffer.getCostIO() << std::endl;
    std::cout << "\t\tBuffer Manager Size: " << buffer.getNumFrames() << std::endl;
    std::cout << "\t\tBuffer Manager Replacement Strategy: " << (buffer.getReplaceStrategy() == LRU ? "LRU" : "MRU") << std::endl;
    std::cout << "\t\tDisk Access Strategy: " << (DiskAccessStrategy == RANDOM ? "RANDOM" : "SEQUENTIAL") << std::endl;
    
    // Storing the sorted files for Demonstration
    storeResult<Employee>(buffer, startEmployeeSorted, endEmployeeSorted, "./MergeSort/sorted_employee.csv");
    storeResult<Company>(buffer, startCompanySorted, endCompanySorted, "./MergeSort/sorted_company.csv");
    storeResult<JoinEmployeeCompany>(buffer, startJoin, endJoin, "./MergeSort/joined_result.csv");

    return;
}

int main()
{
    struct stat st;
    if(stat("MergeSort", &st) == -1) 
    { 
        if(mkdir("MergeSort", 0755) != 0) perror("mkdir failed");
    }
    else if(S_ISDIR(st.st_mode));
    testing(RANDOM, LRU);
    testing(RANDOM, MRU);
    testing(SEQUENTIAL, LRU);
    testing(SEQUENTIAL, MRU);
    std::cout << "\t========================================================\n" << std::endl;
    return 0;
}