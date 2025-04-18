#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>
#include <cassert>
#include <Storage/Disk.hpp>
#include <Storage/BufferManager.hpp>
#include <Utilities/Utils.hpp>

block_id_t BLOCK_SIZE = (4 KB);
storage_t DISK_SIZE = (4 MB);
storage_t BUFFER_SIZE = (64 KB);

std::ofstream outFile(STAT_DIR + "external_sort_stats.txt", std::ios::out | std::ios::trunc);

template <typename T>
auto mergeRuns(BufferManager &buffer, const std::vector<std::pair<address_id_t, address_id_t>> &Runs, address_id_t NextUsableAddress) -> std::pair<address_id_t, address_id_t>
{
    const bool isEmployee = std::is_same_v<T, Employee>;
    const auto size = (isEmployee) ? EmployeeSize : CompanySize;
    address_id_t baseAddress = NextUsableAddress;
    size_t runCount = Runs.size();
    std::priority_queue<std::tuple<T, size_t, address_id_t>, std::vector<std::tuple<T, size_t, address_id_t>>, std::greater<std::tuple<T, size_t, address_id_t>>> minHeap;
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
auto externalSort(BufferManager &buffer, address_id_t StartAddress, address_id_t EndAddress, address_id_t NextUsableAddress) -> std::pair<int, int> // Returns the Start and the End index of the final Sorted data
{
    const bool isEmployee = std::is_same_v<T, Employee>;
    const auto size = (isEmployee) ? EmployeeSize : CompanySize;
    const address_id_t dataSize = EndAddress - StartAddress;
    // Sorting the Data and storing it in the same Blocks in Disk
    std::vector<T> dataInBlock;
    std::vector<std::pair<address_id_t, address_id_t>> Runs;
    for (address_id_t i = StartAddress; i < EndAddress; i += BUFFER_SIZE)
    {
        Runs.push_back({i, std::min(EndAddress, i + BUFFER_SIZE)});
        auto data = buffer.readAddress(i, std::min(EndAddress, i + BUFFER_SIZE));
        dataInBlock.insert(dataInBlock.end(), reinterpret_cast<T *>(data.data()), reinterpret_cast<T *>(data.data() + std::min(BUFFER_SIZE, EndAddress - i)));
        std::ranges::sort(dataInBlock.begin(), dataInBlock.end()); // Merge Sort each Block
        buffer.writeAddress(i, std::vector<std::byte>(reinterpret_cast<std::byte *>(dataInBlock.data()), reinterpret_cast<std::byte *>(dataInBlock.data() + dataInBlock.size())));
        dataInBlock.clear();
    }

    // Merging the Sorted Blocks
    size_t usedFrameCnt = (EndAddress - StartAddress + BLOCK_SIZE - 1) / BLOCK_SIZE;
    std::pair<address_id_t, address_id_t> result;
    if (usedFrameCnt < buffer.getNumFrames() - 1)
    {
        //  Merged in a single Run
        result = mergeRuns<T>(buffer, Runs, NextUsableAddress);
    }
    else
    {
        std::vector<std::pair<address_id_t, address_id_t>> nextRuns;
        bool toUse = false; // if false use NextUsableAddress or use StartAddress replacably
        while (Runs.size() > buffer.getNumFrames() - 1)
        {
            address_id_t toFillIndex = (toUse) ? StartAddress : NextUsableAddress;
            for (size_t i = 0; i < Runs.size(); i += (buffer.getNumFrames() - 1))
            {
                std::vector<std::pair<address_id_t, address_id_t>> tempRuns;
                for (size_t j = i; j < i + (buffer.getNumFrames() - 1) && j < Runs.size(); ++j)
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
        address_id_t toFillIndex = (toUse) ? StartAddress : NextUsableAddress;
        result = mergeRuns<T>(buffer, Runs, toFillIndex);
    }
    if (result.first == NextUsableAddress)
    {
        address_id_t readAddr = NextUsableAddress;
        address_id_t writeAddr = StartAddress;
        for (address_id_t i = readAddr; i < result.second; i += size)
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

auto mergeJoin(BufferManager &buffer, address_id_t startEmployee, address_id_t endEmployee, address_id_t startCompany, address_id_t endCompany, address_id_t NextUsableAddress) -> std::pair<address_id_t, address_id_t>
{
    // Pinning and Not based implementation
    address_id_t baseAddress = NextUsableAddress;
    address_id_t employeePtr = startEmployee;
    address_id_t companyPtr = startCompany;
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

auto testing(bool DiskAccessStrategy, int BufferReplacementStategy) -> void
{
    auto [StartAddressEmployee, EndAddressEmployee, StartAddressCompany, EndAddressCompany] = loadData();
    Disk disk(DiskAccessStrategy, BLOCK_SIZE, DISK_SIZE);
    BufferManager buffer(&disk, BufferReplacementStategy, BUFFER_SIZE);

    auto stat = buffer.getStats();

    // External Sort the Employee and Company data
    address_id_t NextUsableAddress = getNextFreeFrame(EndAddressCompany);
    auto [startEmployeeSorted, endEmployeeSorted] = externalSort<Employee>(buffer, StartAddressEmployee, EndAddressEmployee, NextUsableAddress);
    auto [startCompanySorted, endCompanySorted] = externalSort<Company>(buffer, StartAddressCompany, EndAddressCompany, NextUsableAddress);

    buffer.printStats(outFile, stat, "Statistics of the External Sort");
    stat = buffer.getStats();
    
    // Merge Join the Employee and Company data
    auto [startJoin, endJoin] = mergeJoin(buffer, startEmployeeSorted, endEmployeeSorted, startCompanySorted, endCompanySorted, NextUsableAddress);

    // print statistics
    buffer.printStats(outFile, stat, "Statistics of the Merge Join (excluding sorting)"); 
    std::string s = (BufferReplacementStategy == LRU ? "_lru" : "_mru");
    s += (DiskAccessStrategy == RANDOM ? "_rand" : "_seq");
    s += ".csv";

    // Storing the sorted files for Demonstration
    storeResult<Employee>(buffer, startEmployeeSorted, endEmployeeSorted, RES_DIR + "merge_join_sorted_employee" + s);
    storeResult<Company>(buffer, startCompanySorted, endCompanySorted, RES_DIR + "merge_join_sorted_company" + s);
    storeResult<JoinEmployeeCompany>(buffer, startJoin, endJoin, RES_DIR + "merge_join_joined_result" + s);

    return;
}

int main()
{
    if(!outFile.is_open())
    {
        std::cerr << "Error opening file for writing statistics." << std::endl;
        return 1;
    }

    testing(RANDOM, LRU);
    testing(RANDOM, MRU);
    testing(SEQUENTIAL, LRU);
    testing(SEQUENTIAL, MRU);
    std::cout << "Statistics saved to " << STAT_DIR + "external_sort_stats.txt" << std::endl;
    return 0;
}