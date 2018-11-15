# -*- coding: utf-8 -*-

class BaseError(Exception):
    
    def __init__(self, code, msg):
        super(BaseError, self).__init__(msg)
        assert type(code) == int, 'BaseError input code not int'
        assert type(msg) == str, 'BaseError input meg not string'
        self.code = code
        self.msg = msg
    
    def __repr__(self):
        return '[ERRCODE=%d] %s' % (self.code, self.msg)

class LogicError(BaseError):
    def __init__(self, msg):
        super(LogicError, self).__init__(1, msg)


class InternalError(BaseError):
    def __init__(self, msg):
        super(InternalError, self).__init__(2, msg)

def connect_required(func):
    def wrapper(self, *args, **kwargs):
        if self.status == self.STATUS_OFFLINE or self.control_sock is None:
            raise LogicError('not connect yet')
        else:
            return func(self, *args, **kwargs)
    return wrapper

def offline_required(func):
    def wrapper(self, *args, **kwargs):
        if self.status != self.STATUS_OFFLINE and self.control_sock is None:
            raise LogicError('do disconnect first')
        else:
            return func(self, *args, **kwargs)
    return wrapper