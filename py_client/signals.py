from PyQt5.QtCore import pyqtSignal, QObject


class cloudFileSignals(QObject):
    rename = pyqtSignal(str)
    download = pyqtSignal(str, int)
    delete = pyqtSignal(str)
    enter = pyqtSignal(str)
    makedir = pyqtSignal()

class localFileSignals(QObject):
    upload = pyqtSignal(str, int)
    select_root = pyqtSignal()

class transmitSignals(QObject):
    # 第一个代表是否完成，还是中途停止，第二个代表已经传了的大小
    finished = pyqtSignal(bool, int)
    progress = pyqtSignal(int)