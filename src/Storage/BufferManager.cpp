#include <Utilities/Utils.hpp>
#include <Storage/BufferManager.hpp>

BufferManager::BufferManager ( Disk *_disk, int _replaceStrategy, storage_t _bufferSize )
    : disk( _disk ),
      replaceStrategy( _replaceStrategy ),
      numFrames( _bufferSize / disk->blockSize ),
      bufferData( _bufferSize / disk->blockSize,
      std::vector< std::byte >( disk->blockSize ) ),
      pinCount( _bufferSize / disk->blockSize, 0 ),
      isDirty( _bufferSize / disk->blockSize, false )
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
            disk->writeBlock( invPageTable[i], bufferData[i] );
            isDirty[i] = false;
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
                if ( isDirty[*it] )
                {
                    disk->writeBlock( invPageTable[*it], bufferData[*it] );
                }
                auto frame = *it;
                busyFrames.erase( it );
                framePos.erase( frame );
                pageTable.erase( invPageTable[frame] );
                invPageTable.erase( frame );
                return frame;
            }
        }
    }
    else if ( replaceStrategy == MRU )
    {
        for ( auto it = busyFrames.rbegin(); it != busyFrames.rend(); ++it )
        {
            if ( pinCount[*it] == 0 )
            {
                if ( isDirty[*it] )
                {
                    disk->writeBlock( invPageTable[*it], bufferData[*it] );
                }
                auto frame = *it;
                busyFrames.erase( std::next( it ).base() );
                framePos.erase( frame );
                pageTable.erase( invPageTable[frame] );
                invPageTable.erase( frame );
                return frame;
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
    // Not present in buffer
    if ( pageTable.find( pageNumber ) == pageTable.end() )
    {
        auto frame = findFreeFrame();
        if ( frame.has_value() )
        {
            busyFrames.push_back( frame.value() );
            framePos[frame.value()] = std::prev( busyFrames.end() );
            pageTable[pageNumber] = frame.value();
            invPageTable[frame.value()] = pageNumber;
            bufferData[frame.value()] = disk->readBlock( pageNumber );
        }
        else
        {
            return std::nullopt;
        }
    }

    frame_id_t frame = pageTable[pageNumber];

    // Update the position of the frame in the busy list
    auto it = framePos.find( frame );
    if ( it != framePos.end() )
    {
        busyFrames.erase( it->second );
        busyFrames.push_back( frame );
        framePos[frame] = std::prev( busyFrames.end() );
        return frame;
    }

    return std::nullopt;
}


auto BufferManager::readPage ( page_id_t pageNumber ) -> std::vector< std::byte >
{
    if( pageNumber >= disk->blockCount )
    {
        throw std::runtime_error( "Page number out of range");
    }
    auto frameNumber = getFrame( pageNumber );
    if(frameNumber.has_value())
    {
        return bufferData[ frameNumber.value() ];
    }
    else
    {
        throw std::runtime_error( "Buffer space full");
    }
}

auto BufferManager::writePage ( page_id_t pageNumber, const std::vector< std::byte > &data ) -> void
{
    if( pageNumber >= disk->blockCount )
    {
        throw std::runtime_error( "Page number out of range");
    }
    auto frameNumber = getFrame( pageNumber );
    if(frameNumber.has_value())
    {
        bufferData[frameNumber.value()] = data;
        isDirty[frameNumber.value()] = true;
    }
    else
    {
        throw std::runtime_error( "Buffer space full");
    }
}

auto BufferManager::readAddress ( address_id_t address, storage_t size ) -> std::vector< std::byte >
{
    auto pageNumber = address / disk->blockSize;
    auto offset = address % disk->blockSize;

    std::vector< std::byte > data( size );
    auto firstFrame = readPage( pageNumber );

    if( offset + size <= disk->blockSize )
    {
        std::copy( firstFrame.begin() + offset, firstFrame.begin() + offset + size, data.begin() );
    }
    else
    {
        std::copy( firstFrame.begin() + offset, firstFrame.end(), data.begin() );
        
        storage_t remainingSize = size - ( disk->blockSize - offset );

        while( remainingSize > 0 )
        {
            pageNumber++;
            auto nextFrame = readPage( pageNumber );
            storage_t bytesToCopy = std::min( remainingSize, disk->blockSize );
            std::copy( nextFrame.begin(), nextFrame.begin() + bytesToCopy, data.begin() + size - remainingSize );
            remainingSize -= bytesToCopy;
        }
    }
    return data;
}

auto BufferManager::writeAddress ( address_id_t address, const std::vector< std::byte > &data ) -> void
{
    auto pageNumber = address / disk->blockSize;
    auto offset = address % disk->blockSize;

    auto firstFrame = readPage( pageNumber );

    if( offset + data.size() <= disk->blockSize )
    {
        std::copy( data.begin(), data.end(), firstFrame.begin() + offset );
        writePage( pageNumber, firstFrame );
    }
    else
    {
        std::copy( data.begin(), data.begin() + disk->blockSize - offset, firstFrame.begin() + offset );
        writePage( pageNumber, firstFrame );
        
        storage_t remainingSize = data.size() - ( disk->blockSize - offset );

        while( remainingSize > 0 )
        {
            pageNumber++;
            auto nextFrame = readPage( pageNumber );
            storage_t bytesToCopy = std::min( remainingSize, disk->blockSize );
            std::copy( data.begin() + data.size() - remainingSize, data.begin() + data.size() - remainingSize + bytesToCopy, nextFrame.begin() );
            remainingSize -= bytesToCopy;
            
            writePage( pageNumber, nextFrame );
        }
    }
    return;
}