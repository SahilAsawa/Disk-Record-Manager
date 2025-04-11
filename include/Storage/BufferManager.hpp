#pragma once

#ifndef _BUFFER_MANAGER_HPP_
    #define _BUFFER_MANAGER_HPP_

    #include <unordered_map>
    #include <stack>
    #include <list>

    #include <Utils.hpp>
    #include <Storage/Disk.hpp>

    #define LRU 0
    #define MRU 1
    #define CLOCK 2

class BufferManager 
{
    private:

    //
    Disk *disk;

    //
    int replaceStrategy;

    // number of frames
    unsigned int numFrames;

    // Global page table
    std::unordered_map< page_id_t, frame_id_t > pageTable {};

    // actual buffer data
    std::vector< std::vector< std::byte > > bufferData;

    // free frame list
    std::stack< frame_id_t > freeFrames {};

    // pin count for each frame
    std::vector< int > pinCount;

    // list of frames in use
    std::list< frame_id_t > busyFrames {};

    // map of frame id to list iterator
    std::unordered_map< frame_id_t, std::list< frame_id_t >::iterator > framePos {};

    // dirty bit for each frame
    std::vector< bool > isDirty;

    auto findVictim ( ) -> frame_id_t;

    auto readPage ( page_id_t pageNumber ) -> std::vector< std::byte >;

    auto writePage ( page_id_t pageNumber, const std::vector< std::byte > &data ) -> void;

    public:

    // Constructor
    BufferManager (  Disk *_disk, int _replaceStrategy = LRU, unsigned int _numFrames = 1024);

    // Destructor
    ~BufferManager ();

    
};

#endif // _BUFFER_MANAGER_HPP_