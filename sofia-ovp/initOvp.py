#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from argparse import ArgumentParser
import os
import subprocess

def main():

    parser = ArgumentParser()

    parser.add_argument(
        '-l',
        '--license',
        action='store',
        dest='license',
        required=True,
        help="license origin")

    parser.add_argument(
        '-v',
        '--version',
        action='store',
        dest='version',
        required=True,
        help="imperas version")

    args = parser.parse_args()

    with open('ovp.sh', 'w+') as ovpFile:
        ovpFile.write("#!/bin/bash\n")
        ovpFile.write("export IMPERASD_LICENSE_FILE=2700@" + args.license + "\n")
        ovpFile.write(
            "source /soft64/ovp/Imperas." +
            args.version +
            "/bin/setup.sh\n")
        ovpFile.write("setupImperas /soft64/ovp/Imperas." + args.version + "\n")
        ovpFile.write("eval \"$@\"\n")

    subprocess.call(['chmod', '+x', 'ovp.sh'])

if __name__ == "__main__":
    main()
