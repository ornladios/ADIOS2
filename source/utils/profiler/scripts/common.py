import matplotlib.pyplot as plt
import parseMe
import os


def saveMe(figName):
    parentDir = os.path.dirname(figName)

    plt.tight_layout()
    # print (figName)
    if os.path.isdir(parentDir):
        plt.savefig(figName)
    else:
        if len(parentDir) > 1:
            print("Missing directory! Can not save plot to dir:", parentDir)
        else:
            plt.savefig(figName)

    showFig = parseMe.command_options[parseMe.TAGS["showfig"]]
    if showFig:
        plt.show()
