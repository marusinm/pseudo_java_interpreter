/* ******************************** ilist.h ********************************* */
/* Soubor:              ilist.h                                               */
/* Datum:               prosinec.2016                                         */
/* Predmet:             Formalni jazyky a prekladace (IFJ)                    */
/* Projekt:             Implementace interpretu jazyka IFJ16                  */
/* Varianta zadani:                                                           */
/* Titul,Autori, login:         Javorka Martin        xjavor18                */
/*                              Marušin Marek         xmarus08                */
/*                              Utkin Kirill          xutkin00                */
/*                              Mrva Marián           xmrvam01                */
/*                              Shapochkin Victor     xshapo00                */
/* ************************************************************************** */

#ifndef IFJ2016_ILIST_H
#define IFJ2016_ILIST_H

#include <stdio.h>
#include <stdlib.h>


// Types of instructions

typedef enum {
    I_START,            // - / - / -
    I_STOP,             // - / - / -
    I_READLN_INT,       // arg1 / - / -
    I_READLN_DOUBLE,    // arg1 / - / -
    I_READLN_STRING,    // arg1 / - / -
    I_PRINT,            // arg_expr / - / -
    I_LENGTH,           // arg1 / - / -
    I_SUBSTR,           // arg1 / arg2 / arg3
    I_COMPARE,          // arg1 / arg2 / -
    I_FIND,             // arg1 / arg2 / -
    I_SORT,             // arg1 / arg2 / -
    I_ADD,              // dst / op1 / op2
    I_SUB,              // dst / op1 / op2
    I_MUL,              // dst / op1 / op2
    I_DIV,              // dst / op1 / op2
    I_CONCATENATE,      // dst / op1 / op2
    I_LESS,             // dst / op1 / op2
    I_MORE,             // dst / op1 / op2
    I_LESS_EQUAL,       // dst / op1 / op2
    I_MORE_EQUAL,       // dst / op1 / op2
    I_EQUAL,            // dst / op1 / op2
    I_NOT_EQUAL,        // dst / op1 / op2
    /*
     * Extension operations (BOOLEAN)
     */
    I_NOT,              // dst / op1 / -
    I_OR,               // dst / op1 / op2
    I_AND,              // dst / op1 / op2
    /*
     * End of extension operations
     */
    I_ASSIGN,           // dst / src / -
    I_CALL_FUNC,        // dst / ptrSymbolItem / list_of_args
    I_RETURN,           // ptr_to_function / return_val / -
    I_SET_PARAM,        // dst / src / -

    I_JUMP,             // ptr_instruction / - / -
    I_JUMP_IF_FALSE,     // ptr_instruciton / dst / -
    I_LABEL             // - / - / -
    // TODO JUMPS ... etc.
} instructions;


typedef struct {
    instructions instType;  // type of instruction  I_LESS
    void *addr1; // adress 1 - entry_s destination
    void *addr2; // adress 2 - entry_s operand 1
    void *addr3; // adress 3 - entry_s operand 2
} tInstr;

typedef struct listItem
{
    tInstr Instruction;
    struct listItem *nextItem;
} tListItem;

typedef struct
{
    struct listItem *first;  // ukazatel na prvy prvok
    struct listItem *last;   // ukazatel na posledny prvok
    struct listItem *active; // ukazatel na aktivny prvok
} tListOfInstr;

//prototypy funkcii
void listInit(tListOfInstr *L);
void listFree(tListOfInstr *L);
void listInsertLast(tListOfInstr *L, tInstr I);
void listFirst(tListOfInstr *L);
void listNext(tListOfInstr *L);
void listGoto(tListOfInstr *L, void *gotoInstr);
void *listGetPointerLast(tListOfInstr *L);
tInstr *listGetData(tListOfInstr *L);
void listLastActive(tListOfInstr *L);
void print_elements_of_list(tListOfInstr TL);
void customPrintInstType(int instType);
#endif //IFJ2016_ILIST_H