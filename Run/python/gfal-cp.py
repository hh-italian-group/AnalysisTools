#!/usr/bin/env python
# Transfer files using gfal tools.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

import argparse
import sys
import os
import re
import subprocess
import xml.etree.ElementTree

parser = argparse.ArgumentParser(
    description='Transfer files using gfal tools.' \
                '\nRemote format is SITE:path (e.g. T2_IT_Pisa:/store/user/foo/bar)',
                                 formatter_class = lambda prog: argparse.HelpFormatter(prog,width=90))
parser.add_argument('--include', required=False, default=None, help='regex to match files that should be included')
parser.add_argument('--exclude', required=False, default=None, help='regex to match files that should be excluded')
parser.add_argument('--max-tries', required=False, type=int, default=10,
                    help="the maximum number of tries before failing")
parser.add_argument('--n-streams', required=False, type=int, default=2,
                    help="the maximum number of parallel streams to use for the copy")
parser.add_argument('--verbose', action="store_true", help="print verbose output")
parser.add_argument('--dry-run', action="store_true", help="dry run (don't do actual copying)")
parser.add_argument('--xrootd', action="store_true",
                    help="use xrootd to copy (works only for remote -> local transfers)")
parser.add_argument('input', nargs=1, type=str,
                    help="input location (local or remote)")
parser.add_argument('output', nargs=1, type=str,
                    help="output location (local or remote)")
args = parser.parse_args()

def Run(cmd):
    if args.verbose:
        print('> {0}'.format(cmd))
    p = subprocess.Popen([cmd], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    return out, err, p.returncode

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
    desc_path = '/cvmfs/cms.cern.ch/SITECONF/{0}/PhEDEx'.format(site_name)
    desc_file_names = [ 'storage.xml', 'storage_disk.xml' ]
    desc_file_found = False
    for name_cand in desc_file_names:
        desc_file = '{0}/{1}'.format(desc_path, name_cand)
        desc_file_found = os.path.isfile(desc_file)
        if desc_file_found: break
    if not desc_file_found:
        raise RuntimeError("Storage description for {0} not found.".format(site_name))
    storage_desc = xml.etree.ElementTree.parse(desc_file).getroot()
    lfn_to_pfn =storage_desc.findall('lfn-to-pfn')
    return GetPfnPath(lfn_to_pfn, path, 'srmv2')

def GetRemoteFileInfo(url):
    out, err, returncode = Run('gfal-stat {0}'.format(url))
    if len(err) != 0:
        raise RuntimeError(err)
    info_lines = [ s.strip() for s in out.split('\n') if len(s.strip()) != 0 ]
    if len(info_lines) >= 2:
        size_info = re.search('^ *Size: *([0-9]*) *\t*(.*)', info_lines[1])
        if size_info is not None and len(size_info.groups()) == 2:
            size = int(size_info.group(1))
            type = size_info.group(2)
            if type in ['directory', 'regular file']:
                return size, type == 'directory'

    raise RuntimeError('Unable to get stat for remote file "{0}".\nRemote output:\n{1}'.format(url, out))

def CollectFiles(target_desc, include_pattern = None, exclude_pattern = None, sub_folder = ''):
    files = []

    if len(sub_folder) == 0:
        if target_desc.local:
            if os.path.isfile(target_desc.path):
                files.append(FileDesc(os.path.basename(target_desc.path), os.path.getsize(target_desc.path)))
                return files, True
        else:
            size, is_dir = GetRemoteFileInfo(target_desc.url)
            if not is_dir:
                files.append(FileDesc(os.path.basename(target_desc.path)), size)
                return files, True

    if target_desc.local:
        ls_path = target_desc.path
        ls_tool = 'ls'
    else:
        ls_path = target_desc.url
        ls_tool = 'gfal-ls'

    if len(sub_folder) != 0:
        ls_path += '/' + sub_folder
    out, err, returncode = Run('{0} -l {1}'.format(ls_tool, ls_path))
    if len(err) != 0:
        raise RuntimeError(err)

    item_lines = [ s.strip() for s in out.split('\n') if len(s.strip()) != 0 ]
    if target_desc.local and item_lines[0][0:5] == 'total':
        item_lines = item_lines[1:]

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
            item_files, _ = CollectFiles(target_desc, include_pattern, exclude_pattern, rel_name)
            files.extend(item_files)
        else:
            if (include_pattern is None or re.match(inculde_pattern, rel_name) is not None) \
                    and (exclude_pattern is None or re.match(exclude_pattern, rel_name) is None):
                files.append(FileDesc(rel_name, item_size))
    return files, False

class TargetDesc:
    def __init__(self, target_str, input_name = ''):
        self.target_str = target_str
        self.single_file = False
        split_target = target_str.split(':')
        if len(split_target) == 1:
            self.local = True
            self.path = target_str
        elif len(split_target) == 2:
            self.local = False
            self.site_name = split_target[0]
            self.path = split_target[1]
            self.url = GetSitePfnPath(self.site_name, self.path)
            self.xrd_url = 'root://xrootd-cms.infn.it/' + self.path
        else:
            raise RuntimeError('Invalid target string {0}'.format(target_str))

        if len(input_name) > 0:
            if self.local:
                self.full_path = os.path.join(self.path, input_name)
            else:
                self.full_url = self.url + '/' + input_name
                self.full_xrd_url = self.xrd_url + '/' + input_name
        else:
            if self.local:
                self.full_path = self.path
            else:
                self.full_url = self.url
                self.full_xrd_url = self.xrd_url
            self.input_name = os.path.basename(self.path)

        if self.local:
            self.abs_path = os.path.abspath(self.full_path)

    def GetFileSize(self, file_name):
        full_path = self.GetFullPath(file_name, False)
        if self.local:
            if os.path.isfile(full_path):
                return os.path.getsize(full_path)
            return -1

        out, err, returncode = Run('gfal-ls -l {0}'.format(full_path))
        if len(err) != 0:
            return -1
        item_lines = [ s.strip() for s in out.split('\n') if len(s.strip()) != 0 ]
        if len(item_lines) != 1:
            raise RuntimeError('Unable to get size for file "{0}".'.format(file_name))

        item = item_lines[0]
        prop = [ s for s in item.split(' ') if len(s) != 0 ]
        return int(prop[4])

    def DeleteFile(self, file_name, force):
        full_path = self.GetFullPath(file_name, False)
        if self.local:
            if force or os.path.isfile(full_path):
                os.remove(full_path)
        else:
            out, err, returncode = Run('gfal-rm {0}'.format(full_path))
            if force and len(err) != 0:
                raise RuntimeError('Unable to delete file "{0}".'.format(file_name))

    def GetFullPath(self, file_name, pfn = True, xrd=False):
        if self.local:
            local_path = os.path.join(self.abs_path, file_name) if not self.single_file else self.abs_path
            if pfn:
                local_path = 'file://' + local_path
            return local_path
        url = self.full_xrd_url if xrd else self.full_url
        return url + '/' + file_name if not self.single_file else url

def TransferFile(source_file, destination_file, number_of_threads):
    #if xrd:
    #    cmd = 'xrdcp --streams {0} "{1}" "{2}"'.format(number_of_threads, source_file, destination_file)
    #else:
    cmd = 'gfal-copy -p -n {0} "{1}" "{2}"'.format(number_of_threads, source_file, destination_file)
    if args.verbose:
        print('> {0}'.format(cmd))
    if args.dry_run:
        ok = True
    else:
        out, err, returncode = Run(cmd)
        ok = len(err) == 0 and returncode == 0
        if args.verbose:
            print(out)
        if not ok:
            print(err)
    return ok

def natural_sort_key(s, _nsre=re.compile('([0-9]+)')):
    return [int(text) if text.isdigit() else text.lower() for text in _nsre.split(s)]

input = TargetDesc(args.input[0])
files, input.single_file = CollectFiles(input, args.include, args.exclude)
files = sorted(files, key=lambda f: natural_sort_key(f.name))
input_prefix = '' if input.single_file else input.input_name
output = TargetDesc(args.output[0], input_prefix)
print('"{0}" -> "{1}"'.format(input.target_str, output.target_str))

total_size_MB = 0.
total_n_files = len(files)

for file in files:
    total_size_MB += file.size_MB

file_id = 0
try_id = 1
processed_MB = 0.
while file_id < len(files):
    file = files[file_id]
    display_name = os.path.join(input.input_name, file.name) if not input.single_file else file.name
    existing_file_size = output.GetFileSize(file.name)
    if existing_file_size == file.size or (args.dry_run and try_id > 1):
        print('OK {0}'.format(display_name))
        file_id += 1
        try_id = 1
        processed_MB += file.size_MB
        continue
    elif existing_file_size >= 0:
        output.DeleteFile(file.name, True)
    if try_id > args.max_tries:
        raise RuntimeError('Max number of tries is reached while transfering "{}".'.format(display_name))
    print('Progress: {0}/{1} ({2:.1f}%) files, {3:.1f}/{4:.1f} ({5:.1f}%) MiB.') \
            .format(file_id, total_n_files, float(file_id) / total_n_files * 100.,
                    processed_MB, total_size_MB, processed_MB / total_size_MB * 100.)
    print('Transfering "{0}" ({1:.1f} MiB, try {2})...'.format(display_name, file.size_MB, try_id))
    input_full_name = input.GetFullPath(file.name, xrd=args.xrootd)
    output_full_name = output.GetFullPath(file.name, xrd=args.xrootd)
    transfered = TransferFile(input_full_name, output_full_name, args.n_streams)
    if not transfered:
        output.DeleteFile(file.name, False)
    try_id += 1

print('"{0}" -> "{1}" successfully finished.'.format(input.target_str, output.target_str))
