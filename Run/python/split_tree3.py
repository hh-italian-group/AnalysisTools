#!/usr/bin/env python
# Split TTree into a separate file.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

import sys
import os
import subprocess
import re
import argparse
import ROOT

parser = argparse.ArgumentParser(description='Split ROOT TTree.',
                                 formatter_class = lambda prog: argparse.HelpFormatter(prog,width=90))
parser.add_argument('--original-file', required=True, dest='originalFile', type=str, metavar='FILE',
                    help="original ROOT file")
parser.add_argument('--tree-name', required=True, dest='treeName', type=str, metavar='NAME', help="Tree name.")
parser.add_argument('--n-entries', required=False, dest='nEntries', type=int, metavar='N', default=-1,
                    help="number of entries to keep in the tree in the original file")
args = parser.parse_args()

namePrefix = re.sub('\.[^\.]*$', '', args.originalFile)
nameFormat = '{}_part{}.root'
original_FileName = '{}_original.root'.format(namePrefix)
part1_FileName = nameFormat.format(namePrefix, 1)
part2_FileName = nameFormat.format(namePrefix, 2)
part3_FileName = nameFormat.format(namePrefix, 3)

os.rename(args.originalFile, original_FileName)

originalFile = ROOT.TFile(original_FileName, 'UPDATE')
part1_File = ROOT.TFile(part1_FileName, 'RECREATE', '', 1 * 100 + 9)
part2_File = ROOT.TFile(part2_FileName, 'RECREATE', '', 1 * 100 + 9)
part3_File = ROOT.TFile(part3_FileName, 'RECREATE', '', 1 * 100 + 9)

originalTree = originalFile.Get(args.treeName)
nEntries = args.nEntries
if nEntries == -1:
    nEntries = originalTree.GetEntries() / 3

part1_File.cd()
part1_Tree = originalTree.CopyTree('', '', nEntries, 0)
part1_Tree.Write()
part1_File.Close()

part2_File.cd()
part2_Tree = originalTree.CopyTree('', '', 2*nEntries, nEntries)
part2_Tree.Write()
part2_File.Close()

part3_File.cd()
part3_Tree = originalTree.CopyTree('', '', originalTree.GetEntries(), 2*nEntries)
part3_Tree.Write()
part3_File.Close()

originalFile.cd()
originalFile.Delete(args.treeName + ';*')
originalFile.Close()

subprocess.call([ 'hadd -f9 {} {} {}'.format(args.originalFile, original_FileName, part1_FileName) ], shell=True)
