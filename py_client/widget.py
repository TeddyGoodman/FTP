from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
from PyQt5.QtCore import *

class ClientMain(QWidget):
    def __init__(self):
        super().__init__()
        self.initUI()
    
    def center(self):
        qr = self.frameGeometry()
        cp = QDesktopWidget().availableGeometry().center()
        qr.moveCenter(cp)
        self.move(qr.topLeft())

    def connect_login_ui(self):
        self.ip = QLabel('IP/Domain')
        self.port = QLabel('Port')
        self.user = QLabel('User')
        self.password = QLabel('Password')

        self.ipEdit = QLineEdit()
        self.portEdit = QLineEdit()
        self.userEdit = QLineEdit()
        self.passwordEdit = QLineEdit()

        self.connect_btn = QPushButton('Connect')
        self.login_btn = QPushButton('Login')
        return

    def files_ui(self):
        self.cmd_prompt = QLabel('Command Prompt:')
        self.cmd_prompt_browser = QTextBrowser()

        self.local_files = QLabel('Local Files')
        self.cloud_files = QLabel('Cloud Files')
        self.local_files_browser = QTextBrowser()
        self.cloud_files_browser = QTextBrowser()

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
    
        self.show()
    
    def closeEvent(self, event):
        reply = QMessageBox.question(self, 'Message', 'Are you sure to quit?',
            QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)
        
        if reply == QMessageBox.Yes:
            event.accept()
        else:
            event.ignore()
        
