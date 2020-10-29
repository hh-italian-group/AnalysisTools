# Merge multiple TTrees.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.
import uproot
import argparse
import tqdm

parser = argparse.ArgumentParser(description='Merging Multiple TTrees.')
parser.add_argument('--files', required=True, type=str, nargs='+', help="Input the files names")
parser.add_argument('--out', required=True, type=str, help="Output ROOT file")
parser.add_argument('--tree', required=True, type=str, help="Tree name")
parser.add_argument('--tree-out', required=False, type=str, default=None, help=" out tree name")
parser.add_argument('--chunk-size', required=False, type=int, default=1000000, help="Number of entries per iteration")
parser.add_argument('--compression_type', required=False, default='LZMA')
parser.add_argument('--compression_level', required=False, type=int, default=9)
args = parser.parse_args()
if args.tree_out is None:
    args.tree_out = args.tree
branches={}
n=0
trees=[]
n_entries=uproot.open((args.files)[0])[args.tree].numentries
pbar=tqdm.tqdm(total=n_entries)
for file in (args.files):
    tree= uproot.open(file)[args.tree]
    if tree.numentries==n_entries:
        trees.append(tree)
    else:
        raise RuntimeError('all trees must have the same number of entries!')
with uproot.recreate(args.out) as f:
    f.compression = getattr(uproot.write.compress, args.compression_type)(args.compression_level)
    while n < n_entries:
        entrystart = n
        entrystop = min(n_entries, n + args.chunk_size)
        for tree in trees:
            branches.update(tree.arrays(entrystart=entrystart, entrystop=entrystop))
        if n==0:
            branches_dict = {k:branches[k].dtype for k in branches.keys()}
            f[args.tree_out] = uproot.newtree(branches_dict)
        f[args.tree_out].extend(branches)
        pbar.update(entrystop-entrystart)
        n+=args.chunk_size

pbar.close()
print("The merging ended successfully")

