#!/usr/bin/env python3
import os
import time
import argparse
import numpy
import adios2
import diagram

def run():
    parser = argparse.ArgumentParser(description=('Graph KdV equation'))
    parser.add_argument("-f", "--filename",
                        help=".bp filename", default="kdv.bp")
    options = parser.parse_args()
    dgoptions = diagram.DOption()
    dgoptions.mode = 'g'

    with adios2.open(options.filename, "r") as file_handle:
        for step in file_handle:
            points = step.read("u")
            # Adding these points helps us with whiplash;
            # otherwise the data is minmaxed on every iteration.
            # It does produce a visual artefact that I wish wasn't there!
            points = numpy.append(points, [3.0, -1.5])
            values = [None]
            dg = diagram.DGWrapper(dg_option=dgoptions, data=[points, values])
            os.system('cls' if os.name == 'nt' else 'clear')
            dg.show()
            time.sleep(0.1)


if __name__ == '__main__':
    run()
