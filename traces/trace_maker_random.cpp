#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <time.h>

using namespace std;

unsigned int sector_size_in_bytes = 512;
unsigned int zone_size_MB = 256;
unsigned int smallest_zone_number = 2;
unsigned int biggest_zone_number = 6;
unsigned int total_no_of_request = 10000;

unsigned int zone_number = biggest_zone_number - smallest_zone_number + 1;

struct zone {
    unsigned int start_LBA;
    unsigned int writepoint_LBA;
    bool available;
};

struct zone *zonelist = (struct zone*)malloc(sizeof(struct zone)*zone_number);

bool all_zonelist_false()
{
    for (int i = 0; i < zone_number; i++) {
        if(zonelist[i].available == true)
            return false;
    }
    return true;   
}

int main(int argc, char** argv) {

    std::cout << "This generates a random write requests." << std::endl;

    if (argc < 3) {
        cout << "please, name the out file, \"./a.out <request size in KB> <output file name>\"" << endl;
        return 1;
    }
    string outputfile = argv[2];
    string request_size = argv[1];
    for (int i = 0; i < zone_number; i++) {
        zonelist[i].start_LBA = (smallest_zone_number + i) * 256 * 1024 * 1024 / 512;
        zonelist[i].writepoint_LBA = zonelist[i].start_LBA;
        zonelist[i].available = true;
    }

    srand(time(NULL));

    ofstream writeFile;
    writeFile.open(outputfile);
    string trace_line = "";

    unsigned int request_size_in_KB = stoi(request_size);
    cout << "request size is " << request_size_in_KB << " KB" << endl;
    unsigned int first_arrival_time = 48513000;
    unsigned long long int first_start_LBA = smallest_zone_number * 256 * 1024 * 1024 / 512;

    string arrival_time;
    string start_LBA;
    string device_number;
    string request_size_in_sector;
    string type_of_request;

    unsigned int prev_arrival_time = first_arrival_time;
    unsigned long long int prev_start_LBA; // = zonelist[3].start_LBA;
    for (int i = 0; i < total_no_of_request; i++) {
        int zone_id;
        while (zone_id = rand() % zone_number + smallest_zone_number) {
            if(zonelist[zone_id].available == true)
                break; 
            else if (all_zonelist_false()) {
                cout << "Every zone is filled up" << endl;
                writeFile.close();
                exit(0);
            }
        }

        int access_count_in_one_zone = rand() % 100;
        prev_start_LBA = zonelist[zone_id].writepoint_LBA;

        for (int j = 0; j < access_count_in_one_zone; j++) {
            arrival_time = to_string(prev_arrival_time);
            start_LBA = to_string(prev_start_LBA);
            device_number = "1";
            request_size_in_sector = to_string(request_size_in_KB * 2); // 2 == 1024 / sector_size_in_bytes
            type_of_request = "0";   // 0 means that it's a write request, read request is "1"
            
            trace_line = arrival_time + " " + device_number + " " + start_LBA + " " + request_size_in_sector + " " + type_of_request + "\n";
            
            writeFile.write(trace_line.c_str(), trace_line.size());
            trace_line.clear();
            
            sscanf(arrival_time.c_str(), "%d", &prev_arrival_time); 
            sscanf(start_LBA.c_str(), "%llu", &prev_start_LBA); 
            prev_arrival_time = prev_arrival_time + ((rand() % 15) * 1000);
            prev_start_LBA = prev_start_LBA + request_size_in_KB * 1024;
            zonelist[zone_id].writepoint_LBA = prev_start_LBA;
            if (zonelist[zone_id].writepoint_LBA >= zonelist[zone_id+1].start_LBA) {
                zonelist[zone_id].available = false;
                break;
            }
            i++;
        }
    }
    writeFile.close();
    return 0;
}
