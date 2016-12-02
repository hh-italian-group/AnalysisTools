#!/usr/bin/env python
# Copy TTree.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

import sys
import argparse
import ROOT

parser = argparse.ArgumentParser(description='Copy ROOT TTree.',
                                 formatter_class = lambda prog: argparse.HelpFormatter(prog,width=90))
parser.add_argument('--input-file', required=True, dest='inputFile', type=str, metavar='FILE', help="input ROOT file")
parser.add_argument('--output-file', required=True, dest='outputFile', type=str, metavar='FILE',
                    help="output ROOT file")
parser.add_argument('--tree-name', required=True, dest='treeName', type=str, metavar='NAME', help="Tree name.")
parser.add_argument('--selection', required=False, dest='selection', type=str, metavar='SEL', default='',
                    help="event selection")
parser.add_argument('--n-entries', required=False, dest='nEntries', type=int, metavar='N', default=-1,
                    help="number of entries to copy")
parser.add_argument('--first-entry', required=False, dest='firstEntry', type=int, metavar='N', default=0,
                    help="first entry to copy")
args = parser.parse_args()

inputFile = ROOT.TFile(args.inputFile, 'READ')
outputFile = ROOT.TFile(args.outputFile, 'RECREATE', '', 1 * 100 + 9)
originalTree = inputFile.Get(args.treeName)
nEntries = args.nEntries
if nEntries == -1:
    nEntries = originalTree.GetEntries()
newTree = originalTree.CopyTree(args.selection, '', nEntries, args.firstEntry)
newTree.Write()
