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
#include <memory>
/// \endcond

#include "core/Transform.h"



namespace adios
{

class ADIOS;

using Dims = std::vector<size_t>;

/**
 * @param Base (parent) class for template derived (child) class CVariable. Required to put CVariable objects in STL containers.
 */
template< class T >
class Variable
{

public:

    const ADIOS& m_ADIOS; ///< reference to the adios class it belongs to
    const std::string m_Name; ///< variable name
    const std::string m_Type; ///< variable type

    const Dims m_Dimensions;
    const std::string m_DimensionsCSV; ///< comma separated list for variables to search for local dimensions

    const Dims m_GlobalDimensions;
    const std::string m_GlobalDimensionsCSV; ///< comma separated list for variables to search for global dimensions

    const Dims m_GlobalOffsets;
    const std::string m_GlobalOffsetsCSV; ///< comma separated list for variables to search for global offsets

    const bool m_DebugMode = false;

    const T* m_ValuesWrite = nullptr; ///< pointer to values passed from user in ADIOS Write, it might change in ADIOS Read
    T* m_ValuesRead = nullptr;

    bool IsDimension = false; ///< true: is used as a dimension in another variable (typically scalars), false: none


    struct TransformData
    {
        Transform* Transform = nullptr; ///< pointer to transform object
        std::map<std::string, std::string> Parameters; ///< transforms parameters
        std::vector<std::size_t> Size;  ///< vector that carries the sizes after a transformation is applied
    };

    std::vector< TransformData > m_Transforms; ///< associated transforms, sequence determines application order, e.g. first Transforms[0] then Transforms[1]. Pointer used as reference (no memory management).

    /**
     * Constructor for dynamic config file
     * @param group
     * @param name
     * @param dimensionsCSV
     * @param globalBoundsIndex
     * @param debugMode
     */
    template< class T >
    Variable( const ADIOS& adios, const std::string name, const std::string dimensionsCSV, const std::string globalDimensionsCSV,
              const std::string globalOffsetsCSV, const bool debugMode ):
        m_ADIOS{ adios },
        m_Name{ name },
        m_DimensionsCSV{ dimensionsCSV },
        m_GlobalDimensionsCSV{ globalDimensionsCSV },
        m_GlobalOffsetsCSV{ globalOffsetsCSV },
        m_DebugMode{ debugMode }
    { }

    /**
     * Constructor for static config file
     * @param group
     * @param name
     * @param dimensionsCSV
     * @param globalBoundsIndex
     * @param debugMode
     */
    template< class T >
    Variable( const ADIOS& adios, const std::string name, const Dims dimensionsCSV, const Dims globalDimensionsCSV,
              const Dims globalOffsetsCSV, const bool debugMode ):
        m_ADIOS{ adios },
        m_Name{ name },
        m_DimensionsCSV{ dimensionsCSV },
        m_GlobalDimensionsCSV{ globalDimensionsCSV },
        m_GlobalOffsetsCSV{ globalOffsetsCSV },
        m_DebugMode{ debugMode }
    { }


    template< class ...Args>
    void AddTransform( Transform& transform, Args... args )
    {
        std::vector<std::string> parameters = { args... };
        m_Transforms.emplace_back( transform, BuildParametersMap( parameters, m_DebugMode ) ); //need to check
    }

};


} //end namespace
