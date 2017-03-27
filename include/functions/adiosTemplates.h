/*
 * adiosTemplates.h
 *
 *  Created on: Jan 26, 2017
 *      Author: wfg
 */

#ifndef ADIOSTEMPLATES_H_
#define ADIOSTEMPLATES_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <cstring> //std::memcpy
#include <vector>
#include <thread>
#include <set>
#include <complex>
#include <cmath> //std::sqrt
#include <iostream>
/// \endcond


namespace adios
{
/**
 * Get the primitive type in a string from a template
 * @return if T is a char, returns string = "char"
 */
template<class T> inline std::string GetType( ) noexcept { return "compound"; }
template<> inline std::string GetType<void>() noexcept { return "unknown"; }
template<> inline std::string GetType<char>() noexcept { return "char"; }
template<> inline std::string GetType<unsigned char>() noexcept { return "unsigned char"; }
template<> inline std::string GetType<short>() noexcept { return "short"; }
template<> inline std::string GetType<unsigned short>() noexcept { return "unsigned short"; }
template<> inline std::string GetType<int>() noexcept { return "int"; }
template<> inline std::string GetType<unsigned int>() noexcept { return "unsigned int"; }
template<> inline std::string GetType<long int>() noexcept { return "long int"; }
template<> inline std::string GetType<unsigned long int>() noexcept { return "unsigned long int"; }
template<> inline std::string GetType<long long int>() noexcept { return "long long int"; }
template<> inline std::string GetType<unsigned long long int>() noexcept { return "unsigned long long int"; }
template<> inline std::string GetType<float>() noexcept { return "float"; }
template<> inline std::string GetType<double>() noexcept { return "double"; }
template<> inline std::string GetType<long double>() noexcept { return "long double"; }
template<> inline std::string GetType<std::complex<float>>() noexcept { return "float complex"; }
template<> inline std::string GetType<std::complex<double>>() noexcept { return "double complex"; }
template<> inline std::string GetType<std::complex<long double>>() noexcept { return "long double complex"; }



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
	if( type == GetType<T>() ) //most of the time we will pass the same type, which is a key in aliases
		return true;

	bool isAlias = false;
	if( aliases.at( GetType<T>() ).count( type ) == 1 )
	    isAlias = true;

	return isAlias;
}


/**
 * Get the minimum and maximum values in one loop
 * @param values array of primitives
 * @param size of the values array
 * @param min from values
 * @param max from values
 * @param cores threaded version not yet implemented
 */
template<class T> inline
void GetMinMax( const T* values, const std::size_t size, T& min, T& max, const unsigned int cores = 1 ) noexcept
{
    min = values[0];
    max = min;

    for( std::size_t i = 1; i < size; ++i )
    {
        if( values[i] < min )
        {
            min = values[i];
            continue;
        }

        if( values[i] > max  )
            max = values[i];
    }
}

/**
 * Overloaded version for complex types, gets the "doughnut" range between min and max modulus
 * @param values array of complex numbers
 * @param size of the values array
 * @param min modulus from values
 * @param max modulus from values
 * @param cores
 */
template<class T> inline
void GetMinMax( const std::complex<T>* values, const std::size_t size, T& min, T& max, const unsigned int cores = 1 ) noexcept
{

    min = std::norm( values[0] );
    max = min;

    for( std::size_t i = 1; i < size; ++i )
    {
        T norm = std::norm( values[i] );

        if( norm < min )
        {
            min = norm;
            continue;
        }

        if( norm > max )
        {
            max = norm;
        }
    }

    min = std::sqrt( min );
    max = std::sqrt( max );
}

/**
 * threaded version of std::memcpy
 * @param dest
 * @param source
 * @param count total number of bytest (as in memcpy)
 * @param cores
 */
template<class T, class U>
void MemcpyThreads( T* destination, const U* source, std::size_t count, const unsigned int cores = 1 )
{
    if( cores == 1 )
    {
        std::memcpy( destination, source, count );
        return;
    }

    const std::size_t stride =  count/cores;
    const std::size_t remainder = count % cores;
    const std::size_t last = stride + remainder;

    std::vector<std::thread> memcpyThreads;
    memcpyThreads.reserve( cores );

    for( unsigned int core = 0; core < cores; ++core )
    {
        const size_t initialDestination = stride * core / sizeof(T);
        const size_t initialSource = stride * core / sizeof(U);

        if( core == cores-1 )
            memcpyThreads.push_back( std::thread( std::memcpy, &destination[initialDestination], &source[initialSource], last ) );
        else
            memcpyThreads.push_back( std::thread( std::memcpy, &destination[initialDestination], &source[initialSource], stride ) );
    }
    //Now join the threads (is this really needed?)
    for( auto& thread : memcpyThreads )
        thread.join( );
}


template< class T >
void MemcpyToBuffer( std::vector<char>& raw, std::size_t& position, const T* source, std::size_t size ) noexcept
{
    std::memcpy( &raw[position], source, size );
    position += size;
}



/**
 * Version that pushed to the end of the buffer, updates vec.size() automatically
 * @param raw
 * @param source using pointer notation
 * @param elements
 */
template<class T>
void CopyToBuffer( std::vector<char>& buffer, const T* source, const std::size_t elements = 1 ) noexcept
{
    const char* src = reinterpret_cast<const char*>( source );
    buffer.insert( buffer.end(), src, src + elements*sizeof(T) );
}

/**
 * Overloaded version to copies data to a specific location in the buffer, doesn't update vec.size()
 * @param raw
 * @param position
 * @param source
 * @param elements
 */
template<class T>
void CopyToBuffer( std::vector<char>& buffer, const std::size_t position, const T* source, const std::size_t elements = 1 ) noexcept
{
    const char* src = reinterpret_cast<const char*>( source );
    std::copy( src, src + elements*sizeof(T), buffer.begin() + position );
}


template<class T>
void CopyFromBuffer( T* destination, std::size_t elements, const std::vector<char>& raw, std::size_t& position ) noexcept
{
    std::copy( raw.begin() + position, raw.begin() + position + sizeof(T)*elements, reinterpret_cast<char*>(destination) );
    position += elements*sizeof(T);
}


template< class T >
void PrintValues( const std::string name, const char* buffer, const std::size_t position, const std::size_t elements )
{
    std::vector<T> values( elements );
    std::memcpy( values.data(), &buffer[position], elements * sizeof(T) );

    std::cout << "Read " << name << "\n";
    for( const auto value : values )
        std::cout << value << " ";

    std::cout << "\n";
}


} //end namespace


#endif /* ADIOSTEMPLATES_H_ */
