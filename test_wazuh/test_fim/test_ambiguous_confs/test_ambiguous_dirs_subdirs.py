# Copyright (C) 2015-2019, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

import os

import pytest

from wazuh_testing.fim import (LOG_FILE_PATH, regular_file_cud, WAZUH_PATH,
                               CHECK_ALL, CHECK_GROUP, CHECK_INODE,
                               CHECK_MD5SUM, CHECK_MTIME, CHECK_OWNER,
                               CHECK_PERM, CHECK_SHA1SUM, CHECK_SHA256SUM,
                               CHECK_SIZE, CHECK_SUM, REQUIRED_ATTRIBUTES)
from wazuh_testing.tools import (FileMonitor, check_apply_test,
                                 load_wazuh_configurations)

# variables

test_data_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'data')
configurations_path = os.path.join(test_data_path, 'wazuh_conf.yaml')
checkdir_default = os.path.join('/', 'checkdir_default')
checkdir_checkall = os.path.join(checkdir_default, 'checkdir_checkall')
checkdir_no_inode = os.path.join(checkdir_checkall, 'checkdir_checkdir_no_inode')
checkdir_no_checksum = os.path.join(checkdir_no_inode, 'checkdir_no_checksum')
test_directories = [os.path.join('/', 'testdir'), os.path.join('/', 'testdir', 'subdir'),
                    os.path.join('/', 'recursiondir'), checkdir_default, checkdir_checkall,
                    checkdir_no_inode, checkdir_no_checksum]
testdir, subdir = test_directories[0:2]
testdir_recursion = test_directories[2]

tag = 'Sample_tag'


wazuh_log_monitor = FileMonitor(LOG_FILE_PATH)

# configurations

configurations = load_wazuh_configurations(configurations_path, __name__,
                                           params=[{'TAGS': tag}],
                                           metadata=[{'tags': tag}]
                                           )
configurations = [configurations[6]]

# fixtures

@pytest.fixture(scope='module', params=configurations)
def get_configuration(request):
    """Get configurations from the module."""
    return request.param


# functions


# tests

@pytest.mark.parametrize('folders', [
    ([testdir, subdir])
])
@pytest.mark.parametrize('tags_to_apply', [
    ({'ambiguous_restrict'}),
    ({'ambiguous_ignore'}),

])
def _test_ambiguous_restrict_ignore(folders, tags_to_apply,
                                   get_configuration, configure_environment,
                                   restart_syscheckd, wait_for_initial_scan):
    """ Checks if syscheckd detects regular file changes (add, modify, delete)

    """
    check_apply_test(tags_to_apply, get_configuration['tags'])

    file_list = ['example.csv']
    min_timeout = 3

    regular_file_cud(folders[0], wazuh_log_monitor, file_list=file_list,
                     time_travel=False,
                     min_timeout=min_timeout, triggers_event=False)

    regular_file_cud(folders[1], wazuh_log_monitor, file_list=file_list,
                     time_travel=False,
                     min_timeout=min_timeout, triggers_event=True)


@pytest.mark.parametrize('folders, tags_to_apply', [
    ([testdir, subdir], {'ambiguous_report_changes'})
])
def _test_ambiguous_report(folders, tags_to_apply,
                          get_configuration, configure_environment,
                          restart_syscheckd, wait_for_initial_scan):
    """ Checks if syscheckd detects regular file changes (add, modify, delete)

    """
    check_apply_test(tags_to_apply, get_configuration['tags'])

    file_list = ['regular']
    min_timeout = 3
    folder = folders[1]

    def report_changes_validator(event):
        """ Validate content_changes attribute exists in the event """
        for file in file_list:
            diff_file = os.path.join(WAZUH_PATH, 'queue', 'diff', 'local',
                                     folder.strip('/'), file)
            assert (os.path.exists(diff_file)), f'{diff_file} does not exist'
            assert (event['data'].get('content_changes') is not None), f'content_changes is empty'

    def no_report_changes_validator(event):
        """ Validate content_changes attribute does not exist in the event """
        for file in file_list:
            diff_file = os.path.join(WAZUH_PATH, 'queue', 'diff', 'local',
                                     folder.strip('/'), file)
            assert (not os.path.exists(diff_file)), f'{diff_file} exists'
            assert ('content_changes' not in event['data'].keys()), f"'content_changes' in event"

    regular_file_cud(folders[1], wazuh_log_monitor, file_list=file_list,
                     time_travel=False,
                     min_timeout=min_timeout, triggers_event=True,
                     validators_after_update=[report_changes_validator])

    folder = folders[0]
    regular_file_cud(folders[0], wazuh_log_monitor, file_list=file_list,
                     time_travel=False,
                     min_timeout=min_timeout, triggers_event=True,
                     validators_after_update=[no_report_changes_validator])


@pytest.mark.parametrize('folders, tags_to_apply', [
    ([testdir, subdir], {'ambiguous_tags'})
])
def _test_ambiguous_tags(folders, tags_to_apply,
                        get_configuration, configure_environment,
                        restart_syscheckd, wait_for_initial_scan):
    """ Checks if syscheckd detects regular file changes (add, modify, delete)

    """
    check_apply_test(tags_to_apply, get_configuration['tags'])

    defined_tags = get_configuration['metadata']['tags']

    def tag_validator(event):
        """ Validate tag attribute exists in the event """
        assert defined_tags == event['data']['tags'], f'defined_tags are not equal'

    def no_tag_validator(event):
        """ Validate tag attribute dpes not exist in the event """
        assert 'tags' not in event['data'].keys(), f"'tags' key found in event"

    regular_file_cud(folders[0], wazuh_log_monitor,
                     time_travel=False,
                     min_timeout=3, validators_after_cud=[no_tag_validator]
                     )

    regular_file_cud(folders[1], wazuh_log_monitor,
                     time_travel=False,
                     min_timeout=3, validators_after_cud=[tag_validator]
                     )


@pytest.mark.parametrize('dirname, recursion_level, tags_to_apply', [
    (testdir_recursion,  1, {'ambiguous_recursion_over'}),
    (testdir_recursion, 4, {'ambiguous_recursion'})
])
def _test_ambiguous_recursion(dirname, recursion_level, tags_to_apply,
                             get_configuration, configure_environment,
                             restart_syscheckd, wait_for_initial_scan):
    check_apply_test(tags_to_apply, get_configuration['tags'])

    min_timeout = 3

    recursion_subdir = 'subdir'
    path = dirname
    for n in range(recursion_level):
        path = os.path.join(path, recursion_subdir + str(n + 1))
        regular_file_cud(path, wazuh_log_monitor,
                         time_travel=False,
                         min_timeout=min_timeout, triggers_event=True)

    for n in list(range(recursion_level, 4)):
        path = os.path.join(path, recursion_subdir + str(n + 1))
        regular_file_cud(path, wazuh_log_monitor,
                         time_travel=False,
                         min_timeout=min_timeout, triggers_event=False)


@pytest.mark.parametrize('tags_to_apply', [
    {'ambiguous_check'}
])
@pytest.mark.parametrize('dirname, checkers', [
    (checkdir_default, {CHECK_SIZE} | {CHECK_PERM} | {CHECK_OWNER} | {CHECK_GROUP} | {CHECK_SHA256SUM} |
            {CHECK_MTIME} | {CHECK_INODE}),
    (checkdir_checkall, REQUIRED_ATTRIBUTES[CHECK_ALL]),
    (checkdir_no_checksum, REQUIRED_ATTRIBUTES[CHECK_ALL] - {CHECK_SUM}),
    (checkdir_no_inode, REQUIRED_ATTRIBUTES[CHECK_ALL] - {CHECK_INODE})
])
def test_ambiguous_check(dirname, checkers, tags_to_apply,
                         get_configuration, configure_environment,
                         restart_syscheckd, wait_for_initial_scan):
    check_apply_test(tags_to_apply, get_configuration['tags'])

    min_timeout = 3

    regular_file_cud(dirname, wazuh_log_monitor, min_timeout=min_timeout, options=checkers,
                     time_travel=False)



