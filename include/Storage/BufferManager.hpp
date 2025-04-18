#pragma once

#ifndef _BUFFER_MANAGER_HPP_
    #define _BUFFER_MANAGER_HPP_

    #include <unordered_map>
    #include <optional>
    #include <stack>
    #include <list>

    #include <Utilities/Utils.hpp>
    #include <Storage/Disk.hpp>

    #define LRU 0
    #define MRU 1

class BufferManager 
{
    private:

    // Pointer to the disk object
    Disk *disk;

    // Replacement strategy: LRU or MRU
    int replaceStrategy;

    // number of frames
    unsigned int numFrames;

    // number of IO operations
    unsigned long long numIO;

    // Global page table: Page ID -> Frame ID
    std::unordered_map< page_id_t, frame_id_t > pageTable {};

    // Inverted page table: Frame ID -> Page ID
    std::unordered_map< frame_id_t, page_id_t > invPageTable {};

    // actual buffer data
    std::vector< std::vector< std::byte > > bufferData;

    // free frame list
    std::stack< frame_id_t > freeFrames {};

    // pin count for each frame
    std::vector< int > pinCount;

    // list of frames in use, recently used at the end
    std::list< frame_id_t > busyFrames {};

    // map of frame id to list iterator
    std::unordered_map< frame_id_t, std::list< frame_id_t >::iterator > framePos {};

    // dirty bit for each frame
    std::vector< bool > isDirty;

    /**
     * @brief Find a victim frame to replace using the specified replacement strategy.
     * @returns The frame ID of the victim frame, or std::nullopt if no victim frame is found.
     */
    auto findVictim ( ) -> std::optional< frame_id_t >;

    /**
     * @brief Find a free frame to use, frees a victim frame if no free frame is available.
     * @returns The frame ID of the free frame, or std::nullopt if no free frame is found.
     */
    auto findFreeFrame ( ) -> std::optional< frame_id_t >;

    /**
     * @brief Get the frame ID for a given page number, the page's data is present in buffer in this frame.
     * @param pageNumber The page number to get the frame for.
     * @returns The frame ID of the page, or std::nullopt if the page is not in the buffer.
     * @note Pin count is not updated here, it should be done in the caller function.
     */
    auto getFrame ( page_id_t pageNumber ) -> std::optional< frame_id_t >;

    /**
     * @brief Read data for a given page number.
     * @param pageNumber The page number to get the data for.
     * @returns The data for the page.
     */
    auto readPage ( page_id_t pageNumber ) -> std::vector< std::byte >;

    /**
     * @brief Write data for a given page number.abort
     * @param pageNumber The page number to update the data for.
     * @param data The data to write for the page.
     */
    auto writePage ( page_id_t pageNumber, const std::vector< std::byte > &data ) -> void;

    public:

    // Constructor
    BufferManager (  Disk *_disk, int _replaceStrategy = LRU, storage_t _bufferSize = (4 MB) );

    // Destructor
    ~BufferManager ();

    /**
     * @brief Read data from a given address.
     * @param address The address to read from.
     * @param size The size of the data to read.
     * @returns The data read from the address.
     */
    auto readAddress ( address_id_t address, storage_t size ) -> std::vector< std::byte >;

    /**
     * @brief Write data to a given address.
     * @param address The address to write to.
     * @param data The data to write to the address.
     */
    auto writeAddress ( address_id_t address, const std::vector< std::byte > &data ) -> void;

    /**
     * @brief Get the number of disk IO till now from creation of disk.
     * @returns The number of disk IO.
     */
    auto getNumIO ( ) const -> unsigned long long
    {
        return disk->numIO;
    }

    /**
     * @brief Get the cost of disk IO till now from creation of disk.
     * @returns The cost of disk IO.
     */
    auto getCostIO ( ) const -> unsigned long long
    {
        return disk->costIO;
    }

    /**
     * @brief Get the number of frames in the buffer manager.
     * @returns The number of frames.
     */
    auto getNumFrames ( ) const -> unsigned int
    {
        return numFrames;
    }

    /**
     * @brief Get the type of replacement strategy used.
     * @returns The type of replacement strategy used, LRU or MRU macro.
     * @note LRU = 0, MRU = 1
     */
    auto getReplaceStrategy ( ) const -> int
    {
        return replaceStrategy;
    }

    /**
     * @brief Clear the buffer
     * @note This function clears the buffer and writes all dirty pages to the disk.
     */
    auto clearCache( ) -> void;

    /**
     * @brief Get the statistics related to IO operations.
     * @returns A Stats object containing the number of IO operations, disk accesses, and cost of disk accesses.
     * @note The statistics are updated after each IO operation.
     */
    auto getStats ( ) const -> Stats
    {
        return { (long long) numIO, (long long) disk->numIO, (long long) disk->costIO };
    }

    /**
     * @brief Print the statistics of related to IO operations.
     * @param os The output stream to print the statistics to.
     * @param startStats The starting statistics to compare with.
     * @param header The header to print before the statistics.
     * @note You can store initial statistics in a Stats object using the getStats() method.
     */
    auto printStats ( std::ostream &os, Stats &startStats, std::string header = "" ) -> void;
};

#endif // _BUFFER_MANAGER_HPP_