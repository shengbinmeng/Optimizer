/*
 * BasicBlock.cpp
 *
 *  Created on: Dec 10, 2011
 *      Author: edwin
 */

#include "BasicBlock.h"

BasicBlock::BasicBlock() {
	// TODO Auto-generated constructor stub

}

BasicBlock::~BasicBlock() {
	// TODO Auto-generated destructor stub
}

void BasicBlock::addPreds(int i){
	preds.push_back(i);
}

void BasicBlock::addSuccs(int i){
	succs.push_back(i);
}

void BasicBlock::resetLiveness(){
	for(unsigned int i = 0 ; i < lives.size(); i++){
		lives[i].clear();
	}

	for(unsigned int i = 0 ; i < defs.size(); i++){
		defs[i].clear();
	}

	for(unsigned int i = 0 ; i < uses.size(); i++){
		uses[i].clear();
	}
}

