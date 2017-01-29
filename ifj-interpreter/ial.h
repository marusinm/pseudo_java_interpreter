/* ******************************** ial.h *********************************** */
/* Soubor:              ial.h                                                 */
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

#ifndef IAL_H
#define IAL_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "str.h"
#include "ilist.h"


void heap_sort(String* st);// samotna funkce heapsort
void sift_down(String* st, int left, int right);

int find(const String* , const String* );



/*  TABULKA SYMBOLU S ROZPTYLENYMI POLOZKAMI  */

enum tab_insert {
    TAB_INSERT_FOUND,
    TAB_INSERTED_OK,
    TAB_ERR_INSERT,
};

enum tab_search {
    TAB_SEARCH_OK,
    TAB_SEARCH_FAIL,
};

typedef union {
    int t_int;
    double t_double;
    String str;
}t_table_value;


typedef struct entry_s entry_t;//structura pro polozky tabulky
typedef struct hashtable_s t_hashtable;//structura pro tab.symb
typedef struct tab_data tab_t_data;

typedef enum{
    valueType_double = 0,   //0
    valueType_string = 1,   //1
    valueType_int = 2,      //2
    valueType_boolean = 3,  //3
    valueType_void = 4
}data_value_type;

typedef union {
    struct{
        data_value_type type;
        t_table_value value;
        bool is_init;
        bool is_declared;
    }variable;
    struct{
        //instruction * first_instr;
        bool has_def;
        tListOfInstr * instrList;
        t_hashtable * local_t;//lokalni tabulka
        entry_t * first_par;
        entry_t * first_var;
        t_table_value * ret_val;
        data_value_type return_type;
        int argcount;//pocet argumentu funkce
        int varcount;//pocet parametru funkce
    }funkce;
    struct{
        bool has_def;
        t_hashtable * class_table;
    }class;
    struct{
        data_value_type type;
        t_table_value value;
    }constant;
    struct {
        int type;
        int top_index;
        entry_t * next;
    } fun_item;
}table_data_value;

typedef enum{
    tTypeVariable, //0
    tTypeFunction,
    tTypeClass,
    tTypeConstant,
    tTypeFunItem,
}tab_data_type;

typedef struct tab_data{
    tab_data_type type;
    table_data_value value;
}tab_data;


typedef struct entry_s{
    char * key;
    tab_t_data * data;
    entry_t * next;
}entry_s;


struct hashtable_s{
    int size;
    entry_t * htable[];
}hashtable_s;



int htHash(t_hashtable * hashtable_s, char * key);//hashovaci funkce

t_hashtable * htInit(int size);//inicicalizace hashtab
int htInsert(t_hashtable * hashtable_s, char * key,  tab_t_data * data);//fce hleda polozku, a pokud ho najde vlozi synonym

int htLookup(t_hashtable * hashtable_s, char * key);//hleda polozku
entry_s* htSearch(t_hashtable * hashtable_s, char * key);
void htDelete(t_hashtable * hashtable_s);//fce smaze tabulku
void htDeleteitem(t_hashtable * hashtable_s, char * key);//fce zrusi polozky v tabulce

void table_print(t_hashtable * hashtable_s);


#endif
