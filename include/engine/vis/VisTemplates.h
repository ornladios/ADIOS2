/*
 * DataManTemplates.h
 *
 *  Created on: Jan 18, 2017
 *      Author: wfg
 */

#ifndef VISTEMPLATES_H_
#define VISTEMPLATES_H_

#include <vector>
#include <iostream>


#include "core/Group.h"
#include "core/Variable.h"
#include "capsule/Heap.h"
#include "core/Transport.h"
#include "format/BP1Writer.h"


namespace adios
{

template<class T>
void VisReadVariable( Transport& transport, const char* rawDataBuffer, const Var variableName )
{
    //Here make decisions on what to do with a certain variable in a rawDataBuffer (BP Format?)
}


template<class T>
void VisReadVariables( Transport& transport, const char* rawDataBuffer, const std::vector<Var>& variableName )
{
    //Here make decisions on what to do with many variables in a rawDataBuffer (BP Format?)
}

template<class T>
void VisReadAllVariables( Transport& transport, const char* rawDataBuffer )
{
    //Here make decisions on what to do with all variables in a rawDataBuffer (BP Format?)
}



/**
 *
 * @param group variable owner
 * @param variableName string type
 * @param variable
 * @param buffer heap buffer to writer variables to for disk I/O
 * @param transports
 * @param bp1Writer
 */
template<class T>
void VisWriteVariable( const Group& group, const Var variableName, Variable<T>& variable,
                       std::vector< std::shared_ptr<Capsule> >& capsules,
                       std::vector< std::shared_ptr<Transport> >& transports,
                       format::BP1Writer& bp1Writer,
                       const int rank )

{
    //here write your magic, this template replaces C MACROS in ADIOS 1.0
    std::cout << "Hello from Vis engine, writing variable " << variableName << " of typeid(T).name() = " << typeid(T).name() << "\n";
    if( variable.IsDimension )
    {
        std::cout << "Which is a dimension variable\n";
    }

    auto localDimensions = group.GetDimensions( variable.DimensionsCSV );
    unsigned long long int totalDimensions = GetTotalSize( localDimensions );

    std::cout << "Values: ";
    for( unsigned int i = 0; i < totalDimensions; ++i )
    {
        std::cout << variable.Values[i] << " ";
    }
    std::cout << "\nfrom RANK = " << rank << "\n\n";


    // This is just pseudo-code telling potential scenarios the engine can execute
    //if( story = "simple task" )
    //{
             //Could be private functions
//           RunSimpleTask( group, variableName, variable );  // e.g. Write to POSIX file
//
    //}
//    else if( story == "simple task + VisIt" )
//    {
//            ///Could be private functions
//         RunSimpleTask( group, variableName, variable );  // e.g. Write to POSIX file
//         SendToVisIt( group, variableName, variable );
//
//
//    }
//    else if( story == "Send to Service Flow Velocity and Potential" )
//    {


//         if( m_IsPotentialWritten = true && m_IsVelocityWritten == true && m_IsSameGroup == true )
//         {
//              SendToService( );  will send a buffer
//         }
//         else
//         {
//              if( variableName == "velocity" )
//              {
//
//                    WriteVariableToBuffer( group, variableName, variable, ... );  ///here
//                    m_WrittenVariables.push_back( std::make_pair( group, variableName ) ); //  <Group*, variableName> keeps track of Written Variables
//                    m_IsVelocityWritten = true;
//              }
//              else if( variableName == "potential" )
//              {
//                    WriteVariableToBuffer( group, variableName, variable, ... );  ///here
//                    m_WrittenVariables.push_back( std::make_pair( group, variableName ) ); //  <Group*, variableName> keeps track of Written Variables
//                    m_IsPotentialWritten = true;
//              }
//         }
//
//    }
//
//
}


} //end namespace



#endif /* VISTEMPLATES_H_ */
