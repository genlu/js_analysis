/*
 * syntax.h
 *
 *  Created on: May 17, 2011
 *      Author: genlu
 */

#ifndef SYNTAX_H_
#define SYNTAX_H_

#include <stdint.h>
#include "prv_types.h"
#include "array_list.h"
#include "writeSet.h"


typedef struct SyntaxStack SyntaxStack;

/*********************************************************************************/

typedef struct AndOrTarget{
	ADDRESS targetAddr;
	int num;
}AndOrTarget;


/*********************************************************************************/

typedef enum{
//binary
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,


	OP_BITAND,
	OP_BITOR,

	OP_GE,
	OP_GT,
	OP_LE,
	OP_LT,
	OP_NE,
	OP_EQ,

	OP_INITPROP,

	OP_ASSIGN,

//unary

	OP_NOT,

// for TN_INC_DEC
	OP_VINC,
	OP_VDEC,	
	OP_INCV,
	OP_DECV,

// for AND and OR,
	OP_AND,
	OP_OR
	
} Operator;


/*********************************************************************************/

typedef enum{
	EXP_NAME,	//a name (for var/prop/func/etc)
	EXP_UNDEFINED,
	EXP_CONST_VALUE,		// string, double, int
	EXP_BIN,		//binary op
	EXP_UN,		//unary op
	EXP_CALL,	// a function call, contains a list with func name node and its parameter nodes
	EXP_NEW,		// a constructor call, i.e. new abc()
	EXP_ARRAY_ELM,
	EXP_ARRAY_INIT,		//and array initialzation struct, as [1,2,3...], contains a list of
	EXP_PROP,		//contain 2 children, one is the name/prep represent obj, other is name represent prop
	EXP_EVAL,
	EXP_TOKEN		// this is for processing AND/OR instructions, serve as a temp token in the espression stack
} ExpTreeNodeType;

typedef struct ExpTreeNode ExpTreeNode;

struct ExpTreeNode{
	ExpTreeNodeType type;
	uint32_t		flags;
	union{
		//EXP_NAME
		char	*name;

		//EXP_CONST_VALUE
		double	const_value_double;
		char	*const_value_str;
		long	const_value_int;
		bool	const_value_bool;

		//EXP_UN
		struct{
			char	*name;	// this is for inc/dec operation only
			Operator op;
			ExpTreeNode *lval;
		} un_op;

		//EXP_BIN
		struct{
			Operator op;
			ExpTreeNode *lval;
			ExpTreeNode *rval;
		} bin_op;

		//EXP_CALL and EXP_NEW, EXP_EVAL
		struct{
			ExpTreeNode *name;
			int			num_paras;
			ArrayList 	*parameters;	//this is a list of ExpTreeNode CAUTION: function call with an assignment as parameter is not support
			BasicBlock 	*funcEntryBlock;
		} func;

		//EXP_ARRAY_ELM
		struct{
			ExpTreeNode *name;
			ExpTreeNode *index;
		}array_elm;

		//EXP_ARRAY_INIT
		struct{
			int	size;
			ArrayList *initValues;	//an list of ExpTreeNode
		}array_init;

		//EXP_PROP
		struct{
			ExpTreeNode *objName;
			ExpTreeNode *propName;
		} prop;

		struct{
			ADDRESS targetAddr;
			Operator op;
		} tok;
	} u;
};

#define EXP_IS_STRING			(1<<0)
#define EXP_IS_INT				(1<<1)
#define	EXP_IS_DOUBLE			(1<<2)
#define	EXP_IS_BOOL				(1<<3)
#define EXP_IS_FUNCTION_OBJ		(1<<4)
#define EXP_IS_PROP_INIT		(1<<5)


#define	EXP_SET_FLAG(exp, flag)		(exp->flags |= flag)
#define	EXP_CLR_FLAG(exp, flag)		(exp->flags &= ~flag)
#define	EXP_HAS_FLAG(exp, flag)		(exp->flags & flag)


/*********************************************************************************/

typedef enum {
	TN_FUNCTION,
	TN_BLOCK,	//basic block
	//TN_INC_DEC,
	TN_IF_ELSE,
	TN_GOTO,
	TN_RETURN,
	TN_RETEXP,
	TN_EXP,		//expression
	TN_DEFVAR,
	TN_WHILE
} SyntaxTreeNodeType;


typedef struct SyntaxTreeNode SyntaxTreeNode;

struct SyntaxTreeNode{
	SyntaxTreeNodeType	type;
	uint32_t			flags;
	union{
		struct{
			int 		id;
			int			old_id;
			ArrayList 	*statements;		// a list of SyntaxTreeNode
			//SyntaxStack	*stack;
		} block;

		struct{
			ExpTreeNode *cond;
			NaturalLoop *loopStruct;
			BasicBlock	*header;
			ArrayList 	*loopBody;
		}loop;

		struct{
			Function 	*funcStruct;
			ArrayList 	*funcBody;		// a list of SyntaxTreeNode
		} func;

		ExpTreeNode *expNode;

		struct{
			ExpTreeNode *lval;
			ExpTreeNode *rval;
			//void *rval;
		} assign;

/*		struct{
			ExpTreeNode *name;
			Operator 	op;
		} inc_dec;*/

		struct{
			BasicBlock *targetBlock;
		} go_to;

		struct{
			ExpTreeNode *cond;
			// ifeq: if-path->adj_path  else-path->branch_path
			// ifne: if-path->branch_path  else-path->adj_path
			SyntaxTreeNode *if_path;
			SyntaxTreeNode *else_path;
		} if_else;
	} u;
};

#define	TN_IS_GOTO_BREAK				(1<<0)
#define TN_IS_GOTO_CONTINUE				(1<<1)

#define	TN_SET_FLAG(tn, flag)		(tn->flags |= flag)
#define	TN_CLR_FLAG(tn, flag)		(tn->flags &= ~flag)
#define	TN_HAS_FLAG(tn, flag)		(tn->flags & flag)

/*********************************************************************************/


typedef enum{
	SYN_NODE,
	EXP_NODE
} NodeType;

typedef struct StackNode StackNode;

struct StackNode{
	NodeType type;
	void *node;			//pointer to ExpTreeNode or SyntaxTreeNode, depends on type
	StackNode *prev;		//point to the direction of stack top
	StackNode *next;		//point to the direction of stack bottom
};

struct SyntaxStack{
	StackNode	*bottom;
	StackNode 	*top;
	int			number;
};

StackNode *popSyntaxStack(SyntaxStack *stack);
void pushSyntaxStack(SyntaxStack *stack, StackNode *node);
SyntaxStack *createSyntaxStack(void);

/*********************************************************************************/

typedef struct FuncObjTableEntry{
	uint32_t 		flag;
	char 			*funcName;
	ADDRESS			funcObjAddr;
	Function		*funcStruct;
}FuncObjTableEntry;

FuncObjTableEntry *createFuncObjTableEntry(void);
void destroyFuncObjTableEntry(void *entry);

/*********************************************************************************/

StackNode *popSyntaxStack(SyntaxStack *stack);
void pushSyntaxStack(SyntaxStack *stack, StackNode *node);
StackNode *createSyntaxStackNode();
void destroySyntaxStackNode(StackNode *node );
SyntaxStack *createSyntaxStack(void);
void stackTest();

void printSyntaxTreeNode(SyntaxTreeNode *node);
ExpTreeNode *createExpTreeNode();
void destroyExpTreeNode(ExpTreeNode *node);
SyntaxTreeNode *createSyntaxTreeNode();
void destroySyntaxTreeNode(SyntaxTreeNode *node);
int SyntaxTreeNodeCompareByBlockID(void *i1, void *i2);
SyntaxTreeNode *buildSyntaxTreeForBlock(BasicBlock *block, uint32_t flag, ArrayList *funObjTable, ArrayList *funcCFGs);
ArrayList *buildSyntaxTree(InstrList *iList, ArrayList *blockList, ArrayList *loopList, ArrayList *funcCFGs);










#endif /* SYNTAX_H_ */
