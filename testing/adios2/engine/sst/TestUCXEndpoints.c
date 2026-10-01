/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include the actual static data-plane functions; inject UCX results without
 * an RDMA device. Unused entry points are discarded with --gc-sections.
 */
#include <sys/time.h>
#include "adios2/toolkit/sst/dp/ucx_dp.c"

#define CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            fprintf(stderr, "FAIL at line %d: %s\n", __LINE__, #condition); \
            exit(1); \
        } \
    } while (0)

static int creation_attempts;
static int unpack_attempts;
static int fail_creation = 1;
static int close_attempts;
static int worker_destroys;
static int context_cleanups;
static char invalid_endpoint;
static char created_endpoint;

static void quiet_verbose(void *stream, int level, char *format, ...)
{
    (void)stream;
    (void)level;
    (void)format;
}

ucs_status_t __wrap_ucp_ep_create(ucp_worker_h worker, const ucp_ep_params_t *params,
                                ucp_ep_h *endpoint)
{
    (void)worker;
    creation_attempts++;
    if (fail_creation)
    {
        /* An error must take precedence over an unusable output handle. */
        *endpoint = (ucp_ep_h)&invalid_endpoint;
        return UCS_ERR_IO_ERROR;
    }
    CHECK(params->field_mask == UCP_EP_PARAM_FIELD_REMOTE_ADDRESS);
    CHECK(((const char *)params->address)[0] == 'w');
    *endpoint = (ucp_ep_h)&created_endpoint;
    return UCS_OK;
}

ucs_status_t __wrap_ucp_ep_rkey_unpack(ucp_ep_h endpoint, const void *packed,
                                     ucp_rkey_h *key)
{
    (void)packed;
    (void)key;
    unpack_attempts++;
    CHECK(endpoint == (ucp_ep_h)&created_endpoint);
    /* Stop before submitting a real get on a mocked endpoint. */
    return UCS_ERR_IO_ERROR;
}

ucs_status_ptr_t __wrap_ucp_ep_close_nbx(ucp_ep_h endpoint, const ucp_request_param_t *params)
{
    CHECK(endpoint == (ucp_ep_h)&created_endpoint);
    CHECK(params->op_attr_mask == 0);
    close_attempts++;
    return NULL;
}

void __wrap_ucp_worker_destroy(ucp_worker_h worker)
{
    (void)worker;
    worker_destroys++;
}

void __wrap_ucp_cleanup(ucp_context_h context)
{
    (void)context;
    CHECK(worker_destroys == 1);
    context_cleanups++;
}

int main(void)
{
    enum { writer_count = 2048 };
    struct _CP_Services services = {0};
    services.verbose = quiet_verbose;
    Ucx_RS_Stream stream = calloc(1, sizeof(*stream));
    CHECK(stream != NULL);
    stream->Fabric = calloc(1, sizeof(*stream->Fabric));
    CHECK(stream->Fabric != NULL);
    FabricState fabric = stream->Fabric;
    char address[] = "writer address";
    struct _UcxWriterContactInfo contact = {0};
    contact.Address = address;
    contact.Length = sizeof(address);
    void *contacts[writer_count];
    for (int i = 0; i < writer_count; i++)
        contacts[i] = &contact;
    UcxProvideWriterDataToReader(&services, stream, writer_count, NULL, contacts);
    CHECK(creation_attempts == 0);
    CHECK(stream->WriterContactInfo[0].Address != address);
    CHECK(stream->WriterContactInfo[0].Length == sizeof(address));
    address[0] = 'X';
    CHECK(((char *)stream->WriterContactInfo[0].Address)[0] == 'w');

    char buffer[16];
    struct _UcxBufferHandle info = {0};
    info.Block = buffer;
    info.rkey = "mock key";
    void *handle = UcxReadRemoteMemory(&services, stream, 0, 0, 0,
                                     sizeof(buffer), buffer, &info);
    CHECK(handle == NULL && unpack_attempts == 0);
    CHECK(stream->WriterEP[0] == NULL);
    CHECK(UcxWaitForCompletion(&services, handle) == 0);
    CHECK(creation_attempts == 1);

    fail_creation = 0;
    CHECK(UcxReadRemoteMemory(&services, stream, 0, 0, 0, 0, buffer, &info) == NULL);
    CHECK(creation_attempts == 2 && unpack_attempts == 1);
    CHECK(UcxReadRemoteMemory(&services, stream, 0, 0, 0, 0, buffer, &info) == NULL);
    CHECK(creation_attempts == 2 && unpack_attempts == 2);
    CHECK(stream->WriterEP[1] == NULL);
    CHECK(stream->WriterEP[writer_count - 1] == NULL);
    CHECK(UcxReadRemoteMemory(&services, stream, -1, 0, 0, 0, buffer, &info) == NULL);
    CHECK(UcxReadRemoteMemory(&services, stream, writer_count, 0, 0, 0, buffer, &info) == NULL);
    free(stream->WriterContactInfo[1].Address);
    stream->WriterContactInfo[1].Address = NULL;
    CHECK(UcxReadRemoteMemory(&services, stream, 1, 0, 0, 0, buffer, &info) == NULL);
    CHECK(creation_attempts == 2);
    UcxDestroyReader(&services, stream);
    CHECK(close_attempts == 1 && worker_destroys == 1 && context_cleanups == 1);
    free(fabric);
    puts("PASS: 2048 contacts, no eager endpoints, copied addresses, "
         "failure handling, retry, cached reuse and shutdown cleanup");
    return 0;
}
