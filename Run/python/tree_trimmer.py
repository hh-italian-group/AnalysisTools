#!/usr/bin/env python

'''
NAME
    tree_trimmer.py - general ROOT tree skimmer and slimmmer

SYNOPSIS
    tree_trimmer.py [OPTIONS] file1.root [file2.root ... skim.py] [OPTIONS]

DESCRIPTION
    This is a very general and configurable script for skimming (skipping
    events) and slimming (dropping branches) ROOT TTrees.

    An example call is:
    tree_trimmer.py -t mytree -b vars.txt -s skim -k 'CollectionTree,tauPerfMeta/TrigConfTree' *.root skim.py

    This call would chain together TTrees named 'mytree' in all '*.root' files.
    Then it would turn-off the branch status of all branches in mytree, except
    for those listed in vars.txt, which can look something like this:

    EventNumber
    RunNumber
    lbn
    el_*

    Then, it would skim mytree according to the skim(tree) function specified
    with the option -s, and implemented in skim.py.  The additional trees
    specified by the option -k, would also be kept in their entirety.

OPTIONS
    -h, --help
        Prints this documentation and exits.
       
    -b FILE, --branches_on_file=FILE
        When specified, FILE should be the name of a file containing branch
        names, or fnmatch patterns for branch names, one per line.
        (# comments are allowed.)  This specifies the branches that
        should be turned-on, all others are turned-off.

    -B FILE, --branches_off_file=FILE
        When specified, FILE should be the name of a file containing branch
        names, or fnmatch patterns for branch names, one per line.
        (# comments are allowed.)  This specifies the branches that
        should be turned-off, even if they were turned-on in the
        branches_off_file above; good for specifying exceptions.

    -k 'TREE1,TREE2,...', --keep_trees='TREE1,TREE2,...'
        The names of additional trees (metadata, config, ...) that should be
        coppied in their entirety, in addtion to the main tree being skimmed.

    -m NUM, --max_events=NUM
        Max number of events to run over the input trees.

    -M MB, --max_tree_size=MB
        Specifies the ROOT.TTree.SetMaxTreeSize in MB.

    -o FILE, --out=FILE
        Specifies the name of the output file, 'skim.root' by default.

    -s FUNC, --skim=FUNC
        Specifies the name of a user-supplied skimming function, FUNC(tree),
        that should return True/False if the current entry in tree passes
        the skim.  The user should implement FUNC in a python file and provide
        it as a command-line argument.

            def FUNC(tree):
                return passing_my_criteria(tree)

    -t NAME, --tree=NAME
        Specifies the name of the main tree to be skimmed (skipping events) and
        slimmed (dropping branches), 'H2TauTauTreeProducerTauTau' by default.

AUTHOR
    Ryan Reece  <ryan.reece@cern.ch>

COPYRIGHT
    Copyright 2011 Ryan Reece
    License: GPL <http://www.gnu.org/licenses/gpl.html>

SEE ALSO
    ROOT <http://root.cern.ch>

TO DO
    - what do you want this script to do?

2011-06-15
'''
#------------------------------------------------------------------------------

## std
import optparse
import fnmatch
import os

## ROOT
import ROOT  ##, rootlogon
ROOT.gROOT.SetBatch(True)

## my modules

## local modules

#_____________________________________________________________________________
def options():
    parser = optparse.OptionParser(description="hello_world")
    parser.add_option('-b', '--branches_on_file' , dest='branches_on_file'  , type=str, default='')
    parser.add_option('-B', '--branches_off_file', dest='branches_off_file' , type=str, default='')
    parser.add_option('-k', '--keep_trees'       , dest='keep_trees'        , type=str, default='')
    parser.add_option('-m', '--max_events'       , dest='max_events'        , type=int, default=-1)
    parser.add_option('-M', '--max_tree_size'    , dest='max_tree_size'     , type=int, default=0)
    parser.add_option('-o', '--out'              , dest='out'               , type=str, default='skim.root')
    parser.add_option('-s', '--skim'             , dest='skim'              , type=str, default='')
    parser.add_option('-t', '--tree'             , dest='tree'              , type=str, default='H2TauTauTreeProducerTauTau')
    return parser.parse_args()


#______________________________________________________________________________
def main():
    ops, args = options()

    input_files = filter(lambda arg: fnmatch.fnmatch(arg, '*.root*') , args)
    py_configs = filter(lambda arg: fnmatch.fnmatch(arg, '*.py') , args)

    ## exec config files to declare any skimming functions
    for py in py_configs:
        execfile(py)
   
    ## build the chain
    ch_in = ROOT.TChain(ops.tree)
    for f in input_files:
        ch_in.Add(f)

    ## max_events
    max_events = ch_in.GetEntries()
    if ops.max_events > 0 and max_events > ops.max_events:
        max_events = ops.max_events

    ## branch status
    set_status_of_branches(ch_in, ops.branches_on_file, ops.branches_off_file)

    ## start new file
    new_file = ROOT.TFile(ops.out, 'RECREATE')
    ch_out = ch_in.CloneTree(0)

    ## max_tree_size
    if ops.max_tree_size:
        ch_out.SetMaxTreeSize(ops.max_tree_size*1000000) # ops.max_tree_size in MB

    tree_names_to_keep = filter(None, ops.keep_trees.split(','))

    ## event loop
    i_prev_infile = -1
    i_prev_outfile = -1
    n_events = 0
    n_events_passed = 0
    n_events_hists = []
    outfile_names = []
    infile_names = []
    infile_names_list = []
    h_n_events = None
    for i_event in xrange(max_events):
        i_entry = ch_in.LoadTree(i_event)
        ch_in.GetEntry(i_event)

        if i_event % 1000 == 0:
            print 'Processing event %i of %i' % (i_event, max_events)

        i_infile = ch_in.GetTreeNumber()
        i_outfile = ch_out.GetFileNumber()

        ## check if the output file changed ##
        if i_outfile != i_prev_outfile:
            outfile_name = ch_out.GetCurrentFile().GetName()
            outfile_names.append(outfile_name)
            h_n_events = ROOT.TH1D('h_n_events', '', 20, -0.5, 19.5)
            h_n_events.SetDirectory(0) # memory, not TFile, resident
            n_events_hists.append(h_n_events)
            infile_names_list.append(infile_names)
            del infile_names[:] # clear list of inputfiles read for the previous outfile
#            if i_infile == i_prev_infile: # count the current infile (probably counted twice)
#                infile_names.append(ch_in.GetCurrentFile().GetName())
            print 'Writing to output file: %s' % outfile_name

        ## check if the input file changed ##
        if i_infile != i_prev_infile:
            infile_name = ch_in.GetCurrentFile().GetName()
            infile_names.append(infile_name)
            print 'Reading from input file: %s' % infile_name

        # count all events
        h_n_events.Fill(0)
        n_events += 1

        passed_skim = True
        if ops.skim:
            skim_func = eval(ops.skim)
            passed_skim = skim_func(ch_in)

        if passed_skim:
            # count events passing skim
            ch_out.Fill() # fill the skimmed tree
            h_n_events.Fill(1)
            n_events_passed += 1


        i_prev_infile = i_infile
        i_prev_outfile = i_outfile
        prev_outfile_name = ch_out.GetCurrentFile().GetName()
        ## end of event-loop
   
#    ch_out.Print()
   
    ch_out.GetCurrentFile().Write()
    ch_out.GetCurrentFile().Close()
   
    print 'The skim has finished.'
    print 'n_events        = %16i' % n_events
    print 'n_events_passed = %16i' % n_events_passed
    print 'skim efficiency = %16.3g%%' % (100.0*float(n_events_passed)/n_events)
   
    print 'Saving h_n_events histograms and additional trees...'

    # temporarily SetMaxTreeSize big so as not to change files when writing the addtional trees
    ROOT.TTree.SetMaxTreeSize(3000*1000000)

    for outfile_name, h_n_events, infile_names in zip(outfile_names, n_events_hists, infile_names_list):
        f_out = ROOT.TFile(outfile_name, 'UPDATE')
        f_out.cd()
        h_n_events.Write()
        for tree_name in tree_names_to_keep:
            print 'Saving tree: %s' % tree_name
            cwd = f_out
            tree_dirname = os.path.dirname(tree_name)
            if tree_dirname:
                print 'Creating directory: %s' % tree_dirname
                cwd = f_out.mkdir(tree_dirname)
            tree_out = None
            is_first_infile = True
            for infile_name in infile_names:
                f_in = ROOT.TFile(infile_name, 'READ')
                tree_in = f_in.Get(tree_name)
                cwd.cd()
                if is_first_infile:
                    tree_out = tree_in.CloneTree(0)
                tree_in.CopyAddresses(tree_out)
                tree_out.CopyEntries(tree_in)
                f_in.Close()
            tree_out.Write()
        f_out.Close()
    print '  done.'
    print 'Done.'


#______________________________________________________________________________
def set_status_of_branches(chain, branches_on_file='', branches_off_file=''):
    if branches_on_file or branches_off_file:
        ## get the set of all branches in the tree
        branches_all = [ b.GetName() for b in chain.GetListOfBranches() ]
        branches_on = branches_all

        if branches_on_file:
            ## parse branches_on_file
            patterns = parse_lines(branches_on_file)
       
            ## filter for branches matching branches_on_file
            branches_on = filter(lambda b: any(fnmatch.fnmatch(b, p) for p in patterns), branches_on)

        if branches_off_file:
            ## parse branches_off_file
            patterns = parse_lines(branches_off_file)
       
            ## filter for branches not matching branches_off_file
            branches_on = filter(lambda b: not any(fnmatch.fnmatch(b, p) for p in patterns), branches_on)

        ## set all branches off first
        chain.SetBranchStatus('*',0)

        ## turn on just what we need
        for bn in branches_on:
            chain.SetBranchStatus(bn, 1)
    pass


#______________________________________________________________________________
def parse_lines(fn):
    lines = []
    f = open(fn)
    for line in f:
        line = line.split('#')[0].strip() # remove comments
        lines.append(line)
    f.close()
    return lines


#______________________________________________________________________________
if __name__ == '__main__': main()
