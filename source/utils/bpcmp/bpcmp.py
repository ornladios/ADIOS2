import numpy as np
import sys
import errno
import argparse
from adios2 import Stream
import itertools

verbose = False

def DetectMismatch(val1, val2, tolerance, tolperc):
    """
    Detects the mismatch between two scalar of any comparable type.

    Args:
        val1 : Any type scalar
        val2 : Any type scalar
        tolerance (float) : Tolerance used by the comparison
        tolperc (float) : Accepted tolerance between the two values as percentage

    Returns:
        bool: true (if there is a mismatch), false otherwise
    """
    mismatch = True
    if val1 == val2:
        return False
    if tolerance > 0:
        mismatch = (np.abs(val1 - val2) > tolerance)
    if tolperc > 0:
        if val1 == 0 and val2 != 0:
            mismatch = True
        else:
            mismatch = (np.abs(val1 - val2) / val1 > tolperc)
    if tolerance == 0 and tolperc == 0:
        mismatch = (val1 != val2)
    return mismatch

def CompareValues(array1, array2, tolerance, tolperc):
    """
    Detects the mismatch between two N dim arrays

    Args:
        array1 : N dimensional array
        array2 : N dimensional array
        tolerance (float) : Tolerance used by the comparison
        tolperc (float) : Accepted tolerance between the two values as percentage

    Returns:
        list(tuple): a list of mismatch values in the array: (index, value1, value2)
    """
    mismatch = []
    dimensions = array1.shape
    for idx in itertools.product(*[range(s) for s in dimensions]):
        if DetectMismatch(array1[idx], array2[idx], tolerance, tolperc):
            mismatch.append((idx, array1[idx], array2[idx]))
    return mismatch

def CompareFiles(filename1, filename2, tolerance, tolperc, stats=False):
    """
    Detects the mismatch between two BP files

    Args:
        filename1 : BP file name
        filename2 : BP file name
        tolerance (float) : Tolerance used by the comparison
        tolperc (float) : Accepted tolerance between the two values as percentage
        stats (bool) : boolean that constrols if stats are compared or not

    Returns:
        int: total number of mismatches between the two files
    """
    err = 0
    with Stream(filename1, 'rra') as s1, Stream(filename2, 'rra') as s2:
        # compare total number of steps
        num_steps = s1.num_steps()
        if num_steps != s2.num_steps():
            print(f"DIFFER : NUM_STEPS : {num_steps} <> {s2.num_steps()} : {filename1}"
                  f" : {filename2}")
            num_steps = min(num_steps, s2.num_steps())
            err += 1

        varInfoS1 = s1.available_variables()
        varInfoS2 = s2.available_variables()
        notinS1 = np.setdiff1d(list(varInfoS2.keys()), list(varInfoS1.keys()))
        notinS2 = np.setdiff1d(list(varInfoS1.keys()), list(varInfoS2.keys()))
        # compare total number of written variables
        if len(notinS1) > 0 or len(notinS1) > 0:
            print(f"DIFFER : AVAIL_VARS : {notinS1} <> {notinS2} : {filename1} : {filename2}")
            err += 1

        # compare each common variable
        for varName in np.intersect1d(list(varInfoS1.keys()), list(varInfoS2.keys())):
            # compare number of steps
            varSteps = int(varInfoS1[varName]["AvailableStepsCount"])
            if varSteps != int(varInfoS2[varName]["AvailableStepsCount"]):
                print(f"DIFFER : VARIABLE : {varName} : NUM_STEPS: "
                      f"{varInfoS1[varName]['AvailableStepsCount']} <> "
                      f"{varInfoS2[varName]['AvailableStepsCount']} : {filename1} : {filename2}")
                varSteps = min(varSteps, int(varInfoS2[varName]["AvailableStepsCount"]))
                err += 1

            # compare types
            if varInfoS1[varName]["Type"] != varInfoS2[varName]["Type"]:
                print(f"DIFFER : VARIABLE : {varName} : TYPE : {varInfoS1[varName]['Type']}"
                      f" <> {varInfoS2[varName]['Type']} : {filename1} : {filename2}")
                err += 1

            # compare dimensions (single value and shape)
            if varInfoS1[varName]["SingleValue"] != varInfoS2[varName]["SingleValue"]:
                print(f"DIFFER : VARIABLE : {varName} : SINGLE_VALUE : "
                      f"{varInfoS1[varName]['SingleValue']} <> "
                      f"{varInfoS2[varName]['SingleValue']} : {filename1} : {filename2}")
                err += 1
                # no point in comparing values if there is a difference in shape
                continue

            if varInfoS1[varName]["Shape"] != varInfoS2[varName]["Shape"]:
                print(f"DIFFER : VARIABLE : {varName} : SHAPE : {varInfoS1[varName]['Shape']}"
                      f" <> {varInfoS2[varName]['Shape']} : {filename1} : {filename2}")
                err += 1
                # no point in comparing values if there is a difference in shape
                continue

            if stats:
                # compare min/max
                if DetectMismatch(float(varInfoS1[varName]["Min"]),
                                  float(varInfoS2[varName]["Min"]),
                                  tolerance, tolperc):
                    print(f"DIFFER : VARIABLE : {varName} : MIN : {varInfoS1[varName]['Min']}"
                          f" <> {varInfoS2[varName]['Min']} : {filename1} : {filename2}")
                    err += 1
                if DetectMismatch(float(varInfoS1[varName]["Max"]),
                                  float(varInfoS2[varName]["Max"]),
                                  tolerance, tolperc):
                    print(f"DIFFER : VARIABLE : {varName} : MAX : {varInfoS1[varName]['Max']}"
                          f" <> {varInfoS2[varName]['Max']} : {filename1} : {filename2}")
                    err += 1
            # compare values
            for step in range(varSteps):
                values1 = s1.read(varName, step_selection=[step, 1])
                values2 = s2.read(varName, step_selection=[step, 1])
                ret = CompareValues(values1, values2, tolerance, tolperc)
                if len(ret) == 0:
                    continue
                err += len(ret)
                if verbose:
                    for (idx, value1, value2) in ret:
                        print(f"DIFFER : VARIABLE : {varName} : STEP : {step} : POSITION : "
                              f"{idx} : VALUE : {value1} <> {value2} : {filename1} : {filename2}")
                else:
                    print(f"DIFFER : VARIABLE : {varName} : STEP : {step} : VALUES_SUMMARY : "
                          f"{len(ret)} : {filename1} : {filename2}")

    return err


if __name__ == '__main__':
    description = 'Utility code to compare the data within BP files'
    epilog = 'If more than two files are provided, all files are compared to the first'
    parser = argparse.ArgumentParser(prog="bpcmp", description=description, epilog=epilog)

    parser.add_argument('bpfiles', metavar='file.bp', nargs='+',
                        help='list of BP files to be compared')
    parser.add_argument('-t', '--tolerance', dest='tolerance', type=float, default=0,
                        help='tolerance for comparing values (default: 0)')
    parser.add_argument('-p', '--perc-tolerance', dest='tolperc', type=float, default=0,
                        help='tolerance as percentage of values (default: 0)')
    parser.add_argument('-s', '--stats', action='store_true',
                        help='compare stats (min/max) and not values')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='all different values are printed, not just a summary')

    args = parser.parse_args()
    if len(args.bpfiles) < 2:
        print("WARNING : At least two files need to be provided for any comparison")
        exit(errno.ENOEXEC)

    verbose = args.verbose

    diff_found = 0
    for i in range(1, len(args.bpfiles)):
        print(f"COMPARE : FILE : {args.bpfiles[0]} : FILE : {args.bpfiles[i]}")
        diff_file = CompareFiles(args.bpfiles[0], args.bpfiles[i], args.tolerance, args.tolperc,
                                 args.stats)
        if diff_file > 0:
            print(f"TOTAL_DIFFERENCES : {diff_file} : FILE : {args.bpfiles[0]} : FILE : "
                  f"{args.bpfiles[i]}")
            diff_found += 1

    if diff_found == 0:
        print("DONE : Files have the same content")
