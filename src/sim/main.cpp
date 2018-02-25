#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <cstring>
#include "../ssd/SSD_Defs.h"
#include "../exec/Execution_Parameter_Set.h"
#include "../exec/SSD_Device.h"
#include "../exec/Host_System.h"
#include "../utils/rapidxml/rapidxml.hpp"

using namespace std;



void command_line_args(char* argv[], string& input_file_path, string& workload_file_path)
{

	for (int arg_cntr = 1; arg_cntr < 5; arg_cntr++)
	{
		string arg = argv[arg_cntr];

		char file_path_switch[] = "-i";
		if (arg.compare(0, strlen(file_path_switch), file_path_switch) == 0) {
			input_file_path.assign(argv[++arg_cntr]);
			//cout << input_file_path << endl;
			continue;
		}

		char workload_path_switch[] = "-w";
		if (arg.compare(0, strlen(workload_path_switch), workload_path_switch) == 0) {
			workload_file_path.assign(argv[++arg_cntr]);
			//cout << workload_file_path << endl;
			continue;
		}
	}
}

void read_parameters(string ssd_config_file_path, string workload_defs_file_path, Execution_Parameter_Set* exec_params)
{
	ifstream ssd_config_file;
	ssd_config_file.open(ssd_config_file_path.c_str());

	if (!ssd_config_file) {
		PRINT_MESSAGE("The specified SSD configuration file does not exits")
		PRINT_MESSAGE("Using MQSim's default parameters")
	}
	else
	{
		//Read input workload parameters
		string line;
		ssd_config_file >> line;
		if (line.compare("USE_INTERNAL_PARAMS"))
		{
			PRINT_MESSAGE("Using MQSim's default parameters")
		}
		else
		{
		}
	}

	ifstream workload_defs_file;
	workload_defs_file.open(workload_defs_file_path.c_str());
	bool use_default_workloads = true;
	if (!workload_defs_file) {
		PRINT_MESSAGE("The specified workload definition file does not exits!")
		PRINT_MESSAGE("Using MQSim's default workload definitions")
	}
	else
	{
		string line((std::istreambuf_iterator<char>(workload_defs_file)),
			std::istreambuf_iterator<char>());
		if (line.compare("USE_INTERNAL_PARAMS") != 0)
		{
			rapidxml::xml_document<> doc;    // character type defaults to char
			char* temp_string = new char[line.length() + 1];
			strcpy(temp_string, line.c_str());
			doc.parse<0>(temp_string);
			rapidxml::xml_node<> *mqsim_io_scenarios = doc.first_node("MQSim_IO_Scenarios");
			if (mqsim_io_scenarios != NULL)
			{
				for (auto io_scenario = mqsim_io_scenarios->first_node(); io_scenario; io_scenario = io_scenario->next_sibling())
				{
					for (auto flow_def = io_scenario->first_node(); flow_def; flow_def = flow_def->next_sibling())
					{
						IO_Flow_Parameter_Set* flow;
						if (strcmp(flow_def->name(), "IO_Flow_Parameter_Set_Synthetic") == 0)
						{
							flow = new IO_Flow_Parameter_Set_Synthetic;
							((IO_Flow_Parameter_Set_Synthetic*)flow)->Deserialize(flow_def);
						}
						else if (strcmp(flow_def->name(), "IO_Flow_Parameter_Set_Trace_Based") == 0)
						{
							flow = new IO_Flow_Parameter_Set_Trace_Based;
							((IO_Flow_Parameter_Set_Trace_Based*)flow)->Deserialize(flow_def);
						}
						exec_params->Host_Configuration.IO_Flow_Definitions.push_back(flow);
					}
					use_default_workloads = false;
				}
			} 
			else
			{
				PRINT_MESSAGE("Error in the workload definition file!")
				PRINT_MESSAGE("Using MQSim's default workload definitions")
			}
		}
	}

	if (use_default_workloads)
	{
		IO_Flow_Parameter_Set_Synthetic* io_flow_1 = new IO_Flow_Parameter_Set_Synthetic;
		io_flow_1->Device_Level_Data_Caching_Mode = SSD_Components::Caching_Mode::WRITE_CACHE;
		io_flow_1->Type = Flow_Type::SYNTHETIC;
		io_flow_1->Priority_Class = IO_Flow_Priority_Class::HIGH;
		io_flow_1->Channel_No = 8;
		io_flow_1->Channel_IDs = new flash_channel_ID_type[8];
		io_flow_1->Channel_IDs[0] = 0; io_flow_1->Channel_IDs[1] = 1; io_flow_1->Channel_IDs[2] = 2; io_flow_1->Channel_IDs[3] = 3;
		io_flow_1->Channel_IDs[4] = 4; io_flow_1->Channel_IDs[5] = 5; io_flow_1->Channel_IDs[6] = 6; io_flow_1->Channel_IDs[7] = 7;
		io_flow_1->Chip_No = 4;
		io_flow_1->Chip_IDs = new flash_chip_ID_type[4];
		io_flow_1->Chip_IDs[0] = 0; io_flow_1->Chip_IDs[1] = 1; io_flow_1->Chip_IDs[2] = 2; io_flow_1->Chip_IDs[3] = 3;
		io_flow_1->Die_No = 2;
		io_flow_1->Die_IDs = new flash_die_ID_type[2];
		io_flow_1->Die_IDs[0] = 0; io_flow_1->Die_IDs[1] = 1;
		io_flow_1->Plane_No = 2;
		io_flow_1->Plane_IDs = new flash_plane_ID_type[2];
		io_flow_1->Plane_IDs[0] = 0; io_flow_1->Plane_IDs[1] = 1;
		io_flow_1->Read_Percentage = 100;
		io_flow_1->Address_Distribution = Host_Components::Address_Distribution_Type::UNIFORM_RANDOM;
		io_flow_1->Percentage_of_Hot_Region = 0.0;
		io_flow_1->Request_Size_Distribution = Host_Components::Request_Size_Distribution_Type::FIXED;
		io_flow_1->Average_Request_Size = 8;
		io_flow_1->Variance_Request_Size = 0;
		io_flow_1->Seed = 12344;
		io_flow_1->Average_No_of_Reqs_in_Queue = 2;
		io_flow_1->Stop_Time = 1000000000;
		io_flow_1->Total_Requests_To_Generate = 0;
		exec_params->Host_Configuration.IO_Flow_Definitions.push_back(io_flow_1);

		IO_Flow_Parameter_Set_Synthetic* io_flow_2 = new IO_Flow_Parameter_Set_Synthetic;
		io_flow_2->Device_Level_Data_Caching_Mode = SSD_Components::Caching_Mode::WRITE_CACHE;
		io_flow_2->Type = Flow_Type::SYNTHETIC;
		io_flow_2->Priority_Class = IO_Flow_Priority_Class::HIGH;
		io_flow_2->Channel_No = 8;
		io_flow_2->Channel_IDs = new flash_channel_ID_type[8];
		io_flow_2->Channel_IDs[0] = 0; io_flow_2->Channel_IDs[1] = 1; io_flow_2->Channel_IDs[2] = 2; io_flow_2->Channel_IDs[3] = 3;
		io_flow_2->Channel_IDs[4] = 4; io_flow_2->Channel_IDs[5] = 5; io_flow_2->Channel_IDs[6] = 6; io_flow_2->Channel_IDs[7] = 7;
		io_flow_2->Chip_No = 4;
		io_flow_2->Chip_IDs = new flash_chip_ID_type[4];
		io_flow_2->Chip_IDs[0] = 0; io_flow_2->Chip_IDs[1] = 1; io_flow_2->Chip_IDs[2] = 2; io_flow_2->Chip_IDs[3] = 3;
		io_flow_2->Die_No = 2;
		io_flow_2->Die_IDs = new flash_die_ID_type[2];
		io_flow_2->Die_IDs[0] = 0; io_flow_2->Die_IDs[1] = 1;
		io_flow_2->Plane_No = 2;
		io_flow_2->Plane_IDs = new flash_plane_ID_type[2];
		io_flow_2->Plane_IDs[0] = 0; io_flow_2->Plane_IDs[1] = 1;
		io_flow_2->Read_Percentage = 100;
		io_flow_2->Address_Distribution = Host_Components::Address_Distribution_Type::STREAMING;
		io_flow_2->Percentage_of_Hot_Region = 0.0;
		io_flow_2->Request_Size_Distribution = Host_Components::Request_Size_Distribution_Type::FIXED;
		io_flow_2->Average_Request_Size = 8;
		io_flow_2->Variance_Request_Size = 0;
		io_flow_2->Seed = 6533;
		io_flow_2->Average_No_of_Reqs_in_Queue = 2;
		io_flow_2->Stop_Time = 1000000000;
		io_flow_2->Total_Requests_To_Generate = 0;
		exec_params->Host_Configuration.IO_Flow_Definitions.push_back(io_flow_2);

		PRINT_MESSAGE("Writing default workload parameters to the expected input file.")

		Utils::XmlWriter xmlwriter;
		string tmp;
		xmlwriter.Open(workload_defs_file_path.c_str());
		tmp = "MQSim_IO_Scenarios";
		xmlwriter.Write_open_tag(tmp);
		tmp = "IO_Scenario";
		xmlwriter.Write_open_tag(tmp);

		io_flow_1->Serialize(xmlwriter);
		io_flow_2->Serialize(xmlwriter);

		xmlwriter.Write_close_tag();
		xmlwriter.Write_close_tag();
		xmlwriter.Close();
	}
	workload_defs_file.close();
	ssd_config_file.close();
}

void collect_results(Host_System& host)
{
	std::vector<Host_Components::IO_Flow_Base*> IO_flows = host.Get_io_flows();
	for (int stream_id = 0; stream_id < IO_flows.size(); stream_id++)
	{
		cout << "Flow " << IO_flows[0]->ID() << " - total requests generated: " << IO_flows[0]->Get_generated_request_count()
			<< " total requests serviced:" << IO_flows[0]->Get_serviced_request_count() << endl;
		cout << "                   - device response time: " << IO_flows[0]->Get_device_response_time() << " (us)"
			<< " end-to-end request delay:" << IO_flows[0]->Get_end_to_end_request_delay() << " (us)" << endl;
	}
	cin.get();
}

void print_help()
{
	cout << "MQSim - A simulator for modern NVMe and SATA SSDs developed at SAFARI group in ETH Zurich" << endl <<
		"Standalone Usage:" << endl <<
		"./MQSim [-i path/to/config/file] [-w path/to/workload/file]" << endl;
}

int main(int argc, char* argv[])
{
	string ssd_config_file_path, workload_defs_file_path;
	if (argc != 5)
	{
		// We expect 3 arguments: the program name, the source path and the destination path
		print_help();
		return 1;
	}

	command_line_args(argv, ssd_config_file_path, workload_defs_file_path);

	time_t start_time = time(0);
	char* dt = ctime(&start_time);
	cout << "MQSim started at " << dt << "......." << endl;
	
	Execution_Parameter_Set* exec_params = new Execution_Parameter_Set;
	read_parameters(ssd_config_file_path, workload_defs_file_path, exec_params);
	SSD_Device ssd(&exec_params->SSD_Device_Configuration, &exec_params->Host_Configuration.IO_Flow_Definitions);
	Host_System host(&exec_params->Host_Configuration, ssd.Host_interface);
	host.Attach_ssd_device(&ssd);
	ssd.Perform_preconditioning();

	Simulator->Start_simulation();

	time_t end_time = time(0);
	dt = ctime(&end_time);
	cout << "MQSim finished at " << dt << endl;
	uint64_t duration = (uint64_t)difftime(end_time, start_time);
	cout << "Total simulation time: " << duration / 3600 << ":" << (duration % 3600) / 60 << ":" << ((duration % 3600) % 60) << endl;
	
	collect_results(host);

	return 0;
}