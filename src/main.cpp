#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <cstring>
#include "ssd/SSD_Defs.h"
#include "exec/Execution_Parameter_Set.h"
#include "exec/SSD_Device.h"
#include "exec/Host_System.h"
#include "utils/rapidxml/rapidxml.hpp"
#include "utils/DistributionTypes.h"

using namespace std;


void command_line_args(char* argv[], string& input_file_path, string& workload_file_path)
{

	for (int arg_cntr = 1; arg_cntr < 5; arg_cntr++) {
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

void read_configuration_parameters(const string ssd_config_file_path, Execution_Parameter_Set* exec_params)
{
	ifstream ssd_config_file;
	ssd_config_file.open(ssd_config_file_path.c_str());

	if (!ssd_config_file) {
		PRINT_MESSAGE("The specified SSD configuration file does not exist.")
		PRINT_MESSAGE("Using MQSim's default configuration.")
		PRINT_MESSAGE("Writing the default configuration parameters to the expected configuration file.")

		Utils::XmlWriter xmlwriter;
		string tmp;
		xmlwriter.Open(ssd_config_file_path.c_str());
		exec_params->XML_serialize(xmlwriter);
		xmlwriter.Close();
		PRINT_MESSAGE("[====================] Done!\n")
	} else {
		//Read input workload parameters
		string line((std::istreambuf_iterator<char>(ssd_config_file)),
			std::istreambuf_iterator<char>());
		ssd_config_file >> line;
		if (line.compare("USE_INTERNAL_PARAMS") != 0) {
			rapidxml::xml_document<> doc;    // character type defaults to char
			char* temp_string = new char[line.length() + 1];
			strcpy(temp_string, line.c_str());
			doc.parse<0>(temp_string);
			rapidxml::xml_node<> *mqsim_config = doc.first_node("Execution_Parameter_Set");
			if (mqsim_config != NULL) {
				exec_params = new Execution_Parameter_Set;
				exec_params->XML_deserialize(mqsim_config);
			} else {
				PRINT_MESSAGE("Error in the SSD configuration file!")
				PRINT_MESSAGE("Using MQSim's default configuration.")
			}
		} else {
			PRINT_MESSAGE("Using MQSim's default configuration.");
			PRINT_MESSAGE("Writing the default configuration parameters to the expected configuration file.");

			Utils::XmlWriter xmlwriter;
			string tmp;
			xmlwriter.Open(ssd_config_file_path.c_str());
			exec_params->XML_serialize(xmlwriter);
			xmlwriter.Close();
			PRINT_MESSAGE("[====================] Done!\n")
		}
	}

	ssd_config_file.close();
}

std::vector<std::vector<IO_Flow_Parameter_Set*>*>* read_workload_definitions(const string workload_defs_file_path)
{
	std::vector<std::vector<IO_Flow_Parameter_Set*>*>* io_scenarios = new std::vector<std::vector<IO_Flow_Parameter_Set*>*>;

	ifstream workload_defs_file;
	workload_defs_file.open(workload_defs_file_path.c_str());
	bool use_default_workloads = true;
	if (!workload_defs_file) {
		PRINT_MESSAGE("The specified workload definition file does not exist!");
		PRINT_MESSAGE("Using MQSim's default workload definitions.");
		PRINT_MESSAGE("Writing the default workload definitions to the expected workload definition file.");
		PRINT_MESSAGE("[====================] Done!\n");
	} else {
		string line((std::istreambuf_iterator<char>(workload_defs_file)), std::istreambuf_iterator<char>());
		if (line.compare("USE_INTERNAL_PARAMS") != 0) {
			rapidxml::xml_document<> doc;
			// character type defaults to char
			char* temp_string = new char[line.length() + 1];
			strcpy(temp_string, line.c_str());
			doc.parse<0>(temp_string);
			rapidxml::xml_node<> *mqsim_io_scenarios = doc.first_node("MQSim_IO_Scenarios");
			if (mqsim_io_scenarios != NULL) {
				for (auto xml_io_scenario = mqsim_io_scenarios->first_node("IO_Scenario"); xml_io_scenario; xml_io_scenario = xml_io_scenario->next_sibling("IO_Scenario")) {
					std::vector<IO_Flow_Parameter_Set*>* scenario_definition = new std::vector<IO_Flow_Parameter_Set*>;
					for (auto flow_def = xml_io_scenario->first_node(); flow_def; flow_def = flow_def->next_sibling()) {
						IO_Flow_Parameter_Set* flow;
						if (strcmp(flow_def->name(), "IO_Flow_Parameter_Set_Synthetic") == 0) {
							flow = new IO_Flow_Parameter_Set_Synthetic;
							((IO_Flow_Parameter_Set_Synthetic*)flow)->XML_deserialize(flow_def);
						} else if (strcmp(flow_def->name(), "IO_Flow_Parameter_Set_Trace_Based") == 0) {
							flow = new IO_Flow_Parameter_Set_Trace_Based;
							((IO_Flow_Parameter_Set_Trace_Based*)flow)->XML_deserialize(flow_def);
						}
						scenario_definition->push_back(flow);
					}
					io_scenarios->push_back(scenario_definition);
					use_default_workloads = false;
				}
			} else {
				PRINT_MESSAGE("Error in the workload definition file!");
				PRINT_MESSAGE("Using MQSim's default workload definitions.");
				PRINT_MESSAGE("Writing the default workload definitions to the expected workload definition file.");
				PRINT_MESSAGE("[====================] Done!\n");
			}
		}
	}

	if (use_default_workloads) {
		std::vector<IO_Flow_Parameter_Set*>* scenario_definition = new std::vector<IO_Flow_Parameter_Set*>;
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
		io_flow_1->Initial_Occupancy_Percentage = 50;
		io_flow_1->Working_Set_Percentage = 85;
		io_flow_1->Synthetic_Generator_Type = Utils::Request_Generator_Type::QUEUE_DEPTH;
		io_flow_1->Read_Percentage = 100;
		io_flow_1->Address_Distribution = Utils::Address_Distribution_Type::RANDOM_UNIFORM;
		io_flow_1->Percentage_of_Hot_Region = 0;
		io_flow_1->Generated_Aligned_Addresses = true;
		io_flow_1->Address_Alignment_Unit = 16;
		io_flow_1->Request_Size_Distribution = Utils::Request_Size_Distribution_Type::FIXED;
		io_flow_1->Average_Request_Size = 8;
		io_flow_1->Variance_Request_Size = 0;
		io_flow_1->Seed = 12344;
		io_flow_1->Average_No_of_Reqs_in_Queue = 2;
		io_flow_1->Bandwidth = 262144;
		io_flow_1->Stop_Time = 1000000000;
		io_flow_1->Total_Requests_To_Generate = 0;
		scenario_definition->push_back(io_flow_1);

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
		io_flow_2->Initial_Occupancy_Percentage = 50;
		io_flow_2->Working_Set_Percentage = 85;
		io_flow_2->Synthetic_Generator_Type = Utils::Request_Generator_Type::QUEUE_DEPTH;
		io_flow_2->Read_Percentage = 100;
		io_flow_2->Address_Distribution = Utils::Address_Distribution_Type::RANDOM_UNIFORM;
		io_flow_2->Percentage_of_Hot_Region = 0;
		io_flow_2->Generated_Aligned_Addresses = true;
		io_flow_2->Address_Alignment_Unit = 16;
		io_flow_2->Request_Size_Distribution = Utils::Request_Size_Distribution_Type::FIXED;
		io_flow_2->Average_Request_Size = 8;
		io_flow_2->Variance_Request_Size = 0;
		io_flow_2->Seed = 6533;
		io_flow_2->Average_No_of_Reqs_in_Queue = 2;
		io_flow_2->Bandwidth = 131072;
		io_flow_2->Stop_Time = 1000000000;
		io_flow_2->Total_Requests_To_Generate = 0;
		scenario_definition->push_back(io_flow_2);

		io_scenarios->push_back(scenario_definition);

		PRINT_MESSAGE("Writing default workload parameters to the expected input file.")

		Utils::XmlWriter xmlwriter;
		string tmp;
		xmlwriter.Open(workload_defs_file_path.c_str());
		tmp = "MQSim_IO_Scenarios";
		xmlwriter.Write_open_tag(tmp);
		tmp = "IO_Scenario";
		xmlwriter.Write_open_tag(tmp);

		io_flow_1->XML_serialize(xmlwriter);
		io_flow_2->XML_serialize(xmlwriter);

		xmlwriter.Write_close_tag();
		xmlwriter.Write_close_tag();
		xmlwriter.Close();
	}
	workload_defs_file.close();

	return io_scenarios;
}

void collect_results(SSD_Device& ssd, Host_System& host, const char* output_file_path)
{
	Utils::XmlWriter xmlwriter;
	xmlwriter.Open(output_file_path);

	std::string tmp("MQSim_Results");
	xmlwriter.Write_open_tag(tmp);
	
	host.Report_results_in_XML("", xmlwriter);
	ssd.Report_results_in_XML("", xmlwriter);

	xmlwriter.Write_close_tag();

	std::vector<Host_Components::IO_Flow_Base*> IO_flows = host.Get_io_flows();
	for (unsigned int stream_id = 0; stream_id < IO_flows.size(); stream_id++) {
		cout << "Flow " << IO_flows[stream_id]->ID() << " - total requests generated: " << IO_flows[stream_id]->Get_generated_request_count()
			<< " total requests serviced:" << IO_flows[stream_id]->Get_serviced_request_count() << endl;
		cout << "                   - device response time: " << IO_flows[stream_id]->Get_device_response_time() << " (us)"
			<< " end-to-end request delay:" << IO_flows[stream_id]->Get_end_to_end_request_delay() << " (us)" << endl;
	}
}

void print_help()
{
	cout << "MQSim - SSD simulator with both NVMe and SATA host interface behavior, see ReadMe.md for details" << endl <<
		"Standalone Usage:" << endl <<
		"./MQSim [-i path/to/config/file] [-w path/to/workload/file]" << endl;
}

int main(int argc, char* argv[])
{
	string ssd_config_file_path, workload_defs_file_path;
	if (argc != 5) {
		// MQSim expects 2 arguments: 1) the path to the SSD configuration definition file, and 2) the path to the workload definition file
		print_help();
		return 1;
	}

	command_line_args(argv, ssd_config_file_path, workload_defs_file_path);

	Execution_Parameter_Set* exec_params = new Execution_Parameter_Set;
	read_configuration_parameters(ssd_config_file_path, exec_params);
	std::vector<std::vector<IO_Flow_Parameter_Set*>*>* io_scenarios = read_workload_definitions(workload_defs_file_path);

	int cntr = 1;
	for (auto io_scen = io_scenarios->begin(); io_scen != io_scenarios->end(); io_scen++, cntr++) {
		time_t start_time = time(0);
		char* dt = ctime(&start_time);
		PRINT_MESSAGE("MQSim started at " << dt)
		PRINT_MESSAGE("******************************")
		PRINT_MESSAGE("Executing scenario " << cntr << " out of " << io_scenarios->size() << " .......")

		//The simulator should always be reset, before starting the actual simulation
		Simulator->Reset();

		exec_params->Host_Configuration.IO_Flow_Definitions.clear();
		for (auto io_flow_def = (*io_scen)->begin(); io_flow_def != (*io_scen)->end(); io_flow_def++) {
			exec_params->Host_Configuration.IO_Flow_Definitions.push_back(*io_flow_def);
		}

		SSD_Device ssd(&exec_params->SSD_Device_Configuration, &exec_params->Host_Configuration.IO_Flow_Definitions);//Create SSD_Device based on the specified parameters
		exec_params->Host_Configuration.Input_file_path = workload_defs_file_path.substr(0, workload_defs_file_path.find_last_of("."));//Create Host_System based on the specified parameters
		Host_System host(&exec_params->Host_Configuration, exec_params->SSD_Device_Configuration.Enabled_Preconditioning, ssd.Host_interface);
		host.Attach_ssd_device(&ssd);

		Simulator->Start_simulation();

		time_t end_time = time(0);
		dt = ctime(&end_time);
		PRINT_MESSAGE("MQSim finished at " << dt)
		uint64_t duration = (uint64_t)difftime(end_time, start_time);
		PRINT_MESSAGE("Total simulation time: " << duration / 3600 << ":" << (duration % 3600) / 60 << ":" << ((duration % 3600) % 60))
		PRINT_MESSAGE("");

		PRINT_MESSAGE("Writing results to output file .......");
		collect_results(ssd, host, (workload_defs_file_path.substr(0, workload_defs_file_path.find_last_of(".")) + "_scenario_" + std::to_string(cntr) + ".xml").c_str());
	}
    cout << "Simulation complete; Press any key to exit." << endl;

	cin.get(); // Disable if you prefer batch runs

	return 0;
}