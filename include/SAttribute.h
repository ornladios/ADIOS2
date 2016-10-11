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
    bool IsStatic; ///< true: static, defined in XML Config file, false: dynamic, defined in non-XML API
    std::string Name; ///< Attribute name
    std::string Path; ///< Attribute name
    std::string Value; ///< information about the attribute
    std::string Type; ///< string or numeric type

    /**
     * @brief Unique constructor, variables must be extracted and passed before Object creation
     * @param isStatic passed to IsStatic
     * @param name passed to Name
     * @param path passed to Path
     * @param value passed to Value
     * @param type passed to Type
     */
    SAttribute( const bool isStatic, const std::string name, const std::string path, const std::string value, const std::string type ):
        IsStatic( isStatic ),
        Name( name ),
        Path( path ),
        Value( value ),
        Type( type )
    { }
};


} //end namespace



#endif /* SATTRIBUTE_H_ */
