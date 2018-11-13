from PyQt5.QtWidgets import QWidget,QToolTip,QPushButton,QDesktopWidget
from PyQt5.QtGui import QIcon,QFont

class ClientMain(QWidget):
    def __init__(self):
        super().__init__()
        self.initUI()
    
    def center(self):
        qr = self.frameGeometry()
        cp = QDesktopWidget().availableGeometry().center()
        qr.moveCenter(cp)
        self.move(qr.topLeft())
        
    def initUI(self):

        # QToolTip.setFont(QFont('SansSerif',10))
        # self.setToolTip('This is a <b>QWidget</b> widget')

        btn1 = QPushButton('button', self)
        btn1.setToolTip('This is a <b>QPushButton</b> widget')
        btn1.resize(btn1.sizeHint())
        btn1.move(50,50)
        # self.setGeometry(300, 300, 300, 220)
        self.resize(400, 600)
        self.center()
        self.setWindowTitle('FTP Client')
        # self.setWindowIcon(QIcon('web.png'))
    
        self.show()
        
