/*
 * DataflowAnalyzer.h
 *
 *  Created on: Dec 10, 2011
 *      Author: edwin
 */

#ifndef DATAFLOWANALYZER_H_
#define DATAFLOWANALYZER_H_
#include <vector>

#include "Instruction.h"
#include "Procedure.h"

class DataflowAnalyzer {
	vector<Instruction> instructions;
	vector<Procedure *> procedures;

public:
	DataflowAnalyzer();
	virtual ~DataflowAnalyzer();

	void setInstructions(vector<Instruction> _instructions);
	vector<Instruction> getInstructions();

	void printInstructions(ostream& o);
	void refineInstructions();

	void buildProcedureCFGs();
	void outputProcedures(ostream& o);
	void deleteProcedures();

	void findSCRs();
	void scrReport(ostream& o);

	void doDSE();
	void dseReport(ostream& o);
	void doSCP();
	void scpReport(ostream& o);
    
    void removePhi();
    void buildSSA();
    void doSSA_CP();
    void doSSA_LICM();
    void ssaLicmReport(ostream& o);

};

#endif /* DATAFLOWANALYZER_H_ */
