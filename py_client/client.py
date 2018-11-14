#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys
from PyQt5.QtWidgets import QApplication
import widget
from client_core import *
        
if __name__ == '__main__':
    a = ClientSession('/User')
    # app = QApplication(sys.argv)
    # ex = widget.ClientMain()
    # sys.exit(app.exec_())