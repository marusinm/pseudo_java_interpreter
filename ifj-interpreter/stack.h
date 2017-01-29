/* ****************************** stack.h *********************************** */
/* Soubor:              stack.h                                               */
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

#ifndef IFJ2016_STACK_H
#define IFJ2016_STACK_H

#include <stdio.h>
#include "scanner.h"
#include "ial.h"

#define STACK_INIT_SIZE 200//inicializacni velikost zasobniku

typedef struct {
    int entry;
    tToken token;
    entry_s *item; //item from symbol table
}tData;

typedef struct {
    int top;
    tData *arr[];
}tPrecStack;

void stackInit(tPrecStack *s);
int stackEmpty(const tPrecStack *s);
int stackFull(const tPrecStack *s);
void stackTop(const tPrecStack *s, tData *value);
int stackTopTerminal(const tPrecStack *s, tData *value);
void stackPop(tPrecStack *s);
void stackPush(tPrecStack *s, int entry, tToken token, entry_s *item);

#endif //IFJ2016_STACK_H
