#ifndef STREAMMAN_H_
#define STREAMMAN_H_



#include "DataMan.h"
#include "CacheMan.h"
#include <thread>

using namespace std;


class StreamMan : public DataManBase{
    public:
        StreamMan() = default;
        virtual ~StreamMan();

        virtual int init(json p_jmsg);
        virtual int put(const void *p_data, json p_jmsg);
        virtual void on_recv(json msg) = 0;
        void flush();
        virtual string type(){return "Stream";}

    protected:
        void *zmq_context = NULL;
        CacheMan m_cache;
        void callback();

        string m_get_mode = "callback";
        string m_stream_mode;
        string m_local_ip;
        string m_remote_ip;
        int m_local_port;
        int m_remote_port;
        int m_num_channels = 1;
        vector<int> m_tolerance;
        vector<int> m_priority;

        // parallel
        string m_parallel_mode = "round"; // round, priority
        int m_current_channel = 0;

        inline string make_address(string ip, int port, string protocol){
            stringstream address;
            address << protocol << "://" << ip << ":" << port;
            return address.str();
        }

    private:
        void *zmq_meta = NULL;
        void zmq_meta_rep_thread_func();
        bool zmq_meta_rep_thread_active;
        thread *zmq_meta_rep_thread=NULL;
};


#endif
