#include "TemporalMan.h"


TemporalMan::TemporalMan()
    :CompressMan()
{
}

TemporalMan::~TemporalMan()
{
}

int TemporalMan::init(json p_jmsg){
    return 0;
}

int TemporalMan::put(const void *p_data, json p_jmsg){
    put_begin(p_data, p_jmsg);
    put_end(p_data, p_jmsg);
    return 0;
}

int TemporalMan::get(void *p_data, json &p_jmsg){
    return 0;
}

void TemporalMan::flush(){

}

void TemporalMan::transform(const void* p_in, void* p_out, json &p_jmsg){

}

