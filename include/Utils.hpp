#pragma once
#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <array>
#include <compare>
#include <cstdint>
#include <cstring>
#include <algorithm>

using frame_id_t = unsigned long long;
using page_id_t = unsigned long long;
using block_id_t = unsigned long long;
using address_id_t = unsigned long long;

class Disk;
class BufferManager;

struct Employee
{
	int32_t id;
	int32_t company_id;
	int32_t salary;
	std::array<char, 58> fname;
	std::array<char, 58> lname;

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
};

struct Company
{
	int32_t id;
	std::array<char, 62> name;
	std::array<char, 62> slogan;

	friend auto operator==(const Company &lhs, const Company &rhs) -> bool
	{
		return lhs.id == rhs.id;
	}

	friend auto operator<=>(const Company &lhs, const Company &rhs)
		-> std::strong_ordering
	{
		return lhs.id <=> rhs.id;
	}
};

struct JoinEmployeeCompany
{
	int32_t employee_id;
	int32_t company_id;
	int32_t salary;
	std::array<char, 58> fname{};
	std::array<char, 58> lname{};
	std::array<char, 64> name = {};   // '\0' initialized
	std::array<char, 64> slogan = {}; // '\0' initialized

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

	JoinEmployeeCompany() = default;
};

static constexpr auto EmployeeSize = sizeof(Employee);
static constexpr auto CompanySize = sizeof(Company);
static constexpr auto JoinedSize = sizeof(JoinEmployeeCompany);

#endif // _UTILS_HPP_