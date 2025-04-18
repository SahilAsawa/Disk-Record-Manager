#pragma once

	#ifndef _UTILS_HPP_
	#define _UTILS_HPP_

	#include <array>
	#include <string>
	#include <algorithm>
	#include <optional>
	#include <vector>
	#include <string>
	#include <sstream>

using frame_id_t = unsigned long long;
using page_id_t = unsigned long long;
using block_id_t = unsigned long long;
using address_id_t = unsigned long long;
using node_id_t = long long;
using bucket_id_t = unsigned long long;
using storage_t = unsigned long long;

#define GB * 1024ll * 1024 * 1024
#define MB * 1024ll * 1024
#define KB * 1024ll
#define B * 1ll

#define EMP_SIZE 5000
#define COMP_SIZE 1000

#define BLOCK_SIZE (4 KB)
#define DISK_SIZE (4 MB)
#define BUFFER_SIZE (64 KB)

const std::string BIN_DIR = "./bin/";
const std::string CSV_DIR = "./files_large/";
const std::string RES_DIR = "./Results/";
const std::string STAT_DIR = "./Statistics/";

class Disk;
class BufferManager;
template<typename KeyType, typename ValueType>
class BPlusTreeIndex;

struct Employee
{
	int id;
	int company_id;
	int salary;
	std::array<char, 58> fname;
	std::array<char, 58> lname;
	static constexpr size_t size = 128;

	friend auto operator==(const Employee &lhs, const Employee &rhs)
		-> bool
	{
		return lhs.company_id == rhs.company_id;
	}

	friend auto operator<=>(const Employee &lhs, const Employee &rhs)
		-> std::strong_ordering
	{
		return lhs.company_id <=> rhs.company_id;
	}
	
	static std::string getTitle() {
		return "id;company_id;salary;fname;lname";
	}

	std::string toString() const {
		std::stringstream ss;
		ss << id << ";" << company_id << ";" << salary << ";"
		   << fname.data() << ";" << lname.data();
		return ss.str();
	}
};

struct Company
{
	int id;
	std::array<char, 62> name;
	std::array<char, 62> slogan;
	static constexpr size_t size = 128;

	friend auto operator==(const Company &lhs, const Company &rhs) -> bool
	{
		return lhs.id == rhs.id;
	}

	friend auto operator<=>(const Company &lhs, const Company &rhs)
		-> std::strong_ordering
	{
		return lhs.id <=> rhs.id;
	}

	static std::string getTitle() {
		return "id;name;slogan";
	}

	std::string toString() const {
		std::stringstream ss;
		ss << id << ";" << name.data() << ";" << slogan.data();
		return ss.str();
	}
};

struct JoinEmployeeCompany
{
	int employee_id;
	int company_id;
	int salary;
	std::array<char, 58> fname{};
	std::array<char, 58> lname{};
	std::array<char, 64> name = {};   // '\0' initialized
	std::array<char, 64> slogan = {}; // '\0' initialized
	static constexpr size_t size = 256;

	JoinEmployeeCompany() = default;

	JoinEmployeeCompany(const Employee &employee, const Company &company)
	{
		employee_id = employee.id;
		company_id = company.id;
		salary = employee.salary;
		fname = employee.fname;
		lname = employee.lname;
		std::copy_n(company.name.begin(), 62, name.begin());
		std::copy_n(company.slogan.begin(), 62, slogan.begin());
	}

	static std::string getTitle() {
		return "employee_id;company_id;salary;fname;lname;name;slogan";
	}

	std::string toString() const {
        std::stringstream ss;
		ss << employee_id << ";" << company_id << ";" << salary << ";"
		   << fname.data() << ";" << lname.data() << ";"
		   << name.data() << ";" << slogan.data();
        return ss.str();
    }
};

struct Stats
{
	long long numIO = 0;
	long long numDiskAccess = 0;
	long long costDiskAccess = 0;

	//overload + operator
	friend auto operator+(const Stats &lhs, const Stats &rhs) -> Stats
	{
		return {lhs.numIO + rhs.numIO, lhs.numDiskAccess + rhs.numDiskAccess, lhs.costDiskAccess + rhs.costDiskAccess};
	}
	//overload += operator
	friend auto operator+=(Stats &lhs, const Stats &rhs) -> Stats &
	{
		lhs.numIO += rhs.numIO;
		lhs.numDiskAccess += rhs.numDiskAccess;
		lhs.costDiskAccess += rhs.costDiskAccess;
		return lhs;
	}
	//overload - operator
	friend auto operator-(const Stats &lhs, const Stats &rhs) -> Stats
	{
		return {lhs.numIO - rhs.numIO, lhs.numDiskAccess - rhs.numDiskAccess, lhs.costDiskAccess - rhs.costDiskAccess};
	}
	//overload -= operator
	friend auto operator-=(Stats &lhs, const Stats &rhs) -> Stats &
	{
		lhs.numIO -= rhs.numIO;
		lhs.numDiskAccess -= rhs.numDiskAccess;
		lhs.costDiskAccess -= rhs.costDiskAccess;
		return lhs;
	}
};

static constexpr auto EmployeeSize = sizeof(Employee);
static constexpr auto CompanySize = sizeof(Company);
static constexpr auto JoinedSize = sizeof(JoinEmployeeCompany);

/**
 * @brief Loads a file into the disk in 128-byte chunks.
 * @param buffer Reference to the BufferManager that handles writing data to disk.
 * @param fileName Name (or path) of the file to be loaded.
 * @param startingAddress The address in the buffer where writing should begin.
 * @return On success, returns a pair of (startingAddress, endAddress) indicating the range written.
 *         Returns std::nullopt if the file cannot be opened or an error occurs.
 */
auto loadFileInDisk (BufferManager& buffer, std::string fileName, address_id_t startingAddress) -> std::optional<std::pair<address_id_t,address_id_t>>;

/**
 * @brief Calculates the starting address of the next free frame given the current Address and the block size.
 * @param readBytes The total number of bytes that have been used so far.
 * @param BLOCK_SIZE The size of each block/frame.
 * @return The starting address (offset) of the next free frame.
 */
auto getNextFreeFrame(int readBytes, block_id_t blockSize = (4 KB)) -> int;

/**
 * @brief Converts a byte vector to an object of type T via direct memory copy.
 * @tparam T Type of the object to extract (e.g., `Employee`, `Company`).
 * @param data Binary data to reinterpret as type `T`.
 * @return T Deserialized object populated from `data`.
 */
template <typename T>
T extractData(const std::vector<std::byte> &data);

/**
 * @brief Loads "employee.bin" and "company.bin" files into disk storage via a buffer manager.
 * @param BLOCK_SIZE Size of each disk block (in bytes).
 * @param DISK_SIZE Total capacity of the disk (in bytes).
 * @param BUFFER_SIZE Maximum buffer cache size (in bytes).
 * @return Contains four addresses in order:
 *         - Start of employee data
 *         - End of employee data
 *         - Start of company data
 *         - End of company data
 */
auto loadData(block_id_t blockSize = (4 KB), storage_t diskSize = (4 MB), storage_t bufferSize = (64 KB)) -> std::tuple<address_id_t, address_id_t, address_id_t, address_id_t>;

/**
 * @brief Serializes data from the disk to a text file, converting binary data to objects of type `T`.
 * @tparam T Type of object to extract (requires `T::size` and `T::toString()` methods).
 * @param buffer BufferManager handling disk reads.
 * @param start Starting address to read from (inclusive).
 * @param end Ending address to read until (exclusive).
 * @param fileName Output file name/path for text data.
 */
template <typename T>
auto storeResult(BufferManager &buffer, address_id_t start, address_id_t end, std::string fileName) -> void;

#endif // _UTILS_HPP_