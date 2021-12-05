#include <cstring>
#include "disassembler.h"
#include "output.h"

void check_files(int argc);

int main(int argc, char** argv){
    check_files(argc); 
    disassemble(argv[1], argv[2]);
   return 0;
}


/**
* Checks if the user input the correct amount of command arguments. The program prematurely
* terminates if the user fails to input the two required command line arguments.
* @param argc: The amount of command arguments, including the name of the .exe file.
*/ 
void check_files(int argc){
    if(argc == 3){
        return;
    } 
    else{
        std::cout << "ERROR: Too many or too few input files.";
        std::cout << "Restart the program and enter the TWO (2) required input files." << std::endl;
        std::cout << "The execution should be as follows: ./dissem test.obj test.sym" << std::endl;
        std::cout << "Program Terminated.\n" << std::endl;
        exit(0);
    }
}
