// 
// subexpr.h : constant propagation pass
// 

#ifndef _SUBEXPR_H
#define _SUBEXPR_H

#include "ProgramUnit.h"

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

bool set_has_expr(RefSet<Expression> & outc, Expression &r);
void subexpr_elimination(ProgramUnit & pgm,List<BasicBlock> * pgm_basic_blocks, int p_avail, int availexpr, int gcseliminate, int copyprop);
RefSet<Expression> egen(Statement *s);
RefSet<Expression> ekill(Statement *s, RefSet<Expression> &U);
bool expr_contains(Expression & big, Expression & small);
void cal_available_expression(ProgramUnit & pgm, map<Statement *, RefSet<Expression> >  &IN, map<Statement *, RefSet<Expression> >  &OUT); 

RefSet<Statement> get_prev_reaching(Expression &r, Statement * s,map<Statement *, RefSet<Expression> >  &OUT);
int meets_condition(Statement *s, RefSet<Expression> &IN );
template <class T>
RefSet<T> intersect(RefSet<T> &a, RefSet<T> &b);

void print_in_out(StmtList & stmts, map<Statement *, RefSet<Expression> > & IN, map<Statement *, RefSet<Expression> >  & OUT );
void copy_propagation(ProgramUnit & pgm);
void use_def_chain();
void GCSE(ProgramUnit & pgm, map<Statement *, RefSet<Expression> >  &IN,map<Statement *, RefSet<Expression> >  &OUT);
template <typename T> string NumberToString ( T Number );
const char* new_temp_name(int & counter);
void split_all(ProgramUnit & pgm);
int expr_type_check(Expression &e);
int expr_op_check(Expression &e);
void split_binary(ProgramUnit & pgm,Expression &e,vector<Statement * > & v,int & counter);
void split_func(ProgramUnit & pgm,Expression &e,vector<Statement * > & v,int & counter);
void split_expr(Expression &e,ProgramUnit & pgm,Statement *s,int &counter,int flag);
bool is_temp_var(Expression & e);
void replace_var(Mutator<Expression> &ep, map <Expression*, Expression*> &table);
#endif
