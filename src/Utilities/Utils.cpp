#include <Utilities/Utils.hpp>
#include <Storage/BufferManager.hpp>
#include <cstring>
#include <iostream>

auto loadFileInDisk (BufferManager& buffer, std::string fileName, address_id_t startingAddress) -> std::optional<std::pair<address_id_t, address_id_t>>
{
	address_id_t endAddress = startingAddress;
	try {
		std::ifstream file { fileName, std::ios::binary };
		std::array<std::byte, 128> ReadBuffer;
		if ( ! file.is_open() )
		{
			std::cerr << "Error opening file" << '\n';
			return std::nullopt;
		}
		while ( file.read( reinterpret_cast<char*> ( ReadBuffer.data() ), ReadBuffer.size() ) )
		{
			buffer.writeAddress( endAddress, std::vector<std::byte>( ReadBuffer.begin(), ReadBuffer.end() ) );
			endAddress += ReadBuffer.size();
		}
		file.close();
		file.clear();
	}
	catch (const std::exception &e ) {
		std::cerr << e.what() << std::endl;
	}
	return std::make_pair(startingAddress, endAddress);
}

auto getNextFreeFrame(int readBytes, block_id_t BLOCK_SIZE) -> int
{
    int usedFrameCnt = (readBytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
    return usedFrameCnt * BLOCK_SIZE;
}

template <typename T>
auto extractData(const std::vector<std::byte> &data) -> T
{
    T result;
    std::memcpy(&result, data.data(), sizeof(T));
    return result;
}

template Employee extractData<Employee>(const std::vector<std::byte> &data);
template Company extractData<Company>(const std::vector<std::byte> &data);
template JoinEmployeeCompany extractData<JoinEmployeeCompany>(const std::vector<std::byte> &data);

auto loadData(block_id_t BLOCK_SIZE, storage_t DISK_SIZE, storage_t BUFFER_SIZE) -> std::tuple<address_id_t, address_id_t, address_id_t, address_id_t>
{
    Disk disk(RANDOM, BLOCK_SIZE, DISK_SIZE);
    BufferManager buffer(&disk, MRU, BUFFER_SIZE);

    auto locationEmployee = loadFileInDisk(buffer, BIN_DIR + "employee.bin", 0);
    if (!locationEmployee.has_value())
    {
        std::cerr << "Error loading Employee data" << std::endl;
        exit(1);
    }
    auto [StartAddressEmployee, EndAddressEmployee] = locationEmployee.value();

    auto locationCompany = loadFileInDisk(buffer, BIN_DIR + "company.bin", EndAddressEmployee);
    if (!locationCompany.has_value())
    {
        std::cerr << "Error loading Company data" << std::endl;
        exit(1);
    }
    auto [StartAddressCompany, EndAddressCompany] = locationCompany.value();
    return {StartAddressEmployee, EndAddressEmployee, StartAddressCompany, EndAddressCompany};
}

template <typename T>
auto storeResult(BufferManager &buffer, address_id_t start, address_id_t end, std::string fileName) -> void
{
    T storeData;
    auto size = T::size;
    std::ofstream file(fileName);
    if (!file.is_open())
    {
        std::cerr << "Error opening file" << '\n';
        return;
    }
    for (address_id_t i = start; i < end; i += size)
    {
        auto data = buffer.readAddress(i, size);
        storeData = extractData<T>(data);
        file << storeData.toString() << std::endl;
    }
    file.close();
    return;
}

template void storeResult<Employee>(BufferManager &buffer, address_id_t start, address_id_t end, std::string fileName);
template void storeResult<Company>(BufferManager &buffer, address_id_t start, address_id_t end, std::string fileName);
template void storeResult<JoinEmployeeCompany>(BufferManager &buffer, address_id_t start, address_id_t end, std::string fileName);