#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include "read.h"
#include "opcode.h"

#define MAX_CHARS_PER_LINE	512

static int lineno = 0;

//this function used only for address conversion
//so 0 indicates an error is OK
unsigned long atoh(char *s){
	unsigned long value = 0, digit;
    char c;
    if(s==NULL)
    	return 0;
    while ((c = *s++) != '\0') {
        if (c >= '0' && c <= '9')
        	digit = (unsigned long) (c - '0');
        else if (c >= 'a' && c <= 'f')
        	digit = (unsigned long) (c - 'a') + 10;
        else if (c >= 'A' && c <= 'F')
        	digit = (unsigned long) (c - 'A') + 10;
        else if (c=='x'||c=='X')
        	continue;
        else
        	return 0;
        value = (value << 4) + digit;
    }
    return value;
}

Instruction *GetInstrFromText(char *buffer){
	Instruction *new = NULL;
	char *s, *tok, *tokSave;
	int slot;
	unsigned long addr;
	long id;
	double number;
	int fun_int;

	static bool 	ignore = false;
	static ADDRESS 	next_addr = 0;
	static bool		last_is_doc_write = false;
	static Instruction *last_native_call = NULL;
	//ignore xor next_addr = 0
	assert((ignore==false && next_addr==0) || (ignore==true && next_addr>0));
	if(last_is_doc_write)
		assert(last_native_call);

	lineno += 1;

	// a '%' appearing as the first character of a line indicates an instr occurance.
	if (*buffer != '%') {
		return NULL;
	}
	s = ++buffer;

	//create instr object, and initialize its udis object
	new = InstrCreate();

	//get first token
	tok = strtok_r(s, " \t", &tokSave);

	if (tok == NULL) {
		fprintf(stderr, "ERROR [line %d]: malformed trace [at entry]\n", lineno);
		abort();
	}

	//check for program counter
	if((addr=atoh(tok)) != 0) {
		new->addr = addr;
		if(ignore==false || (ignore==true && new->addr==next_addr)){
			ignore = false;
			next_addr = 0;
			last_is_doc_write = false;
			last_native_call = NULL;
		}
		//doc.write() generate trace, can't ignore them
		else if(last_is_doc_write){
			last_is_doc_write = false;
			ignore = false;
			next_addr = 0;
			assert(INSTR_HAS_FLAG(last_native_call, INSTR_IS_NATIVE_INVOKE));
			INSTR_SET_FLAG(last_native_call, INSTR_IS_SCRIPT_INVOKE);
			last_native_call = NULL;
		}
		//callback trace
		else{
			last_is_doc_write = false;
			last_native_call = NULL;
			fprintf(stderr, "IGNORE: %lx %s\n", new->addr, new->opName);
			InstrDestroy(new);
			return NULL;
		}
		//fprintf(stderr, "%lx\t", addr);
	}else{
		fprintf(stderr, "ERROR [line %d]: malformed trace [at PC (%s)] \n", lineno, tok);
		abort();
	}

	//get opcode name
	tok = strtok_r(NULL, " \t", &tokSave);
	if(tok){
		strcpy(new->opName, tok);
		//fprintf(stderr, "%s\t", tok);
	}else{
		fprintf(stderr, "ERROR [line %d]: malformed trace [at opcode] \n", lineno);
		abort();
	}

	//get LEN, S_U/D, L_U/D, PROP_U/D, etc
	while((tok=strtok_r(NULL, " \t\n", &tokSave))){
		if(*tok!='#'){
			//fprintf(stderr, "ignore token %s at line %d\n", tok, lineno);
			continue;
		}
		if(!strcmp(tok, "#IGNORE") || !strcmp(tok, "#NotImplemented")){
			InstrDestroy(new);
			return NULL;
		}
		else if(!strcmp(tok, "#DOC_WRITE") ){
			INSTR_SET_FLAG(new, INSTR_IS_DOC_WRITE);
			last_is_doc_write = true;
		}
		else if(!strcmp(tok, "#LEN:")){
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at LEN] \n", lineno);
				abort();
			}
			new->length = atoi(tok);
			//fprintf(stderr, "#LEN: %d\t", new->length);
		}
		else if(!strcmp(tok, "#FUN_INT:")){
			assert(isInvokeInstruction(NULL, new));
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at LEN] \n", lineno);
				abort();
			}
			fun_int = atoi(tok);
			assert(fun_int==1 || fun_int==0);
			if(fun_int==1){
				INSTR_SET_FLAG(new, INSTR_IS_SCRIPT_INVOKE);
			}else{
				INSTR_SET_FLAG(new, INSTR_IS_NATIVE_INVOKE);
				assert(new->length && new->addr);
				ignore = true;
				next_addr = new->addr + new->length;
				last_native_call = new;
			}
			//fprintf(stderr, "#LEN: %d\t", new->length);
		}
		else if(!strcmp(tok, "#FILE:")){
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at FILE] \n", lineno);
				abort();
			}
			/* TODO
			(char *)malloc(strlen(tok)+1);
			assert(new->str);
			memcpy(new->str, tok, strlen(tok)+1);
			*/
		}

		else if(!strcmp(tok, "#S_USE:")){
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at S_USE] \n", lineno);
				abort();
			}
			slot = atoi(tok);
			new->stackUse = slot;
			if(slot>0){
				tok=strtok_r(NULL, " \t\n", &tokSave);
				if(!tok || (addr=atoh(tok)) == 0){
					fprintf(stderr, "ERROR [line %d]: malformed trace [at S_USE: addr_start %s] \n", lineno, tok?tok:" ");
					abort();
				}
			}else
				addr = 0;
			new->stackUseStart = addr;
			//fprintf(stderr, "#S_USE: %d\t", slot);
		}
		else if(!strcmp(tok, "#S_DEF:")){
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at S_DEF] \n", lineno);
				abort();
			}
			slot = atoi(tok);
			new->stackDef = slot;
			if(slot>0){
				tok=strtok_r(NULL, " \t\n", &tokSave);
				if(!tok || (addr=atoh(tok)) == 0){
					fprintf(stderr, "ERROR [line %d]: malformed trace [at S_DEF: addr_start %s] \n", lineno, tok?tok:" ");
					abort();
				}
			}else
				addr = 0;
			new->stackDefStart = addr;
			//fprintf(stderr, "#S_DEF: %d\t", slot);
		}
/*		else if(!strcmp(tok, "#NUM_PARAS:")){
			assert(0);
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at NUM_PARAS] \n", lineno);
				abort();
			}
			slot = atoi(tok);
			new->parameterNum = slot;
			if(slot<0){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at NUM_PARAS: %d] \n", lineno, slot);
				abort();
			}
		}*/
		else if(!strcmp(tok, "#L_USE:")){
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if((addr=atoh(tok)) != 0) {
				new->localUse = addr;
				//fprintf(stderr, "#L_USE: %lx\t", addr);
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at L_USE (%s)] \n", lineno, tok);
				abort();
			}
		}
		else if(!strcmp(tok, "#L_DEF:")){
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if((addr=atoh(tok)) != 0) {
				new->localDef = addr;
				//fprintf(stderr, "#L_USE: %lx\t", addr);
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at L_DEF (%s)] \n", lineno, tok);
				abort();
			}
		}
		else if(!strcmp(tok, "#L_DEF2:")){
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if((addr=atoh(tok)) != 0) {
				new->localDef2 = addr;
				//fprintf(stderr, "#L_USE: %lx\t", addr);
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at L_DEF2 (%s)] \n", lineno, tok);
				abort();
			}
		}
		else if(!strcmp(tok, "#USE_PROP:")){
			//get scope addr
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at USE_PROP:scope] \n", lineno);
				abort();
			}
			//fprintf(stderr, "#USE_PROP: ");
			if((addr=atoh(tok)) != 0) {
				new->propUseScope = addr;
				//fprintf(stderr, "#obj: %lx\t", addr);
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #USE_PROP:scope (%s)] \n", lineno, tok);
				abort();
			}
			//get id
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at USE_PROP:id] \n", lineno);
				abort();
			}
			if((id=atol(tok)) != 0) {
				new->propUseId = id;
				//fprintf(stderr, "#id: %ld\t", id);
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #USE_PROP:id (%s)] \n", lineno, tok);
				abort();
			}
			//get name
			tok = strtok_r(NULL, " \t\n", &tokSave);
			if(tok){
				if(new->str==NULL){
					new->str = (char *)malloc(strlen(tok)+1);
					assert(new->str);
					memcpy(new->str, tok, strlen(tok)+1);
				}
				else{
					assert(!strcmp(new->str, tok));
					;//printf("!!!!!! prop_use name: %s\tprop_def name: %s\n", tok, new->str);
				}
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #USE_PROP:name] \n", lineno);
				abort();
			}
			//get obj_ref
			assert(new->objRef == 0);
			tok = strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at USE_PROP:obj_ref] \n", lineno);
				abort();
			}
			if(*tok=='0'){
				//TODO:
				//fprintf(stderr, "0\t");
				new->objRef = 0;
			}
			else if((addr=atoh(tok)) != 0) {
				//TODO:
				//fprintf(stderr, "#obj_ref: %lx\t", addr);
				new->objRef = addr;
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #USE_PROP:obj_ref (%s)] \n", lineno, tok);
				abort();
			}

		}
		else if(!strcmp(tok, "#DEF_PROP:")){
			//get scope addr
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #DEF_PROP:scope] \n", lineno);
				abort();
			}
			//fprintf(stderr, "#PROP_DEF: ");
			if((addr=atoh(tok)) != 0) {
				new->propDefScope = addr;
				//fprintf(stderr, "#obj: %lx\t", addr);
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #DEF_PROP:obj (%s)] \n", lineno, tok);
				abort();
			}
			//get id
			tok=strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #DEF_PROP:id] \n", lineno);
				abort();
			}
			if((id=atol(tok)) != 0) {
				new->propDefId = id;
				//fprintf(stderr, "#id: %ld\t", id);
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #DEF_PROP:id (%s)] \n", lineno, tok);
				abort();
			}
			//get name
			tok = strtok_r(NULL, " \t\n", &tokSave);
			if(tok){
				//TODO:
				if(new->str==NULL){
					new->str = (char *)malloc(strlen(tok)+1);
					assert(new->str);
					memcpy(new->str, tok, strlen(tok)+1);
				}else{
					assert(!strcmp(new->str, tok));
					;//printf("!!!!!! prop_def name: %s\tprop_use name: %s\n", tok, new->str);
				}
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #DEF_PROP:name] \n", lineno);
				abort();
			}
			//get obj_ref
			tok = strtok_r(NULL, " \t", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #DEF_PROP:obj_ref] \n", lineno);
				abort();
			}
			assert(new->objRef == 0);
			if(*tok=='0'){
				//TODO:
				//fprintf(stderr, "#obj_ref: 0\t");
				new->objRef = 0;
			}
			else if((addr=atoh(tok)) != 0) {
				//TODO:
				//fprintf(stderr, "#obj_ref: %lx\t", addr);
				new->objRef = addr;
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #DEF_PROP:obj_ref (%s)] \n", lineno, tok);
				abort();
			}

		}else if(!strcmp(tok, "#ACCESS_VARS:")){
			tok = strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #ACCESS_VARS] \n", lineno);
				abort();
			}
			slot=atoi(tok);
			new->operand.i = slot;
			INSTR_SET_FLAG(new, INSTR_IS_LVAR_ACCESS);
			//fprintf(stderr, "#id: %ld\t", id);
		}
		else if(!strcmp(tok, "#ACCESS_ARGS:")){
			tok = strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #ACCESS_ARGS] \n", lineno);
				abort();
			}
			slot=atoi(tok);
			new->operand.i = slot;
			INSTR_SET_FLAG(new, INSTR_IS_LARG_ACCESS);
			//fprintf(stderr, "#id: %ld\t", id);
		}
		else if(!strcmp(tok, "#NVARS:")){
			tok = strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #NVARS] \n", lineno);
				abort();
			}
			slot=atoi(tok);
			new->nvars = slot;
		}
		else if(!strcmp(tok, "#OPND_I:")){
			tok = strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #OPND_I] \n", lineno);
				abort();
			}
			slot=atoi(tok);
			new->operand.i = slot;
			//fprintf(stderr, "#id: %ld\t", id);


		}else if(!strcmp(tok, "#OPND_D:")){
			tok = strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #OPND_D] \n", lineno);
				abort();
			}
			number=atof(tok);
			new->operand.d = number;
			//fprintf(stderr, "#id: %ld\t", id);

		}
		//OPND_S is used by string and regexp
		else if(!strcmp(tok, "#OPND_S:")){
			//hack, we know OPND_S always the last in trace entry if appeared, and there's no '\0', but a '\t\n' at the end
			//so to accommodate entire string (with space, etc in between), we get to the end of line
			//and substitute '\t\n' with '\0'
			tok = tokSave;
			//printf("#OPND_S:  tok:%s toklen:%d\n", tok, strlen(tok));
/*			printf("#OPND_S:  tok:%s toklen:%d\n", tok, strlen(tok));
			int xxx;
			for(xxx=0;xxx<strlen(tok);xxx++){
				printf("%d ", tok[xxx]);
			}*/
			*(tok + strlen(tok) -2)='\0';

			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #OPND_S] \n", lineno);
				abort();
			}
			if(*tok == '\0'){
				tok[0] = '\0';	//maybe should use ''
				tok[1] = '\0';
				printf("instr addr: %lx\t#OPND_S: empty string! @line %d\n", new->addr, lineno);

			}
			new->operand.s = (char *)malloc(strlen(tok)+1);
			assert(new->operand.s);
			memcpy(new->operand.s, tok, strlen(tok)+1);
		}
		//used by eval
		else if(!strcmp(tok, "#js_CompileScript:")){
			if(strcmp(new->opName, "eval")){
				printf("non-eval instr %s has #js_CompileScript\n", new->opName);
				continue;
			}
			//hack, we know #js_CompileScript: always the last in trace entry if appeared, and there's no '\0', but a '\t\n' at the end
			//so to acommodate entire string (with space, etc in between), we get to the end of line
			//and substitute '\t\n' with '\0'
			tok = tokSave;
			*(tok + strlen(tok) -2)='\0';

			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #OPND_S] \n", lineno);
				abort();
			}
			if(*tok == '\0'){
				tok[0] = ' ';
				tok[1] = '\0';
				printf("instr addr: %lx\t#OPND_S: empty string! @line %d\n", new->addr, lineno);
			}
			new->eval_str = (char *)malloc(strlen(tok)+1);
			assert(new->eval_str);
			memcpy(new->eval_str, tok, strlen(tok)+1);
			printf("get a eval: %s\n", new->eval_str);
		}
		else if(!strcmp(tok, "#callee_obj:")){
			tok = strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #callee_obj] \n", lineno);
				abort();
			}
			assert(new->objRef == 0);
			if((addr=atoh(tok)) != 0) {
				new->objRef = addr;
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #callee_obj (%s)] \n", lineno, tok);
				abort();
			}
		}
		else if(!strcmp(tok, "#IN_CALLEE:")){
			tok = strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #IN_CALLEE] \n", lineno);
				abort();
			}
			assert(new->inCallee == 0);
			if((addr=atoh(tok)) != 0) {
				new->inCallee = addr;
			}else{
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #IN_CALLEE (%s)] \n", lineno, tok);
				abort();
			}
		}
		else if(!strcmp(tok, "#JMP_OFFSET:")){
			tok = strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #JMP_OFFSET] \n", lineno);
				abort();
			}
			slot=atoi(tok);
			new->jmpOffset=slot;
			tok = strtok_r(NULL, " \t\n", &tokSave);
			if(!tok){
				fprintf(stderr, "ERROR [line %d]: malformed trace [at #JMP_OFFSET] \n", lineno);
				abort();
			}
			slot=atoi(tok);
			assert(slot==0||slot==1);
			if(slot==0)
				INSTR_SET_FLAG(new, INSTR_BRANCH_NOT_TAKEN);
			else
				INSTR_SET_FLAG(new, INSTR_BRANCH_TAKEN);
		}
		else{
			fprintf(stderr, "ERROR [line %d]: unrecognized token %s\n", lineno, tok);
		}
	}
	//fprintf(stderr,"\n");

	return (new);
}

InstrList *BuildIListFromFile(FILE *file) {
    InstrList *iList = malloc(sizeof(InstrList));
    iList->numInstrs = 0;
    int order = 0; //dyn trace, so add instr's in order of exec.

    iList->iList = al_new();
    char *buffer = (char *)malloc(MAX_CHARS_PER_LINE);

    while (fgets(buffer, MAX_CHARS_PER_LINE, file)) {
        //skip unrelated lines
        if(buffer[0]!='%')
                continue;
        Instruction *new = GetInstrFromText(buffer);
        //printf("%s\n", buffer);
        if(new) {
            new->order = order++;
            new->opCode = getOpCodeFromString(new->opName);
            //this is an local access instr but using a global access opcode
            //need to clear the propUse/Def
            if(INSTR_HAS_FLAG(new, INSTR_IS_LARG_ACCESS) || INSTR_HAS_FLAG(new, INSTR_IS_LVAR_ACCESS)){
            	new->propDefScope = new->propUseScope = new->propDefId = new->propUseId = 0;
            	assert(new->localDef || new->localUse);
            }
/*            if(new->opCode == JSOP_EVAL){
            	new->stackUseStart = 0;
            	new->stackUse = 0;
            }*/
            iList->numInstrs++;
            al_add(iList->iList, new);
        }
    }
    free(buffer);

    return (iList);
}

InstrList *InitInstrList(char *inFileName){
        InstrList *iList;
        FILE *traceFile;

        if((traceFile = fopen(inFileName, "r")) == NULL){
                fprintf(stderr, "ERROR: unable to open %s\n", inFileName);
                exit(0);
        }

        iList = BuildIListFromFile(traceFile);
        fprintf(stderr, "InitInstrList done.\n");
        fclose(traceFile);
        fprintf(stderr, "Total instructions: %d\n", iList->numInstrs);
        return iList;
}

