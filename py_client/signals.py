from PyQt5.QtCore import pyqtSignal, QObject

# 自定义信号：云端文件操作相关
class cloudFileSignals(QObject):
    rename = pyqtSignal(str)
    download = pyqtSignal(str, int)
    delete = pyqtSignal(str)
    enter = pyqtSignal(str)
    makedir = pyqtSignal()

# 自定义信号：本地文件操作相关
class localFileSignals(QObject):
    upload = pyqtSignal(str, int)
    select_root = pyqtSignal()

# 发送文件的相关信号
class transmitSignals(QObject):
    # 第一个代表是否完成，还是中途停止，第二个代表已经传了的大小
    finished = pyqtSignal(bool, int)
    progress = pyqtSignal(int)