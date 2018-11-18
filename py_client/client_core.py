# -*- coding: utf-8 -*-
import socket
import re, os
from bases import InternalError, LogicError, connect_required, offline_required, login_required, transmitting_required
import utility
from file_thread import downloadThread, uploadThread

class ClientSession:
    # 会话状态
    STATUS_OFFLINE = -1
    STATUS_CONNECT = 0
    STATUS_LOGGED = 1

    # 传输模式
    MODE_PASV = 1
    MODE_PORT = 0

    BUFF_SIZE = 4096

    def __init__(self, root='/tmp', outer_show=None):

        # local relative
        if not os.path.isdir(root):
            raise InternalError('input not a path')
        self.root_directory = root
        self.console_log(0, 'local root set to: ' + root)

        # connect relative
        self.status = self.STATUS_OFFLINE
        self.trans_mode = self.MODE_PASV
        self.control_sock = None
        self.data_sock = None
        if outer_show is not None and not callable(outer_show):
            raise InternalError('function given to Session not callable')
        self.outer_show = outer_show

        # file relative
        self.TRANSMITTING = False
        self.trans_size = 0
        self.is_upload = False
        self.current_upload = {}
        self.current_download = {}

        # server relative
        self.server_ip = ''
        self.server_port = 0
        self.server_root = ''
        self.server_current_dir = ''

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
            self.console_log(0, 'local root changed to: ' + new_root)
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

    # ----------- command implement --------------

    @login_required
    def data_port(self):
        raise NotImplementedError('not done yet')

    # 如果成功，将在self.data_sock创建连接，否则抛出一场
    @login_required
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

    @login_required
    def change_trans_mode(self, ipaddr=''):
        if self.trans_mode == self.MODE_PASV:
            assert type(ipaddr) == str, 'input not a string'
            if ipaddr == '' or (not utility.is_address(ipaddr, 0)):
                raise LogicError('Input IP adress not legal')
            self.trans_mode = self.MODE_PORT
            self.console_log(0, 'transfer mode swtiched to PORT')
        else:
            self.trans_mode = self.MODE_PASV
            self.console_log(0, 'transfer mode swtiched to PASV')
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
        try:
            self.control_sock.connect((self.server_ip, self.server_port))
        except:
            self.control_sock.close()
            self.control_sock = None
            raise LogicError('can\'t connect to server')
        code = self.get_res_code(self.expect_respond())
        if code != 220:
            self.control_sock.close()
            self.control_sock = None
            raise LogicError('server refused!')
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
                self.status = self.STATUS_LOGGED
                # 获取一下目录
                self.get_server_current_root()
                self.server_root = self.server_current_dir
                self.send_msg('TYPE I')
                code = self.get_res_code(self.expect_respond())
                if code != 200:
                    raise InternalError('Server rejected TYPE')
                return
            elif code == 202:
                raise LogicError('permission already granted')
            elif code == 530:
                raise LogicError('username or password unacceptable')
            else:
                raise LogicError('failed to login, see command prompt for infomation')
        else:
            raise LogicError('server denied the input user name')
    
    '''
    --------------- 路径相关 --------------------
    '''
    @login_required
    def in_server_root(self):
        return self.server_current_dir == self.server_root

    @login_required
    def get_server_current_root(self):
        # PWD 命令，获取server_current_dir
        self.send_msg('PWD')
        res = self.expect_respond()
        code = self.get_res_code(res)
        if code != 257:
            raise InternalError('Server rejected PWD')
        dir_re = re.compile('[^"]*"([^"]*)".*')
        match_ans = dir_re.match(res)
        assert match_ans is not None, 'matching result is none'
        assert len(match_ans.groups()) == 1, 'match result is wrong: ' + str(match_ans)
        self.server_current_dir = match_ans.groups()[0]
        self.console_log(0, 'server\'s root dir is: ' + self.server_current_dir)
        return

    @login_required
    def change_server_current_root(self, target_dir):
        # CWD命令，获取server_current_dir
        assert type(target_dir) == str and target_dir != '', 'given target dir wrong'
        # 可能要返回上级目录
        if target_dir == '..':
            if self.in_server_root():
                raise InternalError('should not have parent dir here')
            pare, child = os.path.split(self.server_current_dir)
            if child == '':
                raise InternalError('child path is none')
            self.send_msg('CWD ' + pare)
        else:
            self.send_msg('CWD ' + target_dir)

        res = self.expect_respond()
        code = self.get_res_code(res)
        if code not in [200, 250]:
            raise InternalError('Server rejected CWD')

        # 修改目录
        if target_dir == '..':
            self.server_current_dir = pare
        else:
            self.server_current_dir = os.path.join(self.server_current_dir, target_dir)
        self.console_log(0, 'server\'s root dir changed to: ' + self.server_current_dir)

    @login_required
    def list_server_file(self):
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
            if data is None or len(data) == 0:
                break
            result += data.decode()
        
        # 接受完毕
        self.data_sock.close()
        self.data_sock = None
        code = self.get_res_code(self.expect_respond())
        if code != 226:
            raise InternalError('LIST failed!')

        return result

    @login_required
    def server_make_dir(self, dir_name):
        assert type(dir_name) == str and dir_name != '', 'given dir name wrong'
        self.send_msg('MKD ' + dir_name)
        res = self.expect_respond()
        code = self.get_res_code(res)
        if code not in [257, 250]:
            raise LogicError('The server rejected, see command for info')
        self.console_log(0, 'server created dir: ' + dir_name)
    
    @login_required
    def server_rename(self, from_name, to_name):
        assert type(from_name) == str and from_name != '', 'given old file name wrong'
        assert type(to_name) == str and to_name != '', 'given new file name wrong'
        self.send_msg('RNFR ' + from_name)
        res = self.expect_respond()
        code = self.get_res_code(res)
        if code != 350:
            raise LogicError('The server rejected, see command for info')
        self.send_msg('RNTO ' + to_name)
        res = self.expect_respond()
        code = self.get_res_code(res)
        if code != 250:
            raise LogicError('The server rejected, see command for info')
        self.console_log(0, 'server renamed %s to %s.' % (from_name, to_name))
    
    @login_required
    def server_delete(self, name):
        assert type(name) == str and name != '', 'given dir name wrong'
        self.send_msg('RMD ' + name)
        res = self.expect_respond()
        code = self.get_res_code(res)
        if code != 250:
            raise LogicError('The server rejected, see command for info')
        self.console_log(0, 'server removed dir: ' + name)

        
    '''
    文件传输相关函数
    '''
    @login_required
    def download_file(self, name='', size='', progress_func=None, finish_func=None):
        assert type(name) == str, 'input not a string'

        try:
            if name == '' or size == '':
                name = self.current_download['name']
                size = self.current_download['size']
                local_file = open(os.path.join(self.root_directory, name), 'ab+')
            else:
                self.current_download['name'] = name
                self.current_download['size'] = size
                local_file = open(os.path.join(self.root_directory, name), 'wb')
        except Exception as e:
            raise LogicError('Failed to open file: ' + str(e))

        if self.trans_mode == self.MODE_PORT:
            self.data_port()
        else:
            self.data_pasv()
        self.send_msg('RETR ' + name)
        code = self.get_res_code(self.expect_respond())
        if code != 150:
            raise InternalError('Server rejected RETR, see command for info')

        self.down_thread = downloadThread(self.data_sock, local_file, self.BUFF_SIZE, size,  
            progress_func=progress_func, finish_func=finish_func, have_done=self.trans_size)
        self.down_thread.start()
        self.TRANSMITTING = True
        self.is_upload = False
        self.console_log(0, 'begin downloading')

    @login_required
    def upload_file(self, name, size, progress_func=None, finish_func=None):
        assert type(name) == str, 'input not a string'
        if name == '' or size == '':
            name = self.current_upload['name']
            size = self.current_upload['size']
        else:
            self.current_upload['name'] = name
            self.current_upload['size'] = size
        try:
            local_file = open(name, 'rb')
        except Exception as e:
            raise LogicError('Failed to open file: ' + str(e))
        
        if self.trans_mode == self.MODE_PORT:
            self.data_port()
        else:
            self.data_pasv()

        basename, filename = os.path.split(name)
        self.send_msg('STOR ' + filename)

        code = self.get_res_code(self.expect_respond())
        if code != 150:
            raise InternalError('Server rejected STOR, see command for info')

        self.up_thread = uploadThread(self.data_sock, local_file, self.BUFF_SIZE, size,  
            progress_func=progress_func, finish_func=finish_func)
        
        self.up_thread.start()
        self.is_upload = True
        self.TRANSMITTING = True
        self.console_log(0, 'begin uploading')

    @transmitting_required
    def finish_trans_file(self, is_done, size_done):
        if is_done:
            code = self.get_res_code(self.expect_respond())
            if code != 226:
                raise InternalError('failed to transmit')
            self.trans_size = 0
            self.TRANSMITTING = False
            self.console_log(0, 'finished transmit')
        else:
            code = self.get_res_code(self.expect_respond())
            self.TRANSMITTING = False
            self.trans_size = size_done
            self.console_log(0, 'stopped at size ' + str(self.trans_size))

    @transmitting_required
    def stop_trans(self):
        if hasattr(self, 'down_thread') and self.down_thread.isRunning():
            self.down_thread.stop_transmit = True
        elif hasattr(self, 'up_thread') and self.up_thread.isRunning():
            self.up_thread.stop_transmit = True
        else:
            return

    @login_required
    def continue_transmit(self):
        if self.is_upload:
            pass
        else:
            self.send_msg('REST ' + str(self.trans_size))
            code = self.get_res_code(self.expect_respond())
            if code != 350:
                raise InternalError('Server rejected REST')
            return