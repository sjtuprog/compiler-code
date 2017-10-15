// 
// ssa.cc : SSA transformations
// https://www.cs.purdue.edu/homes/hosking/502/notes/14-dep.pdf
// https://parasol.tamu.edu/~rwerger/Courses/605/ssa_wt.pdf
// http://cs.stackexchange.com/questions/24112/algorithm-to-find-dominance-frontiers

#include "ssa.h"
#include "StmtList.h"
#include "Collection/List.h"
#include "Statement/Statement.h"
#include "Statement/AssignmentStmt.h"
#include "Expression/FunctionCallExpr.h"
#include "Expression/IDExpr.h" 

#include "bblock/bblock.h"
#include "bblock/BasicBlock.h"
#include <string>
#include <vector>
#include <map> 
#include <stack>
#include <sstream>
using namespace std;

void ssa(ProgramUnit & pgm, List<BasicBlock> * bb)
{
	map<BasicBlock *, RefSet<BasicBlock> >  DOM;
	map<BasicBlock *, RefSet<BasicBlock> >  rDOM; // s is dominated by refset
	map<BasicBlock *, BasicBlock*> idom;
	map<Symbol *, RefSet<BasicBlock> > var_table;
	map<BasicBlock *, RefSet<BasicBlock> > DF;
	map<const char*, int> visited;
	StmtList & stmts = pgm.stmts();

	rDOM = find_dom(bb);
	

	DOM = reverse_dom(rDOM);


	//print_dom(DOM);
	
	idom = find_idom(rDOM,DOM);

	//print_idom(idom);

	
	
	DF = find_df(bb,idom); // here is a bug inside 

	//print_dom(DF); 
	

	var_table = left_var_table(bb);
 
	//print_table(var_table); 
	
	insert_phi_nodes(pgm,bb,var_table,DF); 
	
	map<Symbol *,int> Counters;
	
	map<Symbol *, stack<int> > Stack;

	map<Symbol *, RefSet<BasicBlock> >::iterator it; 

	for ( it = var_table.begin(); it != var_table.end(); it++ ){
		Counters[it->first] = 0;
		Stack[it->first] = stack<int>();
		Stack[it->first].push(0);
	}
	
	
	
	for(int i=0;i<bb->entries();i++){
		visited[(*bb)[i].bb_name] = 0;

	}

	Rename(&(*bb)[0], Counters,Stack, DOM,visited);
	Iterator<Statement> stmt_it = pgm.stmts().iterator(); 
	cout<<"============="<<endl;
	for(;stmt_it.valid();++stmt_it){
		stmt_it.current().print(cout);
	}
	cout<<"-----########------"<<endl;
	pgm.write(cout);
	cout<<"finished"<<endl;
	return; 
}
void dessa(ProgramUnit & pgm, List<BasicBlock> * bb){
	StmtList & stmts = pgm.stmts();
	
	for(Iterator<Statement> it = stmts.iterator();it.valid();++it){
		Statement * p = & it.current();
		
		if(is_phi_stmt(p)){
			stmts.del(*p);
		}
		else if(p->stmt_class()==ASSIGNMENT_STMT){
			Expression * e = & p->lhs();
			Expression * f = & p->rhs();
			recover_var_name(e);
			recover_var_name(f);
		}
		else if(p->stmt_class()==IF_STMT){
			Expression * e = & p->expr();
			recover_var_name(e);
		}
		else if(p->stmt_class()==PRINT_STMT){
			Expression * e = & p->io_list_guarded();
			recover_var_name(e);
		}
		else if(p->stmt_class()==CALL_STMT){
			Expression * e = & p->parameters_guarded();
			recover_var_name(e);
		}
	}
     
}

void recover_var_name(Expression * expr){
	if(expr->op()==ID_OP){ 
		string n = expr->symbol().name_ref();
		if(n.find('@')!=string::npos){
			n = cut(n);
			expr->symbol().name(n.c_str());
		}
		
		return;
	}
	else{
		Iterator <Expression> ep_it = expr->arg_list(); 
		for(;ep_it.valid();++ep_it){
			recover_var_name(&ep_it.current());
		}
		
	}

}



inline void print_dom(map<BasicBlock *, RefSet<BasicBlock> >  & DOM){
	for (map<BasicBlock *, RefSet<BasicBlock> >::iterator  it = DOM.begin(); it != DOM.end(); it++ ){
		BasicBlock * a = it->first;
		RefSet<BasicBlock> &b = it->second;
		cout<<a->bb_name;
		cout<<" has DF of ";
		//cout<<" doms ";
		for(int i=0;i<b.entries();i++){
			cout<<b._element(i).bb_name<<' ';
		}  
		cout<<endl;
	} 
}
inline void print_idom(map<BasicBlock *, BasicBlock *> & idom){
	for (map<BasicBlock *,BasicBlock * >::iterator  it = idom.begin(); it != idom.end(); it++ ){
		BasicBlock * a = it->first;
		BasicBlock * b = it->second;
		cout<<a->bb_name;
		cout<<" idom by ";
		cout<<b->bb_name;
		cout<<endl;
	} 
}
inline void print_table(map<Symbol *, RefSet<BasicBlock> > & table){
	for (map<Symbol *,RefSet<BasicBlock> >::iterator  it = table.begin(); it != table.end(); it++ ){
		Symbol * a = it->first;
		RefSet<BasicBlock> &b = it->second;
		a->print(cout);
		cout<<" in stmts of ";
		for(int i=0;i<b.entries();i++){
			cout<<b._element(i).bb_name<<' ';
		}  
		cout<<endl;
	} 
}




bool is_phi_stmt(Statement *s){
	if(s->stmt_class()== ASSIGNMENT_STMT){
		Expression & right = s->rhs();
		if(right.op()==FUNCTION_CALL_OP){ 
			Expression & func = right.function();
			string name = func.symbol().name_ref();
			if(name=="PHI"){ 
				return true;
			}
		}


	}
	return false;
}

Expression* unnamed_subexpr(Expression & para){
	Iterator <Expression> ep_it = para.arg_list();
	for(;ep_it.valid();++ep_it){
		string x = ep_it.current().symbol().name_ref();
		if(x.find("@")==std::string::npos) return &ep_it.current();
	}
	return NULL;
}


void Rename(BasicBlock * x, map<Symbol *,int> & Counters, map<Symbol *, stack<int> > & Stack,
	map<BasicBlock *, RefSet<BasicBlock> > & DOM, map<const char*, int> & visited){

	if(visited[x->bb_name]==1) return;
	visited[x->bb_name]=1;

	//cout<<"renaming "<<x->bb_name<<endl; 

	for(int i=0;i<x->statements().size();i++){
		Statement * cur = x->statements()[i];
		if(is_phi_stmt(cur)){   //phi node 
			//cout<<"gen name for left part of phi stmt "<<endl;
			//cur->print(cout);
			gen_name(& cur->lhs(),Counters, Stack);
			//cout<<"f name for phi stmt "<<endl;
			//cur->print(cout);
		}
	}
	//cout<<"1"<<endl;
	for(int i=0;i<x->statements().size();i++){ 
		Statement * cur = x->statements()[i]; 
		//cout<<"woop "<<i<<endl;
		if(cur->stmt_class()== ASSIGNMENT_STMT && !is_phi_stmt(cur)){
			Expression * right = & cur->rhs();
			Expression * left = & cur->lhs();
			rename_expr(right, Stack); 
			gen_name(left,Counters, Stack); 

				 
			
		}
		else if(cur->stmt_class()== IF_STMT){
			Expression * if_expr = & cur->expr();
			rename_expr(if_expr, Stack);  
		}
		else if(cur->stmt_class()== PRINT_STMT){
			Expression * pr_expr = & cur->io_list_guarded();
			rename_expr(pr_expr, Stack);  
		}
		else if(cur->stmt_class()== CALL_STMT){
			Expression * pr_expr = & cur->parameters_guarded();
			rename_expr(pr_expr, Stack);  
		}
		//cout<<"wo3op "<<i<<endl;
	}  
	//cout<<"2"<<endl;
 
	vector<BasicBlock*> succ = x->succ();
	for(int i=0;i<succ.size();i++){
		BasicBlock * y = succ[i];

		//cout<<"i="<<i<<endl;

		for(int j=0;j< y->statements().size();j++){
			if(is_phi_stmt(y->statements()[j])){
				//cout<<"rename for args of phi stmt"<<endl;
				//y->statements()[j]->print(cout);
				
				Expression & para = y->statements()[j]->rhs().parameters_guarded();  
				Expression * to_replace = unnamed_subexpr(para);  
				if(to_replace==NULL) continue; 
				string n = to_replace->symbol().name_ref();    
				

				Symbol * V = &y->statements()[j]->lhs().symbol();

				if(Stack.find(V)==Stack.end()){
					for(map<Symbol *, stack<int> >::iterator it = Stack.begin();it!=Stack.end();it++){
						string a = it->first->name_ref();
						string b = V->name_ref();
						if(a==b){
							V = it->first;
						}
					}
				}


				cout<<V->name_ref()<<endl;
				int k = Stack[V].top();   
				n = cut(n); 
				n = n + '@' + to_string(k);
				to_replace->symbol().name(n.c_str());
				//cout<<"finish name for phi stmt"<<endl;
				//y->statements()[j]->print(cout);
			}
		}
	} 

	//cout<<"3"<<endl;





	for(int i=0;i<DOM[x].entries();i++){ // rename successor in dom tree 
		Rename(&DOM[x]._element(i),Counters,Stack,DOM,visited);
		 
	}
	for(int i=0;i<x->statements().size();i++){
		Statement * cur = x->statements()[i];
		if(cur->stmt_class()== ASSIGNMENT_STMT){ // V_i on the left
			Expression * left = & cur->lhs();
			string n = left->symbol().name_ref();
			if(n.find("@") != string::npos){ 
				//Expression * mapped_var = get_mapped_var(Stack,left);
				
				pop_old_symbol(Stack,n);
			}

		}
	}
	 
	//cout<<"finishing "<<x->bb_name<<endl;
	return;

}


void pop_old_symbol(map<Symbol *, stack<int> > &Stack, string n){
	string origin_name = cut(n);
	
	map<Symbol *, stack<int> > :: iterator it;
	for(it=Stack.begin();it!=Stack.end();it++){
		string name = it->first->name_ref();
		if(name==origin_name){
			Stack[it->first].pop();
			return;
		}
	}

}
 


void rename_expr(Expression * expr, map<Symbol *, stack<int> > &Stack){

	//cout<<"rename expresion = ";
	//expr->print(cout);
	//cout<<endl;
	if(expr->op()==ID_OP){ 
		if(Stack.find(&expr->symbol()) != Stack.end()){
			int i = Stack[&expr->symbol()].top(); 
			string n = expr->symbol().name_ref();
			n = cut(n);
			n = n + '@' + to_string(i);
			expr->symbol().name(n.c_str());
		}
		
		return;
	}
	else{
		Iterator <Expression> ep_it = expr->arg_list(); 
		for(;ep_it.valid();++ep_it){
			rename_expr(&ep_it.current(),Stack);
		}
		
	}

}

string to_string(unsigned int i){
	stringstream ss;
	ss << i;
	return ss.str();
}

string cut(string n){
	int i=0;
	string s="";
	while(i<n.length() && n[i]!='@'){
		s += n[i];
		i++;
	}
	return s;
}

void gen_name(Expression * var, map<Symbol *,int> &Counters, map<Symbol *, stack<int> > &Stack){
	//cout<<"gen name for ";
	//var->print(cout);
	//cout<<endl; 
	Symbol * old_sym = &var->symbol();
	int i = Counters[old_sym];  
	string n = old_sym->name_ref();
	n = cut(n);
	n = n+ '@' + to_string(i);  
	Symbol * new_sym = old_sym->clone(); 
	new_sym->name(n.c_str());   
	var->symbol(*(new_sym->clone()));
	Stack[old_sym].push(i); 
	Counters[old_sym] = i + 1; 
	//cout<<"gen name into ";
	//var->print(cout);
	//cout<<endl; 
	return;
}

map<Symbol *, RefSet<BasicBlock> > left_var_table(List<BasicBlock> *bbl){
	map<Symbol *, RefSet<BasicBlock> > table; 

	for(int i=0; i < bbl->entries();i++){
		BasicBlock * bb = &(*bbl)[i]; 
		vector<Statement*> ss = bb->statements();
		for(int j=0;j<ss.size();j++){
			Statement * s = ss[j];
			if(s->stmt_class() == ASSIGNMENT_STMT){
				Expression * e = & s->lhs();
				Symbol * q = & e->symbol();
				if(!table[q].member(*bb)){
					table[q].ins(*bb);
				}
				
			}
		}

	}
 
	return table; 
}

template <class T> 
bool vector_has_element(vector<T*> &a, T * b){
	for(int i=0;i<int(a.size());i++){
		if(a[i]==b) return true;
	}
	return false;
}

void insert_phi_nodes(ProgramUnit & pgm, List<BasicBlock> *bbl,map<Symbol *, RefSet<BasicBlock> > & var_table,
	map<BasicBlock *, RefSet<BasicBlock> > &DF){

	vector<BasicBlock*> has_already;
	vector<BasicBlock*> worklist;
	StmtList & stmts = pgm.stmts();
	map<Symbol  *, RefSet<BasicBlock> >::iterator it;
	Expression* phi_func = new_function("PHI", *(new Type(INTEGER_TYPE)), pgm);
	

	for ( it = var_table.begin(); it != var_table.end(); it++ ){
		Symbol * var = it->first; 
		 
		has_already.clear();
		worklist.clear();
		RefSet<BasicBlock> related = it->second;
		for(int i=0;i<related.entries();i++){
			worklist.push_back(&related._element(i));
		}

		while(!worklist.empty()){
			BasicBlock * x = worklist.back();
			worklist.pop_back();
 
			for(int j=0;j<DF[x].entries();j++){
				BasicBlock * y = & DF[x]._element(j);

				if(!vector_has_element(has_already,y)){  //insert a phi node for var at y

					Symbol * left_symbol = var->clone();
					List<Expression> *L = new List<Expression> ;
					for(int k=0;k<y->pred().size();k++) {
						Expression * arg = new IDExpr(left_symbol->type(), *(left_symbol->clone()));
						L->ins(&*arg,k);
					} 
					//Expression* args = comma(&L);  //new CommaExpr

					Expression* args = new CommaExpr(L); 
					 
					//Expression* right_expr = function_call(phi_func->clone(), args->clone()); //new FunctionCallExpr
					Expression* right_expr = new FunctionCallExpr(*(new Type(INTEGER_TYPE)), phi_func->clone(),args->clone());//phi_func->clone(), args->clone()); 
					Expression* left_expr = new IDExpr(left_symbol->type(), *left_symbol); 
					Statement* phi_stmt = new AssignmentStmt(stmts.new_tag(), left_expr->clone(), right_expr->clone());	
					//cout<<" is phi stmt = "<<is_phi_stmt(phi_stmt)<<endl;
					
					stmts.ins_after(phi_stmt,y->get_lead_stmt());
					y->insert_stmt(phi_stmt);
					
					has_already.push_back(y);
					if(!vector_has_element(worklist,y)) worklist.push_back(y);
				}
			}
		}

	} 
 

}


map<BasicBlock *, RefSet<BasicBlock> > find_df(List<BasicBlock> *bbl, map<BasicBlock *, BasicBlock*> & idom){
	map<BasicBlock *, RefSet<BasicBlock> >  DF;  
	
	for(int k=0;k<bbl->entries();k++){
		BasicBlock * bb = &(*bbl)[k]; 
		vector<BasicBlock*> pred = bb->pred();    
		if(pred.size()>1){ 
			for(int i=0;i<pred.size();i++){   
				BasicBlock *runner  = pred[i];  
				while(runner != idom[bb]){
					//cout<<runner->tag()<<endl;
					DF[runner].ins(*bb);
					runner = idom[runner]; 
				}
			}
		} 
		
	}

	return DF;

}

map<BasicBlock *, RefSet<BasicBlock> > reverse_dom(map<BasicBlock *, RefSet<BasicBlock> >  &DOM){
	map<BasicBlock *, RefSet<BasicBlock> >  rDOM;
	map<BasicBlock *, RefSet<BasicBlock> >::iterator it;
	 
	for ( it = DOM.begin(); it != DOM.end(); it++ ){ 
		BasicBlock * cur = it->first;
		RefSet<BasicBlock> doms = it->second; 
		for(int i=0;i<doms.entries();i++){
			BasicBlock * s = &doms._element(i);
			if(!rDOM[s].member(*cur)) rDOM[s].ins(*cur);
		} 
	} 
	return rDOM;
}


map<BasicBlock *, BasicBlock*> find_idom(map<BasicBlock *, RefSet<BasicBlock> > & rDOM, map<BasicBlock *, RefSet<BasicBlock> > & DOM){
	map<BasicBlock *, BasicBlock*> idom; 
	map<BasicBlock *, RefSet<BasicBlock> >::iterator it;
	for(it = rDOM.begin(); it!=rDOM.end(); it++){
		BasicBlock * s = it->first;
		RefSet<BasicBlock> rdoms = it->second;
		rdoms.del(*s);
		if(rdoms.empty()) continue;
		RefSet<BasicBlock> idom_s = DOM[&rdoms._element(0)];
		for(int j=1;j<rdoms.entries();j++){
			idom_s = intersect(idom_s,DOM[&rdoms._element(j)]);
		} 
		for(int i=0;i<idom_s.entries();i++){
			if(rdoms.member(idom_s._element(i))){
				idom[s] = &idom_s._element(i);
				//break;
				 
			}
		} 
 
	}
	/*
	map<Statement *, Statement * >::iterator it_;
	for(it_ = idom.begin(); it_ != idom.end(); it_++){
		ridom[it_->second] = it_->first; 
	}
	*/
	return idom;
}



map<BasicBlock *, RefSet<BasicBlock> > find_dom(List<BasicBlock> *bbl){
	map<BasicBlock *, RefSet<BasicBlock> >  DOM;  

	for(int i=0;i<bbl->entries();i++){
		BasicBlock * bb = &(*bbl)[i];
		DOM[bb].ins(*bb);
	}

	bool changed; 
	do{
		changed = false; 
		for(int i=1;i<bbl->entries();i++){
			BasicBlock * bb = &(*bbl)[i];
			RefSet<BasicBlock> old_dom = DOM[bb];
			RefSet<BasicBlock> new_dom;
			vector<BasicBlock*> pred = bb->pred();
			for(int j=0;j<pred.size();j++){
				if(j==0) new_dom = DOM[pred[j]];
				else{
					new_dom = intersect(new_dom,DOM[pred[j]]);
				}
				
			} 
			DOM[bb] = new_dom;
			if(!DOM[bb].member(*bb)) DOM[bb].ins(*bb);

			if( !(DOM[bb]==old_dom)) changed = true;
		}
		

	}while(changed);

	return DOM;
}



template <class T>
RefSet<T> intersect(RefSet<T> &a, RefSet<T> &b){
	RefSet<T> c;
	for(int j=0;j<a.entries();j++){
		T & s =  a._element(j); 
		if(b.member(s)){
			c.ins(s);
		}
				
	}
	return c;
}
