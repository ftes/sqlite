#ifndef VM_ENCLAVE_T_H__
#define VM_ENCLAVE_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */


#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif


void dummy_public_root_ecall();

sgx_status_t SGX_CDECL ocall_sqlite3_exec_with_sqlite3InitCallback(int* retval, void* database, const char* sql, void* argument, size_t arg_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
