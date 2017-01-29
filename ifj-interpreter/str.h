/* ****************************** str.h ************************************* */
/* Soubor:              str.h                                                 */
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

#ifndef _String_H_
#define _String_H_

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


#define STR_LEN_INIT 8 //ukazuje kolik baytu potrebuje pocatecni alokace pameti, kdybychom nacitali
//retezec znak po znaku. Pamet se alokuje na nasobky tohoto cisla.

#define STR_OK 	0
#define STR_ERROR  1


typedef struct
{
    char* str;		// misto pro retezec ukonceny znakem '\0'
    int length;		// delka retezce
    int alloc_size;	//pocet b pameti
} String;

int init_value_str(String *str, const char *value_str);
String initEmptyString();   //inicializace prazdneho retezce
String initStringSize(int); //nastaveni parametru size pro retezec
String initString(char*); //inicializace neprazdneho retezce
void deleteString(String*); //uvolneni retezce z pameti
void GetSubstr(String *st, int i, int n);//vrati podretezec zadaneho retezce
int StringCmpString(String *st1, String *st2);// porovna oba retezce a vrati vysledek
int StringLength(String *st);// vrati delku daneho retezce
void printStr(String*); //zobrazuje obsah promenne dat. typu String na stdin


#endif