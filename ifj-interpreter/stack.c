/* ****************************** stack.c *********************************** */
/* Soubor:              stack.c                                               */
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

#include <stdlib.h>
#include "stack.h"
#include "scanner.h"
#include "garb.h"

int STACK_SIZE = STACK_INIT_SIZE;

void stackInit(tPrecStack *s) {

    if (s != NULL)
        s->top = -1; // init stack by setting top position to empty stack

//    s->arr = malloc(STACK_SIZE*sizeof(tData *));
}

int stackEmpty(const tPrecStack *s) {
    // return true if the top element respond to empty stack "-1"
    return s->top == -1;
}

int stackFull(const tPrecStack *s) {

    // return true if the top element respond to full stack
    return s->top == (STACK_SIZE - 1);
}

void stackTop(const tPrecStack *s, tData *value) {

    if (stackEmpty(s))
        return;

    value->entry = (s->arr[s->top])->entry;
    value->token = (s->arr[s->top])->token;
    value->item = (s->arr[s->top])->item;
}

int stackTopTerminal(const tPrecStack *s, tData *value) {
// !!!!
//    nechť funkce top vrací terminál na zásobníku nejblíže vrcholu !!!
// !!!!


    if (stackEmpty(s))
        return -1;

    int index = s->top;
    while ((s->arr[index])->entry == s_expression || (s->arr[index])->entry == precedence_less) {
        index--;
    }


//    printf("top neterminal: %d \n", (s->arr[index])->entry);

    return (s->arr[index])->entry;
//    value->expression = (s->arr[index])->expression;
}

void stackPop(tPrecStack *s) {


    if (!stackEmpty(s)){
//        free( (s->arr[s->top])->token );
//        free( (s->arr[s->top]) );
        s->top--; // decrement top position after removing top element
    }
}

void stackPush(tPrecStack *s, int entry, tToken token, entry_s *item) {
    if (stackFull(s)) {
        s = myRealloc(s, sizeof(tPrecStack)+ sizeof(tData*)*(s->top+STACK_INIT_SIZE));
    }

//    tData *data = malloc(sizeof(tData));
    tData *data = myMalloc(sizeof(tData));
    data->entry = entry;
    data->token = token;
    data->item = item;

    s->top++; // increment top position
    s->arr[s->top] = data;
}
