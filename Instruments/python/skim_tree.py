# Skim TTree.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

import argparse

parser = argparse.ArgumentParser(description='Skim tree.')
parser.add_argument('--input', required=True, type=str, help="input root file or txt file with a list of files")
parser.add_argument('--output', required=True, type=str, help="output root file")
parser.add_argument('--tree', required=True, type=str, help="selection")
parser.add_argument('--sel', required=False, type=str, default='', help="selection")
parser.add_argument('--include-columns', required=False, default='', type=str, help="columns to be included")
parser.add_argument('--exclude-columns', required=False, default='', type=str, help="columns to be excluded")
parser.add_argument('--input-prefix', required=False, type=str, default='',
                    help="prefix to be added to input each input file")
parser.add_argument('--processing-module', required=False, type=str, default='',
                    help="Python module used to process DataFrame. Should be in form file:method")
parser.add_argument('--n-threads', required=False, type=int, default=4, help="number of threads")
args = parser.parse_args()

import ROOT
ROOT.gROOT.SetBatch(True)

if args.n_threads > 1:
    ROOT.ROOT.EnableImplicitMT(args.n_threads)
columns_to_include = args.include_columns.split(',')
columns_to_exclude = args.exclude_columns.split(',')

inputs = ROOT.vector('string')()
if args.input.endswith('.root'):
    inputs.push_back(args.input_prefix + args.input)
    print("Adding input '{}'...".format(args.input))
elif args.input.endswith('.txt'):
    with open(args.input, 'r') as input_list:
        for name in input_list.readlines():
            name = name.strip()
            if len(name) > 0 and name[0] != '#':
                inputs.push_back(args.input_prefix + name)
                print("Adding input '{}'...".format(name))
else:
    raise RuntimeError("Input format is not supported.")

df = ROOT.RDataFrame(args.tree, inputs)

if len(args.processing_module):
    module_desc = args.processing_module.split(':')
    import imp
    module = imp.load_source('processing', module_desc[0])
    fn = getattr(module, module_desc[1])
    df = fn(df)

branches = ROOT.vector('string')()
for column in df.GetColumnNames():
    include_column = False
    if len(columns_to_include) == 0 or column in columns_to_include:
        include_column = True
    if column in columns_to_exclude:
        include_column = False
    if include_column:
        branches.push_back(column)
        print("Adding column '{}'...".format(column))


if len(args.sel):
    df = df.Filter(args.sel)

opt = ROOT.RDF.RSnapshotOptions()
opt.fCompressionAlgorithm = ROOT.ROOT.kLZ4
opt.fCompressionLevel = 4
df.Snapshot(args.tree, args.output, branches, opt)
