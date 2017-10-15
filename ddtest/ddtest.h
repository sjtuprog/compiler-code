// 
// ddtest.h : Data Dependence Test pass
// 

#ifndef _DDTEST_H
#define _DDTEST_H

#include "ProgramUnit.h"

#include <string>
#include <vector>
#include <map> 
#include <stack>
#include <sstream>

using namespace std;

struct loop{
	Expression * init;
	Expression * limit;
	Expression * step;
	Expression * index;
	string tag;
	vector<Statement *> stmts;
	void print(){
		cout<<" index= ";
		index->print(cout);
		cout<<endl;
		cout<<" init= ";
		init->print(cout);
		cout<<endl;
		cout<<" limit= ";
		limit->print(cout);
		cout<<endl;
		cout<<" step= ";
		step->print(cout);
		cout<<endl;
		for(int i=0;i<stmts.size();i++){
			stmts[i]->print(cout);
		}
		cout<<"======="<<endl;

	}

};


void ddtest(ProgramUnit & pgm,int dds_flow, int dds_output, int dds_anti, int dda_flow, int dda_output,int dda_anti);
int has_integer_solution(Expression &a1, Expression &a2, loop &l);
void find_array(Expression &e, Expression &index, vector<Expression *> &v);
bool contain_expr(Expression &e, Expression & x);
bool is_interger_constant(Expression &e);
int solve_diophantine_equation_in_range(loop &l, int consts, int coefficient_1, int coefficient_2);
void find_coefficient(Expression &e, Expression &index, int & coeffic, int & constant);
int gcd(int a, int b);
void dds_flow_(loop & l);
void dds_output_(loop & l);
void dds_anti_(loop &l);
void dda_flow_(loop &l);
void dda_anti_(loop &l);
void dda_output_(loop &l); 


#endif
