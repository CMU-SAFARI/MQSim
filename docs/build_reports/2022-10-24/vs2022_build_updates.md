# Visual Studio Code IDE

## Build Updates

- Visual Studio Community 2022 (64-bit) Version 17.7.5
  - Platform Toolset = v143
- change project settings (MQSim.vcxproj)
  - Project | Properties | Configuration Properties
    - **General** | All Configurations and All Platforms
      - Output Directory = $(SolutionDir)build\$(Platform)\$(Configuration)\
      - Intermediate Directory = $(SolutionDir)build\$(Platform)\$(Configuration)\interm
    - **Debugging** | All Configurations and All Platforms
      - Command Arguments = -i $(SolutionDir)ssdconfig.xml -w $(SolutionDir)workload.xml
    - C/C++
      - **Preprocessor**
        - Configuration = Release | Platform = Win32
          - Preprocessor Definitions = WIN32;NDEBUG;_CONSOLE;_CRT_SECURE_NO_WARNINGS;%(PreprocessorDefinitions)
      - **Precompiled Headers** | All Configurations and All Platforms
        - Precompiled Header = Not Using Precompiled Headers

## Batch Build Results

```shell
Build started...
------ Build started: Project: MQSim, Configuration: Release Win32 ------
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Microsoft\VC\v170\Microsoft.CppBuild.targets(516,5): warning MSB8004: Intermediate Directory does not end with a trailing slash.  This build instance will add the slash as it is required to allow proper evaluation of the Intermediate Directory.
Device_Parameter_Set.cpp
Execution_Parameter_Set.cpp
Flash_Parameter_Set.cpp
Host_Parameter_Set.cpp
Host_System.cpp
IO_Flow_Parameter_Set.cpp
SSD_Device.cpp
IO_Flow_Base.cpp
IO_Flow_Synthetic.cpp
IO_Flow_Trace_Based.cpp
C:\code\ghcwm\MQSim_public\src\host\IO_Flow_Trace_Based.cpp(292,30): warning C4244: 'argument': conversion from 'sim_time_type' to 'const unsigned int', possible loss of data
C:\code\ghcwm\MQSim_public\src\host\IO_Flow_Trace_Based.cpp(312,29): warning C4244: 'argument': conversion from 'sim_time_type' to 'const unsigned int', possible loss of data
PCIe_Link.cpp
PCIe_Root_Complex.cpp
PCIe_Switch.cpp
SATA_HBA.cpp
main.cpp
Block.cpp
Die.cpp
Flash_Chip.cpp
Physical_Page_Address.cpp
Plane.cpp
Compiling...
Engine.cpp
EventTree.cpp
Address_Mapping_Unit_Base.cpp
Address_Mapping_Unit_Hybrid.cpp
Address_Mapping_Unit_Page_Level.cpp
C:\code\ghcwm\MQSim_public\src\ssd\Address_Mapping_Unit_Page_Level.cpp(200,41): warning C4244: 'initializing': conversion from 'LPA_type' to 'unsigned int', possible loss of data
C:\code\ghcwm\MQSim_public\src\ssd\Address_Mapping_Unit_Page_Level.cpp(716,125): warning C4018: '<': signed/unsigned mismatch
Data_Cache_Flash.cpp
Data_Cache_Manager_Base.cpp
Data_Cache_Manager_Flash_Advanced.cpp
Data_Cache_Manager_Flash_Simple.cpp
Flash_Block_Manager.cpp
C:\code\ghcwm\MQSim_public\src\ssd\Flash_Block_Manager.cpp(69,21): warning C4018: '<': signed/unsigned mismatch
Flash_Block_Manager_Base.cpp
Flash_Transaction_Queue.cpp
FTL.cpp
GC_and_WL_Unit_Base.cpp
GC_and_WL_Unit_Page_Level.cpp
Host_Interface_Base.cpp
Host_Interface_NVMe.cpp
Host_Interface_SATA.cpp
NVM_Firmware.cpp
NVM_PHY_Base.cpp
Compiling...
NVM_PHY_ONFI.cpp
NVM_PHY_ONFI_NVDDR2.cpp
NVM_Transaction_Flash.cpp
NVM_Transaction_Flash_ER.cpp
NVM_Transaction_Flash_RD.cpp
NVM_Transaction_Flash_WR.cpp
ONFI_Channel_Base.cpp
ONFI_Channel_NVDDR2.cpp
Queue_Probe.cpp
Stats.cpp
TSU_Base.cpp
TSU_FLIN.cpp
TSU_OutofOrder.cpp
TSU_Priority_OutofOrder.cpp
User_Request.cpp
CMRRandomGenerator.cpp
Helper_Functions.cpp
C:\code\ghcwm\MQSim_public\src\utils\Helper_Functions.cpp(38,21): warning C4018: '<=': signed/unsigned mismatch
C:\code\ghcwm\MQSim_public\src\utils\Helper_Functions.cpp(45,23): warning C4018: '<': signed/unsigned mismatch
C:\code\ghcwm\MQSim_public\src\utils\Helper_Functions.cpp(70,22): warning C4018: '<': signed/unsigned mismatch
Logical_Address_Partitioning_Unit.cpp
RandomGenerator.cpp
StringTools.cpp
Compiling...
XMLWriter.cpp
Generating code
Previous IPDB not found, fall back to full compilation.
All 5024 functions were compiled because no usable IPDB/IOBJ from previous compilation was found.
Finished generating code
MQSim.vcxproj -> C:\code\ghcwm\MQSim_public\build\Win32\Release\MQSim.exe
Done building project "MQSim.vcxproj".
------ Build started: Project: MQSim, Configuration: Release x64 ------
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Microsoft\VC\v170\Microsoft.CppBuild.targets(516,5): warning MSB8004: Intermediate Directory does not end with a trailing slash.  This build instance will add the slash as it is required to allow proper evaluation of the Intermediate Directory.
Device_Parameter_Set.cpp
Execution_Parameter_Set.cpp
Flash_Parameter_Set.cpp
Host_Parameter_Set.cpp
Host_System.cpp
IO_Flow_Parameter_Set.cpp
SSD_Device.cpp
IO_Flow_Base.cpp
IO_Flow_Synthetic.cpp
IO_Flow_Trace_Based.cpp
PCIe_Link.cpp
PCIe_Root_Complex.cpp
PCIe_Switch.cpp
SATA_HBA.cpp
main.cpp
Block.cpp
Die.cpp
Flash_Chip.cpp
Physical_Page_Address.cpp
Plane.cpp
Compiling...
Engine.cpp
EventTree.cpp
Address_Mapping_Unit_Base.cpp
Address_Mapping_Unit_Hybrid.cpp
Address_Mapping_Unit_Page_Level.cpp
Data_Cache_Flash.cpp
Data_Cache_Manager_Base.cpp
Data_Cache_Manager_Flash_Advanced.cpp
Data_Cache_Manager_Flash_Simple.cpp
Flash_Block_Manager.cpp
Flash_Block_Manager_Base.cpp
Flash_Transaction_Queue.cpp
FTL.cpp
GC_and_WL_Unit_Base.cpp
GC_and_WL_Unit_Page_Level.cpp
Host_Interface_Base.cpp
Host_Interface_NVMe.cpp
Host_Interface_SATA.cpp
NVM_Firmware.cpp
NVM_PHY_Base.cpp
Compiling...
NVM_PHY_ONFI.cpp
NVM_PHY_ONFI_NVDDR2.cpp
NVM_Transaction_Flash.cpp
NVM_Transaction_Flash_ER.cpp
NVM_Transaction_Flash_RD.cpp
NVM_Transaction_Flash_WR.cpp
ONFI_Channel_Base.cpp
ONFI_Channel_NVDDR2.cpp
Queue_Probe.cpp
Stats.cpp
TSU_Base.cpp
TSU_FLIN.cpp
TSU_OutofOrder.cpp
TSU_Priority_OutofOrder.cpp
User_Request.cpp
CMRRandomGenerator.cpp
Helper_Functions.cpp
Logical_Address_Partitioning_Unit.cpp
RandomGenerator.cpp
StringTools.cpp
Compiling...
XMLWriter.cpp
Generating code
Previous IPDB not found, fall back to full compilation.
All 5024 functions were compiled because no usable IPDB/IOBJ from previous compilation was found.
Finished generating code
MQSim.vcxproj -> C:\code\ghcwm\MQSim_public\build\x64\Release\MQSim.exe
Done building project "MQSim.vcxproj".
------ Build started: Project: MQSim, Configuration: Debug Win32 ------
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Microsoft\VC\v170\Microsoft.CppBuild.targets(516,5): warning MSB8004: Intermediate Directory does not end with a trailing slash.  This build instance will add the slash as it is required to allow proper evaluation of the Intermediate Directory.
Device_Parameter_Set.cpp
Execution_Parameter_Set.cpp
Flash_Parameter_Set.cpp
Host_Parameter_Set.cpp
Host_System.cpp
IO_Flow_Parameter_Set.cpp
SSD_Device.cpp
IO_Flow_Base.cpp
IO_Flow_Synthetic.cpp
IO_Flow_Trace_Based.cpp
C:\code\ghcwm\MQSim_public\src\host\IO_Flow_Trace_Based.cpp(292,30): warning C4244: 'argument': conversion from 'sim_time_type' to 'const unsigned int', possible loss of data
C:\code\ghcwm\MQSim_public\src\host\IO_Flow_Trace_Based.cpp(312,29): warning C4244: 'argument': conversion from 'sim_time_type' to 'const unsigned int', possible loss of data
PCIe_Link.cpp
PCIe_Root_Complex.cpp
PCIe_Switch.cpp
SATA_HBA.cpp
main.cpp
Block.cpp
Die.cpp
Flash_Chip.cpp
Physical_Page_Address.cpp
Plane.cpp
Generating Code...
Compiling...
Engine.cpp
EventTree.cpp
Address_Mapping_Unit_Base.cpp
Address_Mapping_Unit_Hybrid.cpp
Address_Mapping_Unit_Page_Level.cpp
C:\code\ghcwm\MQSim_public\src\ssd\Address_Mapping_Unit_Page_Level.cpp(200,41): warning C4244: 'initializing': conversion from 'LPA_type' to 'unsigned int', possible loss of data
C:\code\ghcwm\MQSim_public\src\ssd\Address_Mapping_Unit_Page_Level.cpp(716,125): warning C4018: '<': signed/unsigned mismatch
Data_Cache_Flash.cpp
Data_Cache_Manager_Base.cpp
Data_Cache_Manager_Flash_Advanced.cpp
Data_Cache_Manager_Flash_Simple.cpp
Flash_Block_Manager.cpp
C:\code\ghcwm\MQSim_public\src\ssd\Flash_Block_Manager.cpp(69,21): warning C4018: '<': signed/unsigned mismatch
Flash_Block_Manager_Base.cpp
Flash_Transaction_Queue.cpp
FTL.cpp
GC_and_WL_Unit_Base.cpp
GC_and_WL_Unit_Page_Level.cpp
Host_Interface_Base.cpp
Host_Interface_NVMe.cpp
Host_Interface_SATA.cpp
NVM_Firmware.cpp
NVM_PHY_Base.cpp
Generating Code...
Compiling...
NVM_PHY_ONFI.cpp
NVM_PHY_ONFI_NVDDR2.cpp
NVM_Transaction_Flash.cpp
NVM_Transaction_Flash_ER.cpp
NVM_Transaction_Flash_RD.cpp
NVM_Transaction_Flash_WR.cpp
ONFI_Channel_Base.cpp
ONFI_Channel_NVDDR2.cpp
Queue_Probe.cpp
Stats.cpp
TSU_Base.cpp
TSU_FLIN.cpp
TSU_OutofOrder.cpp
TSU_Priority_OutofOrder.cpp
User_Request.cpp
CMRRandomGenerator.cpp
Helper_Functions.cpp
C:\code\ghcwm\MQSim_public\src\utils\Helper_Functions.cpp(38,21): warning C4018: '<=': signed/unsigned mismatch
C:\code\ghcwm\MQSim_public\src\utils\Helper_Functions.cpp(45,23): warning C4018: '<': signed/unsigned mismatch
C:\code\ghcwm\MQSim_public\src\utils\Helper_Functions.cpp(70,22): warning C4018: '<': signed/unsigned mismatch
Logical_Address_Partitioning_Unit.cpp
RandomGenerator.cpp
StringTools.cpp
Generating Code...
Compiling...
XMLWriter.cpp
Generating Code...
MQSim.vcxproj -> C:\code\ghcwm\MQSim_public\build\Win32\Debug\MQSim.exe
Done building project "MQSim.vcxproj".
------ Build started: Project: MQSim, Configuration: Debug x64 ------
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Microsoft\VC\v170\Microsoft.CppBuild.targets(516,5): warning MSB8004: Intermediate Directory does not end with a trailing slash.  This build instance will add the slash as it is required to allow proper evaluation of the Intermediate Directory.
Device_Parameter_Set.cpp
Execution_Parameter_Set.cpp
Flash_Parameter_Set.cpp
Host_Parameter_Set.cpp
Host_System.cpp
IO_Flow_Parameter_Set.cpp
SSD_Device.cpp
IO_Flow_Base.cpp
IO_Flow_Synthetic.cpp
IO_Flow_Trace_Based.cpp
PCIe_Link.cpp
PCIe_Root_Complex.cpp
PCIe_Switch.cpp
SATA_HBA.cpp
main.cpp
Block.cpp
Die.cpp
Flash_Chip.cpp
Physical_Page_Address.cpp
Plane.cpp
Generating Code...
Compiling...
Engine.cpp
EventTree.cpp
Address_Mapping_Unit_Base.cpp
Address_Mapping_Unit_Hybrid.cpp
Address_Mapping_Unit_Page_Level.cpp
Data_Cache_Flash.cpp
Data_Cache_Manager_Base.cpp
Data_Cache_Manager_Flash_Advanced.cpp
Data_Cache_Manager_Flash_Simple.cpp
Flash_Block_Manager.cpp
Flash_Block_Manager_Base.cpp
Flash_Transaction_Queue.cpp
FTL.cpp
GC_and_WL_Unit_Base.cpp
GC_and_WL_Unit_Page_Level.cpp
Host_Interface_Base.cpp
Host_Interface_NVMe.cpp
Host_Interface_SATA.cpp
NVM_Firmware.cpp
NVM_PHY_Base.cpp
Generating Code...
Compiling...
NVM_PHY_ONFI.cpp
NVM_PHY_ONFI_NVDDR2.cpp
NVM_Transaction_Flash.cpp
NVM_Transaction_Flash_ER.cpp
NVM_Transaction_Flash_RD.cpp
NVM_Transaction_Flash_WR.cpp
ONFI_Channel_Base.cpp
ONFI_Channel_NVDDR2.cpp
Queue_Probe.cpp
Stats.cpp
TSU_Base.cpp
TSU_FLIN.cpp
TSU_OutofOrder.cpp
TSU_Priority_OutofOrder.cpp
User_Request.cpp
CMRRandomGenerator.cpp
Helper_Functions.cpp
Logical_Address_Partitioning_Unit.cpp
RandomGenerator.cpp
StringTools.cpp
Generating Code...
Compiling...
XMLWriter.cpp
Generating Code...
MQSim.vcxproj -> C:\code\ghcwm\MQSim_public\build\x64\Debug\MQSim.exe
Done building project "MQSim.vcxproj".
========== Build: 4 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========
========== Build started at 8:45 AM and took 03:25.026 minutes ==========

```

---
