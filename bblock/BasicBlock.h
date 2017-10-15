#ifndef _MINI_POLARIS_basicblock_h
#define _MINI_POLARIS_basicblock_h


#include <stdlib.h>
#include <fstream>
#include <strstream>
#include <vector>
#include "WorkSpace.h"
#include "WorkSpaceStack.h"
#include "ProgramUnit.h"
#include "Program.h"
#include "StmtList.h"
#include "Collection/List.h"
#include  "Statement/Statement.h"


using namespace std;

class BasicBlock : public Listable
{
// be sure to define the following virtual functions yourself, since
// they are inherited from Listable :

    // Duplicate and return a pointer to the new item
    // (MUST be implemented correctly by subclasses
    // for List copy functions to work).
private:
	vector<BasicBlock*> predecessors;
	vector<BasicBlock*> successors;
	vector<Statement*> stmts;


public:
	String bb_name;
	BasicBlock();
	virtual ~BasicBlock(){
	}
	BasicBlock(String n) {bb_name=n;}
    INLINE Listable *listable_clone() const; 
    INLINE void print(ostream &o) const;
    Statement * get_lead_stmt(); 
    Statement * get_last_stmt();
    void insert_stmt(Statement *s);
    void insert_pred(BasicBlock *s);
    void insert_succ(BasicBlock *s);
    vector<BasicBlock*> pred();
    vector<BasicBlock*> succ();
    vector<Statement*> statements();
    
};
vector<BasicBlock*> BasicBlock::pred(){
	return predecessors;
}
vector<BasicBlock*> BasicBlock::succ(){
	return successors;
}
vector<Statement*> BasicBlock::statements(){
	return stmts;
}

void 
BasicBlock::insert_stmt(Statement *s){
	stmts.push_back(s); 
}


void 
BasicBlock::insert_pred(BasicBlock *s){
	predecessors.push_back(s); 
}

void 
BasicBlock::insert_succ(BasicBlock *s){
	successors.push_back(s); 
}

Statement *
BasicBlock:: get_lead_stmt(){
	return stmts.front();
}
Statement * 
BasicBlock:: get_last_stmt(){
	return stmts.back();
}

INLINE Listable*
BasicBlock::listable_clone() const{
	return new BasicBlock((BasicBlock &) *this);

}

// one example of a member function :
INLINE void
BasicBlock::print(ostream &o) const
{
	int num_stmts = stmts.size();
	int num_pred = predecessors.size();
	int num_succ = successors.size();

	o << "    Basic Block " << bb_name << " :\n";
	// change X2 to the number of statements in block 
	o << "      " << num_stmts << " statements.\n";
 	o << "      starts : ";

	// change X3 to a variable containing the first statment in the block
    //    Statement* X3;
 	int indent = 0;
 	(stmts.front())->write(o, indent);
	 
 	if (num_stmts > 1)
	{
	    o << "      ends   : ";
	    (stmts.back())->write(o, indent);
	}
	// change X5 to number of predecessors
    //    int X5;
	o << "\n      " << num_pred << " predecessors : \n";
	int seen = 0;
	// change X6 to number of predecessors
    //    int X6;
	if (num_pred > 0)
	{
	    o << "        ";
	    // for every predecessor : 
	    for(int i=0;i<num_pred;++i){
	    	BasicBlock * pst = predecessors[i];
			if (seen)
			    o << ", ";
			else
			    seen++;
			
			o <<  pst->bb_name;
	    }
	    o << "\n";
	}
	o << "\n      " << num_succ << " successors : \n";
	seen = 0; 
	if (num_succ > 0)
	{
	    o << "        ";
	    // for every predecessor :
	    for(int i=0;i<num_succ;++i){
	    	BasicBlock * pst =  successors[i];
			if (seen)
			    o << ", ";
			else
			    seen++;
			
			o <<  pst->bb_name;
	    }
	    o << "\n";
	}

}




// you will need many more functions here (or you can put some in BasicBlock.cc
// for example, making sure to mention it in the Makefile in CPPSRCS...)

class BasicBlockWork : public WorkSpace{
	private:
		BasicBlock		  * _basicblock;
    public:
		BasicBlockWork(BasicBlockWork & other) : _basicblock(other._basicblock), WorkSpace(other) { }
		BasicBlockWork(int tag, BasicBlock * b) : _basicblock(b), WorkSpace(tag)	{ }
		BasicBlock		  * basicblock() { return (BasicBlock *) _basicblock; }

		INLINE Listable *listable_clone() const{
			return new BasicBlockWork((BasicBlockWork &) *this);
		}

		INLINE void print(ostream &o) const{
			o << "BasicBlockWork : [ " << _basicblock << " ]";
		}

		INLINE int structures_OK() const{
			return 1;
		}
		virtual ~BasicBlockWork(){

		}
};

#endif
