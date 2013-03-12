/*
 * CLanguageGenerator.h
 *
 *  Created on: Nov 8, 2011
 *      Author: edwin
 */

#ifndef CLANGUAGEGENERATOR_H_
#define CLANGUAGEGENERATOR_H_
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

#include "Instruction.h"

class CLanguageGenerator {
private:
	vector<Instruction> instructions;
	bool isMain;
	unsigned int funcEntry; //entry address of a function
	unsigned int funcEnd;
	unsigned int funcParaNum; //num of parameters
	unsigned int funcVarNum; //num of local variables
	vector<string> statements; //translated statements within the function
	vector<string> realParams;
	ofstream headerFile;
public:
	CLanguageGenerator();
	~CLanguageGenerator();
	void setInstructions(vector<Instruction> _instructions);
	void generate (ostream &output);
	void writeFunction(ostream& o);
	bool translateFunction();
	void translateStatement(unsigned int idx);
	void translateOperand(Operand p, string& cr);
};

#endif /* CLANGUAGEGENERATOR_H_ */
