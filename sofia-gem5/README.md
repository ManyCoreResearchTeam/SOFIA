SOFIA gem5 [![License][license-img]][license-url]
=

WILL BE AVAILABLE SOON

## Suggested citation

> Jonas Gava, Vitor Bandeira, Felipe Rosa, Rafael Garibotti, Ricardo Reis, Luciano Ost, SOFIA: An automated framework for early soft error assessment, identification, and mitigation, Journal of Systems Architecture, 2022, 102710, ISSN 1383-7621, https://doi.org/10.1016/j.sysarc.2022.102710.



## Limitations: 

The ARMv7 model uses the VExpress_EMM platform with dtb files to 1, 2, and 4 cores with a 3.13 Linux kernel. This setup is keep for comparison purposes with the OVP. Newer kernels 4.x can be used with the VExpress_GEM5_V1 platform up to 8 cores.


## Remarks: 
	1) Checkpoint Drain() - The Gem5 simulator suffers of problems in the timing model during the drain() function after a unexpected termination (Page Fault error, for instance). The models continue in the simulation for ever.
	2) timout - https://github.com/pshved/timeout - You can use "./timeout -t 60 -m 4000000 ./ScriptFaultInection.sh" to delimit the time or memory during the simulation
	3) threading implementation in to the linux image getconf GNU_LIBPTHREAD_VERSION NPTL 2.11.1
	4) For open a telnet continuously while :; do telnet 127.0.0.1 3456 2>/dev/null ; sleep 1; done


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
[github-follow-url]: https://github.com/ManyCoreResearchTeam
