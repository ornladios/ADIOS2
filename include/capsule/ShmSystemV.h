
#ifndef SHMSYSTEMV_H_
#define SHMSYSTEMV_H_

#include <sys/types.h>
#include <sys/ipc.h>


#include "core/Capsule.h"


namespace adios
{

/**
 * Buffer and Metadata are allocated in virtual memory using interprocess communication (IPC) of Unix's System V
 */
class ShmSystemV : public Capsule
{

public:

    char* m_Data = nullptr; ///< reference to a shared memory data buffer created with shmget
    size_t m_DataSize = 2147483648; ///< size of the allocated shared memory segment, needs a default (2 Gb?)
    key_t m_DataKey; ///< key associated with the data buffer, created with ftok
    int m_DataShmID; ///< data shared memory buffer id

    char* m_Metadata = nullptr; ///< reference to a shared memory metadata buffer created with shmget
    size_t m_MetadataSize = 1048576; ///< size of the allocated shared memory segment, needs a default ( 1 Mb?)
    key_t m_MetadataKey; ///< key associated with the metadata buffer, created with ftok
    int m_MetadataShmID; ///< metadata shared memory buffer id

    /**
     * Create a Capsule in shared memory
     * @param accessMode
     * @param pathName used to create the key as a unique identifier
     * @param id used to create the key as a unique identifier, non-zero typically rank+1
     * @param dataSize size of allocated memory segment for data
     * @param metadataSize size of allocated memory segment for metadata
     */
    ShmSystemV( const std::string accessMode, const int rankMPI, const std::string pathName, const int id,
                const size_t dataSize = 2147483648, const size_t metadataSize = 0 );

    ~ShmSystemV( );


    void Write( const Variable<char>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<unsigned char>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<short>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<unsigned short>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<int>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<unsigned int>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<long int>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<unsigned long int>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<long long int>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<unsigned long long int>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<float>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<double>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<long double>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

};

} //end namespace


#endif /* SHMSYSTEMV_H_ */
