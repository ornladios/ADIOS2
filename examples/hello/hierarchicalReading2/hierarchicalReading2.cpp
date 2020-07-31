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
    adios2::IO bpIO;
    adios2::Engine engine;
    adios2::Variable<std::string> meta_info;

public:
    Group( adios2::IO in_bpIO, adios2::Engine in_engine): bpIO(in_bpIO), engine(in_engine) {
        //create meta info
        adios2::Variable<std::string> meta_info =
            bpIO.DefineVariable<std::string>("meta_info");
    };
    ~Group(){};
    void getMetaInfo();
    void updateMetaInfo();
    void DefineGroup(std::string);
    void DefineVariable(const std::string);
    void DefineAttribute(const std::string);
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

void Group::getMetaInfo() {
    //meta info  is stored as absoulute path "group1,group2,group3,:group1/var1,var,group2/var2,:attr,group1/attr1,group2/attr2"
    adios2::Variable<std::string> meta_info =
        bpIO.InquireVariable<std::string>("meta_info");
    std::string meta_info_str_read;
    if (meta_info){
        engine.Get<std::string>(meta_info, meta_info_str_read, adios2::Mode::Sync);
        //split a string by a separator ":"
        std::vector<std::string> groups_variables_attributes = split(meta_info_str_read, ':');
        groups = split(groups_variables_attributes[0], ',');
        variables = split(groups_variables_attributes[1], ',');
        attributes = split(groups_variables_attributes[1], ',');
        return;
    }else{
        throw std::exception();
    }
}

void Group::updateMetaInfo(){
    std::string meta_info_str;
    for (std::string s : groups){
        meta_info_str += s;
        meta_info_str += ",";
    }
    meta_info_str +=":";
    for (std::string s : variables){
        meta_info_str += s;
        meta_info_str += ",";
    }
    meta_info_str +=":";
    for (std::string s : attributes){
        meta_info_str += s;
        meta_info_str += ",";
    }
    //alternatively we can implement "make transaction" at the end with Put
    engine.Put<std::string>(meta_info, meta_info_str.data());
    return;
}

void Group::DefineGroup(std::string groupName) {
    groups.push_back(groupName);
    return;
}

void Group::DefineVariable(const std::string variable_name) {

    /* adios2::Variable<T> bpFloats = bpIO.DefineVariable<T>(
         variableName, {size * Nx}, {rank * Nx}, {Nx},
         adios2::ConstantDims);
         Put ...*/

    variables.push_back(variable_name);
    return;
}

void Group::DefineAttribute(const std::string attribute_name) {

//wrappers to actual adios functions
    /* adios2::Variable<T> bpFloats = bpIO.DefineVariable<T>(
         variableName, {size * Nx}, {rank * Nx}, {Nx},
         adios2::ConstantDims);
         Put ... */
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

        Group g = Group(bpIO, bpFileWriter);

        /** global array : name, { shape (total) }, { start (local) }, {
         * count
         * (local) }, all are constant dimensions */
        g.DefineGroup("/group1");
        g.DefineGroup("/group1/subroup1");
        g.DefineVariable("/group1/subroup1/bpFloats");
        adios2::Variable<float> bpFloats = bpIO.DefineVariable<float>(
            "/group1/subgroup1/bpFloats", {size * Nx}, {rank * Nx}, {Nx},
            adios2::ConstantDims);
        g.DefineVariable("/group1/subroup1/bpInts");
        adios2::Variable<int> bpInts =
            bpIO.DefineVariable<int>("/group1/subgroup1/bpInts", {size * Nx},
                                     {rank * Nx}, {Nx}, adios2::ConstantDims);


        std::vector<std::string> myStrings = {"one two three"};
        g.DefineAttribute("bpString");
        adios2::Variable<std::string> Array_of_strings =  bpIO.DefineVariable<std::string>("bpString");

        adios2::Variable<std::string> bpString =
            bpIO.DefineVariable<std::string>("bpString");


        for (int i = 0; i < number_of_steps; i++)
        {
            std::string meta_info_str;
            bpFileWriter.BeginStep();

            /** Put variables for buffering, template type is optional */

            bpFileWriter.Put<float>(bpFloats, myFloats.data());

            bpFileWriter.Put(bpInts, myInts.data());

            bpFileWriter.Put(bpString, myString);

            g.updateMetaInfo();
            bpFileWriter.EndStep();
        }
        /** Create bp file, engine becomes unreachable after this*/
        bpFileWriter.Close();
        if (rank == 0)
        {
            std::cout << "Wrote file " << filename
                      << " to disk. It can now be read by running "
                         "./bin/hello_bpReader.\n";
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
        Group g = Group(bpIO, bpReader);
        g.getMetaInfo();
        auto all_groups = g.AvailableGroups();
        auto all_variables = g.AvailableVariables();
        auto all_attriutes = g.AvailableVariables();

        for (int step = 0; step < number_of_steps; step++)
        {
            bpReader.BeginStep();
            adios2::Variable<std::string> meta_info =
                bpIO.InquireVariable<std::string>("meta_info");
            const std::map<std::string, adios2::Params> variables =
                bpIO.AvailableVariables();

            for (const auto variablePair : variables)
            {
                std::cout << "Name: " << variablePair.first;

                for (const auto &parameter : variablePair.second)
                {
                    std::cout << "\t" << parameter.first << ": "
                              << parameter.second << "\n";
                }
            }

            adios2::Variable<std::string> meta_info_str =
                bpIO.InquireVariable<std::string>("meta_info");
            std::string meta_info_str_read;
            if (meta_info){
                bpReader.Get<std::string>(meta_info, meta_info_str_read, adios2::Mode::Sync);
                std::cout << "meta info " << meta_info_str_read << std::endl;
            }
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
