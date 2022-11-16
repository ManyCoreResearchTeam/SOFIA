SOFIA OVP [![License][license-img]][license-url]
=

## Suggested citation

> Jonas Gava, Vitor Bandeira, Felipe Rosa, Rafael Garibotti, Ricardo Reis, Luciano Ost, SOFIA: An automated framework for early soft error assessment, identification, and mitigation, Journal of Systems Architecture, 2022, 102710, ISSN 1383-7621, https://doi.org/10.1016/j.sysarc.2022.102710.

## Getting started with the SOFIA OVP framework

First, you need to aquire and run an imperas license to enable the necessary tools for the OVP simulator. Then we suggest using a docker container to configure the Linux environment to run SOFIA. With that ready, it is possible to run the fault injections.

### Imperas OVP infrastructure

OVP was formed in 2008 by Imperas Software, who donated a range of capabilities and model libraries to simplify the creation and use of virtual platforms. Imperas also provide a simulator, OVPsim, to demonstrate the OVP capabilities.

1. **Download OVP simulator**
   * See intructions [here](https://www.ovpworld.org)

2. **Get and Configure imperas license**
   * Contact Larry Lapides (LarryL@imperas.com) for more information.


### Docker container setup

1. **Download and Install docker** 
    * See instructions [here](https://www.docker.com)


2. **Create a docker image**
    * Get the Dockerfile [here](https://github.com/ManyCoreResearchTeam/SOFIA/blob/main/sofia-ovp/Dockerfile)
    * > docker build -t sofia-ovp:latest - < Dockerfile


3. **Run the docker container**
    * > docker run -it --rm sofia-ovp
    * If you need to syncronize files between host and target use -v parameter. 
    * > **Example:** docker run -it --rm -v /home/myuser/SOFIA:/root/ sofia-ovp

### Fault injection (FI) campaign setup

1. **Configure config script**.
Customise the variables to control the FI campaign. 
See example [here](https://github.com/ManyCoreResearchTeam/SOFIA/blob/main/sofia-ovp/config)
    * > ARCHITECTURE - choose what processor to use. Eg.: ARM_CORTEX_M7F, ARM_CORTEX_A72, RISCV32IMAC
    * > NUM_CORES - number of processor cores. *Multicore only supported when using Linux and Arm Cortex A9 or A72
    * > NUMBER_OF_FAULTS - equals to the number of injected faults
    * > FAULTS_PER_PLATFORM - number of faults per run
    * > NUMBER_OF_BITFLIPS - bitflips in the same register, variable, or memory location
    * > SEQUENTIAL_BIT_FLIP - enables sequential bitflips in the same target
    * > GENERATE_FAULT_LIST - enables the automated fault list generation
    * > NUMBER_CHKP - select the number of checkpoints to use during each FI
    * > CHECKPOINT_INTERVAL - interval of executed instructions for the checkpoints
    * > REG_LIST - list of target registers. Eg.: r0,r1,r2,r3,pc
    * > FAULT_TYPE - fault type choice. Eg.: register/memory/variable
    * > MEMORY_OPTIONS - set the target memory location (FLASH, RAM).
    * > SYMBOL - target function name when using FAULT_TYPE=functiontrace
    * > FUNCTION_INSTANCE - number of function trace instances
    * > GENERATE_FAULT_LIST_DISTR - enable the same number of faults per register
    * > TARGET_MEM_BASE - memory fault injection paramters
    * > TARGET_MEM_SIZE - memory fault injection paramters
    * > WORKLOAD_TYPE - two options available (WORKLOAD_BAREMETAL, WORKLOAD_LINUX)
    * > LI_OPTIONS - compile the application and run (LI_USE_CLEAN), do not compile and run available elf in the workload folder (LI_DONT_COMPILE), just compile (LI_JUST_COMPILE)
    * > ONLY_GOLD - execute only gold run
    * > ENABLE_CONSOLE - set 1 to enable UART window
    * > ENABLE_GRAPHIC - set 1 to enable UART window
    * > ENABLE_TRACE - enable OVP Instruction Trace
    * > ENABLE_PROFILING - enable register profiling. *Available only for Arm Cortex A72 with Linux
    * > TIME_SLICE - quantum/time-slice
    * > PARALLEL - maximum number of parallel threads in the host CPU
    * > LICENSE - license server name
    * > IMPERAS_VERSION - imperas version
    * > OPTFLAG - compiler optimization flag. Eg.: 3 (for O3)
    * > *Next variables set the Bare Metal and Linux Crosscompiler paths*

2. **Run Fault Injection**.
    * With the configuration done, now you can run the fault injection campaign.
    * > ./FI.sh app-name
    * > Example: ./FI.sh factorial

3. **FI results Analysis**.
    * Check the results in workspace/appname/*reportfile
    * Check image plot in workspace/appname/groupsdac.eps
    * If you have prints in your code, you can see it in PlatformLogs/uart0-x for Linux and PlatformLogs/platform-x for bare metal


### How to add new code
  * Add a folder with C code inside workloads/baremetal (for bare metal applications) or workloads/linux (for Linux applications)
  * Modify the app.c code including the FIM.h header
  * Put FIM_Instantiate() at the start of main function and FIM_exit() at the end of main function (before return)

See the available examples ([baremetal](https://github.com/ManyCoreResearchTeam/SOFIA/tree/main/sofia-ovp/workloads/baremetal) and [linux](https://github.com/ManyCoreResearchTeam/SOFIA/tree/main/sofia-ovp/workloads/linux))


Authors
------
* [**Felipe da Rosa**](https://www.linkedin.com/in/frdarosa)
* [**Jonas Gava**](https://www.linkedin.com/in/jfgava)
* [**Geancarlo Abich**](https://www.linkedin.com/in/geancarloabich/)
* [**Vitor Bandeira**](https://www.linkedin.com/in/vitor-bandeira-093a0b118/)
* [**Isadora Oliveira**](https://www.linkedin.com/in/isadora-oliveira-6344b815b/)


License
-------
MIT License. See [LICENSE](LICENSE) for details.

[main-url]: https://github.com/ManyCoreResearchTeam/SOFIA
[readme-url]: https://github.com/ManyCoreResearchTeam/SOFIA/blob/main/README.md
[license-url]: https://github.com/ManyCoreResearchTeam/SOFIA/blob/main/LICENSE
[license-img]: https://img.shields.io/github/license/rsp/travis-hello-modern-cpp.svg
[github-follow-url]: https://github.com/jfgava
