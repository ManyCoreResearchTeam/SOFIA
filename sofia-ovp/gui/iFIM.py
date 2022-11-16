#!/usr/bin/python
#   Fault Injection Module Graphical Interface
#
#
#
#
#   Author: Felipe Rocha da Rosa

# Libraries
# Graphcis
import matplotlib

# Force matplotlib to not use any Xwindows backend.
matplotlib.use('TkAgg')

# Local classes
from options        import *
from faultcampaign  import *
from userinterface  import *
from explorationflow    import *


class noguiFaultInection():
    def __init__(self,options):
        self.options = options

    # Scroll terminal
    def terminalScroll(self,outputFI,logfile):
        with open(logfile,'w') as stdout:
            while True:
                output = outputFI.get()
                if not output:
                    break
                
                #Redirect the log to the terminal
                if self.options.verbose:
                    print(output.rstrip("\n"))
                stdout.write(output)

    def run(self):
        outputFI = Queue.Queue()
        FI = faultCampaign(options=self.options,terminal=outputFI)
        logfile="./workspace/logfile"

        threadFI = threading.Thread(target=FI.run,)
        threadFI.start()
        threadTerminal = threading.Thread(target=self.terminalScroll, args=(outputFI,logfile))
        threadTerminal.start()

# parse the options for the first time
(options, args) = parseArgs().parse()

if options.nogui:
    noguiFaultInection(options).run()
else:
    gui = explorationFlowOne(options)
    gui.createMainWindow()
