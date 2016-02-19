## Definition of the function to read intput file list from a text file.
## This file is part of https://github.com/hh-italian-group/AnalysisTools.

def readFileList(fileList, inputFileName, fileNamePrefix):
    inputFile = open(inputFileName, 'r')
    for name in inputFile:
        fileList.extend([ fileNamePrefix + name ])
    inputFile.close()
