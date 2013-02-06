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

#define PRINT_ALL	0
#define PRINT_SLICE	1


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

	OP_MOD,


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

// for INC and DEC (EXP_UN)
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
	EXP_TOKEN		// this is for processing AND/OR instructions, serve as a temp token in the expression stack
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
			ArrayList 	*parameters;	//this is a list of ExpTreeNode
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
#define EXP_IS_NULL				(1<<6)
#define EXP_IS_REGEXP			(1<<7)
#define EXP_IS_DOC_WRITE		(1<<8)


#define EXP_IN_SLICE			(1<<16)


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
	TN_WHILE,

	TN_TRY_CATCH,
	TN_THROW		//throw uses only the  in SyntaxTreeNode
} SyntaxTreeNodeType;


typedef struct SyntaxTreeNode SyntaxTreeNode;

struct SyntaxTreeNode{
	int 				id;
	SyntaxTreeNodeType	type;
	uint32_t			flags;
	SyntaxTreeNode		*parent;			//parent SyntaxTreeNode in AST
	ArrayList 			*predsList;			//a list of predesessor syntaxTreeNode of this node
											//(ONLY for those end with a goto to this node)
	union{
		struct{

			int			cfg_id;				//old id is the id# BasicBlock contained in this SyntaxTreeNode when its created
			BasicBlock 	*cfgBlock;
			ArrayList 	*statements;		// a list of SyntaxTreeNode
			//SyntaxStack	*stack;
		} block;

		struct{
			ExpTreeNode *cond;
			NaturalLoop *loopStruct;
			BasicBlock	*header;

			SyntaxTreeNode	*synHeader;	//pointer to the header syntaxTree nnde
			ArrayList 	*loopBody;
			ArrayList 	*loopBody2;		//this list is merely used for easier check of loop membership

		}loop;

		struct{
			Function 	*funcStruct;
			ArrayList 	*funcBody;		// a list of SyntaxTreeNode
			SyntaxTreeNode	*synFuncEntryNode;	//pointer to the func entry syntax tree node
		} func;

		ExpTreeNode *expNode;

		struct{
			BasicBlock 		*targetBlock;
			SyntaxTreeNode	*synTargetBlock;	//for continue and break, this point to a parent block it cont/break
		} go_to;

		struct{
			ExpTreeNode *cond;
			// ifeq: if-path->adj_path  else-path->branch_path
			// ifne: if-path->branch_path  else-path->adj_path
			ArrayList 	*if_path;				//both are lists of SyntaxTreeNodes
			ArrayList 	*else_path;
		} if_else;

		//current implementation is incomplete and only for signature function generation
		struct{
			ArrayList *try_body;
			ArrayList *catch_body;
		} try_catch;

		//todo:
		//switch
	} u;
};



#define	TN_IS_GOTO_BREAK				(1<<0)
#define TN_IS_GOTO_CONTINUE				(1<<1)
#define	TN_NOT_SHOW_LABEL				(1<<2)
#define TN_HIDE							(1<<3)	//don't print this entire statement(s) in printSyntaxNode function (used for go_to nodes)
#define	TN_AFTER_RELINK_GOTO			(1<<4)	//this flag indicates that syntax node id should be uesd in printed label
#define TN_IS_LOOP_HEADER				(1<<5)
#define TN_DONE_MOVE					(1<<6)


#define TN_IN_SLICE						(1<<16)
#define TN_IS_CHECK_POINT				(1<<17)	//all if(decisionVec[index++]!=THIS_PATH) throw "wrong path"
#define TN_IS_SIG_FUNCTION				(1<<18)	//including function, try-catch statement,
												//  return false in catch and return true at the end of function
												// all check points and decision vector

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

StackNode *popSyntaxStack(SyntaxStack *stack);
void pushSyntaxStack(SyntaxStack *stack, StackNode *node);
StackNode *createSyntaxStackNode();
void destroySyntaxStackNode(StackNode *node );
SyntaxStack *createSyntaxStack(void);
void stackTest();

void printSyntaxTreeNode(SyntaxTreeNode *node, int slice_flag);
ExpTreeNode *createExpTreeNode();
void destroyExpTreeNode(ExpTreeNode *node);
SyntaxTreeNode *createSyntaxTreeNode();
void destroySyntaxTreeNode(SyntaxTreeNode *node);
int SyntaxTreeNodeCompareByBlockID(void *i1, void *i2);
SyntaxTreeNode *buildSyntaxTreeForBlock(BasicBlock *block, uint32_t flag, ArrayList *funObjTable, ArrayList *funcCFGs, int slice_flag);
ArrayList *buildSyntaxTree(InstrList *iList, ArrayList *blockList, ArrayList *loopList, ArrayList *funcCFGs, ArrayList *funcObjTable, int slice_flag);


SyntaxTreeNode *findAdjacentStatement(ArrayList *syntaxTree, SyntaxTreeNode *node);
SyntaxTreeNode *findNextStatement(ArrayList *syntaxTree, SyntaxTreeNode *node);

//moving syntaxNode around in syntax tree
SyntaxTreeNode *insertBefore(ArrayList *syntaxTree, SyntaxTreeNode *node_ins, SyntaxTreeNode *node_target);
SyntaxTreeNode *insertAfter(ArrayList *syntaxTree, SyntaxTreeNode *node_ins, SyntaxTreeNode *node_target);
SyntaxTreeNode *moveToBefore(ArrayList *syntaxTree, SyntaxTreeNode *node_move, SyntaxTreeNode *node_target);
SyntaxTreeNode *moveToAfter(ArrayList *syntaxTree, SyntaxTreeNode *node_move, SyntaxTreeNode *node_target);


void transformSyntaxTree(ArrayList *syntaxTree);






#endif /* SYNTAX_H_ */
