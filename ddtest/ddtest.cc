// 
// ddtest.cc : Data Dependence Test pass
// 

#include "ddtest.h"
#include "StmtList.h"
#include "Collection/List.h"
#include "Statement/Statement.h"

#include "Expression/FunctionCallExpr.h"
#include "Expression/IDExpr.h" 
#include "Expression/BinaryExpr.h" 
#include "Expression/NonBinaryExpr.h" 


#include <string>
#include <vector>
#include <map> 
#include <stack>
#include <sstream>

using namespace std;






void ddtest(ProgramUnit & pgm,int dds_flow, int dds_output, int dds_anti, int dda_flow,int dda_output,int dda_anti){
	
	StmtList & stmts = pgm.stmts();
	vector<loop> L;

    for(int i=0;i<stmts.entries();i++){
		Statement * st = & stmts[i]; 
		if(st->stmt_class()==DO_STMT){

			loop l;
			l.init = & st->init();
			l.limit = & st->limit(); 
			l.index = & st->index();
			l.step = & st->step();
			l.tag = st->tag();
			for(int j=i+1;j<stmts.entries();j++){
				Statement * p = & stmts[j];
				if(p->stmt_class()==ASSIGNMENT_STMT){
					l.stmts.push_back(p);
				}
				if(p->stmt_class()==ENDDO_STMT){
					break;
				}

			}
			L.push_back(l);


		}
	} 

	for(int i=0;i<L.size();i++){
		cout<<"LOOP "<<L[i].tag<<" contains the following dependencies :"<<endl;
		if(dds_flow) dds_flow_(L[i]);
		if(dds_output) dds_output_(L[i]);
		if(dds_anti) dds_anti_(L[i]);

		if(dda_flow) dda_flow_(L[i]);
		if(dda_output) dda_output_(L[i]);
		if(dda_anti) dda_anti_(L[i]);

	}




}


void dds_flow_(loop & l){
	vector<Statement *> s = l.stmts;
	map<Statement *, Statement * > m;
	for(int i=0;i<s.size();i++){
		Statement * cur = s[i];
		Expression &left = cur->lhs();
		if(left.op()==ARRAY_REF_OP) continue;
		for(int j=i;j < s.size();j++){
			Statement * p = s[j];
			Expression & right = p->rhs();
			if(contain_expr(right,left)){
						 
				m[cur] = p;

			}

		}


	}
	if(m.empty()) return;
	cout<<"Scalar Flow :"<<endl;
	for(map<Statement *, Statement * >::iterator it = m.begin();it!=m.end();it++ ){
		cout<<"             "<< it->first->tag()<<" to "<<  it->second->tag()<<endl;
		//cout<<" (";it->first->lhs().print(cout);cout<<",";it->second->rhs().print(cout);cout<<")"<<endl; 
	}



}


void dds_output_(loop & l){
	vector<Statement *> s = l.stmts;
	map<Statement *, Statement * > m;
	for(int i=0;i<s.size();i++){
		Statement * cur = s[i];
		Expression &left = cur->lhs();
		if(left.op()==ARRAY_REF_OP) continue;
		for(int j=i+1;j < s.size();j++){
			Statement * p = s[j];
			Expression & left2 = p->lhs();
			if(contain_expr(left2,left)){
				m[cur] = p;
			}

		}

	}
	if(m.empty()) return;
	cout<<"Scalar Output :"<<endl;
	for(map<Statement *, Statement * >::iterator it = m.begin();it!=m.end();it++ ){
		cout<< "             "<< it->first->tag()<<" to "<<  it->second->tag()<<endl;
		//cout<<" (";it->first->lhs().print(cout);cout<<",";it->second->rhs().print(cout);cout<<")"<<endl; 
	}

}


void dds_anti_(loop &l){
	vector<Statement *> s = l.stmts;
	map<Statement *, Statement * > m;
	for(int i=0;i<s.size();i++){
		Statement * cur = s[s.size()-1-i];
		Expression &left = cur->lhs();

		if(left.op()==ARRAY_REF_OP) continue;

		for(int j=i;j < s.size();j++){
			Statement * p = s[s.size()-1-j];
			Expression & right = p->rhs();
			if(contain_expr(right,left)){
				m[cur] = p;
			}
		}
	}
	if(m.empty()) return;

	cout<<"Scalar Anti :"<<endl;
	for(map<Statement *, Statement * >::iterator it = m.begin();it!=m.end();it++ ){
		cout<< "             "<< it->first->tag()<<" to "<<  it->second->tag()<<endl;
		//cout<<" (";it->first->lhs().print(cout);cout<<",";it->second->rhs().print(cout);cout<<")"<<endl; 
	}
}


void dda_flow_(loop &l){  
	vector<Statement *> s = l.stmts;
	map<Statement *, Statement * > m;
	map<Statement *, Expression * > E1;
	map<Statement *, Expression * > E2;

	for(int i=0;i<s.size();i++){
		Statement * cur = s[i];
		Expression &left = cur->lhs();
		if(left.op()!=ARRAY_REF_OP)  continue;
		vector<Expression *> v;
		for(int j=0;j < s.size();j++){ 
			Statement * p = s[j];
			Expression & right = p->rhs();

			find_array(right,left.array(),v);


			for(int k=0;k<v.size();k++){   
				Expression & a1 = v[k]->subscript();
				Expression & a2 = left.subscript();

				//a1.print(cout);
				//cout<<' ';
				//a2.print(cout);

				//cout<<endl;
				
				if(has_integer_solution(a1,a2,l)==1){
					E1[cur] = v[k];
					E2[cur] = &left;
					m[cur] = p;
					break;
				}

			}	
			
		}
	}
	if(m.empty()) return;
	cout<<"Array Flow :"<<endl;
	for(map<Statement *, Statement * >::iterator it = m.begin();it!=m.end();it++ ){
		cout<< "             "<< it->first->tag()<<" to "<<  it->second->tag()<<" ";
		cout<<" (";E1[it->first]->print(cout);cout<<" , ";E2[it->first]->print(cout);cout<<")"<<endl;

	}

}

void dda_anti_(loop &l){
	vector<Statement *> s = l.stmts;
	map<Statement *, Statement * > m;

	map<Statement *, Expression * > E1;
	map<Statement *, Expression * > E2;

	for(int i=0;i<s.size();i++){
		Statement * cur = s[i];
		Expression &left = cur->lhs();
		if(left.op()!=ARRAY_REF_OP)  continue;
		vector<Expression *> v;
		for(int j=0;j < s.size();j++){
			Statement * p = s[j];
			Expression & right = p->rhs();

			find_array(right,left.array(),v);


			for(int k=0;k<v.size();k++){ 
				Expression & a1 = v[k]->subscript();
				Expression & a2 = left.subscript();

				//a1.print(cout);
				//cout<<' ';
				//a2.print(cout);
				//cout<<endl;
				
				if(has_integer_solution(a1,a2,l)==2){
					E1[cur] = v[k];
					E2[cur] = &left;
					m[cur] = p;
					break;
				}

			}
			
		}
	}
	if(m.empty()) return;
	cout<<"Array Anti :"<<endl;
	for(map<Statement *, Statement * >::iterator it = m.begin();it!=m.end();it++ ){
		cout<< "             "<< it->first->tag()<<" to "<<  it->second->tag();
		cout<<" (";E1[it->first]->print(cout);cout<<" , ";E2[it->first]->print(cout);cout<<")"<<endl;
	}
}

void dda_output_(loop &l){
	vector<Statement *> s = l.stmts;
	map<Statement *, Statement * > m;

	map<Statement *, Expression * > E1;
	map<Statement *, Expression * > E2;

	for(int i=0;i<s.size();i++){
		Statement * cur = s[i];
		Expression &left = cur->lhs();
		if(left.op()!=ARRAY_REF_OP)  continue;
		vector<Expression *> v;
		for(int j=i+1;j < s.size();j++){ 
			Statement * p = s[j];
			Expression & left2 = p->lhs();

			find_array(left2,left.array(),v);


			for(int k=0;k<v.size();k++){
				Expression & a1 = v[k]->subscript();
				Expression & a2 = left.subscript();

				//a1.print(cout);
				//cout<<' ';
				//a2.print(cout);
				//cout<<endl;
				
				if(has_integer_solution(a1,a2,l)>0){
					m[cur] = p;
					E1[cur] = v[k];
					E2[cur] = &left;
					break;
				}

			}
			
		}
	}
	if(m.empty()) return;
	cout<<"Array Output :"<<endl;
	for(map<Statement *, Statement * >::iterator it = m.begin();it!=m.end();it++ ){
		cout<< "             "<< it->first->tag()<<" to "<<  it->second->tag();
		
		cout<<" (";E1[it->first]->print(cout);cout<<" , ";E2[it->first]->print(cout);cout<<")"<<endl;
	}
}

bool is_interger_constant(Expression &e){
	List<Expression> L = e.arg_list();
	if(L.entries()==1){
		if(L[0].op()==INTEGER_CONSTANT_OP){
			return true;
		}
	}
	return false;
}


int has_integer_solution(Expression &a1, Expression &a2, loop & l){ // 1 for flow, 2 for anti

	//cout<<a1.op()<<" "<<a2.op()<<" "<<COMMA_OP<<endl;

	if(is_interger_constant(a1) && is_interger_constant(a2)){
		if(a1==a2){
			return 1;
		}
	}
	else if (is_interger_constant(a1) && !is_interger_constant(a2)){ 
	    // a[1] = ...
	    //  ... = a[j]   
		int coefficient_2;
		int constant_2   ;
		find_coefficient(a2,*l.index,coefficient_2,constant_2);
 

		int consts = a1.arg_list()[0].value() - constant_2; 
		if(consts% coefficient_2 ==0){
			int solution = consts/coefficient_2;
			if(solution<= l.limit->value() && solution>= l.init->value()){
				if(solution >= a1.arg_list()[0].value()) return 1;
				return 2; 
			}
		}

	}
	else if(is_interger_constant(a2) && !is_interger_constant(a1)){
		// a[j] = ...
	    //  ... = a[10]  
 
		int coefficient_1=0;
		int constant_1=0   ;

		find_coefficient(a1,*l.index,coefficient_1,constant_1);

		int consts = a2.arg_list()[0].value() - constant_1;
		if(consts% coefficient_1 ==0){
			int solution = consts/coefficient_1;
			if(solution<= l.limit->value() && solution>= l.init->value()){
				if(solution < a2.arg_list()[0].value()) return 1;
				return 2; 
			}
		}

	}
	else{ 
		//a1.print(cout);cout<<"  ";a2.print(cout);

		int coefficient_1=0;
		int constant_1 =0  ;

		find_coefficient(a1,*l.index,coefficient_1,constant_1);

		//cout<<(coefficient_1)<<"i op "<<(constant_1 )<<endl;
		 

		int coefficient_2=0;
		int constant_2 =0  ;
		find_coefficient(a2,*l.index,coefficient_2,constant_2);

		//cout<<(coefficient_2)<<"i op "<<(constant_2 )<<endl;

		int consts = constant_1 - constant_2;
		int GCD = gcd(abs(coefficient_1),abs(coefficient_2)); 

		if(abs(consts) % GCD ==0){
			return solve_diophantine_equation_in_range(l,consts, coefficient_1, coefficient_2);
		}  

	}
	return 0;

}

int solve_diophantine_equation_in_range(loop &l, int consts, int coefficient_1, int coefficient_2){
	for(int i= l.init->value(); i <= l.limit->value();i++){
		int k = consts + coefficient_1 * i;
		if(k % coefficient_2==0){
			int m = k / coefficient_2;
			if(m<= l.limit->value() && m>=l.init->value()){
				if(m<=i){
					return 1;
				}
				return 2;
			}
		}
	}
	return 0;

}



void find_coefficient(Expression &e, Expression &index, int & coeffic, int & constant){

	//cout<<"======\n";
	//e.print(cout);
	//cout<<" ";
	//index.print(cout);
	//cout<<"\n"; 
	
	Expression * E = simplify(&e);

	List<Expression> L = E->arg_list();
	if(L.entries()==1){
		if(L[0].op()==INTEGER_CONSTANT_OP){
			coeffic = 0;
			constant = L[0].value();
		}
		else if(L[0]==index){
			coeffic = 1;
			constant = 0;
		}
		else{
			List<Expression> q = L[0].arg_list();
	//cout<<E->arg_list()[0].arg_list().entries()<<endl;
			for(int i=0;i<q.entries();i++){
				if(q[i].op()==INTEGER_CONSTANT_OP){
					constant = q[i].value();
				}
				else if(q[i]==index){
					coeffic = 1;
				}
				else if(q[i].op()==MULT_OP){ 
					List<Expression> L = q[i].arg_list();
					if(L.entries()!=2) return; 
					if(L[0]==index && L[1].op()==INTEGER_CONSTANT_OP){
						coeffic = L[1].value();
						 
					}
					else if(L[1]==index && L[0].op()==INTEGER_CONSTANT_OP){
						coeffic = L[0].value();
						 
					}
				}

			}
			 

		}
	}



	
	 
}



void find_array(Expression &e, Expression &index, vector<Expression *> &v){
	if(e.op()==ARRAY_REF_OP){
		if(e.array()==index){
			v.push_back(&e);
		}
	}
	else{
		Iterator <Expression> ep =e.arg_list();
		for(;ep.valid();++ep){
			find_array(ep.current(),index,v); 
		}

	} 

}

bool contain_expr(Expression &e, Expression & x){
	if(e==x){
		return true;
	}
		
	else{
		Iterator <Expression> ep =e.arg_list();
		for(;ep.valid();++ep){
			bool c = contain_expr(ep.current(),x);
			if(c) return true;
		}

	}
	return false;

}
int gcd(int a, int b) {
    return b == 0 ? a : gcd(b, a % b);
}
