/*
 * CLanguageGenerator.cpp
 *
 *  Created on: Nov 8, 2011
 *      Author: edwin
 */

#include "CLanguageGenerator.h"
#include <sstream>
#include <cstdio>
using namespace std;

CLanguageGenerator::CLanguageGenerator() {
	// TODO Auto-generated constructor stub
}

CLanguageGenerator::~CLanguageGenerator() {
	// TODO Auto-generated destructor stub
}

void CLanguageGenerator::setInstructions(vector<Instruction> _instructions){
	this->instructions = _instructions;
}

void CLanguageGenerator::generate (ostream &out){
	out << "#include <stdio.h>"<<endl;
	out << "#define WriteLine() printf(\"\\n\");" <<endl;
	out << "#define WriteLong(x) printf(\" %lld\", (long)x);" <<endl;
	out << "#define ReadLong(a) if (fscanf(stdin, \"%lld\", &a) != 1) a = 0;" <<endl;
	out << "#define long long long" << endl;
	out << "#define _push(a) _sp = _sp - 1; *(_sp) = (a);" << endl;
	out << "#define _pop(a) _sp = _sp + (a);" << endl;
	out << "#define _alloc(a) _sp = _sp - (a);" << endl;
	out << endl;
	out << "long _global_var[32768/8];"<<endl;
	out << "long * _gp = _global_var;" << endl;
	out << "long reg[" << instructions.size() << "];" <<endl;
	out << "long stack[4096];" << endl;
	out << "long * _sp = stack + 4096;" << endl;
	out << "long * _fp = 0;" << endl<<endl;

	headerFile.open("translated_functions.h");
	headerFile << "//declarations of the translated functions\n\n";

	funcEntry = 1;
	while(funcEntry < instructions.size() && translateFunction()){
		writeFunction(out);
		funcEntry = funcEnd + 1;
	}

	headerFile.close();
}

// write the translated function to output stream
void CLanguageGenerator::writeFunction(ostream& o){
	for(unsigned int i = 0 ; i < statements.size(); i++){
		o << statements[i];
	}
}

bool CLanguageGenerator::translateFunction(){
	statements.clear();
	isMain = false;

	unsigned int i = funcEntry;
	while(i < instructions.size() && instructions[i].getOperator() != ENTER){
		if(instructions[i].getOperator() == ENTRYPC)
			isMain = true;
		i++;
	}

	if(i < instructions.size()){
		funcVarNum = instructions[i].getFirstOperand().value/8;
		funcEntry = i;
	} else return false;

	while(i < instructions.size() && instructions[i].getOperator() != RET){
		i++;
	}

	if(i < instructions.size()){
		funcParaNum = instructions[i].getFirstOperand().value/8;
		funcEnd = i;
	}else return false;

	for(i = funcEntry; i <= funcEnd; i++){
		translateStatement(i);
	}

	return true;
}

// translate an 3addr instrution to a valid C statement
void CLanguageGenerator::translateStatement(unsigned int idx){
	if(!(idx >= 1 && idx < instructions.size())) return;

	stringstream ss;
	Instruction instr = instructions[idx];
	ss << "reg[" << idx << "] = ";
	string reg = ss.str();
	ss.str("");
	ss << "label_" << idx <<":";

	string op1, op2;
	translateOperand(instr.getFirstOperand(), op1);
	translateOperand(instr.getSecondOperand(), op2);
	stringstream real_param_list;
	stringstream formal_param_list;
	stringstream real_param;
	switch(instr.getOperator()){
	case ADD:
		ss << reg;
		ss << "(long)(" << op1 << ") + (long)(" << op2 <<");\n";
		break;
	case SUB:
		ss << reg;
		ss << "(long)(" << op1 << ") - (long)(" << op2 <<");\n";
		break;
	case MUL:
		ss << reg;
		ss << "(long)(" << op1 << ") * (long)(" << op2 <<");\n";
		break;
	case DIV:
		ss << reg;
		ss << "(long)(" << op1 << ") / (long)(" << op2 <<");\n";
		break;
	case MOD:
		ss << reg;
		ss << "(long)(" << op1 << ") % (long)(" << op2 <<");\n";
		break;
	case NEG:
		ss << reg;
		ss << "-(long)(" << op1<<");\n";
		break;
	case CMPEQ:
		ss << reg;
		ss << "(long)(" << op1 << ") == (long)(" << op2 <<");\n";
		break;
	case CMPLE:
		ss << reg;
		ss << "(long)(" << op1 << ") <= (long)(" << op2 <<");\n";
		break;
	case CMPLT:
		ss << reg;
		ss << "(long)(" << op1 << ") < (long)(" << op2 <<");\n";
		break;
	case BR:
		ss << "goto " << op1<<";\n";
		break;
	case BLBC:
		ss << "if((" << op1 << ")==0) goto " << op2<<";\n";
		break;
	case BLBS:
		ss << "if((" << op1 << ")!=0) goto " << op2<<";\n";
		break;
	case CALL:
		ss << "_push(0);\n";//LNK
		ss << "_push(_fp);\n";//save previous fp

		// effect of function calls can be implemented by stack frame,
		// but I tend to keep the normal format of calling a function (with parameters, as in natural C language)
		real_param_list.str("");
		if(realParams.size() > 0){
			real_param_list << realParams[0];
			for(unsigned int i = 1; i < realParams.size(); i++){
				real_param_list << ", " << realParams[i];
			}
			realParams.clear();
		}
		ss << "function_" << instr.getFirstOperand().value << "(" << real_param_list.str() << ");\n";
		break;
	case LOAD:
		ss << reg;
		ss << "*((long *)(" << op1 << "));\n";
		break;
	case STORE:
		ss << "*((long *)(" << op2 << ")) = (long)(" << op1<<");\n";
		break;
	case MOVE:
		ss << op2 << "=(long)(" << op1 <<");\n";
		ss << reg;
		ss << "(long)(" << op2 << ");\n";
		break;
	case READ:
		ss << "ReadLong(reg[" << idx << "]);\n";
		break;
	case WRITE:
		ss << "WriteLong((long)(" << op1 << "));\n";
		break;
	case WRL:
		ss << "WriteLine();\n";
		break;
	case PARAM:
		// record the parameters to use in the next function call
		real_param.str("");
		real_param << "(long)(" << op1 << ")";
		realParams.push_back(real_param.str());

		ss << "\n";
		break;
	case ENTER:
		// just to keep the normal format of a function (i.e. with parameters)
		formal_param_list.str("");
		if(funcParaNum > 0){
			formal_param_list << "long _param1";
			for(unsigned int i = 1; i < funcParaNum; i++){
				formal_param_list << ", long _param" << i + 1;
			}
		}

		ss.str("");
		if(isMain)
			ss << "void main(" << formal_param_list.str() << "){\n";
		else{
			ss << "void function_" << idx << "(" << formal_param_list.str() << "){\n";
			headerFile << "void function_" << idx << "(" << formal_param_list.str() << ");\n\n";
		}

		//only local variables use stack
		ss << "_fp = _sp;\n";
		ss << "_alloc(" << funcVarNum << ");\n";
		break;
	case RET:
		ss << "_fp = (long *)*(_fp);\n";
		ss << "_pop(" << (funcVarNum + 2) << ");\n";
		ss << "return;\n}\n";
		break;
	default: return;
	}

	statements.push_back(ss.str());
}

// translate an Operand to a string of C code
void CLanguageGenerator::translateOperand(Operand p, string& str){
	stringstream ss;
	switch(p.type){
		case NA:
			str = "";
			break;
		case GP:
			str = "_gp";
			break;
		case FP:
			str = "_fp";
			break;
		case CONST:
			ss << p.value;
			str = ss.str();
			break;
		case ADDR_OFFSET:
			ss << (p.value);
			str = ss.str();
			break;
		case FIELD_OFFSET:
			ss << (p.value);
			str = ss.str();
			break;
		case LOCAL_VAR:
			// use function parameters
			if(p.value / 8 > 0){
				unsigned int param_idx = funcParaNum + 2 - p.value / 8;
				ss << "_param" << param_idx;
			}
			else ss << "(*(_fp + " << (p.value/8) << "))";
			str = ss.str();
			break;
		case REG:
			ss << "reg[" << p.value << "]";
			str = ss.str();
			break;
		case LABEL:
			ss << "label_" << p.value;
			str = ss.str();
			break;
	}
}
