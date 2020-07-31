//
// Created by ganyush on 7/31/20.
//
//Group g   =  io("/a", separator="/");
//g.AvailableGroups();    // [b,f] groups directly under /a/*, not recursive
//g.AvailableVariables();    // [x] variables directly under /a/*, not recursive
//g.AvailableAttributes();   // not recursive
//
//Group gb = g.InquireGroup("b");
//Variable<T> v = g.InquireVariable("x");
//Variable<T> v = g.InquireVariable("b/c"); // NOT ALLOWED
//
//Attribute<T> a = g.InquireAttribute("attr1");
//
//std::string VariableType(const std::string &name) const;
//std::string AttributeType(const std::string &name) const;


// Write data first
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>
#if ADIOS2_USE_MPI
#include <mpi.h>
#endif

std::vector<std::string> split(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

class Group
{
private:
    std::string current_path;
    std::vector<std::string> variables;
    std::vector<std::string> groups;
    std::vector<std::string> attributes;

public:
    Group( ){
    };
    ~Group(){};

    void DefineGroup(std::string);
    void DefineVariable(const std::string);
    void DefineAttribute(const std::string);
    void ClassifyRecord(const std::string);
    std::vector<std::string> AvailableGroups();    // [b,f] groups directly under /a/*, not recursive
    std::vector<std::string> AvailableVariables();    // [x] variables directly under /a/*, not recursive
    std::vector<std::string> AvailableAttributes();
    std::string InquirePath(){
        return current_path;
    }
    void setPath(std::string path){
        current_path = path;
    }
};



void Group::DefineGroup(std::string groupName) {
    groups.push_back(groupName);
    return;
}

void Group::DefineVariable(const std::string variable_name) {

    variables.push_back(variable_name);
    return;
}

void Group::DefineAttribute(const std::string attribute_name) {

    attributes.push_back(attribute_name);
    return;
}

std::vector<std::string> Group::AvailableGroups(){
    std::vector<std::string> available_groups;
    for (std::string  s : groups ){
        if (s.find(current_path) == 0)  available_groups.push_back(s);
    }
    return available_groups;
}
std::vector<std::string> Group::AvailableVariables(){
    //simple linear search
    std::vector<std::string> available_variables;
    for (std::string  s :  variables){
        if (s.find(current_path) == 0)  available_variables.push_back(s);
    }
    return available_variables;
}
std::vector<std::string> Group::AvailableAttributes(){
    //simple linear search
    std::vector<std::string> available_attributes;
    for (std::string  s :  attributes){
        if (s.find(current_path) == 0)  available_attributes.push_back(s);
    }
    return available_attributes;
}

void Group::ClassifyRecord(const std::string record) {
    //Classify if is a dataset of an attribute
    std::vector<std::string> tokens = split(record, '/');
    std::string suffix = tokens.back();
    if ( suffix.compare("__data__") == 0){
        tokens.pop_back();
        std::string variableName;
        for (auto t : tokens) variableName += t;
        variables.push_back(variableName);
    }else {
        attributes.push_back(record);
    }
    //do we need to set up groups?
}


int main(int argc, char *argv[])
{
    int rank, size;
#if ADIOS2_USE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#else
    rank = 0;
    size = 1;
#endif
    const int number_of_steps = 3;
    /** Application variable */
    std::string filename = "myVector_cpp.bp";
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> myInts = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9};
    const std::size_t Nx = myFloats.size();

    const std::string myString("Hello Variable String from rank " +
                               std::to_string(rank));

    try
    {
        /** ADIOS class factory of IO class objects */
#if ADIOS2_USE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD);
#else
        adios2::ADIOS adios;
#endif

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO bpIO = adios.DeclareIO("BPFile_N2N");
        /** Engine derived class, spawned to start IO operations */
        adios2::Engine bpFileWriter = bpIO.Open(filename, adios2::Mode::Append);

        /** global array : name, { shape (total) }, { start (local) }, {
         * count
         * (local) }, all are constant dimensions */

        adios2::Variable<float> bpFloats = bpIO.DefineVariable<float>(
            "/group1/subgroup1/bpFloats/__data__", {size * Nx}, {rank * Nx}, {Nx},
            adios2::ConstantDims);
        adios2::Variable<int> bpInts =
            bpIO.DefineVariable<int>("/group1/subgroup1/bpInts/__data__", {size * Nx},
                                     {rank * Nx}, {Nx}, adios2::ConstantDims);


        std::string groupAttributeStr ="group attribute";
        std::string variableAttributeStr ="variable attribute";
        //string variables does not work
        //adios2::Variable<std::string > groupAttribute =
        //    bpIO.DefineVariable<std::string>("/group1/subgroup1/bpInts/groupAttribute");
        adios2::Variable<uint32_t> groupAttribute = bpIO.DefineVariable<uint32_t>("/group1/subgroup1/groupAttribute");

        //adios2::Variable<std::string> variableAttribute =
        //    bpIO.DefineVariable<std::string>("/group1/subgroup1/bpInts/variableAttribute");
        adios2::Variable<uint32_t> variableAttribute = bpIO.DefineVariable<uint32_t>("/group1/subgroup1/bpInts/variableAttribute");


        for (int i = 0; i < number_of_steps; i++)
        {
            bpFileWriter.BeginStep();

            /** Put variables for buffering, template type is optional */

            bpFileWriter.Put<float>(bpFloats, myFloats.data());

            bpFileWriter.Put(bpInts, myInts.data());

           // bpFileWriter.Put(groupAttribute, groupAttributeStr);
           // bpFileWriter.Put(variableAttribute, variableAttributeStr);

            uint32_t n = 1;
            bpFileWriter.Put(groupAttribute, n);
            n = 2;
            bpFileWriter.Put(variableAttribute, n);

            bpFileWriter.EndStep();
        }
        /** Create bp file, engine becomes unreachable after this*/
        bpFileWriter.Close();
        if (rank == 0)
        {
            std::cout << "Wrote file " << filename
                      << " to disk.  " << std::endl;
        }
    }
    catch (std::invalid_argument &e)
    {
        std::cerr << "Invalid argument exception: " << e.what() << "\n";
#if ADIOS2_USE_MPI
        std::cerr << "STOPPING PROGRAM from rank " << rank << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }
    catch (std::ios_base::failure &e)
    {
        std::cerr << "IO System base failure exception: " << e.what() << "\n";
#if ADIOS2_USE_MPI
        std::cerr << "STOPPING PROGRAM from rank " << rank << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
#if ADIOS2_USE_MPI
        std::cerr << "STOPPING PROGRAM from rank " << rank << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }

    try
    {
        /** ADIOS class factory of IO class objects */
        // adios2::ADIOS adios(MPI_COMM_WORLD);
#if ADIOS2_USE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD);
#else
        adios2::ADIOS adios;
#endif

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO bpIO = adios.DeclareIO("ReadBP");

        const auto attributesInfo = bpIO.AvailableAttributes();

        for (const auto &attributeInfoPair : attributesInfo)
        {
            std::cout << "Attribute: " << attributeInfoPair.first;
            for (const auto &attributePair : attributeInfoPair.second)
            {
                std::cout << "\tKey: " << attributePair.first
                          << "\tValue: " << attributePair.second << "\n";
            }
            std::cout << "\n";
        }

        /** Engine derived class, spawned to start IO operations */
        adios2::Engine bpReader = bpIO.Open(filename, adios2::Mode::Read);

        Group g = Group();
        for (int step = 0; step < number_of_steps; step++)
        {
            bpReader.BeginStep();
            const std::map<std::string, adios2::Params> variables =
                bpIO.AvailableVariables();
            for (const auto variablePair : variables)
            {
                std::cout << "Name: " << variablePair.first;
                g.ClassifyRecord(variablePair.first);

                for (const auto &parameter : variablePair.second)
                {
                    std::cout << "\t" << parameter.first << ": "
                              << parameter.second << "\n";
                }
            }

#if 0
            /** Write variable for buffering */
            adios2::Variable<float> bpFloats =
                bpIO.InquireVariable<float>("/group1/subgroup1/bpFloats");
            adios2::Variable<int> bpInts =
                bpIO.InquireVariable<int>("/group1/subgroup1/bpInts");

            const std::size_t Nx = 10;
            if (bpFloats) // means found
            {
                std::vector<float> myFloats;

                // read only the chunk corresponding to our rank
                bpFloats.SetSelection({{Nx * rank}, {Nx}});
                // myFloats.data is pre-allocated
                bpReader.Get<float>(bpFloats, myFloats, adios2::Mode::Sync);

                if (rank == 0)
                {
                    std::cout << "MyFloats: \n";
                    for (const auto number : myFloats)
                    {
                        std::cout << number << " ";
                    }
                    std::cout << "\n";
                }
            }

            if (bpInts) // means not found
            {
                std::vector<int> myInts;
                // read only the chunk corresponding to our rank
                bpInts.SetSelection({{Nx * rank}, {Nx}});

                bpReader.Get<int>(bpInts, myInts, adios2::Mode::Sync);

                if (rank == 0)
                {
                    std::cout << "myInts: \n";
                    for (const auto number : myInts)
                    {
                        std::cout << number << " ";
                    }
                    std::cout << "\n";
                }
            }
#endif
            bpReader.EndStep();
        }
        /** Close bp file, engine becomes unreachable after this*/
        bpReader.Close();
    }
    catch (std::invalid_argument &e)
    {
        if (rank == 0)
        {
            std::cerr
                << "Invalid argument exception, STOPPING PROGRAM from rank "
                << rank << "\n";
            std::cerr << e.what() << "\n";
        }
#if ADIOS2_USE_MPI
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }
    catch (std::ios_base::failure &e)
    {
        if (rank == 0)
        {
            std::cerr << "IO System base failure exception, STOPPING PROGRAM "
                         "from rank "
                      << rank << "\n";
            std::cerr << e.what() << "\n";
            std::cerr << "The file " << filename << " does not exist."
                      << " Presumably this is because hello_bpWriter has not "
                         "been run."
                      << " Run ./hello_bpWriter before running this program.\n";
        }
#if ADIOS2_USE_MPI
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }
    catch (std::exception &e)
    {
        if (rank == 0)
        {
            std::cerr << "Exception, STOPPING PROGRAM from rank " << rank
                      << "\n";
            std::cerr << e.what() << "\n";
        }
#if ADIOS2_USE_MPI
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif
    // now read part

    return 0;
}
