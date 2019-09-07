#include "Worker.h"
#include "XmlUtil.h"

namespace adios2
{
namespace query
{
void XmlWorker::ParseMe()
{
    // std::cerr << "TODO: ...  will ... build index file if indicated " <<
    // std::endl;

    auto lf_FileContents = [&](const std::string &configXML) -> std::string {
        std::ifstream fileStream(configXML);
        if (!fileStream)
            throw std::ios_base::failure("ERROR: file " + configXML +
                                         " not found. ");

        std::ostringstream fileSS;
        fileSS << fileStream.rdbuf();
        fileStream.close();

        if (fileSS.str().empty())
            throw std::invalid_argument("ERROR: config xml file is empty.");

        return fileSS.str();
    }; // local  function lf_FileContents

    const std::string fileContents = lf_FileContents(m_QueryFile);
    const pugi::xml_document document =
        adios2::query::XmlUtil::XMLDocument(fileContents);

    const pugi::xml_node config =
        adios2::query::XmlUtil::XMLNode("adios-query", document, true);

    const pugi::xml_node ioNode = config.child("io");
    ParseIONode(ioNode);

    // for (const pugi::xml_node &ioNode : config.children("io"))
} // Parse()

void XmlWorker::ParseIONode(const pugi::xml_node &ioNode)
{
    auto lf_GetParametersXML =
        [&](const pugi::xml_node &node) -> adios2::Params {
        const std::string errorMessage("in node " + std::string(node.value()));

        adios2::Params parameters;

        for (const pugi::xml_node paramNode : node.children("parameter"))
        {
            const pugi::xml_attribute key =
                adios2::query::XmlUtil::XMLAttribute("key", paramNode);
            const pugi::xml_attribute value =
                adios2::query::XmlUtil::XMLAttribute("value", paramNode);
            parameters.emplace(key.value(), value.value());
        }
        return parameters;
    }; // local function lf_GetParamtersXML

#ifdef PARSE_IO
    const pugi::xml_attribute ioName =
        adios2::query::XmlUtil::XMLAttribute("name", ioNode);
    const pugi::xml_attribute fileName =
        adios2::query::XmlUtil::XMLAttribute("file", ioNode);

    // must be unique per io
    const pugi::xml_node &engineNode =
        adios2::query::XmlUtil::XMLNode("engine", ioNode, false, true);
    // adios2::ADIOS adios(m_Comm, adios2::DebugON);
    // adios2::IO currIO = m_Adios2.DeclareIO(ioName.value());
    m_IO = &(m_Adios2.DeclareIO(ioName.value()));

    if (engineNode)
    {
        const pugi::xml_attribute type =
            adios2::query::XmlUtil::XMLAttribute("type", engineNode);
        m_IO->SetEngine(type.value());

        const adios2::Params parameters = lf_GetParametersXML(engineNode);
        m_IO->SetParameters(parameters);
    }
    else
    {
        m_IO->SetEngine("BPFile");
    }
    // adios2::Engine reader =  currIO.Open(fileName.value(),
    // adios2::Mode::Read, m_Comm);
    m_SourceReader =
        &(m_IO->Open(fileName.value(), adios2::Mode::Read, m_Comm));
#else
    const pugi::xml_attribute ioName =
        adios2::query::XmlUtil::XMLAttribute("name", ioNode);
    if (m_SourceReader->m_IO.m_Name.compare(ioName.value()) != 0)
        throw std::ios_base::failure("invalid query io. Expecting io name = " +
                                     m_SourceReader->m_IO.m_Name +
                                     " found:" + ioName.value());
#endif
    // std::cout<<m_SourceReader.Type()<<std::endl;

    std::map<std::string, QueryBase *> subqueries;

    adios2::Box<adios2::Dims> ref;
    for (const pugi::xml_node &qTagNode : ioNode.children("tag"))
    {
        const pugi::xml_attribute name =
            adios2::query::XmlUtil::XMLAttribute("name", qTagNode);
        const pugi::xml_node &variable = qTagNode.child("var");
        QueryVar *q =
            ParseVarNode(variable, m_SourceReader->m_IO, *m_SourceReader);
        if (ref.first.size() == 0)
            ref = q->m_Selection;
        else if (!q->IsCompatible(ref))
            throw std::ios_base::failure("impactible query found on var:" +
                                         q->GetVarName());
        // std::cout<<" found sub query for: "<<tag.value()<<std::endl;
        subqueries[name.value()] = q;
    }

    const pugi::xml_node &qNode = ioNode.child("query");
    if (qNode == nullptr)
    {
        const pugi::xml_node &variable = ioNode.child("var");
        m_Query = ParseVarNode(variable, m_SourceReader->m_IO, *m_SourceReader);
    }
    else
    {
        const pugi::xml_attribute op =
            adios2::query::XmlUtil::XMLAttribute("op", qNode);
        QueryComposite *q =
            new QueryComposite(adios2::query::strToRelation(op.value()));
        for (const pugi::xml_node &sub : qNode.children())
        {
            // std::cout<<" .. "<<sub.name()<<std::endl;
            q->AddNode(subqueries[sub.name()]);
        }
        m_Query = q;
        // m_Query->AddNode();
    }

    // if (m_Query) 	    m_Query->Print();
} // parse_io_node

// node is the variable node
QueryVar *XmlWorker::ParseVarNode(const pugi::xml_node &node,
                                  adios2::core::IO &currentIO,
                                  adios2::core::Engine &reader)

{
    const std::string variableName =
        std::string(adios2::query::XmlUtil::XMLAttribute("name", node).value());

    // const std::string varType = currentIO.VariableType(variableName);
    const std::string varType = currentIO.InquireVariableType(variableName);
    if (varType.size() == 0)
    {
        std::cerr << "No such variable: " << variableName << std::endl;
        return nullptr;
    }
#define declare_type(T)                                                        \
    if (varType == helper::GetType<T>())                                       \
    {                                                                          \
        core::Variable<T> *var = currentIO.InquireVariable<T>(variableName);   \
        if (var)                                                               \
        {                                                                      \
            QueryVar *q = new QueryVar(variableName);                          \
            adios2::Dims zero(var->Shape().size(), 0);                         \
            adios2::Dims shape = var->Shape();                                 \
            q->SetSelection(zero, shape);                                      \
            ConstructQuery(*q, node);                                          \
            return q;                                                          \
        }                                                                      \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
    return nullptr;
} //  parse_var_node

void XmlWorker::ConstructTree(RangeTree &host, const pugi::xml_node &node)
{
    std::string relationStr =
        adios2::query::XmlUtil::XMLAttribute("value", node).value();
    host.SetRelation(adios2::query::strToRelation(relationStr));
    for (const pugi::xml_node rangeNode : node.children("range"))
    {
        std::string valStr =
            adios2::query::XmlUtil::XMLAttribute("value", rangeNode).value();
        std::string opStr =
            adios2::query::XmlUtil::XMLAttribute("compare", rangeNode).value();
        /*
        std::stringstream convert(valStr);
        T val;
        convert >> val;
        */
        host.AddLeaf(adios2::query::strToQueryOp(opStr), valStr);
    }

    for (const pugi::xml_node opNode : node.children("op"))
    {
        adios2::query::RangeTree subNode;
        ConstructTree(subNode, opNode);
        host.AddNode(subNode);
    }
}

void XmlWorker::ConstructQuery(QueryVar &simpleQ, const pugi::xml_node &node)
{
    // QueryVar* simpleQ = new QueryVar(variableName);
    pugi::xml_node bbNode = node.child("boundingbox");

    if (bbNode)
    {
        adios2::Box<adios2::Dims> box =
            adios2::Box<adios2::Dims>({100, 100}, {200, 200});
        std::string startStr =
            adios2::query::XmlUtil::XMLAttribute("start", bbNode).value();
        std::string countStr =
            adios2::query::XmlUtil::XMLAttribute("count", bbNode).value();

        adios2::Dims start = split(startStr, ',');
        adios2::Dims count = split(countStr, ',');

        if (start.size() != count.size())
        {
            throw std::ios_base::failure(
                "dim of startcount does match in the bounding box definition");
        }

        // simpleQ.setSelection(box.first, box.second);
        adios2::Dims shape =
            simpleQ.m_Selection.second; // set at the creation for default
        simpleQ.SetSelection(start, count);
        if (!simpleQ.IsSelectionValid(shape))
            throw std::ios_base::failure(
                "invalid selections for selection of var: " +
                simpleQ.GetVarName());
    }

#ifdef NEVER // don't know whether this is useful.
    pugi::xml_node tsNode = node.child("tstep");
    if (tsNode)
    {
        std::string startStr =
            adios2::query::XmlUtil::XMLAttribute("start", tsNode).value();
        std::string countStr =
            adios2::query::XmlUtil::XMLAttribute("count", tsNode).value();

        if ((startStr.size() > 0) && (countStr.size() > 0))
        {
            std::stringstream ss(startStr), cc(countStr);
            ss >> simpleQ.m_TimestepStart;
            cc >> simpleQ.m_TimestepCount;
        }
    }
#endif
    pugi::xml_node relationNode = node.child("op");
    ConstructTree(simpleQ.m_RangeTree, relationNode);
}

} // namespace query
} // namespace adios2
