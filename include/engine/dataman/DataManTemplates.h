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
#include "core/Capsule.h"
#include "core/Transport.h"


namespace adios
{

template<class T>
void DataManWriteVariable( Group& group, const std::string variableName, Variable<T>& variable, std::vector<std::shared_ptr<Capsule> >& capsules,
		                   std::vector<std::shared_ptr<Transport> >& transports )
{
    //here write your magic, this template replaces MACRO
	std::cout << "Hello from DataMan Write variable " << variableName << "\n";
}



} //end namespace



#endif /* DATAMANTEMPLATES_H_ */
