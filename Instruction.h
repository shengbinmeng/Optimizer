/*
 * Instruction.h
 *
 *  Created on: Nov 7, 2011
 *      Author: edwin
 */

#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_
#include <string>
#include <map>
using namespace std;

enum OperandType {NA, GP, FP, CONST, ADDR_OFFSET, FIELD_OFFSET, LOCAL_VAR, REG, LABEL};

enum Operator {UNDEF, NOP, ADD, SUB, MUL, DIV, MOD, NEG, CMPEQ, CMPLE, CMPLT, BR, BLBC, BLBS, CALL, LOAD, STORE, MOVE, READ, WRITE, WRL, PARAM, ENTER, RET, ENTRYPC, PHI};

struct Operand {
	OperandType type;
	int value;
	string name;
    int var_id;
};


class Instruction {
    
public:
    Operator opt;
	Operand firstOperand;
	Operand secondOperand;
    string optName;

    map<int, int> phi_param;
    
	Instruction();
	~Instruction();

	void parse(string instr_line);
	Operator parseOperator(string opt_str);
	void parseOperand (Operand& op, string op_str);
	Operand& getFirstOperand();
	Operand& getSecondOperand();
	Operator getOperator();
	void setOperator(Operator newValue);

	void print(ostream& out);
    
private:
    bool isNum(string s);

};

#endif /* INSTRUCTION_H_ */
