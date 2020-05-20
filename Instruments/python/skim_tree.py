# Skim TTree.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

import argparse

parser = argparse.ArgumentParser(description='Skim tree.')
parser.add_argument('--input', required=True, type=str, help="input root file")
parser.add_argument('--output', required=True, type=str, help="output root file")
parser.add_argument('--tree', required=True, type=str, help="selection")
parser.add_argument('--sel', required=True, type=str, help="selection")
parser.add_argument('--include-columns', required=False, default='', type=str, help="columns to be included")
parser.add_argument('--exclude-columns', required=False, default='', type=str, help="columns to be excluded")
parser.add_argument('--n-threads', required=False, type=int, default=4, help="number of threads")
args = parser.parse_args()

import ROOT
ROOT.gROOT.SetBatch(True)

if args.n_threads > 1:
    ROOT.ROOT.EnableImplicitMT(args.n_threads)
columns_to_include = args.include_columns.split(',')
columns_to_exclude = args.exclude_columns.split(',')

df = ROOT.RDataFrame(args.tree, args.input)
branches = ROOT.vector('string')()
for column in df.GetColumnNames():
    if (len(columns_to_include) and column in columns_to_include) or (column not in columns_to_exclude):
        branches.push_back(column)

df = df.Filter(args.sel)
opt = ROOT.RDF.RSnapshotOptions()
opt.fCompressionAlgorithm = ROOT.ROOT.kLZ4
opt.fCompressionLevel = 4
df.Snapshot(args.tree, args.output, branches, )
