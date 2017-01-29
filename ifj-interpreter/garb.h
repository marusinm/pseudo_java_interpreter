/* ******************************** garb.h ********************************** */
/* Soubor:              garb.h                                                */
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
#ifndef _GC_
#define _GC_

#include <stdlib.h>
#include <stdio.h>


typedef struct t_garb_coll_rec {
    void *ptr;
    struct t_garb_coll_rec *next;
}t_garb_coll_rec;


typedef struct  {
    t_garb_coll_rec *first;
    t_garb_coll_rec *last;
    int length;
} t_garb_coll;


/**POINTER TO GC*/
t_garb_coll* garb_coll;

int initGC();
void* myMalloc(size_t size);
void* myRealloc(void *ptr, size_t size);
void freeGC();
void garbageAppend(t_garb_coll** garb_coll, t_garb_coll_rec *ptr);
t_garb_coll_rec* myFind(t_garb_coll** garb_coll, void *ptr);

#endif