/*
 * BP1.h
 *
 *  Created on: Jan 24, 2017
 *      Author: wfg
 */

#ifndef BP1WRITER_H_
#define BP1WRITER_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <vector>
#include <cstdint>  //std::intX_t fixed size integers
#include <algorithm> //std::count, std::copy, std::for_each
#include <cstring> //std::memcpy
#include <cmath>   //std::ceil
/// \endcond

#include "BP1.h"
#include "core/Variable.h"
#include "core/Capsule.h"
#include "core/Profiler.h"
#include "capsule/heap/STLVector.h"
#include "functions/adiosTemplates.h"
#include "functions/adiosFunctions.h"


namespace adios
{
namespace format
{


class BP1Writer : public BP1
{

public:

    unsigned int m_Cores = 1;  ///< number of cores for thread operations in large array (min,max)
    unsigned int m_Verbosity = 0; ///< statistics verbosity, can change if redefined in Engine method.
    float m_GrowthFactor = 1.5; ///< memory growth factor, can change if redefined in Engine method.
    const std::uint8_t m_Version = 3; ///< BP format version

    /**
     * Calculates the Process Index size in bytes according to the BP format, including list of method with no parameters (for now)
     * @param name
     * @param timeStepName
     * @param numberOfTransports
     * @return size of pg index
     */
    std::size_t GetProcessGroupIndexSize( const std::string name, const std::string timeStepName,
                                          const std::size_t numberOfTransports ) const noexcept;

    /**
     * Writes a process group index PGIndex and list of methods (from transports), done at Open or aggregation of new time step
     * Version that operates on a single heap buffer and metadataset.
     * @param isFortran
     * @param name
     * @param processID
     * @param timeStepName
     * @param timeStep
     * @param transports
     * @param buffer
     * @param metadataSet
     */
    void WriteProcessGroupIndex( const bool isFortran, const std::string name, const unsigned int processID,
                                 const std::string timeStepName, const unsigned int timeStep,
                                 const std::vector< std::shared_ptr<Transport> >& transports,
                                 capsule::STLVector& buffer,
                                 BP1MetadataSet& metadataSet ) const noexcept;

    /**
     * Returns the estimated variable index size
     * @param group
     * @param variableName
     * @param variable
     * @param verbosity
     * @return variable index size
     */
    template< class T >
    size_t GetVariableIndexSize( const Variable<T>& variable ) const noexcept
    {
        //size_t indexSize = varEntryLength + memberID + lengthGroupName + groupName + lengthVariableName + lengthOfPath + path + datatype
        size_t indexSize = 23; //without characteristics
        indexSize += variable.m_Name.size();

        // characteristics 3 and 4, check variable number of dimensions
        const std::size_t dimensions = variable.DimensionsSize(); //number of commas in CSV + 1
        indexSize += 28 * dimensions; //28 bytes per dimension
        indexSize += 1; //id

        //characteristics 3, variable offset in data
        indexSize += 8;
        indexSize += 1; //id
        //characteristics 6, variable payload offset in data
        indexSize += 8;
        indexSize += 1; //id

        //characteristic 0, if scalar add value, for now only allowing string
        if( dimensions == 1 )
        {
            indexSize += sizeof(T);
            indexSize += 1; //id
            //must have an if here
            indexSize += 2 + variable.m_Name.size();
            indexSize += 1; //id
        }

        //characteristic statistics
        if( m_Verbosity == 0 ) //default, only min and max
        {
            indexSize += 2 * ( sizeof(T) + 1 );
            indexSize += 1 + 1; //id
        }

        return indexSize + 12; ///extra 12 bytes in case of attributes
        //need to add transform characteristics
    }

    /**
         *
         * @param variable
         * @param dataBuffers
         * @param dataPositions
         * @param dataAbsolutePositions
         * @param metadataBuffers
         * @param metadataPositions
         * @param variablesCount
         */
    template<class T>
    void WriteVariableIndex( const Variable<T>& variable, capsule::STLVector& buffer, BP1MetadataSet& metadataSet ) const noexcept
    {
        auto lf_String = []( const std::string name, const std::uint16_t length,
                             std::vector<char>& buffer, std::size_t& position )
        {
            MemcpyToBuffer( buffer, position, &length, 2 );
            MemcpyToBuffer( buffer, position, name.c_str(), length );
        };

        auto lf_MemberID = []( const std::uint32_t memberID, capsule::STLVector& buffer, BP1MetadataSet& metadataSet )
        {
            MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &memberID, 4 );
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &memberID, 4 );
        };

        auto lf_VarName = [&]( const std::string name, capsule::STLVector& buffer, BP1MetadataSet& metadataSet )
        {
            const std::uint16_t length = name.length();
            lf_String( name, length, buffer.m_Data, buffer.m_DataPosition );
            lf_String( name, length, metadataSet.VarsIndex, metadataSet.VarsIndexPosition );
        };

        auto lf_DataType = []( const std::uint8_t dataType, capsule::STLVector& buffer, BP1MetadataSet& metadataSet )
        {
            MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &dataType, 1 );
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &dataType, 1 );
        };


        //BODY of function starts here
        //capture initial positions storing the variable Length
        const std::size_t metadataVarLengthPosition = metadataSet.VarsIndexPosition;
        const std::size_t dataVarLengthPosition = buffer.m_DataPosition;

        metadataSet.VarsIndexPosition += 4; //skip var length
        buffer.m_DataPosition += 8; //skip var length

        lf_MemberID( metadataSet.VarsCount, buffer, metadataSet ); //memberID in metadata and data
        metadataSet.VarsIndexPosition += 2; //skipping 2 bytes for length of group name which is zero, only in metadata
        lf_VarName( variable.m_Name, buffer, metadataSet ); //variable name to metadata and data

        metadataSet.VarsIndexPosition += 2; //skip path
        buffer.m_DataPosition += 2; //skip path

        //dataType
        const std::uint8_t dataType = GetDataType<T>();
        lf_DataType( dataType, buffer, metadataSet );

        //write in data if it's a dimension variable (scalar) y or n
        const char dimensionYorN = ( variable.m_IsDimension ) ? 'y' : 'n';
        MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &dimensionYorN, 1 );

        //Characteristics Sets Count in Metadata
        const std::uint64_t sets = 1; //write one for now
        MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &sets, 8 );

        //Characteristics set
        std::uint8_t characteristicsCounter = 0; //used for characteristics count, characteristics length will be calculated at the end
        const std::size_t metadataCharacteristicsCountPosition = metadataSet.VarsIndexPosition;
        metadataSet.VarsIndexPosition += 5; //here move positions 5 bytes in data and metadata for characteristics count + length

        //DIMENSIONS CHARACTERISTIC
        const std::vector<std::size_t>& localDimensions = variable.m_Dimensions;

        //write to metadata characteristic
        //characteristic: dimension
        std::uint8_t characteristicID = characteristic_dimensions;
        MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &characteristicID, 1 );
        const std::uint8_t dimensions = localDimensions.size();
        MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &dimensions, 1 );
        const std::uint16_t dimensionsLength = dimensions * 24; //24 is from 8 bytes for each: local dimension, global dimension, global offset
        MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &dimensionsLength, 2 );

        //write dimensions count and length in data
        MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &dimensions, 1 );
        const std::uint16_t dimensionsLengthInData = dimensions * 27; //27 is from 9 bytes for each: var y/n + local, var y/n + global dimension, var y/n + global offset
        MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &dimensionsLengthInData, 2 );

        std::size_t dataCharacteristicsCountPosition = buffer.m_DataPosition; //will be modified

        if( variable.m_GlobalDimensions.empty() ) //local variable
        {
            WriteDimensionRecord( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, localDimensions, 16 );
            WriteDimensionRecord( buffer.m_Data, buffer.m_DataPosition, localDimensions, 18, true ); //not using memberID for now

            dataCharacteristicsCountPosition = buffer.m_DataPosition; //very important to track as writer is going back to this position
            buffer.m_DataPosition += 5; //skip characteristics count(1) + length (4)

            //dimensions in data characteristic entry
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &characteristicID, 1 );
            const std::int16_t lengthOfDimensionsCharacteristic = 24 * dimensions; // 24 = 3 local, global, global offset x 8 bytes/each
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &lengthOfDimensionsCharacteristic, 2 );
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &dimensions, 1 );
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &dimensionsLength, 2 );
            WriteDimensionRecord( buffer.m_Data, buffer.m_DataPosition, localDimensions, 16 );
        }
        else //global variable
        {
            const std::vector<std::size_t>& globalDimensions = variable.m_GlobalDimensions;
            const std::vector<std::size_t>& globalOffsets = variable.m_GlobalOffsets;

            WriteDimensionRecord( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, localDimensions, globalDimensions, globalOffsets );
            WriteDimensionRecord( buffer.m_Data, buffer.m_DataPosition, localDimensions, globalDimensions, globalOffsets, true );

            dataCharacteristicsCountPosition = buffer.m_DataPosition; //very important, going back to these positions
            buffer.m_DataPosition += 5; //skip characteristics count(1) + length (4)

            //dimensions in data characteristic entry
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &characteristicID, 1 ); //id
            const std::int16_t lengthOfDimensionsCharacteristic = 24 * dimensions; // 24 = 3 local, global, global offset x 8 bytes/each
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &lengthOfDimensionsCharacteristic, 2 );
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &dimensions, 1 );
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &dimensionsLength, 2 );
            WriteDimensionRecord( buffer.m_Data, buffer.m_DataPosition, localDimensions, globalDimensions, globalOffsets );
        }
        ++characteristicsCounter;

        //VALUE for SCALAR or STAT min, max for ARRAY
        //Value for scalar
        if( variable.m_IsScalar ) //scalar //just doing string scalars for now (by name), needs to be modified when user passes value
        {
            characteristicID = characteristic_value;
            MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &characteristicID, 1  );
            MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, variable.m_AppValues, sizeof(T) );

            //data
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &characteristicID, 1 );
            const std::uint16_t lengthOfValue = sizeof( T );
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &lengthOfValue, 2 ); //add length of characteristic in data
            MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, variable.m_AppValues, sizeof(T) );

            ++characteristicsCounter;
        }
        else // Stat -> Min, Max for arrays,
        {
            if( m_Verbosity == 0 ) //default verbose
            {
                WriteMinMax( variable, buffer, metadataSet );
                characteristicsCounter += 2;
            }
        }

        //Characteristic time index in metadata and data
        characteristicID = characteristic_time_index;
        MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &characteristicID, 1  );
        MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &metadataSet.TimeStep, 4 );

        MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &characteristicID, 1 );
        const std::uint16_t lengthOfTimeIndex = 4;
        MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &lengthOfTimeIndex, 2 ); //add length of characteristic in data
        MemcpyToBuffer( buffer.m_Data, buffer.m_DataPosition, &metadataSet.TimeStep, 4 );
        ++characteristicsCounter;

        //Back to characteristics count and length in Data
        //count
        std::memcpy( &buffer.m_Data[dataCharacteristicsCountPosition], &characteristicsCounter, 1 );
        //length
        const std::uint32_t dataCharacteristicsLength = buffer.m_DataPosition - dataCharacteristicsCountPosition - 4 - 1; //remove its own length (4 bytes) + characteristic counter ( 1 byte )
        std::memcpy( &buffer.m_Data[dataCharacteristicsCountPosition+1], &dataCharacteristicsLength, 4 );

        //Metadata only: Offsets should be last, they come from data absolute positions
        characteristicID = characteristic_offset;
        MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &characteristicID, 1 ); //variable offset id
        MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &buffer.m_DataAbsolutePosition, 8 ); //variable offset
        ++characteristicsCounter;

        //Back to length of var including payload size in data + update absolute position
        const std::uint64_t varLength = buffer.m_DataPosition - dataVarLengthPosition + variable.PayLoadSize() - 8; //remove its own size
        std::memcpy( &buffer.m_Data[dataVarLengthPosition], &varLength, 8 );
        buffer.m_DataAbsolutePosition += buffer.m_DataPosition - dataVarLengthPosition; //payload offset

        characteristicID = characteristic_payload_offset;
        MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &characteristicID, 1 ); //variable payload offset id
        MemcpyToBuffer( metadataSet.VarsIndex, metadataSet.VarsIndexPosition, &buffer.m_DataAbsolutePosition, 8 ); //variable payload offset
        ++characteristicsCounter;

        //Back to writing characteristics count and length in Metadata
        //count
        std::memcpy( &metadataSet.VarsIndex[metadataCharacteristicsCountPosition], &characteristicsCounter, 1 );
        //length
        const std::uint32_t metadataCharacteristicsLength = metadataSet.VarsIndexPosition - metadataCharacteristicsCountPosition - 4 - 1; //remove its own size and characteristic counter size
        std::memcpy( &metadataSet.VarsIndex[metadataCharacteristicsCountPosition+1], &metadataCharacteristicsLength, 4 );

        //Back to writing var entry length in Metadata
        const std::uint32_t metadataVarEntryLength = metadataSet.VarsIndexPosition - metadataVarLengthPosition - 4; //remove its own size
        std::memcpy( &metadataSet.VarsIndex[metadataVarLengthPosition], &metadataVarEntryLength, 4 );

        ++metadataSet.VarsCount;
        ++metadataSet.DataPGVarsCount;
    }


    /**
     * Expensive part this is only for heap buffers need to adapt to vector of capsules
     * @param variable
     * @param buffer
     */
    template< class T >
    void WriteVariablePayload( const Variable<T>& variable, capsule::STLVector& buffer, const unsigned int cores = 1 ) const noexcept
    {
        std::size_t payloadSize = variable.PayLoadSize(); //not using const due to memcpy inside Memcpythreads
        //EXPENSIVE part, might want to use threads if large, serial for now
        MemcpyThreads( &buffer.m_Data[buffer.m_DataPosition], variable.m_AppValues, payloadSize, cores );
        //update indices
        buffer.m_DataPosition += payloadSize;
        buffer.m_DataAbsolutePosition += payloadSize;
    }


    void Advance( BP1MetadataSet& metadataSet, capsule::STLVector& buffer );

    /**
     * Function that sets metadata (if first close) and writes to a single transport
     * @param metadataSet current rank metadata set
     * @param buffer contains data
     * @param transport does a write after data and metadata is setup
     * @param isFirstClose true: metadata has been set and aggregated
     * @param doAggregation true: for N-to-M, false: for N-to-N
     */
    void Close( BP1MetadataSet& metadataSet, capsule::STLVector& buffer, Transport& transport, bool& isFirstClose,
                const bool doAggregation ) const noexcept;


    /**
     * Writes the ADIOS log information (buffering, open, write and close) for a rank process
     * @param rank current rank
     * @param metadataSet contains Profile info for buffering
     * @param transports  contains Profile info for transport open, writes and close
     * @return string for this rank that will be aggregated into profiling.log
     */
    std::string GetRankProfilingLog( const int rank, const BP1MetadataSet& metadataSet,
                                     const std::vector< std::shared_ptr<Transport> >& transports ) const noexcept;

private:

    /**
     * Writes name record using a
     * @param name to be written
     * @param length number of characters in name
     * @param buffers to be written
     * @param positions to be moved
     */
    void WriteNameRecord( const std::string name, const std::uint16_t length,
                          std::vector<char>& buffer, std::size_t& position ) const noexcept;

    /**
     * Write a dimension record for a global variable used by WriteVariableCommon
     * @param buffer
     * @param position
     * @param localDimensions
     * @param globalDimensions
     * @param globalOffsets
     * @param addType true: for data buffers, false: for metadata buffer and data characteristic
     */
    void WriteDimensionRecord( std::vector<char>& buffer, std::size_t& position,
                               const std::vector<std::size_t>& localDimensions,
                               const std::vector<std::size_t>& globalDimensions,
                               const std::vector<std::size_t>& globalOffsets,
                               const bool addType = false ) const noexcept;

    /**
     * Write a dimension record for a local variable used by WriteVariableCommon
     * @param buffer
     * @param position
     * @param localDimensions
     * @param skip
     * @param addType true: for data buffers, false: for metadata buffer and data characteristic
     */
    void WriteDimensionRecord( std::vector<char>& buffer, std::size_t& position,
                               const std::vector<std::size_t>& localDimensions,
                               const unsigned int skip,
                               const bool addType = false ) const noexcept;

    /**
     * Function that writes min and max into data and metadata, called from WriteVariableIndex common.
     * Will be specialized for complex types, this is the version for primitive types
     * @param variable
     * @param dataBuffers
     * @param dataPositions
     * @param metadataBuffers
     * @param metadataPositions
     */
    template<class T> inline
    void WriteMinMax( const Variable<T>& variable, capsule::STLVector& buffer, BP1MetadataSet& metadataSet ) const noexcept
    {
        T min, max;
        const std::size_t valuesSize = variable.TotalSize();
        if( valuesSize >= 10000000 ) //ten million? this needs actual results //here we can make decisions for threads based on valuesSize
            GetMinMax( variable.m_AppValues, valuesSize, min, max, m_Cores ); //here we can add cores from constructor
        else
            GetMinMax( variable.m_AppValues, valuesSize, min, max );

        WriteMinMaxValues( min, max, buffer, metadataSet );
    }


    /**
     * Common part of WriteMinMax specialized templates. Writes to buffers after min and max are calculated.
     */
    template<class T>
    void WriteMinMaxValues( const T min, const T max, capsule::STLVector& buffer, BP1MetadataSet& metadataSet ) const noexcept
    {
        constexpr std::int8_t characteristicMinID = characteristic_min;
        constexpr std::int8_t characteristicMaxID = characteristic_max;

        WriteValueRecord( characteristicMinID, min, metadataSet.VarsIndex, metadataSet.VarsIndexPosition );
        WriteValueRecord( characteristicMaxID, max, metadataSet.VarsIndex, metadataSet.VarsIndexPosition );
        WriteValueRecord( characteristicMinID, min, buffer.m_Data, buffer.m_DataPosition, true ); //true: addLength in between for data
        WriteValueRecord( characteristicMaxID, max, buffer.m_Data, buffer.m_DataPosition, true ); //true: addLength in between for data
    }


    /**
     * Write a statistics record to buffer
     * @param id
     * @param value
     * @param buffers
     * @param positions
     * @param addLength true for data, false for metadata
     */
    template<class T>
    void WriteValueRecord( const std::uint8_t& characteristicID, const T& value,
                           std::vector<char>& buffer, std::size_t& position,
                           const bool addLength = false ) const noexcept
    {
        MemcpyToBuffer( buffer, position, &characteristicID, 1 );

        if( addLength == true )
        {
            const std::uint16_t lengthCharacteristic = 1 + sizeof( T ); //id
            MemcpyToBuffer( buffer, position, &lengthCharacteristic, 2 );
        }

        MemcpyToBuffer( buffer, position, &value, sizeof(T) );
    }

    /**
     * Flattens the data and fills the pg length, vars count, vars length and attributes
     * @param metadataSet
     * @param buffer
     */
    void FlattenData( BP1MetadataSet& metadataSet, capsule::STLVector& buffer ) const noexcept;

    /**
     * Flattens the metadata indices into a single metadata buffer in capsule
     * @param metadataSet
     * @param buffer
     */
    void FlattenMetadata( BP1MetadataSet& metadataSet, capsule::STLVector& buffer ) const noexcept; ///< sets the metadata buffer in capsule with indices and minifooter

};



/**
 * Specialized version of WriteMinMax for std::complex<float>
 * @param variable
 * @param dataBuffers
 * @param dataPositions
 * @param metadataBuffers
 * @param metadataPositions
 */
template<> inline
void BP1Writer::WriteMinMax<std::complex<float>>( const Variable<std::complex<float>>& variable, capsule::STLVector& buffer,
                                                  BP1MetadataSet& metadataSet ) const noexcept
{
    float min, max;
    const std::size_t valuesSize = variable.TotalSize();
    if( valuesSize >= 10000000 ) //ten million? this needs actual results //here we can make decisions for threads based on valuesSize
        GetMinMax( variable.m_AppValues, valuesSize, min, max, m_Cores ); //here we can add cores from constructor
    else
        GetMinMax( variable.m_AppValues, valuesSize, min, max );

    WriteMinMaxValues( min, max, buffer, metadataSet );
}


template<> inline
void BP1Writer::WriteMinMax<std::complex<double>>( const Variable<std::complex<double>>& variable, capsule::STLVector& buffer,
                                                   BP1MetadataSet& metadataSet ) const noexcept
{
    double min, max;
    const std::size_t valuesSize = variable.TotalSize();
    if( valuesSize >= 10000000 ) //ten million? this needs actual results //here we can make decisions for threads based on valuesSize
        GetMinMax( variable.m_AppValues, valuesSize, min, max, m_Cores ); //here we can add cores from constructor
    else
        GetMinMax( variable.m_AppValues, valuesSize, min, max );

    WriteMinMaxValues( min, max, buffer, metadataSet );
}


template<> inline
void BP1Writer::WriteMinMax<std::complex<long double>>( const Variable<std::complex<long double>>& variable,
                                                        capsule::STLVector& buffer,
                                                        BP1MetadataSet& metadataSet ) const noexcept
{
    long double min, max;
    const std::size_t valuesSize = variable.TotalSize();
    if( valuesSize >= 10000000 ) //ten million? this needs actual results //here we can make decisions for threads based on valuesSize
        GetMinMax( variable.m_AppValues, valuesSize, min, max, m_Cores ); //here we can add cores from constructor
    else
        GetMinMax( variable.m_AppValues, valuesSize, min, max );

    WriteMinMaxValues( min, max, buffer, metadataSet );
}




} //end namespace format
} //end namespace adios

#endif /* BP1WRITER_H_ */
