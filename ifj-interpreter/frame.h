/* ******************************** frame.h  ******************************** */
/* Soubor:              frame.h                                               */
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

#ifndef IFJ_INTERPRETER_FRAME_H
#define IFJ_INTERPRETER_FRAME_H

#include <stdio.h>
//#include <jmorecfg.h>
#include "stdbool.h"
//#include "str.h"

#include "ilist.h"
#include "ial.h"
//#include "interpret.h"


typedef struct t_frame_item t_table_frame_item;
typedef struct t_frame t_table_frame;

typedef struct t_frame_item {
    char * key;
    t_table_value value;
    data_value_type type;
    bool is_init;
    t_table_frame_item *next;
};

typedef struct t_frame {
    int size;
    instructions *nextIntruction;
    t_table_frame_item *items[];
};

void createFrame();

//void generateFrameConstant(t_table_frame* hashtable, char * key);

/*void mapToFrameTable(t_hashtable *hashtable_s){

//t_table_frame_item* newMapTableItem;

}*/


int frameHash(t_table_frame * t_frame, char * key);//hashovaci funkce
t_table_frame * frameInit(int size);//inicicalizace hashtab
int frameInsert(t_table_frame * t_frame, char * key,  t_table_frame_item *data);// last argument ????? fce hleda polozku, a pokud ho najde vlozi synonym
int frameLookup(t_table_frame * t_frame, char * key);//hleda polozku
t_table_frame_item* frameSearch(t_table_frame * t_frame, char * key);

/*
void htDelete(t_hashtable * hashtable_s);//fce smaze tabulku
void htDeleteitem(t_hashtable * hashtable_s, char * key);//fce zrusi polozky v tabulce

void table_print(t_hashtable * hashtable_s);
*/

#define MAX_STACK 20

//extern int STACK_SIZE;

#define SERR_INIT   70                                  /* chyba při stackInit */
#define SERR_PUSH   69                                  /* chyba při stackPush */
#define SERR_TOP    71                                   /* chyba při stackTop */
#define STACK_ERR   72

typedef struct {
    /* pole pro uložení hodnot */
    //char arr[MAX_STACK];
    int top;                                /* index prvku na vrcholu zásobníku */
    t_table_frame *arr[];

} tStack;


void frameStackError ( int error_code );
void frameStackInit ( tStack* s );
int frameStackEmpty ( const tStack* s );
int frameStackFull ( const tStack* s );
//void frameStackTop (const tStack* s, char* c );
void frameStackPop ( tStack* s );
void frameStackPush ( tStack* s, t_table_frame *newFrameTable );


#endif //IFJ_INTERPRETER_FRAME_H
