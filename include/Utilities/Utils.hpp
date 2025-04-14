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
#include <iomanip>

using frame_id_t = unsigned long long;
using page_id_t = unsigned long long;
using block_id_t = unsigned long long;
using address_id_t = unsigned long long;
using node_id_t = long long;

const std::string BIN_DIR = "./bin/";
const std::string CSV_DIR = "./files/";
const std::string RES_DIR = "./Results/";
const std::string STAT_DIR = "./Statistics/";

class Disk;
class BufferManager;

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

	std::string toString() const {
        std::stringstream ss;
		ss << employee_id << ";" << company_id << ";" << salary << ";"
		   << fname.data() << ";" << lname.data() << ";"
		   << name.data() << ";" << slogan.data();
        return ss.str();
    }
};

auto loadFileInDisk (BufferManager& buffer, std::string fileName, address_id_t startingAddress) -> std::optional<std::pair<address_id_t,address_id_t>>;

template <typename T>
T extractData(const std::vector<std::byte> &data);

static constexpr auto EmployeeSize = sizeof(Employee);
static constexpr auto CompanySize = sizeof(Company);
static constexpr auto JoinedSize = sizeof(JoinEmployeeCompany);

#endif // _UTILS_HPP_