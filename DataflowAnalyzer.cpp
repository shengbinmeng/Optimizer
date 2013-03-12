/*
 * DataflowAnalyzer.cpp
 *
 *  Created on: Dec 10, 2011
 *      Author: edwin
 */

#include "DataflowAnalyzer.h"
#include <iostream>
using namespace std;

DataflowAnalyzer::DataflowAnalyzer() {
	// TODO Auto-generated constructor stub
}

DataflowAnalyzer::~DataflowAnalyzer() {
	// TODO Auto-generated destructor stub
	deleteProcedures();
}

void DataflowAnalyzer::setInstructions(vector<Instruction> _instructions){
	this->instructions = _instructions;
}

vector<Instruction> DataflowAnalyzer::getInstructions(){
	return this->instructions;
}

void DataflowAnalyzer::buildProcedureCFGs()
{
	unsigned int start = 1;
	while(start < instructions.size()){
		Procedure * p = new Procedure(instructions);
		if(p->buildCFG(start) == false){
			delete p;
			return;
		}
		procedures.push_back(p);
		start = p->end + 1;
	}
}

void DataflowAnalyzer::findSCRs()
{
	for(unsigned int i = 0; i < procedures.size(); i++){
		procedures[i]->findSCR();
	}
}


void DataflowAnalyzer::deleteProcedures(){
	for(unsigned int i = 0 ; i < procedures.size(); i++){
		delete procedures[i];
	}
	procedures.clear();
}

void DataflowAnalyzer::outputProcedures(ostream& o){
	for(unsigned int i = 0; i < procedures.size(); i++){
		procedures[i]->output(o);
	}
}


void DataflowAnalyzer::doDSE()
{
	for(unsigned int i = 0; i < procedures.size(); i++){
		do{
			procedures[i]->liveness();
		} while(procedures[i]->dse());
	}

	refineInstructions();
}

void DataflowAnalyzer::dseReport(ostream& o)
{
	for(unsigned int i = 0; i < procedures.size(); i++){
		o<<"Function: "<< procedures[i]->entry<<endl;
		o<<"Number of statements eliminated in SCR: "<< procedures[i]->dseInSCR<<endl;
		o<<"Number of statements eliminated not in SCR: "<<procedures[i]->dseOutSCR<<endl;
	}
}



void DataflowAnalyzer::doSCP()
{
	for(unsigned int i = 0; i < procedures.size(); i++){
		procedures[i]->scp();
	}
}

void DataflowAnalyzer::scpReport(ostream& o)
{
	for(unsigned int i = 0; i < procedures.size(); i++){
		procedures[i]->printSCP(o);
	}
}

void DataflowAnalyzer::scrReport(ostream& o)
{
	for(unsigned int i = 0; i < procedures.size(); i++){
		procedures[i]->printSCR(o);
	}
}

void DataflowAnalyzer::refineInstructions()
{
	unsigned int c = 1;
	vector<Instruction> orig = instructions;

	for(unsigned int i = 1; i < orig.size(); i++){
		if(orig[i].getOperator() != UNDEF){
			if(c != i){
				for(unsigned int j = 1; j< orig.size(); j++){
					Instruction& instr = orig[j];
					if((instr.getFirstOperand().type == REG || instr.getFirstOperand().type == LABEL) && instr.getFirstOperand().value == (int)i)
						instr.getFirstOperand().value = c;
					if((instr.getSecondOperand().type == REG || instr.getSecondOperand().type == LABEL) && instr.getSecondOperand().value == (int)i)
						instr.getSecondOperand().value = c;
				}
			}
			c++;
		}
	}
	instructions.clear();
	for(unsigned int i = 0; i < orig.size(); i++){
		if(orig[i].getOperator() != UNDEF){
			instructions.push_back(orig[i]);
		}
	}
}

void DataflowAnalyzer::printInstructions(ostream& o)
{
	for(unsigned int i = 1; i < instructions.size(); i++){
		o<<"instr " << i <<": ";
		instructions[i].print(o);
	}
}


void DataflowAnalyzer::buildSSA()
{
	vector<Instruction> oldir = instructions;
	map<int, int> newreg;
	map<int, int> newlab;
	map<int, BasicBlock *> bb;
    
	for(unsigned int i = 0; i < procedures.size(); i++){
		procedures[i]->findDominators();
		procedures[i]->buildDomTree();
		procedures[i]->findDomFrontier(procedures[i]->nodes.begin()->second);
		procedures[i]->liveness();
		//procedures[i]->printDomFrontier(cout);
		procedures[i]->placePhi();
		//procedures[i]->printPhi(cout);
		bb.insert(procedures[i]->nodes.begin(), procedures[i]->nodes.end());
	}
    
	instructions.clear();
	int c = 0;
	for(unsigned int i = 0; i < oldir.size(); i++){
		if(bb.find(i) != bb.end()){
			BasicBlock * b = bb[i];
			newlab[i] = c;
			for(unsigned int j = 0 ; j < b->phis.size(); j++){//insert phi funcs
				instructions.push_back(b->phis[j]);
			}
			c += b->phis.size();
		}
		instructions.push_back(oldir[i]);
		newreg[i] = c;
		c++;
	}
    
	//update r* label*
	for(unsigned int i = 0; i < instructions.size(); i++){
		Instruction& instr = instructions[i];
		if(instr.firstOperand.type == REG)
			instr.firstOperand.value = newreg[instr.firstOperand.value];
		else if(instr.firstOperand.type == LABEL)
			instr.firstOperand.value = newlab[instr.firstOperand.value];
        
		if(instr.secondOperand.type == REG)
			instr.secondOperand.value = newreg[instr.secondOperand.value];
		else if(instr.secondOperand.type == LABEL)
			instr.secondOperand.value = newlab[instr.secondOperand.value];
        
		if(instr.opt == MOVE && instr.firstOperand.type == REG && instr.firstOperand.value == 0){
			instr.firstOperand.value = i - 1;
		}
	}
    
	//rebuild procedures
	deleteProcedures();
	buildProcedureCFGs();
    
	//now do renaming
	for(unsigned int i = 0; i < procedures.size(); i++){
		procedures[i]->findDominators();
		procedures[i]->buildDomTree();
		procedures[i]->rename();
	}
}

void DataflowAnalyzer::removePhi()
{
	vector<Instruction> oldir = instructions;
	map<int, int> newreg;
	map<int, int> newlab;
	map<int, BasicBlock *> bb;
    
	for(unsigned int i = 0; i < procedures.size(); i++){
		bb.insert(procedures[i]->nodes.begin(), procedures[i]->nodes.end());
	}
    
	instructions.clear();
	int c = 0;
	for(unsigned int i = 0; i < oldir.size(); i++){
		if(bb.find(i) != bb.end()){
			BasicBlock * b = bb[i];
			newlab[i] = c;
			int nphi = 0;
			for(int j = b->in; j < b->out && oldir[j].opt==PHI; j+=2){
				nphi++;
			}
			i += (2*nphi);
		}
		instructions.push_back(oldir[i]);
		newreg[i] = c;
		c++;
	}
    
	//update r* label*
	for(unsigned int i = 0; i < instructions.size(); i++){
		Instruction& instr = instructions[i];
		if(instr.firstOperand.type == REG)
			instr.firstOperand.value = newreg[instr.firstOperand.value];
		else if(instr.firstOperand.type == LABEL)
			instr.firstOperand.value = newlab[instr.firstOperand.value];
		else if(instr.firstOperand.type == LOCAL_VAR)
			instr.firstOperand.var_id = -1;
        
		if(instr.secondOperand.type == REG)
			instr.secondOperand.value = newreg[instr.secondOperand.value];
		else if(instr.secondOperand.type == LABEL)
			instr.secondOperand.value = newlab[instr.secondOperand.value];
		else if(instr.secondOperand.type == LOCAL_VAR)
			instr.secondOperand.var_id = -1;
        
	}
	//rebuild procedures
	deleteProcedures();
	buildProcedureCFGs();
}

//SSA Constant Propagation
void DataflowAnalyzer::doSSA_CP(){
	for(unsigned int i = 0; i < procedures.size(); i++){
		procedures[i]->ssaCP();
	}
}

//SSA Loop Invarient Code Motion
void DataflowAnalyzer::doSSA_LICM(){
	for(unsigned int i = 0; i < procedures.size(); i++){
		vector<vector<int> >* SCR = &procedures[i]->SCR;
        
		if(!procedures[i]->scr_found){
			procedures[i]->findSCR();
		}
		if(SCR->size() == 0)
			continue;
        
		bool changed = false;
		unsigned int scrsize = SCR->size();
        
		for(unsigned int j = 0; j < scrsize;){
			changed = procedures[i]->ssaLICM(j);
			if(changed){
				int lines_moved = procedures[i]->lines_moved;
				int num_propagated = procedures[i]->num_propagated;
				//cout << "lines_moved: " << lines_moved << endl;
				//ir.print(cout);
                deleteProcedures();
                buildProcedureCFGs();
				procedures[i]->findSCR();
				SCR = &procedures[i]->SCR;
				//printSCR(procedures, cout);
				//printCFG(procedures, cout);
				procedures[i]->lines_moved = lines_moved;
				procedures[i]->num_propagated = num_propagated;
			}
			else
				j++;
		}
        
		int lines_moved = procedures[i]->lines_moved;
		int num_propagated = procedures[i]->num_propagated;

        deleteProcedures();
        buildProcedureCFGs();
		procedures[i]->findSCR();
		procedures[i]->lines_moved = lines_moved;
		procedures[i]->num_propagated = num_propagated;
	}
}

void DataflowAnalyzer::ssaLicmReport(ostream& o){
	for(unsigned int i = 0; i < procedures.size(); i++){
		procedures[i]->printSSAlicm(o);
	}
}



