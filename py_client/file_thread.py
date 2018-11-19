from PyQt5 import QtCore
from signals import transmitSignals

class downloadThread(QtCore.QThread):
    def __init__(self, data_sock, file_obj, BUFF_SIZE, total_size,
        progress_func=None, finish_func=None, have_done=0):
        super(downloadThread, self).__init__()
        self.data_sock = data_sock
        self.file_obj = file_obj
        self.BUFF_SIZE = BUFF_SIZE
        self.total_size = total_size
        self.transmit_signal = transmitSignals()
        if progress_func:
            self.transmit_signal.progress.connect(progress_func)
        if finish_func:
            self.transmit_signal.finished.connect(finish_func)
        self.stop_transmit = False
        self.have_done = have_done

    def run(self):
        current_size = self.have_done
        self.transmit_signal.progress.emit(0)
        while True:
            if self.stop_transmit:
                break
            data = self.data_sock.recv(self.BUFF_SIZE)
            data_size = len(data)
            if data is None or data_size == 0:
                break
            self.file_obj.write(data)
            current_size += data_size
            self.transmit_signal.progress.emit(int(current_size/self.total_size * 100))
        self.data_sock.close()
        self.data_sock = None
        self.file_obj.close()
        if self.stop_transmit:
            self.transmit_signal.finished.emit(False, current_size)
        else:
            self.transmit_signal.finished.emit(True, current_size)
            

class uploadThread(QtCore.QThread):
    def __init__(self, data_sock, file_obj, BUFF_SIZE, total_size,
        progress_func=None, finish_func=None, have_done=0):
        super(uploadThread, self).__init__()
        self.data_sock = data_sock
        self.file_obj = file_obj
        self.BUFF_SIZE = BUFF_SIZE
        self.total_size = total_size
        self.transmit_signal = transmitSignals()
        if progress_func:
            self.transmit_signal.progress.connect(progress_func)
        if finish_func:
            self.transmit_signal.finished.connect(finish_func)
        self.stop_transmit = False
        self.have_done = have_done
    
    def run(self):
        current_size = self.have_done
        self.transmit_signal.progress.emit(0)
        while True:
            if self.stop_transmit:
                break
            data = self.file_obj.read(self.BUFF_SIZE)
            data_size = len(data)
            if data is None or data_size == 0:
                break
            self.data_sock.send(data)
            current_size += data_size
            self.transmit_signal.progress.emit(int(current_size/self.total_size * 100))
        self.data_sock.close()
        self.data_sock = None
        self.file_obj.close()
        if self.stop_transmit:
            self.transmit_signal.finished.emit(False, current_size)
        else:
            self.transmit_signal.finished.emit(True, current_size)

