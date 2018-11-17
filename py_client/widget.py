from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
from PyQt5.QtCore import *
from client_core import ClientSession
import os
from bases import LogicError, InternalError
from signals import *
import utility

def qt_front_wrapper(func):
    def wrapper(self, *args, **kwargs):
        try:
            return func(self, *args, **kwargs)
        except LogicError as e:
            QMessageBox.information(self, 'Failed', 'Action Failed:' + str(e), 
                QMessageBox.Yes, QMessageBox.Yes)
        except InternalError as e:
            QMessageBox.information(self, 'Error', 'Meet with Internal Error: ' + str(e), 
                QMessageBox.Yes, QMessageBox.Yes)
        except Exception as e:
            QMessageBox.information(self, 'Error', 'Meet with Base Error: ' + str(e), 
                QMessageBox.Yes, QMessageBox.Yes)
    return wrapper


class CloudFileWidget(QListWidget):
    '''
    用于显示网络文件的Widget，添加了右键、双击的事件处理
    当进行上面的操作时，会发出自定义文件信号（在signals文件中定义）
    通过connect函数连接一个发送的函数（有一个参数为要操作的文件名）
    通过update_files_show更新这里显示的文件，输入文件相关的长字符串和是否显示..
    '''

    def __init__(self):
        super(CloudFileWidget, self).__init__()
        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.current_select_name = ''

        # 定义发送的信号
        self.file_signals = fileSignals()
        makedir_act = QAction(u'MakeDir', self,
            triggered=lambda: self.file_signals.makedir.emit())
      
        # 定义服务器文件右键菜单
        self.file_right_menu = QMenu(self)
        self.file_right_menu.addAction(
            QAction(u'Rename', self, triggered=lambda: self.file_signals.rename.emit(self.current_select_name)))
        self.file_right_menu.addAction(
            QAction(u'Download', self, triggered=lambda: self.file_signals.download.emit(self.current_select_name)))
        self.file_right_menu.addAction(makedir_act)

        # 定义服务器目录右键菜单
        self.dir_right_menu = QMenu(self)
        self.dir_right_menu.addAction(
            QAction(u'Delete', self, triggered=lambda: self.file_signals.delete.emit(self.current_select_name)))
        self.dir_right_menu.addAction(
            QAction(u'Enter', self, triggered=lambda: self.file_signals.enter.emit(self.current_select_name)))
        self.dir_right_menu.addAction(makedir_act)

        # 定义服务器默认右键菜单
        self.default_right_menu = QMenu(self)
        self.default_right_menu.addAction(makedir_act)
    
    # 处理事件相关
    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.LeftButton:
            cursor_item = self.itemAt(self.mapFromGlobal(QCursor.pos()))
            if cursor_item is not None and cursor_item.file_info['type'] == utility.FILE_INFO_DIR:
                self.current_select_name = cursor_item.file_info['name']
                self.file_signals.enter.emit(self.current_select_name)
        return

    def mousePressEvent(self, mouse_event):
        super(CloudFileWidget, self).mousePressEvent(mouse_event)
        if mouse_event.button() == Qt.RightButton:
            cursor_item = self.itemAt(self.mapFromGlobal(QCursor.pos()))
            if cursor_item is not None:
                self.current_select_name = cursor_item.file_info['name']
            self.show_right_menu(cursor_item)
        return
    
    # 显示右键菜单
    @qt_front_wrapper
    def show_right_menu(self, cur_item):
        
        info = getattr(cur_item, 'file_info', None)
        if info is None:
            self.default_right_menu.exec_(QCursor.pos())
            return
        if info['type'] == utility.FILE_INFO_DIR: # 是一个目录
            self.dir_right_menu.exec_(QCursor.pos())
        else:
            self.file_right_menu.exec_(QCursor.pos())
        return

    # 更新文件信息，接受一个文件列表相关的字符串
    @qt_front_wrapper
    def update_files_show(self, files_str, add_parent_dir):
        self.clear()
        if add_parent_dir:
            parent_path = QListWidgetItem('../')
            parent_path.file_info = {
                    'name': '..',
                    'type': utility.FILE_INFO_DIR
                }
            self.addItem(parent_path)
        files = files_str.splitlines()
        for line in files:
            info = utility.parse_file_info(line)
            if info['type'] == utility.FILE_INFO_DIR:
                dir_item = QListWidgetItem(info['name'] + '/')
                dir_item.file_info = {
                    'name': info['name'],
                    'type': utility.FILE_INFO_DIR
                }
                self.addItem(dir_item)
            else:
                file_item = QListWidgetItem(info['name'])
                file_item.file_info = {
                    'name': info['name'],
                    'type': utility.FILE_INFO_FILE
                }
                self.addItem(file_item)
        return
    
class ClientMain(QWidget):
    def __init__(self):
        super().__init__()
        self.session = ClientSession(root=os.getcwd(), outer_show=self.show_command)
        self.initUI()
    
    # 将窗口放置在屏幕中心
    def center(self):
        qr = self.frameGeometry()
        cp = QDesktopWidget().availableGeometry().center()
        qr.moveCenter(cp)
        self.move(qr.topLeft())

    # 声明输入框ui
    def connect_login_ui(self):
        self.ip = QLabel('IP/Domain')
        self.port = QLabel('Port')
        self.user = QLabel('User')
        self.password = QLabel('Password')

        self.ipEdit = QLineEdit()
        self.portEdit = QLineEdit()
        self.userEdit = QLineEdit()
        self.passwordEdit = QLineEdit()

        self.portEdit.setValidator(QIntValidator(0, 65535))
        self.passwordEdit.setContextMenuPolicy(Qt.NoContextMenu)
        self.passwordEdit.setEchoMode(QLineEdit.Password)

        self.connect_btn = QPushButton('Connect')
        self.login_btn = QPushButton('Login')
        return
    # 声明文件相关的ui
    def files_ui(self):
        self.cmd_prompt = QLabel('Command Prompt:')
        self.cmd_prompt_browser = QTextBrowser()

        self.local_files = QLabel('Local Files')
        self.cloud_files = QLabel('Cloud Files')
        self.local_files_browser = QTextBrowser()
        self.cloud_files_browser = CloudFileWidget()
        
        self.downloading = QLabel('Downloading')
        self.downloading_browser = QLineEdit()
        return
    # 声明总体表格ui
    def set_grid_ui(self):
        self.grid = QGridLayout()
        grid = self.grid
        grid.setSpacing(10)

        grid.addWidget(self.ip, 1, 0)
        grid.addWidget(self.ipEdit, 1, 1)
        grid.addWidget(self.port, 1, 3)
        grid.addWidget(self.portEdit, 1, 4)
        grid.addWidget(self.connect_btn, 1, 6)

        grid.addWidget(self.user, 2, 0)
        grid.addWidget(self.userEdit, 2, 1)
        grid.addWidget(self.password, 2, 3)
        grid.addWidget(self.passwordEdit, 2, 4)
        grid.addWidget(self.login_btn, 2, 6)

        grid.addWidget(self.cmd_prompt, 3, 0, 1, 2)
        grid.addWidget(self.cmd_prompt_browser, 4, 0, 5, 0)

        grid.addWidget(self.local_files, 9, 0)
        grid.addWidget(self.cloud_files, 9, 6)
        grid.addWidget(self.local_files_browser, 10, 0, 4, 3)
        grid.addWidget(self.cloud_files_browser, 10, 4, 4, 3)

        grid.addWidget(self.downloading, 14, 0, 1, 2)
        grid.addWidget(self.downloading_browser, 15, 0, 1, 0)
        return
    
    # 清空所有ui
    def clear_ui(self):
        self.cmd_prompt_browser.clear()
        self.cloud_files_browser.clear()
    
    def initUI(self):
        # 声明 ui
        self.connect_login_ui()
        self.files_ui()
        self.set_grid_ui()
        
        # 设置总体界面的属性
        self.setLayout(self.grid)
        self.resize(1000, 600)
        self.center()
        self.setWindowTitle('FTP Client')

        # 连接信号槽
        self.connect_btn.clicked.connect(self.connect_server)
        self.login_btn.clicked.connect(self.server_login)
        self.cloud_files_browser.file_signals.enter.connect(self.enter_path)
        self.cloud_files_browser.file_signals.makedir.connect(self.make_dir)
        self.cloud_files_browser.file_signals.rename.connect(self.rename_file)
        self.cloud_files_browser.file_signals.delete.connect(self.delete_file_dir)

        self.show()
    
    # 显示一条command
    def show_command(self, msg):
        self.cmd_prompt_browser.append(msg.rstrip('\r\n'))

    # 关闭事件
    def closeEvent(self, event):
        reply = QMessageBox.question(self, 'Message', 'Are you sure to quit?',
            QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)
        
        if reply == QMessageBox.Yes:
            event.accept()
            if self.session.status != self.session.STATUS_OFFLINE:
                self.session.disconnect()
        else:
            event.ignore()
    
    # 连接服务器
    @qt_front_wrapper
    def connect_server(self, *args, **kwargs):
        if self.session.status == self.session.STATUS_OFFLINE:
            ip = self.ipEdit.text()
            port = self.portEdit.text()
            if self.ipEdit.text() == '' or self.portEdit.text() == '':
                raise LogicError('Please input correct ip and port')
            self.session.connect(ip, int(port))
            QMessageBox.information(self, 'Successful', 'Successfully connected', 
                QMessageBox.Yes, QMessageBox.Yes)
            self.connect_btn.setText('disconnect')
        else:
            self.session.disconnect()
            self.clear_ui()
            QMessageBox.information(self, 'Successful', 'Successfully disconnected', 
                QMessageBox.Yes, QMessageBox.Yes)
            self.connect_btn.setText('connect')

    # 服务器登录
    @qt_front_wrapper
    def server_login(self, *args, **kwargs):
        if self.session.status == self.session.STATUS_LOGGED:
            raise LogicError('You have logged')
        name = self.userEdit.text()
        password = self.passwordEdit.text()
        # 登录，这时会自动获取目前的路径和总体的根目录
        self.session.login(name, password)
        # 获取目录下所有文件
        self.update_cloud_file()
        return

    @qt_front_wrapper
    def update_cloud_file(self):
        files_str = self.session.list_server_file() # 获取目录下所有文件
        if self.session.in_server_root():
            self.cloud_files_browser.update_files_show(files_str, False)
        else:
            self.cloud_files_browser.update_files_show(files_str, True)

    @qt_front_wrapper
    def enter_path(self, path_name):
        self.session.change_server_current_root(path_name)
        self.update_cloud_file()
        return

    @qt_front_wrapper
    def make_dir(self):
        name,ok = QInputDialog.getText(self,'Input',"Please input a directory name: ",
            QLineEdit.Normal)
        if ok:
            if name == '' or name[0] == '/':
                raise LogicError('please input a legal dir name')
            else:
                self.session.server_make_dir(name)
                self.update_cloud_file()

    @qt_front_wrapper
    def rename_file(self, file_name):
        new_name,ok = QInputDialog.getText(self,'Input',"Please input a new file name: ",
            QLineEdit.Normal, file_name)
        if ok:
            if new_name == '' or new_name[0] == '/':
                raise LogicError('please input a legal file name')
            else:
                self.session.server_rename(file_name, new_name)
                self.update_cloud_file()
    
    @qt_front_wrapper
    def delete_file_dir(self, name):
        self.session.server_delete(name)
        self.update_cloud_file()
        return

        