# MQSim: A Simulator for Modern NVMe and SATA SSDs

This is a preliminary version of MQSim. An updated version will be released in March 2018.


## Usage in Linux
Run following commands:
	
1. $ make

2. $ ./MQSim -i ./ssdconfig.xml -w ./workload.xml


## Usage in Windows

1. Open MQSim.sln solution file in MS Visual Studio 17 or later.
2. Set the Solution Configuration to Release (typically it is set to Debug by default).
3. You can run the generated executable file, i.e., MQSim.exe, either in command line mode or via clicking the MS Visual Studio run button.
You need to specify the path to two files 1) SSD configuration, and 2) workload definition.

Example command line execution:

$MQSim.exe -i ./ssdconfig.xml -w ./workload.xml

## SSD Configuratoin

You can specify your preferred SSD configuration in XML format. If the specified SSD configuration (i.e., ssdconfig.xml in our example) does not exist, MQSim will create a sample XML file for you. Here is the definition of configuration parameters:

### Host
1. PCIe_Lane_Bandwidth: the PCIe bandwidth per lane in GB/s. Range = {all positive double precision values}.
2. PCIe_Lane_Count: the number of PCIe lanes. Range = {all positive integers}.

### SSD Device
1. Seed: the seed value that is used for random number generation. Range = {all positive integers}.
2. HostInterface_Type: type of host interface. Range = {NVME, SATA}.
3. IO_Queue_Depth: the length of the host-side IO queue. If the host interface is set to NVME, then this value will be the capacity of each IO Submission Queue and IO Completion Queue. If host interface is set to SATA, then this value will be the capacity of the Native Command Queue (NCQ). Range = {all positive integers}
4. Queue_Fetch_Size: the value of the QueueFetchSize as described in FAST 2018 paper.
5. Data_Cache_Sharing_Mode: sharing mode of data cache (buffer) among the concurrently running IO flows when NVMe host interface is used. Range = {SHARED, EQUAL_PARTITIONING}.
6. Data_Cache_Capacity: size of the in-DRAM data cache in bytes. Range = {all positive integers}
7. Data_Cache_DRAM_Row_Size: size of DRAM rows in bytes. Range = {all positive power of two numbers}.
8. Data_Cache_DRAM_Data_Rate: DRAM data transfer rate in MT/s. Range = {all positive integer values}.
9. Data_Cache_DRAM_Data_Busrt_Size: the number of bytes that are transferred in one burst (it depends on the number of DRAM chips). Range = {all positive integer values}.
10. Data_Cache_DRAM_tRCD: tRCD parameter to access DRAM in the data cache, the is nanoseconds unit. Range = {all positive integer values}.
10. Data_Cache_DRAM_tCL: tCL parameter to access DRAM in the data cache, the is nanoseconds unit. Range = {all positive integer values}.
10. Data_Cache_DRAM_tRP: tRP parameter to access DRAM in the data cache, the is nanoseconds unit. Range = {all positive integer values}.
11. Address_Mapping: the logical-to-physical address mapping policy implemented in FTL. Range = {PAGE_LEVEL, HYBRID}.
12. CMT_Capacity: the size of the SRAM/DRAM space that is used to cache address mapping table, in kilobytes (kB) unit. Range = {all positive integer values}.
13. CMT_Sharing_Mode: determines how the entire CMT space is shared among concurrently running flows when NVMe host interface is used. Range = {SHARED, EQUAL_PARTITIONING}.
14. Plane_Allocation_Scheme: determines the plane allocation scheme. Range = {CWDP, CWPD, CDWP, CDPW, CPWD, CPDW, WCDP, WCPD, WDCP, WDPC, WPCD, WPDC, DCWP, DCPW, DWCP, DWPC, DPCW, DPWC, PCWD, PCDW, PWCD, PWDC, PDCW, PDWC, F}
15. Transaction_Scheduling_Policy: the transaction scheduling policy that is used in the SSD back end. Range = {OUT_OF_ORDER as defined in the Sprinkler paper, HPCA 2014}.
16. Overprovisioning_Ratio: the ratio of reserved storage space with respect to the whole available flash storage capacity. Range = {all positive double precision values}.
17. GC_Exect_Threshold: the threshold to starts GC execution. When the ratio of the free physical page for a plane drops below this threshold, the GC execution is started. Range = {all positive double precision values}.
18. GC_Block_Selection_Policy: the GC block selection policy. Range {GREEDY, 	RGA, /*The randomized-greedy algorithm described in: "B. Van Houdt, A Mean Field Model for a Class of Garbage Collection Algorithms in Flash - based Solid State Drives, SIGMETRICS, 2013" and "Stochastic Modeling of Large-Scale Solid-State Storage Systems: Analysis, Design Tradeoffs and Optimization, SIGMETRICS, 2013".*/ RANDOM, RANDOM_P, RANDOM_PP,/*The RANDOM, RANDOM+, and RANDOM++ algorithms described in: "B. Van Houdt, A Mean Field Model  for a Class of Garbage Collection Algorithms in Flash - based Solid State Drives, SIGMETRICS, 2013".*/, FIFO /*The FIFO algortihm described in "P. Desnoyers, Analytic  Modeling  of  SSD Write Performance, SYSTOR, 2012".*/}.
19. Preemptible_GC_Enabled: determines if preemptible GC is enabled or not. Range = {true, false}.
20. GC_Hard_Threshold: the threshold to stop preemptible GC execution. Range = {all possible positive double precision values < GC_Exect_Threshold}.
21. Prefered_suspend_erase_time_for_read: the reasonable time to suspend an ongoing flash erase operation in favor of a newly arrived read operation. Range = {all positive integer values}.
22. Prefered_suspend_erase_time_for_write: the reasonable time to suspend an ongoing flash erase operation in favor of a newly arrived read operation. Range = {all positive integer values}.
23. Prefered_suspend_write_time_for_read: the reasonable time to suspend an ongoing flash erase operation in favor of a newly arrived program operation. Range = {all positive integer values}.
24. Flash_Channel_Count: number of flash channels in the SSD back end. Range = {all positive integer values}.
25. Flash_Channel_Width: the width of each flash channel in bits. Range = {all positive integer values}.
26. Channel_Transfer_Rate: the transfer rate of flash channels in the SSD back end in MT/s unit. Range = {all positive integer values}.
27. Chip_No_Per_Channel: number of flash chips attached to each channel in the SSD back end. Range = {all positive integer values}.
28. Flash_Comm_Protocol: the ONFI protocol used for data transfer over flash channels in the SSD back end. Range = {NVDDR2}.