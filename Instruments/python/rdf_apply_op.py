# Apply operation on TTree.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

import argparse

parser = argparse.ArgumentParser(description='Skim tree.')
parser.add_argument('--tree', required=True, type=str, help="selection")
parser.add_argument('--op', required=True, type=str, help="operation to apply")
parser.add_argument('--branch', required=True, type=str, help="name of the branch to which operation should be applied")
parser.add_argument('--sel', required=False, type=str, default=None, help="selection")
parser.add_argument('--n-threads', required=False, type=int, default=4, help="number of threads")
parser.add_argument('input', nargs='+', type=str, help="input root files")
args = parser.parse_args()

import ROOT
ROOT.gROOT.SetBatch(True)

if args.n_threads > 1:
    ROOT.ROOT.EnableImplicitMT(args.n_threads)

input_files = ROOT.vector('string')()
for f in args.input:
    input_files.push_back(f)
df = ROOT.RDataFrame(args.tree, input_files)
if args.sel is not None and len(args.sel) > 0:
    df = df.Filter(args.sel)

supported_op = ['Sum', 'Mean', 'StdDev', 'Min', 'Max']
if args.op in supported_op:
    op_result = getattr(df, args.op)(args.branch)
else:
    raise RuntimeError('Operation "{}" is not supported. Supported operations: {}' \
                       .format(args.op, ', '.join(supported_op)))

print(op_result.GetValue())
