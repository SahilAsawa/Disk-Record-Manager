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

template <typename T>
T extractData(const std::vector<std::byte> &data)
{
    T result;
    std::memcpy(&result, data.data(), sizeof(T));
    return result;
}

template Employee extractData<Employee>(const std::vector<std::byte> &data);
template Company extractData<Company>(const std::vector<std::byte> &data);
template JoinEmployeeCompany extractData<JoinEmployeeCompany>(const std::vector<std::byte> &data);