# -*- coding: utf-8 -*-

import os
import re

def is_address(ip, port):
    assert type(ip) == str, 'input ip not a string'
    assert type(port) == int, 'input port not a number'
    ip_re = re.compile('^((25[0-5]|2[0-4]\d|[01]?\d?\d)\.){3}((25[0-5]|2[0-4]\d|[01]?\d?\d))$')
    
    if not ip_re.match(ip):
        return False
    if port < 0 or port > 65535:
        return False
    return True
