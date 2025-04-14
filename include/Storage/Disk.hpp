#pragma once

#ifndef _DISK_HPP_
    #define _DISK_HPP_

    #include <Utilities/Utils.hpp>
    #include <vector>
    #include <fstream>

    #define SEQUENTIAL 1
    #define RANDOM 0

class Disk
{
    friend class BufferManager;

    private:

    //
    bool accessType;

    //
    size_t blockSize;

    //
    size_t blockCount;

    //
    std::string diskFile;

    // fstream file
    std::fstream diskFileStream;

    // number of IO operations
    unsigned long long numIO;

    // cost of IO operations
    unsigned long long costIO;
    
    //
    auto readBlock ( block_id_t blockNumber ) -> std::vector< std::byte >;
    
    //
    auto writeBlock ( block_id_t blockNumber, const std::vector< std::byte > &data ) -> void;

    public:

    //
    Disk ( bool _accessType = SEQUENTIAL, size_t _blockSize = 4096, size_t _blockCount = 1024, std::string _diskFile = "disk.dat" );

    //
    ~Disk ();
};

#endif // _DISK_HPP_