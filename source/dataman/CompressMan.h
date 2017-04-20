#ifndef COMPRESSMAN_H_
#define COMPRESSMAN_H_


#include "DataMan.h"

using namespace std;


class CompressMan : public DataManBase{
    public:
        CompressMan() = default;
        ~CompressMan() = default;
        virtual string type(){return "Compress";}
};


#endif
