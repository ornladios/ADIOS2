#include "nvs/store.h"
#include "nvs/store_manager.h"
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "nvswrapper.h"

extern "C" {
void *nvs_create_store()
{
    nvs::Store *st = nvs::StoreManager::GetInstance("");
    std::string store_name = st->get_store_id();
    return (void *)st;
}

void *nvs_open_store(char *STORE_ID)
{
    std::string store_name = STORE_ID;
    nvs::Store *st = nvs::StoreManager::GetInstance(store_name);
    return (void *)st;
}

char *nvs_get_store_name(void *vst)
{
    nvs::Store *st = (nvs::Store *)vst;
    const char *tmp = st->get_store_id().c_str();
    return strdup(tmp);
}

void *nvs_alloc(void *vst, unsigned long *size, char *s)
{
    void *ptr;
    nvs::Store *st = (nvs::Store *)vst;
    st->create_obj(std::string(s), *size, &ptr);
    return ptr;
}

void *nvs_get_with_malloc(void *vst, char *key, long version)
{
    void *ptr;
    nvs::Store *st = (nvs::Store *)vst;
    nvs::ErrorCode tmp = st->get_with_malloc(std::string(key), version, &ptr);
    if (tmp != nvs::ErrorCode::NO_ERROR)
    {
        std::cout << "get with malloc error" << std::endl;
    }
    return ptr;
}

void nvs_free_(void *vst, void *ptr) {}

void nvs_snapshot_(void *vst, int *proc_id)
{
    nvs::Store *st = (nvs::Store *)vst;
    st->put_all();
}

int nvs_finalize_(void *vst)
{
    nvs::Store *st = (nvs::Store *)vst;
    delete st;
}
}
