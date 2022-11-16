#   Fault Injection Module Graphical Interface
#
#
#
#
#   Author: Felipe Rocha da Rosa

# Libraries
import sys
import glob
import queue
import datetime
import atexit
import webbrowser

#GUI
try:
    # for Python2
    from tkinter import *   ## notice capitalized T in Tkinter
except ImportError:
    # for Python3
    from tkinter import *   ## notice lowercase 't' in tkinter here
from tkinter.ttk import *

import tkinter.messagebox

# Local classes
from classification     import *
from commondefines      import *
from graphics           import *
from options            import *
from faultcampaign      import *
from explorationflow    import *


class userinterface():
    # Constructor
    def __init__(self,options):
        self.options= options
        atexit.register(self.cleanup)

    def cleanup(self):
        print("exit")
    
    #
    # Main Window
    #

    # Create the initial window
    def createMainWindow(self):
        self.secondWindowFlag=False

        # Root object for the layout
        self.rootMainWindow=Tk()
        
        # Window title
        self.rootMainWindow.wm_title("Fault Injection Module")

        # Top Bar
        self.createFuncBar()

        self.currentFrame = Frame(master=self.rootMainWindow,width=400,height=500)
        self.currentFrame.pack(side=LEFT)

        # Title
        Label(self.currentFrame, text="Fault Injection \n Introduction \n\n\n\n\n", anchor=CENTER).pack()
        self.currentFrame.propagate(0) # Avoid lose the frame size

        # Configure
        Button(self.currentFrame, width=45, text="Parameters", command=lambda:self.secondWindow(window=0)).pack()

        # Fault Injection
        Button(self.currentFrame, width=45, text="Fault Campaign", command=lambda:self.secondWindow(window=1)).pack()
        
        # Disassembly Application
        Button(self.currentFrame, width=45, text="Disassembly", command=lambda:self.secondWindow(window=3)).pack()
        
        # Source code
        Button(self.currentFrame, width=45, text="Source Code", command=lambda:self.secondWindow(window=4)).pack()
        
        # Source code
        Button(self.currentFrame, width=45, text="Symbol finder", command=lambda:self.secondWindow(window=5)).pack()
        
        # Profile
        Button(self.currentFrame, width=45, text="Profile", command=lambda:self.secondWindow(window=6)).pack()
        
        # Replot reports
        Button(self.currentFrame, width=45, text="Replot", command=lambda:self.secondWindow(window=2)).pack()
        
        # Beta flow
        Button(self.currentFrame, width=45, text="Exploration flow", command=lambda:explorationFlowOne(self.options).createMainWindow()).pack()

        # Title
        Label(self.currentFrame, text="Copyright / License \n", anchor=CENTER).pack(side=BOTTOM)
        self.currentFrame.propagate(0) # Avoid lose the frame size
        
        # Main Loop
        self.rootMainWindow.mainloop()

    def createFuncBar(self):
        # Create menus top bar
        menuBar=Menu(self.rootMainWindow)
        self.rootMainWindow.config(menu=menuBar)

        # Create File sub menu
        fileMenu = Menu(menuBar)
        menuBar.add_cascade(label="File",menu=fileMenu)

        #Create functions
        fileMenu.add_command(label="Func 1")
        fileMenu.add_command(label="Func 2")
        fileMenu.add_command(label="Func 3")
        fileMenu.add_command(label="Func 4")
        fileMenu.add_separator()
        fileMenu.add_command(label="Exit",command=self.exitProg)

    def destroySecondWindow(self):
        self.secondFrame.pack_forget()
        self.secondFrame.destroy()
        self.secondWindowFlag=False
        
    def secondWindow(self,window):
        if self.secondWindowFlag:
            return #Already Created

        #Avoid window overlap
        self.secondWindowFlag=True

        # Create a frame containing the fault injection configuration
        self.secondFrame = Frame(master=self.rootMainWindow,width=600,height=500)
        self.secondFrame.pack(side="left", fill="both", expand=True)

        # Seleect window
        if   window == 0:
            self.createParametersWindow()
        elif window == 1:
            self.createExecutionWindow()
        elif window == 2:
            self.displayResultsWindow()
        elif window == 3:
            self.disassemblyAppliationWindow()
        elif window == 4:
            self.sourceAppliationWindow()
        elif window == 5:
            self.findSymbolsWindow()
        elif window == 6:
            self.profileFunctionUsageWindow()
        elif window == 7:
            self.lineCovarageWindow()

    #Exit program method
    def exitProg(self):
        print("Exit Button")
        sys.exit(0)

    #
    # Fault Injection Parameters
    #

    # Convert the values and begin the FI process
    def submitParameters(self,entries,possibleOptions):
        arguments= []
        for i in range(len(entries)):
            if not entries[i].get() == "False" and len(entries[i].get())!=0:
                arguments.append(possibleOptions[i].command)
                arguments.append(entries[i].get())
        
        # Parse the user commands
        (self.options, args) = parseArgs().parse(arguments)

        self.exitParametersWindow()

    # exit parameters window
    def exitParametersWindow(self):
        self.destroySecondWindow()

    # Configure a fault injection
    def createParametersWindow(self):
        possibleOptions = [ item for item in parseArgs().getOptionL() if item.module=="FI" ]

        entries = [] # List of Labels (same size of parameters)
        labels  = [] # List of Labels

        # Title
        Label(self.secondFrame, text="Fault Injection Configuration \n Insert the configuration \n").grid(row=0, column=0, columnspan=2)

        for i in range(len(possibleOptions)):
            options=possibleOptions[i]
            labels.append(Label(self.secondFrame, text=options.name))
            labels[-1].config(width=15,justify=LEFT)
            labels[-1].grid(row=1+i, column=0)

            if options.buttom == "tick":
                entries.append(StringVar())
                entries[-1].set("False")
                options.buttomobj = Checkbutton(self.secondFrame, variable=entries[-1], offvalue="False", onvalue="True")
                options.buttomobj.config(width=15,justify=CENTER)
                
                # Pack in the frame
                options.buttomobj.grid(row=1+i, column=1, sticky=E)
            elif options.buttom == "choice":
                entries.append(StringVar())
                entries[-1].set(options.default)

                options.buttomobj = OptionMenu(self.secondFrame, entries[-1], *options.choices)
                options.buttomobj.config(width=15,justify=CENTER)
                # Pack in the frame
                options.buttomobj.grid(row=1+i, column=1, sticky=E)
            elif options.buttom == "choice2":
                entries.append(StringVar())
                entries[-1].set(options.default)

                options.buttomobj = Entry(self.secondFrame, width=15, textvariable=entries[-1])
                options.buttomobj.config(width=15,justify=CENTER)
                # Pack in the frame
                options.buttomobj.grid(row=1+i, column=1, sticky=E)
            else:
                entries.append(StringVar())
                entries[-1].set(options.default)

                options.buttomobj = Entry(self.secondFrame, width=15, textvariable=entries[-1])
                options.buttomobj.config(width=15,justify=CENTER)
                # Pack in the frame
                options.buttomobj.grid(row=1+i, column=1, sticky=E)

        # Entry bottom
        Button(self.secondFrame, text="Enter", width=15, command=lambda:self.submitParameters(entries,possibleOptions)).grid(columnspan=2)
        Button(self.secondFrame, text="Exit",  width=15, command=lambda:self.exitParametersWindow()).grid(columnspan=2)
        self.secondFrame.grid_propagate(0)

    #
    # Fault Injection Campaign
    #

    # Window show the fault injection
    def createExecutionWindow(self):
        Label(self.secondFrame, anchor=CENTER, text="Fault Injection Process \n").pack(side="top", fill="both", expand=True)
        self.finishsimulation = False

        # Kill the Fault Injection
        buttonsFrame = Frame(master=self.secondFrame,width=600,height=500)
        buttonsFrame.pack(side="top", fill="both", expand=True)
        
        Button(buttonsFrame, width=25, command=lambda:self.startFI(),               text="Start"         ).pack(side="left", expand=False)
        Button(buttonsFrame, width=25, command=lambda:self.stopFI(),                text="Kill Process"  ).pack(side="left", expand=False)
        Button(buttonsFrame, width=25, command=lambda:self.exitParametersWindow(),  text="Exit"          ).pack(side="left", expand=False)

        #Terminal Window
        self.terminal=Text(self.secondFrame,width=500,height=400,bg="white")
        self.terminal.pack(side="top", fill="both", expand=True)
    
    # Start fault injection process
    def startFI(self):
        # FI OBJS
        outputFI = queue.Queue()
        FI = faultCampaign(options=self.options,terminal=outputFI)

        self.threadFI = threading.Thread(target=FI.run,)
        self.threadFI.start()
        self.threadterminal = threading.Thread(target=self.terminalScroll, args=(outputFI,self.terminal))
        self.threadterminal.start()
        
    # Interrupt fault injection process
    def stopFI(self):
        # FI OBJS
        outputFI = queue.Queue()
        outputFI.put(None)

    # Scroll terminal
    def terminalScroll(self,outputFI,terminal):
        while True:
            output = outputFI.get()
            if not output:
                self.finishsimulation = True
                break
            terminal.insert(END,output)
            terminal.see(END)

    #
    # Symbol-related (Under Construction)
    #
    def disassemblyAppliationWindow(self):
        Label(self.secondFrame, anchor=CENTER, text="Disassembly Application Window\n").pack(side="top", fill="both", expand=True)

        # Kill the Fault Injection
        Button(self.secondFrame, width=45, command=lambda:self.exitParametersWindow(),  text="Exit").pack(side="top", fill="both", expand=True)

        #Terminal Window
        self.terminal=Text(self.secondFrame,width=500,height=400,bg="white")
        
        self.terminal.pack(side="top", fill="both", expand=True)
        
        #~ columnconfigure(self, columns, weight=1)
        
        output = subprocess.check_output("arm-linux-gnueabi-objdump ./workspace/backprop/app.ARM_CORTEX_A9.elf --section=.text -d",shell=True,)
        i=0
        for line in output.splitlines():
            i+=1
            if i==1000:
                break
            
            text="{0:05} {1} \n".format(i,line)
            self.terminal.insert(END,text)
            
        self.secondFrame.grid_propagate(0)
    
    def lineTrigger(self,line):
        print(line)

    def sourceAppliationWindow(self):
        Label(self.secondFrame, anchor=CENTER, text="Application source code Window\n Importing *.c *.cpp *.f *.h").pack(side="top", expand=False)
        # Kill the Fault Injection
        Button(self.secondFrame, width=45, command=lambda:self.exitParametersWindow(),  text="Exit").pack(side="top", expand=False)
        
        folder="./workspace/backprop2"
        folder="./workspace/backprop"
        
        # Serach the files through the extension
        files = []
        files.extend(glob.glob("{0}/*.c".format(folder)))   #Hardcoded
        files.extend(glob.glob("{0}/*.cpp".format(folder))) #Hardcoded
        files.extend(glob.glob("{0}/*.h".format(folder)))   #Hardcoded
        files.extend(glob.glob("{0}/*.f".format(folder)))   #Hardcoded
        
        if len(files) == 0:
            Label(self.secondFrame, anchor="e", text="No report found\n").pack()
            return

        tabs = Notebook(self.secondFrame)
        
        for file in files:
            with open(file, 'r') as sourcefile:
                data=sourcefile.read().replace("\r","")

            frame  = Frame(tabs,width=500,height=400,bg="white")
            source = Text(frame,width=500,height=400,bg="white")
            source.pack(side="top", fill="both", expand=True)
            source.insert(END,data)
            tabs.add(frame, text=file.split("/")[-1])
        
        tabs.pack(side="top", fill="both", expand=True)

        self.secondFrame.propagate(0)

    def findSymbolsWindow(self):
        Label(self.secondFrame, anchor=CENTER, text="Application source code Window\n Importing *.c *.cpp *.f *.h").pack(side="top", expand=False)
        # Kill the Fault Injection
        buttonsFrame = Frame(master=self.secondFrame,width=600,height=500)
        buttonsFrame.pack(side="top", fill="both", expand=True)
        
        Button(buttonsFrame, width=15, command=lambda:self.exitParametersWindow(),text="Exit").pack(side="left", expand=False)
        Button(buttonsFrame, width=15, command=lambda:self.searchSymbol(symbollist=symbollist,symbolentry=symbolentry,tabs=tabs),text="Search").pack(side="left", expand=False)
        Button(buttonsFrame, width=15, command=lambda:self.symbolFaultInjection(symbollist=symbollist,symbolentry=symbolentry,tabs=tabs),text="FI").pack(side="left", expand=False)
        
        
        symbolentry = Entry(buttonsFrame,  width=15, textvariable="Insert Symbol")
        symbolentry.pack(side="left", fill="both", expand=True)

        folder="./workspace/backprop"
        objectfile="app.ARM_CORTEX_A9.elf"
        
        # Serach the files through the extension
        files = []
        files.extend(glob.glob("{0}/*.c".format(folder)))   #Hardcoded
        files.extend(glob.glob("{0}/*.cpp".format(folder))) #Hardcoded
        files.extend(glob.glob("{0}/*.h".format(folder)))   #Hardcoded
        files.extend(glob.glob("{0}/*.f".format(folder)))   #Hardcoded
        
        if len(files) == 0:
            Label(self.secondFrame, anchor="e", text="No report found\n").pack()
            return

        tabs = Notebook(self.secondFrame)

        # Symbol list
        # Get the symbol list
        symbollist = subprocess.check_output("arm-linux-gnueabi-objdump ./workspace/backprop/app.ARM_CORTEX_A9.elf --section=.text --syms",shell=True,)
        
        # Create Tab
        frame  = Frame(tabs,width=500,height=400,bg="white")
        source = Text(frame,width=500,height=400,bg="white")
        source.pack(side="top", fill="both", expand=True)
        source.insert(END,"Symbol List:\n")
        source.insert(END,symbollist)
        tabs.add(frame, text="Symbol list")

        # Source code
        for file in files:
            with open(file, 'r') as sourcefile:
                data=sourcefile.read().replace("\r","")

            frame  = Frame(tabs,width=500,height=400,bg="white")
            source = Text(frame,width=500,height=400,bg="white")
            source.pack(side="top", fill="both", expand=True)
            source.insert(END,data)
            tabs.add(frame, text=file.split("/")[-1])
        
        tabs.pack(side="top", fill="both", expand=True)

        self.secondFrame.propagate(0)

    def searchSymbol(self,symbolentry,symbollist,tabs):
        symbolusage="""
        COLUMN ONE: the symbol's value
        COLUMN TWO: a set of characters and spaces indicating the flag bits that are set on the symbol. There are seven groupings which are listed below:
        group one: (l,g,,!) local, global, neither, both.
        group two: (w,) weak or strong symbol.
        group three: (C,) symbol denotes a constructor or an ordinary symbol.
        group four: (W,) symbol is warning or normal symbol.
        group five: (I,) indirect reference to another symbol or normal symbol.
        group six: (d,D,) debugging symbol, dynamic symbol or normal symbol.
        group seven: (F,f,O,) symbol is the name of function, file, object or normal symbol.
        COLUMN THREE: the section in which the symbol lives, ABS means not associated with a certain section
        COLUMN FOUR: the symbol's size or alignment.
        COLUMN FIVE: the symbol's name."""

        symbolname  = symbolentry.get()

        if len(symbolname) == 0: # No symbol found
            tkinter.messagebox.showwarning("Symbol Search","Symbol Invalid!\n(Empty field)" )
            return

        symbol = [ sym for sym in symbollist.split("\n") if sym.find(symbolname) !=-1 ]
        
        if len(symbol) == 0: # No symbol found
            tkinter.messagebox.showwarning("Symbol Search","Symbol not found!\n({0})".format(symbolname) )
            return

        if len(symbol) > 1: # Too many symbols
            if tkinter.messagebox.askquestion("Symbol Search","The symbol is not unique! \n Found matches: {0})".format(len(symbol))) == "NO":
                return

        # Create Tab
        frame  = Frame(tabs,width=500,height=400,bg="white")
        source = Text(frame,width=500,height=400,bg="white")
        source.pack(side="top", fill="both", expand=True)
        source.insert(END,"Disassembling symbols:\n\n {0}".format(symbolusage))
        tabs.insert(0,frame, text="Disassembly Symbols",)
        tabs.select(0)

        for sym in symbol:
            lowaddress  = int(sym.split()[0],16)
            size        = int(sym.split(".text")[1].split()[0],16)
            disa = subprocess.check_output("arm-linux-gnueabi-objdump ./workspace/backprop/app.ARM_CORTEX_A9.elf -d --section=.text --start-address={0} --stop-address={1}".format(lowaddress,lowaddress+size),shell=True,)

            source.insert(END,"\n\n\nSymbols Information:\n {0}".format(sym))
            source.insert(END,disa)

    # Fault Injection symbol based
    def symbolFaultInjection(self,symbolentry,symbollist,tabs):
        symbolname  = symbolentry.get()

        if len(symbolname) == 0:    # No symbol found
            tkinter.messagebox.showwarning("Symbol Search","Symbol Invalid!\n(Empty field)" )
            return

        symbol = [ sym for sym in symbollist.split("\n") if sym.find(symbolname) !=-1 ]
        
        if len(symbol) == 0:        # No symbol found
            tkinter.messagebox.showwarning("Symbol Search","Symbol not found!\n({0})".format(symbolname) )
            return

        if len(symbol) > 1:         # Too many symbols
            if tkinter.messagebox.askquestion("Symbol Search","The symbol is not unique! \n Found matches: {0})".format(len(symbol))) == "yes":
                return

        # Create Tab
        frame    = Frame(tabs,width=500,height=400,bg="white")
        terminal = Text(frame,width=500,height=400,bg="white")
        terminal.pack(side="top", fill="both", expand=True)
        terminal.insert(END,"Executuion:\n\n")
        tabs.insert(0,frame, text="Terminal",)
        tabs.select(0)

        # Communication queue
        outputqueue = queue.Queue()
        symbolFI    = faultCampaign(options=self.options,terminal=outputqueue)
        
        self.threadsymbol = threading.Thread(target=symbolFI.symbolTraceRun, args=("bpnn_adjust_weights._omp_fn.1",))
        
        self.threadsymbol.start()
        self.threadterminal = threading.Thread(target=self.terminalScroll, args=(outputqueue,terminal))
        self.threadterminal.start()

    #
    # Profiling the application
    #
    def profileFunctionUsageWindow(self):
        
        Label(self.secondFrame, anchor=CENTER, text="Enable the profile of the application after the boot just before the application starts \n").pack(side="top", fill="both", expand=True)
        self.finishsimulation = False

        # Kill the Fault Injection
        buttonsFrame = Frame(master=self.secondFrame,width=600,height=500)
        buttonsFrame.pack(side="top", fill="both", expand=True)
        
        outputfile="./workspace/backprop/app.html"
        
        Button(buttonsFrame, width=15, command=lambda:self.exitParametersWindow(),                  text="Exit"                     ).pack(side="left", expand=False)
        Button(buttonsFrame, width=15, command=lambda:self.startProfile("enablefunctionprofile"),   text="Start Function Profile"   ).pack(side="left", expand=False)
        Button(buttonsFrame, width=15, command=lambda:self.startProfile("enablelinecoverage"),      text="Start Line Covarage"      ).pack(side="left", expand=False)
        Button(buttonsFrame, width=15, command=lambda:self.startProfile("enableitrace"),            text="Start Instruction Trace"  ).pack(side="left", expand=False)
        Button(buttonsFrame, width=15, command=lambda:self.showProfiling(outputfile),               text="Show Results"             ).pack(side="left", expand=False)

        #Terminal Window
        self.terminal=Text(self.secondFrame,width=500,height=400,bg="white")
        self.terminal.pack(side="top", fill="both", expand=True)
        
    # Start profile function
    def startProfile(self,type):
        # Communication queue
        outputqueue = queue.Queue()
        profiler = faultCampaign(options=self.options,terminal=outputqueue)
        
        self.threadprofiler = threading.Thread(target=profiler.profileFunctionUsage, args=(type,))
        
        self.threadprofiler.start()
        self.threadterminal = threading.Thread(target=self.terminalScroll, args=(outputqueue,self.terminal))
        self.threadterminal.start()

    # Display html file
    def showProfiling(self,file):
        if self.finishsimulation:
            webbrowser.open(file)
    
    #
    # Replot the report
    #

    # Display results
    def displayResultsWindow(self):

        Label(self.secondFrame, anchor=CENTER, text="Fault Injection Report \n").grid(row=0, column=0,sticky=N+S+E+W)
        Label(self.secondFrame, anchor="e", text="Last Modification\n").grid(row=0, column=1,sticky=N+S+E+W)
        self.secondFrame.grid_propagate(0)
        
        files  = glob.glob("./workspace/*reportfile") #Hardcoded
        if len(files) == 0:
            Label(self.secondFrame, anchor="e", text="No report found\n").grid(row=0, column=0, columnspan=2,sticky=N+S+E+W)
        else:
            i=0
            for filename in  files:
                i+=1
                timestap=datetime.datetime.fromtimestamp(os.path.getmtime(filename)).strftime('%Y-%m-%d %H:%M:%S')
                grap = graphics(replotfile=filename, environment=self.options.environment)
                
                Button(self.secondFrame, width=45, command=grap.groupsdac, text=filename ).grid(row=1,column=0,sticky=N+S+E+W)
                Label(self.secondFrame, anchor="e",text=timestap).grid(row=i,column=1,sticky=N+S+E+W)
                
            self.secondFrame.grid_propagate(0)
        
        
        Button(self.secondFrame, text="Exit",  width=15, command=lambda:self.exitParametersWindow()).grid(columnspan=2)
        self.secondFrame.grid_propagate(0)
