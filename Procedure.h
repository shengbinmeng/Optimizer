/*
 * Procedure.h
 *
 *  Created on: Dec 10, 2011
 *      Author: edwin
 */

#ifndef PROCEDURE_H_
#define PROCEDURE_H_

#include <map>
#include <queue>
#include "Instruction.h"
#include "BasicBlock.h"

class Procedure {
public:
	vector<Instruction>& is;
	map<int, BasicBlock *> nodes;	//key: block id(BasicBlock::in); value: pointer to the block
	int entry;	//entry of the function
	int end;	//end of the function
	bool isMain;

    bool scr_found;
	vector<vector<int> > SCR; //Strongly connect regions for this function

	int dseInSCR;
	int dseOutSCR;
	int num_propagated;//number of constants propagated in this function

    int lines_moved;//number of lines moved with ssalicm;
    
	map<string, set<int> > var_def;
    
public:
	Procedure();
	virtual ~Procedure();
	Procedure(vector<Instruction>& instrs):
		is(instrs), nodes(), entry(0), end(0), isMain(false),
		scr_found(false), SCR(1), dseInSCR(0), dseOutSCR(0), num_propagated(0), lines_moved(0){}

	bool buildCFG(int start);
	void output(ostream& o);

	void findSCR();
	void findSCRhelper(set<int>& assigned, map<int, int>& ordering, int current, int& c, stack<int>& S, stack<int>& P, int& cur_scr);
	void printSCR(ostream& o);
    
	void scp();
	void printSCP(ostream& o);

	void liveness();
	bool dse();
	void printLiveness(ostream& o);
	void resetLiveness();
    
    void findDominators();
	void buildDomTree();
	void findDomFrontier(BasicBlock *);
	void printDomFrontier(ostream& o);
    
	void placePhi();
	void printVarDef(ostream& o);
	void printPhi(ostream& o);
    
	void rename();
    
	void ssaCP(); //Single Static Assignment Constant Propagation
	bool ssaLICM(int scr); //Single Static Assignment Loop Invariant Code Motion
	void printSSAlicm(ostream& o);

private:
	void findDefinitions();
	void reachingDefinitions(queue<BasicBlock*>& workList, set<int>& doneList);
	void propagate(int line, Instruction* inst, map<string, set<int> >& curDefinitions, int& propagated, bool& changed);
	void calDefUse();
	bool isInSCR(int idx);
    
    void initVarDef();
	void renameHelper(BasicBlock * b, map<string, int>& c, map<string, stack<int> >& s);
    
	//ssalicm helpers
	bool canMove(int line, int scr);
	void doMove(const vector<int>& linestomove, int scr);
	bool inSCR(int line, int scr);
	bool definedInSCR(const Operand& p, int scr);
};

#endif /* PROCEDURE_H_ */
