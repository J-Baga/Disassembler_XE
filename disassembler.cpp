#include "disassembler.h"
#include "output.h"

const static std::string g_ops[] = {                // Data structure provided by prompt. Program does not
"18", "58", "90", "40", "B4", "28",                 // support Opcodes that represent Format 1 instructions.
"88", "A0", "24", "64", "9C", "C4",
"C0", "F4", "3C", "30", "34", "38",
"48", "00", "68", "50", "70", "08",
"6C", "74", "04", "D0", "20", "60",
"98", "C8", "44", "D8", "AC", "4C",
"A4", "A8", "F0", "EC", "0C", "78",
"54", "80", "D4", "14", "7C", "E8",
"84", "10", "1C", "5C", "94", "B0",
"E0", "F8", "2C", "B8", "DC"
};

const static std::string g_mnemonics[] = {          // Data structure provided by prompt. Program does not
"ADD", "ADDF", "ADDR", "AND", "CLEAR", "COMP",      // support Format 1 one instructions.
"COMPF", "COMPR", "DIV", "DIVF", "DIVR", "FIX",
"FLOAT", "HIO", "J", "JEQ", "JGT", "JLT",
"JSUB", "LDA", "LDB", "LDCH", "LDF", "LDL",
"LDS", "LDT", "LDX", "LPS", "MUL", "MULF",
"MULR", "NORM", "OR", "RD", "RMO", "RSUB",
"SHIFTL", "SHIFTR", "SIO", "SSK", "STA", "STB",
"STCH", "STF", "STI", "STL","STS", "STSW",
"STT", "STX", "SUB", "SUBF", "SUBR", "SVC",
"TD", "TIO", "TIX", "TIXR", "WD"
};

const static std::string g_registers[7] = {"A", "X", "L", "B", "S", "T", "F"};
int g_registerValues[7] = {0,0,0,0,0,0,0};

static std::vector<std::string> g_records;
static std::map <int, std::string> g_symMap;

static std::vector<int> g_programCounter;
static std::vector<std::string> g_labels;
static std::vector<std::string> g_opCodes;
static std::vector<std::string> g_operands;
static std::vector<std::string> g_objectCodes;

/**
* Converts a given object file and its symbol table into assembly language. 
* Goes through each object code and deciphers and stores the necessary information
* for each line. Additional functionalties are included to account for "non-standard"
* cases such as LTORG and BASE directives, CLEAR, LOAD, and RSUB instructions, and literals.
* @param objFile: The object file.
* @param symTab: The symbol file.
*/ 
void disassemble(std::string objFile, std::string symTab){
    parse_obj(objFile);
    parse_sym(symTab);
    int currAddr = std::stoi(g_records[0].substr(7,6), nullptr, 16);    // Starting address of the object file.
    g_programCounter.push_back(currAddr);
    g_labels.push_back(g_records[0].substr(1,6));                       // Name of the program.
    g_opCodes.push_back("START");                                       
    if(currAddr == 0){                                                  
        g_operands.push_back("0");
    }
    else{                                                               // Format issues fixed if starting address
        std::string startOperand = g_records[0].substr(7,6);            // isn't 0.
        startOperand.erase(0, startOperand.find_first_not_of('0'));
        g_operands.push_back(startOperand);
    }
    g_objectCodes.push_back("");
    
    for(int i = 0; i < g_records.size(); i++){                          // Iteration through each text record.
        if(g_records[i].find("T") != 0){
            continue;
        }
        currAddr = stoi(g_records[i].substr(1,6), nullptr, 16);         // Starting address of the text record.

        for(int j = 9; j < g_records[i].length() - 3;){                 // Iteration through object codes up to 
                                                                        // the last possible object code to extract
                                                                        // three hexadecimals (excluding literals)
            if((g_symMap.find(currAddr)->second).find("=") == 0){       // Checking for literals.
                add_LTORG();
                while((g_symMap.find(currAddr)->second).find("=") == 0){// Accounts for literal(s) being called
                    std::string lit = g_symMap.find(currAddr)->second;  // before the LTORG directive was used.
                    int length;
                    if(lit.find("=X") == 0){                            // Length of literal's object code differs
                        length = lit.rfind("'") - 3;                    // depending on the type of literal 
                    }                                                   // (X for Hexadecimal, C for Character).
                    else if(lit.find("=C") == 0){
                        length = (lit.rfind("'") - 3) * 2;
                    }
                    int bytes = length / 2;
                    std::string litObjCode = g_records[i].substr(j, length);
                    add_literal(lit, litObjCode, currAddr);
                    currAddr = currAddr + bytes;
                    j = j + length;
                } 
                continue;
            }

            std::string firstHex = g_records[i].substr(j, 1);
            std::string secondHex = g_records[i].substr(j + 1, 1);
            std::string thirdHex = g_records[i].substr(j + 2, 1);
            std::string mnemonic = get_mnemonic(firstHex, secondHex);
            int format = get_format(mnemonic, thirdHex);
            std::string objCode = g_records[i].substr(j, format * 2);
            int targetAddr = get_TA(format, currAddr, objCode, addr_mode_TA(thirdHex), is_indexed(thirdHex));
            std::string operand = get_operand(targetAddr, format, mnemonic, objCode, addr_mode_OP(secondHex), 
                                              is_indexed(thirdHex));

            g_programCounter.push_back(currAddr);
            add_label(currAddr);

            if(mnemonic.find("LD") == 0){                               // Checking for specific instructions.
                load_reg(targetAddr, format, mnemonic, objCode);
            }
            else if(mnemonic == "CLEAR"){
                clear_reg(objCode);
            }

            if(format == 4){
                mnemonic = "+" + mnemonic;
            }

            g_opCodes.push_back(mnemonic);

            if(mnemonic != "RSUB"){
                g_operands.push_back(operand);
            }
            else{                                                       // The RSUB instruction does NOT
                g_operands.push_back("");                               // have an operand.
            }

            g_objectCodes.push_back(objCode);

            if(get_mnemonic(firstHex, secondHex) == "LDB"){             // The instruction is LOAD BASE.
                g_programCounter.push_back(32);                         // See output.cpp.
                g_labels.push_back("");
                g_opCodes.push_back("BASE");
                g_operands.push_back(g_symMap.find(targetAddr)->second);
                g_objectCodes.push_back("");
            }

            j = j + (format * 2);                                       // Next iteration through the current text    
                                                                        // record starts at the first hexadecimal
                                                                        // of the next object code.
            currAddr = currAddr + format;                                                        
        }
        fill_gap(currAddr, i);
    }
    g_programCounter.push_back(32);                                     
    g_labels.push_back("");
    g_opCodes.push_back("END");
    g_operands.push_back(g_symMap.find(stoi(g_records[g_records.size() - 1].substr(1,6) , nullptr, 16))->second);
    g_objectCodes.push_back("");

    create_output(g_programCounter, g_labels, g_opCodes, g_operands, g_objectCodes);

    g_programCounter.clear();
    g_labels.clear();
    g_opCodes.clear();
    g_operands.clear();
    g_objectCodes.clear();

    return;
}


/**
* Parses the object code file and stores each individual record into a vector.
* @param objFile: The file containing the object code.
* @return A vector containing the object code, parsed line by line into distinct index positions.
*/ 
void parse_obj(std::string objFile){
    std::ifstream textFile(objFile);
    std::string line;

    while(std::getline(textFile, line)){
        g_records.push_back(line);
    }
}


/**
* Parses the symbol file and stores the symbols and their addresses as keys and values respectively into a map.
* Any other lines and information will be ignored (Addressing type, lines without important information).
* @param symFile: The file containing the symbols and their information.
*/ 
void parse_sym(std::string symFile){
    std::ifstream symbols(symFile);
    std::string line;
    std::string token1;
    std::string token2;
    std::string token3;

    while(std::getline(symbols, line)){
        if(line.find("Symbol") == 0 || line.find("-") == 0 || line.find("Name") == 0 || line.length() == 0){
            // Current line has no relevant information.
            continue;
        }
        else{
            std::istringstream symLine(line);
            symLine >> token1 >> token2 >> token3;
            if(token3.find("R") == 0 || token3.find("A") == 0) {
                // Current line has information about symbols.
                g_symMap.insert(std::pair<int, std::string>(stoi(token2, nullptr, 16), token1));
            }
            else{
                // Current line has information about literals.
                g_symMap.insert(std::pair<int, std::string>(stoi(token3, nullptr, 16), token1)); 
            }
        }
    }
}


/**
* Determines and returns the object code's corresponding mnemonic. A pattern
* of omitting the last two bits in the second hexadecimal is used to determine 
* the true opCode to find the correct mnemonic.
* @param firstHex: The first hexadecimal of the object code.
* @param secondHex: The second hexadecimal of the obejct code.
* @return The object code's corresponding mnemonic.
*/
std::string get_mnemonic(std::string firstHex, std::string secondHex){
    std::string opCode;
    int opIndex = 0;
    if(secondHex == "0" || secondHex == "1" || secondHex == "2" || secondHex == "3"){
        opCode = firstHex + "0";
    }
    else if(secondHex == "4" || secondHex == "5" || secondHex == "6" || secondHex == "7"){
        opCode = firstHex + "4";
    }
    else if(secondHex == "8" || secondHex == "9" || secondHex == "A" || secondHex == "B"){
        opCode = firstHex + "8";
    }
    else if(secondHex == "C" || secondHex == "D" || secondHex == "E" || secondHex == "F"){
        opCode = firstHex + "C";
    }

    for(int i = 0; i < sizeof(g_ops); i++){
        if(g_ops[i] == opCode){
            opIndex = i;
            break;
        }
    }

    return g_mnemonics[opIndex];    
}


/**
* Determines and returns the format of the object code. Excludes format 1 instructions.
* @param mnemonic: The object code's corresponding instruction.
* @param thirdHex: The third hexadecimal of the object code.
* @return The format of the object code.
*/ 
int get_format(std::string mnemonic, std::string thirdHex){
    int format = 0;
    if(mnemonic == "ADDR" || mnemonic == "CLEAR" || mnemonic == "COMPR" || mnemonic == "MULR" || 
       mnemonic == "RMO" || mnemonic == "SHIFTL" || mnemonic == "SHIFTR" || mnemonic == "SUBR" ||
       mnemonic == "SUBR" || mnemonic == "SVC" || mnemonic == "TIXR"){ //Every mnemonic associated with format 2.

       format = 2;    
    }
    else if(thirdHex == "0" || thirdHex == "2" || thirdHex == "4" || thirdHex == "6" || thirdHex == "8" ||
            thirdHex == "A" || thirdHex == "C" || thirdHex == "E"){    // flag bit e = 0

       format = 3;
    }
    else{ // Hexadecimals not listed above have a 'e' flag bit value of 1, which indicates a format 4 instruction.
        format = 4;
    }
    return format;
}


/**
* Determines and returns the addressing mode of for the Operand Value.
* @param secondHex: The second hexadecimal of the object code. Contains the n and i bits required
*                   to calculate this specific addressing mode.
* @return The addressing mode for the Operand Value.
*/ 
std::string addr_mode_OP(std::string secondHex){
    std::string addrOp;
    if(secondHex == "1" || secondHex == "5" || secondHex == "9" || secondHex == "D"){      // flag bits i = 1, n = 0
        addrOp = "Immediate";
    }
    else if(secondHex == "2" || secondHex == "6" || secondHex == "A" || secondHex == "E"){ // flag bits i = 0, n = 1
        addrOp = "Indirect";
    }
    else{ 
        addrOp = "Simple";
    }
    return addrOp;
}


/**
* Determines and returns the addressing mode for calculating the Target Address.
* The pattern of bits provided by the third hexadecimal determines which mode.
* @param thirdHex: The third hexadecimal of the object code. Contains the b and p bits required
*                  to calculate this specific addressing mode.
* @return The addressing mode of for calculating the Target Address.
*/ 
std::string addr_mode_TA(std::string thirdHex){
    std::string addrTA;
    if(thirdHex == "2" || thirdHex == "3" || thirdHex == "A" || thirdHex == "B"){      // flag bits b = 0, p = 1
        addrTA = "PC-Relative";
    }
    else if(thirdHex == "4" || thirdHex == "5" || thirdHex == "C" || thirdHex == "D"){ // flag bits b = 1, p = 0
        addrTA = "Base Relative";
    }
    else if(thirdHex == "0" || thirdHex == "1" || thirdHex == "8" || thirdHex == "9"){ // flat bits b = 0, p = 0 
        addrTA = "Direct";
    }
    return addrTA;
}


/**
* Determines if the addressing mode to calculate the Target Address is also Indexed.
* @param thirdHex: The third hexadecimal of the object code. Contains the x bit required
*                  to calculate this specific addressing mode.
* @return True: The addressing mode to calculate the Target Address is also Indexed.
*         False: The addressing mode to calculate the Target Address is not Indexed.
*/ 
bool is_indexed(std::string thirdHex){
    if(thirdHex == "A" || thirdHex == "B" || thirdHex == "C" || thirdHex == "D" || thirdHex == "8" || thirdHex == "9"){
        // flag bit x = 1
        return true;
    }
    else{
        return false;
    }
}


/**
* Calculates and returns the Target Address based on the specific conditions of the given parameters.
* @param format: The format of the object code instruction.
* @param locAddr: The address of where the instruction is located.
* @param objCode: The object code.
* @param addrTA: The addressing mode of how to calculate the Target Address.
* @param isIndex: Provides whether Indexed addressing must be accounted for or not.
* @return The Target Address of the object code.
*/ 
int get_TA(int format, int locAddr, std::string objCode, std::string addrTA, bool isIndex){
    int taADDR = 0;
    int dispOrAddr;
    if(format == 2){                                                // Format 2 instructions don't have TA's.
        return 0;
    }
    else if(format == 3){                                           // Extracting disp/Addr section from object code.
        dispOrAddr = std::stoi(objCode.substr(3,3), nullptr, 16);
        if(dispOrAddr > 2047){                                      // stoi() function always returns unsigned 
            dispOrAddr = dispOrAddr - 4096;                         // hexadecimal value in decimal. Any decimals 
        }                                                           // not within [-2048, 2047] must be subtracted by
    }                                                               // 4096 to store the correct, signed value.
    else if(format == 4){
        dispOrAddr = std::stoi(objCode.substr(3,5), nullptr, 16);
    }
    
    if(addrTA == "PC-Relative"){                                    // Altering TA value depending on its addressing mode.
        taADDR = dispOrAddr + (locAddr + format);
    }
    else if(addrTA == "Base Relative"){
        taADDR = dispOrAddr + g_registerValues[3];
    }
    else if(addrTA == "Direct"){
        taADDR = dispOrAddr;
    }

    if(isIndex){                                                    // Adds the current value stored in the X register.
        taADDR = taADDR + g_registerValues[1];
    }

    return taADDR;
}


/**
* Determines and returns the operand of the current object code being analyzed based on the given parameters.
* @param targetAddr: The Target Address of the object code.
* @param format: The format of the instruction.
* @param mnemonic: The object code's corresponding instruction.
* @param objCode: The current object code being analyzed.
* @param addrOP: The addressing mode for the Operand Value.
* @param isIndexed: Helps determine whether the value of the register will be added to the Target Address or not.
* @return The operand of the object code.
*/ 
std::string get_operand(int targetAddr, int format, std::string mnemonic, std::string objCode, std::string addrOP, 
                        bool isIndex){
    std::string operand;
    if(format == 2){                                                    // Operand is a register. Program accounts
        operand = g_registers[stoi(objCode.substr(2,1), nullptr, 16)];  // for CLEAR instructions only.
        return operand;     
    }
    
    if(format == 3 && three_bits_zero(objCode.substr(2,1))){            // Operand is a constant.
        int constant = stoi(objCode.substr(3,3), nullptr, 16);
        operand = std::to_string(constant);
    }
    else if(format == 4 && three_bits_zero(objCode.substr(2,1))){
        int constant = stoi(objCode.substr(3,5), nullptr, 16);
        operand = std::to_string(constant);
    }    
    else{                                                               // Operand is a symbol.
        operand = (g_symMap.find(targetAddr)->second);
    }

    if(addrOP == "Immediate"){
        operand = "#" + operand;
    }
    else if(addrOP == "Indirect"){
        operand = "@" + operand;
    }
    else if(addrOP == "Simple"){
        operand = operand;
    }

    if(isIndex){
        operand = operand + ",X";
    }

    return operand;
}


/**
* Determines whether the last three bits in the third hexadecimal of the object code are zeros or not.
* The last three bits in a the third hexadecimal indicates that the operand is a constant. See get_operand().
* @param thirdHex: The third hexadecimal of the current object code being analyzed.
* @return True: The last three bits in the hexadecimal are zero.
          False: The last three bits in the hexadecimal are not all zero.
*/ 
bool three_bits_zero(std::string thirdHex){
    if(thirdHex == "0" || thirdHex == "8"){ // Only hexadecimals where flag bits b, p and e are all zero.
        return true;
    }
    else{
        return false;
    }
}


/**
* Adds a symbol to the list of labels if the current address matches the address where that symbol 
* is located. If there the current address does not satisfy that condition, there is no label
* for that specific line of instruction.
* @param currAddr: The address of the to be disassembled object code.
*/ 
void add_label(int currAddr){
   if(g_symMap.find(currAddr) != g_symMap.end()){ // The current address is associated with a symbol.
        g_labels.push_back((g_symMap.find(currAddr)->second));
    }
    else{
        g_labels.push_back("");
    } 
}


/**
* Inserts lines of instruction for symbols whose addresses are not within the range of a text record in respect
* to both a text records specified length and the length of the entire program in bytes. The range of which
* symbols and their address will be added during a specific calling of this function is explained in the
* find_addr_gap_matches() function. RESW instructions will be used to reserve the number of bytes it takes to
* get from one address to the next. This is repeated until all symbols within that range have been created a
* line of instruction.
* @param endAddr: The ending address of a text record.
* @param currItr: The current iteration through the text records.
*/ 
void fill_gap(int endAddr, int currItr){
    std::vector<int> matches = find_addr_gap_matches(endAddr, currItr);
    int currAddr = endAddr;
    for(int i = 0; i < matches.size() - 1; i++){ 
        g_programCounter.push_back(currAddr);           
        add_label(currAddr);                            // Each of the found addresses is associated with a label.
        g_opCodes.push_back("RESW");
        int numBytes = (matches[i + 1] - currAddr) / 3; // The number of bytes to reserve.
        g_operands.push_back(std::to_string(numBytes));
        g_objectCodes.push_back("");                    
        currAddr = matches[i + 1];                      // Update the address to the next one in the vector.
    } 
}


/**
* Finds, stores, and returns the addresses of symbols not within the range of a text record in respect to
* its specified length in bytes. Both cases where symbols with addresses between text records and symbols
* with addresses between the last text record and the entire length of the program are accounted for with
* both having different matching conditions.
* @param currAddr: The address the text record leaves off on.
* @param currItr: The current iteration through the object code records.
*/ 
std::vector<int> find_addr_gap_matches(int currAddr, int currItr){
    std::vector<int> matches;

    if(g_records[currItr + 1].find("T") == 0){ // Next record is a text record.
       for(const auto& pair : g_symMap){       // Iteration through the map.
            if(pair.first >= currAddr && pair.first < std::stoi(g_records[currItr + 1].substr(1,6), nullptr, 16)){
                matches.push_back(pair.first);
            } 
        }
        matches.push_back(std::stoi(g_records[currItr + 1].substr(1,6), nullptr, 16));  
    }
    else{                                      // Current record is the last text record.
        for(const auto& pair : g_symMap){ 
            if(pair.first >= currAddr && pair.first < std::stoi(g_records[0].substr(13,6), nullptr, 16)){
                matches.push_back(pair.first);
            } 
        }
        matches.push_back(std::stoi(g_records[0].substr(13,6), nullptr, 16)); // The length of the program in bytes,
    }                                                                         // extracted from the Header record.

    return matches;
}


/**
* Loads a specific value into a specific register when a load instruction is detected.
* @param targetAddr: The target address of the load instruction.
* @param format: The format of the load instruction. It can be either only format 3 or 4.
* @param mnemonic: The instruction mnemonic. Used to determine what register is being addressed.
* @param objCode: The object code of the instruction. 
*/  
void load_reg(int targetAddr, int format, std::string mnemonic, std::string objCode){
    std::string registerName = mnemonic.substr(2,1);
    int regValIdx = -1;
    for(int i = 0; i < sizeof(g_registers)/sizeof(g_registers[0]); i++){ // Match register name to its value index.
        if(registerName == g_registers[i]){
            regValIdx = i;
        }
    }

    if(regValIdx == -1){                                                // Accounts for possible "LDCH" instruction 
        return;                                                         // passed in, which is not a required case
    }                                                                   // to address in this implementation.

    if(format == 3 && three_bits_zero(objCode.substr(2,1))){            // Constant is to be stored.
        int constant = stoi(objCode.substr(3,3), nullptr, 16);
        g_registerValues[regValIdx] = constant;
    }
    else if(format == 4 && three_bits_zero(objCode.substr(2,1))){
        int constant = stoi(objCode.substr(3,5), nullptr, 16);
        g_registerValues[regValIdx] = constant;
    }
    else{                                                               // Address that references a symbol is stored.
        g_registerValues[regValIdx] = targetAddr;
    }
    
}


/**
* Clears a specific register if the disassembler detects a CLEAR instruction. 
* The register being cleared depends on the 2nd bit in the object code.
* @param objCode: The object code of the CLEAR instruction.
*/ 
void clear_reg(std::string objCode){
    std::string regClear = objCode.substr(2,1);
    for(int i = 0; i < sizeof(g_registers)/sizeof(g_registers[0]); i++){ // Match register to its corresponding value index.
        if(regClear == g_registers[i]){
            g_registerValues[i] = 0;
            return;
        }
    }
}


/**
* Creates a LTORG instruction when the disassembler detects that the current program counter address matches
* any address of a literal.
*/ 
void add_LTORG(){
    g_programCounter.push_back(32); // The reason 32 is added is explained in output.cpp.
    g_labels.push_back("");
    g_opCodes.push_back("LTORG");
    g_operands.push_back("");
    g_objectCodes.push_back("");
}


/**
* Defines any literal referenced before a detected LTORG instruction was declared. 
* @param literal: The literal to be added to a line of instruction.
* @param objCode: The object code of the literal.
* @param currAddr: The address where the literal is according to the symbol table.
*/ 
void add_literal(std::string literal, std::string objCode, int currAddr){
    g_programCounter.push_back(currAddr);
    g_labels.push_back("*");
    g_opCodes.push_back(literal);
    g_operands.push_back("");
    g_objectCodes.push_back(objCode);
}




