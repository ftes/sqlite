#include "vm_enclave_t.h"

#include "sgx_trts.h" /* for sgx_ocalloc, sgx_is_outside_enclave */

#include <string.h> /* for memcpy etc */
#include <stdlib.h> /* for malloc/free etc */

#define CHECK_REF_POINTER(ptr, siz) do {	\
	if (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_UNIQUE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

/* sgx_ocfree() just restores the original outside stack pointer. */
#define OCALLOC(val, type, len) do {	\
	void* __tmp = sgx_ocalloc(len);	\
	if (__tmp == NULL) {	\
		sgx_ocfree();	\
		return SGX_ERROR_UNEXPECTED;\
	}			\
	(val) = (type)__tmp;	\
} while (0)



typedef struct ms_ocall_sqlite3_exec_with_sqlite3InitCallback_t {
	int ms_retval;
	void* ms_database;
	char* ms_sql;
	void* ms_argument;
	size_t ms_arg_size;
} ms_ocall_sqlite3_exec_with_sqlite3InitCallback_t;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127)
#pragma warning(disable: 4200)
#endif

static sgx_status_t SGX_CDECL sgx_dummy_public_root_ecall(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	dummy_public_root_ecall();
	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* call_addr; uint8_t is_priv;} ecall_table[1];
} g_ecall_table = {
	1,
	{
		{(void*)(uintptr_t)sgx_dummy_public_root_ecall, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[1][1];
} g_dyn_entry_table = {
	1,
	{
		{0, },
	}
};


sgx_status_t SGX_CDECL ocall_sqlite3_exec_with_sqlite3InitCallback(int* retval, void* database, const char* sql, void* argument, size_t arg_size)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_sql = sizeof(*sql);
	size_t _len_argument = arg_size;

	ms_ocall_sqlite3_exec_with_sqlite3InitCallback_t* ms;
	OCALLOC(ms, ms_ocall_sqlite3_exec_with_sqlite3InitCallback_t*, sizeof(*ms));

	ms->ms_database = SGX_CAST(void*, database);
	if (sql != NULL && sgx_is_within_enclave(sql, _len_sql)) {
		OCALLOC(ms->ms_sql, char*, _len_sql);
		memcpy((void*)ms->ms_sql, sql, _len_sql);
	} else if (sql == NULL) {
		ms->ms_sql = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	if (argument != NULL && sgx_is_within_enclave(argument, _len_argument)) {
		OCALLOC(ms->ms_argument, void*, _len_argument);
		memcpy(ms->ms_argument, argument, _len_argument);
	} else if (argument == NULL) {
		ms->ms_argument = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms_arg_size = arg_size;
	status = sgx_ocall(0, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
