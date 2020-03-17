#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"
#include "MiniC.tbl"

#define HASH_BUCKET_SIZE 97
#define SYMTAB_SIZE 200
#define LEVEL_STACK_SIZE 10
#define LABEL_SIZE 12

FILE *sourceFile;
FILE *ucodeFile;
FILE *astFile;

enum opcodeEnum {
	notop, neg, incop, decop, dup,
	add, sub, mult, divop, modop, swp,
	andop, orop, gt, lt, ge, le, eq, ne,
	lod, str, ldc, lda,
	ujp, tjp, fjp,
	chkh, chkl,
	ldi, sti,
	call, ret, retv, ldp, proc, endop,
	nop, bgn, sym
};

char *opcodeName[] = {
	"notop",    "neg",	"inc",	"dec",	"dup",
	"add",	"sub",	"mult",	"div",	"mod",	"swp",
	"and",	"or",	"gt",	"lt",	"ge",	"le",	"eq",	"ne",
	"lod",	"str",	"ldc",	"lda",
	"ujp",	"tjp",	"fjp",
	"chkh",	"chkl",
	"ldi",	"sti",
	"call",	"ret",	"retv",	"ldp",	"proc",	"end",
	"nop",	"bgn",	"sym"
};
int lvalue, rvalue;

char *nodeName[] = {
	"ACTUAL_PARAM",   "ADD",            "ADD_ASSIGN",     "ARRAY_VAR",      "ASSIGN_OP",
	"CALL",           "COMPOUND_ST",    "CONST_NODE",     "DCL",            "DCL_ITEM",
	"DCL_LIST",       "DCL_SPEC",       "DIV",            "DIV_ASSIGN",     "EQ",
	"ERROR_NODE",     "EXP_ST",         "FORMAL_PARA",    "FUNC_DEF",       "FUNC_HEAD",
	"GE",             "GT",             "IDENT",          "IF_ELSE_ST",     "IF_ST",
	"INDEX",          "INT_NODE",       "LE",             "LOGICAL_AND",    "LOGICAL_NOT",
	"LOGICAL_OR",     "LT",             "MOD",            "MOD_ASSIGN",     "MUL",
	"MUL_ASSIGN",     "NE",             "NUMBER",         "PARAM_DCL",      "POST_DEC",
	"POST_INC",       "PRE_DEC",        "PRE_INC",        "PROGRAM",        "RETURN_ST",
	"SIMPLE_VAR",     "STAT_LIST",      "SUB",            "SUB_ASSIGN",     "UNARY_MINUS",
	"VOID_NODE",      "WHILE_ST"
};

int ruleName[] = {
	/* 0               1               2               3               4           */
	0,              PROGRAM,        0,              0,              0,
	/* 5               6               7               8               9           */
	0,              FUNC_DEF,       FUNC_HEAD,      DCL_SPEC,       0,
	/* 10              11              12              13              14          */
	0,              0,              0,              CONST_NODE,     INT_NODE,
	/* 15              16              17              18              19          */
	VOID_NODE,      0,              FORMAL_PARA,    0,              0,
	/* 20              21              22              23              24          */
	0,              0,              PARAM_DCL,      COMPOUND_ST,    DCL_LIST,
	/* 25              26              27              28              29          */
	DCL_LIST,       0,              0,              DCL,            0,
	/* 30              31              32              33              34          */
	0,              DCL_ITEM,       DCL_ITEM,       SIMPLE_VAR,     ARRAY_VAR,
	/* 35              36              37              38              39          */
	0,              0,              STAT_LIST,      0,              0,
	/* 40              41              42              43              44          */
	0,              0,              0,              0,              0,
	/* 45              46              47              48              49          */
	0,              EXP_ST,         0,              0,              IF_ST,
	/* 50              51              52              53              54          */
	IF_ELSE_ST,     WHILE_ST,       RETURN_ST,      0,              0,
	/* 55              56              57              58              59          */
	ASSIGN_OP,      ADD_ASSIGN,     SUB_ASSIGN,     MUL_ASSIGN,     DIV_ASSIGN,
	/* 60              61              62              63              64          */
	MOD_ASSIGN,     0,              LOGICAL_OR,     0,              LOGICAL_AND,
	/* 65              66              67              68              69          */
	0,              EQ,             NE,             0,              GT,
	/* 70              71              72              73              74          */
	LT,             GE,             LE,             0,              ADD,
	/* 75              76              77              78              79          */
	SUB,            0,              MUL,            DIV,            MOD,
	/* 80              81              82              83              84          */
	0,              UNARY_MINUS,    LOGICAL_NOT,    PRE_INC,        PRE_DEC,
	/* 85              86              87              88              89          */
	0,              INDEX,          CALL,           POST_INC,       POST_DEC,
	/* 90              91              92              93              94          */
	0,              0,              ACTUAL_PARAM,   0,              0,
	/* 95              96              97                                          */
	0,              0,              0
};

char *tokenName[] = {
	/* 0          1      2       3        4           5 */
	"!",        "!=",   "%",    "%=",   "ident",    "%number",
	/* 6          7      8       9       10          11 */
	"&&",       "(",    ")",    "*",    "*=",       "+",
	/*12         13     14      15       16          17 */
	"++",       "+=",   ",",    "-",    "--",       "-=",
	/*18         19     20      21       22          23 */
	"/",        "/=",   ";",    "<",    "<=",       "=",
	/*24         25     26      27       28          29 */
	"==",       ">",    ">=",   "[",    "]",        "eof",
	/* ............ word symbols ...................... */
	/*30         31     32      33       34          35 */
	"const",    "else", "if",   "int",  "return",   "void",
	/*36         37     38      39                      */
	"while",    "{",    "||",   "}"
};

typedef enum {
	NON_SPECIFIER, VOID_TYPE, INT_TYPE
} TypeSpecifier;

typedef enum {
	NON_QUALIFIER, FUNC_TYPE, PARAM_TYPE, CONST_TYPE, VAR_TYPE
} TypeQuailfier;

char *typeName[] = {
	"none",   "void",   "int"
};

char *qualifierName[] = {
	"NONE", "FUNC_TYPE",  "PARAM_TYPE",  "CONST_TYPE", "VAR_TYPE"
};

typedef enum {
	ZERO_DIMENSION, ONE_DIMENSION
} Dimension;

struct SymbolEntry {
	char symbolName[ID_LENGTH];
	int typeSpecifier;
	int typeQualifier;
	int base;
	int offset;
	int width;					// size
	int initialValue;			// initial value
	int nextIndex;				// link to next entry.
};
struct SymbolEntry symbolTable[SYMTAB_SIZE];		// symbol table
int symbolTableTop;
int hashBucket[HASH_BUCKET_SIZE];
int levelStack[SYMTAB_SIZE];						// level table
int levelTop;
int base, offset;

int errcnt = 0;
int sp;                     // stack pointer
int stateStack[PS_SIZE];    // state stack
int symbolStack[PS_SIZE];   // symbol stack
Node* valueStack[PS_SIZE];    // value stack

void semantic(int n);
void printToken(struct tokenType token);
void dumpStack();
void errorRecovery();
int meaningfulToken(struct tokenType token);
Node* buildTree(int nodeNumber, int rhsLength);
Node* buildNode(struct tokenType token);
void printTree(Node *pt, int indent);
void printNode(Node *pt, int indent);
Node* parser();


void rv_emit(Node *ptr);
int checkPredefined(Node *ptr);
void processOperator(Node *ptr);
void processArrayVariable(Node *ptr, int typeSpecifier, int typeQualifier);
void processStatement(Node *ptr);
void processCondition(Node *ptr);
void emitLabel(char *label);
void genLabel(char *label);
int typeSize(int typeSpecifier);
void icg_error(int err);
void emitFunc(char *FuncName, int operand1, int operand2, int operand3);
void codeGen(Node *ptr);
void processFunction(Node *ptr);
void processFuncHeader(Node *ptr);
void processDeclaration(Node *ptr);
void processSimpleVariable(Node *ptr, int typeSpecifier, int typeQualifier);
void initSymbolTable();
int hash(char *);
int lookup(char *);
int insert(char*, int, int, int, int, int, int);
void dumpSymbolTable();
void set();
void reset();
void emitSym(int base, int offset, int size);

int main(int argc, char *argv[])
{
	char fileName[30];
	Node *root;

	printf(" *** start of Mini C Compiler\n");
	if (argc != 2) {
		icg_error(1);
		exit(1);
	}
	strcpy(fileName, argv[1]);
	printf("   * source file name: %s\n", fileName);

	freopen(fileName, "r", stdin); // stdin redirect
	if ((sourceFile = fopen(fileName, "r")) == NULL) {
		icg_error(2);
		exit(1);
	}
	astFile = fopen(strcat(strtok(fileName, "."), ".ast"), "w");
	ucodeFile = fopen(strcat(strtok(fileName, "."), ".uco"), "w");

	printf(" === start of Parser\n");
	root = parser();
	printTree(root, 0);
	printf(" === start of ICG\n");
	codeGen(root);
	printf(" *** end of Mini C Compiler\n");

	fclose(sourceFile);
	fclose(astFile);
	fclose(ucodeFile);
	return 0;
}

void emitSym(int base, int offset, int size) {
	fprintf(ucodeFile, "%14s  %d  %d  %d\n", "sym", base, offset, size);
	printf("%14s  %d  %d  %d\n", "sym", base, offset, size);

}

void emit0(char *value)
{
	fprintf(ucodeFile, "           %s\n", value);
	printf("           %s\n", value);
}

void emit1(char *value, int p)
{
	fprintf(ucodeFile, "           %s  %d\n", value, p);
	printf("           %s  %d\n", value, p);

}

void emit2(char *value, int p, int q)
{
	fprintf(ucodeFile, "           %s  %d  %d\n", value, p, q);
	printf("           %s  %d  %d\n", value, p, q);

}

void emitJump(char *value, char *label)
{
	fprintf(ucodeFile, "           %s  %s\n", value, label);
	printf("           %s  %s\n", value, label);

}

void emitLabel(char *label)
{
	int length;
	length = strlen(label);
	fprintf(ucodeFile, "%s", label);
	printf("%s", label);
	for (; length < LABEL_SIZE - 1; length++) {
		fprintf(ucodeFile, " ");
		printf(" ");
	}
	fprintf(ucodeFile, "nop\n");
	printf("nop\n");

}

void rv_emit(Node *ptr)
{
	int stIndex;

	if (ptr->token.number == tnumber)        // number
		emit1("ldc", ptr->token.value.num);
	else {                                  // identifier
		stIndex = lookup(ptr->token.value.id);
		if (stIndex == -1) return;
		if (symbolTable[stIndex].typeQualifier == CONST_TYPE) // constant
			emit1("ldc", symbolTable[stIndex].initialValue);
		else if (symbolTable[stIndex].width > 1)     // array var
			emit2("lda", symbolTable[stIndex].base, symbolTable[stIndex].offset);
		else                                        // simple var
			emit2("lod", symbolTable[stIndex].base, symbolTable[stIndex].offset);
	}
}

void processOperator(Node *ptr)
{
	switch (ptr->token.number) {
		// assignment operator
	case ASSIGN_OP:
	{
		Node *lhs = ptr->son, *rhs = ptr->son->brother;
		int stIndex;

		// step 1: generate instructions for left-hand side if INDEX node.
		if (lhs->noderep == nonterm) { // array variable
			lvalue = 1;
			processOperator(lhs);
			lvalue = 0;
		}

		// step 2: generate instructions for right-hand side
		if (rhs->noderep == nonterm) processOperator(rhs);
		else rv_emit(rhs);

		// step 3: generate a store instruction
		if (lhs->noderep == terminal) { // simple variable
			stIndex = lookup(lhs->token.value.id);
			if (stIndex == -1) {
				printf("undefined variable : %s\n", lhs->token.value.id);
				return;
			}
			if (ptr->son->brother->token.number == ASSIGN_OP) {
				stIndex = lookup(ptr->son->brother->son->token.value.id);
				emit2("lod", symbolTable[stIndex].base, symbolTable[stIndex].offset);
				//printf("%d", ptr->son->brother->token.number);
			}
			stIndex = lookup(lhs->token.value.id);
			emit2("str", symbolTable[stIndex].base, symbolTable[stIndex].offset);
		}

		else
			emit0("sti");
		break;
	}

	// complex assignment operators
	case ADD_ASSIGN: case SUB_ASSIGN: case MUL_ASSIGN:
	case DIV_ASSIGN: case MOD_ASSIGN:
	{
		Node *lhs = ptr->son, *rhs = ptr->son->brother;
		int nodeNumber = ptr->token.number;
		int stIndex;

		ptr->token.number = ASSIGN_OP;
		// step 1: code generation for left hand side
		if (lhs->noderep == nonterm) {
			lvalue = 1;
			processOperator(lhs);
			lvalue = 0;
		}
		ptr->token.number = nodeNumber;
		// step 2: code generation for repeating part
		if (lhs->noderep == nonterm)
			processOperator(lhs);
		else rv_emit(lhs);
		// step 3: code generation for right hand side
		if (rhs->noderep == nonterm)
			processOperator(rhs);
		else rv_emit(rhs);
		// step 4: emit the corresponding operation code
		switch (ptr->token.number) {
		case ADD_ASSIGN: emit0("add"); break;
		case SUB_ASSIGN: emit0("sub"); break;
		case MUL_ASSIGN: emit0("mult"); break;
		case DIV_ASSIGN: emit0("div"); break;
		case MOD_ASSIGN: emit0("mod"); break;
		}
		// step 5: code generation for store code
		if (lhs->noderep == terminal) {
			stIndex = lookup(lhs->token.value.id);
			if (stIndex == -1) {
				printf("undefined variable : %s\n", lhs->son->token.value.id);
				return;
			}
			emit2("str", symbolTable[stIndex].base, symbolTable[stIndex].offset);
		}
		else
			emit0("sti");
		break;
	}

	// binary(arithmetic/relational/logical) operators
	case ADD: case SUB: case MUL: case DIV: case MOD:
	case EQ: case NE: case GT: case LT: case GE: case LE:
	case LOGICAL_AND: case LOGICAL_OR:
	{
		Node *lhs = ptr->son, *rhs = ptr->son->brother;

		// step 1: visit left operand
		if (lhs->noderep == nonterm) processOperator(lhs);
		else rv_emit(lhs);
		// step 2: visit right operand
		if (rhs->noderep == nonterm) processOperator(rhs);
		else rv_emit(rhs);
		// step 3: visit root
		switch (ptr->token.number) {
		case ADD: emit0("add"); break;            // arithmetic operators
		case SUB: emit0("sub"); break;
		case MUL: emit0("mult"); break;
		case DIV: emit0("div"); break;
		case MOD: emit0("mod"); break;
		case EQ: emit0("eq"); break;              // relational operators
		case NE: emit0("ne"); break;
		case GT: emit0("gt"); break;
		case LT: emit0("lt"); break;
		case GE: emit0("ge"); break;
		case LE: emit0("le"); break;
		case LOGICAL_AND: emit0("and"); break;  // logical operators
		case LOGICAL_OR: emit0("or"); break;
		}
		break;
	}

	// unary operators
	case UNARY_MINUS: case LOGICAL_NOT:
	{
		Node *p = ptr->son;

		if (p->noderep == nonterm) processOperator(p);
		else rv_emit(p);
		switch (ptr->token.number) {
		case UNARY_MINUS: emit0("neg"); break;
		case LOGICAL_NOT: emit0("notop"); break;
		}
		break;
	}

	// increment/decrement operators
	case PRE_INC: case PRE_DEC: case POST_INC: case POST_DEC:
	{
		Node *p = ptr->son; Node *q;
		int stIndex; // int amount = 1;
		if (p->noderep == nonterm) processOperator(p); // compute operand
		else rv_emit(p);

		q = p;
		while (q->noderep != terminal) q = q->son;
		if (!q || (q->token.number != tident)) {
			printf("increment/decrement operators can not be applied in expression\n");
			return;
		}
		stIndex = lookup(q->token.value.id);
		if (stIndex == -1) return;

		switch (ptr->token.number) {
		case PRE_INC:
			emit0("inc");
			// if(isOperation(ptr)) emit0(dup);
			break;
		case PRE_DEC:
			emit0("dec");
			// if(isOperation(ptr)) emit0(dup);
			break;
		case POST_INC:
			// if(isOperation(ptr)) emit0(dup);
			emit0("inc");
			break;
		case POST_DEC:
			// if(isOperation(ptr)) emit0(dup);
			emit0("dec");
			break;
		}
		if (p->noderep == terminal) {
			stIndex = lookup(p->token.value.id);
			if (stIndex == -1) return;
			emit2("str", symbolTable[stIndex].base, symbolTable[stIndex].offset);
		}
		else if (p->token.number == INDEX) { // compute index
			lvalue = 1;
			processOperator(p);
			lvalue = 0;
			emit0("swp");
			emit0("sti");
		}
		else printf("error in increment/decrement operators\n");
		break;
	}

	case INDEX:
	{
		Node *indexExp = ptr->son->brother;
		int stIndex;

		if (indexExp->noderep == nonterm) processOperator(indexExp);
		else rv_emit(indexExp);
		stIndex = lookup(ptr->son->token.value.id);
		if (stIndex == -1) {
			printf("undefined variable: %s\n", ptr->son->token.value.id);
			return;
		}
		emit2("lda", symbolTable[stIndex].base, symbolTable[stIndex].offset);
		emit0("add");
		if (!lvalue) emit0("ldi"); // rvalue
		break;
	}

	case CALL:
	{
		Node *p = ptr->son;     // function name
		char *functionName;
		int stIndex; int noArguments;
		if (checkPredefined(p))  // predefined(Library) functions
			break;

		// handle for user function
		functionName = p->token.value.id;
		stIndex = lookup(functionName);
		if (stIndex == -1) break; // undefined function !!!
		noArguments = symbolTable[stIndex].width;

		emit0("ldp");
		p = p->brother;     // ACTUAL_PARAM
		while (p) {          // processing actual arguemtns
			if (p->noderep == nonterm) processOperator(p);
			else rv_emit(p);
			noArguments--;
			p = p->brother;
		}
		if (noArguments > 0)
			printf("%s: too few actual arguments", functionName);
		if (noArguments < 0)
			printf("%s: too many actual arguments", functionName);
		emitJump("call", ptr->son->token.value.id);
		break;
	}
	} // end switch
}


void processCondition(Node *ptr)
{
	if (ptr->noderep == nonterm) processOperator(ptr);
	else rv_emit(ptr);
}


int checkPredefined(Node *ptr)
{
	Node *p = NULL;
	int stIndex;

	if (strcmp(ptr->token.value.id, "read") == 0) {
		emit0("ldp");
		p = ptr->brother; // ACTUAL_PARAM
		stIndex = lookup(p->token.value.id);
		while (p) {
			if (p->noderep == nonterm) processOperator(p);
			else
				emit2("lda", symbolTable[stIndex].base, symbolTable[stIndex].offset);
			p = p->brother;
		}
		emitJump("call", "read");
		return 1;
	}
	else if (strcmp(ptr->token.value.id, "write") == 0) {
		emit0("ldp");
		p = ptr->brother; // ACTint checkPredefined(Node *ptr)
		{
			Node *p = NULL;
			int stIndex;

			if (strcmp(ptr->token.value.id, "read") == 0) {
				emit0("ldp");
				p = ptr->brother; // ACTUAL_PARAM
				while (p) {
					if (p->noderep == nonterm) processOperator(p);
					else {
						stIndex = lookup(p->token.value.id);
						emit2("lda", symbolTable[stIndex].base, symbolTable[stIndex].offset);
					}
					p = p->brother;
				}
				emitJump("call", "read");
				return 1;
			}
			else if (strcmp(ptr->token.value.id, "write") == 0) {
				emit0("ldp");
				p = ptr->brother; // ACTUAL_PARAM
				while (p) {
					if (p->noderep == nonterm) processOperator(p);
					else {
						stIndex = lookup(p->token.value.id);
						emit2("lod", symbolTable[stIndex].base, symbolTable[stIndex].offset);
					}
					p = p->brother;
				}
				emitJump("call", "write");
				return 1;
			}
			else if (strcmp(ptr->token.value.id, "lf") == 0) {
				emitJump("call", "lf");
				return 1;
			}
			return 0;
		}//AL_PARAM
		stIndex = lookup(p->token.value.id);
		while (p) {
			if (p->noderep == nonterm) processOperator(p);
			else
				emit2("lod", symbolTable[stIndex].base, symbolTable[stIndex].offset);
			p = p->brother;
		}
		emitJump("call", "write");
		return 1;
	}
	else if (strcmp(ptr->token.value.id, "lf") == 0) {
		emitJump("call", "lf");
		return 1;
	}
	return 0;
}

void genLabel(char *label)
{
	static int labelNum = 0;
	sprintf(label, "$$%d", labelNum++);
}


int typeSize(int typeSpecifier)
{
	if (typeSpecifier == INT_TYPE)
		return 1;
	else {
		printf("not yet implemented\n");
		return 1;
	}
}

void icg_error(int err)
{
	printf("ICG_ERROR: %d\n", err);
}


void processArrayVariable(Node *ptr, int typeSpecifier, int typeQualifier)
{
	Node *p = ptr->son; // variable name(=> identifier)
	int stIndex, size;

	if (ptr->token.number != ARRAY_VAR) {
		printf("error in ARRAY_VAR\n");
		return;
	}
	if (p->brother == NULL) // no size
		printf("array size must be specified\n");
	else size = p->brother->token.value.num;

	size *= typeSize(typeSpecifier);

	stIndex = insert(p->token.value.id, typeSpecifier, typeQualifier,
		base, offset, size, 0);
	offset += size;
}

void emitFunc(char *FuncName, int operand1, int operand2, int operand3)
{
	fprintf(ucodeFile, "%-10s %-s  %d  %d  %d\n", FuncName, "fun", operand1, operand2, operand3);
	printf("%-10s %-s  %d  %d  %d\n", FuncName, "fun", operand1, operand2, operand3);
}


void semantic(int n)
{
	printf("reduced rule number = %d\n", n);
}

void printToken(struct tokenType token)
{
	if (token.number == tident)
		printf("%s", token.value.id);
	else if (token.number == tnumber)
		printf("%d", token.value.num);
	else
		printf("%s", tokenName[token.number]);
}

void dumpStack()
{
	int i, start;

	if (sp > 10) start = sp - 10;
	else start = 0;

	printf("\n *** dump state stack :");
	for (i = start; i <= sp; i++)
		printf(" %d", stateStack[i]);

	printf("\n *** dump symbol stack :");
	for (i = start; i <= sp; i++)
		printf(" %d", symbolStack[i]);
	printf("\n");
}

void errorRecovery()
{
	struct tokenType tok;
	int parenthesisCount, braceCount;
	int i;

	// step 1: skip to the semicolon
	parenthesisCount = braceCount = 0;
	while (1) {
		tok = scanner();
		if (tok.number == teof) exit(1);
		if (tok.number == tlparen) parenthesisCount++;
		else if (tok.number == trparen) parenthesisCount--;
		if (tok.number == tlbrace) braceCount++;
		else if (tok.number == trbrace) braceCount--;
		if ((tok.number == tsemicolon) && (parenthesisCount <= 0) && (braceCount <= 0))
			break;
	}

	// step 2: adjust state stack
	for (i = sp; i >= 0; i--) {
		// statement_list -> statement_list . statement
		if (stateStack[i] == 36) break; // second statement part

										// statement_list -> . statement
										// statement_list -> . statement_list statement
		if (stateStack[i] == 24) break; // first statement part

										// declaration_list -> declaration_list . declaration
		if (stateStack[i] == 25) break; // second internal dcl

										// declaration_list -> . declaration
										// declaration_list -> . declaration_list declaration
		if (stateStack[i] == 17) break; // internal declaration

										// external declaration
										// external_dcl -> . declaration
		if (stateStack[i] == 2) break;  // after first external dcl
		if (stateStack[i] == 0) break;  // first external declaration
	}
	sp = i;
}

//SDT
int meaningfulToken(struct tokenType token)
{
	if ((token.number == tident) || (token.number == tnumber)) return 1;
	else return 0;
}

Node* buildNode(struct tokenType token)
{
	Node *ptr;
	ptr = (Node*)malloc(sizeof(Node));
	if (!ptr) {
		printf("malloc error in buildNode()\n");
		exit(1);
	}
	ptr->token = token;
	ptr->noderep = terminal;
	ptr->son = ptr->brother = NULL;
	return ptr;
}

Node* buildTree(int nodeNumber, int rhsLength)
{
	int i, j, start;
	Node *first, *ptr;

	i = sp - rhsLength + 1;

	// step 1: find a first index with node in value stack
	while (i <= sp && valueStack[i] == NULL) i++;
	if (!nodeNumber && i > sp) return NULL;
	start = i;

	// step 2: linking brothers
	while (i <= sp - 1) {
		j = i + 1;
		while (j <= sp && valueStack[j] == NULL) j++;
		if (j <= sp) {
			ptr = valueStack[i];
			while (ptr->brother) ptr = ptr->brother;
			ptr->brother = valueStack[j];
		}
		i = j;
	}
	first = (start > sp) ? NULL : valueStack[start];

	// step 3: making subtree root and linking son
	if (nodeNumber) {
		ptr = (Node*)malloc(sizeof(Node));
		if (!ptr) {
			printf("malloc error in buildTree()\n");
			exit(1);
		}
		ptr->token.number = nodeNumber;
		//ptr->token.tokenValue = NULL;
		ptr->noderep = nonterm;
		ptr->son = first;
		ptr->brother = NULL;
		return ptr;
	}
	else return first;
}

void printNode(Node *pt, int indent)
{

	extern FILE* astFile;
	int i;

	for (i = 1; i <= indent; i++) fprintf(astFile, " ");
	if (pt->noderep == terminal) {
		if (pt->token.number == tident)
			fprintf(astFile, " Terminal: %s", pt->token.value.id);
		else if (pt->token.number == tnumber)
			fprintf(astFile, " Terminal: %d", pt->token.value.num);
	}
	else { // nonterminal node
		int i;
		i = (int)(pt->token.number);
		fprintf(astFile, " Nonterminal: %s", nodeName[i]);
	}
	fprintf(astFile, "\n");

}

void printTree(Node *pt, int indent)
{
	Node *p = pt;
	while (p != NULL) {
		printNode(p, indent);
		if (p->noderep == nonterm) printTree(p->son, indent + 5);
		p = p->brother;
	}
}

Node* parser()
{
	extern int parsingTable[NO_STATES][NO_SYMBOLS + 1];
	extern int leftSymbol[NO_RULES + 1], rightLength[NO_RULES + 1];
	int entry, ruleNumber, lhs;
	int currentState;
	struct tokenType token;
	Node* ptr;

	sp = 0; stateStack[sp] = 0; // initial state
	token = scanner();
	while (1) {
		currentState = stateStack[sp];
		entry = parsingTable[currentState][token.number];
		if (entry > 0) {                    // shift action
			sp++;
			if (sp > PS_SIZE) {
				printf("critical compiler error: parsing stack overflow");
				exit(1);
			}
			symbolStack[sp] = token.number;
			stateStack[sp] = entry;
			valueStack[sp] = meaningfulToken(token) ? buildNode(token) : NULL;
			token = scanner();
		}
		else if (entry < 0) {               // reduce action
			ruleNumber = -entry;
			if (ruleNumber == GOAL_RULE) {  // accept action
											/*if (errcnt == 0) printf(" *** valid source ***\n");
											else printf(" *** error in source : %d\n", errcnt);
											return;*/
				return valueStack[sp - 1];
			}
			//semantic(ruleNumber);
			ptr = buildTree(ruleName[ruleNumber], rightLength[ruleNumber]);
			sp = sp - rightLength[ruleNumber];
			lhs = leftSymbol[ruleNumber];
			currentState = parsingTable[stateStack[sp]][lhs];
			sp++;
			symbolStack[sp] = lhs;
			stateStack[sp] = currentState;
			valueStack[sp] = ptr;
		}
		else {                              // error action
			printf(" === error in source ===\n");
			//errcnt++;
			printf("Current Token : ");
			printToken(token);
			dumpStack();
			errorRecovery();
			token = scanner();
		}
	} // while
} // parser

void processSimpleVariable(Node *ptr, int typeSpecifier, int typeQualifier)
{
	Node *p = ptr->son;     // variable name(=> identifier)
	Node *q = ptr->brother; // initial value part
	int stIndex, size, initialValue;
	int sign = 1;

	if (ptr->token.number != SIMPLE_VAR) printf("error in SIMPLE_VAR\n");

	if (typeQualifier == CONST_TYPE) {   // constant type
		if (q == NULL) {
			printf("%s must have a constant value\n", ptr->son->token.value.id);
			return;
		}
		if (q->token.number == UNARY_MINUS) {
			sign = -1;
			q = q->son;
		}
		initialValue = sign * q->token.value.num;

		stIndex = insert(p->token.value.id, typeSpecifier, typeQualifier,
			0/*base*/, 0/*offset*/, 0/*width*/, initialValue);
	}
	else {
		size = typeSize(typeSpecifier);
		stIndex = insert(p->token.value.id, typeSpecifier, typeQualifier,
			base, offset, size, 0);
		offset += size;
	}
}


void processDeclaration(Node *ptr)
{
	int typeSpecifier, typeQualifier;
	Node *p, *q;

	if (ptr->token.number != DCL_SPEC) icg_error(4);

	// step 1: process DCL_SPEC
	typeSpecifier = INT_TYPE; // default type
	typeQualifier = VAR_TYPE;
	p = ptr->son;
	while (p) {
		if (p->token.number == INT_NODE) typeSpecifier = INT_TYPE;
		else if (p->token.number == CONST_NODE)
			typeQualifier = CONST_TYPE;
		else { // AUTO, EXTERN, REGISTER, FLOAT, DOUBLE, SIGNED, UNSIGEND
			printf("not yet implemented\n");
			return;
		}
		p = p->brother;
	}

	// step 2: process DCL_ITEM
	p = ptr->brother;
	if (p->token.number != DCL_ITEM) icg_error(5);

	while (p) {
		q = p->son; // SIMPLE_VAR or ARRAY_VAR
		switch (q->token.number) {
		case SIMPLE_VAR: // simple variable
			processSimpleVariable(q, typeSpecifier, typeQualifier);
			break;
		case ARRAY_VAR:  // array variable
			processArrayVariable(q, typeSpecifier, typeQualifier);
			break;
		default:
			printf("error in SIMPLE_VAR or ARRAY_VAR\n");
			break;
		}
		p = p->brother;
	}
}

void processFuncHeader(Node *ptr)
{
	int noArguments, returnType;
	int stIndex;
	Node *p;

	// printf("processFuncHeader\n");
	if (ptr->token.number != FUNC_HEAD)
		printf("error in processFuncHeader\n");
	// step 1: process the function return type
	p = ptr->son->son;
	while (p) {
		if (p->token.number == INT_NODE) returnType = INT_TYPE;
		else if (p->token.number == VOID_NODE) returnType = VOID_TYPE;
		else printf("invalid function return type\n");
		p = p->brother;
	}

	// step 2: count the number of formal parameters
	p = ptr->son->brother->brother; // FORMAL_PARA
	p = p->son; // PARAM_DCL

	noArguments = 0;
	while (p) {
		noArguments++;
		p = p->brother;
	}

	// step 3: insert the function name
	stIndex = insert(ptr->son->brother->token.value.id, returnType, FUNC_TYPE,
		1/*base*/, 0/*offset*/, noArguments/*width*/, 0/*initialValue*/);
	// if(!strcmp("main", functionName)) mainExist = 1;
}

void processFunction(Node *ptr)
{
	Node *p, *q;
	int sizeOfVar = 0;
	int numOfVar = 0;
	int stIndex;

	base++;
	offset = 1;

	if (ptr->token.number != FUNC_DEF) icg_error(4);

	// step 1: process formal parameters
	p = ptr->son->son->brother->brother; // FORMAL_PARA
	p = p->son; // PARAM_DCL
	while (p) {
		if (p->token.number == PARAM_DCL) {
			processDeclaration(p->son); // DCL_SPEC
			sizeOfVar++;
			numOfVar++;
		}
		p = p->brother;
	}

	// step 2: process the declaration part in function body
	p = ptr->son->brother->son->son; // DCL
	while (p) {
		if (p->token.number == DCL) {
			processDeclaration(p->son);
			q = p->son->brother;
			while (q) {
				if (q->token.number == DCL_ITEM) {
					if (q->son->token.number == ARRAY_VAR) {
						sizeOfVar += q->son->son->brother->token.value.num;
					}
					else {
						sizeOfVar += 1;
					}
					numOfVar++;
				}
				q = q->brother;
			}
		}
		p = p->brother;
	}

	// step 3: emit the function start code
	p = ptr->son->son->brother;	// IDENT
	emitFunc(p->token.value.id, sizeOfVar, base, 2);
	for (stIndex = symbolTableTop - numOfVar + 1; stIndex<symbolTableTop + 1; stIndex++) {
		emitSym(symbolTable[stIndex].base, symbolTable[stIndex].offset, symbolTable[stIndex].width);
	}

	// step 4: process the statement part in function body
	p = ptr->son->brother;	// COMPOUND_ST
	processStatement(p);

	// step 5: check if return type and return value
	p = ptr->son->son;	// DCL_SPEC
	if (p->token.number == DCL_SPEC) {
		p = p->son;
		if (p->token.number == VOID_NODE)
			emit0("ret");
		else if (p->token.number == CONST_NODE) {
			if (p->brother->token.number == VOID_NODE)
				emit0("ret");
		}
	}

	// step 6: generate the ending codes
	emit0("end");
	base--;
	levelTop++;
}

/*------------------- generate Symbol Table -----------------------------*/
void genSym(int base)
{
	int stIndex;

	//	fprintf(ucodeFile, "// Information for Symbol Table\n");

	for (stIndex = 0; stIndex <= symbolTableTop; stIndex++) {
		if ((symbolTable[stIndex].typeQualifier == FUNC_TYPE) ||
			(symbolTable[stIndex].typeQualifier == CONST_TYPE)) continue;

		if (base == symbolTable[stIndex].base)
			emitSym(symbolTable[stIndex].base, symbolTable[stIndex].offset, symbolTable[stIndex].width);
	}
}



void codeGen(Node *ptr)
{
	Node *p;
	int globalSize;

	initSymbolTable();
	// step 1: process the declaration part
	for (p = ptr->son; p; p = p->brother) {
		if (p->token.number == DCL) processDeclaration(p->son);
		else if (p->token.number == FUNC_DEF) processFuncHeader(p->son);
		else icg_error(3);
	}

	// dumpSymbolTable();
	globalSize = offset - 1;
	// printf("size of global variables = %d\n", globalSize);

	genSym(base);

	// step 2: process the function part
	for (p = ptr->son; p; p = p->brother)
		if (p->token.number == FUNC_DEF) processFunction(p);
	// if(!mainExist) warningmsg("main does not exist");

	// step 3: generate code for starting routine
	//      bgn     globalSize
	//      ldp
	//      call    main
	//      end
	emit1("bgn", globalSize);
	emit0("ldp");
	emitJump("call", "main");
	emit0("end");
}




void processStatement(Node *ptr)
{
	Node *p;
	int returnWithValue;

	switch (ptr->token.number) {
	case COMPOUND_ST:
		p = ptr->son->brother; // STAT_LIST
		p = p->son;
		while (p) {
			processStatement(p);
			p = p->brother;
		}
		break;
	case EXP_ST:
		if (ptr->son != NULL) processOperator(ptr->son);
		break;
	case RETURN_ST:
		if (ptr->son != NULL) {
			returnWithValue = 1;
			p = ptr->son;
			if (p->noderep == nonterm)
				processOperator(p); // return value
			else rv_emit(p);
			emit0("retv");
		}
		else
			emit0("ret");
		break;
	case IF_ST:
	{
		char label[LABEL_SIZE];

		genLabel(label);
		processCondition(ptr->son);             // condition part
		emitJump("fjp", label);
		processStatement(ptr->son->brother);    // true part
		emitLabel(label);
	}
	break;
	case IF_ELSE_ST:
	{
		char label1[LABEL_SIZE], label2[LABEL_SIZE];

		genLabel(label1); genLabel(label2);
		processCondition(ptr->son);             // condition part
		emitJump("fjp", label1);
		processStatement(ptr->son->brother);    // true part
		emitJump("ujp", label2);
		emitLabel(label1);
		processStatement(ptr->son->brother->brother); // false part
		emitLabel(label2);
	}
	break;
	case WHILE_ST:
	{
		char label1[LABEL_SIZE], label2[LABEL_SIZE];

		genLabel(label1); genLabel(label2);
		emitLabel(label1);
		processCondition(ptr->son);             // condition part
		emitJump("fjp", label2);
		processStatement(ptr->son->brother);    // loop body
		emitJump("ujp", label1);
		emitLabel(label2);
	}
	break;
	default:
		printf("not yet implemented.\n");
		break;
	} // end switch
}


/*------------------ TABLE INITIALIZE FUNCTION -----------------------------*/
void initSymbolTable()
{
	int i;

	symbolTableTop = -1;
	levelTop = -1;
	base = 1; offset = 1;

	for (i = 0; i < HASH_BUCKET_SIZE; i++)
		hashBucket[i] = -1;

	for (i = 0; i < LEVEL_STACK_SIZE; i++)
		levelStack[i] = 0;
}

/*--------------------- HASH FUNCTION --------------------------------------*/
int hash(char *symbolName)
{
	int hashValue;

	for (hashValue = 0; *symbolName; symbolName++)
		hashValue += *symbolName;

	return (hashValue % HASH_BUCKET_SIZE);
}


/*------------------ LOOKUP FUNCTION ---------------------------------------*/
int lookup(char *symbol)
{
	int stIndex;

	stIndex = hash(symbol);
	for (stIndex = hashBucket[stIndex];
		stIndex >= 0 && strcmp(symbol, symbolTable[stIndex].symbolName);
		stIndex = symbolTable[stIndex].nextIndex);

	if (stIndex < 0)
		printf("undefined identifier: %s\n", symbol);

	return stIndex;
}




/*------------------ SET BLOCK FUNCTION ------------------------------------*/
void set()
{
	++levelTop;
	//	printf("level stack top in set function = %d\n", levelTop);
	if (levelTop > LEVEL_STACK_SIZE)
		printf("error: level stack overflow.\n");

	levelStack[levelTop] = symbolTableTop;
	++base; offset = 1;
}

/*--------------------- RESET BLOCK FUNCTION -------------------------------*/
void reset(void)
{
	int hashValue;
	int i;

	i = symbolTableTop;
	symbolTableTop = levelStack[levelTop];
	levelTop--;

	for (; i>symbolTableTop; i--) {
		hashValue = hash(symbolTable[i].symbolName);
		hashBucket[hashValue] = symbolTable[i].nextIndex;
	}
	base--;

}

/*-------------------   Dump Symbol Table   -----------------------------*/
void dumpSymbolTable()
{
	int i;

	printf(" === Dump Symbol Table ===\n");
	for (i = 0; i <= symbolTableTop; i++)
		printf("id = %-12s : \ttype = %s, \tqualifier = %s, \tbase = %d, \toffset = %d \twidth = %d, \tvalue = %d\n",
			symbolTable[i].symbolName,
			typeName[symbolTable[i].typeSpecifier],
			qualifierName[symbolTable[i].typeQualifier],
			symbolTable[i].base,
			symbolTable[i].offset,
			symbolTable[i].width,
			symbolTable[i].initialValue);
}


/*-------------------- INSERT SYMBOL FUNCTION ------------------------------*/
int insert(char *symbol, int specifier, int qualifier, int base, int offset,
	int width, int initialValue)
{
	int hashValue;
	int i;

	hashValue = hash(symbol);
	for (i = hashBucket[hashValue]; i >= 0 &&
		strcmp(symbol, symbolTable[i].symbolName); )
		i = symbolTable[i].nextIndex;
	if ((i >= 0) && (base == symbolTable[i].base)) {	// found
		printf("multiply defined identifier : %s\n", symbol);

		printf("base = %d, symbolTable[i].base = %d\n", base, symbolTable[i].base);
		return -1;
	}

	if (symbolTableTop > SYMTAB_SIZE) {
		printf("symbol table overflow\n");
		dumpSymbolTable();
		exit(1);
	}

	symbolTableTop++;
	strcpy(symbolTable[symbolTableTop].symbolName, symbol);
	symbolTable[symbolTableTop].typeSpecifier = specifier;
	symbolTable[symbolTableTop].typeQualifier = qualifier;
	symbolTable[symbolTableTop].base = base;
	symbolTable[symbolTableTop].offset = offset;
	symbolTable[symbolTableTop].width = width;
	symbolTable[symbolTableTop].initialValue = initialValue;

	symbolTable[symbolTableTop].nextIndex = hashBucket[hashValue];
	hashBucket[hashValue] = symbolTableTop;

	return symbolTableTop;
}