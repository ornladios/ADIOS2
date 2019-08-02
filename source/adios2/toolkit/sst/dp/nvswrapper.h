//
// Created by pradeep on 1/11/18.
// Adapted by eisen July 2019
//

#ifndef NVSWRAPPER_H
#define NVSWRAPPER_H

/* C interface for fortran apps */
#ifdef __cplusplus
extern "C" {
#endif

void *nvs_open_store(char *store_name);

void *nvs_create_store();

char *nvs_get_store_name(void *vst);

void *nvs_alloc(void *vst, unsigned long *n, char *s);

void *nvs_get_with_malloc(void *vst, char *key, long version);

void nvs_free_(void *vst, void *ptr);

void nvs_snapshot_(void *vst, int *proc_id);

int nvs_finalize_(void *vst);

#ifdef __cplusplus
}
#endif

#endif // NVSWRAPPER_H
