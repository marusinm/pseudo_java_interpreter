/* ******************************** common.h ******************************** */
/* Soubor:              common.h                                              */
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
#ifndef IFJ2016_COMMON_H
#define IFJ2016_COMMON_H

#include "ial.h"
#include "ilist.h"
#include "scanner.h"
#include "garb.h"


#define OK  (0)
#define LEXICAL_ERROR (1)
#define SYNTAX_ERROR (2)
#define SEMANTIC_ERROR (3)
#define SEMANTIC_ERROR_TYPE_COMPATIBILITY (4)
#define UNINITILIZE_ERRORS (8)
#define NOT_INITIALIZED_VARIABLE (8)
//#define ALLOC_ERROR 42
#define FILE_ERROR (99)
#define INTERNAL_ERROR (99)

typedef struct {
    tab_data_type data_type; // tTypeFunction | tTypeClass
    t_hashtable* classSymbolTable;
    t_hashtable* functionSymbolTable;
    t_hashtable* destinationSymbolTable;
    tListOfInstr * instList;
} tListOfSymbolsTables;


tListOfInstr *staticInstrList; // instruction list for static variables

t_hashtable *symbol_table;
tToken *globalToken;
tListOfSymbolsTables *ListOfSymbolsTables;


/**
 * struktura sluzi na docasne ulozenie parsovania klasifikovaneho pristupu
 */
typedef struct classifiedIdentifier{
    char * class_identifier;
    char * var_identifier;
    t_hashtable* classSymbolTable;
    bool isBuiltInFunction;
}tClassifiedIdentifier;

tClassifiedIdentifier * classifiedIdentifier;

void freeAndExit(int exit_code);




#endif //IFJ2016_COMMON_H