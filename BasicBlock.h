/*
 * BasicBlock.h
 *
 *  Created on: Dec 10, 2011
 *      Author: edwin
 */

#ifndef BASICBLOCK_H_
#define BASICBLOCK_H_

#include <map>
#include <set>
#include <stack>
#include <string>
#include <queue>
using namespace std;

#include "Instruction.h"

class BasicBlock {
public:
	BasicBlock();
	virtual ~BasicBlock();

public:
	int in;		//first line number of the block, also can be seen as the id of the block
	int out;	//last line number of the block
	vector<int> preds;		// predecessor basic blocks (vector of id)
	vector<int> succs;		// successor basic blocks (vector of id)
	map<string, set<int> > definitions; //definitions that reach past this block from this block, maps variable name to line number.
	map<string, set<int> > predDefinitions; //incoming definitions from this block's predecessors that we might propagate
	map<string, set<int> > outDefinitions; //all definitions reaching out of this block;

	vector< set<string> > lives; //live variables (string) in every point(before and after each statement) of this block
	vector< set<string> > defs;  //a set of variables (string) defined in each statement
	vector< set<string> > uses;  //a set of variables (string) used in each statement

    //dominators
	set<int> dom;
    
	//dom tree
	int dt_parent;
	vector<int> dt_children;
    
	//dom frontier
	set<int> df;
    
	vector<Instruction> phis;
    
	BasicBlock(int i, int o)
		:in(i), out(o), dt_parent(0){
		set<string> s;
		lives.assign(out - in + 2, s);  // (out-in+2) points
		defs.assign(out - in + 1, s);	// (out-in+1) statements
		uses.assign(out - in + 1, s);
	}

	set<string>& def(int idx){
		return defs[idx - in];
	}

	set<string>& use(int idx){
		return uses[idx - in];
	}

	set<string>& iInLive(int idx){
		return lives[idx - in];
	}

	set<string>& iOutLive(int idx){
		return lives[idx - in + 1];
	}

	set<string>& bInLive(){
		return lives[0];
	}

	set<string>& bOutLive(){
		return lives[out - in + 1];
	}

	void addPreds(int i);
	void addSuccs(int i);

	void resetLiveness();

};

#endif /* BASICBLOCK_H_ */
