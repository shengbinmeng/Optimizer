objs = Instruction.o ThreeAddrCodeParser.o CLanguageGenerator.o BasicBlock.o \
		Procedure.o DataflowAnalyzer.o  Optimize.o

optimize : $(objs)
	g++ -o optimize $(objs)
$(objs) : *.cpp
	g++ -c *.cpp

.PHONI : clean
clean :
	-rm *.o optimize
