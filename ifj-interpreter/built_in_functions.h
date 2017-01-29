/* *************************** built_in_functions.h ************************* */
/* Soubor:              built_in_functions.h                                  */
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

#ifndef IFJ_INTERPRETER_BUILT_IN_FUNCTIONS_H
#define IFJ_INTERPRETER_BUILT_IN_FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "str.h"
#include "frame.h"

#define REALLOC_ERROR 1
//#define NULL_SYMBOL 2
//#define INVALID_VALUE 3
#define INPUT_ERR 7
#define BUILT_OK 4


int readInt();
double readDouble();
String readString();
int read_tmp(String*);
void print(const t_table_frame_item*);



#endif //IFJ_INTERPRETER_BUILT_IN_FUNCTIONS_H
