# -*- coding: utf-8 -*-
import socket
import re, os
from bases import InternalError, LogicError, connect_required, offline_required
import utility

class ClientSession:
    STATUS_OFFLINE = -1
    STATUS_CONNECT = 0
    STATUS_LOGGED = 1

    MODE_PASV = 1
    MODE_PORT = 0

    BUFF_SIZE = 4096

    def __init__(self, root='/tmp', outer_show=None):
        self.status = self.STATUS_OFFLINE
        self.server_ip = ''
        self.server_port = 0
        self.root_directory = root
        self.trans_mode = self.MODE_PASV
        self.control_sock = None
        self.data_sock = None
        self.control_port = 0
        self.outer_show = outer_show
        if self.outer_show is not None and not callable(self.outer_show):
            raise InternalError('function given to Session not callable')

    def get_res_code(self, res):
        assert type(res) == str, 'input res not string'
        lines = res.split('\n')
        if len(lines) < 2:
            raise InternalError('input for res code is empty')
        code = lines[-2][:3]
        return int(code)

    def set_root(self, new_root):
        assert type(new_root) == str, 'input not a string'
        if os.path.isdir(new_root):
            self.root_directory = new_root
        else:
            raise LogicError('wrong input')

    # 发送一条控制端口消息
    def send_msg(self, msg):
        self.control_sock.send((msg + '\r\n').encode())

    # 接受一条消息
    def expect_respond(self):
        res = self.control_sock.recv(self.BUFF_SIZE).decode()
        if self.outer_show is not None:
            self.outer_show(res)
        return res

    def console_log(self, code, msg):
        if code == 0:
            print('[MESSAGE]: ' + msg)
        elif code == 1:
            print('[WARNING]: ' + msg)
        else:
            print('[ERROR]: ' + msg)

    @connect_required
    def data_port(self):
        raise NotImplementedError('not done yet')

    # 如果成功，将在self.data_sock创建连接，否则抛出一场
    @connect_required
    def data_pasv(self):
        self.send_msg('PASV')
        res = self.expect_respond()
        code = self.get_res_code(res)
        if code != 227:
            raise InternalError('Server rejected pasv')
        # 解析服务器返回的字符串
        reg = re.compile('^\S*\s\D*(\d{1,3},\d{1,3},\d{1,3},\d{1,3}),(\d{1,3},\d{1,3}).*$')
        match_ans = reg.match(res)
        assert match_ans is not None, 'matching result is none'
        assert len(match_ans.groups()) == 2, 'match result is wrong: ' + str(match_ans)
        
        datacon_ip = match_ans.groups()[0].replace(',', '.')
        port_ls = match_ans.groups()[1].split(',')
        assert len(port_ls) == 2, 'port recived error'
        datacon_port = int(port_ls[0]) * 256 + int(port_ls[1])
        assert utility.is_address(datacon_ip, datacon_port), 'recived ip or address format wrong.'

        # 开始连接
        if self.data_sock:
            self.data_sock.close()
            self.data_sock = None
        self.data_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # self.data_sock.connect((datacon_ip, datacon_port))
        self.data_sock.connect((self.server_ip, datacon_port))
        return

    @offline_required
    def connect(self, ip, port):
        if not utility.is_address(ip, port):
            raise LogicError('Input Ip or port format wrong.')

        self.console_log(0, 'connect to host: ' + str(ip) + '/' + str(port))
        self.server_ip = ip
        self.server_port = port
        # 开始连接时才创建socket
        self.control_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.control_sock.connect((self.server_ip, self.server_port))
        code = self.get_res_code(self.expect_respond())
        if code != 220:
            raise LogicError('can\'t connect to server')
        self.status = self.STATUS_CONNECT
        return
    
    @connect_required
    def disconnect(self):
        self.send_msg('QUIT')
        self.expect_respond()
        self.control_sock.close()
        # 连接关闭后置为None
        self.control_sock = None
        self.status = self.STATUS_OFFLINE
        self.console_log(0, 'disconnected with the host')
        return

    @connect_required
    def login(self, user, psd):
        assert type(user) == str, 'input user not a string'
        assert type(psd) == str, 'input password not a string'

        if user == '' or psd == '':
            raise LogicError('name and password can\'t be empty.')
        
        self.send_msg('USER ' + user)
        res = self.expect_respond()
        code = self.get_res_code(res)
        if code == 230:
            return
        elif code in [331, 332]:
            self.send_msg('PASS ' + psd)
            res = self.expect_respond()
            code = self.get_res_code(res)
            if code == 230:
                return
            elif code == 202:
                raise LogicError('permission already granted')
            elif code == 530:
                raise LogicError('username or password unacceptable')
            else:
                raise LogicError('failed to login, see command prompt for infomation')
        else:
            raise LogicError('server denied the input user name')
    
    @connect_required
    def get_server_root(self):
        self.send_msg('PWD')
        res = self.expect_respond()
        code = self.get_res_code(res)
        if code != 257:
            raise InternalError('Server rejected PWD')
        dir_re = re.compile('[^"]*"([^"]*)".*')
        match_ans = dir_re.match(res)
        assert match_ans is not None, 'matching result is none'
        assert len(match_ans.groups()) == 1, 'match result is wrong: ' + str(match_ans)
        self.server_root = match_ans.groups()[0]
        self.console_log(0, 'server\'s root dir is: ' + self.server_root)
        return

    @connect_required
    def listServerFile(self):
        if self.trans_mode == self.MODE_PORT:
            self.data_port()
        else:
            self.data_pasv()
        self.send_msg('LIST')
        code = self.get_res_code(self.expect_respond())
        if code != 150:
            raise InternalError('Server return not 150')

        result = ''
        while True:
            data = self.data_sock.recv(self.BUFF_SIZE)
            if data is None or data == '':
                break
            if len(data) < self.BUFF_SIZE:
                result += data.decode()
                break
            result += data.decode()
        return result


        
