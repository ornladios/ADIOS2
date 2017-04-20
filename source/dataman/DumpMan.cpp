#include "DumpMan.h"


int DumpMan::init(json p_jmsg){
    if(p_jmsg["dumping"]!=nullptr){
        m_dumping = p_jmsg["dumping"].get<bool>();
    }
    return 0;
}
int DumpMan::get(void *p_data, json &p_jmsg){
    return 0;
}

int DumpMan::put(const void *p_data, json p_jmsg){
    put_begin(p_data, p_jmsg);

    if(!m_dumping){
        return 0;
    }

    string doid = p_jmsg["doid"];
    string var = p_jmsg["var"];
    string dtype = p_jmsg["dtype"];
    vector<size_t> putshape = p_jmsg["putshape"].get<vector<size_t>>();
    vector<size_t> varshape = p_jmsg["varshape"].get<vector<size_t>>();
    vector<size_t> offset = p_jmsg["offset"].get<vector<size_t>>();
    int numbers_to_print = 100;
    if(numbers_to_print > product(putshape,1)){
        numbers_to_print = product(putshape,1);
    }
    size_t putbytes = product(putshape, dsize(dtype));

    cout << p_jmsg.dump(4) << endl;
    cout << "total MBs = " << product(putshape,dsize(dtype)) / 1000000 << endl;

    const void *data_to_dump;

    vector<char> data;
    data.resize(putbytes);

    if(auto_transform(p_data, data.data(), p_jmsg)){
        data_to_dump = data.data();
    }
    else{
        data_to_dump = p_data;
    }

    if(dtype == "float")
        for (size_t i=0; i<numbers_to_print; i++) cout << ((float*)data_to_dump)[i] << " ";
    if(dtype == "double")
        for (size_t i=0; i<numbers_to_print; i++) cout << ((double*)data_to_dump)[i] << " ";

    cout << endl;
    put_end(p_data, p_jmsg);
    return 0;
}

void DumpMan::flush(){
}



