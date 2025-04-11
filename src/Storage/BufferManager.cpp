#include "Utils.hpp"
#include <Storage/BufferManager.hpp>

BufferManager::BufferManager ( Disk *_disk, int _replaceStrategy, unsigned int _numFrames )
    : disk( _disk ),
      replaceStrategy( _replaceStrategy ),
      numFrames( _numFrames ),
      bufferData( _numFrames,
      std::vector< std::byte >( disk->blockSize ) ),
      pinCount( _numFrames, 0 ),
      isDirty( _numFrames, false )
{
    for ( frame_id_t i = 0; i < numFrames; ++i )
    {
        freeFrames.push( i );
    }
}

BufferManager::~BufferManager ()
{
    for ( frame_id_t i = 0; i < numFrames; ++i )
    {
        if ( isDirty[i] )
        {
            disk->writeBlock( i, bufferData[i] );
        }
    }
}

auto BufferManager::findVictim ( ) -> std::optional< frame_id_t >
{
    if( replaceStrategy == LRU )
    {
        for ( auto it = busyFrames.begin(); it != busyFrames.end(); ++it )
        {
            if ( pinCount[*it] == 0 )
            {
                if (isDirty[*it])
                {
                    // TODO: Need page number of this frame
                    // disk->writeBlock( ..., bufferData[*it] );
                }
                framePos.erase( *it );
                busyFrames.erase( it );
                return *it;
            }
        }
    }
    else if ( replaceStrategy == MRU )
    {
        for ( auto it = busyFrames.rbegin(); it != busyFrames.rend(); ++it )
        {
            if ( pinCount[*it] == 0 )
            {
                if (isDirty[*it])
                {
                    // TODO: Need page number of this frame
                    // disk->writeBlock( ..., bufferData[*it] );
                }
                framePos.erase( *it );
                busyFrames.erase( std::next( it ).base() );
                return *it;
            }
        }
    }
    return std::nullopt;
}

auto BufferManager::findFreeFrame ( ) -> std::optional< frame_id_t >
{
    if ( freeFrames.empty() )
    {
        return findVictim();
    }
    else
    {
        frame_id_t frame = freeFrames.top();
        freeFrames.pop();
        return frame;
    }
    return std::nullopt;
}

auto BufferManager::getFrame ( page_id_t pageNumber ) -> std::optional< frame_id_t >
{
    if ( pageTable.find( pageNumber ) == pageTable.end() )
    {
        auto frame = findFreeFrame();
        if ( frame.has_value() )
        {
            busyFrames.push_back( frame.value() );
            pageTable[pageNumber] = frame.value();
        }
        else
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}