// Optimize.cpp

#include "ThreeAddrCodeParser.h"
#include "CLanguageGenerator.h"
#include "DataflowAnalyzer.h"
#include <iostream>
using namespace std;

#define _CFG 1
#define _TAC 2
#define _C	3
#define _REP 4

#define _NOPT	0
#define _SCP 1
#define _DSE 2

int main (int argc, const char * argv[])
{
	// this main function receives input from and outputs result to stdio
	// use redirection if want to process files

    string s;
	string arg;
	int do_scp = 0;
	int do_dse = 0;
    int do_ssa = 0;
	int do_licm = 0;
	int backend = 0;
    int ssa_output = 0;

	if(argc == 2 || argc == 3){
		for(int i = 1; i < argc; i++){
			s = argv[i];
			if(s.find("-opt=") != string::npos){
                if(s.find("ssa") != string::npos)
                    do_ssa = 1;
                if(s.find("licm") != string::npos)
                    do_licm = 1;
				if(s.find("scp") != string::npos)
					do_scp = 1;
				if(s.find("dse") != string::npos)
					do_dse = 1;
			}

			if(s.find("-backend=") != string::npos){
				arg = s.substr(9);
				if(arg.compare("c") == 0)
					backend = _C;
				else if(arg.compare("cfg") == 0)
					backend = _CFG;
				else if(arg.compare("3addr") == 0)
					backend = _TAC;
				else if(arg.compare("rep") == 0)
					backend = _REP;
                else if(arg.compare("ssa,cfg") == 0){
                    backend = _CFG;
                    ssa_output = 1;
                }
                else if(arg.compare("ssa,3addr") == 0){
                    backend = _TAC;
                    ssa_output = 1;
                }
                else if(arg.compare("ssa,rep") == 0){
                    backend = _REP;
                    ssa_output = 1;
                }

			}
		}
	}

	if(backend == 0){
		//cout << "usage: <TODO> "<<endl; //TODO: display usage information
		backend = _CFG;
	}

	ThreeAddrCodeParser parser;
	parser.parse(cin);
	vector<Instruction> instructions = parser.getInstructions();

	DataflowAnalyzer analyzer;
	analyzer.setInstructions(instructions);

	analyzer.buildProcedureCFGs();
	analyzer.findSCRs();

    if(do_ssa){
		analyzer.buildSSA();

        if(do_scp == 1){
            analyzer.doSSA_CP();
        }

        if(do_dse == 1){
            cout << "Dead Code Elimination with SSA not implemented." << endl;
			return 0;
        }

        if(do_licm == 1){
			analyzer.doSSA_LICM();
		}
    } else {
        if(do_licm == 1){
			cout << "Enable SSA to do loop invarient code motion." << endl;
			return 0;
		}

		if(do_scp == 1){
			analyzer.doSCP();
		}

		if(do_dse == 1){
			analyzer.doDSE();
		}
    }

	switch(backend){
		case _C:
        {
            if(do_ssa == 1) analyzer.removePhi();
			CLanguageGenerator generator;
			generator.setInstructions(instructions);
			generator.generate(cout);
            break;
        }
		case _CFG:
            if(do_ssa == 1 && ssa_output == 0) {
                    analyzer.removePhi(); //convert from SSA back to normal;
			}
            if(do_dse == 1) {
                analyzer.deleteProcedures();
                analyzer.buildProcedureCFGs();
            }

			analyzer.outputProcedures(cout);
			break;
		case _TAC:
            if(do_ssa == 1 && ssa_output == 0){
				analyzer.removePhi(); //convert from SSA back to normal;
			}
			analyzer.printInstructions(cout);
			break;
		case _REP:
			analyzer.scrReport(cout);
			cout << endl;
			if(do_scp == 1){
				analyzer.scpReport(cout);
				cout << endl;
			}
			if(do_dse == 1){
				analyzer.dseReport(cout);
				cout << endl;
			}
            if(do_licm == 1){
				analyzer.ssaLicmReport(cout);
				cout << endl;
			}
	}

    return 0;
}
