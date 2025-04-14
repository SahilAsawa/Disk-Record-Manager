#include <fstream>
#include <iostream>
#include <ranges>
#include <vector>
#include <algorithm>
#include <queue>
#include <cassert>
#include <iomanip>
#include <Storage/Disk.hpp>
#include <Storage/BufferManager.hpp>
#include <Utilities/Utils.hpp>

int BLOCK_SIZE = 4096;
int BLOCK_COUNT_DISK = 1024;
int BLOCK_COUNT_BUFFER = 16;

auto getNextFreeFrame ( int readBytes ) -> int
{
    int usedFrameCnt = (readBytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
    return usedFrameCnt * BLOCK_SIZE;
} 

template <typename T>
T extractData ( const std::vector<std::byte>& data ) 
{
    T result;
    std::memcpy( &result, data.data(), sizeof(T) );
    return result;
}

template <typename T>
auto mergeRuns (BufferManager& buffer, const std::vector < std::pair < int, int > > &Runs, int NextUsableAddress ) -> std::pair < int, int >
{
    const bool isEmployee = std::is_same_v<T, Employee>;
    const auto size = (isEmployee) ? EmployeeSize : CompanySize;
    int baseAddress = NextUsableAddress;
    size_t runCount = Runs.size();
    std::priority_queue < std::tuple < T, size_t, int >, std::vector < std::tuple < T, size_t, int > >, std::greater < std::tuple < T, size_t, int > > > minHeap;
    for ( size_t i = 0; i < runCount; ++i )
    {
        minHeap.push( { extractData<T>( buffer.readAddress( Runs[i].first, size ) ), i, Runs[i].first } );
    }
    while ( !minHeap.empty() )
    {
        auto [data, runIndex, address] = minHeap.top();
        minHeap.pop();
        buffer.writeAddress( baseAddress, std::vector<std::byte>( reinterpret_cast<std::byte*>( &data ), reinterpret_cast<std::byte*>( &data ) + size ) );
        baseAddress += size;
        if ( address + size < Runs[runIndex].second )
        {
            auto nextData = extractData<T>( buffer.readAddress( address + size, size ) );
            minHeap.push( { nextData, runIndex, address + size } );
        }
    }
    return std::make_pair( NextUsableAddress, baseAddress );
}

template <typename T>
auto externalSort ( BufferManager& buffer, int StartAddress, int EndAddress, int NextUsableAddress ) -> std::pair < int, int > // Returns the Start and the End index of the final Sorted data
{
    const bool isEmployee = std::is_same_v<T, Employee>;
    const auto size = (isEmployee) ? EmployeeSize : CompanySize;
    const int dataSize = EndAddress - StartAddress;
    // Sorting the Data and storing it in the same Blocks in Disk
    std::vector<T> dataInBlock;
    std::vector<std::pair<int,int>> Runs;
    for ( int i = StartAddress; i < EndAddress; i += BLOCK_SIZE )
    {
        // std::cout << "Sorting Block: " << (i>>12) << std::endl;
        // std::cerr << "Sorting Block: " << (i>>12) << std::endl;
        Runs.push_back( { i, std::min( EndAddress, i + BLOCK_SIZE ) } );
        auto data = buffer.readAddress( i, std::min( EndAddress, i + BLOCK_SIZE ) );
        // std::cout << "Data Size: " << data.size() << std::endl;
        dataInBlock.insert( dataInBlock.end(), reinterpret_cast<T*>(data.data()), reinterpret_cast<T*>(data.data() + std::min( BLOCK_SIZE, EndAddress - i ) ) );
        std::ranges::sort( dataInBlock.begin(), dataInBlock.end() ); // Merge Sort each Block
        // Sorted and placed in the same block
        // std::cout << std::vector<std::byte>( reinterpret_cast<std::byte*>(dataInBlock.data()), reinterpret_cast<std::byte*>(dataInBlock.data() + dataInBlock.size() ) ).size() << std::endl;
        buffer.writeAddress( i, std::vector<std::byte>( reinterpret_cast<std::byte*>(dataInBlock.data()), reinterpret_cast<std::byte*>(dataInBlock.data() + dataInBlock.size() ) ) );
        // std::cout << "Sorted Block: " << i << std::endl;
        dataInBlock.clear();
    }

    // Merging the Sorted Blocks
    int usedFrameCnt = (EndAddress - StartAddress + BLOCK_SIZE - 1) / BLOCK_SIZE;
    std::pair < int, int > result;
    if ( usedFrameCnt < BLOCK_COUNT_BUFFER - 1 ) 
    {
        //  Merged in a single Run
        result = mergeRuns<T>(buffer, Runs, NextUsableAddress);
    }
    else 
    {
        std::vector<std::pair<int,int>> nextRuns;
        bool toUse = false; // if false use NextUsableAddress or use StartAddress replacably
        while ( Runs.size() > BLOCK_COUNT_BUFFER - 1 ) 
        {
            int toFillIndex = (toUse) ? StartAddress : NextUsableAddress;
            for (size_t i = 0; i < Runs.size(); i += ( BLOCK_COUNT_BUFFER - 1 ))
            {
                std::vector < std::pair < int, int > > tempRuns;
                for (size_t j = i; j < i + ( BLOCK_COUNT_BUFFER - 1 ) && j < Runs.size(); ++j)
                {
                    tempRuns.push_back( Runs[j] );
                }
                auto mergedRun = mergeRuns<T>(buffer, tempRuns, toFillIndex);
                nextRuns.push_back( mergedRun );
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
        for ( int i = readAddr; i < result.second; i += size )
        {
            auto data = buffer.readAddress( i, size );
            buffer.writeAddress( writeAddr, data );
            readAddr += size;
            writeAddr += size;
        }
        result.first = StartAddress;
        result.second = StartAddress + dataSize;
    }
    return result; // Must be there where the Data was initially stored (for ease of Join implementation)
}

auto mergeJoin ( BufferManager& buffer, int startEmployee, int endEmployee, int startCompany, int endCompany, int NextUsableAddress ) -> std::pair < int, int >
{
    // Pinning and Not based implementation
    int baseAddress = NextUsableAddress;
    int employeePtr = startEmployee;
    int companyPtr = startCompany;
    while ( employeePtr < endEmployee && companyPtr < endCompany )
    {
        auto employeeData = extractData<Employee>( buffer.readAddress( employeePtr, EmployeeSize ) );
        auto companyData = extractData<Company>( buffer.readAddress( companyPtr, CompanySize ) );
        if ( employeeData.company_id == companyData.id )
        {
            JoinEmployeeCompany joinData( employeeData, companyData );
            buffer.writeAddress( baseAddress, std::vector<std::byte>( reinterpret_cast<std::byte*>( &joinData ), reinterpret_cast<std::byte*>( &joinData ) + JoinedSize ) );
            baseAddress += JoinedSize;
            employeePtr += EmployeeSize;
            // companyPtr += CompanySize;
        }
        else if ( employeeData.company_id < companyData.id )
        {
            employeePtr += EmployeeSize;
        }
        else
        {
            companyPtr += CompanySize;
        }
    }
    return std::make_pair( NextUsableAddress, baseAddress );
}

template<typename T>
auto storeSortedResult ( BufferManager& buffer, int start, int end ) -> void
{
    const bool isEmployee = std::is_same_v<T, Employee>;
    const auto size = (isEmployee) ? EmployeeSize : CompanySize;
    std::ofstream file ( (isEmployee) ? "./files/employeeSorted.csv" : "./files/companySorted.csv" );
    if ( ! file.is_open() )
    {
        std::cerr << "Error opening file" << '\n';
        return;
    }
    for ( int i = start; i < end; i += size ) 
    {
        auto data = buffer.readAddress( i, size );
        T sortedData = extractData<T>( data );
        if ( isEmployee )
        {
            file << std::setw(3)  << sortedData.id       << " ;"
                 << std::setw(3)  << sortedData.company_id << " ;"
                 << std::setw(3)  << sortedData.salary     << " ;"
                 << std::setw(30) << sortedData.fname.data()  << " ;"
                 << std::setw(30) << sortedData.lname.data()  << std::endl;
        }
        else
        {
            // file << std::setw(3)  << sortedData.id       << " ;"
            //      << std::setw(30) << sortedData.name.data()   << " ;"
            //      << std::setw(30) << sortedData.slogan.data() << std::endl;
        }
    }
    file.close();
    std::cout << "Sorted Result stored in the file" << std::endl;
    return;
}

auto storeJoinResult ( BufferManager& buffer, int startJoin, int endJoin ) -> void
{
    std::ofstream file ( "./files/joined_result.csv" );
    if ( ! file.is_open() )
    {
        std::cerr << "Error opening file" << '\n';
        return;
    }
    for ( int i = startJoin; i < endJoin; i += JoinedSize ) 
    {
        auto data = buffer.readAddress( i, JoinedSize );
        JoinEmployeeCompany joinData = extractData<JoinEmployeeCompany>( data );
        file << std::setw(3)  << joinData.employee_id   << " ;"
             << std::setw(3)  << joinData.company_id    << " ;"
             << std::setw(3)  << joinData.salary        << " ;"
             << std::setw(30) << joinData.fname.data()  << " ;"
             << std::setw(30) << joinData.lname.data()  << " ;"
             << std::setw(50) << joinData.name.data()   << " ;"
             << std::setw(50) << joinData.slogan.data() << std::endl;
    }
    file.close();
    std::cout << "Join Result stored in the file" << std::endl;
    return;
}



int main () {
    int StartAddressEmployee = 0, StartAddressCompany = 0;
    int EndAddressEmployee = 0, EndAddressCompany = 0;
    int NextUsableAddress = 0;
    {

        Disk disk( RANDOM, BLOCK_SIZE, BLOCK_COUNT_DISK );
        std::cout << "Max Disk Size: " << ( BLOCK_SIZE * BLOCK_COUNT_DISK ) << std::endl;
        BufferManager buffer( &disk, MRU, BLOCK_COUNT_BUFFER );
    
        // Read the CSV Files and stored in the Disk
        try {
            std::ifstream file { "./files/employee.bin", std::ios::binary };
            std::array<std::byte, 128> ReadBuffer;
            if ( ! file.is_open() )
            {
                std::cerr << "Error opening file" << '\n';
                return 1;
            }
            while ( file.read( reinterpret_cast<char*> ( ReadBuffer.data() ), ReadBuffer.size() ) )
            {
                buffer.writeAddress( EndAddressEmployee, std::vector<std::byte>( ReadBuffer.begin(), ReadBuffer.end() ) );
                auto tmpEmployee = extractData<Employee>( buffer.readAddress( EndAddressEmployee, EmployeeSize ) );
                // std::cout << "Employee ID: " << tmpEmployee.id << std::endl;
                EndAddressEmployee += ReadBuffer.size();
            }
            file.close();
            file.clear();
            StartAddressCompany = getNextFreeFrame( EndAddressEmployee );
            EndAddressCompany = StartAddressCompany;
            file.open( "./files/company.bin", std::ios::binary );
            if ( ! file.is_open() )
            {
                std::cerr << "Error opening file" << '\n';
                return 1;
            }
            while ( file.read( reinterpret_cast<char*> ( ReadBuffer.data() ), ReadBuffer.size() ) )
            {
                buffer.writeAddress( EndAddressCompany, std::vector<std::byte>( ReadBuffer.begin(), ReadBuffer.end() ) );
                auto tmpCompany = extractData<Company>( buffer.readAddress( EndAddressCompany, CompanySize ) );
                // std::cout << "Company ID: " << tmpCompany.id << std::endl;
                EndAddressCompany += ReadBuffer.size();
            }
            file.close();
            std::cout << "Employee data written to buffer : " << StartAddressEmployee << std::endl;
            std::cout << "Company data written to buffer : " << StartAddressCompany << std::endl;
        }
        catch (const std::exception &e ) {
            std::cerr << e.what() << std::endl;
        }
    }

    std::cout << "Done reading the files" << std::endl;
    // return 0;

    Disk disk( RANDOM, BLOCK_SIZE, BLOCK_COUNT_DISK );
    std::cout << "Max Disk Size: " << ( BLOCK_SIZE * BLOCK_COUNT_DISK ) << std::endl;
    BufferManager buffer( &disk, MRU, BLOCK_COUNT_BUFFER );

    // External Sort the Employee and Company data
    NextUsableAddress = getNextFreeFrame( EndAddressCompany );
    auto [startEmployeeSorted, endEmployeeSorted] = externalSort<Employee>( buffer, StartAddressEmployee, EndAddressEmployee, NextUsableAddress );
    auto [startCompanySorted, endCompanySorted] = externalSort<Company>( buffer, StartAddressCompany, EndAddressCompany, NextUsableAddress );
    assert ( startEmployeeSorted == StartAddressEmployee );
    assert ( startCompanySorted == StartAddressCompany );
    assert ( endEmployeeSorted == EndAddressEmployee );
    assert ( endCompanySorted == EndAddressCompany );
    std::cout << "Sorted everything" << std::endl;
    storeSortedResult<Employee>( buffer, startEmployeeSorted, endEmployeeSorted );
    // storeSortedResult<Company>( buffer, startCompanySorted, endCompanySorted );
    // Merge Join the Employee and Company data
    auto [startJoin, endJoin] = mergeJoin( buffer, startEmployeeSorted, endEmployeeSorted, startCompanySorted, endCompanySorted, NextUsableAddress );
    storeJoinResult( buffer, startJoin, endJoin );

    // print statistics
    std::cout << "\n\t========================================================\n" << std::endl;
    std::cout << "\t\tDisk IO operations: " << buffer.getNumIO() << std::endl;
    std::cout << "\t\tDisk IO cost: " << buffer.getCostIO() << std::endl;
    std::cout << "\t\tBuffer Manager Size: " << buffer.getNumFrames() << std::endl;
    std::cout << "\t\tBuffer Manager Replacement Strategy: " << (buffer.getReplaceStrategy()==LRU ? "LRU" : "MRU") << std::endl;
    return 0;
}