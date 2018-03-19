# MQSim: A Simulator for Modern NVMe and SATA SSDs

This is a preliminary version of MQSim. An updated version will be released in March 2018.


## Usage in Linux
Run following commands:
	
1. $ make

2. $ ./MQSim -i ./ssdconfig.xml -w ./workload.xml


## Usage in Windows

1. Open MQSim.sln solution file in MS Visual Studio 17 or later.
2. Set the Solution Configuration to Release (typically it is set to Debug by default).
3. You can run the generated executable file, i.e., MQSim.exe, both in command line mode or via clicking the MS Visual Studio run button.
You need to specify the path to two files 1) SSD configuration, and 2) workload definition.

Example commandline execution:

$MQSim.exe -i ./ssdconfig.xml -w ./workload.xml

### SSD Configuratoin

You can specify your preferred SSD configuration in XML format. If the specified SSD configuration (i.e., ssdconfig.xml in our example) does not exists, MQSim will create a sample XML file for you.
