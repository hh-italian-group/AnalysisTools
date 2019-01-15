#!/usr/bin/env python
# Transfer files using gfal tools.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

import argparse
import sys
import os
import re
import subprocess
import xml.etree.ElementTree

parser = argparse.ArgumentParser(description='Transfer files using gfal tools.',
                                 formatter_class = lambda prog: argparse.HelpFormatter(prog,width=90))
parser.add_argument('--max-tries', required=False, type=int, default=10,
                    help="maximal number of tries before failing")
parser.add_argument('input', nargs=1, type=str,
                    help="input location in the format SITE:path (e.g. T2_IT_Pisa:/store/user/foo/bar)")
parser.add_argument('output', nargs=1, type=str,
                    help="output location in the local file system")
args = parser.parse_args()

class FileDesc:
    def __init__(self, name, size):
        self.name = name
        self.size = size
        self.size_MB = float(size) / (1024 ** 2)

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

def CollectFiles(base_url, sub_folder = ''):
    files = []
    full_url = base_url
    if len(sub_folder) != 0:
        full_url += '/' + sub_folder
    p = subprocess.Popen(['gfal-ls -l {0}'.format(full_url)], shell=True,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()

    if len(err) != 0:
        raise RuntimeError(err)

    item_lines = [ s.strip() for s in out.split('\n') if len(s.strip()) != 0 ]
    for item in item_lines:
        prop = [ s for s in item.split(' ') if len(s) != 0 ]
        item_name = prop[-1]
        item_size = int(prop[4])
        is_dir = prop[0][0] == 'd'
        rel_name = ''
        if len(sub_folder):
            rel_name += sub_folder + '/'
        rel_name += item_name
        if is_dir:
            item_files = CollectFiles(base_url, rel_name)
            files.extend(item_files)
        else:
            files.append(FileDesc(rel_name, item_size))
    return files

def TransferFile(source_file, destination_file, number_of_threads):
    cmd = 'gfal-copy -n {0} "{1}" "{2}"'.format(number_of_threads, source_file, destination_file)
    p = subprocess.Popen([cmd], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    ok = len(err) == 0 and p.returncode == 0
    if not ok:
        print(err)
    return ok

input = args.input[0]
split_input = input.split(':')
if len(split_input) != 2:
    raise RuntimeError("Invalid input")
site_name = split_input[0]
input_path = split_input[1]
input_name = [ s for s in input_path.split('/') if len(s) != 0 ][-1]

output = args.output[0]
full_output = os.path.join(output, input_name)
abs_output = os.path.abspath(full_output)
print('"{0}" -> "{1}"'.format(input, output))


input_url = GetSitePfnPath(site_name, input_path)

files = CollectFiles(input_url)

total_size_MB = 0.
total_n_files = len(files)

for file in files:
    total_size_MB += file.size_MB

file_id = 0
try_id = 1
processed_MB = 0.
while file_id < len(files):
    file = files[file_id]
    local_path = os.path.join(abs_output, file.name)
    display_name = os.path.join(input_name, file.name)
    if os.path.isfile(local_path):
        if os.path.getsize(local_path) == file.size:
            print('OK {0}'.format(display_name))
            file_id += 1
            try_id = 1
            processed_MB += file.size_MB
            continue
        os.remove(local_path)
    if try_id > args.max_tries:
        raise RuntimeError('Max number of tries is reached while transfering "{}".'.format(display_name))
    local_dir = os.path.join(abs_output, os.path.dirname(file.name))
    if not os.path.isdir(local_dir):
        os.makedirs(local_dir)
    print('Progress: {0}/{1} ({2:.1f}%) files, {3:.1f}/{4:.1f} ({5:.1f}%) MiB.') \
            .format(file_id, total_n_files, float(file_id) / total_n_files * 100.,
                    processed_MB, total_size_MB, processed_MB / total_size_MB * 100.)
    print('Downloading "{0}" ({1:.1f} MiB, try {2})...'.format(display_name, file.size_MB, try_id))
    transfered = TransferFile(input_url + '/' + file.name, 'file://' + local_path, 2)
    if not transfered and os.path.isfile(local_path):
        os.remove(local_path)
    try_id += 1

print('"{0}" -> "{1}" successfully finished.'.format(input, output))
