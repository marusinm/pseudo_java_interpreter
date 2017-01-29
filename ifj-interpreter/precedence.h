/* *************************** precedence.h ********************************* */
/* Soubor:              precedence.h                                          */
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

#ifndef IFJ2016_PRECEDENCE_H
#define IFJ2016_PRECEDENCE_H

#include <stdio.h>
#include <stdbool.h>
#include "scanner.h"
#include "stack.h"
#include "stack_test.h"
#include "ilist.h"
#include "ial.h"
#include "garb.h"

#define NUMBER_OF_RULES 15
#define PRECEDENCE_TABLE_SIZE 19
//#define _DEBUG_PRECEDENCE


void precedence_init( short (*precedence_table)[PRECEDENCE_TABLE_SIZE] );
entry_s *precedenceCore(int parser_phase);
int compareRules(int defined_rules[NUMBER_OF_RULES][3], int rule_to_compare[3]);
void generateConstant(tPrecStack *ptrstack, tPrecStack *helpstack, t_hashtable *hashtable);
void generateFullInstruction(tPrecStack *helpstack, instructions instType, tToken token, tPrecStack *ptrstack);
int generateRuleFromStack(int **defined_rules, tPrecStack *ptrstack, tToken temp_token, t_hashtable *hashtable);
void customPrintState(int cislo);
void customPrintRule(int cislo);


enum operators {
    OP_LBRACE = 1,  // ( (1)
    OP_RBRACE,      // ) (2)
    OP_ADD,         // + (3)
    OP_SUB,         // - (4)
    OP_MUL,         // * (5)
    OP_DIV,         // / (6)
    OP_LESS,        // < (7)
    OP_MORE,        // > (8)
    OP_LESSEQUAL,   // <= (9)
    OP_MOREEQUAL,   // >= (10)
    OP_EQUAL,       // == (11)
    OP_NOTEQUAL,    // != (12)

    IDENTIFIER,     // i (13)
    DOLLAR,         // $ (14)
    EXPRESSION,     // E (15) neterminal

    /*
     * Rozsirenie BOOLEAN
     */
    CONDITIONAL_AND, // && (16)
    CONDITIONAL_OR,  // || (17)
    LOGICAL_NOT      // !  (18)
};


#endif //IFJ2016_PRECEDENCE_H
