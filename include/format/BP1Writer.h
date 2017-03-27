/*
 * BP1.h
 *
 *  Created on: Jan 24, 2017
 *      Author: wfg
 */

#ifndef BP1WRITER_H_
#define BP1WRITER_H_

/// \cond EXCLUDE_FROM_DOXYGEN
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
                                 capsule::STLVector& buffer, BP1MetadataSet& metadataSet ) const noexcept;


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
     * Version for primitive types (except std::complex<T>)
     * @param variable
     * @param heap
     * @param metadataSet
     */
    template<class T> inline
    void WriteVariableMetadata( const Variable<T>& variable, capsule::STLVector& heap, BP1MetadataSet& metadataSet ) const noexcept
    {
    	Stats<T> stats = GetStats( variable );
    	WriteVariableMetadataCommon( variable, stats, heap, metadataSet );
    }

    /**
     * Overloaded version for std::complex<T> variables
     * @param variable
     * @param heap
     * @param metadataSet
     */
    template<class T>
    void WriteVariableMetadata( const Variable<std::complex<T>>& variable, capsule::STLVector& heap, BP1MetadataSet& metadataSet ) const noexcept
    {
    	Stats<T> stats = GetStats( variable );
    	WriteVariableMetadataCommon( variable, stats, heap, metadataSet );
    }


    template< class T, class U>
    void WriteVariableMetadata( const Variable<T>& variable, const Stats<U>& stats, const bool isNew, BP1Index& index )
	{
    	auto& buffer = index.Metadata;
    	auto& position = index.Position;

    	if( isNew == true ) //write header if variable is new
    	{
            position += 4;
        	CopyToBuffer( buffer, position, &index.MemberID );
        	position += 2; //skip group name
        	WriteNameRecord( variable.m_Name, buffer, position );
        	position += 2; //skip path
        	const std::uint8_t dataType = GetDataType<T>(); //dataType
        	CopyToBuffer( buffer, position, &dataType );

        	//Characteristics Sets Count in Metadata
        	index.Count = 1;
	        CopyToBuffer( buffer, position, &index.Count );
    	}

    	//Characteristics sets:




	}



    template< class T >
    void WriteNewVariableMetadata( const Variable<T>& variable, capsule::STLVector& buffer, BP1Index& varIndex ) const noexcept
    {
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

    template< class T, class U >
    void WriteVariableMetadataCommon( const Variable<T>& variable, Stats<U>& stats,
    		                          capsule::STLVector& heap, BP1MetadataSet& metadataSet ) const noexcept
	{
    	bool isNew = true; //flag to check if variable is new
    	BP1Index& varIndex = GetBP1Index( variable.m_Name, metadataSet.VarsIndices, isNew );

    	stats.Offset = heap.m_DataAbsolutePosition;
    	WriteVariableMetadataInData( variable, stats, varIndex.MemberID, metadataSet.TimeStep, heap );
    	stats.PayloadOffset = heap.m_DataAbsolutePosition;

    	if( isNew == true )
    	{
    		//NewVariable
    	}
    	else
    	{
    		//Existing variables, just add to characteristics sets
    	}
	}


    template< class T, class U >
    void WriteVariableMetadataInData( const Variable<T>& variable, const Stats<U>& stats, const std::uint32_t memberID,
    								  const std::uint32_t timeStep, capsule::STLVector& heap )
    {
        auto& buffer = heap.m_Data;
        auto& position = heap.m_DataPosition;

        const std::size_t varLengthPosition = position; //capture initial position for variable length
        position += 8; //skip var length

        CopyToBuffer( buffer, position, &memberID ); //memberID
        WriteNameRecord( variable.m_Name, buffer, position ); //variable name
        position += 2; //skip path
        const std::uint8_t dataType = GetDataType<T>(); //dataType
        CopyToBuffer( buffer, position, &dataType );
        constexpr char no = 'n';  //isDimension
        CopyToBuffer( buffer, position, &no );

        //write variable dimensions
        const auto& localDimensions = variable.m_Dimensions;
        const auto& globalDimensions = variable.m_GlobalDimensions;
        const auto& globalOffsets = variable.m_GlobalOffsets;

        //write dimensions count and length in data
        const std::uint8_t dimensions = localDimensions.size();
        CopyToBuffer( buffer, position, &dimensions ); //count
        std::uint16_t dimensionsLength = 27 * dimensions; //27 is from 9 bytes for each: var y/n + local, var y/n + global dimension, var y/n + global offset, changed for characteristic
        CopyToBuffer( buffer, position, &dimensionsLength ); //length
        WriteDimensionsRecord( buffer, position, localDimensions, globalDimensions, globalOffsets, 18, true );

        //START CHARACTERISTICS
        const std::size_t characteristicsCountPosition = position; //very important to track as writer is going back to this position
        position += 5; //skip characteristics count(1) + length (4)
        std::uint8_t characteristicsCounter = 0;

        //DIMENSIONS
        std::uint8_t characteristicID = characteristic_dimensions;
        CopyToBuffer( buffer, position, &characteristicID );
        const std::int16_t lengthOfDimensionsCharacteristic = 24 * dimensions + 3; // 24 = 3 local, global, global offset x 8 bytes/each
        CopyToBuffer( buffer, position, &lengthOfDimensionsCharacteristic );
        CopyToBuffer( buffer, position, &dimensions ); //count
        dimensionsLength = 24 * dimensions;
        CopyToBuffer( buffer, position, &dimensionsLength ); //length
        WriteDimensionsRecord( buffer, position, localDimensions, globalDimensions, globalOffsets, 16 );
        ++characteristicsCounter;

        //VALUE for SCALAR or STAT min, max for ARRAY
        WriteStatsRecord( variable.m_IsScalar, stats, buffer, position, characteristicsCounter, true );
        //TIME INDEX
        WriteStatsValueRecord( characteristic_time_index, timeStep, buffer, position, characteristicsCounter, true );
        //END OF CHARACTERISTICS

        //Back to characteristics count and length
        std::size_t backPosition = characteristicsCountPosition;
        CopyToBuffer( buffer, backPosition, &characteristicsCounter ); //count
        const std::uint32_t characteristicsLength = position - characteristicsCountPosition - 4 - 1; //remove its own length (4 bytes) + characteristic counter ( 1 byte )
        CopyToBuffer( buffer, backPosition, &characteristicsLength ); //length

        //Back to varLength including payload size
        const std::uint64_t varLength = position - varLengthPosition + variable.PayLoadSize() - 8; //remove its own size
        backPosition = varLengthPosition;
        CopyToBuffer( buffer, backPosition, &varLength ); //length
        heap.m_DataAbsolutePosition += position - varLengthPosition; // update absolute position to be used as payload position
    }



    /**
     * Writes from &buffer[position]:  [2 bytes:string.length()][string.length(): string.c_str()]
     * @param name
     * @param buffer
     * @param position
     */
    void WriteNameRecord( const std::string name, std::vector<char>& buffer, std::size_t& position );


    /**
     * Write a dimension record for a global variable used by WriteVariableCommon
     * @param buffer
     * @param position
     * @param localDimensions
     * @param globalDimensions
     * @param globalOffsets
     * @param addType true: for data buffers, false: for metadata buffer and data characteristic
     */
    void WriteDimensionsRecord( std::vector<char>& buffer, std::size_t& position,
                                const std::vector<std::size_t>& localDimensions,
                                const std::vector<std::size_t>& globalDimensions,
                                const std::vector<std::size_t>& globalOffsets,
							    const unsigned int skip,
                                const bool addType = false ) const noexcept;

    /**
     * GetStats for primitive types except std::complex<T> types
     * @param variable
     * @return stats
     */
    template<class T>
	Stats<T> GetStats( const Variable<T>& variable ) const noexcept
	{
		Stats<T> stats;
		const std::size_t valuesSize = variable.TotalSize();

		if( m_Verbosity == 0 )
		{
			if( valuesSize >= 10000000 ) //ten million? this needs actual results //here we can make decisions for threads based on valuesSize
				GetMinMax( variable.m_AppValues, valuesSize, stats.Min, stats.Max, m_Cores ); //here we can add cores from constructor
			else
				GetMinMax( variable.m_AppValues, valuesSize, stats.Min, stats.Max );
		}
		return stats;
	}

    /**
     * GetStats for std::complex<T> types
     * @param variable
     * @return stats
     */
    template<class T>
	Stats<T> GetStats( const Variable<std::complex<T>>& variable ) const noexcept
	{
		Stats<T> stats;
		const std::size_t valuesSize = variable.TotalSize();

		if( m_Verbosity == 0 )
		{
			if( valuesSize >= 10000000 ) //ten million? this needs actual results //here we can make decisions for threads based on valuesSize
				GetMinMax( variable.m_AppValues, valuesSize, stats.Min, stats.Max, m_Cores ); //here we can add cores from constructor
			else
				GetMinMax( variable.m_AppValues, valuesSize, stats.Min, stats.Max );
		}
		return stats;
	}


    template< class T >
    void WriteStatsRecord( const bool isScalar, const Stats<T>& stats, std::vector<char>& buffer, std::size_t& position,
    		               std::uint8_t& characteristicsCounter, const bool addLength = false )
    {
    	if( isScalar == true )
    	{
    		WriteStatsValueRecord( characteristic_value, stats.min, buffer, position, characteristicsCounter, true ); //stats.min = stats.max = value
    		return;
    	}

    	if( m_Verbosity == 0 ) //default verbose
        {
        	WriteStatsValueRecord( characteristic_min, stats.Min, buffer, position, characteristicsCounter, true );
        	WriteStatsValueRecord( characteristic_max, stats.Max, buffer, position, characteristicsCounter, true );
        }
    }

    /**
     * Write a characteristic value record to buffer
     * @param id
     * @param value
     * @param buffers
     * @param positions
     * @param characvteristicsCounter to be updated by 1
     * @param addLength true for data, false for metadata
     */
    template<class T>
    void WriteStatsValueRecord( const std::uint8_t characteristicID, const T& value,
                                std::vector<char>& buffer, std::size_t& position,
						        std::uint8_t& characteristicsCounter,
                                const bool addLength = false ) const noexcept
    {
        const std::uint8_t id = characteristicID;
    	CopyToBuffer( buffer, position, &id );

        if( addLength == true )
        {
            const std::uint16_t lengthCharacteristic = sizeof( T ); //id
            CopyToBuffer( buffer, position, &lengthCharacteristic );
        }

        CopyToBuffer( buffer, position, &value );
        ++characteristicsCounter;
    }

    /**
     * Returns corresponding index of type BP1Index, if doesn't exists creates a new one.
     * Used for variables and attributes
     * @param name variable or attribute name to look for index
     * @param indices look up hash table of indices
     * @param isNew true: index is newly created, false: index already exists in indices
     * @return reference to BP1Index in indices
     */
    BP1Index& GetBP1Index( const std::string name, std::unordered_map<std::string, BP1Index>& indices, bool& isNew );

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




} //end namespace format
} //end namespace adios

#endif /* BP1WRITER_H_ */
