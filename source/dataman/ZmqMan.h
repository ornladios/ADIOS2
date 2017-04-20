#ifndef ZMQMAN_H_
#define ZMQMAN_H_

#include"StreamMan.h"


class ZmqMan : public StreamMan{
    public:
        ZmqMan() = default;
        ~ZmqMan();

        virtual int init(json p_jmsg);
        virtual int put(const void *p_data, json p_jmsg);
        virtual int get(void *p_data, json &p_jmsg);
        virtual void transform(const void* p_in, void* p_out, json &p_jmsg){};


        virtual void on_recv(json msg);
        string name(){return "ZmqMan";}


    private:
        void *zmq_data = NULL;


};


extern "C" DataManBase* getMan(){
    return new ZmqMan;
}



#endif
