#ifndef NVME_DEFINITIONS_H
#define NVME_DEFINITIONS_H

#include <cstdint>
#include <string>
#include "Host_Interface_NVMe_Priorities.h"

enum class HostInterface_Types { SATA, NVME };

#define NVME_FLUSH_OPCODE 0x0000
#define NVME_WRITE_OPCODE 0x0001
#define NVME_READ_OPCODE 0x0002

#define SATA_WRITE_OPCODE 0x0001
#define SATA_READ_OPCODE 0x0002

const uint64_t NCQ_SUBMISSION_REGISTER = 0x1000;
const uint64_t NCQ_COMPLETION_REGISTER = 0x1003;
const uint64_t SUBMISSION_QUEUE_REGISTER_0 = 0x1000;
const uint64_t COMPLETION_QUEUE_REGISTER_0 = 0x1003;
const uint64_t SUBMISSION_QUEUE_REGISTER_1 = 0x1010;
const uint64_t COMPLETION_QUEUE_REGISTER_1 = 0x1013;
const uint64_t SUBMISSION_QUEUE_REGISTER_2 = 0x1020;
const uint64_t COMPLETION_QUEUE_REGISTER_2 = 0x1023;
const uint64_t SUBMISSION_QUEUE_REGISTER_3 = 0x1030;
const uint64_t COMPLETION_QUEUE_REGISTER_3 = 0x1033;
const uint64_t SUBMISSION_QUEUE_REGISTER_4 = 0x1040;
const uint64_t COMPLETION_QUEUE_REGISTER_4 = 0x1043;
const uint64_t SUBMISSION_QUEUE_REGISTER_5 = 0x1050;
const uint64_t COMPLETION_QUEUE_REGISTER_5 = 0x1053;
const uint64_t SUBMISSION_QUEUE_REGISTER_6 = 0x1060;
const uint64_t COMPLETION_QUEUE_REGISTER_6 = 0x1063;
const uint64_t SUBMISSION_QUEUE_REGISTER_7 = 0x1070;
const uint64_t COMPLETION_QUEUE_REGISTER_7 = 0x1073;
const uint64_t SUBMISSION_QUEUE_REGISTER_8 = 0x1080;
const uint64_t COMPLETION_QUEUE_REGISTER_8 = 0x1083;

struct Completion_Queue_Entry
{
	uint32_t Command_specific;
	uint32_t Reserved;
	uint16_t SQ_Head; //SQ Head Pointer, Indicates the current Submission Queue Head pointer for the Submission Queue indicated in the SQ Identifier field
	uint16_t SQ_ID;//SQ Identifier, Indicates the Submission Queue to which the associated command was issued to.
	uint16_t Command_Identifier;//Command Identifier, Indicates the identifier of the command that is being completed
	uint16_t SF_P; //Status Field (SF)+ Phase Tag(P)
				   //SF: Indicates status for the command that is being completed
				   //P:Identifies whether a Completion Queue entry is new
};

struct Submission_Queue_Entry
{
	uint8_t Opcode;//Is it a read or write request
	uint8_t PRP_FUSE;
	uint16_t Command_Identifier;//The id of the command in the I/O submission queue
	uint64_t Namespace_identifier;
	uint64_t Reserved;
	uint64_t Metadata_pointer_1;
	uint64_t PRP_entry_1;
	uint64_t PRP_entry_2;
	uint32_t Command_specific[6];
};

#endif // !NVME_DEFINISIONS_H
