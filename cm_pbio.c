#include "config.h"

#ifndef MODULE
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#ifdef HAVE_WINDOWS_H
#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#else
#include <asm/uaccess.h>
#include <linux/in.h>
#include "kernel/kcm.h"
#include "kernel/cm_kernel.h"
#include "kernel/library.h"

static char *
inet_ntoa(struct in_addr ina)
{
    static char buf[4*sizeof "123"];
    unsigned char *ucp = (unsigned char *)&ina;

    sprintf(buf, "%d.%d.%d.%d",
	ucp[0] & 0xff,
	ucp[1] & 0xff,
	ucp[2] & 0xff,
	ucp[3] & 0xff);
    return buf;
}
#endif
#undef NDEBUG
#include "assert.h"
#include "ffs.h"
#include "atl.h"
#include "evpath.h"
#include "chr_time.h"
#include "cm_internal.h"

/*
 * CM_pbio_query is the message handler for CM/PBIO messages.  CM/PBIO
 * messages are not ordinary CM messages (since they are not encoded with
 * PBIO themselves — that would be a nasty recursion) but instead are
 * handled as foreign messages.  CM_pbio_query supports three types of
 * incoming message:  QUERY (a format request), RESPONSE (the return
 * message after a query), and CACHE_PUSH (an unsolicited format ID/body
 * pair to be entered into the local context).  Message encoding is network
 * byte order for a simple structure of 4-byte integers.  A magic number
 * 'PBIO' as 4 bytes in ascii hex precedes all messages and is the foreign
 * message tag used by CM to recognize CM/PBIO messages.
 *
 * For formats that CM knows about (I.E. not CM user formats, but CM
 * message formats), CM uses CACHE_PUSH messages to preload needed formats
 * before a message is sent.  Which formats have been preloaded are tracked
 * on a per-connection basis (see cm_formats.c, CMformat_preload()).
 * Generally this means QUERY messages are not generated for CM message
 * formats.  However, they are necessary for CM user formats, such as those
 * used to encode ECho event messages.
 *
 * You can select CM self-hosted formats by setting the "CMSelfFormats"
 * environment variable, or force the use of an external format server with
 * "CMExternalFormats".
 */

extern int CM_pbio_query(CMConnection conn, CMTransport trans,
			 char *buffer, size_t length);

/*
 *  This is a bit tricky as we might have multiple pending format requests
 *  at a time, even for the same format_ID.  When this happens it's possible
 *  to build up a stack of requests.  So, we're careful to signal the 
 *  most recent request only.  The prior ones get a NULL return value.  
 *  This will cause PBIO to search once again for the format, which it 
 *  should now find.
 */

static void
signal_requests(CManager cm, char *server_rep, int condition)
{
    /* 
     *  signal the most recent (top) request and give it the server rep.
     *  The others get signalled with NULL;
     */
    
    int i;
    char *format_ID = NULL;
    int format_id_length = 0;
    /* find info for this request */
    for (i=0; i<cm->pending_request_max; i++) {
	if (cm->pbio_requests[i].condition == condition) {
	    format_ID = cm->pbio_requests[i].server_id;
	    format_id_length = cm->pbio_requests[i].id_length;
	}
    }
    if (format_id_length == 0) {
	printf("CMpbio Error in signal requests\n");
	return;
    }
    /* tag any duplicates as no longer the most recent request */
    for (i=0; i<cm->pending_request_max; i++) {
	if ((cm->pbio_requests[i].server_id != NULL) &&
	    (format_id_length = cm->pbio_requests[i].id_length) &&
	    (memcmp(cm->pbio_requests[i].server_id, format_ID,
		    format_id_length) == 0)) {
	    char **server_rep_ptr;
	    server_rep_ptr = 
		INT_CMCondition_get_client_data(cm, 
					    cm->pbio_requests[i].condition);
	    if (cm->pbio_requests[i].top_request == 1) {
		*server_rep_ptr = server_rep;
	    } else {
		*server_rep_ptr = NULL;
	    }
	    INT_CMCondition_signal(cm, cm->pbio_requests[i].condition);
	    cm->pbio_requests[i].id_length = 0;
	    cm->pbio_requests[i].server_id = NULL;
	    cm->pbio_requests[i].top_request = 0;
	    cm->pbio_requests[i].condition = -1;
	}
    }
}

#define MAGIC 0x5042494f
#define REVERSE_MAGIC 0x4f494250

#define PBIO_QUERY 0
#define PBIO_RESPONSE 1
#define PBIO_CACHE_PUSH 2

struct pbio_exchange_msg {
    int magic;
    int msg_len;
    int msg_type;    /* 0 is query.   1 is response */
    int cond;
    int payload1_length;
    int payload2_length;
};

extern struct CMtrans_services_s CMstatic_trans_svcs;

static int
CMpbio_send_format_response(FMFormat ioformat, CMConnection conn, 
			    int cond)
{
    struct pbio_exchange_msg msg;
    struct FFSEncodeVec vec[2];
    int actual;
    char *format_body_rep;
    int body_len = 0;

    format_body_rep = get_server_rep_FMformat(ioformat, &body_len);
    /*
     * msg_len is (sizeof(msg) minus 8 + whatever) because the msg struct 
     * includes magic and overall len, which is read separately.
     */
    msg.magic = MAGIC;
    msg.msg_len = sizeof(msg) - 8 + body_len;
    msg.msg_type = PBIO_RESPONSE;
    msg.payload1_length = body_len;
    msg.payload2_length = 0;
    msg.cond = cond;
    vec[0].iov_base = &msg;
    vec[0].iov_len = sizeof(msg);
    vec[1].iov_base = format_body_rep;
    vec[1].iov_len = body_len;
    CMtrace_out(conn->cm, CMLowLevelVerbose, "CMpbio send format response - total %ld bytes in writev\n", (long)(body_len + sizeof(msg)));
    actual = conn->trans->writev_func(&CMstatic_trans_svcs, 
				      conn->transport_data, 
				      &vec[0], 2, NULL);
    if (actual != 2) {
	internal_connection_close(conn);
	return 0;
    }
    return 1;
}

extern int
CMpbio_send_format_preload(FMFormat ioformat, CMConnection conn)
{
    struct pbio_exchange_msg msg;
    struct FFSEncodeVec vec[3];
    int actual;
    char *format_body_rep;
    char *server_ID;
    int body_len = 0;
    int id_len = 0;

    format_body_rep = get_server_rep_FMformat(ioformat, &body_len);
    server_ID = get_server_ID_FMformat(ioformat, &id_len);
    /*
     * msg_len is (sizeof(msg) minus 8 + whatever) because the msg struct 
     * includes magic and overall len, which is read separately.
     */
    msg.magic = MAGIC;
    msg.msg_len = sizeof(msg) - 8 + body_len + id_len;
    msg.msg_type = PBIO_CACHE_PUSH;
    msg.payload1_length = id_len;
    msg.payload2_length = body_len;
    msg.cond = 0;
    vec[0].iov_base = &msg;
    vec[0].iov_len = sizeof(msg);
    vec[1].iov_base = server_ID;
    vec[1].iov_len = id_len;
    vec[2].iov_base = format_body_rep;
    vec[2].iov_len = body_len;
    CMtrace_out(conn->cm, CMLowLevelVerbose, "CMpbio send format preload - total %ld bytes in writev\n", (long)(body_len + id_len + sizeof(msg)));
    actual = conn->trans->writev_func(&CMstatic_trans_svcs, 
				      conn->transport_data, 
				      &vec[0], 3, NULL);
    if (actual != 3) {
	internal_connection_close(conn);
	return 0;
    }
    return 1;
}

int CMself_hosted_formats = -1;

extern FMContext INT_CMget_FMcontext(CManager cm)
{
    return FMContext_from_FFS(cm->FFScontext);
}

extern void
CMinit_local_formats(CManager cm)
{
    if (CMself_hosted_formats == -1) {
	CMself_hosted_formats = CM_SELF_FORMATS;  /* default set in CMake */
	if (getenv("CMSelfFormats") != NULL) {
	    CMself_hosted_formats = 1;
	} else if (getenv("CMExternalFormats") != NULL) {
	    CMself_hosted_formats = 0;
	}
    }
    if (CMself_hosted_formats == 1) {
	FMContext fmc = create_local_FMcontext();
	cm->FFScontext = create_FFSContext_FM(fmc);
	CMtrace_out(cm, CMFormatVerbose, 
		    "\nUsing self-hosted PBIO formats\n");
	free_FMcontext(fmc);  /* really just drop the ref count */
    } else {
	cm->FFScontext = create_FFSContext();
	FMcontext_allow_self_formats(FMContext_from_FFS(cm->FFScontext));
	CMtrace_out(cm, CMFormatVerbose, 
		    "\nUsing external PBIO format server\n");
    }
    cm->FFSserver_identifier = FMcontext_get_format_server_identifier(FMContext_from_FFS(cm->FFScontext));
    if (cm->FFSserver_identifier == -1) {
	CMself_hosted_formats = 1;
    }
//   handle these natively to avoid foreign handlers     
//    INT_CMregister_non_CM_message_handler(0x5042494f, CM_pbio_query);
//    INT_CMregister_non_CM_message_handler(0x4f494250, CM_pbio_query);
}

static int
conn_read_to_buffer(CMConnection conn, void *buffer, int length)
{
    transport_entry trans = conn->trans;
    if (trans->read_to_buffer_func) {
	if (trans->read_to_buffer_func(&CMstatic_trans_svcs, 
				       conn->transport_data, buffer, length, 0)
	    != length) {
	    internal_connection_close(conn);
	    return 0;
	}
    } else {
	void *tmp_buffer;
	ssize_t actual;
	tmp_buffer = trans->read_block_func(&CMstatic_trans_svcs, 
					    conn->transport_data,
					    &actual, NULL);
	if (actual < length) {
	    internal_connection_close(conn);
	    return 0;
	}
	memcpy(buffer, tmp_buffer, length);
	free(tmp_buffer);
    }
    return 1;
}

static void
byte_swap(char *data, int size)
{
    int i;
    assert((size % 2) == 0);
    for (i = 0; i < size / 2; i++) {
	char tmp = data[i];
	data[i] = data[size - i - 1];
	data[size - i - 1] = tmp;
    }
}

extern int
CM_pbio_query(CMConnection conn, CMTransport trans, char *buffer, size_t length)
{
    struct pbio_exchange_msg tmp_msg;
    struct pbio_exchange_msg *msg = NULL;
    int swap;

    int *incoming_length;
    int tmp_length;
    size_t used_length = 4;
    int header = *(int*)buffer;

    CManager_lock(conn->cm);
    CMtrace_out(conn->cm, CMFormatVerbose, "CMPbio operation in progress\n");
     
    if (header == 0x5042494f) {
	swap = 0;
    } else {
	swap = 1;
    }
    if (length < used_length + 4) {
	if (trans->read_to_buffer_func) {
	    int actual = (int) trans->read_to_buffer_func(&CMstatic_trans_svcs,
						    conn->transport_data,
						    &tmp_length, 4, 0);
	    CMtrace_out(conn->cm, CMLowLevelVerbose, "CMpbio reading 4 length bytes\n");
	    if (actual != 4) {
		CMtrace_out(conn->cm, CMLowLevelVerbose, 
			    "CMdata read failed, actual %d\n", actual);
		internal_connection_close(conn);
		CManager_unlock(conn->cm);
		return 0;
	    }
	    incoming_length = &tmp_length;
	    length += 4;
	} else {
	    assert(0);
	}
    } else {
	incoming_length = (int *)(buffer + used_length);
    }
    used_length += 4;
    if (swap == 1) {
	byte_swap((char*)incoming_length, (int)sizeof(int));
    }

    if (length < used_length + sizeof(tmp_msg) - 8) {
	if (trans->read_to_buffer_func) {
	    int actual = (int) trans->read_to_buffer_func(&CMstatic_trans_svcs,
						    conn->transport_data,
						    ((char*)&tmp_msg) + 8, 
						    (int)sizeof(tmp_msg) - 8, 0);
	    CMtrace_out(conn->cm, CMLowLevelVerbose, "CMpbio reading %ld msg bytes\n",
			(long)sizeof(tmp_msg) - 8);
	    if (actual != (sizeof(tmp_msg) - 8)) {
		CMtrace_out(conn->cm, CMLowLevelVerbose, 
			    "CMdata read failed, actual %d\n", actual);
		internal_connection_close(conn);
		CManager_unlock(conn->cm);
		return 0;
	    }
	    msg = &tmp_msg;
	} else {
	    assert(0);
	}
    } else {
	msg = (struct pbio_exchange_msg *)(buffer);
	used_length += (sizeof(tmp_msg) - 8);
    }

    if (swap == 1) {
	byte_swap((char*)&msg->msg_type, (int)sizeof(msg->msg_type));
	byte_swap((char*)&msg->cond, (int)sizeof(msg->cond));
	byte_swap((char*)&msg->payload1_length, (int)sizeof(msg->payload1_length));
	byte_swap((char*)&msg->payload2_length, (int)sizeof(msg->payload2_length));
    }
    if (*incoming_length - sizeof(tmp_msg) + 8 != 
	(msg->payload1_length + msg->payload2_length)) {
	CMtrace_out(conn->cm, CMFormatVerbose, 
		    "CMpbio Inconsistent length information, incoming %d, pay1 %d, pay2 %d\n", 
		    *incoming_length, msg->payload1_length, msg->payload2_length);
	internal_connection_close(conn);
	CManager_unlock(conn->cm);
	return 0;
    }
    CMtrace_out(conn->cm, CMFormatVerbose, 
		"CMpbio Msg incoming length = %d, type %d, cond %d, pay1 len %d, pay2 len %d\n", 
		*incoming_length, msg->msg_type, msg->cond,
		msg->payload1_length, msg->payload2_length);
    switch (msg->msg_type) {
    case PBIO_QUERY: {
	char tmp_format_id[64];  /* oversize */
	char *format_id;
	FMFormat ioformat;
	CMtrace_out(conn->cm, CMFormatVerbose, 
		    "CMpbio Incoming Query message\n");
	if (msg->payload1_length > sizeof(tmp_format_id)) {
	    CMtrace_out(conn->cm, CMFormatVerbose, 
			"CMpbio Huge incoming payload on query - ignoring\n");
	    internal_connection_close(conn);
	}
	if (length < used_length + msg->payload1_length) {
	    CMtrace_out(conn->cm, CMLowLevelVerbose, 
			"CMpbio reading %d payload bytes\n",
			msg->payload1_length);
	    if (conn_read_to_buffer(conn, &tmp_format_id[0], 
				    msg->payload1_length) != 1) {
		CMtrace_out(conn->cm, CMFormatVerbose, "CMpbio Read Failed\n");
		internal_connection_close(conn);
	    }
	    format_id = &tmp_format_id[0];
	    length += msg->payload1_length;
	} else {
	    format_id = buffer + used_length;
	    used_length += msg->payload1_length;
	}
	ioformat = FMformat_from_ID(FMContext_from_FFS(conn->cm->FFScontext), 
				    &format_id[0]);
	if (ioformat == NULL) {
	    CMtrace_out(conn->cm, CMFormatVerbose, 
			"CMpbio No matching format\n");
	} else {
	    CMtrace_out(conn->cm, CMFormatVerbose, 
			"CMpbio Returning format %s\n",
			name_of_FMformat(ioformat));
	}
	if (CMpbio_send_format_response(ioformat, conn, msg->cond) != 1) {
	    CMtrace_out(conn->cm, CMFormatVerbose, "CMpbio - Write Failed\n");
	    internal_connection_close(conn);
	}
	break;
    }
    case PBIO_RESPONSE: {
	char *server_rep;
	CMtrace_out(conn->cm, CMFormatVerbose, 
		    "CMpbio - Incoming Response message\n");
	server_rep = malloc(msg->payload1_length);
	if (length < used_length + msg->payload1_length) {
	    CMtrace_out(conn->cm, CMLowLevelVerbose, 
			"CMpbio reading %d payload bytes\n",
			msg->payload1_length);
	    if (conn_read_to_buffer(conn, server_rep, 
				    msg->payload1_length) != 1) {
		CMtrace_out(conn->cm, CMFormatVerbose, "CMpbio Read Failed\n");
		internal_connection_close(conn);
	    }
	    length += msg->payload1_length;
	} else {
	    memcpy(server_rep, buffer + used_length, msg->payload1_length);
	    used_length += msg->payload1_length;
	}
	signal_requests(conn->cm, server_rep, msg->cond);
	break;
    }
    case PBIO_CACHE_PUSH: {
	char *server_rep;
	char *format_ID;
	CMtrace_out(conn->cm, CMFormatVerbose, 
		    "CMpbio - Incoming Cache Preload message\n");
	format_ID = malloc(msg->payload1_length);
	server_rep = malloc(msg->payload2_length);
	
	if (length < used_length + msg->payload1_length) {
	    CMtrace_out(conn->cm, CMLowLevelVerbose, 
			"CMpbio reading %d payload bytes\n",
			msg->payload1_length);
	    if (conn_read_to_buffer(conn, format_ID, 
				    msg->payload1_length) != 1) {
		CMtrace_out(conn->cm, CMFormatVerbose, "CMpbio Read Failed\n");
		internal_connection_close(conn);
	    }
	    length += msg->payload1_length;
	} else {
	    memcpy(format_ID, buffer + used_length, msg->payload1_length);
	    used_length += msg->payload1_length;
	}
	if (length < used_length + msg->payload2_length) {
	    CMtrace_out(conn->cm, CMLowLevelVerbose, 
			"CMpbio reading %d payload bytes\n",
			msg->payload2_length);
	    if (conn_read_to_buffer(conn, server_rep, 
				    msg->payload2_length) != 1) {
		CMtrace_out(conn->cm, CMFormatVerbose, "CMpbio Read Failed\n");
		internal_connection_close(conn);
	    }
	    length += msg->payload2_length;
	} else {
	    memcpy(server_rep, buffer + used_length, msg->payload2_length);
	    used_length += msg->payload2_length;
	}
	if (!load_external_format_FMcontext(FMContext_from_FFS(conn->cm->FFScontext), format_ID,
					    msg->payload1_length, server_rep)) {
	    CMtrace_out(conn->cm, CMFormatVerbose, 
			"CMpbio - cache load failed\n");
	} else {
	    CMtrace_out(conn->cm, CMFormatVerbose, "CMpbio - loaded format\n");
#ifndef MODULE
	    if (CMtrace_on(conn->cm, CMFormatVerbose)) {
		fprintf(conn->cm->CMTrace_file, "CMpbio Preload is format ");
		fprint_server_ID(conn->cm->CMTrace_file, (unsigned char*)format_ID);
		fprintf(conn->cm->CMTrace_file, "\n");
	    }
#endif
	}
	free(format_ID);
	break;
    }
    default: 
        {
	    char *buffer2;
	    int len = msg->payload1_length + msg->payload2_length;
	    CMtrace_out(conn->cm, CMFormatVerbose, 
			"CMpbio - Unknown incoming message type %d\n",
			msg->msg_type);
	    buffer2 = malloc(len);
	    if (conn_read_to_buffer(conn, buffer2, len) != 1) {
		CMtrace_out(conn->cm, CMFormatVerbose, "CMpbio Read Failed\n");
		internal_connection_close(conn);
	    }
	    /* ignore message */
	    free(buffer2);
	    break;
	}
    }
    CManager_unlock(conn->cm);
    return 0;
}
