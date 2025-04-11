#include <Storage/Disk.hpp>

Disk::Disk ( bool _accessType, size_t _blockSize, size_t _blockCount, std::string _diskFile )
        : accessType( _accessType), blockSize( _blockSize ), blockCount( _blockCount ), diskFile( _diskFile ), 
            numIO( 0 ), costIO( 0 )
{
    diskFileStream.open( diskFile, std::ios::in | std::ios::out | std::ios::binary );
    if (!diskFileStream.is_open())
    {
        diskFileStream.open( diskFile, std::ios::out | std::ios::binary );
        diskFileStream.close();
        diskFileStream.open( diskFile, std::ios::in | std::ios::out | std::ios::binary );
        if( !diskFileStream.is_open())
        {
            throw std::runtime_error( "Disk file could not be created / opened" );
        }
    }
    diskFileStream.seekg( 0, std::ios::end );

    if ( diskFileStream.tellg() == 0 )
    {
        std::vector< std::byte > emptyBlock( blockSize, std::byte( 0 ) );
        for ( size_t i = 0; i < blockCount; ++i )
        {
            diskFileStream.write( reinterpret_cast< const char * >( emptyBlock.data() ), blockSize );
        }
    }
    diskFileStream.seekg( 0, std::ios::beg );
    diskFileStream.seekp( 0, std::ios::beg );
    diskFileStream.flush();
}   

Disk::~Disk ()
{
    diskFileStream.flush();
    diskFileStream.close();
}

auto Disk::readBlock ( block_id_t blockNumber ) -> std::vector< std::byte >
{
    if ( blockNumber >= blockCount )
    {
        throw std::out_of_range( "Block number out of range" );
    }

    // seek cost
    if( accessType == SEQUENTIAL ) costIO += (blockNumber - diskFileStream.tellg() / blockSize + blockCount) % blockCount;
    ++costIO;
    ++numIO;
    
    diskFileStream.seekg( blockNumber * blockSize, std::ios::beg );

    std::vector< std::byte > data( blockSize );
    diskFileStream.read( reinterpret_cast< char * >( data.data() ), blockSize );
    
    return data;
}

auto Disk::writeBlock ( block_id_t blockNumber, const std::vector< std::byte > &data ) -> void
{
    if ( blockNumber >= blockCount )
    {
        throw std::out_of_range( "Block number out of range" );
    }

    if( accessType == SEQUENTIAL ) costIO += (blockNumber - diskFileStream.tellp() / blockSize + blockCount) % blockCount;
    ++costIO;
    ++numIO;
    
    diskFileStream.seekp( blockNumber * blockSize, std::ios::beg );
    diskFileStream.write( reinterpret_cast< const char * >( data.data() ), blockSize );
}
