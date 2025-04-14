#include <Utilities/Utils.hpp>
#include <Storage/BufferManager.hpp>
#include <Storage/Disk.hpp>
#include <Indexes/BPlusTreeIndex.hpp>

#include <iostream>
#include <vector>
#include <optional>
#include <string>

std::string to_string(const std::array<char, 58>& arr) {
    for(size_t i = 0; i < arr.size(); ++i) {
        if (arr[i] == '\0') {
            return std::string(arr.data(), i);
        }
    }
    return std::string(arr.data(), arr.size());
}

template <typename T>
auto storeResult(BufferManager &buffer, int start, int end, std::string fileName) -> void
{
    T sortedData;
    auto size = sortedData.size;
    std::ofstream file(fileName);
    if (!file.is_open())
    {
        std::cerr << "Error opening file" << '\n';
        return;
    }
    for (int i = start; i < end; i += size)
    {
        auto data = buffer.readAddress(i, size);
        sortedData = extractData<T>(data);
        file << sortedData.toString() << std::endl;
    }
    file.close();
    return;
}

int main() {
    Disk disk( RANDOM, 256, 1024 * 1024 );
    BufferManager bm ( &disk, MRU, 16 );

    auto p = loadFileInDisk( bm, "files/employee.bin", 0);
    if ( !p.has_value() ) {
        std::cerr << "Error loading employee.bin" << std::endl;
        return 1;
    }
    auto [employeeStartAddress, employeeEndAddress] = p.value();

    p = loadFileInDisk( bm, "files/company.bin", employeeEndAddress );
    if ( !p.has_value() ) {
        std::cerr << "Error loading company.bin" << std::endl;
        return 1;
    }
    auto [companyStartAddress, companyEndAddress] = p.value();

    // std::cout << "Employee Start Address: " << employeeStartAddress << std::endl;
    // std::cout << "Employee End Address: " << employeeEndAddress << std::endl;
    // std::cout << "Company Start Address: " << companyStartAddress << std::endl;
    // std::cout << "Company End Address: " << companyEndAddress << std::endl;

    BPlusTreeIndex< unsigned long long, address_id_t > emp_index(&bm, 10, companyEndAddress);

    for ( int i = 0; i < 1000; ++i ) {
        address_id_t addr = employeeStartAddress + i * sizeof(Employee);
        Employee emp = extractData<Employee>(bm.readAddress(addr, sizeof(Employee)));
        emp_index.insert( emp.company_id * 1e5 + emp.id, addr );
    }
    // std::cout << emp_index << std::endl;
    auto [empStartIndex, empEndIndex] = emp_index.getAddressRange();

    BPlusTreeIndex< unsigned long long, address_id_t > comp_index(&bm, 10, empEndIndex);

    for( int i = 0; i < 200; ++i) {
        address_id_t addr = companyStartAddress + i * sizeof(Company);
        Company comp = extractData<Company>(bm.readAddress(addr, sizeof(Company)));
        comp_index.insert( comp.id, addr );
    }
    // std::cout << comp_index << std::endl;
    auto [compStartIndex, compEndIndex] = comp_index.getAddressRange();

    std::cout << "\n\t========================================================\n" << std::endl;
    std::cout << "\t\tDisk IO operations: " << bm.getNumIO() << std::endl;
    std::cout << "\t\tDisk IO cost: " << bm.getCostIO() << std::endl;
    std::cout << "\t\tBuffer Manager Size: " << bm.getNumFrames() << std::endl;
    std::cout << "\t\tBuffer Manager Replacement Strategy: " << (bm.getReplaceStrategy() == LRU ? "LRU" : "MRU") << std::endl;

    // Join the two indexes
    auto empRange = emp_index.rangeSearch( 0, 1e18 );
    // for( auto& [key, value] : empRange ) {
    //     std::cout << "Company ID: " << key / (int) 1e5 << ", Employee ID: " << key % (int) 1e5 << ", Address: " << value << std::endl;
    // }

    auto compRange = comp_index.rangeSearch( 0, 1e18 );
    // for( auto& [key, value] : compRange ) {
    //     std::cout << "Company ID: " << key << ", Address: " << value << std::endl;
    // }

    size_t i = 0, j = 0;
    address_id_t joinAddr = compEndIndex;
    while( i < empRange.size() && j < compRange.size() ) {
        auto& [empKey, empValue] = empRange[i];
        auto& [compKey, compValue] = compRange[j];

        if( empKey / (int) 1e5 == compKey ) {
            Employee emp = extractData<Employee>(bm.readAddress(empValue, sizeof(Employee)));
            Company comp = extractData<Company>(bm.readAddress(compValue, sizeof(Company)));
            JoinEmployeeCompany joinData(emp, comp);
            bm.writeAddress(joinAddr, std::vector<std::byte>(reinterpret_cast<std::byte*>(&joinData), reinterpret_cast<std::byte*>(&joinData) + sizeof(JoinEmployeeCompany)));
            joinAddr += sizeof(JoinEmployeeCompany);
            ++i;
        } else if( empKey / (int) 1e5 < compKey ) {
            ++i;
        } else {
            ++j;
        }
    }
    // Store the result in a file
    storeResult<JoinEmployeeCompany>(bm, compEndIndex, joinAddr, "files/joined_data.csv");

    std::cout << "\n\t========================================================\n" << std::endl;
    std::cout << "\t\tDisk IO operations: " << bm.getNumIO() << std::endl;
    std::cout << "\t\tDisk IO cost: " << bm.getCostIO() << std::endl;
    std::cout << "\t\tBuffer Manager Size: " << bm.getNumFrames() << std::endl;
    std::cout << "\t\tBuffer Manager Replacement Strategy: " << (bm.getReplaceStrategy() == LRU ? "LRU" : "MRU") << std::endl;

    return 0;
}