// 
// ssa.h : SSA pass
// 

#ifndef _SSA_H
#define _SSA_H

#include "ProgramUnit.h"

#include "bblock/bblock.h"
#include "bblock/BasicBlock.h"
#include <stack>
#include <vector>
#include <map> 

void ssa(ProgramUnit & pgm, List<BasicBlock> * bb);
void dessa(ProgramUnit & pgm, List<BasicBlock> * bb);
template <class T> RefSet<T> intersect(RefSet<T> &a, RefSet<T> &b);
map<BasicBlock *, RefSet<BasicBlock> > find_dom(List<BasicBlock> * bb);
map<BasicBlock *, RefSet<BasicBlock> > reverse_dom(map<BasicBlock *, RefSet<BasicBlock> >  &DOM);
map<BasicBlock *, BasicBlock * > find_idom(map<BasicBlock *, RefSet<BasicBlock> > & rDOM, map<BasicBlock *, RefSet<BasicBlock> > & DOM);
map<BasicBlock *, RefSet<BasicBlock> > find_df(List<BasicBlock> *bbl, map<BasicBlock *, BasicBlock*> & idom);


void insert_phi_nodes(ProgramUnit & pgm, List<BasicBlock> *bbl,map<Symbol *, 
	RefSet<BasicBlock> > & var_table,map<BasicBlock *, RefSet<BasicBlock> > &DF);

map<Symbol *, RefSet<BasicBlock> > left_var_table(List<BasicBlock> * bb);


template <class T> bool vector_has_element(vector<T*> &a, T * b);
  


void gen_name(Expression *, map<Symbol *,int> &Counters, map<Symbol *, stack<int > > &Stack);
void rename_expr(Expression * expr, map<Symbol *, stack<int> > &Stack);
void Rename(BasicBlock * cur, map<Symbol *,int> &Counters, map<Symbol *, stack<int> > &Stack,
	map<BasicBlock *, RefSet<BasicBlock> > & DOM,map<const char*, int> &visited);

inline void print_idom(map<BasicBlock *, BasicBlock *> & idom);
inline void print_dom(map<BasicBlock *, RefSet<BasicBlock> >  & DOM);
inline void print_table(map<Symbol * , RefSet<BasicBlock> > & table);


void pop_old_symbol(map<Symbol *, stack<int> > &Stack, string n);
Expression* unnamed_subexpr(Expression & para);
string to_string(unsigned int i);
string cut(string c);

bool is_phi_stmt(Statement *s);
void recover_var_name(Expression * expr);
 
#endif
