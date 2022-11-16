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
from classification import *
from commondefines  import *
from graphics       import *
from options        import *
from faultcampaign  import *

class explorationFlowOne():
    # Constructor
    def __init__(self,options):
        self.options        = options
        atexit.register(self.cleanup)
        
        # Acquire the current folder and derive some default paths
        self.dirproject=subprocess.check_output("pwd",shell=True,).rstrip('\n')
        self.dirworkspace=self.dirproject+"/workspace"
        self.dirworkload =self.dirproject+"/workloads"
        self.dirsupport  =self.dirproject+"/support"

        # Create workspace dir
        if not os.path.exists(self.dirworkspace):
            os.makedirs(self.dirworkspace)

    def cleanup(self):
        print("Exit explorationFlowOne cleanup")

    # Exit program method
    def exitProg(self):
        print("Exit explorationFlowOne Button")
        sys.exit(0)

    #
    # Graphical mis.
    #
    # Create a new tab with scrolbar
    def createTabWithScroll(self,name):
        frame  = Frame(self.tabs,width=500,height=400,bg="white")
        scrollbar = Scrollbar(frame)
        scrollbar.pack(side=RIGHT, fill=Y)

        # Add buttons
        buttonsFrame = Frame(master=frame,width=500,height=25)
        buttonsFrame.pack(side="top", fill="both", expand=True)
        Button(buttonsFrame, width=15, command=lambda:frame.destroy(),text="Exit").pack(side="left", expand=False)

        # Introduce a search mechanism? 
        #~ symbolentry = Entry(buttonsFrame,  width=15, textvariable="Insert Symbol")
        #~ symbolentry.pack(side="left", fill="both", expand=True)
        
        text = Text(frame,width=500,height=400,bg="white",yscrollcommand=scrollbar.set)
        text.pack(side="top", fill="both", expand=True)
        self.tabs.insert(0,frame, text=name,)
        scrollbar.config(command=text.yview)
        self.tabs.select(0)
        return text,buttonsFrame

    # Scroll terminal
    def terminalScroll(self,outputQueue,terminal):
        while True:
            output = outputQueue.get()
            if not output:
                self.finishsimulation = True
                break
            terminal.insert(END,output)
            terminal.see(END)

    #
    # Main Window
    #
    # Create the initial window
    def createMainWindow(self):
        #Root object for the layout
        self.rootMainWindow=Tk()

        # Window title
        self.rootMainWindow.wm_title("Reliability Exploration Flow")

        # Top Bar
        self.createFuncBar()

        self.currentFrame = Frame(master=self.rootMainWindow,width=1000,height=500)
        self.currentFrame.pack(fill="both", expand=True)

        Label(self.currentFrame, anchor=CENTER, text="Reliability Exploration Flow First Window \n").pack(side="top", expand=False)

        # Notebook container
        self.tabs = Notebook(self.currentFrame)

        #~ # Create first tab with usage/help/message
        frame  = Frame(self.tabs,width=500,height=400,bg="white")
        source = Text(frame,width=500,height=400,bg="white")
        source.pack(side="top", fill="both", expand=True)
        source.insert(END,fimMessages().msgintflow)
        self.tabs.add(frame, text="Introduction")
        self.tabs.pack(side="top", fill="both", expand=True)

        # Avoid lose the frame size
        self.currentFrame.propagate(0)

        # Main Loop
        self.rootMainWindow.mainloop()

    # Crete the top bar containing the features
    def createFuncBar(self):
        # Create menus top bar
        menuBar=Menu(self.rootMainWindow)
        self.rootMainWindow.config(menu=menuBar)

        # Create File sub menu
        dropmenu = Menu(menuBar)
        menuBar.add_cascade(label="File",menu=dropmenu)

        # create sub functions
        dropmenu.add_command(label="Func 1")
        dropmenu.add_command(label="Func 2")
        dropmenu.add_command(label="Func 3")
        dropmenu.add_command(label="Func 4")
        dropmenu.add_separator()
        dropmenu.add_command(label="Exit",command=self.exitProg)

        # Configuration menu
        configMenu = Menu(menuBar)
        menuBar.add_cascade(label="Edit",menu=configMenu)

        # create sub functions
        configMenu.add_command(label="Select Application"   ,command=self.selectApplicationWindow)
        configMenu.add_command(label="  Source Code"        ,command=self.appliationSourceCode)
        configMenu.add_command(label="  Disassembly"        ,command=self.appliationDisassembly)
        dropmenu.add_separator()
        configMenu.add_command(label="Compile"              ,command=self.appliationCompile)
        configMenu.add_command(label="Execute"              ,command=self.appliationExecute)

        # Profile menu
        dropmenu = Menu(menuBar)
        menuBar.add_cascade(label="Profile",menu=dropmenu)

        # create sub functions
        dropmenu.add_command(label="Function Profile"    ,command=lambda:self.profileFunctions("enablefunctionprofile"))
        dropmenu.add_command(label="Line Coverage"       ,command=lambda:self.profileFunctions("enablelinecoverage"))
        dropmenu.add_command(label="Instruction Trace"   ,command=lambda:self.profileFunctions("enableitrace"))

        # Fault Injection menu
        dropmenu = Menu(menuBar)
        menuBar.add_cascade(label="Fault Campaign",menu=dropmenu)

        # Fault Injection sub functions
        dropmenu.add_command(label="Fault Injection"        ,command=lambda:self.faultInjectionStater("regular"))
        dropmenu.add_separator()
        dropmenu.add_command(label="Function Symbol"        ,command=self.findSymbolsWindow)
        dropmenu.add_separator()
        dropmenu.add_command(label="Variable Symbol"        ,command=self.findVariableWindow)

    #
    # Configuration
    #
    # Acquire from the user one application (workload required)
    def selectApplicationWindow(self):

        #Root object for the layout
        window=Tk()

        # Window title
        window.wm_title("Select Application")

        frame = Frame(master=window)
        frame.pack(side=LEFT)

        Label(frame, anchor=CENTER, text="First select one workload\nThe application selection will be automatically updated").pack(side="top", fill="both", expand=True)

        # workload choices
        Label(frame, anchor=CENTER, text="Workload").pack(side="top", fill="both", expand=True)
        workloadOptions=[ item for item in parseArgs().getOptionL() if item.dest=="workload" ][0]
        workloadOptions.entry=StringVar()
        workloadOptions.entry.set(workloadOptions.default)
        workloadOptions.buttomobj = OptionMenu(frame, workloadOptions.entry, *workloadOptions.choices, command=lambda x:self.getWorkload(workloadOptions,appOptions))
        workloadOptions.buttomobj.config(width=15,justify=CENTER)
        workloadOptions.buttomobj.pack(side=TOP)

        # apps choices
        Label(frame, anchor=CENTER, text="Application").pack(side="top", fill="both", expand=True)
        appOptions=[ item for item in parseArgs().getOptionL() if item.dest=="apps" ][0]
        appOptions.entry=StringVar()
        appOptions.entry.set(appOptions.default)
        appOptions.buttomobj = OptionMenu(frame, appOptions.entry, [])
        appOptions.buttomobj.config(width=15,justify=CENTER)
        appOptions.buttomobj.pack(side=TOP)

        # Exit
        Button(frame, text="Accept",width=15, command=lambda:self.getApp(appOptions,window)).pack(side=TOP)
        Button(frame, text="Exit",  width=15, command=lambda:window.destroy()).pack(side=TOP)

        # main Loop
        window.mainloop()

    # Get the workload and update the app selector
    def getWorkload(self,workloadOptions,appOptions):
        self.options.workload=workloadOptions.entry.get()
        applist = os.listdir("./workloads/"+self.options.workload)
        applist.sort()

        appOptions.buttomobj['menu'].delete(0, 'end')
        for app in applist:
            appOptions.buttomobj['menu'].add_command(label=app, command=lambda value=app: appOptions.entry.set(value))

    # Get the app name
    def getApp(self,appOptions,rootMainWindow):
        self.options.apps=appOptions.entry.get()
        if len(self.options.apps)==0:
            tkinter.messagebox.askquestion("Selected Application","Any application selected \n Accept?")
            rootMainWindow.destroy()
        elif tkinter.messagebox.askquestion("Selected Application","The selected application: \n {0} \n Accept?".format(self.options.apps)) == "yes":
            rootMainWindow.destroy()

    # Display the application source code
    def appliationSourceCode(self):
        # No application selected
        if (len(self.options.apps)==0):
            tkinter.messagebox.showerror("Error","Any application selected\n Please Select one")
            return

        # Application folder
        folder="workloads/{0}/{1}".format(self.options.workload,self.options.apps)

        # Serach the files through the extension
        files = []
        files.extend(glob.glob("{0}/*.c".format(folder)))   #Hardcoded
        files.extend(glob.glob("{0}/*.cpp".format(folder))) #Hardcoded
        files.extend(glob.glob("{0}/*.h".format(folder)))   #Hardcoded
        files.extend(glob.glob("{0}/*.f".format(folder)))   #Hardcoded

        if len(files) == 0:
            tkinter.messagebox.showerror("Error"," No source code found (*.c *.cpp *.f *.h)")
            return

        # Show the source code
        for file in files:
            with open(file, 'r') as sourcefile:
                data=sourcefile.read().replace("\r","")
                
            (text, buttonsFrame)=self.createTabWithScroll(name=file.split("/")[-1])
            text.insert(END,data)

    # Display the application object code disassembly
    def appliationDisassembly(self):
        # No application selected
        if (len(self.options.apps)==0):
            tkinter.messagebox.showerror("Error","Any application selected\n Please Select one")
            return

        files=[file for file in glob.glob("./workspace/{0}/*elf".format(self.options.apps)) if "OVPFIM" not in file if "smpboot" not in file ]

        # No application selected
        if (len(files)==0):
            tkinter.messagebox.showerror("Error","No object code found\nPlease compile the application first")
            return

        # Get the disassembly
        output = subprocess.check_output("arm-linux-gnueabi-objdump {0} --section=.text -d".format(files[0]) ,shell=True,)

        # Add a new tab
        (text, buttonsFrame)=self.createTabWithScroll(name="Dis. {0}".format(self.options.apps))
        text.insert(END,output)

    # Compile the application
    def appliationCompile(self):
        # No application selected
        if (len(self.options.apps)==0):
            tkinter.messagebox.showerror("Error","Any application selected\n Please Select one")
            return

        # Create a new tab window
        (text, buttonsFrame)=self.createTabWithScroll(name="compile: {0}".format(self.options.apps))
        
        # communication queue
        output = queue.Queue()
        FI = faultCampaign(options=self.options,terminal=output)
        
        # FI thread
        command="compile"
        profile=""
        self.threadFI = threading.Thread(target=FI.profile, args=(command,profile))
        self.threadFI.start()
        
        # communication thread
        threading.Thread(target=self.terminalScroll, args=(output,text)).start()

    # Execute the application without faults (faultless)
    def appliationExecute(self):
        # No application selected
        if (len(self.options.apps)==0):
            tkinter.messagebox.showerror("Error","Any application selected\n Please Select one")
            return

        # Create a new tab window
        (text, buttonsFrame)=self.createTabWithScroll(name="Compile: {0}".format(self.options.apps))
        
        # communication queue
        output = queue.Queue()
        FI = faultCampaign(options=self.options,terminal=output)
        
        # FI thread
        command="execute"
        profile=""
        self.threadFI = threading.Thread(target=FI.profile, args=(command,profile))
        self.threadFI.start()
        
        # communication thread
        threading.Thread(target=self.terminalScroll, args=(output,text)).start()

    #
    # Function symbol search
    #
    # Create the search window
    def findSymbolsWindow(self):
        # No application selected
        if (len(self.options.apps)==0):
            tkinter.messagebox.showerror("Error","Any application selected\n Please Select one")
            return

        # Create a new tab window
        (text, buttonsFrame)=self.createTabWithScroll(name="Search Function: {0}".format(self.options.apps))
        
        # Add buttons
        Button(buttonsFrame, width=15, command=lambda:self.searchSymbol(symbollist=symbollist,symbolentry=symbolentry,objfile=files[0]),text="Search").pack(side="left", expand=False)
        Button(buttonsFrame, width=15, command=lambda:self.symbolFaultInjection(symbollist=symbollist,symbolentry=symbolentry,faulttype=faultTypesE.functiontrace2.name),text="FI").pack(side="left", expand=False)
        symbolentry = Entry(buttonsFrame,  width=15, textvariable="Insert Symbol")
        symbolentry.pack(side="left", fill="both", expand=True)
        
        files=[file for file in glob.glob("./workspace/{0}/*elf".format(self.options.apps)) if "OVPFIM" not in file if "smpboot" not in file ]

        # Get the symbol list
        symbollist = subprocess.check_output("arm-linux-gnueabi-objdump {0} --section=.text --syms".format(files[0]),shell=True,)

        text.insert(END,"Symbol List: Application {0} \n".format(self.options.apps))
        text.insert(END,fimMessages().symbolusage)
        text.insert(END,symbollist)

    # Search for symbols name and disassemble it
    def searchSymbol(self,symbolentry,symbollist,objfile):
        symbolname  = symbolentry.get()
        # Invalid symbol name
        if len(symbolname) == 0:
            tkinter.messagebox.showwarning("Symbol Search","Symbol Invalid!\n(Empty field)" )
            return

        # Search in the symbol list
        symbol = [ sym for sym in symbollist.split("\n") if sym.find(symbolname) !=-1 ]
        
        # No symbol found
        if len(symbol) == 0: 
            tkinter.messagebox.showwarning("Symbol Search","Symbol not found!\n({0})".format(symbolname) )
            return

        # More than one symbol
        if len(symbol) > 1: 
            if tkinter.messagebox.askquestion("Symbol Search","The symbol is not unique! \n Found matches: {0})".format(len(symbol))) == "NO":
                return

        # Create a new tab window
        (text, buttonsFrame)=self.createTabWithScroll(name="Search: {0}".format(symbolname))

        for sym in symbol:
            lowaddress  = int(sym.split()[0],16)
            size        = int(sym.split(".text")[1].split()[0],16)
            disa = subprocess.check_output("arm-linux-gnueabi-objdump {0} -d --section=.text --start-address={1} --stop-address={2}".format(objfile,lowaddress,lowaddress+size),shell=True,)
            text.insert(END,"\n\n\n Symbols Information:\n {0}".format(sym))
            text.insert(END,disa)

    # Check if the symbol exist and call the FI starter
    def symbolFaultInjection(self,symbolentry,symbollist,faulttype):
        symbolname = symbolentry.get()
        # Invalid symbol name
        if len(symbolname) == 0:
            tkinter.messagebox.showerror("Symbol Search","Symbol Invalid!\n(Empty field)" )
            return

        # Search in the symbol list
        symbol = [ sym for sym in symbollist.split("\n") if sym.find(symbolname) !=-1 ]
        
        # No symbol found
        if len(symbol) == 0: 
            tkinter.messagebox.showerror("Symbol Search","Symbol not found!\n({0})".format(symbolname) )
            return

        # More than one symbol
        if len(symbol) > 1: 
            #Search for exec match
            if [ True for sym in symbol if sym.split()[-1]==symbolname]:
                symbol = [ sym for sym in symbol if sym.split()[-1]==symbolname]
                if len(symbol) > 1:
                    tkinter.messagebox.showerror("Symbol Search","The symbol is not unique! \n Found matches: {0})".format(len(symbol)))
                    return
            else:
                tkinter.messagebox.showerror("Symbol Search","The symbol is not unique! \n Found matches: {0})".format(len(symbol)))
                return

        self.options.tracesymbol = symbolname
        self.faultInjectionStater(command="symbol",faulttype=faulttype)

    #
    # Variable symbol search
    #
    # Find variable
    def findVariableWindow(self):
        # No application selected
        if (len(self.options.apps)==0):
            tkinter.messagebox.showerror("Error","Any application selected\n Please Select one")
            return

        # Create a new tab window
        (text, buttonsFrame)=self.createTabWithScroll(name="Search Variable: {0}".format(self.options.apps))
        
        # Add buttons
        Button(buttonsFrame, width=15, command=lambda:self.symbolFaultInjection(symbollist=symbollist,symbolentry=symbolentry,faulttype=faultTypesE.variable2.name),text="FI").pack(side="left", expand=False)
        symbolentry = Entry(buttonsFrame,  width=15, textvariable="Insert Symbol")
        symbolentry.pack(side="left", fill="both", expand=True)
        
        files=[file for file in glob.glob("./workspace/{0}/*elf".format(self.options.apps)) if "OVPFIM" not in file if "smpboot" not in file ]

        # Get the symbol list
        symbollist = subprocess.check_output("arm-linux-gnueabi-readelf {0} --symbols | grep OBJECT | grep GLOBAL".format(files[0]),shell=True,)

        text.insert(END,"Symbol List: Application {0} \n".format(self.options.apps))
        text.insert(END,fimMessages().readelfusage)
        text.insert(END,symbollist)

    #
    # Profile
    #
    # Application profiles
    def profileFunctions(self,profilefunction):
        # No application selected
        if (len(self.options.apps)==0):
            tkinter.messagebox.showerror("Error","Any application selected\n Please Select one")
            return

        # Create a new tab window
        (text, buttonsFrame)=self.createTabWithScroll(name="Profile. {0}".format(self.options.apps))
        self.finishsimulation = False

        self.dirapplicationsource="workloads/{0}".format(self.options.workload)

        # Acquire the application
        self.currentapplication = self.options.apps

        outputfile="./workspace/{0}/app.html".format(self.currentapplication)
        text.insert(END,"Starting the application profile: {0} \n".format(self.currentapplication))

        command="symbol"

        # Communication queue
        outputqueue = queue.Queue()
        profiler = faultCampaign(options=self.options,terminal=outputqueue)

        # Profile thread
        thread = threading.Thread(target=profiler.profile, args=(command,profilefunction,))
        thread.start()

        # Output thread
        threading.Thread(target=self.terminalScroll, args=(outputqueue,text)).start()

        # Show results thread
        threading.Thread(target=self.showProfiling, args=(outputfile,thread)).start()

    # Display HTML file
    def showProfiling(self,file,threadprofiler):
        if threadprofiler!=None:
            threadprofiler.join()
        if tkinter.messagebox.askquestion("Results"," Display the results? \n(requires a browser)") == "yes":
            webbrowser.open(file)

    #
    # Fault Injection
    #
    # Fault Injection Starter
    def faultInjectionStater(self,command,faulttype=faultTypesE.register.name):
        # Default list
        options = [ item for item in parseArgs().getOptionL() if item.module=="FI" ]
        
        # define fault type
        self.options.faulttype = faulttype
        
        # No application selected
        if (len(self.options.apps)==0):
            tkinter.messagebox.showerror("Error","Any application selected\n Please Select one")
            return

        # Choose options according with the fault injection type
        if   command == "regular":
            regularopt=["--environment","--architecture","--numcores","--numfaults", "--dummy", "--eqdist","--parallel"]
            options = [ item for item in options if [ True for opt in regularopt if opt==item.command] ]
        elif command == "symbol":
            regularopt=["--environment","--architecture","--numcores","--numfaults", "--dummy", "--eqdist","--parallel"]
            options = [ item for item in options if [ True for opt in regularopt if opt==item.command] ]
            

        # Create the parameter window
        window=Tk()

        # Window title
        window.wm_title("Fault Injection Parameters")

        frame = Frame(master=window,width=300,height=500)
        frame.pack(side=LEFT)

        # Title
        Label(frame, text="Fault Injection Configuration\n").grid(row=0, column=0, columnspan=2)

        # Create the buttons
        for i in range(len(options)):
            option=options[i]
            label=Label(frame, text=option.name)
            label.config(width=15,justify=LEFT)
            label.grid(row=1+i, column=0)

            if option.buttom == "tick":
                option.entry=StringVar()
                option.entry.set("False")
                option.buttomobj = Checkbutton(frame, variable=option.entry, offvalue="False", onvalue="True")
                option.buttomobj.config(width=15,justify=CENTER)
                # Pack in the frame
                option.buttomobj.grid(row=1+i, column=1, sticky=E)

            elif option.buttom == "choice":
                option.entry=StringVar()
                option.entry.set(option.default)
                option.buttomobj = OptionMenu(frame, option.entry, *option.choices)
                option.buttomobj.config(width=15,justify=CENTER)
                # Pack in the frame
                option.buttomobj.grid(row=1+i, column=1, sticky=E)

            elif option.buttom == "choice2":
                option.entry=StringVar()
                option.entry.set(option.default)
                option.buttomobj = Entry(frame, width=15, textvariable=option.entry)
                option.buttomobj.config(width=15,justify=CENTER)
                # Pack in the frame
                option.buttomobj.grid(row=1+i, column=1, sticky=E)

            else:
                option.entry=StringVar()
                option.entry.set(option.default)
                option.buttomobj = Entry(frame, width=15, textvariable=option.entry)
                option.buttomobj.config(width=15,justify=CENTER)
                # Pack in the frame
                option.buttomobj.grid(row=1+i, column=1, sticky=E)

        # Entry bottom
        Button(frame, text="Start", width=15, command=lambda:self.faultInjection(options,command,window)).grid(columnspan=2)
        Button(frame, text="Exit",  width=15, command=lambda:window.destroy()).grid(columnspan=2)
        frame.grid_propagate(0)

        # main Loop
        window.mainloop()

    # Convert the values and begin the FI process
    def faultInjection(self,options,command,window):
        window.destroy()
        arguments=[]

        # Include the application and workload
        arguments.append("--apps")
        arguments.append(self.options.apps)
        arguments.append("--workload")
        arguments.append(self.options.workload)
        arguments.append("--faulttype")
        arguments.append(self.options.faulttype)
        
        # Add symbol command
        if self.options.faulttype==faultTypesE.functiontrace2.name \
        or self.options.faulttype==faultTypesE.variable2.name:
            arguments.append("--tracesymbol")
            arguments.append(self.options.tracesymbol)

        # Get arguments
        for i in range(len(options)):
            if not options[i].entry.get() == "False" and len(options[i].entry.get())!=0:
                arguments.append(options[i].command)
                arguments.append(options[i].entry.get())

        # Parse the user commands
        #~ print arguments
        (FIoptions, args) = parseArgs().parse(arguments)

        # Create a new tab window
        (text, buttonsFrame)=self.createTabWithScroll(name="Fault Injection: {0}".format(self.options.apps))
        
        # communication queue
        output = queue.Queue()
        FI = faultCampaign(options=FIoptions,terminal=output)
        
        # FI thread
        self.threadFI = threading.Thread(target=FI.run, args=(command,))
        self.threadFI.start()
        
        # communication thread
        threading.Thread(target=self.terminalScroll, args=(output,text)).start()
        
        # Show results thread
        threading.Thread(target=self.showResults, args=(self.threadFI,buttonsFrame)).start()
        
    # Display result graphically 
    def showResults(self,thread,frame):
        if thread!=None:
            thread.join()
            # Report file
            filename = glob.glob("./workspace/{0}/*.reportfile".format(self.options.apps))[0]
            # Add button
            grap = graphics(replotfile=filename, environment=self.options.environment)
            Button(frame, width=15, command=grap.groupsdac, text="Plot").pack(side="left", expand=False)

