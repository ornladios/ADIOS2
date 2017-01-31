/*
 * DataManTemplates.h
 *
 *  Created on: Jan 18, 2017
 *      Author: wfg
 */

#ifndef DATAMANTEMPLATES_H_
#define DATAMANTEMPLATES_H_

#include <vector>
#include <iostream>


#include "core/Group.h"
#include "core/Variable.h"
#include "capsule/Heap.h"
#include "core/Transport.h"
#include "format/BP1Writer.h"


namespace adios
{

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
void DataManWriteVariable( const Group& group, const Var variableName, Variable<T>& variable,
                           Heap& buffer, std::vector< std::shared_ptr<Transport> >& transports,
                           format::BP1Writer& bp1Writer )

{
    //here write your magic, this template replaces C MACROS
    std::cout << "Hello from DataMan, writing variable " << variableName << " of typeid(T).name() = " << typeid(T).name() << "\n";
    if( variable.IsDimension )
    {
        std::cout << "Which is a dimension variable\n";
    }
}



} //end namespace



#endif /* DATAMANTEMPLATES_H_ */
