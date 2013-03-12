/*
 * ThreeAddrCodeParser.cpp
 *
 *  Created on: Nov 7, 2011
 *      Author: edwin
 */

#include "ThreeAddrCodeParser.h"

ThreeAddrCodeParser::ThreeAddrCodeParser() {
	// TODO Auto-generated constructor stub
}

ThreeAddrCodeParser::~ThreeAddrCodeParser() {
	// TODO Auto-generated destructor stub
}

// parse the 3addr input into a set(vector) of instructions 
void ThreeAddrCodeParser::parse(istream &input)
{
	instructions.clear();
	Instruction instr;
	instructions.push_back(instr); // little trick to make an instr's label-number and its vector-index become the same value

	string instr_line;
	while (input.good()) {
		getline(input, instr_line);
		if (instr_line.size() <= 0)
			break;
		Instruction instr;
		instr.parse(instr_line);
		instructions.push_back(instr);
	}
}

vector<Instruction> ThreeAddrCodeParser::getInstructions(){
	return instructions;
}
