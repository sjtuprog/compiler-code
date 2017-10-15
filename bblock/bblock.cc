// 
// bblock.cc : basic block routines
//

#include <stdlib.h>
#include <fstream>
#include <strstream>

#include "ProgramUnit.h"
#include "Program.h"
#include "StmtList.h"
#include "Collection/List.h"
#include  "Statement/Statement.h"
#include  "bblock/bblock.h"

// it is necessary to declare these template classes whenever you create
// a new class from a template so that the inherited member functions will
// be created
template class TypedCollection<BasicBlock>;
template class List<BasicBlock>;
template class Iterator<BasicBlock>;
template class Assign<BasicBlock>;
template class RefSet<BasicBlock>;



using namespace std;

List<BasicBlock> * find_basic_blocks(ProgramUnit & pgm)
{ 
    int bb_number=0; // The index of basic block
    String pgm_name = pgm.routine_name_ref(); // The program unit's name

    List<BasicBlock> *bbl = new List<BasicBlock>;
	int	PASStag = create_pass_tag();
    int num_of_stmts = pgm.stmts().entries();
 	int type = 0;
 	strstream o; 
 	String bb_name;
 	int * block_info = new int[num_of_stmts];



 	Iterator<Statement> stmt_it = pgm.stmts().iterator();
 	for(int i=0;i<num_of_stmts;++i){
 		Statement & st = stmt_it.current();  
 		block_info[i]= get_type(st);
 		++stmt_it;
 	}
 	for(int i=0;i<num_of_stmts-1;++i){
 		if(block_info[i]==1 || block_info[i]==3){
 			if(block_info[i+1]==0){
 				block_info[i+1]=2;
 			}
 			if(block_info[i+1]==1){
 				block_info[i+1]=3;
 			}
 		}

 	}
 	/*
 	stmt_it = pgm.stmts().iterator();
 	for(int i=0;i<num_of_stmts;i++){
 		int indent = 0;
 		cout<<block_info[i]<<' '<<endl;
 		stmt_it.current().write(cout, indent);
 		cout<<i<<endl;
 		cout<<"-----"<<endl;
 		++stmt_it;
 	}
	*/

	 
 	stmt_it = pgm.stmts().iterator();
 	for(int i=0;i<num_of_stmts;++i){
 		Statement & st = stmt_it.current();  
 		Statement * pst = &stmt_it.current();  

 		//Statement *np = pst->clone();
 		//cout<< i<<' '<<pst->pred().entries()<<endl;
 		//cout<< i<<' '<<np->pred().entries()<<endl;

	 	type = block_info[i];
	 	if(type==0){ // not start not end
	 		BasicBlock * bb = bbl->last_ref(); 
	 		bb->insert_stmt(pst); 
	 		pst->work_stack().push(new BasicBlockWork(PASStag,bb));
	 	}
	 	
	 	else if(type==1){ //end
	 		BasicBlock * bb = bbl->last_ref(); 
	 		bb->insert_stmt(pst); 
	 		pst->work_stack().push(new BasicBlockWork(PASStag,bb));
	 	}
	 	else if(type ==2){ //start , not end
	 		bb_name = gen_bbname(pgm_name,bb_number);
	 		bb_number++;
	 		BasicBlock *bb = new BasicBlock(bb_name);  
	 		bb->insert_stmt(pst);
	 		pst->work_stack().push(new BasicBlockWork(PASStag,bb));
	 		bbl->ins_last(bb); 
	 	}
	 	 
	 	else{ // both start and end
	 		bb_name = gen_bbname(pgm_name,bb_number);
	 		bb_number++;
	 		BasicBlock *bb = new BasicBlock(bb_name);  
	 		bb->insert_stmt(pst);
	 		pst->work_stack().push(new BasicBlockWork(PASStag,bb));
	 		bbl->ins_last(bb); 
	 	} 
		
	 	++stmt_it;
 	}
 	
 	//cout<<"blocks: "<<bb_number<<endl;
 	//cout<<"entries: "<<bbl->entries()<<endl; 
 	


 	for(int i=0;i<bbl->entries();i++){        // build connection between blocks 
 		BasicBlock * cur = &(*bbl)[i];
 		Statement * lead = cur->get_lead_stmt();
 		StringElem se("C                - - - - - - - - - BASIC BLOCK : " + cur->bb_name);
 		lead->pre_directives().ins_last(new StringElem(se)); 
 		Statement * last = cur->get_last_stmt();
 		RefSet<Statement> pred = lead->pred();
 		RefSet<Statement> succ = last->succ();
 		//cout<<i<<' '<<pred.entries()<<' '<<succ.entries()<<endl;
 		for(int j=0; j< pred.entries();j++){
 			Statement & s = pred._element(j);
 			BasicBlockWork	  * bb_workspace = (BasicBlockWork *) s.work_stack().top_ref(PASStag);
 			BasicBlock		  * the_bb = bb_workspace->basicblock();	 
 			cur->insert_pred(the_bb); 
 		}
 		//cout<<i<<' '<<pred.entries()<<' '<<succ.entries()<<endl;
 		for(int j=0; j< succ.entries();j++){
 			Statement & s = succ._element(j);
 			BasicBlockWork	  * bb_workspace = (BasicBlockWork *) s.work_stack().top_ref(PASStag);
 			BasicBlock		  * the_bb = bb_workspace->basicblock();	
 			cur->insert_succ(the_bb);
 		}

 	}

	pgm.clean_workspace(PASStag);
 	delete [] block_info;
	 
 	 
	return bbl;
	  
	
}

String gen_bbname(String pgm_name, int bb_number){
	strstream o;  
    o << pgm_name << "#" << bb_number << '\000';
    char *name = o.str();
    String bb_name = name;
    return bb_name;

}


int get_type(Statement & s){
	Statement * next = s.next_ref();
	//Statement * prev = s.prev_ref();
	RefSet<Statement> succ = s.succ();
	RefSet<Statement> pred = s.pred();

	int start = 0, end = 0;

	if(pred.entries()>=2 || pred.entries()==0){
		start = 1;
	}
	if(succ.entries()>=2 || (succ.entries()==1 &&   &succ._element(0)!= next ) ){
		end = 1;
	}

	if(start==0){
		if(end==0) return 0; // not end. not start.
		else return 1; // is end. but not a start.
	}
	else{
		if(end==0) return 2;// is start, not end.
		else return 3;// is start is end
	}


}


// suggestions for print format added for convenience

void		    summarize_basic_blocks(ProgramUnit& pgm, List<BasicBlock> * bbl, ostream &o)
{
    // this exits the program upon an unexpected condition, bbl == 0 (NULL)
	 

    p_assert(bbl, "summarize_basic_blocks() : null bbl");

    o << "BASIC BLOCK SUMMARY FOR ";

// print PROGRAM/SUBROUTINE/FUNCTION depending upon type ...
//    and if not the right type, unconditionally exit
    if (pgm.pu_class() == BLOCK_DATA_PU_TYPE || pgm.pu_class() == UNDEFINED_PU_TYPE || pgm.pu_class()== NUM_PU_TYPES){
    	p_abort("summarize_basic_blocks() : invalid PU type");
    }
 	 

    o << pgm.routine_name_ref()<<'\n';

    o << "=============================================\n";

    // change X to the number of basic blocks in pgm
    int X = bbl->entries();
    o << "   " << X << " basic blocks total.\n\n";

    for(int i=0;i<bbl->entries();i++){       
 		BasicBlock * cur = &(*bbl)[i];
 		cur->print(o);
 	}

    o << "\n";
}
