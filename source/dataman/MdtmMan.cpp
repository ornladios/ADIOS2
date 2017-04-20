#include <sys/stat.h>
#include <unistd.h>
#include "zmq.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include "MdtmMan.h"


MdtmMan::MdtmMan()
    :StreamMan()
{}

int MdtmMan::init(json p_jmsg){

    StreamMan::init(p_jmsg);

    if(p_jmsg["pipe_prefix"] == nullptr){
        pipe_desc["pipe_prefix"] = "/tmp/MdtmManPipes/";
    }
    else{
        pipe_desc["pipe_prefix"] = p_jmsg["pipe_prefix"];
    }

    pipe_desc["operation"] = "init";
    pipe_desc["mode"] = m_stream_mode;

    string pipename_prefix = "MdtmManPipe";
    for(int i=0; i<m_num_channels; i++){
        stringstream pipename;
        pipename << pipename_prefix << i;
        if(i==0){
            pipe_desc["pipe_names"] = {pipename.str()};
            pipe_desc["loss_tolerance"] = {m_tolerance[i]};
            pipe_desc["priority"] = {m_priority[i]};
        }
        else{
            pipe_desc["pipe_names"].insert(pipe_desc["pipe_names"].end(), pipename.str());
            pipe_desc["loss_tolerance"].insert(pipe_desc["loss_tolerance"].end(), m_tolerance[i]);
            pipe_desc["priority"].insert(pipe_desc["priority"].end(), m_priority[i]);
        }
    }

    // ZMQ_DataMan_MDTM
    if(m_stream_mode=="sender"){
        zmq_ipc_req = zmq_socket (zmq_context, ZMQ_REQ);
        zmq_connect (zmq_ipc_req, "ipc:///tmp/ADIOS_MDTM_pipe");
        char buffer_return[10];
        zmq_send (zmq_ipc_req, pipe_desc.dump().c_str(), pipe_desc.dump().length(), 0);
        zmq_recv (zmq_ipc_req, buffer_return, 10, 0);
    }

    // Pipes
    mkdir(pipe_desc["pipe_prefix"].get<string>().c_str(), 0755);
    for (int i=0; i<pipe_desc["pipe_names"].size(); i++){
        string filename = pipe_desc["pipe_prefix"].get<string>() + pipe_desc["pipe_names"][i].get<string>();
        mkfifo(filename.c_str(), 0666);
    }

    for(int i=0; i<m_num_channels; i++){
        stringstream pipename;
        pipename << pipename_prefix << i;
        string fullpipename = pipe_desc["pipe_prefix"].get<string>() + pipename.str();
        if (m_stream_mode == "sender"){
            int fp = open(fullpipename.c_str(), O_WRONLY);
            pipes.push_back(fp);
        }
        if (m_stream_mode == "receiver"){
            int fp = open(fullpipename.c_str(), O_RDONLY | O_NONBLOCK);
            pipes.push_back(fp);
        }
        pipenames.push_back(pipename.str());
    }
    return 0;
}

MdtmMan::~MdtmMan(){
    run_cmd("rm -rf " + pipe_desc["pipe_prefix"].get<string>());
    if(zmq_ipc_req) zmq_close(zmq_ipc_req);
}

int MdtmMan::put(const void *p_data, json p_jmsg){
    put_begin(p_data, p_jmsg);

    vector<size_t> putshape = p_jmsg["putshape"].get<vector<size_t>>();
    vector<size_t> varshape = p_jmsg["varshape"].get<vector<size_t>>();
    string dtype = p_jmsg["dtype"];

    int priority = 100;
    if(p_jmsg["priority"] != nullptr){
        priority = p_jmsg["priority"].get<int>();
    }

    int index;
    if(m_parallel_mode == "round"){
        if(m_current_channel == m_num_channels - 1){
            index = 0;
            m_current_channel = 0;
        }
        else{
            m_current_channel ++;
            index = m_current_channel;
        }
    }
    else if(m_parallel_mode == "priority"){
        index = closest(priority, pipe_desc["priority"], true);
    }

    p_jmsg["pipe"] = pipe_desc["pipe_names"][index];
    size_t putbytes = product(putshape, dsize(dtype));
    p_jmsg["putbytes"] = putbytes;
    size_t varbytes = product(varshape, dsize(dtype));
    p_jmsg["varbytes"] = varbytes;

    StreamMan::put(p_data, p_jmsg);

    index=0;
    for(int i=0; i<pipenames.size(); i++){
        if(p_jmsg["pipe"].get<string>() == pipenames[i]){
            index=i;
        }
    }
    string pipename = pipe_desc["pipe_prefix"].get<string>() + p_jmsg["pipe"].get<string>();
    write(pipes[index], p_data, putbytes);
    put_end(p_data, p_jmsg);
    return 0;
}

int MdtmMan::get(void *p_data, json &p_jmsg){
    return 0;
}

void MdtmMan::on_recv(json jmsg){
    cout << "MdtmMan::on_recv " << endl;

    // push new request
    jqueue.push(jmsg);
    bqueue.push(NULL);
    iqueue.push(0);

    // for flush
    if(jqueue.front()["operation"] == "flush"){
        callback();
        m_cache.clean_all("nan");
        bqueue.pop();
        iqueue.pop();
        jqueue.pop();
    }

    if(jqueue.size() == 0){
        return;
    }

    // for put
    for(int outloop=0; outloop<jqueue.size()*2; outloop++){
        if(jqueue.front()["operation"] == "put"){
            // allocate buffer
            size_t putbytes = jqueue.front()["putbytes"].get<size_t>();
            if(bqueue.front() == NULL) bqueue.front() = malloc(putbytes);

            // determine the pipe for the head request
            json msg = jqueue.front();
            if(msg == nullptr) break;
            int pipeindex=0;
            for(int i=0; i<pipenames.size(); i++){
                if(msg["pipe"].get<string>() == pipenames[i]){
                    pipeindex=i;
                }
            }

            // read the head request
            int error_times=0;
            int s = iqueue.front();
            putbytes = msg["putbytes"].get<int>();
            while(s<putbytes){
                int ret = read(pipes[pipeindex], ((char*)bqueue.front()) + s, putbytes - s);
                if(ret > 0){
                    s += ret;
                }
                else{
                    error_times++;
                    continue;
                }
                if(error_times > 1000000){
                    break;
                }
            }

            if(s == putbytes){
                m_cache.put(bqueue.front(),msg);
                if(bqueue.front()) free(bqueue.front());
                bqueue.pop();
                iqueue.pop();
                jqueue.pop();
                break;
            }
            else{
                iqueue.front()=s;
            }
        }
    }
}



