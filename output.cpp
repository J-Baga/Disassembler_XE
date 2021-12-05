#include "output.h"
#include "disassembler.h"

void create_output(std::vector<int> programCounter, std::vector<std::string> labels, std::vector<std::string> opCodes, std::vector<std::string> operands, std::vector<std::string> objectCodes){
    std::ofstream outfile("out.lst");
    int addrLength = 4;
    for(int i = 0; i < programCounter.size(); i++){
        if(programCounter[i] > 65535){
           addrLength = 5; 
        }

        if(programCounter[i] == 32){ // Character representation of 32 is a blank space. This is used
                                     // for instructions that don't represent a program counter address (LTORG, BASE).
            outfile << std::setw(addrLength) << char(programCounter[i]);
            outfile << std::right << std::setw(10) << labels[i];
            outfile << std::right << std::setw(15) << opCodes[i];
            outfile << std::right << std::setw(15) << operands[i];
            outfile << std::right << std::setw(12) << objectCodes[i] << std::endl;
        }
        else{
            outfile << std::setw(addrLength) << std::setfill('0') << std::hex << std::uppercase << programCounter[i];
            outfile << std::right << std::setw(10) << std::setfill(' ') << labels[i];
            outfile << std::right << std::setw(15) << std::setfill(' ') << opCodes[i];
            outfile << std::right << std::setw(15) << std::setfill(' ') << operands[i];
            outfile << std::right << std::setw(12) << std::setfill(' ') << objectCodes[i] << std::endl;
        }
    }
    outfile.close();
}