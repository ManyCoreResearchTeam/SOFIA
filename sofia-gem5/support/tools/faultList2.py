#!/usr/bin/python

import sys
from os import getcwd
from os.path import join as joinpath

sys.path.append('../../gui')

# Local classes
from options            import *
from faultlist          import *

####################################################################################################
# Options                                                                                          #
####################################################################################################
# parse the options for the first time
(options, args) = parseArgs().parse()

####################################################################################################
# Call Generator                                                                                   #
####################################################################################################

# fault generator 
faultgenerator = faultGenerator(options=options)

# create fault list
faultlist = faultgenerator.createfaultlist()


