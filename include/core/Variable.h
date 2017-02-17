/*
 * Variable.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef VARIABLE_H_
#define VARIABLE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
#include <map>
#include <ostream> //std::ostream in MonitorGroups
/// \endcond

#include "core/Transform.h"
#include "functions/adiosFunctions.h"
#include "functions/adiosTemplates.h"



namespace adios
{

using Dims = std::vector<size_t>;

struct TransformData
{
    Transform& Operation; ///< pointer to transform object
    std::map<std::string, std::string> Parameters; ///< transforms parameters
    std::vector<std::size_t> Size;  ///< vector that carries the sizes after a transformation is applied
};

/**
 * @param Base (parent) class for template derived (child) class CVariable. Required to put CVariable objects in STL containers.
 */
template< class T >
class Variable
{

public:

    const std::string m_Name; ///< variable name
    const std::string m_Type; ///< variable type

    Dims m_Dimensions;
    std::string m_DimensionsCSV; ///< comma separated list for variables to search for local dimensions

    Dims m_GlobalDimensions;
    std::string m_GlobalDimensionsCSV; ///< comma separated list for variables to search for global dimensions

    Dims m_GlobalOffsets;
    std::string m_GlobalOffsetsCSV; ///< comma separated list for variables to search for global offsets

    const bool m_DebugMode = false;

    const T* m_AppValues = nullptr; ///< pointer to values passed from user in ADIOS Write, it might change in ADIOS Read
    std::vector<T> m_Values; ///< Vector variable returned to user, might be used for zero-copy?

    bool m_IsScalar = false;
    const bool m_IsDimension = false;
    std::vector< TransformData > m_Transforms; ///< associated transforms, sequence determines application order, e.g. first Transforms[0] then Transforms[1]. Pointer used as reference (no memory management).

    Variable( const std::string name, const Dims dimensions, const Dims globalDimensions, const Dims globalOffsets, const bool debugMode ):
        m_Name{ name },
        m_Type{ GetType<T>() },
        m_Dimensions{ dimensions },
        m_GlobalDimensions{ globalDimensions },
        m_GlobalOffsets{ globalOffsets },
        m_DebugMode{ debugMode }
    {
        if( m_Dimensions == Dims{1} )
            m_IsScalar = true;
    }

    template< class ...Args>
    void AddTransform( Transform& transform, Args... args )
    {
        std::vector<std::string> parameters = { args... };
        m_Transforms.emplace_back( transform, BuildParametersMap( parameters, m_DebugMode ) ); //need to check
    }


    void Monitor( std::ostream& logInfo ) const noexcept
    {
        logInfo << "Variable: " << m_Name << "\n";
        logInfo << "Type: " << m_Type << "\n";
        logInfo << "Size: " << TotalSize() << " elements\n";
        logInfo << "Payload: " << PayLoadSize() << " bytes\n";

        if( m_AppValues != nullptr )
        {
            logInfo << "Values (first 10 or max_size): \n";
            std::size_t size = TotalSize();
            if( size > 10 )
                size = 10;

            for( std::size_t i = 0; i < size; ++i  )
            {
                logInfo << m_AppValues[i] << " ";
            }
            logInfo << " ...";
        }

        logInfo << "\n";
    }

    /**
     * Returns the payload size in bytes
     * @return TotalSize * sizeof(T)
     */
    std::size_t PayLoadSize( ) const noexcept
    {
        return GetTotalSize( m_Dimensions ) * sizeof(T);
    }

    /**
     * Returns the total size
     * @return number of elements
     */
    std::size_t TotalSize( ) const noexcept
    {
        return GetTotalSize( m_Dimensions );
    }

};


} //end namespace


#endif /* VARIABLE_H_ */
