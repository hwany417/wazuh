/*
 * Copyright (C) 2015-2019, Wazuh Inc.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>


#include "../headers/syscheck_op.h"
#include "../analysisd/eventinfo.h"

/* Auxiliar structs */

typedef struct __sk_decode_data_s {
    sk_sum_t sum;
    char *c_sum;
    char *w_sum;
}sk_decode_data_t;

typedef struct __sk_fill_event_s {
    sk_sum_t sum;
    Eventinfo lf;
    char *f_name;
}sk_fill_event_t;

typedef struct __sk_build_sum_s {
    sk_sum_t sum;
    char *output;
}sk_build_sum_t;

typedef struct __unescape_syscheck_field_data_s {
    char *input;
    char *output;
}unescape_syscheck_field_data_t;

/* wrappers */

int __wrap_rmdir_ex(const char *name) {
    int ret = mock();

    if(ret == -1) {
        errno = ENOTEMPTY;
    } else {
        errno = 0;
    }

    check_expected(name);
    return ret;
}

char ** __wrap_wreaddir(const char * name) {
    check_expected(name);
    return mock_type(char**);
}

void __wrap__mdebug1(const char * file, int line, const char * func, const char *msg, ...) {
    char *param1;
    va_list args;

    va_start(args, msg);
    param1 = va_arg(args, char*);
    va_end(args);

    check_expected(msg);
    check_expected(param1);
    return;
}

void __wrap__mdebug2(const char * file, int line, const char * func, const char *msg, ...) {
    int param1;
    char *param2;
    va_list args;
    const char *aux = msg;
    int i = 0;

    va_start(args, msg);

    while(aux = strchr(aux, '%'), aux) {
        i++;
        aux++;
    }

    if(i) {
        param1 = va_arg(args, int);
        check_expected(param1);
        i--;
    }
    if(i) {
        param2 = va_arg(args, char*);
        check_expected(param2);
    }
    va_end(args);

    check_expected(msg);
    return;
}

void __wrap__mwarn(const char * file, int line, const char * func, const char *msg, ...) {
    char *param1, *param2;
    va_list args;

    va_start(args, msg);
    param1 = va_arg(args, char*);
    param2 = va_arg(args, char*);
    va_end(args);

    check_expected(msg);
    check_expected(param1);
    check_expected(param2);
}

void __wrap__merror(const char * file, int line, const char * func, const char *msg, ...) {
    char *param1;
    int param2;
    va_list args;

    va_start(args, msg);
    param1 = va_arg(args, char*);
    param2 = va_arg(args, int);
    va_end(args);

    check_expected(msg);
    check_expected(param1);
    check_expected(param2);
}

struct group *__wrap_getgrgid(gid_t gid) {
    return mock_ptr_type(struct group*);
}

extern CJSON_PUBLIC(cJSON *) __real_cJSON_CreateArray(void);
CJSON_PUBLIC(cJSON *) __wrap_cJSON_CreateArray(void) {
    return mock_type(CJSON_PUBLIC(cJSON *));
}

extern CJSON_PUBLIC(cJSON *) __real_cJSON_CreateObject(void);
CJSON_PUBLIC(cJSON *) __wrap_cJSON_CreateObject(void) {
    return mock_type(CJSON_PUBLIC(cJSON *));
}

extern void __real_wstr_split(char *str, char *delim, char *replace_delim, int occurrences, char ***splitted_str);
void __wrap_wstr_split(char *str, char *delim, char *replace_delim, int occurrences, char ***splitted_str) {
    if(mock()) {
        __real_wstr_split(str, delim, replace_delim, occurrences, splitted_str);
    } else {
        *splitted_str = NULL;
    }
}

int __wrap_OS_ConnectUnixDomain(const char *path, int type, int max_msg_size) {
    check_expected(path);
    check_expected(type);
    check_expected(max_msg_size);

    return mock();
}

int __wrap_OS_SendSecureTCP(int sock, uint32_t size, const void * msg) {
    check_expected(sock);
    check_expected(size);
    check_expected(msg);

    return mock();
}

// TODO: Test solaris version of this wrapper.
#ifdef SOLARIS
struct passwd **__wrap_getpwuid_r(uid_t uid, struct passwd *pwd,
                                  char *buf, size_t buflen) {

    pwd->pw_name = mock_type(char*);

    return mock_type(struct passwd*);
}
#else
int __wrap_getpwuid_r(uid_t uid, struct passwd *pwd,
                      char *buf, size_t buflen, struct passwd **result) {

    pwd->pw_name = mock_type(char*);
    *result = mock_type(struct passwd*);

    return mock();
}
#endif

/* setup/teardown */
static int teardown_string(void **state) {
    free(*state);
    return 0;
}

static int setup_sk_decode(void **state) {
    sk_decode_data_t *data = calloc(1, sizeof(sk_decode_data_t));

    if(!data) {
        return -1;
    }

    *state = data;
    return 0;
}

static int teardown_sk_decode(void **state) {
    sk_decode_data_t *data = *state;

    if(data) {
        // sk_sum_clean(&data->sum);

        os_free(data->c_sum);
        os_free(data->w_sum);

        free(data);
    }
    return 0;
}

static int setup_sk_fill_event(void **state) {
    sk_fill_event_t* data = calloc(1, sizeof(sk_fill_event_t));

    if(!data) {
        return -1;
    }

    os_calloc(FIM_NFIELDS, sizeof(DynamicField), data->lf.fields);

    *state = data;
    return 0;
}

static int teardown_sk_fill_event(void **state) {
    sk_fill_event_t* data = *state;

    if(data){
        os_free(data->f_name);

        os_free(data->lf.fields);
        // sk_sum_clean(&data->sum);
        // Free_Eventinfo(&data->lf);
    }
    return 0;
}

static int setup_sk_build_sum(void **state) {
    sk_build_sum_t* data = calloc(1, sizeof(sk_build_sum_t));

    if(!data) {
        return -1;
    }

    os_calloc(OS_MAXSTR, sizeof(char), data->output);

    *state = data;
    return 0;
}

static int teardown_sk_build_sum(void **state) {
    sk_build_sum_t* data = *state;

    if(data){
        os_free(data->output);
        // sk_sum_clean(&data->sum);
    }
    return 0;
}

static int setup_unescape_syscheck_field(void **state) {
    *state = calloc(1, sizeof(unescape_syscheck_field_data_t));

    if(!*state) {
        return -1;
    }
    return 0;
}

static int teardown_unescape_syscheck_field(void **state) {
    unescape_syscheck_field_data_t *data = *state;

    if(data) {
        os_free(data->input);
        os_free(data->output);
        free(data);
    }
    return 0;
}

static int teardown_cjson(void **state) {
    cJSON *array = *state;

    cJSON_Delete(array);

    return 0;
}

/* Tests */

/* delete_target_file tests */
static void test_delete_target_file_success(void **state) {
    int ret = -1;
    char *path = "/test_file.tmp";

    expect_string(__wrap_rmdir_ex, name, "/var/ossec/queue/diff/local/test_file.tmp");
    will_return(__wrap_rmdir_ex, 0);

    ret = delete_target_file(path);

    assert_int_equal(ret, 0);
}

static void test_delete_target_file_rmdir_ex_error(void **state) {
    int ret = -1;
    char *path = "/test_file.tmp";

    expect_string(__wrap_rmdir_ex, name, "/var/ossec/queue/diff/local/test_file.tmp");
    will_return(__wrap_rmdir_ex, -1);

    ret = delete_target_file(path);

    assert_int_equal(ret, 1);
}

/* escape_syscheck_field tests */
static void test_escape_syscheck_field_escape_all(void **state) {
    char *input = "This is: a test string!!";
    char *output = NULL;

    output = escape_syscheck_field(input);

    *state = output;

    assert_string_equal(output, "This\\ is\\:\\ a\\ test\\ string\\!\\!");
}

static void test_escape_syscheck_field_null_input(void **state) {
    char *output;

    output = (char*)0xFFFF;
    output = escape_syscheck_field(NULL);

    assert_null(output);
}

/* normalize_path tests */
static void test_normalize_path_success(void **state) {
    char *test_string = strdup("C:\\Regular/windows/path\\");
    *state = test_string;

    normalize_path(test_string);

    assert_string_equal(test_string, "C:\\Regular\\windows\\path\\");
}

static void test_normalize_path_linux_dir(void **state) {
    char *test_string = *state;
    test_string = strdup("/var/ossec/unchanged/path");

    normalize_path(test_string);

    assert_string_equal(test_string, "/var/ossec/unchanged/path");
}

static void test_normalize_path_null_input(void **state) {
    char *test_string = NULL;

    normalize_path(test_string);

    assert_null(test_string);
}

/* remove_empty_folders tests */
static void test_remove_empty_folders_success(void **state) {
    char *input = "/var/ossec/queue/diff/local/test-dir/";
    int ret = -1;

    expect_string(__wrap_wreaddir, name, "/var/ossec/queue/diff/local/test-dir");
    will_return(__wrap_wreaddir, NULL);

    expect_string(__wrap__mdebug1, msg, "Removing empty directory '%s'.");
    expect_string(__wrap__mdebug1, param1, "/var/ossec/queue/diff/local/test-dir");

    expect_string(__wrap_rmdir_ex, name, "/var/ossec/queue/diff/local/test-dir");
    will_return(__wrap_rmdir_ex, 0);

    ret = remove_empty_folders(input);

    assert_int_equal(ret, 0);
}

static void test_remove_empty_folders_recursive_success(void **state) {
    char *input = "/var/ossec/queue/diff/local/dir1/dir2/";
    int ret = -1;

    // Remove dir2
    expect_string(__wrap_wreaddir, name, "/var/ossec/queue/diff/local/dir1/dir2");
    will_return(__wrap_wreaddir, NULL);

    expect_string(__wrap__mdebug1, msg, "Removing empty directory '%s'.");
    expect_string(__wrap__mdebug1, param1, "/var/ossec/queue/diff/local/dir1/dir2");

    expect_string(__wrap_rmdir_ex, name, "/var/ossec/queue/diff/local/dir1/dir2");
    will_return(__wrap_rmdir_ex, 0);

    // Remove dir1
    expect_string(__wrap_wreaddir, name, "/var/ossec/queue/diff/local/dir1");
    will_return(__wrap_wreaddir, NULL);

    expect_string(__wrap__mdebug1, msg, "Removing empty directory '%s'.");
    expect_string(__wrap__mdebug1, param1, "/var/ossec/queue/diff/local/dir1");

    expect_string(__wrap_rmdir_ex, name, "/var/ossec/queue/diff/local/dir1");
    will_return(__wrap_rmdir_ex, 0);

    ret = remove_empty_folders(input);

    assert_int_equal(ret, 0);
}

static void test_remove_empty_folders_null_input(void **state) {
    int ret = -1;

    ret = remove_empty_folders(NULL);

    assert_int_equal(ret, 0);
}

// TODO: Validate this condition is required to be tested
static void test_remove_empty_folders_relative_path(void **state) {
    char *input = "./local/test-dir/";
    int ret = -1;

    expect_string(__wrap_wreaddir, name, "./local/test-dir");
    expect_string(__wrap_wreaddir, name, "./local");
    expect_string(__wrap_wreaddir, name, ".");
    will_return_always(__wrap_wreaddir, NULL);

    expect_string_count(__wrap__mdebug1, msg, "Removing empty directory '%s'.", 3);
    expect_string(__wrap__mdebug1, param1, "./local/test-dir");
    expect_string(__wrap__mdebug1, param1, "./local");
    expect_string(__wrap__mdebug1, param1, ".");

    expect_string(__wrap_rmdir_ex, name, "./local/test-dir");
    expect_string(__wrap_rmdir_ex, name, "./local");
    expect_string(__wrap_rmdir_ex, name, ".");
    will_return_always(__wrap_rmdir_ex, 0);

    ret = remove_empty_folders(input);

    assert_int_equal(ret, 0);
}

// TODO: Validate this condition is required to be tested
static void test_remove_empty_folders_absolute_path(void **state) {
    char *input = "/home/user1/";
    int ret = -1;

    expect_string(__wrap_wreaddir, name, "/home/user1");
    expect_string(__wrap_wreaddir, name, "/home");
    expect_string(__wrap_wreaddir, name, "");
    will_return_always(__wrap_wreaddir, NULL);

    expect_string_count(__wrap__mdebug1, msg, "Removing empty directory '%s'.", 3);
    expect_string(__wrap__mdebug1, param1, "/home/user1");
    expect_string(__wrap__mdebug1, param1, "/home");
    expect_string(__wrap__mdebug1, param1, "");

    expect_string(__wrap_rmdir_ex, name, "/home/user1");
    expect_string(__wrap_rmdir_ex, name, "/home");
    expect_string(__wrap_rmdir_ex, name, "");
    will_return_always(__wrap_rmdir_ex, 0);

    ret = remove_empty_folders(input);

    assert_int_equal(ret, 0);
}

static void test_remove_empty_folders_non_empty_dir(void **state) {
    char *input = "/var/ossec/queue/diff/local/test-dir/";
    int ret = -1;
    char **subdir;

    os_calloc(2, sizeof(char*), subdir);

    subdir[0] = strdup("some-file.tmp");
    subdir[1] = NULL;

    expect_string(__wrap_wreaddir, name, "/var/ossec/queue/diff/local/test-dir");
    will_return(__wrap_wreaddir, subdir);

    ret = remove_empty_folders(input);

    assert_int_equal(ret, 0);
}

static void test_remove_empty_folders_error_removing_dir(void **state) {
    char *input = "/var/ossec/queue/diff/local/test-dir/";
    int ret = -1;

    expect_string(__wrap_wreaddir, name, "/var/ossec/queue/diff/local/test-dir");
    will_return(__wrap_wreaddir, NULL);

    expect_string(__wrap__mdebug1, msg, "Removing empty directory '%s'.");
    expect_string(__wrap__mdebug1, param1, "/var/ossec/queue/diff/local/test-dir");

    expect_string(__wrap_rmdir_ex, name, "/var/ossec/queue/diff/local/test-dir");
    will_return(__wrap_rmdir_ex, -1);

    expect_string(__wrap__mwarn, msg, "Empty directory '%s' couldn't be deleted. ('%s')");
    expect_string(__wrap__mwarn, param1, "/var/ossec/queue/diff/local/test-dir");
    expect_string(__wrap__mwarn, param2, "Directory not empty");

    ret = remove_empty_folders(input);

    assert_int_equal(ret, 1);
}

/* sk_decode_sum tests */
static void test_sk_decode_sum_no_decode(void **state) {
    int ret = sk_decode_sum(NULL, "-1", NULL);

    assert_int_equal(ret, 1);
}

static void test_sk_decode_sum_deleted_file(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("-1");

    ret = sk_decode_sum(NULL, data->c_sum, NULL);

    assert_int_equal(ret, 1);
}

static void test_sk_decode_sum_no_perm(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, -1);
    assert_ptr_equal(data->sum.size, data->c_sum);
}

static void test_sk_decode_sum_missing_separator(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, data->c_sum);
}

static void test_sk_decode_sum_no_uid(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size::");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
}

static void test_sk_decode_sum_no_gid(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
}

static void test_sk_decode_sum_no_md5(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
}

static void test_sk_decode_sum_no_sha1(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
}

static void test_sk_decode_sum_no_new_fields(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
}

static void test_sk_decode_sum_win_perm_string(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:win_perm:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_string_equal(data->sum.win_perm, "win_perm");
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");

}

static void test_sk_decode_sum_win_perm_encoded(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:|account,0,4:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_string_equal(data->sum.win_perm, "account (allowed): append_data");
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
}

static void test_sk_decode_sum_no_gname(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b:");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
}

static void test_sk_decode_sum_no_uname(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b:"
                         ":gname");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.gname, "gname");
}

static void test_sk_decode_sum_no_mtime(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b:"
                         "uname:gname");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.gname, "gname");
    assert_string_equal(data->sum.uname, "uname");
}

static void test_sk_decode_sum_no_inode(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b:"
                         "uname:gname:2345");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.gname, "gname");
    assert_string_equal(data->sum.uname, "uname");
}

static void test_sk_decode_sum_no_sha256(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b:"
                         "uname:gname:2345:3456");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.gname, "gname");
    assert_string_equal(data->sum.uname, "uname");
    assert_null(data->sum.sha256);
}

static void test_sk_decode_sum_empty_sha256(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b:"
                         "uname:gname:2345:3456::");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.gname, "gname");
    assert_string_equal(data->sum.uname, "uname");
    assert_string_equal(data->sum.sha256, "");
    assert_int_equal(data->sum.mtime, 2345);
    assert_int_equal(data->sum.inode, 3456);
}

static void test_sk_decode_sum_no_attributes(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b:"
                         "uname:gname:2345:3456:"
                         "672a8ceaea40a441f0268ca9bbb33e99f9643c6262667b61fbe57694df224d40");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.gname, "gname");
    assert_string_equal(data->sum.uname, "uname");
    assert_string_equal(data->sum.sha256, "672a8ceaea40a441f0268ca9bbb33e99f9643c6262667b61fbe57694df224d40");
    assert_int_equal(data->sum.mtime, 2345);
    assert_int_equal(data->sum.inode, 3456);
}

static void test_sk_decode_sum_non_numeric_attributes(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b:"
                         "uname:gname:2345:3456:"
                         "672a8ceaea40a441f0268ca9bbb33e99f9643c6262667b61fbe57694df224d40:"
                         "attributes");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.gname, "gname");
    assert_string_equal(data->sum.uname, "uname");
    assert_string_equal(data->sum.sha256, "672a8ceaea40a441f0268ca9bbb33e99f9643c6262667b61fbe57694df224d40");
    assert_int_equal(data->sum.mtime, 2345);
    assert_int_equal(data->sum.inode, 3456);
    assert_string_equal(data->sum.attributes, "attributes");
}

static void test_sk_decode_sum_win_encoded_attributes(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b:"
                         "uname:gname:2345:3456:"
                         "672a8ceaea40a441f0268ca9bbb33e99f9643c6262667b61fbe57694df224d40:"
                         "1");

    ret = sk_decode_sum(&data->sum, data->c_sum, NULL);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.gname, "gname");
    assert_string_equal(data->sum.uname, "uname");
    assert_string_equal(data->sum.sha256, "672a8ceaea40a441f0268ca9bbb33e99f9643c6262667b61fbe57694df224d40");
    assert_int_equal(data->sum.mtime, 2345);
    assert_int_equal(data->sum.inode, 3456);
    assert_string_equal(data->sum.attributes, "READONLY");
}

static void test_sk_decode_sum_extra_data_empty(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
}

static void test_sk_decode_sum_extra_data_no_user_name(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
}

static void test_sk_decode_sum_extra_data_no_group_id(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
}

static void test_sk_decode_sum_extra_data_no_group_name(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
}

static void test_sk_decode_sum_extra_data_no_process_name(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
}

static void test_sk_decode_sum_extra_data_no_audit_uid(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
    assert_string_equal(data->sum.wdata.process_name, "process_name");
}

static void test_sk_decode_sum_extra_data_no_audit_name(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name:"
                         "audit_uid");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
    assert_string_equal(data->sum.wdata.process_name, "process_name");
    assert_string_equal(data->sum.wdata.audit_uid, "audit_uid");
}

static void test_sk_decode_sum_extra_data_no_effective_uid(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name:"
                         "audit_uid:audit_name");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
    assert_string_equal(data->sum.wdata.process_name, "process_name");
    assert_string_equal(data->sum.wdata.audit_uid, "audit_uid");
    assert_string_equal(data->sum.wdata.audit_name, "audit_name");
}

static void test_sk_decode_sum_extra_data_no_effective_name(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name:"
                         "audit_uid:audit_name:effective_uid");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
    assert_string_equal(data->sum.wdata.process_name, "process_name");
    assert_string_equal(data->sum.wdata.audit_uid, "audit_uid");
    assert_string_equal(data->sum.wdata.audit_name, "audit_name");
    assert_string_equal(data->sum.wdata.effective_uid, "effective_uid");
}

static void test_sk_decode_sum_extra_data_no_ppid(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name:"
                         "audit_uid:audit_name:effective_uid:effective_name");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
    assert_string_equal(data->sum.wdata.process_name, "process_name");
    assert_string_equal(data->sum.wdata.audit_uid, "audit_uid");
    assert_string_equal(data->sum.wdata.audit_name, "audit_name");
    assert_string_equal(data->sum.wdata.effective_uid, "effective_uid");
    assert_string_equal(data->sum.wdata.effective_name, "effective_name");
}

static void test_sk_decode_sum_extra_data_no_process_id(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name:"
                         "audit_uid:audit_name:effective_uid:effective_name:ppid");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, -1);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
    assert_string_equal(data->sum.wdata.process_name, "process_name");
    assert_string_equal(data->sum.wdata.audit_uid, "audit_uid");
    assert_string_equal(data->sum.wdata.audit_name, "audit_name");
    assert_string_equal(data->sum.wdata.effective_uid, "effective_uid");
    assert_string_equal(data->sum.wdata.effective_name, "effective_name");
    assert_string_equal(data->sum.wdata.ppid, "ppid");
}

static void test_sk_decode_sum_extra_data_no_tag(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name:"
                         "audit_uid:audit_name:effective_uid:effective_name:ppid:process_id");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
    assert_string_equal(data->sum.wdata.process_name, "process_name");
    assert_string_equal(data->sum.wdata.audit_uid, "audit_uid");
    assert_string_equal(data->sum.wdata.audit_name, "audit_name");
    assert_string_equal(data->sum.wdata.effective_uid, "effective_uid");
    assert_string_equal(data->sum.wdata.effective_name, "effective_name");
    assert_string_equal(data->sum.wdata.ppid, "ppid");
    assert_string_equal(data->sum.wdata.process_id, "process_id");
    assert_null(data->sum.tag);
    assert_null(data->sum.symbolic_path);
}

static void test_sk_decode_sum_extra_data_no_symbolic_path(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name:"
                         "audit_uid:audit_name:effective_uid:effective_name:"
                         "ppid:process_id:tag");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
    assert_string_equal(data->sum.wdata.process_name, "process_name");
    assert_string_equal(data->sum.wdata.audit_uid, "audit_uid");
    assert_string_equal(data->sum.wdata.audit_name, "audit_name");
    assert_string_equal(data->sum.wdata.effective_uid, "effective_uid");
    assert_string_equal(data->sum.wdata.effective_name, "effective_name");
    assert_string_equal(data->sum.wdata.ppid, "ppid");
    assert_string_equal(data->sum.wdata.process_id, "process_id");
    assert_string_equal(data->sum.tag, "tag");
    assert_null(data->sum.symbolic_path);
}

static void test_sk_decode_sum_extra_data_no_inode(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name:"
                         "audit_uid:audit_name:effective_uid:effective_name:"
                         "ppid:process_id:tag:symbolic_path");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
    assert_string_equal(data->sum.wdata.process_name, "process_name");
    assert_string_equal(data->sum.wdata.audit_uid, "audit_uid");
    assert_string_equal(data->sum.wdata.audit_name, "audit_name");
    assert_string_equal(data->sum.wdata.effective_uid, "effective_uid");
    assert_string_equal(data->sum.wdata.effective_name, "effective_name");
    assert_string_equal(data->sum.wdata.ppid, "ppid");
    assert_string_equal(data->sum.wdata.process_id, "process_id");
    assert_string_equal(data->sum.tag, "tag");
    assert_string_equal(data->sum.symbolic_path, "symbolic_path");
}

static void test_sk_decode_sum_extra_data_all_fields(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name:"
                         "audit_uid:audit_name:effective_uid:effective_name:"
                         "ppid:process_id:tag:symbolic_path:-");

    data->sum.silent = 0;

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
    assert_string_equal(data->sum.wdata.process_name, "process_name");
    assert_string_equal(data->sum.wdata.audit_uid, "audit_uid");
    assert_string_equal(data->sum.wdata.audit_name, "audit_name");
    assert_string_equal(data->sum.wdata.effective_uid, "effective_uid");
    assert_string_equal(data->sum.wdata.effective_name, "effective_name");
    assert_string_equal(data->sum.wdata.ppid, "ppid");
    assert_string_equal(data->sum.wdata.process_id, "process_id");
    assert_string_equal(data->sum.tag, "tag");
    assert_string_equal(data->sum.symbolic_path, "symbolic_path");
    assert_int_equal(data->sum.silent, 0);
}

static void test_sk_decode_sum_extra_data_all_fields_silent(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name:"
                         "audit_uid:audit_name:effective_uid:effective_name:"
                         "ppid:process_id:tag:symbolic_path:+");

    data->sum.silent = 0;

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
    assert_string_equal(data->sum.wdata.process_name, "process_name");
    assert_string_equal(data->sum.wdata.audit_uid, "audit_uid");
    assert_string_equal(data->sum.wdata.audit_name, "audit_name");
    assert_string_equal(data->sum.wdata.effective_uid, "effective_uid");
    assert_string_equal(data->sum.wdata.effective_name, "effective_name");
    assert_string_equal(data->sum.wdata.ppid, "ppid");
    assert_string_equal(data->sum.wdata.process_id, "process_id");
    assert_string_equal(data->sum.tag, "tag");
    assert_string_equal(data->sum.symbolic_path, "symbolic_path");
    assert_int_equal(data->sum.silent, 1);
}

static void test_sk_decode_sum_extra_data_null_ppid(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name:"
                         "audit_uid:audit_name:effective_uid:effective_name:"
                         "-:process_id:tag:symbolic_path:+");

    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);

    assert_int_equal(ret, 0);
    assert_string_equal(data->sum.size, "size");
    assert_int_equal(data->sum.perm, 1234);
    assert_string_equal(data->sum.uid, "uid");
    assert_string_equal(data->sum.gid, "gid");
    assert_string_equal(data->sum.md5, "3691689a513ace7e508297b583d7050d");
    assert_string_equal(data->sum.sha1, "07f05add1049244e7e71ad0f54f24d8094cd8f8b");
    assert_string_equal(data->sum.wdata.user_id, "user_id");
    assert_string_equal(data->sum.wdata.user_name, "user_name");
    assert_string_equal(data->sum.wdata.group_id, "group_id");
    assert_string_equal(data->sum.wdata.group_name, "group_name");
    assert_string_equal(data->sum.wdata.process_name, "process_name");
    assert_string_equal(data->sum.wdata.audit_uid, "audit_uid");
    assert_string_equal(data->sum.wdata.audit_name, "audit_name");
    assert_string_equal(data->sum.wdata.effective_uid, "effective_uid");
    assert_string_equal(data->sum.wdata.effective_name, "effective_name");
    assert_null(data->sum.wdata.ppid);
    assert_string_equal(data->sum.wdata.process_id, "process_id");
    assert_string_equal(data->sum.tag, "tag");
    assert_string_equal(data->sum.symbolic_path, "symbolic_path");
    assert_int_equal(data->sum.silent, 1);
}

// TODO: Validate this condition is required to be tested
static void test_sk_decode_sum_extra_data_null_sum(void **state) {
    sk_decode_data_t *data = *state;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b");

    int ret = 123456789;

    ret = sk_decode_sum(NULL, data->c_sum, NULL);

    assert_int_equal(ret, 0);
}

// TODO: Validate this condition is required to be tested
static void test_sk_decode_sum_extra_data_null_c_sum(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 123456789;

    ret = sk_decode_sum(&data->sum, NULL, NULL);

    assert_int_equal(ret, 0);
}

/* sk_decode_extradata tests */
// TODO: Validate this condition is required to be tested
static void test_sk_decode_extradata_null_c_sum(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 12345;

    ret = sk_decode_extradata(&data->sum, NULL);

    assert_int_equal(ret, 0);
}

static void test_sk_decode_extradata_no_changes(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 12345;

    data->c_sum = strdup("some string");

    ret = sk_decode_extradata(&data->sum, data->c_sum);

    assert_int_equal(ret, 0);
    assert_string_equal(data->c_sum, "some string");
}

static void test_sk_decode_extradata_no_date_alert(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 12345;

    data->c_sum = strdup("some string!15");

    ret = sk_decode_extradata(&data->sum, data->c_sum);

    assert_int_equal(ret, 0);
    assert_string_equal(data->c_sum, "some string");
    assert_string_equal(data->c_sum + 12, "15");
}

static void test_sk_decode_extradata_no_sym_path(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 12345;

    data->c_sum = strdup("some string!15:20");

    data->sum.symbolic_path = NULL;

    ret = sk_decode_extradata(&data->sum, data->c_sum);

    assert_int_equal(ret, 1);
    assert_string_equal(data->c_sum, "some string");
    assert_string_equal(data->c_sum + 12, "15");
    assert_string_equal(data->c_sum + 15, "20");
    assert_int_equal(data->sum.changes, 15);
    assert_int_equal(data->sum.date_alert, 20);
    assert_null(data->sum.symbolic_path);
}

static void test_sk_decode_extradata_all_fields(void **state) {
    sk_decode_data_t *data = *state;
    int ret = 12345;

    data->c_sum = strdup("some string!15:20:a symbolic path");

    data->sum.symbolic_path = NULL;

    ret = sk_decode_extradata(&data->sum, data->c_sum);

    assert_int_equal(ret, 1);
    assert_string_equal(data->c_sum, "some string");
    assert_string_equal(data->c_sum + 12, "15");
    assert_string_equal(data->c_sum + 15, "20");
    assert_int_equal(data->sum.changes, 15);
    assert_int_equal(data->sum.date_alert, 20);
    assert_string_equal(data->sum.symbolic_path, "a symbolic path");
}

/* sk_fill_event tests */
static void test_sk_fill_event_full_event(void **state) {
    sk_fill_event_t *data = *state;

    data->f_name = strdup("f_name");

    data->sum.size = "size";
    data->sum.perm = 123456; // 361100 in octal
    data->sum.uid = "uid";
    data->sum.gid = "gid";
    data->sum.md5 = "md5";
    data->sum.sha1 = "sha1";
    data->sum.uname = "uname";
    data->sum.gname = "gname";
    data->sum.mtime = 2345678;
    data->sum.inode = 3456789;
    data->sum.sha256 = "sha256";
    data->sum.attributes = "attributes";
    data->sum.wdata.user_id = "user_id";
    data->sum.wdata.user_name = "user_name";
    data->sum.wdata.group_id = "group_id";
    data->sum.wdata.group_name = "group_name";
    data->sum.wdata.process_name = "process_name";
    data->sum.wdata.audit_uid = "audit_uid";
    data->sum.wdata.audit_name = "audit_name";
    data->sum.wdata.effective_uid = "effective_uid";
    data->sum.wdata.effective_name = "effective_name";
    data->sum.wdata.ppid = "ppid";
    data->sum.wdata.process_id = "process_id";
    data->sum.tag = "tag";
    data->sum.symbolic_path = "symbolic_path";

    sk_fill_event(&data->lf, data->f_name, &data->sum);

    assert_string_equal(data->lf.filename, "f_name");
    assert_string_equal(data->lf.fields[FIM_FILE].value, "f_name");
    assert_string_equal(data->lf.fields[FIM_SIZE].value, "size");
    assert_string_equal(data->lf.fields[FIM_PERM].value, "361100");
    assert_string_equal(data->lf.fields[FIM_UID].value, "uid");
    assert_string_equal(data->lf.fields[FIM_GID].value, "gid");
    assert_string_equal(data->lf.fields[FIM_MD5].value, "md5");
    assert_string_equal(data->lf.fields[FIM_SHA1].value, "sha1");
    assert_string_equal(data->lf.fields[FIM_UNAME].value, "uname");
    assert_string_equal(data->lf.fields[FIM_GNAME].value, "gname");
    assert_int_equal(data->lf.mtime_after, data->sum.mtime);
    assert_string_equal(data->lf.fields[FIM_MTIME].value, "2345678");
    assert_int_equal(data->lf.inode_after, data->sum.inode);
    assert_string_equal(data->lf.fields[FIM_INODE].value, "3456789");
    assert_string_equal(data->lf.fields[FIM_SHA256].value, "sha256");
    assert_string_equal(data->lf.fields[FIM_ATTRS].value, "attributes");

    assert_string_equal(data->lf.user_id, "user_id");
    assert_string_equal(data->lf.fields[FIM_USER_ID].value, "user_id");

    assert_string_equal(data->lf.user_name, "user_name");
    assert_string_equal(data->lf.fields[FIM_USER_NAME].value, "user_name");

    assert_string_equal(data->lf.group_id, "group_id");
    assert_string_equal(data->lf.fields[FIM_GROUP_ID].value, "group_id");

    assert_string_equal(data->lf.group_name, "group_name");
    assert_string_equal(data->lf.fields[FIM_GROUP_NAME].value, "group_name");

    assert_string_equal(data->lf.process_name, "process_name");
    assert_string_equal(data->lf.fields[FIM_PROC_NAME].value, "process_name");

    assert_string_equal(data->lf.audit_uid, "audit_uid");
    assert_string_equal(data->lf.fields[FIM_AUDIT_ID].value, "audit_uid");

    assert_string_equal(data->lf.audit_name, "audit_name");
    assert_string_equal(data->lf.fields[FIM_AUDIT_NAME].value, "audit_name");

    assert_string_equal(data->lf.effective_uid, "effective_uid");
    assert_string_equal(data->lf.fields[FIM_EFFECTIVE_UID].value, "effective_uid");

    assert_string_equal(data->lf.effective_name, "effective_name");
    assert_string_equal(data->lf.fields[FIM_EFFECTIVE_NAME].value, "effective_name");

    assert_string_equal(data->lf.ppid, "ppid");
    assert_string_equal(data->lf.fields[FIM_PPID].value, "ppid");

    assert_string_equal(data->lf.process_id, "process_id");
    assert_string_equal(data->lf.fields[FIM_PROC_ID].value, "process_id");

    assert_string_equal(data->lf.sk_tag, "tag");
    assert_string_equal(data->lf.fields[FIM_TAG].value, "tag");

    assert_string_equal(data->lf.sym_path, "symbolic_path");
    assert_string_equal(data->lf.fields[FIM_SYM_PATH].value, "symbolic_path");
}

static void test_sk_fill_event_empty_event(void **state) {
    sk_fill_event_t *data = *state;

    data->f_name = strdup("f_name");

    sk_fill_event(&data->lf, data->f_name, &data->sum);

    assert_string_equal(data->lf.filename, "f_name");
    assert_string_equal(data->lf.fields[FIM_FILE].value, "f_name");
    assert_null(data->lf.fields[FIM_SIZE].value);
    assert_null(data->lf.fields[FIM_PERM].value);
    assert_null(data->lf.fields[FIM_UID].value);
    assert_null(data->lf.fields[FIM_GID].value);
    assert_null(data->lf.fields[FIM_MD5].value);
    assert_null(data->lf.fields[FIM_SHA1].value);
    assert_null(data->lf.fields[FIM_UNAME].value);
    assert_null(data->lf.fields[FIM_GNAME].value);
    assert_int_equal(data->lf.mtime_after, data->sum.mtime);
    assert_null(data->lf.fields[FIM_MTIME].value);
    assert_int_equal(data->lf.inode_after, data->sum.inode);
    assert_null(data->lf.fields[FIM_INODE].value);
    assert_null(data->lf.fields[FIM_SHA256].value);
    assert_null(data->lf.fields[FIM_ATTRS].value);

    assert_null(data->lf.user_id);
    assert_null(data->lf.fields[FIM_USER_ID].value);

    assert_null(data->lf.user_name);
    assert_null(data->lf.fields[FIM_USER_NAME].value);

    assert_null(data->lf.group_id);
    assert_null(data->lf.fields[FIM_GROUP_ID].value);

    assert_null(data->lf.group_name);
    assert_null(data->lf.fields[FIM_GROUP_NAME].value);

    assert_null(data->lf.process_name);
    assert_null(data->lf.fields[FIM_PROC_NAME].value);

    assert_null(data->lf.audit_uid);
    assert_null(data->lf.fields[FIM_AUDIT_ID].value);

    assert_null(data->lf.audit_name);
    assert_null(data->lf.fields[FIM_AUDIT_NAME].value);

    assert_null(data->lf.effective_uid);
    assert_null(data->lf.fields[FIM_EFFECTIVE_UID].value);

    assert_null(data->lf.effective_name);
    assert_null(data->lf.fields[FIM_EFFECTIVE_NAME].value);

    assert_null(data->lf.ppid);
    assert_null(data->lf.fields[FIM_PPID].value);

    assert_null(data->lf.process_id);
    assert_null(data->lf.fields[FIM_PROC_ID].value);

    assert_null(data->lf.sk_tag);
    assert_null(data->lf.fields[FIM_TAG].value);

    assert_null(data->lf.sym_path);
    assert_null(data->lf.fields[FIM_SYM_PATH].value);
}

static void test_sk_fill_event_win_perm(void **state) {
    sk_fill_event_t *data = *state;

    data->f_name = strdup("f_name");

    data->sum.win_perm = "win_perm";

    sk_fill_event(&data->lf, data->f_name, &data->sum);

    assert_string_equal(data->lf.filename, "f_name");
    assert_string_equal(data->lf.fields[FIM_FILE].value, "f_name");
    assert_null(data->lf.fields[FIM_SIZE].value);
    assert_string_equal(data->lf.fields[FIM_PERM].value, "win_perm");
    assert_null(data->lf.fields[FIM_UID].value);
    assert_null(data->lf.fields[FIM_GID].value);
    assert_null(data->lf.fields[FIM_MD5].value);
    assert_null(data->lf.fields[FIM_SHA1].value);
    assert_null(data->lf.fields[FIM_UNAME].value);
    assert_null(data->lf.fields[FIM_GNAME].value);
    assert_int_equal(data->lf.mtime_after, data->sum.mtime);
    assert_null(data->lf.fields[FIM_MTIME].value);
    assert_int_equal(data->lf.inode_after, data->sum.inode);
    assert_null(data->lf.fields[FIM_INODE].value);
    assert_null(data->lf.fields[FIM_SHA256].value);
    assert_null(data->lf.fields[FIM_ATTRS].value);

    assert_null(data->lf.user_id);
    assert_null(data->lf.fields[FIM_USER_ID].value);

    assert_null(data->lf.user_name);
    assert_null(data->lf.fields[FIM_USER_NAME].value);

    assert_null(data->lf.group_id);
    assert_null(data->lf.fields[FIM_GROUP_ID].value);

    assert_null(data->lf.group_name);
    assert_null(data->lf.fields[FIM_GROUP_NAME].value);

    assert_null(data->lf.process_name);
    assert_null(data->lf.fields[FIM_PROC_NAME].value);

    assert_null(data->lf.audit_uid);
    assert_null(data->lf.fields[FIM_AUDIT_ID].value);

    assert_null(data->lf.audit_name);
    assert_null(data->lf.fields[FIM_AUDIT_NAME].value);

    assert_null(data->lf.effective_uid);
    assert_null(data->lf.fields[FIM_EFFECTIVE_UID].value);

    assert_null(data->lf.effective_name);
    assert_null(data->lf.fields[FIM_EFFECTIVE_NAME].value);

    assert_null(data->lf.ppid);
    assert_null(data->lf.fields[FIM_PPID].value);

    assert_null(data->lf.process_id);
    assert_null(data->lf.fields[FIM_PROC_ID].value);

    assert_null(data->lf.sk_tag);
    assert_null(data->lf.fields[FIM_TAG].value);

    assert_null(data->lf.sym_path);
    assert_null(data->lf.fields[FIM_SYM_PATH].value);
}

// TODO: Validate this condition is required to be tested
static void test_sk_fill_event_null_eventinfo(void **state) {
    sk_fill_event_t *data = *state;

    data->f_name = strdup("f_name");

    data->sum.win_perm = "win_perm";

    sk_fill_event(NULL, data->f_name, &data->sum);
}

// TODO: Validate this condition is required to be tested
static void test_sk_fill_event_null_f_name(void **state) {
    sk_fill_event_t *data = *state;

    data->f_name = strdup("f_name");

    data->sum.win_perm = "win_perm";

    sk_fill_event(&data->lf, NULL, &data->sum);
}

// TODO: Validate this condition is required to be tested
static void test_sk_fill_event_null_sum(void **state) {
    sk_fill_event_t *data = *state;

    data->f_name = strdup("f_name");

    data->sum.win_perm = "win_perm";

    sk_fill_event(&data->lf, data->f_name, NULL);
}

/* sk_build_sum tests */
static void test_sk_build_sum_full_message(void **state) {
    sk_build_sum_t *data = *state;
    int ret;

    data->sum.size = "size";
    data->sum.perm = 123456;
    data->sum.win_perm = NULL;
    data->sum.uid = "uid";
    data->sum.gid = "gid";
    data->sum.md5 = "md5";
    data->sum.sha1 = "sha1";
    data->sum.uname = "username";
    data->sum.gname = "gname";
    data->sum.mtime = 234567;
    data->sum.inode = 345678;
    data->sum.sha256 = "sha256";
    data->sum.attributes = "attributes";
    data->sum.changes = 456789;
    data->sum.date_alert = 567890;

    ret = sk_build_sum(&data->sum, data->output, OS_MAXSTR);

    assert_int_equal(ret, 0);
    assert_string_equal(data->output,
                        "size:123456:uid:gid:md5:sha1:username:gname:234567:345678:sha256:attributes!456789:567890");
}

static void test_sk_build_sum_skip_fields_message(void **state) {
    sk_build_sum_t *data = *state;
    int ret;

    data->sum.size = "size";
    data->sum.perm = 0;
    data->sum.win_perm = NULL;
    data->sum.uid = "uid";
    data->sum.gid = "gid";
    data->sum.md5 = "md5";
    data->sum.sha1 = "sha1";
    data->sum.uname = NULL;
    data->sum.gname = NULL;
    data->sum.mtime = 0;
    data->sum.inode = 0;
    data->sum.sha256 = NULL;
    data->sum.attributes = NULL;
    data->sum.changes = 0;
    data->sum.date_alert = 0;

    ret = sk_build_sum(&data->sum, data->output, OS_MAXSTR);

    assert_int_equal(ret, 0);
    assert_string_equal(data->output, "size::uid:gid:md5:sha1::::::!0:0");
}

static void test_sk_build_sum_win_perm(void **state) {
    sk_build_sum_t *data = *state;
    int ret;

    data->sum.size = "size";
    data->sum.perm = 0;
    data->sum.win_perm = "win_perm";
    data->sum.uid = "uid";
    data->sum.gid = "gid";
    data->sum.md5 = "md5";
    data->sum.sha1 = "sha1";
    data->sum.uname = NULL;
    data->sum.gname = NULL;
    data->sum.mtime = 0;
    data->sum.inode = 0;
    data->sum.sha256 = NULL;
    data->sum.attributes = NULL;
    data->sum.changes = 0;
    data->sum.date_alert = 0;

    ret = sk_build_sum(&data->sum, data->output, OS_MAXSTR);

    assert_int_equal(ret, 0);
    assert_string_equal(data->output, "size:win_perm:uid:gid:md5:sha1::::::!0:0");
}

static void test_sk_build_sum_insufficient_buffer_size(void **state) {
    sk_build_sum_t *data = *state;
    int ret;

    data->sum.size = "size";
    data->sum.perm = 0;
    data->sum.win_perm = "win_perm";
    data->sum.uid = "uid";
    data->sum.gid = "gid";
    data->sum.md5 = "md5";
    data->sum.sha1 = "sha1";
    data->sum.uname = NULL;
    data->sum.gname = NULL;
    data->sum.mtime = 0;
    data->sum.inode = 0;
    data->sum.sha256 = NULL;
    data->sum.attributes = NULL;
    data->sum.changes = 0;
    data->sum.date_alert = 0;

    ret = sk_build_sum(&data->sum, data->output, 10);

    assert_int_equal(ret, -1);
    assert_string_equal(data->output, "size:win_");
}

// TODO: Validate this condition is required to be tested
static void test_sk_build_sum_null_sum(void **state) {
    sk_build_sum_t *data = *state;
    int ret;

    ret = sk_build_sum(NULL, data->output, OS_MAXSTR);

    assert_int_equal(ret, 0);
}

// TODO: Validate this condition is required to be tested
static void test_sk_build_sum_null_output(void **state) {
    sk_build_sum_t *data = *state;
    int ret;

    ret = sk_build_sum(&data->sum, NULL, OS_MAXSTR);

    assert_int_equal(ret, 0);
}

/* sk_sum_clean tests */
static void test_sk_sum_clean_full_message(void **state) {
    sk_decode_data_t *data = *state;
    int ret;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b:"
                         "uname:gname:2345:3456:"
                         "672a8ceaea40a441f0268ca9bbb33e99f9643c6262667b61fbe57694df224d40:"
                         "1");
    data->w_sum = strdup("user_id:user_name:group_id:group_name:process_name:"
                         "audit_uid:audit_name:effective_uid:effective_name:"
                         "ppid:process_id:tag:symbolic_path:-");

    // Fill sum with as many info as possible
    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);
    assert_int_equal(ret, 0);

    // And free it
    sk_sum_clean(&data->sum);

    assert_null(data->sum.symbolic_path);
    assert_null(data->sum.attributes);
    assert_null(data->sum.wdata.user_name);
    assert_null(data->sum.wdata.process_name);
    assert_null(data->sum.uname);
    assert_null(data->sum.win_perm);
}

static void test_sk_sum_clean_shortest_valid_message(void **state) {
    sk_decode_data_t *data = *state;
    int ret;

    data->c_sum = strdup("size:1234:uid:gid:"
                         "3691689a513ace7e508297b583d7050d:"
                         "07f05add1049244e7e71ad0f54f24d8094cd8f8b:"
                         "uname:gname:2345:3456");

    // Fill sum with as many info as possible
    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);
    assert_int_equal(ret, 0);

    // And free it
    sk_sum_clean(&data->sum);

    assert_null(data->sum.symbolic_path);
    assert_null(data->sum.attributes);
    assert_null(data->sum.wdata.user_name);
    assert_null(data->sum.wdata.process_name);
    assert_null(data->sum.uname);
    assert_null(data->sum.win_perm);
}

static void test_sk_sum_clean_invalid_message(void **state) {
    sk_decode_data_t *data = *state;
    int ret;

    data->c_sum = strdup("This is not a valid syscheck message");

    // Fill sum with as many info as possible
    ret = sk_decode_sum(&data->sum, data->c_sum, data->w_sum);
    assert_int_equal(ret, 0);

    // And free it
    sk_sum_clean(&data->sum);

    assert_null(data->sum.symbolic_path);
    assert_null(data->sum.attributes);
    assert_null(data->sum.wdata.user_name);
    assert_null(data->sum.wdata.process_name);
    assert_null(data->sum.uname);
    assert_null(data->sum.win_perm);
}

// TODO: Validate this condition is required to be tested
static void test_sk_sum_clean_null_sum(void **state) {
    sk_sum_clean(NULL);
}

/* unescape_syscheck_field tests */
static void test_unescape_syscheck_field_escaped_chars(void **state) {
    unescape_syscheck_field_data_t *data = *state;

    data->input = strdup("Hi\\!! This\\ is\\ a string with\\: escaped chars:");

    data->output = unescape_syscheck_field(data->input);

    assert_string_equal(data->output, "Hi!! This is a string with: escaped chars:");
}

static void test_unescape_syscheck_field_no_escaped_chars(void **state) {
    unescape_syscheck_field_data_t *data = *state;

    data->input = strdup("Hi!! This is a string without: escaped chars:");

    data->output = unescape_syscheck_field(data->input);

    assert_string_equal(data->output, "Hi!! This is a string without: escaped chars:");
}

static void test_unescape_syscheck_null_input(void **state) {
    unescape_syscheck_field_data_t *data = *state;

    data->input = NULL;

    data->output = unescape_syscheck_field(data->input);

    assert_null(data->output);
}

static void test_unescape_syscheck_empty_string(void **state) {
    unescape_syscheck_field_data_t *data = *state;

    data->input = strdup("");

    data->output = unescape_syscheck_field(data->input);

    assert_null(data->output);
}

/* get_user tests */
static void test_get_user_success(void **state) {
    char *user;

    will_return(__wrap_getpwuid_r, "user_name");
    will_return(__wrap_getpwuid_r, 1);
    #ifndef SOLARIS
    will_return(__wrap_getpwuid_r, 0);
    #endif

    user = get_user(NULL, 1, NULL);

    *state = user;

    assert_string_equal(user, "user_name");
}

static void test_get_user_uid_not_found(void **state) {
    char *user;

    will_return(__wrap_getpwuid_r, "user_name");
    will_return(__wrap_getpwuid_r, NULL);
    #ifndef SOLARIS
    will_return(__wrap_getpwuid_r, 0);
    #endif

    expect_string(__wrap__mdebug2, msg, "User with uid '%d' not found.\n");
    expect_value(__wrap__mdebug2, param1, 1);

    user = get_user(NULL, 1, NULL);

    *state = user;

    assert_null(user);
}

static void test_get_user_error(void **state) {
    char *user;

    will_return(__wrap_getpwuid_r, "user_name");
    will_return(__wrap_getpwuid_r, NULL);
    #ifndef SOLARIS
    will_return(__wrap_getpwuid_r, ENOENT);
    #endif

    expect_string(__wrap__mdebug2, msg, "Failed getting user_name (%d): '%s'\n");
    expect_value(__wrap__mdebug2, param1, ENOENT);
    expect_string(__wrap__mdebug2, param2, "No such file or directory");

    user = get_user(NULL, 1, NULL);

    *state = user;

    assert_null(user);
}

/* get_group tests */
static void test_get_group_success(void **state) {
    struct group group = { .gr_name = "group" };
    const char *output;

    will_return(__wrap_getgrgid, &group);

    output = get_group(0);

    assert_ptr_equal(output, group.gr_name);
}

static void test_get_group_failure(void **state) {
    const char *output;

    will_return(__wrap_getgrgid, NULL);

    output = get_group(0);

    assert_string_equal(output, "");
}

/* ag_send_syscheck tests */
/* ag_send_syscheck does not modify inputs or return anything, so there are no asserts */
/* validation of this function is done through the wrapped functions. */
static void test_ag_send_syscheck_success(void **state) {
    char *input = "This is a mock message, it wont be sent anywhere";

    expect_string(__wrap_OS_ConnectUnixDomain, path, SYS_LOCAL_SOCK);
    expect_value(__wrap_OS_ConnectUnixDomain, type, SOCK_STREAM);
    expect_value(__wrap_OS_ConnectUnixDomain, max_msg_size, OS_MAXSTR);

    will_return(__wrap_OS_ConnectUnixDomain, 1234);

    expect_value(__wrap_OS_SendSecureTCP, sock, 1234);
    expect_value(__wrap_OS_SendSecureTCP, size, 48);
    expect_string(__wrap_OS_SendSecureTCP, msg, input);

    will_return(__wrap_OS_SendSecureTCP, 48);

    ag_send_syscheck(input);
}

static void test_ag_send_syscheck_unable_to_connect(void **state) {
    char *input = "This is a mock message, it wont be sent anywhere";

    expect_string(__wrap_OS_ConnectUnixDomain, path, SYS_LOCAL_SOCK);
    expect_value(__wrap_OS_ConnectUnixDomain, type, SOCK_STREAM);
    expect_value(__wrap_OS_ConnectUnixDomain, max_msg_size, OS_MAXSTR);

    will_return(__wrap_OS_ConnectUnixDomain, OS_SOCKTERR);

    errno = EADDRNOTAVAIL;

    expect_string(__wrap__merror, msg, "dbsync: cannot connect to syscheck: %s (%d)");
    expect_string(__wrap__merror, param1, "Cannot assign requested address");
    expect_value(__wrap__merror, param2, EADDRNOTAVAIL);

    ag_send_syscheck(input);

    errno = 0;
}
static void test_ag_send_syscheck_error_sending_message(void **state) {
    char *input = "This is a mock message, it wont be sent anywhere";

    expect_string(__wrap_OS_ConnectUnixDomain, path, SYS_LOCAL_SOCK);
    expect_value(__wrap_OS_ConnectUnixDomain, type, SOCK_STREAM);
    expect_value(__wrap_OS_ConnectUnixDomain, max_msg_size, OS_MAXSTR);

    will_return(__wrap_OS_ConnectUnixDomain, 1234);

    expect_value(__wrap_OS_SendSecureTCP, sock, 1234);
    expect_value(__wrap_OS_SendSecureTCP, size, 48);
    expect_string(__wrap_OS_SendSecureTCP, msg, input);

    will_return(__wrap_OS_SendSecureTCP, OS_SOCKTERR);

    errno = EWOULDBLOCK;

    expect_string(__wrap__merror, msg, "Cannot send message to syscheck: %s (%d)");
    expect_string(__wrap__merror, param1, "Resource temporarily unavailable");
    expect_value(__wrap__merror, param2, EWOULDBLOCK);

    ag_send_syscheck(input);

    errno = 0;
}

/* decode_win_attributes tests */
static void test_decode_win_attributes_all_attributes(void **state) {
    char str[OS_SIZE_256];
    unsigned int attrs = 0;

    attrs |= FILE_ATTRIBUTE_ARCHIVE     |
             FILE_ATTRIBUTE_COMPRESSED  |
             FILE_ATTRIBUTE_DEVICE      |
             FILE_ATTRIBUTE_DIRECTORY   |
             FILE_ATTRIBUTE_ENCRYPTED   |
             FILE_ATTRIBUTE_HIDDEN      |
             FILE_ATTRIBUTE_NORMAL      |
             FILE_ATTRIBUTE_OFFLINE     |
             FILE_ATTRIBUTE_READONLY    |
             FILE_ATTRIBUTE_SPARSE_FILE |
             FILE_ATTRIBUTE_SYSTEM      |
             FILE_ATTRIBUTE_TEMPORARY   |
             FILE_ATTRIBUTE_VIRTUAL     |
             FILE_ATTRIBUTE_NO_SCRUB_DATA |
             FILE_ATTRIBUTE_REPARSE_POINT |
             FILE_ATTRIBUTE_RECALL_ON_OPEN |
             FILE_ATTRIBUTE_INTEGRITY_STREAM |
             FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
             FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS;

    decode_win_attributes(str, attrs);

    assert_string_equal(str, "ARCHIVE, COMPRESSED, DEVICE, DIRECTORY, ENCRYPTED, "
                             "HIDDEN, INTEGRITY_STREAM, NORMAL, NOT_CONTENT_INDEXED, "
                             "NO_SCRUB_DATA, OFFLINE, READONLY, RECALL_ON_DATA_ACCESS, "
                             "RECALL_ON_OPEN, REPARSE_POINT, SPARSE_FILE, SYSTEM, TEMPORARY, "
                             "VIRTUAL");
}

static void test_decode_win_attributes_no_attributes(void **state) {
    char str[OS_SIZE_256];
    unsigned int attrs = 0;

    decode_win_attributes(str, attrs);

    assert_string_equal(str, "");
}

static void test_decode_win_attributes_some_attributes(void **state) {
    char str[OS_SIZE_256];
    unsigned int attrs = 0;

    attrs |= FILE_ATTRIBUTE_ARCHIVE     |
             FILE_ATTRIBUTE_DEVICE      |
             FILE_ATTRIBUTE_ENCRYPTED   |
             FILE_ATTRIBUTE_NORMAL      |
             FILE_ATTRIBUTE_READONLY    |
             FILE_ATTRIBUTE_SPARSE_FILE |
             FILE_ATTRIBUTE_TEMPORARY   |
             FILE_ATTRIBUTE_NO_SCRUB_DATA |
             FILE_ATTRIBUTE_RECALL_ON_OPEN;

    decode_win_attributes(str, attrs);

    assert_string_equal(str, "ARCHIVE, DEVICE, ENCRYPTED, NORMAL, NO_SCRUB_DATA, "
                             "READONLY, RECALL_ON_OPEN, SPARSE_FILE, TEMPORARY");
}

/* decode_win_permissions tests */
static void test_decode_win_permissions_success_all_permissions(void **state) {
    char *raw_perm = calloc(OS_MAXSTR, sizeof(char));
    char *output;

    snprintf(raw_perm, OS_MAXSTR,  "|account,0,%ld",
        (long int)(GENERIC_READ |
        GENERIC_WRITE |
        GENERIC_EXECUTE |
        GENERIC_ALL |
        DELETE |
        READ_CONTROL |
        WRITE_DAC |
        WRITE_OWNER |
        SYNCHRONIZE |
        FILE_READ_DATA |
        FILE_WRITE_DATA |
        FILE_APPEND_DATA |
        FILE_READ_EA |
        FILE_WRITE_EA |
        FILE_EXECUTE |
        FILE_READ_ATTRIBUTES |
        FILE_WRITE_ATTRIBUTES));

    output = decode_win_permissions(raw_perm);

    os_free(raw_perm);
    *state = output;

    assert_string_equal(output, "account (allowed): generic_read|generic_write|generic_execute|"
        "generic_all|delete|read_control|write_dac|write_owner|synchronize|read_data|write_data|"
        "append_data|read_ea|write_ea|execute|read_attributes|write_attributes");
}

static void test_decode_win_permissions_success_no_permissions(void **state) {
    char *raw_perm = calloc(OS_MAXSTR, sizeof(char));
    char *output;

    snprintf(raw_perm, OS_MAXSTR,  "|account,0,%ld", (long int)0);

    output = decode_win_permissions(raw_perm);

    os_free(raw_perm);
    *state = output;

    assert_string_equal(output, "account (allowed):");
}

static void test_decode_win_permissions_success_some_permissions(void **state) {
    char *raw_perm = calloc(OS_MAXSTR, sizeof(char));
    char *output;

    snprintf(raw_perm, OS_MAXSTR,  "|account,0,%ld",
        (long int)(GENERIC_READ |
        GENERIC_EXECUTE |
        DELETE |
        WRITE_DAC |
        SYNCHRONIZE |
        FILE_WRITE_DATA |
        FILE_READ_EA |
        FILE_EXECUTE |
        FILE_WRITE_ATTRIBUTES));

    output = decode_win_permissions(raw_perm);

    os_free(raw_perm);
    *state = output;

    assert_string_equal(output, "account (allowed): generic_read|generic_execute|"
        "delete|write_dac|synchronize|write_data|read_ea|execute|write_attributes");
}

static void test_decode_win_permissions_success_multiple_accounts(void **state) {
    char *raw_perm = calloc(OS_MAXSTR, sizeof(char));
    char *output;

    snprintf(raw_perm, OS_MAXSTR,  "|first,0,%ld|second,1,%ld", (long int)GENERIC_READ, (long int)GENERIC_EXECUTE);

    output = decode_win_permissions(raw_perm);

    os_free(raw_perm);
    *state = output;

    assert_string_equal(output, "first (allowed): generic_read, second (denied): generic_execute");
}

static void test_decode_win_permissions_fail_no_account_name(void **state) {
    char *raw_perm = "|this wont pass";
    char *output;

    expect_string(__wrap__mdebug1, msg, "The file permissions could not be decoded: '%s'.");
    expect_string(__wrap__mdebug1, param1, "|this wont pass");

    output = decode_win_permissions(raw_perm);

    *state = output;

    assert_null(output);
}

static void test_decode_win_permissions_fail_no_access_type(void **state) {
    char *raw_perm = strdup("|account,this wont pass");
    char *output;

    expect_string(__wrap__mdebug1, msg, "The file permissions could not be decoded: '%s'.");
    expect_string(__wrap__mdebug1, param1, "|account,this wont pass");

    output = decode_win_permissions(raw_perm);

    os_free(raw_perm);
    *state = output;

    assert_null(output);
}

static void test_decode_win_permissions_fail_wrong_format(void **state) {
    char *raw_perm = strdup("this is not the proper format");
    char *output;

    output = decode_win_permissions(raw_perm);

    os_free(raw_perm);
    *state = output;

    assert_null(output);
}

/* attrs_to_json tests */
static void test_attrs_to_json_single_attribute(void **state) {
    char *input = "attribute";
    cJSON *output;
    char *string;

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    output = attrs_to_json(input);

    *state = output;

    string = cJSON_GetStringValue(cJSON_GetArrayItem(output, 0));
    assert_string_equal(string, "attribute");
}

static void test_attrs_to_json_multiple_attributes(void **state) {
    char *input = "attr1, attr2, attr3";
    cJSON *output;
    char *attr1, *attr2, *attr3;

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    output = attrs_to_json(input);

    *state = output;

    attr1 = cJSON_GetStringValue(cJSON_GetArrayItem(output, 0));
    attr2 = cJSON_GetStringValue(cJSON_GetArrayItem(output, 1));
    attr3 = cJSON_GetStringValue(cJSON_GetArrayItem(output, 2));

    assert_string_equal(attr1, "attr1");
    assert_string_equal(attr2, "attr2");
    assert_string_equal(attr3, "attr3");
}

static void test_attrs_to_json_unable_to_create_json_array(void **state)  {
    char *input = "attr1, attr2, attr3";
    cJSON *output;

    will_return(__wrap_cJSON_CreateArray, NULL);

    output = attrs_to_json(input);

    *state = output;

    assert_null(output);
}

// TODO: Validate this condition is required to be tested
static void test_attrs_to_json_null_attributes(void **state)  {
    cJSON *output;

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    output = attrs_to_json(NULL);

    *state = output;
}

/* win_perm_to_json tests*/
static void test_win_perm_to_json_all_permissions(void **state) {
    char *input = "account (allowed): generic_read|generic_write|generic_execute|"
        "generic_all|delete|read_control|write_dac|write_owner|synchronize|read_data|write_data|"
        "append_data|read_ea|write_ea|execute|read_attributes|write_attributes";
    cJSON *output;
    cJSON *user, *permissions_array;
    char *string;

    will_return(__wrap_cJSON_CreateObject, __real_cJSON_CreateObject());

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());
    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    will_return_always(__wrap_wstr_split, 1);  // use real wstr_split

    output = win_perm_to_json(input);

    *state = output;

    assert_int_equal(cJSON_GetArraySize(output), 1);

    user = cJSON_GetArrayItem(output, 0);

    string = cJSON_GetStringValue(cJSON_GetObjectItem(user, "name"));
    assert_string_equal(string, "account");

    permissions_array = cJSON_GetObjectItem(user, "allowed");

    assert_int_equal(cJSON_GetArraySize(permissions_array), 17);

    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 0));
    assert_string_equal(string, "GENERIC_READ");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 1));
    assert_string_equal(string, "GENERIC_WRITE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 2));
    assert_string_equal(string, "GENERIC_EXECUTE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 3));
    assert_string_equal(string, "GENERIC_ALL");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 4));
    assert_string_equal(string, "DELETE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 5));
    assert_string_equal(string, "READ_CONTROL");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 6));
    assert_string_equal(string, "WRITE_DAC");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 7));
    assert_string_equal(string, "WRITE_OWNER");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 8));
    assert_string_equal(string, "SYNCHRONIZE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 9));
    assert_string_equal(string, "READ_DATA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 10));
    assert_string_equal(string, "WRITE_DATA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 11));
    assert_string_equal(string, "APPEND_DATA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 12));
    assert_string_equal(string, "READ_EA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 13));
    assert_string_equal(string, "WRITE_EA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 14));
    assert_string_equal(string, "EXECUTE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 15));
    assert_string_equal(string, "READ_ATTRIBUTES");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 16));
    assert_string_equal(string, "WRITE_ATTRIBUTES");
}

static void test_win_perm_to_json_some_permissions(void **state) {
    char *input = "account (allowed): generic_read|generic_execute|delete|"
        "write_dac|synchronize|write_data|read_ea|execute|write_attributes";
    cJSON *output;
    cJSON *user, *permissions_array;
    char *string;

    will_return(__wrap_cJSON_CreateObject, __real_cJSON_CreateObject());

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());
    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    will_return_always(__wrap_wstr_split, 1);  // use real wstr_split

    output = win_perm_to_json(input);

    *state = output;

    assert_int_equal(cJSON_GetArraySize(output), 1);

    user = cJSON_GetArrayItem(output, 0);

    string = cJSON_GetStringValue(cJSON_GetObjectItem(user, "name"));
    assert_string_equal(string, "account");

    permissions_array = cJSON_GetObjectItem(user, "allowed");

    assert_int_equal(cJSON_GetArraySize(permissions_array), 9);

    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 0));
    assert_string_equal(string, "GENERIC_READ");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 1));
    assert_string_equal(string, "GENERIC_EXECUTE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 2));
    assert_string_equal(string, "DELETE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 3));
    assert_string_equal(string, "WRITE_DAC");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 4));
    assert_string_equal(string, "SYNCHRONIZE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 5));
    assert_string_equal(string, "WRITE_DATA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 6));
    assert_string_equal(string, "READ_EA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 7));
    assert_string_equal(string, "EXECUTE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 8));
    assert_string_equal(string, "WRITE_ATTRIBUTES");
}

static void test_win_perm_to_json_no_permissions(void **state) {
    char *input = "account (allowed)";
    cJSON *output;

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    expect_string(__wrap__mdebug1, msg, "Uncontrolled condition when parsing a Windows permission from '%s'.");
    expect_string(__wrap__mdebug1, param1, "account (allowed)");

    output = win_perm_to_json(input);

    *state = output;

    assert_null(output);
}

static void test_win_perm_to_json_allowed_denied_permissions(void **state) {
    char *input = "account (denied): generic_read|generic_write|generic_execute|"
        "generic_all|delete|read_control|write_dac|write_owner, account (allowed): synchronize|read_data|write_data|"
        "append_data|read_ea|write_ea|execute|read_attributes|write_attributes";
    cJSON *output;
    cJSON *user, *permissions_array;
    char *string;

    will_return(__wrap_cJSON_CreateObject, __real_cJSON_CreateObject());

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());
    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());
    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    will_return_always(__wrap_wstr_split, 1);  // use real wstr_split

    output = win_perm_to_json(input);

    *state = output;

    assert_int_equal(cJSON_GetArraySize(output), 1);

    user = cJSON_GetArrayItem(output, 0);

    string = cJSON_GetStringValue(cJSON_GetObjectItem(user, "name"));
    assert_string_equal(string, "account");

    permissions_array = cJSON_GetObjectItem(user, "denied");
    assert_int_equal(cJSON_GetArraySize(permissions_array), 8);

    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 0));
    assert_string_equal(string, "GENERIC_READ");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 1));
    assert_string_equal(string, "GENERIC_WRITE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 2));
    assert_string_equal(string, "GENERIC_EXECUTE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 3));
    assert_string_equal(string, "GENERIC_ALL");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 4));
    assert_string_equal(string, "DELETE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 5));
    assert_string_equal(string, "READ_CONTROL");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 6));
    assert_string_equal(string, "WRITE_DAC");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 7));
    assert_string_equal(string, "WRITE_OWNER");

    permissions_array = cJSON_GetObjectItem(user, "allowed");
    assert_int_equal(cJSON_GetArraySize(permissions_array), 9);

    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 0));
    assert_string_equal(string, "SYNCHRONIZE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 1));
    assert_string_equal(string, "READ_DATA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 2));
    assert_string_equal(string, "WRITE_DATA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 3));
    assert_string_equal(string, "APPEND_DATA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 4));
    assert_string_equal(string, "READ_EA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 5));
    assert_string_equal(string, "WRITE_EA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 6));
    assert_string_equal(string, "EXECUTE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 7));
    assert_string_equal(string, "READ_ATTRIBUTES");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 8));
    assert_string_equal(string, "WRITE_ATTRIBUTES");
}

static void test_win_perm_to_json_multiple_accounts(void **state) {
    char *input = "first (allowed): generic_read|generic_write|generic_execute,"
        " first (denied): generic_all|delete|read_control|write_dac|write_owner,"
        " second (allowed): synchronize|read_data|write_data,"
        " third (denied): append_data|read_ea|write_ea|execute|read_attributes|write_attributes";
    cJSON *output;
    cJSON *user, *permissions_array;
    char *string;

    will_return(__wrap_cJSON_CreateObject, __real_cJSON_CreateObject());
    will_return(__wrap_cJSON_CreateObject, __real_cJSON_CreateObject());
    will_return(__wrap_cJSON_CreateObject, __real_cJSON_CreateObject());

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());
    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());
    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());
    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());
    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    will_return_always(__wrap_wstr_split, 1);  // use real wstr_split

    output = win_perm_to_json(input);

    *state = output;

    assert_int_equal(cJSON_GetArraySize(output), 3);

    user = cJSON_GetArrayItem(output, 0);

    string = cJSON_GetStringValue(cJSON_GetObjectItem(user, "name"));
    assert_string_equal(string, "first");

    permissions_array = cJSON_GetObjectItem(user, "allowed");
    assert_int_equal(cJSON_GetArraySize(permissions_array), 3);

    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 0));
    assert_string_equal(string, "GENERIC_READ");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 1));
    assert_string_equal(string, "GENERIC_WRITE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 2));
    assert_string_equal(string, "GENERIC_EXECUTE");

    permissions_array = cJSON_GetObjectItem(user, "denied");
    assert_int_equal(cJSON_GetArraySize(permissions_array), 5);

    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 0));
    assert_string_equal(string, "GENERIC_ALL");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 1));
    assert_string_equal(string, "DELETE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 2));
    assert_string_equal(string, "READ_CONTROL");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 3));
    assert_string_equal(string, "WRITE_DAC");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 4));
    assert_string_equal(string, "WRITE_OWNER");

    user = cJSON_GetArrayItem(output, 1);

    string = cJSON_GetStringValue(cJSON_GetObjectItem(user, "name"));
    assert_string_equal(string, "second");

    permissions_array = cJSON_GetObjectItem(user, "allowed");
    assert_int_equal(cJSON_GetArraySize(permissions_array), 3);

    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 0));
    assert_string_equal(string, "SYNCHRONIZE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 1));
    assert_string_equal(string, "READ_DATA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 2));
    assert_string_equal(string, "WRITE_DATA");

    user = cJSON_GetArrayItem(output, 2);

    string = cJSON_GetStringValue(cJSON_GetObjectItem(user, "name"));
    assert_string_equal(string, "third");

    permissions_array = cJSON_GetObjectItem(user, "denied");
    assert_int_equal(cJSON_GetArraySize(permissions_array), 6);

    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 0));
    assert_string_equal(string, "APPEND_DATA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 1));
    assert_string_equal(string, "READ_EA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 2));
    assert_string_equal(string, "WRITE_EA");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 3));
    assert_string_equal(string, "EXECUTE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 4));
    assert_string_equal(string, "READ_ATTRIBUTES");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 5));
    assert_string_equal(string, "WRITE_ATTRIBUTES");
}

static void test_win_perm_to_json_fragmented_acl(void **state) {
    char *input = "first (allowed): generic_read|generic_write|generic_execute,"
        " first (allowed): generic_all|delete|read_control|write_dac|write_owner,";
    cJSON *output;
    cJSON *user, *permissions_array;
    char *string;

    will_return(__wrap_cJSON_CreateObject, __real_cJSON_CreateObject());

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());
    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    will_return_always(__wrap_wstr_split, 1);  // use real wstr_split

    expect_string(__wrap__mdebug2, msg, "ACL [%s] fragmented. All permissions may not be displayed.");
    expect_string(__wrap__mdebug2, param1, input);

    output = win_perm_to_json(input);

    *state = output;

    assert_int_equal(cJSON_GetArraySize(output), 1);

    user = cJSON_GetArrayItem(output, 0);

    string = cJSON_GetStringValue(cJSON_GetObjectItem(user, "name"));
    assert_string_equal(string, "first");

    permissions_array = cJSON_GetObjectItem(user, "allowed");
    assert_int_equal(cJSON_GetArraySize(permissions_array), 3);

    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 0));
    assert_string_equal(string, "GENERIC_READ");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 1));
    assert_string_equal(string, "GENERIC_WRITE");
    string = cJSON_GetStringValue(cJSON_GetArrayItem(permissions_array, 2));
    assert_string_equal(string, "GENERIC_EXECUTE");
}

// TODO: Validate this condition is required to be tested
static void test_win_perm_to_json_null_input(void **state) {
    cJSON *output;

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    output = win_perm_to_json(NULL);

    assert_int_equal(cJSON_GetArraySize(output), 0);
}

static void test_win_perm_to_json_unable_to_create_main_array(void **state) {
    char *input = "first (allowed): generic_read|generic_write|generic_execute,";
    cJSON *output;

    will_return(__wrap_cJSON_CreateArray, NULL);

    output = win_perm_to_json(input);

    assert_null(output);
}

static void test_win_perm_to_json_unable_to_create_sub_array(void **state) {
    char *input = "first (allowed): generic_read|generic_write|generic_execute,";
    cJSON *output;

    will_return(__wrap_cJSON_CreateObject, __real_cJSON_CreateObject());

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());
    will_return(__wrap_cJSON_CreateArray, NULL);

    expect_string(__wrap__mdebug1, msg, "Uncontrolled condition when parsing a Windows permission from '%s'.");
    expect_string(__wrap__mdebug1, param1, "first (allowed): generic_read|generic_write|generic_execute,");

    output = win_perm_to_json(input);

    assert_null(output);
}

static void test_win_perm_to_json_unable_to_create_user_object(void **state) {
    char *input = "first (allowed): generic_read|generic_write|generic_execute,";
    cJSON *output;

    will_return(__wrap_cJSON_CreateObject, NULL);

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    expect_string(__wrap__mdebug1, msg, "Uncontrolled condition when parsing a Windows permission from '%s'.");
    expect_string(__wrap__mdebug1, param1, "first (allowed): generic_read|generic_write|generic_execute,");

    output = win_perm_to_json(input);

    assert_null(output);
}

static void test_win_perm_to_json_incorrect_permission_format(void **state) {
    char *input = "This format is incorrect";
    cJSON *output;

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    expect_string(__wrap__mdebug1, msg, "Uncontrolled condition when parsing a Windows permission from '%s'.");
    expect_string(__wrap__mdebug1, param1, "This format is incorrect");

    output = win_perm_to_json(input);

    assert_null(output);
}
static void test_win_perm_to_json_incorrect_permission_format_2(void **state) {
    char *input = "This format is incorrect (too";
    cJSON *output;

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    expect_string(__wrap__mdebug1, msg, "Uncontrolled condition when parsing a Windows permission from '%s'.");
    expect_string(__wrap__mdebug1, param1, "This format is incorrect (too");

    output = win_perm_to_json(input);

    assert_null(output);
}

static void test_win_perm_to_json_error_splitting_permissions(void **state) {
    char *input = "first (allowed): generic_read|generic_write|generic_execute,";
    cJSON *output;

    will_return(__wrap_cJSON_CreateObject, __real_cJSON_CreateObject());

    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());
    will_return(__wrap_cJSON_CreateArray, __real_cJSON_CreateArray());

    will_return_always(__wrap_wstr_split, 0);  // fail to split string

    expect_string(__wrap__mdebug1, msg, "Uncontrolled condition when parsing a Windows permission from '%s'.");
    expect_string(__wrap__mdebug1, param1, "first (allowed): generic_read|generic_write|generic_execute,");

    output = win_perm_to_json(input);

    assert_null(output);
}


int main(int argc, char *argv[]) {
    const struct CMUnitTest tests[] = {
        /* delete_target_file tests */
        cmocka_unit_test(test_delete_target_file_success),
        cmocka_unit_test(test_delete_target_file_rmdir_ex_error),

        /* escape_syscheck_field tests */
        cmocka_unit_test_teardown(test_escape_syscheck_field_escape_all, teardown_string),
        cmocka_unit_test_teardown(test_escape_syscheck_field_null_input, teardown_string),

        /* normalize_path tests */
        cmocka_unit_test_teardown(test_normalize_path_success, teardown_string),
        cmocka_unit_test_teardown(test_normalize_path_linux_dir, teardown_string),
        cmocka_unit_test(test_normalize_path_null_input),

        /* remove_empty_folders tests */
        cmocka_unit_test(test_remove_empty_folders_success),
        cmocka_unit_test(test_remove_empty_folders_recursive_success),
        cmocka_unit_test(test_remove_empty_folders_null_input),
        cmocka_unit_test(test_remove_empty_folders_relative_path),
        cmocka_unit_test(test_remove_empty_folders_absolute_path),
        cmocka_unit_test(test_remove_empty_folders_non_empty_dir),
        cmocka_unit_test(test_remove_empty_folders_error_removing_dir),

        /* sk_decode_sum tests */
        cmocka_unit_test(test_sk_decode_sum_no_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_deleted_file, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_no_perm, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_missing_separator, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_no_uid, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_no_gid, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_no_md5, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_no_sha1, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_no_new_fields, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_win_perm_string, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_win_perm_encoded, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_no_gname, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_no_uname, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_no_mtime, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_no_inode, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_no_sha256, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_empty_sha256, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_no_attributes, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_non_numeric_attributes, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_win_encoded_attributes, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_empty, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_user_name, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_group_id, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_group_name, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_process_name, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_audit_uid, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_audit_name, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_effective_uid, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_effective_name, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_ppid, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_process_id, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_tag, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_symbolic_path, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_no_inode, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_all_fields, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_all_fields_silent, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_null_ppid, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_null_sum, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_sum_extra_data_null_c_sum, setup_sk_decode, teardown_sk_decode),

        /* sk_decode_extradata tests */
        cmocka_unit_test_setup_teardown(test_sk_decode_extradata_null_c_sum, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_extradata_no_changes, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_extradata_no_date_alert, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_extradata_no_sym_path, setup_sk_decode, teardown_sk_decode),
        cmocka_unit_test_setup_teardown(test_sk_decode_extradata_all_fields, setup_sk_decode, teardown_sk_decode),

        /* sk_fill_event tests */
        cmocka_unit_test_setup_teardown(test_sk_fill_event_full_event, setup_sk_fill_event, teardown_sk_fill_event),
        cmocka_unit_test_setup_teardown(test_sk_fill_event_empty_event, setup_sk_fill_event, teardown_sk_fill_event),
        cmocka_unit_test_setup_teardown(test_sk_fill_event_win_perm, setup_sk_fill_event, teardown_sk_fill_event),
        cmocka_unit_test_setup_teardown(test_sk_fill_event_null_eventinfo, setup_sk_fill_event, teardown_sk_fill_event),
        cmocka_unit_test_setup_teardown(test_sk_fill_event_null_f_name, setup_sk_fill_event, teardown_sk_fill_event),
        cmocka_unit_test_setup_teardown(test_sk_fill_event_null_sum, setup_sk_fill_event, teardown_sk_fill_event),

        /* sk_build_sum tests */
        cmocka_unit_test_setup_teardown(test_sk_build_sum_full_message, setup_sk_build_sum, teardown_sk_build_sum),
        cmocka_unit_test_setup_teardown(test_sk_build_sum_skip_fields_message, setup_sk_build_sum, teardown_sk_build_sum),
        cmocka_unit_test_setup_teardown(test_sk_build_sum_win_perm, setup_sk_build_sum, teardown_sk_build_sum),
        cmocka_unit_test_setup_teardown(test_sk_build_sum_insufficient_buffer_size, setup_sk_build_sum, teardown_sk_build_sum),
        cmocka_unit_test_setup_teardown(test_sk_build_sum_null_sum, setup_sk_build_sum, teardown_sk_build_sum),
        cmocka_unit_test_setup_teardown(test_sk_build_sum_null_output, setup_sk_build_sum, teardown_sk_build_sum),

        /* sk_sum_clean tests */
        cmocka_unit_test_setup(test_sk_sum_clean_full_message, setup_sk_decode),
        cmocka_unit_test_setup(test_sk_sum_clean_shortest_valid_message, setup_sk_decode),
        cmocka_unit_test_setup(test_sk_sum_clean_invalid_message, setup_sk_decode),
        cmocka_unit_test_setup(test_sk_sum_clean_null_sum, setup_sk_decode),

        /* unescape_syscheck_field tests */
        cmocka_unit_test_setup_teardown(test_unescape_syscheck_field_escaped_chars, setup_unescape_syscheck_field, teardown_unescape_syscheck_field),
        cmocka_unit_test_setup_teardown(test_unescape_syscheck_field_no_escaped_chars, setup_unescape_syscheck_field, teardown_unescape_syscheck_field),
        cmocka_unit_test_setup_teardown(test_unescape_syscheck_null_input, setup_unescape_syscheck_field, teardown_unescape_syscheck_field),
        cmocka_unit_test_setup_teardown(test_unescape_syscheck_empty_string, setup_unescape_syscheck_field, teardown_unescape_syscheck_field),

        /* get_user tests */
        cmocka_unit_test_teardown(test_get_user_success, teardown_string),
        cmocka_unit_test_teardown(test_get_user_uid_not_found, teardown_string),
        cmocka_unit_test_teardown(test_get_user_error, teardown_string),

        /* get_group tests */
        cmocka_unit_test(test_get_group_success),
        cmocka_unit_test(test_get_group_failure),

        /* ag_send_syscheck tests */
        cmocka_unit_test(test_ag_send_syscheck_success),
        cmocka_unit_test(test_ag_send_syscheck_unable_to_connect),
        cmocka_unit_test(test_ag_send_syscheck_error_sending_message),


        /* decode_win_attributes tests */
        cmocka_unit_test(test_decode_win_attributes_all_attributes),
        cmocka_unit_test(test_decode_win_attributes_no_attributes),
        cmocka_unit_test(test_decode_win_attributes_some_attributes),

        /* decode_win_permissions tests */
        cmocka_unit_test_teardown(test_decode_win_permissions_success_all_permissions, teardown_string),
        cmocka_unit_test_teardown(test_decode_win_permissions_success_no_permissions, teardown_string),
        cmocka_unit_test_teardown(test_decode_win_permissions_success_some_permissions, teardown_string),
        cmocka_unit_test_teardown(test_decode_win_permissions_success_multiple_accounts, teardown_string),
        cmocka_unit_test_teardown(test_decode_win_permissions_fail_no_account_name, teardown_string),
        cmocka_unit_test_teardown(test_decode_win_permissions_fail_no_access_type, teardown_string),
        cmocka_unit_test_teardown(test_decode_win_permissions_fail_wrong_format, teardown_string),

        /* attrs_to_json tests */
        cmocka_unit_test_teardown(test_attrs_to_json_single_attribute, teardown_cjson),
        cmocka_unit_test_teardown(test_attrs_to_json_multiple_attributes, teardown_cjson),
        cmocka_unit_test_teardown(test_attrs_to_json_unable_to_create_json_array, teardown_cjson),
        cmocka_unit_test_teardown(test_attrs_to_json_null_attributes, teardown_cjson),

        /* win_perm_to_json tests*/
        cmocka_unit_test_teardown(test_win_perm_to_json_all_permissions, teardown_cjson),
        cmocka_unit_test_teardown(test_win_perm_to_json_some_permissions, teardown_cjson),
        cmocka_unit_test_teardown(test_win_perm_to_json_no_permissions, teardown_cjson),
        cmocka_unit_test_teardown(test_win_perm_to_json_allowed_denied_permissions, teardown_cjson),
        cmocka_unit_test_teardown(test_win_perm_to_json_multiple_accounts, teardown_cjson),
        cmocka_unit_test_teardown(test_win_perm_to_json_fragmented_acl, teardown_cjson),
        cmocka_unit_test_teardown(test_win_perm_to_json_null_input, teardown_cjson),
        cmocka_unit_test_teardown(test_win_perm_to_json_unable_to_create_main_array, teardown_cjson),
        cmocka_unit_test_teardown(test_win_perm_to_json_unable_to_create_sub_array, teardown_cjson),
        cmocka_unit_test_teardown(test_win_perm_to_json_unable_to_create_user_object, teardown_cjson),
        cmocka_unit_test_teardown(test_win_perm_to_json_incorrect_permission_format, teardown_cjson),
        cmocka_unit_test_teardown(test_win_perm_to_json_incorrect_permission_format_2, teardown_cjson),
        cmocka_unit_test_teardown(test_win_perm_to_json_error_splitting_permissions, teardown_cjson),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}