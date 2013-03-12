/*
 * ThreeAddrCodeParser.h
 *
 *  Created on: Nov 7, 2011
 *      Author: edwin
 */

#ifndef THREEADDRCODEPARSER_H_
#define THREEADDRCODEPARSER_H_
#include <istream>
#include <vector>
#include "Instruction.h"

class ThreeAddrCodeParser {
private:
	vector<Instruction> instructions;
public:
	ThreeAddrCodeParser();
	~ThreeAddrCodeParser();
	void parse(istream &input);
	vector<Instruction> getInstructions();
};

#endif /* THREEADDRCODEPARSER_H_ */
