#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <random>
#include <Utilities/Utils.hpp>

using Table = std::vector<std::vector<std::string>>;

void shuffleOuterVector(std::vector<std::vector<std::string>>& vec) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(vec.begin(), vec.end(), g);
}

Table readCSV(const std::string &filename, char delim = ',')
{
    std::cout << "Reading file: " << filename << '\n';
    std::ifstream file{filename};

    std::istringstream sstr{};
    Table data{};

    std::string line{}, word{};

    std::getline(file, line);
    while (std::getline(file, line))
    {
        std::vector<std::string> vec{};
        sstr.str(line);

        while (std::getline(sstr, word, delim))
        {
            word.erase(0, word.find_first_not_of(" \t\n\r\""));
            word.erase(word.find_last_not_of(" \t\n\r\"") + 1);

            vec.emplace_back(word);
        }

        data.emplace_back(vec);
        sstr.clear();
    }

    file.close();

    return data;
}

int main()
{
    std::cout << "Size of Employee: " << sizeof(Employee) << '\n';
    std::cout << "Size of Company:  " << sizeof(Company) << '\n';

    try
    {
        auto data = readCSV(CSV_DIR + "employee.csv", ',');
        std::cout << "employee.csv size : " << data.size() << '\n';
        std::cout << "Total Attributes : " << data[0].size() << '\n';

        // shuffleOuterVector(data);
        // std::ofstream file1(CSV_DIR + "employee_random.csv");
        // for (const auto &row : data)
        // {
        //     for (size_t i = 0; i < row.size(); ++i)
        //     {
        //         file1 << row[i];
        //         if (i != row.size() - 1)
        //             file1 << ',';
        //     }
        //     file1 << '\n';
        // }
        // file1.close();

        std::vector<Employee> employees{};

        for (auto &&row : data)
        {
            Employee employee{};
            employee.id = std::stoi(row[0]);
            std::strcpy(employee.fname.data(), row[1].c_str());
            std::strcpy(employee.lname.data(), row[2].c_str());
            employee.salary = std::stoi(row[3]);
            employee.company_id = std::stoi(row[4]);

            employees.emplace_back(employee);
        }

        std::ofstream file{BIN_DIR + "employee.bin", std::ios::binary};

        for (const auto &employee : employees)
        {
            file.write(reinterpret_cast<const char *>(&employee),
                       sizeof(Employee));
        }

        file.close();
        file.clear();

        // Read employees.csv

        data = readCSV(CSV_DIR + "company.csv", ';');
        std::cout << "company.csv Size : " << data.size() << '\n';
        std::cout << "Total Attributes : " << data[0].size() << '\n';

        // shuffleOuterVector(data);
        // std::ofstream file2(CSV_DIR + "company_random.csv");
        // for (const auto &row : data)
        // {
        //     for (size_t i = 0; i < row.size(); ++i)
        //     {
        //         file2 << row[i];
        //         if (i != row.size() - 1)
        //             file2 << ';';
        //     }
        //     file2 << '\n';
        // }
        // file2.close();

        std::vector<Company> companys{};

        for (const auto &row : data)
        {
            Company company{};
            company.id = std::stoi(row[0]);
            std::strcpy(company.name.data(), row[1].c_str());
            std::strcpy(company.slogan.data(), row[2].c_str());

            companys.emplace_back(company);
        }

        file.open(BIN_DIR + "company.bin", std::ios::binary);

        for (const auto &company : companys)
        {
            file.write(reinterpret_cast<const char *>(&company), sizeof(Company));
        }

        file.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}
