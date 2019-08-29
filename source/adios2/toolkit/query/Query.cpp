#include "Query.h"
#include "adios2/helper/adiosFunctions.h" 
//#include <adios2.h>
#include "BlockIndex.h"

#include "Query.tcc"

namespace adios2 {
  namespace query {
    bool QueryComposite::AddNode(QueryBase* var)
    {
      if (adios2::query::Relation::NOT == m_Relation) {
	//if (m_Nodes.size() > 0) return false;
	// don't want to support NOT for composite queries
	return false;
      }
      m_Nodes.push_back(var);
      return true;
    }

    void QueryComposite::BlockIndexEvaluate(adios2::core::IO& io, 
					    adios2::core::Engine& reader, 
					    std::vector<Box<Dims>>& touchedBlocks)
    {
      std::cout<<" to do: QueryComposite::BlockIndexEvalute"<<std::endl;
      if (m_Nodes.size() == 0) return;		            
      // plan to shift all var results to regions start at 0, and find out the overlapped regions
      // boxes can be different size especially if they are from BP3
    }


    bool QueryVar::IsSelectionValid(adios2::Dims& shape) const
     {
       if (0 == m_Selection.first.size()) return true;
       
       if (m_Selection.first.size() != shape.size()) {
	 std::cerr<<"ERROR:  query selection dimension is different from shape dimension"<<std::endl;
	 return false; // different dimension
       }

       for (int i=0; i<shape.size(); i++) {
	 if ((m_Selection.first[i] > shape[i]) || (m_Selection.second[i] > shape[i]))
	   return false;
       }
       return true;	   
     }

    bool QueryVar::TouchSelection(adios2::Dims& start, adios2::Dims& count) const
     {
       if (0 == m_Selection.first.size()) return true;

       const size_t dimensionsSize = start.size();

       for (size_t i=0; i<dimensionsSize; i++) {
	 size_t end = start[i] + count[i];
	 size_t selEnd   = m_Selection.first[i] + m_Selection.second[i];
	 
	 if (end <= m_Selection.first[i])
	   return false;
	 if (selEnd <= start[i])
	   return false;
       }
       return true;
     }


    void QueryVar::BlockIndexEvaluate(adios2::core::IO& io, 
				      adios2::core::Engine& reader, 
				      std::vector<Box<Dims>>& touchedBlocks)
    {
      const std::string varType = io.InquireVariableType(m_VarName);

      //Variable<int> var = io.InquireVariable<int>(m_VarName);
      //BlockIndex<int> idx(io, reader);

	
      // var already exists when loading query. skipping validity checking
#define declare_type(T)                                                 \
      if (varType == adios2::helper::GetType<T>())			\
          {                                                             \
	    core::Variable<T>* var = io.InquireVariable<T>(m_VarName);	\
	    BlockIndex<T> idx(*var, io, reader);			\
	    idx.Evaluate(*this, touchedBlocks);				\
	  }								
      //ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type) //skip complex types
	ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type       
	
    }     
  } // namespace query
} // namespace adios2
