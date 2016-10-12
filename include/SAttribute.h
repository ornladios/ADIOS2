/*
 * SAttribute.h
 *
 *  Created on: Oct 5, 2016
 *      Author: wfg
 */

#ifndef SATTRIBUTE_H_
#define SATTRIBUTE_H_


namespace adios
{

struct SAttribute
{
    std::string Name; ///< Attribute name
    bool IsGlobal; ///< true: static, defined in XML Config file, false: dynamic, defined in non-XML API
    std::string Type; ///< string or numeric type
    std::string Path; ///< Attribute name
    std::string Value; ///< information about the attribute

    /**
     * @brief Unique constructor
     * @param isGlobal passed to IsGlobal
     * @param name passed to Name
     * @param path passed to Path
     * @param value passed to Value
     * @param type passed to Type
     */
    SAttribute( const std::string name, const bool isGlobal, const std::string type, const std::string path, const std::string value ):
        Name( name ),
        IsGlobal( isGlobal ),
        Type( type ),
        Path( path ),
        Value( value )
    { }
};


} //end namespace



#endif /* SATTRIBUTE_H_ */
