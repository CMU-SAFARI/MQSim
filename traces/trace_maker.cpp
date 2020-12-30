#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <time.h>

using namespace std;

unsigned int sector_size_in_bytes = 512;
unsigned int zone_size_MB = 256;
int sequential = 1;     // 1 means it is sequential 
unsigned int smallest_zone_number = 2;
unsigned int biggest_zone_number = 6;

int main(int argc, char** argv) {

    if (argc < 2) {
        cout << "please, name the out file, \"./a.out <output file name>\"" << endl;
        return 1;
    }
    string outputfile = argv[1];
    srand(time(NULL));

    ofstream writeFile;
    writeFile.open(outputfile);
    string trace_line = "";

    unsigned int request_size_in_KB = 128;
    unsigned int first_arrival_time = 48513000;
    unsigned long long int first_start_LBA = smallest_zone_number * 256 * 1024 * 1024 / 512;

    string arrival_time;
    string start_LBA;
    string device_number;
    string request_size_in_sector;
    string type_of_request;

    unsigned int prev_arrival_time = first_arrival_time;
    unsigned long long int prev_start_LBA = first_start_LBA;
    for (int i = 0; i < 10000; i++) {
        arrival_time = to_string(prev_arrival_time);
        start_LBA = to_string(prev_start_LBA);
        device_number = "1";
        request_size_in_sector = to_string(request_size_in_KB * 1024 / sector_size_in_bytes);
        type_of_request = "0";   // 0 means write request
        
        trace_line = arrival_time + " " + device_number + " " + start_LBA + " " + request_size_in_sector + " " + type_of_request + "\n";
        
        writeFile.write(trace_line.c_str(), trace_line.size());
        sscanf(arrival_time.c_str(), "%d", &prev_arrival_time); //prev_arrival_time = stoi(arrival_time.c_str());
        sscanf(start_LBA.c_str(), "%llu", &prev_start_LBA); //prev_start_LBA = stoi(start_LBA.c_str());
        trace_line.clear();

        prev_arrival_time = prev_arrival_time + ((rand() % 15) * 1000);
        prev_start_LBA = prev_start_LBA + request_size_in_KB * 1024;
    }
    writeFile.close();
    return 0;
}