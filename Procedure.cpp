/*
 * Procedure.cpp
 *
 *  Created on: Dec 10, 2011
 *      Author: edwin
 */

#include "Procedure.h"
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
#include <algorithm>
using namespace std;

Procedure::~Procedure() {
	// TODO Auto-generated destructor stub
	map<int, BasicBlock *>::iterator it;
	for(it = nodes.begin(); it != nodes.end(); it++){
		delete (*it).second;
	}
}

void Procedure::resetLiveness(){
	map<int, BasicBlock *>::iterator it;
	for(it = nodes.begin(); it != nodes.end(); it++){
		it->second->resetLiveness();
	}
}

bool Procedure::buildCFG(int start){
	set<int> ins;

	unsigned int i = start;
	while(i < is.size() && is[i].getOperator() != ENTER){
		if(is[i].getOperator() == ENTRYPC)
			isMain = true;
		i++;
	}

	if(i < is.size())
		entry = i;
	else
		return false;

	while(i < is.size() && is[i].getOperator() != RET){
		switch(is[i].getOperator()){
			case BR:
				ins.insert(is[i].firstOperand.value);
				ins.insert(i+1);
				break;
			case BLBC:
			case BLBS:
				ins.insert(is[i].secondOperand.value);
				ins.insert(i+1);
				break;
			case CALL:
				ins.insert(i+1);
				break;
			default:
				break;
		}
		i++;
	}

	if(i < is.size())
		end = i;
	else
		return false;

	ins.insert(entry);

	set<int>::iterator it, nit;
	it = nit = ins.begin();
	nit++;
	while(nit != ins.end()){
		nodes.insert(pair<int, BasicBlock *>(*it, new BasicBlock(*it, *nit - 1)));
		it++;
		nit++;
	}

	nodes.insert(pair<int, BasicBlock *>(*it, new BasicBlock(*it, end)));

	map<int, BasicBlock *>::iterator mit;
	mit = nodes.begin();
	while(mit != nodes.end()){
		int in = (*mit).first;  // pair' first element, the block id and in
		int out = nodes[in]->out;
		int des;
		switch(is[out].getOperator()){
		case BR:
			des = is[out].firstOperand.value;
			nodes[in]->addSuccs(des);
			nodes[des]->addPreds(in);
			mit++;
			break;
		case BLBC:
		case BLBS:
			des = is[out].secondOperand.value;
			nodes[in]->addSuccs(des);
			nodes[des]->addPreds(in);
		default:
			mit++;
			if(mit!= nodes.end()){
				nodes[in]->addSuccs(out+1);
				nodes[out+1]->addPreds(in);
			}
			break;
		}
	}

	return true;
}

void Procedure::output(ostream& o){
	o<<"Function: "<<entry<<endl;
	map<int, BasicBlock *>::iterator it;
	o<<"Basic blocks:";
	for(it = nodes.begin(); it != nodes.end(); it++){
		o<<" "<< (*it).first;
	}
	o<<endl;
	o<<"CFG:"<<endl;
	for(it = nodes.begin(); it != nodes.end(); it++){
		int in = (*it).first;
		o<<in<<" ->";
		if(nodes[in]->succs.size() == 1){
			o<<" "<<nodes[in]->succs[0];
		}
		if(nodes[in]->succs.size() == 2){
			int i = nodes[in]->succs[0] < nodes[in]->succs[1] ? 0:1;
			o<<" "<<nodes[in]->succs[i]<<" "<<nodes[in]->succs[1-i];
		}
		o<<endl;
	}
}


//find the definitions in each block that may reach out of it
void Procedure::findDefinitions(){
	map<int, BasicBlock *>::iterator it;
	map<string, set<int> >::iterator it2;
	BasicBlock* block;
	int line;
	Instruction* inst;

	line = 0;

	for(it = nodes.begin(); it != nodes.end(); it++){
		line = (*it).first;
		block = (*it).second;
		while(line <= block->out){
			//std::cout << "la: " << line << std::endl;
			//line++;
			inst = &(is[line]);
			if(inst->getOperator() == MOVE && inst->getSecondOperand().type == LOCAL_VAR){
				block->definitions[inst->getSecondOperand().name].clear();
				block->definitions[inst->getSecondOperand().name].insert(line);
				block->outDefinitions[inst->getSecondOperand().name].clear();
				block->outDefinitions[inst->getSecondOperand().name].insert(line);
			}
			else if(inst->getOperator() == ADD && ((inst->getFirstOperand().type == ADDR_OFFSET && inst->getSecondOperand().type == GP) ||
										 (inst->getFirstOperand().type == GP && inst->getSecondOperand().type == ADDR_OFFSET)  )){
				Instruction* nextInst = &(is[line+1]);
				if(nextInst->getOperator() == STORE && nextInst->getSecondOperand().value == line){
					block->definitions[(inst->getFirstOperand().name + "_base")].clear();
					block->definitions[(inst->getFirstOperand().name + "_base")].insert(line);
					block->outDefinitions[inst->getFirstOperand().name + "_base"].clear();
					block->outDefinitions[inst->getFirstOperand().name + "_base"].insert(line);
				}
			}
			else if(inst->getOperator() == CALL){
				for(it2 = block->definitions.begin(); it2 != block->definitions.end(); it2++){
					string var_name = (*it2).first;
					if(var_name.find("_base") != string::npos){
						block->definitions[var_name].clear();
					}
				}

				for(it2 = block->outDefinitions.begin(); it2 != block->outDefinitions.end(); it2++){
					string var_name = (*it2).first;
					if(var_name.find("_base") != string::npos){
						block->outDefinitions[var_name].clear();
					}
				}
			}

			line++;
		}
	}
}

//Performs reaching definition analysis
//Uses a worklist
void Procedure::reachingDefinitions(queue<BasicBlock*>& workList, set<int>& doneList){
	findDefinitions(); //find the definitions in the blocks
	BasicBlock* block;
	map<int, BasicBlock *>::iterator it;
	map<string, set<int> >::iterator it2;
	map<string, set<int> >::iterator def1;

	set<int> lines;

	while(!workList.empty()){
		block = workList.front();
		bool changed = false;

		for(unsigned int i = 0; i < block->preds.size(); i++){
			BasicBlock* pred = (*(nodes.find(block->preds[i]))).second;

			for(it2 = pred->outDefinitions.begin(); it2 != pred->outDefinitions.end(); it2++){
				string variable = (*it2).first;
				lines = (*it2).second;
				if(lines.size() > 0){
					def1 = block->definitions.find(variable);

					//if we don't have a definition that kills the previous definitions, then insert them in the out set
					if(def1 == block->definitions.end()){
						unsigned int orig_size = block->outDefinitions[variable].size();
						block->outDefinitions[variable].insert(lines.begin(), lines.end());
						if(orig_size != block->outDefinitions[variable].size()){
							changed = true;
						}
					}

					block->predDefinitions[variable].insert(lines.begin(), lines.end());
				}
			}
		}

		//if block has changed, add successors to the workList if they aren't already on it
		if(changed){
			for(unsigned int i = 0; i < block->succs.size(); i++){
				set<int>::iterator element = doneList.find(block->succs[i]);
				if(element != doneList.end()){
					BasicBlock* pushBlock = (*(nodes.find(block->succs[i]))).second;
					workList.push(pushBlock);
					doneList.erase(element);
				}
			}
		}

		workList.pop();
		doneList.insert(block->in);
	}
}

//simple constant propagation using reaching definitions
void Procedure::scp(){
	int propagated = 0;
	BasicBlock* block;
	map<int, BasicBlock *>::iterator it;
	map<string, set<int> >::iterator it2;
	int line;

	Instruction* inst;
	Instruction* nextInst;

	set<int> doneListSCP;
	set<int> doneListR;
	queue<BasicBlock *> workListSCP;
	queue<BasicBlock *> workListR;

	for(it = nodes.begin(); it != nodes.end(); it++){
		workListR.push((*it).second);
		workListSCP.push((*it).second);
	}

	reachingDefinitions(workListR, doneListR);//perform reaching definitions analysis
	while(!workListSCP.empty()){
		block = workListSCP.front();
		map<string, set<int> > curDefinitions(block->predDefinitions); //definitions at the current line;

		bool changed = false;

		for(line = block->in; line <= block->out; line++){
			inst = &(is[line]);
			nextInst = &(is[line+1]);

			if (inst->getOperator() == NOP){
				//do nothing
			}
			else if (inst->getOperator() == ADD){
				propagate(line, inst, curDefinitions, propagated, changed);

				//check if this is a global variable being addressed and modified
				if((inst->getFirstOperand().type == ADDR_OFFSET && inst->getSecondOperand().type == GP) || (inst->getFirstOperand().type == GP && inst->getSecondOperand().type == ADDR_OFFSET)){
					if(nextInst->getOperator() == STORE && nextInst->getSecondOperand().value == line && nextInst->getSecondOperand().type == REG){
						curDefinitions[inst->getFirstOperand().name + "_base"].clear();
						curDefinitions[inst->getFirstOperand().name + "_base"].insert(line);
					}
				}
			}
			else if (inst->getOperator() == SUB || inst->getOperator() == MUL || inst->getOperator() == DIV || inst->getOperator() == MOD
				  || inst->getOperator() == NEG || inst->getOperator() == CMPEQ || inst->getOperator() == CMPLE || inst->getOperator() == CMPLT){

				propagate(line, inst, curDefinitions, propagated, changed);
			}
			else if (inst->getOperator() == BR){
				//do nothing
			}
			else if (inst->getOperator() == BLBC){
				//do nothing
			}
			else if (inst->getOperator() == BLBS){
				//do nothing
			}
			else if (inst->getOperator() == CALL){
				//kill all global variable definitions since we don't how the function being called will modify them
				for(it2 = curDefinitions.begin(); it2 != curDefinitions.end(); it2++){
					string var_name = (*it2).first;
					if(var_name.find("_base") != string::npos){
						curDefinitions[var_name].clear();
					}
				}
			}
			else if (inst->getOperator() == LOAD || inst->getOperator() == STORE){
				propagate(line, inst, curDefinitions, propagated, changed);
			}
			else if (inst->getOperator() == MOVE){
				propagate(line, inst, curDefinitions, propagated, changed);

				//check if this is a local varible being modified
				if(inst->getSecondOperand().type == LOCAL_VAR){
					curDefinitions[inst->getSecondOperand().name].clear();
					curDefinitions[inst->getSecondOperand().name].insert(line);
				}
			}
			else if (inst->getOperator() == READ){
				//do nothing
			}
			else if (inst->getOperator() == WRITE){
				propagate(line, inst, curDefinitions, propagated, changed);
			}
			else if (inst->getOperator() == WRL){
				//do nothing
			}
			else if (inst->getOperator() == PARAM){
				propagate(line, inst, curDefinitions, propagated, changed);
			}
			else if (inst->getOperator() == ENTER){
				//do nothing
			}
			else if (inst->getOperator() == RET){
				//do nothing
			}
			else if (inst->getOperator() == ENTRYPC){
				//do nothing
			}
			else{
				//do nothing
			}
		}

		if(changed){
			for(unsigned int i = 0; i < block->succs.size(); i++){
				set<int>::iterator element = doneListSCP.find(block->succs[i]);
				BasicBlock* pushBlock = (*(nodes.find(block->succs[i]))).second;
				workListR.push(pushBlock);
				pushBlock->predDefinitions.clear();
				if(element != doneListSCP.end()){
					workListSCP.push(pushBlock);
					doneListSCP.erase(element);
				}
			}
			reachingDefinitions(workListR, doneListR);
		}

		workListSCP.pop();
		doneListSCP.insert(block->in);
	}

	num_propagated = propagated;
}

void Procedure::propagate(int line, Instruction* inst, map<string, set<int> >& curDefinitions, int& propagated, bool& changed){
	Instruction* defInst;
	map<string, set<int> >::iterator it2;
	int reg;
	set<int> defLines;
	set<int>::iterator set_it;

	//Check if this is a local variable and propagate if it is possible
	if(inst->getFirstOperand().type == LOCAL_VAR){
		bool do_propagation = true;
		it2 = curDefinitions.find(inst->getFirstOperand().name);
		if(it2 != curDefinitions.end()){
			defLines = (*it2).second;
			if(defLines.size() == 0){
				//no previous definitions
				do_propagation = false;
			}
			else if(defLines.size() > 1){
			//if more than one definition, check if all are the same constant
				Instruction* inst1 = &(is[*(defLines.begin())]);
				Instruction* inst2;
				if(inst1->getOperator() == MOVE && inst1->getFirstOperand().type == CONST){
					for(set_it = defLines.begin()++; set_it != defLines.end(); set_it++){
						inst2 = &(is[*set_it]);
						if(inst2->getFirstOperand().type != CONST || (inst1->getFirstOperand().value != inst2->getFirstOperand().value)){
							do_propagation = false;
							break;
						}
					}
				}
				else
					do_propagation = false;
			}

			if(do_propagation){
				defInst = &(is[*(defLines.begin())]);
				if(defInst->getOperator() == MOVE && defInst->getFirstOperand().type == CONST){
					inst->getFirstOperand().type = CONST;
					inst->getFirstOperand().value = defInst->getFirstOperand().value;
					inst->getFirstOperand().name = "";
					propagated++;
					changed = true;
				}
			}
		}
	}

	if(inst->getSecondOperand().type == LOCAL_VAR && inst->getOperator() != MOVE){
		bool do_propagation = true;
		it2 = curDefinitions.find(inst->getSecondOperand().name);
		if(it2 != curDefinitions.end()){
			defLines = (*it2).second;

			if(defLines.size() == 0){
				//no previous definitions
				do_propagation = false;
			}
			else if(defLines.size() > 1){
			//if more than one definition, check if all are the same constant
				Instruction* inst1 = &(is[*(defLines.begin())]);
				Instruction* inst2;

				if(inst1->getOperator() == MOVE && inst1->getFirstOperand().type == CONST){
					for(set_it = defLines.begin()++; set_it != defLines.end(); set_it++){
						inst2 = &(is[*set_it]);
						if(inst2->getFirstOperand().type != CONST || (inst1->getFirstOperand().value != inst2->getFirstOperand().value)){
							do_propagation = false;
							break;
						}
					}
				}
				else
					do_propagation = false;
			}

			if(do_propagation){
				defInst = &(is[*(defLines.begin())]);
				if(defInst->getOperator() == MOVE && defInst->getFirstOperand().type == CONST){
					inst->getSecondOperand().type = CONST;
					inst->getSecondOperand().value = defInst->getFirstOperand().value;
					inst->getSecondOperand().name = "";
					propagated++;
					changed = true;
				}
			}
		}
	}

	//Check if this is referencing a global variable and propagate it if it is possible
	if(inst->getFirstOperand().type == REG){
		reg = inst->getFirstOperand().value;
		defInst = &(is[reg]);
		if(defInst->getOperator() == LOAD && defInst->getFirstOperand().type == REG){
			reg = defInst->getFirstOperand().value;
			defInst = &(is[reg]);
			if(defInst->getOperator() == ADD && defInst->getFirstOperand().type == ADDR_OFFSET && defInst->getSecondOperand().type == GP){
				it2 = curDefinitions.find(defInst->getFirstOperand().name + "_base");
				if(it2 != curDefinitions.end()){
					defLines = (*it2).second;
					bool do_propagation = true;
					if(defLines.size() == 0){
						//no previous definitions
						do_propagation = false;
					}
					else if(defLines.size() > 1){
						//if more than one definition, check if all are the same constant
						Instruction* inst1 = &(is[*(defLines.begin()) + 1]);
						Instruction* inst2;

						if(inst1->getOperator() == STORE && inst1->getFirstOperand().type == CONST){
							for(set_it = defLines.begin()++; set_it != defLines.end(); set_it++){
								inst2 = &(is[*set_it + 1]);
								if(inst2->getFirstOperand().type != CONST || (inst1->getFirstOperand().value != inst2->getFirstOperand().value)){
									do_propagation = false;
									break;
								}
							}
						}
						else
							do_propagation = false;
					}

					if(do_propagation){
						defInst = &(is[*(defLines.begin()) + 1]);
						if(defInst->getOperator() == STORE && defInst->getFirstOperand().type == CONST){
							inst->getFirstOperand().type = CONST;
							inst->getFirstOperand().value = defInst->getFirstOperand().value;
							inst->getFirstOperand().name = "";
							propagated++;
							changed = true;
						}
					}
				}
			}
		}
	}

	if(inst->getSecondOperand().type == REG){
		reg = inst->getSecondOperand().value;
		defInst = &(is[reg]);
		if(defInst->getOperator() == LOAD && defInst->getFirstOperand().type == REG){
			reg = defInst->getFirstOperand().value;
			defInst = &(is[reg]);
			if(defInst->getOperator() == ADD && defInst->getFirstOperand().type == ADDR_OFFSET && defInst->getSecondOperand().type == GP){
				it2 = curDefinitions.find(defInst->getFirstOperand().name + "_base");
				if(it2 != curDefinitions.end()){
					defLines = (*it2).second;
					bool do_propagation = true;
					if(defLines.size() == 0){
						//no previous definitions
						do_propagation = false;
					}
					else if(defLines.size() > 1){
						//if more than one definition, check if all are the same constant
						Instruction* inst1 = &(is[*(defLines.begin()) + 1]);
						Instruction* inst2;

						if(inst1->getOperator() == STORE && inst1->getFirstOperand().type == CONST){
							for(set_it = defLines.begin()++; set_it != defLines.end(); set_it++){
								inst2 = &(is[*set_it + 1]);
								if(inst2->getFirstOperand().type != CONST || (inst1->getFirstOperand().value != inst2->getFirstOperand().value)){
									do_propagation = false;
									break;
								}
							}
						}
						else
							do_propagation = false;
					}

					if(do_propagation){
						defInst = &(is[*(defLines.begin()) + 1]);
						if(defInst->getOperator() == STORE && defInst->getFirstOperand().type == CONST){
							inst->getSecondOperand().type = CONST;
							inst->getSecondOperand().value = defInst->getFirstOperand().value;
							inst->getSecondOperand().name = "";
							propagated++;
							changed = true;
						}
					}
				}
			}
		}
	}
}

void Procedure::printSCP(ostream& o){
	o << "Function: " << entry << endl;
	o << "Number of constants propagate: " << num_propagated << endl;
}

void Procedure::calDefUse(){
	map<int, BasicBlock * >::iterator it;
	BasicBlock * b;
	for(it = nodes.begin(); it != nodes.end(); it++){
		b = (*it).second;
		for(int i = b->in; i <= b->out; i++){
			b->def(i).clear();
			b->use(i).clear();
			const Operator& op = is[i].getOperator();
			const Operand& p1 = is[i].firstOperand;
			const Operand& p2 = is[i].secondOperand;

			stringstream ss;

			switch(op){//def
			case ADD:
			case SUB:
			case MUL:
			case DIV:
			case MOD:
			case NEG:
			case CMPEQ:
			case CMPLE:
			case CMPLT:
			case LOAD:
			case READ: ss.str(""); ss<<"r"<<i; b->def(i).insert(ss.str()); break;
			case MOVE:
				ss.str(""); ss<<"r"<<i;
				b->def(i).insert(ss.str());

				if(p2.type == REG){
					ss.str(""); ss<<"r"<<p2.value;
					b->def(i).insert(ss.str());
				}
				else if(p2.type == LOCAL_VAR)
					b->def(i).insert(p2.name);
				break;
			default:
				break;
			}

			//use
			if(p1.type == LOCAL_VAR)
				b->use(i).insert(p1.name);
			else if(p1.type == REG){
				ss.str(""); ss<<"r"<<p1.value;
				b->use(i).insert(ss.str());
			}

			if(p2.type == LOCAL_VAR && op != MOVE)
				b->use(i).insert(p2.name);
			else if(p2.type == REG && op != MOVE){
				ss.str(""); ss<<"r"<<p2.value;
				b->use(i).insert(ss.str());
			}
		}
	}
}

void Procedure::liveness(){
	resetLiveness();
	calDefUse();
	queue<BasicBlock *> worklist;
	BasicBlock * block;
	map<int, BasicBlock *>::reverse_iterator rit;
	//init work list
	for(rit = nodes.rbegin(); rit != nodes.rend(); rit++){
		worklist.push((*rit).second);
	}

	while(!worklist.empty()){
		block = worklist.front();
		bool changed = false;
		block->bOutLive().clear();
		for(unsigned int i = 0 ; i < block->succs.size(); i++){//out[b] = U in[succ]
			BasicBlock * succ = nodes[block->succs[i]];
			block->bOutLive().insert(succ->bInLive().begin(), succ->bInLive().end());
		}

		set<string> oldin = block->bInLive();

		for(int i = block->out; i >= block->in; i--){//in[i] = (out[i] - def[i]) U use[i]
			block->iInLive(i).clear();
			block->iInLive(i) = block->iOutLive(i);
			set<string>::iterator dit, uit;
			for(dit = block->def(i).begin(); dit != block->def(i).end(); dit++){
				block->iInLive(i).erase(*dit);
			}

			for(uit = block->use(i).begin(); uit != block->use(i).end(); uit++){
				block->iInLive(i).insert(*uit);
			}
		}

		//test whether livein changed?
		if(block->bInLive().size() != oldin.size())
			changed = true;
		else{
			set<string>::iterator it;
			for(it = oldin.begin(); it != oldin.end(); it++){
				if(block->bInLive().find(*it) == block->bInLive().end())
					changed = true;
			}
		}

		if(changed){
			for(unsigned int i = 0; i < block->preds.size(); i++)
				worklist.push(nodes[block->preds[i]]);
		}

		worklist.pop();

	}
}

bool Procedure::dse(){

	bool changed = false;

	map<int, BasicBlock *>::iterator it;
	BasicBlock * b;
	set<string>::iterator dit;
	for(it = nodes.begin(); it != nodes.end(); it++){
		b = (*it).second;
		for(int i = b->in; i <= b->out; i++){
			if(!b->def(i).empty()){
				for(dit = b->def(i).begin(); dit != b->def(i).end(); dit++){
					if(b->iOutLive(i).find(*dit) != b->iOutLive(i).end())
						break;
				}
				if(dit == b->def(i).end()){//dead code
					isInSCR(i)? dseInSCR++ : dseOutSCR++;
					is[i].setOperator(UNDEF); //mark for removal
					is[i].firstOperand.type = NA;
					is[i].secondOperand.type = NA;
					changed = true;
				}
			}
		}
	}

	return changed;
}

void Procedure::printLiveness(ostream& o){
	o<<"function_"<<entry<<" liveness:"<<endl;

	map<int, BasicBlock *>::iterator it;
	BasicBlock * b;
	set<string>::iterator dit;
	for(it = nodes.begin(); it != nodes.end(); it++){
		b = (*it).second;
		o<<"************************"<<endl;
		for(int i = b->in; i <= b->out; i++){

			for(dit = b->iInLive(i).begin(); dit != b->iInLive(i).end(); dit++){
				o<<" "<< *dit;
			}
			o<<endl;
			o<<"instr "<<i<<": ";
			is[i].print(cout);
		}

		for(dit = b->bOutLive().begin(); dit != b->bOutLive().end(); dit++){
			o<<" "<< *dit;
		}
		o<<endl;
	}
}


void Procedure::findSCR()
{
	map<int, BasicBlock *>::iterator it;
	map<int, int> ordering;
	set<int> assigned;
	int c = 0;
	int cur_scr = 0;
	stack<int> S;
	stack<int> P;
	BasicBlock* cur;

	for(it = nodes.begin(); it != nodes.end(); it++){
		cur = (*it).second;
		if(ordering.find(cur->in) == ordering.end())
			findSCRhelper(assigned, ordering, cur->in, c, S, P, cur_scr);
	}

	int size = SCR.size();

	for(int i = 0; i < size; i++){
		if(SCR[i].size() <= 1){
			SCR.erase(SCR.begin() + i);
			i--;
			size--;
		}
		else
			sort(SCR[i].begin(), SCR[i].end());
	}

	sort(SCR.begin(), SCR.end());
}

//Recursive helper
void Procedure::findSCRhelper(set<int>& assigned, map<int, int>& ordering, int current, int& c, stack<int>& S, stack<int>& P, int& cur_scr){
	BasicBlock* cur = nodes[current];

	ordering.insert(pair<int,int>(current, c));
	c++;
	S.push(current);
	P.push(current);
	for(unsigned int i = 0; i < cur->succs.size(); i++){
		if(ordering.find(cur->succs[i]) == ordering.end())
			findSCRhelper(assigned, ordering, cur->succs[i], c, S, P, cur_scr);
		else{
			if(!P.empty() && (assigned.find(cur->succs[i]) == assigned.end())){
				while(ordering[cur->succs[i]] < ordering[P.top()]){
					P.pop();
				}
			}
		}
	}

	if(!P.empty() && !S.empty()){
		if(P.top() == current){
			while(S.top() != current){
				SCR[cur_scr].push_back(S.top());
				assigned.insert(S.top());
				S.pop();
			}
			SCR[cur_scr].push_back(S.top());
			assigned.insert(S.top());
			S.pop();
			P.pop();
			cur_scr++;
			SCR.push_back(vector<int>());
		}
	}
}

void Procedure::printSCR(ostream& o)
{
	o<<"Function: "<<entry<<endl;
	map<int, BasicBlock *>::iterator it;
	o<<"Basic blocks:";
	for(it = nodes.begin(); it != nodes.end(); it++){
		o<<" "<< (*it).first;
	}
	o << endl;

	o << "SCR: " << endl;

	for(unsigned int i = 0; i < SCR.size(); i++){
		for(unsigned int j = 0; j < SCR[i].size(); j++){
			o << SCR[i][j] << " " ;
		}
		o << endl;
	}
}

bool Procedure::isInSCR(int idx)
{
	for(unsigned int i = 0 ; i < SCR.size(); i++){
		for(unsigned int j = 0; j < SCR[i].size(); j++){
			if(idx >= nodes[SCR[i][j]]->in && idx <= nodes[SCR[i][j]]->out)
				return true;
		}
	}
	return false;
}

void Procedure::findDominators(){
	//max solution
	map<int, BasicBlock *>::iterator mit;
	set<int>::iterator sit;
	vector<int> nid;
	queue<BasicBlock *> worklist;
	BasicBlock * block;
    
	//collect all the nodes in the cfg
	for(mit = nodes.begin(); mit != nodes.end(); mit++){
		nid.push_back(mit->first);
	}
    
	//set initial dom state for nodes
	for(mit = nodes.begin(); mit != nodes.end(); mit++){
		mit->second->dom.clear();//clear set dom
		mit->second->dom.insert(nid.begin(),nid.end());
		worklist.push(mit->second);
	}
    
	//find dominators using a worklist
	while(!worklist.empty()){
		block = worklist.front();
		bool changed = false;
		set<int> oldom = block->dom;//save for compare
		block->dom.clear();
        
		set<int> un;
		
		//get union of dom of all the preds 
		for(unsigned int i = 0; i < block->preds.size(); i++){
			un.insert(nodes[block->preds[i]]->dom.begin(), nodes[block->preds[i]]->dom.end());
		}
		
		//find intersection of dom of all the preds
		set<int> intersec;
		for(sit = un.begin(); sit != un.end(); sit++){
			bool add = true;
			for(unsigned int i = 0; i < block->preds.size(); i++){
				BasicBlock * pred = nodes[block->preds[i]];
				if(pred->dom.find(*sit) == pred->dom.end()){
					add = false;
					break;
				}
			}
			if(add)
				intersec.insert(*sit);
		}
        
		//add itself to dom
		block->dom = intersec;
		block->dom.insert(block->in);
        
		//test whether dom changed? 
		if(block->dom.size() != oldom.size())
			changed = true;
		else{
			for(sit = oldom.begin(); sit != oldom.end(); sit++){
				if(block->dom.find(*sit) == block->dom.end())
					changed = true;
			}
		}
        
		if(changed){
			for(unsigned int i = 0; i < block->succs.size(); i++)
				worklist.push(nodes[block->succs[i]]);
		}
		worklist.pop();
	}
}

void Procedure::buildDomTree(){
	map<int, set<int> > doms;
	map<int, set<int> >::iterator dit;
	queue<int> q;
    
	map<int, BasicBlock *>::iterator mit;
    
	//clear
	for(mit = nodes.begin(); mit != nodes.end(); mit++){
		mit->second->dt_parent = 0;
		mit->second->dt_children.clear();
	}
	
	//construct the tree
	//push root to the queue
	mit = nodes.begin();
	q.push(mit->first);
    
	mit++;
	while(mit != nodes.end()){
		doms[mit->first] = mit->second->dom;
		mit++;
	}
    
	while(!q.empty()){
		int id = q.front();
		q.pop();
		BasicBlock * b = nodes[id];
		
		vector<int> del;
		for(dit = doms.begin(); dit != doms.end(); dit++){
			dit->second.erase(id);
			if(dit->second.size() == 1 && *(dit->second.begin()) == dit->first){//found a child of node[id]
				q.push(dit->first);//put the child in the queue
				del.push_back(dit->first);//mark for remove from doms
				b->dt_children.push_back(dit->first);//add dom edge
				nodes[dit->first]->dt_parent = id;
			}
		}
        
		//remove from doms
		for(unsigned int i = 0; i < del.size(); i++){
			doms.erase(del[i]);
		}
	}
    
}

//see the Cythron paper
void Procedure::findDomFrontier(BasicBlock * b){
	for(unsigned int i = 0; i < b->dt_children.size(); i++){
		findDomFrontier(nodes[b->dt_children[i]]);
	}
    
	b->df.clear();
	for(unsigned int i = 0; i < b->succs.size(); i++){
		BasicBlock * succ = nodes[b->succs[i]];
		if(succ->dt_parent != b->in){
			b->df.insert(succ->in);
		}
	}
    
	for(unsigned int i = 0; i < b->dt_children.size(); i++){
		BasicBlock * child = nodes[b->dt_children[i]];
		set<int>::iterator sit;
		for(sit = child->df.begin(); sit != child->df.end(); sit++){
			BasicBlock * cdf = nodes[*sit];
			if(cdf->dt_parent != b->in){
				b->df.insert(cdf->in);
			}
		}
	}
	
}

void Procedure::printDomFrontier(ostream& o){
	map<int, BasicBlock *>::iterator it;
	o<<"Dominance frontier"<<endl;
	for(it = nodes.begin(); it != nodes.end(); it++){
		o<<it->first<<" : ";
		BasicBlock * b = it->second;
		set<int>::iterator sit;
		for(sit = b->df.begin(); sit != b->df.end(); sit++){
			o<<*sit<<" ";
		}
		o<<endl;
	}
}


void Procedure::initVarDef(){
	//clear
	var_def.clear();
    
	map<int, BasicBlock *>::iterator it;
	for(it = nodes.begin(); it != nodes.end(); it++){
		BasicBlock * b = it->second;
		for(int i = b->in; i <= b->out; i++){
			Operand& p2 = is[i].secondOperand;
            
			if(is[i].opt == MOVE && p2.type == LOCAL_VAR){
				var_def[p2.name].insert(b->in);
			}
		}
	}
}

void Procedure::placePhi(){
	initVarDef();
	//printVarDef(cout);
	int iterCount = 0;
	map<int, int> already, work;
	map<int, BasicBlock *>::iterator it;
	queue<BasicBlock *> worklist;
    
	for(it = nodes.begin(); it != nodes.end(); it++){
		already[it->first] = 0;
		work[it->first] = 0;
		it->second->phis.clear();
	}
    
	map<string, set<int> >::iterator vit;
	set<int>::iterator sit;
	for(vit = var_def.begin(); vit != var_def.end(); vit++){
		iterCount++;
		for(sit = vit->second.begin(); sit != vit->second.end(); sit++){
			work[*sit] = iterCount;
			worklist.push(nodes[*sit]);
		}
		while(!worklist.empty()){
			BasicBlock * b = worklist.front();
			worklist.pop();
            
			for(sit = b->df.begin(); sit != b->df.end(); sit++){
				if(already[*sit] < iterCount && nodes[*sit]->bInLive().find(vit->first) != nodes[*sit]->bInLive().end()){
					//place vit->first at node *sit
					Operand pv;
                    pv.type = LOCAL_VAR;
                    pv.value = 0;
                    pv.name = vit->first;
                    pv.var_id = -1;
					Operand pr;
                    pr.type = REG;
					Instruction i1, i2;
					i1.opt = PHI;
					i1.firstOperand = pv;
					i1.optName = "phi";
                    
					i2.opt = MOVE;
					i2.firstOperand = pr;
					i2.secondOperand = pv;
					i2.optName = "move";
                    
					nodes[*sit]->phis.push_back(i1);
					nodes[*sit]->phis.push_back(i2);
                    
					already[*sit] = iterCount;
					if(work[*sit] < iterCount){
						work[*sit] = iterCount;
						worklist.push(nodes[*sit]);
					}
                    
				}
			}
		}
	}
}

void Procedure::printVarDef(ostream& o){
	map<string, set<int> >::iterator it;
	set<int>::iterator sit;
	o<<"Var Def:"<<endl;
	for(it = var_def.begin(); it != var_def.end(); it++){
		o<<it->first<<" : ";
		for(sit = it->second.begin(); sit != it->second.end(); sit++){
			o<<*sit<<" ";
		}
		o<<endl;
	}
}

void Procedure::printPhi(ostream& o){
	map<int, BasicBlock *>::iterator it;
	o<<"Phi:"<<endl;
	for(it = nodes.begin(); it != nodes.end(); it++){
		o<<"block "<<it->first<<endl;
		for(unsigned int i = 0; i < it->second->phis.size(); i++){
			it->second->phis[i].print(o);
		}
	}
}


void Procedure::rename(){
	map<string, int> c;
	map<string, stack<int> > s;
	set<string>::iterator sit;
    
	set<string> use;
	//find use of local var
	for(int i = entry; i <= end; i++){
		Operand& p1 = is[i].firstOperand;
		Operand& p2 = is[i].secondOperand;
		if(p1.type == LOCAL_VAR ){
			use.insert(p1.name);
		}
		if(p2.type == LOCAL_VAR && is[i].opt != MOVE){
			use.insert(p2.name);
		}
	}
	//assign an id for all the input parameters
	for(sit = use.begin(); sit != use.end(); sit++){
		s[*sit].push(c[*sit]);
		c[*sit]++;
	}
	renameHelper(nodes.begin()->second, c, s);
}

void Procedure::renameHelper(BasicBlock * b, map<string, int>& c, map<string, stack<int> >& s){
	for(int i = b->in; i <= b->out; i++){
		Operand& p1 = is[i].firstOperand;
		Operand& p2 = is[i].secondOperand;
		
		if(is[i].opt != PHI){
			if(p1.type == LOCAL_VAR)
				p1.var_id = s[p1.name].top();
            
			if(p2.type == LOCAL_VAR && is[i].opt != MOVE)
				p2.var_id = s[p2.name].top();			
		}
        
		if(is[i].opt == MOVE && p2.type == LOCAL_VAR){
			p2.var_id = c[p2.name];
			s[p2.name].push(p2.var_id);
			c[p2.name]++;
		}	
	}
    
	for(unsigned int i = 0; i < b->succs.size(); i++){
		BasicBlock * succ = nodes[b->succs[i]];
		for(int j = succ->in; j <= succ->out && is[j].opt == PHI; j+=2){
			//if(!s[is[j].firstOperand.name].empty())
            is[j].phi_param[b->in] = s[is[j].firstOperand.name].top();
		}
	}
    
	for(unsigned int i = 0; i < b->dt_children.size(); i++){
		renameHelper(nodes[b->dt_children[i]], c, s);
	}
    
	for(int i = b->in; i <= b->out; i++){
		Operand& p2 = is[i].secondOperand;
		if(is[i].opt == MOVE && p2.type == LOCAL_VAR){
			s[p2.name].pop();
		}	
	}
    
}

//---------------------------------------------
//Single Static Assignment Constant Propagation
//---------------------------------------------
void Procedure::ssaCP(){
	vector<int> moves; //vector containing the MOVE instructions
    
	for(unsigned int line = 0; line < is.size(); line++){
		const Instruction& inst = is[line];
		if(inst.opt == MOVE){
			moves.push_back(line);
		}
	}
    
	for(int line = entry; line < end; line++){
		Instruction& inst = is[line];
        
		if(inst.firstOperand.type == LOCAL_VAR){
			const string& cur_lit = inst.firstOperand.name;
			int cur_var_id = inst.firstOperand.var_id;
            
			for(unsigned int j = 0; j < moves.size(); j++){
				if(moves[j] >= line)
					break;
				const Instruction& i = is[moves[j]];
				const string& lit = i.secondOperand.name;
				int var_id = i.secondOperand.var_id;
                
				if( (cur_lit.compare(lit) == 0) && (cur_var_id == var_id) && (i.firstOperand.type == CONST) ){
					inst.firstOperand.type = CONST;
					inst.firstOperand.value = i.firstOperand.value;
					num_propagated++;
				}
			}
		}
		else if(inst.secondOperand.type == LOCAL_VAR){
			const string& cur_lit = inst.secondOperand.name;
			int cur_var_id = inst.secondOperand.var_id;
            
			for(unsigned int j = 0; j < moves.size(); j++){
				if(moves[j] >= line)
					break;
				const Instruction& i = is[moves[j]];
				const string& lit = i.secondOperand.name;
				int var_id = i.secondOperand.var_id;
                
				if( (cur_lit.compare(lit) == 0) && (cur_var_id == var_id) && (i.firstOperand.type == CONST) ){
					inst.secondOperand.type = CONST;
					inst.secondOperand.value = i.firstOperand.value;
					num_propagated++;
				}
			}
		}
	}
}
//----------------------------------------------------
//End of Single Static Assignment Constant Propagation
//----------------------------------------------------

//---------------------------------------------------
//Single Static Assignment Loop Invariant Code Motion
//---------------------------------------------------
bool Procedure::ssaLICM(int scr){
	bool changed = false;
	BasicBlock* block;
	vector<int> linestomove;
    
	int line = 1;
	for(unsigned int j = 0; j < SCR[scr].size(); j++){
		block = nodes[SCR[scr][j]];
		for(line = line + block->in; line <= block->out; line++){
			if(canMove(line, scr)){
				changed = true;
				linestomove.push_back(line);
				lines_moved++;
			}
		}
		line = 0;
	}
	
	if(changed){
		doMove(linestomove, scr);
	}
    
	return changed;
}

bool Procedure::canMove(int line, int scr){
	const Instruction& inst = is[line];
    
	switch(inst.opt){
		case UNDEF: //Should never move these
		case PHI:
		case NOP:
		case RET:
		case ENTRYPC:
		case BR:
		case BLBC:
		case BLBS:
		case CALL:
		case PARAM:
		case ENTER:
		case READ:
		case WRITE:
		case WRL: 
		case MOVE:
		case LOAD:
		case STORE: 
		case CMPEQ:
		case CMPLE:
		case CMPLT: return false;
		case ADD: //If both operands are never altered inside the strongly connected region, we can move the instruction
		case SUB:
		case MUL:
		case DIV:
		case MOD:
		case NEG:{
            int type1 = inst.firstOperand.type;
            int type2 = inst.secondOperand.type;
            if( ( (type1 == CONST) || (type1 == ADDR_OFFSET) || (type1 == FIELD_OFFSET) || (type1 == GP) || (type1 == FP) )
               && ( (type2 == CONST) || (type2 == ADDR_OFFSET) || (type2 == FIELD_OFFSET) || (type2 == GP) || (type2 == FP) ) )
                return true;
            
            if(type1 == REG){
                if(inSCR(inst.firstOperand.value, scr)){
                    return false;
                }
            }
            
            if(type2 == REG){
                if(inSCR(inst.secondOperand.value, scr)){
                    return false;
                }
            }
            
            if(type1 == LOCAL_VAR){
                if(definedInSCR(inst.firstOperand, scr)){
                    return false;
                }
            }
            
            if(type2 == LOCAL_VAR){
                if(definedInSCR(inst.secondOperand, scr)){
                    return false;
                }
            }
        }
			break;
	}
    
	return true;
}

void Procedure::doMove(const vector<int>& linestomove, int scr){
	BasicBlock* block;
	block = nodes[SCR[scr][0]];
	int entryline = block->in;
	for(unsigned int i = 0; i < linestomove.size(); i++){
		int targetline = linestomove[i];
		Instruction inst = is[targetline];
		is.erase(is.begin() + targetline);
		is.insert(is.begin() + entryline, inst);
		for(unsigned int j = 0; j < is.size(); j++){
			Instruction& inst1 = is[j];
			if( inst1.opt == BR ){
				//Operand 1 is always a label to jump to
				if(inst1.firstOperand.type == LABEL){
					if(inst1.firstOperand.value >= entryline && inst1.firstOperand.value <= targetline)
						inst1.firstOperand.value++;
				}
			}
			else if (inst1.opt == CALL){
			} 
			else if ( (inst1.opt == BLBC) || (inst1.opt == BLBS) ){
				//Operand 1 is always a register
				if(inst1.firstOperand.type == REG){
					if(inst1.firstOperand.value == targetline){
						inst1.firstOperand.value = entryline;
					}
					else if(inst1.firstOperand.value >= entryline && inst1.firstOperand.value < targetline)
						inst1.firstOperand.value++;
				}
                
				//Operand 2 is always a label to jump to
				if(inst1.secondOperand.type == LABEL){
					if(j >= (unsigned int)entryline){
						if(inst1.secondOperand.value >= entryline && inst1.secondOperand.value <= targetline)
							inst1.secondOperand.value++;
					}
				}
			}
			else {
				if(inst1.firstOperand.type == REG){
					if(inst1.firstOperand.value == targetline){
						inst1.firstOperand.value = entryline;
					}
					else if(inst1.firstOperand.value >= entryline && inst1.firstOperand.value < targetline)
						inst1.firstOperand.value++;
				}
                
				if(inst1.secondOperand.type == REG){
					if(inst1.secondOperand.value == targetline)
						inst1.secondOperand.value = entryline;
					else if(inst1.secondOperand.value >= entryline && inst1.secondOperand.value < targetline)
						inst1.secondOperand.value++;
				}
			}
		}
		entryline++;
	}
}

bool Procedure::inSCR(int line, int scr){
	BasicBlock* block;
    
	for(unsigned int i = 0; i < SCR[scr].size(); i++){
		block = nodes[SCR[scr][i]];
		if( (line >= block->in) && (line <= block->out) )
			return true;
	}
    
	return false;
}

bool Procedure::definedInSCR(const Operand& p, int scr){
	BasicBlock* block;
	const string& lit = p.name;
	int var_id = p.var_id;
	for(unsigned int i = 0; i < SCR[scr].size(); i++){
		block = nodes[SCR[scr][i]];
		for(int line = block->in; line <= block->out; line++){
			const Instruction& inst = is[line];
			if( (inst.opt == MOVE) && (inst.secondOperand.name.compare(lit) == 0) && (inst.secondOperand.var_id == var_id) )
				return true;
		}
	}
    return false;
}

void Procedure::printSSAlicm(ostream& o){
	o << "Function: " << entry << endl;
	o << "Number of statements hoisted: " << lines_moved << endl;
}

//----------------------------------------------------------
//End of Single Static Assignment Loop Invariant Code Motion
//----------------------------------------------------------
