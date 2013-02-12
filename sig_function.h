/*
 * sig_function.h
 *
 *  Created on: Feb 5, 2013
 *      Author: genlu
 */

#ifndef SIG_FUNCTION_H_
#define SIG_FUNCTION_H_

extern char *vectorStr;
extern char *vectorIndex;
extern char *exceptionStr;

SyntaxTreeNode *createSigFunction(InstrList *iList, ArrayList *syntaxTrees, ArrayList *funcObjTable);


#endif /* SIG_FUNCTION_H_ */
