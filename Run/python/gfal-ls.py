#!/usr/bin/env python
# List files using gfal tools.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

import argparse
import sys
import os
import re
import subprocess
import xml.etree.ElementTree

parser = argparse.ArgumentParser(
    description='List files using gfal tools.' \
                '\nRemote format is SITE:path (e.g. T2_IT_Pisa:/store/user/foo/bar)',
                                 formatter_class = lambda prog: argparse.HelpFormatter(prog,width=90))
parser.add_argument('--verbose', action="store_true", help="print verbose output")
parser.add_argument('path', nargs=1, type=str, help="remote location")
args = parser.parse_args()

def GetPfnPath(lfn_to_pfn, path, protocol):
    for rule in lfn_to_pfn:
        if rule.get('protocol') != protocol: continue
        path_match = rule.get('path-match')
        if path_match is None or re.match(path_match, path) is None: continue
        result = rule.get('result')
        chained_protocol = rule.get('chain')
        if chained_protocol is not None:
            path = GetPfnPath(lfn_to_pfn, path, chained_protocol)
        fixed_path = re.sub(path_match, '\\1', path)
        return result.replace('$1', fixed_path)
    raise RuntimeError("Protocol {0} not found".format(protocol))

def GetSitePfnPath(site_name, path):
    desc_file = '/cvmfs/cms.cern.ch/SITECONF/{0}/PhEDEx/storage.xml'.format(site_name)
    if not os.path.isfile(desc_file):
        raise RuntimeError("Storage description for {0} not found.".format(site_name))
    storage_desc = xml.etree.ElementTree.parse(desc_file).getroot()
    lfn_to_pfn =storage_desc.findall('lfn-to-pfn')
    return GetPfnPath(lfn_to_pfn, path, 'srmv2')

class TargetDesc:
    def __init__(self, target_str):
        self.target_str = target_str
        split_target = target_str.split(':')
        if len(split_target) != 2:
            raise RuntimeError('Invalid target string {0}'.format(target_str))
        self.site_name = split_target[0]
        self.path = split_target[1]
        self.url = GetSitePfnPath(self.site_name, self.path)

target = TargetDesc(args.path[0])
cmd = 'gfal-ls -l {0}'.format(target.url)
if args.verbose:
    print('> {0}'.format(cmd))
subprocess.call([cmd], shell=True)
