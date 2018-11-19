# -*- coding: utf-8 -*-

import os
import re
from bases import LogicError, InternalError

FILE_INFO_DIR = 0
FILE_INFO_FILE = 1

def is_address(ip, port):
    # 判断输入是否为一个IP地址
    assert type(ip) == str, 'input ip not a string'
    assert type(port) == int, 'input port not a number'
    ip_re = re.compile('^((25[0-5]|2[0-4]\d|[01]?\d?\d)\.){3}((25[0-5]|2[0-4]\d|[01]?\d?\d))$')
    
    if not ip_re.match(ip):
        return False
    if port < 0 or port > 65535:
        return False
    return True

def parse_file_info(file_str):
    '''
    解析形如
    -rw-r--r--    1 1000     121       5689110 Oct 31 13:54 bbb.pdf
    格式的字符串
    返回json字典，type、size、name字段分别表示：类型，大小，名字
    '''
    file_re = re.compile('(\S*)\s*\S*\s*\S*\s*\S*\s*(\S*)\s*\S*\s*\S*\s*\S*\s*(.*)')
    match_ans = file_re.match(file_str)
    if len(match_ans.groups()) < 3:
        raise InternalError('parse file info accept wrong input: ' + file_str)
    
    info = {}
    if match_ans.groups()[0][0] == 'd':
        info['type'] = FILE_INFO_DIR
    else:
        info['type'] = FILE_INFO_FILE
    
    info['size'] = int(match_ans.groups()[1])
    info['name'] = match_ans.groups()[2]
    return info