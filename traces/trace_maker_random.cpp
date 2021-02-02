#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <time.h>

using namespace std;

const unsigned int sector_size_in_bytes = 512;
const unsigned int zone_size_MB = 256;
unsigned int smallest_zone_number = 2;
unsigned int biggest_zone_number;
unsigned int total_size_of_accessed_file_in_GB = 1;
unsigned int total_no_of_request = 10000;
unsigned int zone_number;

struct zone {
    unsigned int zone_ID;
    unsigned long long int start_LBA;
    unsigned long long int writepoint_LBA;
    bool available;
};

struct zone *zonelist;

bool all_zonelist_false()
{
    for (int k = 0; k < zone_number; k++) {
        if(zonelist[k].available == true)
            return false;
    }
    return true;   
}

int main(int argc, char** argv) {

    std::cout << "This generates a random write requests." << std::endl;

    unsigned int request_size_in_KB;
    cout << "Please enter the request size in KB : ";
    scanf("%u", &request_size_in_KB);

    cout << "Please enter the smallest zone number : ";
    scanf("%u", &smallest_zone_number);

    cout << "Please enter the whole file size to be accessed in GB : ";
    scanf("%u", &total_size_of_accessed_file_in_GB);

    string outputfile = "random_write_" + to_string(request_size_in_KB) + "KB_from_zone" \
                        + to_string(smallest_zone_number) + "_" + to_string(total_size_of_accessed_file_in_GB) + "GB";
    cout << "output file name is " << outputfile << endl;
    
    zone_number = total_size_of_accessed_file_in_GB * 1024 / 256;
    cout << "We are gonna access " << zone_number << " zones" << endl;

    zonelist = (struct zone*)malloc(sizeof(struct zone) * (zone_number+1));

    // zonelist initialization
    for (int i = 0; i < zone_number + 1; i++) {
        zonelist[i].zone_ID = smallest_zone_number + i;
        zonelist[i].start_LBA = zonelist[i].zone_ID * 1024 * 512; // 256 MB / 512 B
        cout << "zone" << zonelist[i].zone_ID << "'s start_LBA: " << zonelist[i].start_LBA << endl;
        zonelist[i].writepoint_LBA = zonelist[i].start_LBA;
        zonelist[i].available = true;
    }
    zonelist[zone_number].available = false;
    // The reason why the number of zonelist is zone_number+1 is for the checking zone fillup

    srand(time(NULL));

    ofstream writeFile;
    writeFile.open(outputfile);
    string trace_line = "";

   // cout << "request size is " << request_size_in_KB << " KB" << endl;
    total_no_of_request =  total_size_of_accessed_file_in_GB * 1024 * 1024 / request_size_in_KB;
    cout << "total number of requests is " << total_no_of_request << endl;

    unsigned long long int first_arrival_time = 4851300;

    string arrival_time;
    string start_LBA;
    string device_number;
    string request_size_in_sector;
    string type_of_request;

    unsigned long long int prev_arrival_time = first_arrival_time;
    unsigned long long int prev_start_LBA;
    int i, j;
    int zone_id;
    for (i = 0; i < total_no_of_request + 1000000; i++) {
        do {
            if (all_zonelist_false()) {
                cout << "Every zone is filled up!" << endl;
                writeFile.close();
                return 0;
            }
            zone_id = rand() % zone_number;
        } while (zonelist[zone_id].available == false);

        int access_count_in_one_zone = rand() % 50;
        prev_start_LBA = zonelist[zone_id].writepoint_LBA;

        for (j = 0; j < access_count_in_one_zone; j++, i++) {
            arrival_time = to_string(prev_arrival_time);
            start_LBA = to_string(prev_start_LBA);
            device_number = "1";
            request_size_in_sector = to_string(request_size_in_KB * 2); // 2 == 1024 / sector_size_in_bytes
            type_of_request = "0";   // 0 means that it's a write request, read request is "1"
            
            trace_line = arrival_time + " " + device_number + " " + start_LBA + " " + request_size_in_sector + " " + type_of_request + "\n";
            
            writeFile.write(trace_line.c_str(), trace_line.size());
            trace_line.clear();
            
            prev_arrival_time = prev_arrival_time + ((rand() % 15) * 1000);
            prev_start_LBA = prev_start_LBA + (request_size_in_KB * 2); // 2 == 1024 / sector_size_in_bytes
            zonelist[zone_id].writepoint_LBA = prev_start_LBA;
            if (zonelist[zone_id].writepoint_LBA >= zonelist[zone_id+1].start_LBA) {
                zonelist[zone_id].available = false;
                cout << "zone #" << zonelist[zone_id].zone_ID << " is filled up! no more write request!!" << endl;
                break;
            }
        }
    }
    writeFile.close();
    return 0;
}
