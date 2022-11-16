#
# Copyright (c) 2005-2016 Imperas Software Ltd., www.imperas.com
#
# The contents of this file are provided under the Software License
# Agreement that you accepted before downloading this file.
#
# This source forms part of the Software and can be used for educational,
# training, and demonstration purposes but cannot be used for derivative
# works except in cases where the derivative works require OVP technology
# to run.
#
# For open source models released under licenses that you can use for
# derivative works, please visit www.OVPworld.org or www.imperas.com
# for the location of the open source models.
#

set desc "
This platform models the ARM Versatile Express development board with a CoreTile Express A9x4 (V2P-CA9) Daughterboard.
See the ARM documents DUI0447G_v2m_p1_trm.pdf and DUI0448G_v2p_ca9_trm.pdf for details of the hardware being modeled. 
Note this platform implements the motherboard's 'Legacy' memory map.

The default processor is an ARM Cortex-A9MPx4, which may be changed."

set limitations "
This platform provides the peripherals required to boot and run Operating Systems such as Linux or Android.
Some of the peripherals are register-only, non-functional models. See the individual peripheral model documentation for details.

The TrustZone Protection Controller (TZPC) is modeled, the TrustZone Address Space Controller (TZASPC) is not modeled.

CoreSight software debug and trace ports are not modeled.
"

# Setup variables for platform info
set vendor  $::env(vendor)
set library platform
set name    FIM
set version 1.0

#
# Start new platform creation
#
ihwnew -name $name -vendor $vendor -library $library -version $version -stoponctrlc  -purpose epk -releasestatus ovp
iadddocumentation -name Licensing   -text "Open Source Apache 2.0"
iadddocumentation -name Description -text $desc
iadddocumentation -name Limitations -text $limitations

iadddocumentation -name Reference   -text "..."

for {set i 0} {$i < $::env(NODES)} {incr i} {
ihwaddmodule -modelfile $::env(PLATFORM_FOLDER)/module  -instancename DUT${i}
}
#####################################################################################
# Command line arguments
#####################################################################################
ihwaddclp -allargs
ihwaddclparg -name  zimage      -group userPlatformConfig -type stringvar -description "Linux zImage file to load using smartLoader" 
ihwaddclparg -name  zimageaddr  -group userPlatformConfig -type uns64var  -description "Physical address to load zImage file (default:physicalbase + 0x00010000)"
ihwaddclparg -name  initrd      -group userPlatformConfig -type stringvar -description "Linux initrd file to load using smartLoader"
ihwaddclparg -name  initrdaddr  -group userPlatformConfig -type uns64var  -description "Physical address to load initrd file (default:physicalbase + 0x00d00000)"
ihwaddclparg -name  linuxsym    -group userPlatformConfig -type stringvar -description "Linux ELF file with symbolic debug info (CpuManger only)"
ihwaddclparg -name  linuxcmd    -group userPlatformConfig -type stringvar -description "Linux command line (default: 'mem=1024M raid=noautodetect console=ttyAMA0,38400n8 vmalloc=256MB devtmpfs.mount=0'"
ihwaddclparg -name  boardid     -group userPlatformConfig -type int32var  -description "Value to pass to Linux as the boardid (default (0x8e0)"
ihwaddclparg -name  linuxmem    -group userPlatformConfig -type uns64var  -description "Amount of memory allocated to Linux (required in AMP mode)"
ihwaddclparg -name  boot        -group userPlatformConfig -type stringvar -description "Boot code object file (If specified, smartLoader will jump to this rather than zImage)"
ihwaddclparg -name  image0      -group userPlatformConfig -type stringvar -description "Image file to load on cpu0"
ihwaddclparg -name  image0addr  -group userPlatformConfig -type uns64var  -description "load address for image on cpu0 (IMAGE0 must be specified)"
ihwaddclparg -name  image0sym   -group userPlatformConfig -type stringvar -description "Elf file with symbolic debug info for image on cpu0 (IMAGE0 must be specified, CpuManger only)"
ihwaddclparg -name  image1      -group userPlatformConfig -type stringvar -description "Image file to load on cpu1 to n"
ihwaddclparg -name  image1addr  -group userPlatformConfig -type uns64var  -description "Load address for image on cpu1 to n (IMAGE1 must be specified)"
ihwaddclparg -name  image1sym   -group userPlatformConfig -type stringvar -description "Elf file with symbolic debug info for image on cpu1 to n (IMAGE1 must be specified, CpuManger only)"
ihwaddclparg -name  uart0port   -group userPlatformConfig -type stringvar -description "Uart0 port: 'auto' for automatic console, 0 for simulator chosen port #, or number of specific port"
ihwaddclparg -name  uart1port   -group userPlatformConfig -type stringvar -description "Uart1 port: 'auto' for automatic console, 0 for simulator chosen port #, or number of specific port"
ihwaddclparg -name  nographics  -group userPlatformConfig -type boolvar   -description "Inhibit opening of the lcd graphics window"

#####################################################################################
# FAULT INJECTION Command line arguments
#####################################################################################
ihwaddclparg -name  goldrun            -group userPlatformConfig -type boolvar     -description "Gold run to acquire fundamental information about the application"
ihwaddclparg -name  faultcampaignrun   -group userPlatformConfig -type boolvar     -description "Inject faults"
ihwaddclparg -name  enableconsole      -group userPlatformConfig -type boolvar     -description "enable pop-up window terminal console"
ihwaddclparg -name  linuxgraphics      -group userPlatformConfig -type boolvar     -description "enable pop-up window graphical interface"

ihwaddclparg -name  arch               -group userPlatformConfig -type stringvar   -description "Specified architecture  -- @Review"
ihwaddclparg -name  application        -group userPlatformConfig -type stringvar   -description "Application binary file -- @Review"
ihwaddclparg -name  projectfolder      -group userPlatformConfig -type stringvar   -description "Project folder"
ihwaddclparg -name  applicationfolder  -group userPlatformConfig -type stringvar   -description "Workspace folder"
ihwaddclparg -name  mode               -group userPlatformConfig -type stringvar   -description "Mode Linux or Baremetal"
ihwaddclparg -name  faultlist          -group userPlatformConfig -type stringvar   -description "File containing the fault list"
ihwaddclparg -name  faulttype          -group userPlatformConfig -type stringvar   -description "Fault type"
ihwaddclparg -name  linuxdtb           -group userPlatformConfig -type stringvar   -description "DTB file"
ihwaddclparg -name  environment        -group userPlatformConfig -type stringvar   -description "Environment variable"
ihwaddclparg -name  interceptlib       -group userPlatformConfig -type stringvar   -description "intercept model.so"

ihwaddclparg -name  platformid         -group userPlatformConfig -type uns32var    -description "Unique platform identifier default"
ihwaddclparg -name  numberoffaults     -group userPlatformConfig -type uns32var    -description "Number of faults to be injected in this platform default, equal to the number of running processors"
ihwaddclparg -name  checkpointinterval -group userPlatformConfig -type uns64var    -description "Create periodical checkpoints during the gold execution and use to speedup the Fault Injection"
ihwaddclparg -name  targetcpucores     -group userPlatformConfig -type uns32var    -description "Number of cores in the target CPU"
ihwaddclparg -name  applicationargs    -group userPlatformConfig -type uns32var    -description "Argument to the application -- @Review"

# Symbol trace and related options
ihwaddclparg -name  enablefunctionprofile -group userPlatformConfig -type boolvar  -description "Enable the profile of the application after the OS boot just before the application starts"
ihwaddclparg -name  enablelinecoverage    -group userPlatformConfig -type boolvar  -description "Enable the application linecovarage after the OS boot just before the application starts"

ihwaddclparg -name  enableitrace          -group userPlatformConfig -type boolvar       -description "Enable the application function profile"
ihwaddclparg -name  tracesymbol           -group userPlatformConfig -type stringvar     -description "..."
ihwaddclparg -name  tracevariable         -group userPlatformConfig -type stringvar     -description "..."

ihwaddclparg -name  enableftrace          -group userPlatformConfig -type boolvar       -description "Enable the application function profile"














