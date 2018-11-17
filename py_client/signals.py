from PyQt5.QtCore import pyqtSignal, QObject


class fileSignals(QObject):
    rename = pyqtSignal(str)
    download = pyqtSignal(str)
    delete = pyqtSignal(str)
    enter = pyqtSignal(str)
    makedir = pyqtSignal()