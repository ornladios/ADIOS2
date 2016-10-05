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
    const bool IsStatic; ///< true: static, defined in XML, false: dynamic
    const std::string Path; ///< hierarchical path inside the file for the attribute
    const std::string Value; ///< information about the attribute
    const std::string Type; ///< string or numeric type

    /**
     * @brief Unique constructor, variables must be extracted and passed before Object creation (all const)
     * @param isStatic passed to IsStatic
     * @param path passed to Path
     * @param value passed to Value
     * @param type passed to Type
     */
    SAttribute( const bool isStatic, const std::string path, const std::string value, const std::string type ):
        IsStatic( isStatic ),
        Path( path ),
        Value( value ),
        Type( type )
    { }
};


} //end namespace



#endif /* SATTRIBUTE_H_ */
