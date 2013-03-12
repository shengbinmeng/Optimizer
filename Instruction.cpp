/*
 * Instruction.cpp
 *
 *  Created on: Nov 7, 2011
 *      Author: edwin
 */

#include "Instruction.h"
#include <cstdlib>
#include <sstream>
using namespace std;

Instruction::Instruction() {
	// TODO Auto-generated constructor stub
}

Instruction::~Instruction() {
	// TODO Auto-generated destructor stub
}

void Instruction::parse(string instr_line)
{
	stringstream ss(instr_line);
	string num, opt, op1, op2;
	ss >> num; // "instr"
	ss >> num; // line number (with ":")
	ss >> opt; // operator
	ss >> op1; // operand 1 (if exist)
	ss >> op2; // operand 2 (if exist)

	this->optName = opt;
	this->opt = parseOperator(opt);
	parseOperand(firstOperand, op1);
	parseOperand(secondOperand, op2);
}


Operator Instruction::parseOperator(string opt_str)
{
	if (opt_str.compare("nop") == 0)
		return NOP;
	else if (opt_str.compare("add") == 0)
		return ADD;
	else if (opt_str.compare("sub") == 0)
		return SUB;
	else if (opt_str.compare("mul") == 0)
		return MUL;
	else if (opt_str.compare("div") == 0)
		return DIV;
	else if (opt_str.compare("mod") == 0)
		return MOD;
	else if (opt_str.compare("neg") == 0)
		return NEG;
	else if (opt_str.compare("cmpeq") == 0)
		return CMPEQ;
	else if (opt_str.compare("cmple") == 0)
		return CMPLE;
	else if (opt_str.compare("cmplt") == 0)
		return CMPLT;
	else if (opt_str.compare("br") == 0)
		return BR;
	else if (opt_str.compare("blbc") == 0)
		return BLBC;
	else if (opt_str.compare("blbs") == 0)
		return BLBS;
	else if (opt_str.compare("call") == 0)
		return CALL;
	else if (opt_str.compare("load") == 0)
		return LOAD;
	else if (opt_str.compare("store") == 0)
		return STORE;
	else if (opt_str.compare("move") == 0)
		return MOVE;
	else if (opt_str.compare("read") == 0)
		return READ;
	else if (opt_str.compare("write") == 0)
		return WRITE;
	else if (opt_str.compare("wrl") == 0)
		return WRL;
	else if (opt_str.compare("param") == 0)
		return PARAM;
	else if (opt_str.compare("enter") == 0)
		return ENTER;
	else if (opt_str.compare("ret") == 0)
		return RET;
	else if (opt_str.compare("entrypc") == 0)
		return ENTRYPC;
	else
		return UNDEF;
}

bool Instruction::isNum(string s) {
	if (s.empty() || (s.at(0) != '-' && !isdigit(s.at(0))))
		return false;

	for (unsigned int i = 1; i < s.size(); i++) {
		if (!isdigit(s.at(i)))
			return false;
	}
	return true;
}

void Instruction::parseOperand (Operand& op, string op_str)
{
	int val;
	unsigned int i;
	if (op_str.empty())
		op.type = NA;
	else if (op_str.compare("GP") == 0)
		op.type = GP;
	else if (op_str.compare("FP") == 0)
		op.type = FP;
	else if (isNum(op_str)){
		op.type = CONST; op.value = atoi(op_str.c_str());
	}
	else if (op_str[0] == '(' && op_str[op_str.size() - 1] == ')'){
		op.type = REG; op.value = atoi(op_str.substr(1, op_str.size() - 2).c_str());
	}
	else if (op_str[0] == '[' && op_str[op_str.size() - 1] == ']'){
		op.type = LABEL; op.value = atoi(op_str.substr(1, op_str.size() - 2).c_str());
	}
	else if ((i = op_str.find('#')) != string::npos) {
		string left = op_str.substr(0, i);
		string right = op_str.substr(i + 1, op_str.size() - i - 1);
		unsigned int j;
		val = atoi(right.c_str());
		if ((j = left.find_last_of('_')) != string::npos && left.substr(j, left.size() - j).compare("_base") == 0){
			op.type = ADDR_OFFSET; op.value = val; op.name = left.substr(0, left.size() - 5);
		}
		else if ((j = left.find_last_of('_')) != string::npos
					&& left.substr(j, left.size() - j).compare("_offset") == 0){
			op.type = FIELD_OFFSET; op.value = val; op.name = left.substr(0, left.size() - 7);
		}
		else{
			op.type = LOCAL_VAR; op.value = val; op.name = left;
		}
	} else
		op.type = NA;
}

Operand& Instruction::getFirstOperand()
{
	return firstOperand;
}
Operand& Instruction::getSecondOperand()
{
	return secondOperand;
}
Operator Instruction::getOperator()
{
	return opt;
}

void Instruction::setOperator(Operator newValue)
{
	opt = newValue;
}


void Instruction::print(ostream& out)
{
	out << optName;
	OperandType type;
	int value;
	string name;
	for(int i = 0; i < 2; i++){
		if(i == 0) {
			type = firstOperand.type;
			value = firstOperand.value;
			name = firstOperand.name;
		} else {
			type = secondOperand.type;
			value = secondOperand.value;
			name = secondOperand.name;
		}
		switch(type){
			case NA: break;
			case GP: out<<" GP"; break;
			case FP: out<<" FP"; break;
			case CONST: out<< " " << value; break;
			case ADDR_OFFSET: out<<" " << name << "_base#" << value; break;
			case FIELD_OFFSET: out<< " " << name << "_offset#" << value; break;
			case LOCAL_VAR: out<< " " << name << "#" << value ; break;
			case REG: out<< " (" << value << ")"; break;
			case LABEL: out<< " [" << value << "]"; break;
		}
	}

	out << endl;
}

