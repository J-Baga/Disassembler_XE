#include <iostream>
#include <string>
#include <iomanip>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

void disassemble(std::string objFile, std::string symFile);
void parse_obj(std::string objFile);
void parse_sym(std::string symTab); 
std::string get_mnemonic(std::string firstHex, std::string secondHex);
int get_format(std::string mnemonic, std::string thirdHex);
std::string addr_mode_OP(std::string secondHex);
std::string addr_mode_TA(std::string thirdHex);
bool is_indexed(std::string thirdHex);
int get_TA(int format, int locAddr, std::string objCode, std::string addrTA, bool isIndex);
std::string get_operand(int targetAddr, int format, std::string mnemonic, std::string objCode, std::string addrOP, bool isIndex);
bool three_bits_zero(std::string thirdHex);
void add_label(int currAddr);
void fill_gap(int currAddr, int currItr);
std::vector<int> find_addr_gap_matches(int endAddr, int currentItr);
void load_reg(int targetAddr, int format, std::string mnemonic, std::string objCode);
void clear_reg(std::string objCode);
void add_LTORG();
void add_literal(std::string literal, std::string objCode, int currAddr);