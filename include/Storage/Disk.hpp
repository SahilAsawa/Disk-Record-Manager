#pragma once

#ifndef _DISK_HPP_
    #define _DISK_HPP_

    #include <vector>
    #include <fstream>
    
    #include <Utilities/Utils.hpp>
    
    #define SEQUENTIAL 1
    #define RANDOM 0

class Disk
{
    friend class BufferManager;

    private:

    // Access type supported by disk: RANDOM or SEQUENTIAL
    bool accessType;

    // Size of a block in bytes
    storage_t blockSize;

    // Number of blocks in the disk
    size_t blockCount;

    // File name of the disk
    std::string diskFile;

    // fstream file
    std::fstream diskFileStream;

    // number of IO operations
    unsigned long long numIO;

    // cost of IO operations
    unsigned long long costIO;
    
    /**
     * @brief Read a block from the disk.
     * @param blockNumber The block number to read.
     * @return A vector of bytes representing the block data.
     */
    auto readBlock ( block_id_t blockNumber ) -> std::vector< std::byte >;
    
    /**
     * @brief Write data to a block in the disk.
     * @param blockNumber The block number to write to.
     * @param data The data to write to the block.
     */
    auto writeBlock ( block_id_t blockNumber, const std::vector< std::byte > &data ) -> void;

    public:

    // Constructor
    Disk ( bool _accessType, storage_t _blockSize = (4 KB), storage_t _diskSize = (4 GB), std::string _diskFile = "disk.dat" );

    // Destructor
    ~Disk ();
};

#endif // _DISK_HPP_