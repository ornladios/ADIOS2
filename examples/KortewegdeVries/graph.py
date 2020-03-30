#!/usr/bin/env python3
import os
import math
import argparse
import diagram
import adios2
import time

def run():
    parser = argparse.ArgumentParser(description=('Displays solution of KdV equation'))
    parser.add_argument("-f", help=".bp filename (default: kdv.bp)", default="kdv.bp")
    options = parser.parse_args()
    dgoptions = diagram.DOption()
    dgoptions.mode = 'g'

    with adios2.open(options.filename, "r") as file_handle:
        for step in file_handle:
            os.system('cls' if os.name == 'nt' else 'clear')
            points = step.read("u")
            values = [None]
            dg = diagram.DGWrapper(dg_option=dgoptions, data=[points, values])
            dg.show()
            time.sleep(0.1)


if __name__ == '__main__':
    run()
