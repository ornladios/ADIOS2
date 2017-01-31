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

template< class T> inline
std::string GetType( ) noexcept
{ return ""; }

template<> inline
std::string GetType<char>() noexcept { return "char"; }

template<> inline
std::string GetType<unsigned char>() noexcept { return "unsigned char"; }

template<> inline
std::string GetType<short>() noexcept { return "short"; }

template<> inline
std::string GetType<unsigned short>() noexcept { return "unsigned short"; }

template<> inline
std::string GetType<int>() noexcept { return "int"; }

template<> inline
std::string GetType<unsigned int>() noexcept { return "unsigned int"; }

template<> inline
std::string GetType<long int>() noexcept { return "long int"; }

template<> inline
std::string GetType<unsigned long int>() noexcept { return "unsigned long int"; }

template<> inline
std::string GetType<float>() noexcept { return "float"; }

template<> inline
std::string GetType<double>() noexcept { return "double"; }

template<> inline
std::string GetType<long double>() noexcept { return "long double"; }


/**
 * Check in types set if "type" is one of the aliases for a certain type,
 * (e.g. if type = integer is an accepted alias for "int", returning true)
 * @param type input to be compared with an alias
 * @param aliases set containing aliases to a certain type, typically Support::DatatypesAliases from Support.h
 * @return true: is an alias, false: is not
 */
template<class T>
bool IsTypeAlias( const std::string type,
		          const std::map<std::string, std::set<std::string>>& aliases ) noexcept
{
	if( type == GetType<T>() ) //most of the time we will pass the same type
		return true;

	bool isAlias = false;
	if( aliases.at( GetType<T>() ).count( type ) == 1 )
	    isAlias = true;

	return isAlias;
}



} //end namespace


#endif /* ADIOSTEMPLATES_H_ */
