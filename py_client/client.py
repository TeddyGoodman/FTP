#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys
from PyQt5.QtWidgets import QApplication
import widget
        
if __name__ == '__main__':
    
    app = QApplication(sys.argv)
    ex = widget.ClientMain()
    sys.exit(app.exec_())