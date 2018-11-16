from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
from PyQt5.QtCore import *
from client_core import ClientSession
import os
from bases import LogicError, InternalError
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

class ClientMain(QWidget):
    def __init__(self):
        super().__init__()
        self.session = ClientSession(root=os.getcwd(), outer_show=self.show_command)
        self.initUI()
    
    def center(self):
        qr = self.frameGeometry()
        cp = QDesktopWidget().availableGeometry().center()
        qr.moveCenter(cp)
        self.move(qr.topLeft())

    def show_command(self, msg):
        self.cmd_prompt_browser.append(msg.rstrip('\r\n'))

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

    def files_ui(self):
        self.cmd_prompt = QLabel('Command Prompt:')
        self.cmd_prompt_browser = QTextBrowser()

        self.local_files = QLabel('Local Files')
        self.cloud_files = QLabel('Cloud Files')
        self.local_files_browser = QTextBrowser()
        self.cloud_files_browser = QListWidget()
        # self.cloud_files_browser.setFont(QFont('SansSerif', 13))

        self.downloading = QLabel('Downloading')
        self.downloading_browser = QLineEdit()
        return

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

    def initUI(self):
        # declare ui
        self.connect_login_ui()
        self.files_ui()

        # set ui
        self.set_grid_ui()
        
        self.setLayout(self.grid)
        self.resize(1000, 600)
        self.center()
        self.setWindowTitle('FTP Client')
        # self.setWindowIcon(QIcon('web.png'))

        # connect events
        self.connect_btn.clicked.connect(self.connect_server)
        self.login_btn.clicked.connect(self.server_login)
    
        self.show()
    
    def clear_ui(self):
        self.cmd_prompt_browser.clear()
        self.cloud_files_browser.clear()
    
    def closeEvent(self, event):
        reply = QMessageBox.question(self, 'Message', 'Are you sure to quit?',
            QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)
        
        if reply == QMessageBox.Yes:
            event.accept()
            if self.session.status != self.session.STATUS_OFFLINE:
                self.session.disconnect()
        else:
            event.ignore()
    
    @qt_front_wrapper
    def connect_server(self, *args, **kwargs):
        if self.session.status == self.session.STATUS_OFFLINE:
            ip = self.ipEdit.text()
            port = self.portEdit.text()
            if self.ipEdit.text() == '' or self.portEdit.text() == '':
                raise LogicError('Please input correct ip and port')
            self.session.connect(ip, int(port))
            QMessageBox.information(self, 'Successful', 'you have connected', 
                QMessageBox.Yes, QMessageBox.Yes)
            self.connect_btn.setText('disconnect')
        else:
            self.session.disconnect()
            self.clear_ui()
            QMessageBox.information(self, 'Successful', 'you have disconnected', 
                QMessageBox.Yes, QMessageBox.Yes)
            self.connect_btn.setText('connect')

    @qt_front_wrapper
    def server_login(self, *args, **kwargs):
        name = self.userEdit.text()
        password = self.passwordEdit.text()
        self.session.login(name, password) # 登录
        self.session.get_server_root() # 获取根目录
        files_str = self.session.listServerFile() # 获取目录下所有文件

        # 解析目录下的文件
        files = files_str.splitlines()
        for line in files:
            info = utility.parse_file_info(line)
            if info['type'] == 1:
                self.cloud_files_browser.addItem(info['name'] + '/')
            else:
                self.cloud_files_browser.addItem(info['name'])
        return
        # print(files)