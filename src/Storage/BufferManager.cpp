#include "Utils.hpp"
#include <Storage/BufferManager.hpp>

BufferManager::BufferManager ( Disk *_disk, int _replaceStrategy, unsigned int _numFrames )
    : disk( _disk ), replaceStrategy( _replaceStrategy ), numFrames( _numFrames ),
      bufferData( _numFrames, std::vector< std::byte >( disk->blockSize ) ),
      pinCount( _numFrames, 0 ), isDirty( _numFrames, false )
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

auto BufferManager::findVictim ( ) -> frame_id_t
{
    if( replaceStrategy == LRU )
    {
        
    }
    else
    {

    }
}