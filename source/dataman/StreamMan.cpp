#include <iostream>
#include <unistd.h>
#include <sstream>
#include "StreamMan.h"
#include "zmq.h"


StreamMan::~StreamMan(){
    if(zmq_meta) zmq_close(zmq_meta);
    if(zmq_context) zmq_ctx_destroy(zmq_context);
    zmq_meta_rep_thread_active = false;
    if(zmq_meta_rep_thread){
        if(zmq_meta_rep_thread->joinable())
            zmq_meta_rep_thread->join();
        delete zmq_meta_rep_thread;
    }
}

int StreamMan::init(json p_jmsg){
    if(check_json(p_jmsg, {"stream_mode", "remote_ip", "local_ip", "remote_port", "local_port" }, "StreamMan")){
        m_stream_mode = p_jmsg["stream_mode"];
        m_local_ip = p_jmsg["local_ip"];
        m_remote_ip = p_jmsg["remote_ip"];
        m_local_port = p_jmsg["local_port"];
        m_remote_port = p_jmsg["remote_port"];
        string remote_address = make_address(m_remote_ip, m_remote_port, "tcp");
        string local_address = make_address(m_local_ip, m_local_port, "tcp");

        m_tolerance.assign(m_num_channels, 0);
        m_priority.assign(m_num_channels, 100);
        if(p_jmsg["num_channels"] != nullptr) m_num_channels = p_jmsg["num_channels"];
        if(p_jmsg["tolerance"] != nullptr) m_tolerance = p_jmsg["tolerance"].get<vector<int>>();
        if(p_jmsg["priority"] != nullptr) m_priority = p_jmsg["priority"].get<vector<int>>();

        if(!zmq_context){
            zmq_context = zmq_ctx_new ();
            zmq_meta = zmq_socket (zmq_context, ZMQ_PAIR);
            if(m_stream_mode == "sender"){
                zmq_connect (zmq_meta, remote_address.c_str());
                cout << "StreamMan::init " << remote_address << " connected" << endl;
            }
            else if(m_stream_mode == "receiver"){
                zmq_bind (zmq_meta, local_address.c_str());
                cout << "StreamMan::init " << local_address << " bound" << endl;
            }
            zmq_meta_rep_thread_active = true;
            zmq_meta_rep_thread = new thread(&StreamMan::zmq_meta_rep_thread_func, this);
        }
        return 0;
    }
    else{
        return -1;
    }
}

void StreamMan::callback(){
    if(m_callback){
        vector<string> do_list = m_cache.get_do_list();
        for(string i : do_list){
            vector<string> var_list = m_cache.get_var_list(i);
            for(string j : var_list){
                m_callback(m_cache.get_buffer(i,j),
                        i,
                        j,
                        m_cache.get_dtype(i, j),
                        m_cache.get_shape(i, j)
                        );
            }
        }
    }
    else{
        logging("callback called but callback function not registered!");
    }
}

void StreamMan::flush(){
    json msg;
    msg["operation"] = "flush";
    zmq_send(zmq_meta, msg.dump().c_str(), msg.dump().length(), 0);
}

void StreamMan::zmq_meta_rep_thread_func(){
    while (zmq_meta_rep_thread_active){
        char msg[1024]="";
        int err = zmq_recv (zmq_meta, msg, 1024, ZMQ_NOBLOCK);
        if (err>=0){
            cout << "StreamMan::zmq_meta_rep_thread_func: " << msg << endl;
            json j = json::parse(msg);
            on_recv(j);
        }
        usleep(10);
    }
}

int StreamMan::put(const void *p_data, json p_jmsg){
    p_jmsg["operation"] = "put";
    zmq_send(zmq_meta, p_jmsg.dump().c_str(), p_jmsg.dump().length(), 0);
    return 0;
}



