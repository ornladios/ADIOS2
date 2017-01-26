/*
 * adiosTemplates.h
 *
 *  Created on: Jan 26, 2017
 *      Author: wfg
 */

#ifndef ADIOSTEMPLATES_H_
#define ADIOSTEMPLATES_H_


namespace adios
{

template< class T >
std::string GetType( ) noexcept
{
    std::string type;

    if( std::is_same<T,char>::value )
        type = "char";

    else if( std::is_same<T,short>::value )
        type = "short";

    else if( std::is_same<T,int>::value )
        type = "int";

    else if( std::is_same<T,long int>::value )
        type = "long int";

    else if( std::is_same<T,unsigned char>::value )
        type = "unsigned char";

    else if( std::is_same<T,unsigned short>::value )
        type = "unsigned short";

    else if( std::is_same<T,unsigned int>::value )
        type = "unsigned int";

    else if( std::is_same<T,unsigned long int>::value )
        type = "unsigned long int";

    else if( std::is_same<T,float>::value )
        type = "float";

    else if( std::is_same<T,double>::value )
        type = "double";

    else if( std::is_same<T,long double>::value )
        type = "long double";

    return type;
}



} //end namespace


#endif /* ADIOSTEMPLATES_H_ */
