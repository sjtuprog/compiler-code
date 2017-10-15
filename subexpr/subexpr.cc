#include "subexpr.h"
 
#include "StmtList.h"
#include "Collection/List.h"
#include "Statement/Statement.h"
#include "Statement/AssignmentStmt.h"
#include "Expression/FunctionCallExpr.h"
#include "Expression/IDExpr.h" 
#include "Expression/BinaryExpr.h" 
#include "Expression/NonBinaryExpr.h" 
#include "bblock/bblock.h"
#include "bblock/BasicBlock.h"
#include <string>
#include <vector>
#include <map> 
#include <stack>
#include <sstream>

using namespace std;


void subexpr_elimination(ProgramUnit & pgm, List<BasicBlock> * pgm_basic_blocks, int p_avail, int availexpr, int gcseliminate, int copyprop) {
 
  	map<Statement *, RefSet<Expression> >  IN;
	map<Statement *, RefSet<Expression> >  OUT; 
	split_all(pgm); 
	cout << "AFTER SPLITTING INTO BINARY_EXPR\n";
	pgm.write(cout);
	for(int i=0;i<1;i++){
		IN.clear();
		OUT.clear();
		
		if(availexpr) cal_available_expression(pgm, IN,OUT);
		if(p_avail) print_in_out(pgm.stmts(),IN,OUT); 
		if(gcseliminate) {GCSE(pgm,IN,OUT); cout << "AFTER GCSE\n";pgm.write(cout);}
		
		if(copyprop) {copy_propagation(pgm);cout << "AFTER COPY PROPAGATION\n";pgm.write(cout);}
	}
	 	
	
	return;
}
int expr_type_check(Expression &e){ //expr that terminates

	if(e.arg_list().entries()==0){ // single 
		return 1;
	}
	else if(e.arg_list().entries()==2){ // binary
		Expression & e1 = e.arg_list()[0];
		Expression & e2 = e.arg_list()[1];
		if(e1.arg_list().entries()==0 && e2.arg_list().entries()==0){
			return 2;
		}
		
	}
	return 0;
}

int expr_op_check(Expression &e){
	if(e.op()==FUNCTION_CALL_OP ||e.op()==INTRINSIC_CALL_OP ) return 4; 
	if(e.op()==MULT_OP||e.op()==ADD_OP||e.op()==OR_OP||e.op()==AND_OP
		||e.op()==EQV_OP||e.op()==NEQV_OP||e.op()==COLON_OP||e.op()==CONCAT_OP) return 3;


	return 2;
}


void split_all(ProgramUnit & pgm){
	StmtList & stmts = pgm.stmts();
	vector<Statement *> v;
	for(int i=0;i<stmts.entries();i++){
		Statement * st = & stmts[i]; 
		v.push_back(st);
	}
	int counter=-1;
	for(int i=0;i<v.size();i++){
		Statement *st = v[i];
		if(st->stmt_class()==ASSIGNMENT_STMT){
			Expression &r = st->rhs();
			if(!expr_type_check(r)) split_expr(r,pgm,st,counter,1);
			
		}
		else if(st->stmt_class()==IF_STMT || st->stmt_class()==WHILE_STMT){
			Expression &r = st->expr();
			if(!expr_type_check(r)) split_expr(r,pgm,st,counter,0);
		}
		
		else if(st->stmt_class()==PRINT_STMT){ 
			//Expression &r =  st->io_list_guarded();
			
			//if(!expr_type_check(r)) split_expr(r,pgm,st,counter,0);

		}
		else if(st->stmt_class()==CALL_STMT){
			Expression & r = st->parameters_guarded();   
			if(r.op()==COMMA_OP){

				for(int i=0;i<r.arg_list().entries();i++){
					Expression & sub = r.arg_list()[i]; 
					if(sub.arg_list().entries()!=0) {split_expr(sub,pgm,st,counter,0); }
				}
			}
			else{
				if(!expr_type_check(r)) {split_expr(r,pgm,st,counter,0); }
			}
			
		} 

	}

}

void split_expr(Expression &e,ProgramUnit & pgm,Statement *s,int &counter,int flag){
	StmtList & stmts = pgm.stmts(); 
	vector<Statement*> v;
	if(expr_op_check(e)==4) split_func(pgm,e,v,counter);
	else	split_binary(pgm,e,v,counter); 
	for(int i=0;i<v.size();i++){ 
		stmts.ins_before(v[i],s); 
	} 

	if(flag==1){ //assign stmt
		Expression & left = s->lhs();
		Expression * right = &v[v.size()-1]->lhs();
		Statement * n =  new AssignmentStmt(stmts.new_tag(), left.clone(), right->clone());
		stmts.ins_before(n->clone(),s);
		stmts.del(*s); 
	}
	else{  
		Mutator<Expression> ep =  s->iterate_expressions();
		Expression & if_cond = v[v.size()-1]->lhs();
		ep.assign() =    if_cond.clone();
		
	}
}

const char* new_temp_name(int & counter){
	string x = "temp"+NumberToString(counter);
	const char * s = x.c_str();
	return s;
}

void split_func(ProgramUnit & pgm,Expression &e,vector<Statement * > & v,int & counter){
	 
	Expression & para = e.parameters_guarded();
	StmtList & stmts = pgm.stmts();
	vector<Expression*> temp;	
	for(int i=0;i<para.arg_list().entries();i++){
		Expression & sub = para.arg_list()[i];
		if(expr_type_check(sub)==1) {
			temp.push_back(&sub);
		}
		else{
			split_binary(pgm,sub,v,counter);
			temp.push_back(&v[v.size()-1]->lhs());
		}
	}

	counter++;
	Expression * left_temp =  new_variable(new_temp_name(counter), e.type(), pgm);
	List<Expression> *L = new List<Expression> ;
	for(int k=0;k<temp.size();k++) { 
		L->ins(&*temp[k]->clone(),k);
		//temp[k]->print(cout);
		//cout<<',';
	}  


	Expression*	comm = new CommaExpr(L);
	  
	Expression *right_temp = new FunctionCallExpr(e.type(), e.function().clone(),comm->clone());  
	Statement * n =  new AssignmentStmt(stmts.new_tag(), left_temp->clone(), right_temp->clone()); 
	v.push_back(n); 
	return; 
}

void split_binary(ProgramUnit & pgm,Expression &e,vector<Statement * > & v,int & counter){
	counter++;

	StmtList & stmts = pgm.stmts();

 	if(expr_type_check(e)==1){ // single
 		return;
 	}

	else if(expr_type_check(e)==2){  //minimal binary
		  
		Expression * temp =  new_variable(new_temp_name(counter), e.type(), pgm);
		Statement * n =  new AssignmentStmt(stmts.new_tag(), temp->clone(), e.clone()); 
		v.push_back(n);
		 
		return;
	}

	vector<Expression*> temp;	

	for(int i=0;i<e.arg_list().entries();i++){
		Expression & sub = e.arg_list()[i];
		if(expr_type_check(sub)==1) {
			temp.push_back(&sub);
		}
		else{
			split_binary(pgm,sub,v,counter);
			temp.push_back(&v[v.size()-1]->lhs());
		}
		 
		if(temp.size()==2){
			counter++;
			Expression * left_temp =  new_variable(new_temp_name(counter), sub.type(), pgm);
			Expression * right_temp;
			
			if(expr_op_check(e)==3){
				right_temp = new NonBinaryExpr(e.op(),expr_type(e.op(),temp[0]->type(),temp[1]->type()),temp[0]->clone(), temp[1]->clone());
				
			}
			else if(expr_op_check(e)==2){
				right_temp = new BinaryExpr(e.op(),e.type(),temp[0]->clone(), temp[1]->clone());
				 
			}
			Statement * n =  new AssignmentStmt(stmts.new_tag(), left_temp->clone(), right_temp->clone()); 
			v.push_back(n);
			temp.clear();
			temp.push_back(left_temp);
			
		}
		

	}
	
	

 	
 


}




void print_in_out(StmtList & stmts, map<Statement *, RefSet<Expression> > & IN, map<Statement *, RefSet<Expression> >  & OUT ){
	cout << "AFTER AVAILEXPR\n";
	for(int i=0;i< stmts.entries();i++){
	 		Statement * st = & stmts[i];  
	 		st->print(cout);
	 		cout<<" IN = ";
	 		for(int i=0;i<IN[st].entries();i++){
	 			IN[st]._element(i).print(cout);
	 			cout<<" ,";
	 		} 
	 		cout<<" OUT = ";
	 		for(int i=0;i<OUT[st].entries();i++){
	 			OUT[st]._element(i).print(cout);
	 			cout<<" ,";
	 		}
	 		cout<<"\n---------------\n\n";

	 }
}


RefSet<Expression> egen(Statement *s){
	RefSet<Expression> * gen = new RefSet<Expression>;
	if(s->stmt_class() == ASSIGNMENT_STMT){
		Expression & r = s->rhs();
		if(r.op()!=INTEGER_CONSTANT_OP && r.op()!= REAL_CONSTANT_OP && r.op()!= ID_OP && r.op()!=LOGICAL_CONSTANT_OP && !expr_contains(r,s->lhs())){
			gen->ins(r);
		}
		
	}
	else if(s->stmt_class() == IF_STMT||s->stmt_class() == WHILE_STMT){
		Expression & expr = s->expr();

		Iterator <Expression> expr_it = expr.arg_list();
		for(;expr_it.valid();++expr_it){
			Expression  &e =  expr_it.current(); 
			if(e.op()!=INTEGER_CONSTANT_OP && e.op()!= REAL_CONSTANT_OP  && e.op()!=LOGICAL_CONSTANT_OP && e.op()!= ID_OP ){
				gen->ins(e);
			}
		}

	}
	return *gen;
}

bool expr_contains(Expression & big, Expression & small){

	if(big==small){
		return true;
	}
	else{
		Iterator <Expression> ep_it = big.arg_list(); 

		for(;ep_it.valid();++ep_it){
			Expression & e = ep_it.current();
			if(expr_contains(e,small)){
				return true;
			}
		}
	}
	return false;
}
 


RefSet<Expression> ekill(Statement *s, RefSet<Expression> &U){
	RefSet<Expression> * kill = new RefSet<Expression>;
	if(s->stmt_class() == ASSIGNMENT_STMT){
		Expression & left = s->lhs();
		for(int i=0;i<U.entries();i++){
			Expression & c = U._element(i);
			if(expr_contains(c,left)){ 
				kill->ins(c);
			}

		}


	}
	
	return *kill;

}

RefSet<Expression> getU(ProgramUnit & pgm){
	RefSet<Expression> * U = new RefSet<Expression>;
	Iterator<Statement> stmt_it = pgm.stmts().iterator();
	for(;stmt_it.valid();++stmt_it){
 		Statement * s = & stmt_it.current(); 
 		if(s->stmt_class() == ASSIGNMENT_STMT){
			Expression & r = s->rhs();
			if(r.op()!=INTEGER_CONSTANT_OP && r.op()!= REAL_CONSTANT_OP){
				U->ins(r);
			}
		}
		else if(s->stmt_class() == IF_STMT){
			Iterator <Expression> expr_it = s->iterate_expressions();
			for(;expr_it.valid();++expr_it){
				Expression  &e =  expr_it.current(); 
				U->ins(e);
			}

		}
 	}

 	return *U;
}


void cal_available_expression(ProgramUnit & pgm, map<Statement *, RefSet<Expression> >  &IN, map<Statement *, RefSet<Expression> >  &OUT){ //round robin version
	 
	RefSet<Expression> U = getU(pgm);
	/*
 	 
 	for(Iterator<Statement> stmt_it = pgm.stmts().iterator();stmt_it.valid();++stmt_it){
 		Statement * st = & stmt_it.current(); 
 		st->print(cout);
 		RefSet<Expression> gen;
 		RefSet<Expression> kill;
 		gen = egen(st);
 		kill = ekill(st,U);
 		cout<<"egen= ";
 		for(int i=0;i<gen.entries();i++){
 			gen._element(i).print(cout);
 			cout<<",";
 		}
 		cout<<" \n kill = ";
 		for(int i=0;i<kill.entries();i++){
 			kill._element(i).print(cout);
 			cout<<",";
 		}
 		cout<<" \n";

 	} 
	*/

  

	Iterator<Statement> stmt_it = pgm.stmts().iterator();
 	for(;stmt_it.valid();++stmt_it){
 		Statement * st = & stmt_it.current(); 
 		if(st== & pgm.stmts().first()){
 			OUT[st] = egen(st);
 		} 
 		else{
 			OUT[st] = U;
 			OUT[st] -= ekill(st,U);
 		}
 	}
 	bool change = true; 
 	while(change){
		change = false;
		for (stmt_it = pgm.stmts().iterator();stmt_it.valid();++stmt_it){
			Statement * st = & stmt_it.current(); 
			if(st== & pgm.stmts().first()) continue;

			RefSet<Statement> pred = st->pred();
			 
			for(int i=0;i< pred.entries();i++){
				if(i==0){
					IN[st] = OUT[&pred._element(i)];
				}
				else{
					IN[st] = intersect(IN[st],OUT[&pred._element(i)]);
				}

			}
			/*
			if(pred.entries()==2){

				cout<<"doing on";
				st->print(cout);
				RefSet<Expression> z = intersect(OUT[&pred._element(0)],OUT[&pred._element(1)]);
				for(int i=0;i<z.entries();i++){
					z._element(i).print(cout);
					cout<<",";
				}
				cout<<"====\n";
			}
			*/

			RefSet<Expression> oldout = OUT[st];
  
			OUT[st] = egen(st);
			RefSet<Expression> z = IN[st];
			z -= ekill(st,U);

			for (int i=0;i<z.entries();i++){
				Expression & zi = z._element(i);
				if(!set_has_expr(OUT[st],zi)) OUT[st].ins(zi); 
				
			}

			if(! (OUT[st]==oldout) ){
				change=true;
			}

		}


    }  

}



int meets_condition(Statement *s, RefSet<Expression> &IN){
	if(s->stmt_class()==ASSIGNMENT_STMT){ 

		Expression &r = s->rhs();

		for(int i=0;i< IN.entries();i++){
			if(r==IN._element(i)){

				return 1;
			}
		}
 

	}
	return 0;
}

RefSet<Statement> get_prev_reaching(Expression &r, Statement * s, map<Statement *, RefSet<Expression> >  &OUT){
 

	RefSet<Statement>  reaching;
	RefSet<Statement> pr = s->pred();

	for(int i=0;i<pr.entries();i++){
		Statement * c = &pr._element(i); 
		if(c->stmt_class()==ASSIGNMENT_STMT){ 

			RefSet<Expression> outc = OUT[c];
			if(set_has_expr(outc,r)){
				if(c->rhs()==r) reaching.ins(*c);
				reaching += get_prev_reaching(r,c,OUT);
			}
		}
		else{  
			reaching += get_prev_reaching(r,c,OUT);
		}
		
	} 
	return reaching;
}

bool set_has_expr(RefSet<Expression> & outc, Expression &r){
	for(int i=0;i < outc.entries();i++){
		if(outc._element(i)==r){
			return true;
		}
	}
	return false;
}


void GCSE(ProgramUnit & pgm, map<Statement *, RefSet<Expression> >  &IN,map<Statement *, RefSet<Expression> >  &OUT){
	 
	StmtList & stmts = pgm.stmts();
	Iterator<Statement> stmt_it = stmts.iterator();
	RefSet<Statement> to_delete;
	string u = "u";
	int counter = 0; 
 	for(;stmt_it.valid();++stmt_it){ 
  
 		Statement * st = & stmt_it.current();   
 		int condi = meets_condition(st, IN[st]);
 		if(condi){  
 			Expression &r = st->rhs();
 			
 			RefSet<Statement> prev_reaching = get_prev_reaching(r,st,OUT);

 			Expression * left_expr = & st->lhs();
			Symbol * new_symbol = left_expr->symbol().clone();
			new_symbol->name((u+NumberToString(counter)).c_str());
			Expression * ID_expr = new IDExpr(new_symbol->type(), *new_symbol->clone());  
			/*
			cout<<"deal with";
			st->print(cout);
			for(int i=0;i<prev_reaching.entries();i++){
				prev_reaching._element(i).print(cout);
			}
			cout<<"-=-=-=-=-=-"<<endl;
			*/
 			for(int i=0;i<prev_reaching.entries();i++){ 
 				Statement * prev = & prev_reaching._element(i);
 
 				Expression * left_expr = & prev->lhs(); 

 				Symbol * w_symbol = left_expr->symbol().clone(); 
 
 				Symbol * u_symbol = w_symbol->clone();
 				u_symbol->name((u+NumberToString(counter)).c_str());

 				Expression * w_ = new IDExpr(w_symbol->type(), *w_symbol->clone()); 
 				Expression * u_ = new IDExpr(u_symbol->type(), *u_symbol->clone()); 
 			 
 				Statement * to_insert = new AssignmentStmt(stmts.new_tag(), w_->clone(), u_->clone());
 				stmts.ins_after(to_insert,prev);
 				Statement * to_insert_ = new AssignmentStmt(stmts.new_tag(), u_->clone(), prev->rhs().clone());	
 				stmts.ins_before(to_insert_,prev);
 				to_delete.ins(*prev);
 			} 
            
			 
			Statement * to_insert = new AssignmentStmt(stmts.new_tag(), left_expr->clone(), ID_expr->clone());	 
			stmts.ins_before(to_insert,st);
 			 
 			
 			to_delete.ins(*st);  
 			counter++;
 		}

 		

 	}
 	for(int i=0;i<to_delete.entries();i++){ 
 		pgm.stmts().del(to_delete._element(i));
 	}
 	

}

bool is_temp_var(Expression & e){
	string s = e.symbol().name_ref();  
	if(s.find("TEMP")!=string::npos ) return true;
	return false;
}

Expression * table_lookup(map <Expression*, Expression*> &table, Expression &e){

	for(map<Expression *, Expression* >::iterator  it = table.begin(); it != table.end(); it++){
		//it->first->print(cout);
		if(*it->first==e){
			return it->second;
		}
	}
	return NULL;
}

void replace_var(Mutator<Expression> &ep, map <Expression*, Expression*> &table){
	Expression & e = ep.current();
	/*
	cout<<"replacing ";
	e.print(cout);
	cout<<endl;
	*/
	if(e.op()==ID_OP && is_temp_var(e)){
		if(table_lookup(table,e)){ 
			ep.assign() = table_lookup(table,e)->clone(); 
		}
		else{
			cout<<"error: temp does not exist"<<endl;
		}
		return;
	}
	else{
		if(e.arg_list().entries()>1){
			Mutator<Expression> ep =e.arg_list();
			for(;ep.valid();++ep){
				replace_var(ep,table);
			}
		}
		
	}
}

void copy_propagation(ProgramUnit & pgm){ 
	map <Expression*, Expression*> table;
	StmtList & stmts = pgm.stmts();
	Iterator<Statement> stmt_it = stmts.iterator();
 	for(;stmt_it.valid();++stmt_it){
 		Statement * st = & stmt_it.current();  
 		if(st->stmt_class()==ASSIGNMENT_STMT){ 

 			Expression &left = st->lhs();
 			Expression &right = st->rhs();
 			if(is_temp_var(left)){

 				table[left.clone()] = right.clone();
 				stmts.del(*st);
 			}
 			else{
 				if(right.op()==ID_OP && is_temp_var(right)){
 					Statement * p = st->clone();
 					
 					for(int i=0;i<10;i++){   // it should be while no change. 
 						Mutator<Expression> ep = p->iterate_expressions();
	 					++ep;
	 					replace_var(ep,table);
 					} 
 					Statement * to_insert_ = new AssignmentStmt(stmts.new_tag(), left.clone(), p->rhs().clone());	
 					stmts.ins_before(to_insert_, st);
 					stmts.del(*st);

 				}

 			} 

 		}



 	}
 


}



template <class T>
RefSet<T> intersect(RefSet<T> &a, RefSet<T> &b){
	RefSet<T> c;
	for(int j=0;j<a.entries();j++){
		T & s =  a._element(j); 
		for(int i=0;i<b.entries();i++){
			if(b._element(i)==s){
				c.ins(s);
			}
		}
	}
	return c;
}
template <typename T>
std::string NumberToString ( T Number )
{
 std::ostringstream ss;
 ss << Number;
 return ss.str();
}
