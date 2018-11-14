# -*- coding: utf-8 -*-
import socket
import re
from bases import InternalError, LogicError, connect_required

class ClientSession:
    STATUS_OFFLINE = -1
    STATUS_CONNECT = 0
    STATUS_LOGGED = 1

    MODE_PASV = 1
    MODE_PORT = 0

    BUFF_SIZE = 4096

    def __init__(self, root='/tmp', outer_show=None):
        self.status = self.STATUS_OFFLINE
        host = ''
        port = 0
        self.server = (host, port)
        self.root_directory = root
        self.trans_mode = self.MODE_PASV
        self.control_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.control_port = 0
        self.outer_show = outer_show

    @connect_required
    def send_msg(self, msg):
        self.control_sock.send(msg.decode())

    @connect_required
    def expect_respond(self):
        res = self.control_sock.recv(BUFF_SIZE).decode()
        if self.outer_show is not None:
            if not callable(self.outer_show):
                raise InternalError('function given to Session not callable')
            self.outer_show(res)
        return res

    def connect(self, ip, port):
        assert type(ip) == str, 'input ip not a string'
        assert type(port) == int, 'input port not a number'
        ip_re = re.compile('^((25[0-5]|2[0-4]\d|[01]?\d?\d)\.){3}((25[0-5]|2[0-4]\d|[01]?\d?\d))$')
        
        if not ip_re.match(ip):
            raise LogicError('wrong input IP')
        if port < 0 or port > 65535:
            raise LogicError('wrong input port')

        self.server.host = ip
        self.server.port = port
        self.control_sock.connect(self.server)
        self.status = self.STATUS_CONNECT
        return
    
    @connect_required
    def login(self, user, psd):
        assert type(user) == str, 'input user not a string'
        assert type(psd) == str, 'input password not a string'
        raise NotImplementedError('not done')
        

