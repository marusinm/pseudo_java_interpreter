/* ******************************** garb.c ********************************** */
/* Soubor:              garb.c                                                */
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
#include <stdio.h>
#include "garb.h"


/**** ************* ****/
/**** FOR TEST ONLY ****/
/**** ************* ****/

// int main(){
//     //allocate collector. Return -1 if NullErr
//     int a = initGC(&garb_coll);
//     if(a != -1){
//         printf("-------------------------------------------------\n");
//         printf("----->Garbage Collector. Memory Allocated<-------\n");
//         printf("-------------------------------------------------\n");
//     }
//     //Memory alocate for some pointer in collector(can use other types like char,string and other)
//     int* p;

//     p = myMalloc(&garb_coll,sizeof(int));
//     printf("IN MAIN POINTER IS(AFTER) :%p\n",p);
//     //free memory
//     freeGC(&garb_coll);

//     return 0;
// }

/***END MAIN***/


/**** *************** ****/
/**** FUNCTIONS START ****/
/**** *************** ****/
extern t_garb_coll* garb_coll;

//inicializujeme GC
int initGC() {
    if ( (garb_coll = malloc(sizeof(t_garb_coll))) == NULL ) {
        return -1;
    }

    //nastavime pocatecni hodnoty
    garb_coll->first = NULL;
    garb_coll->last = NULL;
    garb_coll->length = 0;
    return 0;

}



//allocate memory in garbagecollector and return pointer in memory
//void* myMalloc(t_garb_coll** garb_coll, size_t size){
void* myMalloc(size_t size){

    void* ptr = malloc(size);

    if (ptr != NULL){

        t_garb_coll_rec *pom_garb_rec = malloc(sizeof(t_garb_coll_rec));// alokujeme garbagerecord

        if (pom_garb_rec == NULL) {
            free(ptr);
            return NULL;
        }

        pom_garb_rec->ptr = ptr;
        pom_garb_rec->next = NULL;

        garbageAppend(&garb_coll,pom_garb_rec);
        // printf("IN ALLOC FUNC POINTER IS :%p\n",ptr);//FOR TEST ONLY
        return ptr;

    } else return NULL;

}


void* myRealloc(void* ptr, size_t size) {

    void* new_ptr = realloc(ptr, size);//realokuje pamet

    t_garb_coll_rec* garb_rec = myFind(&garb_coll,ptr);//budeme hledat odpovidajici zaznam v garbeg collectoru

    if (new_ptr == NULL)
        return NULL;

    if(garb_rec == NULL)
        return NULL;

    garb_rec->ptr = new_ptr;// zmen adresu take v zaznamu garbage collectoru

    return new_ptr;//vrati novou adresu
}


void freeGC() {
    t_garb_coll_rec* rec = garb_coll->first;

    while (rec != NULL) {// projde cely linearni seznam

        //uvolnime data
        free(rec->ptr);
        rec->ptr = NULL;

        t_garb_coll_rec* tmpNext = rec->next;

        free(rec);//uvolnime structuru zaznamu

        rec = tmpNext;
    }

    //uvolni cely garbage collector
    free(garb_coll);

    // printf("MEMORY AFTER FREE: %p\n", rec );//FOR TEST ONLY
}


//HELPER FUNCTIONS for allocate memory 

void garbageAppend(t_garb_coll** garb_coll, t_garb_coll_rec *ptr){
    // projistotu vynuluj ukazatel na nasledujici polozku
    ptr->next = NULL;

    if ((*garb_coll)->first == NULL){//pokud je prvni zaznam v seznamu
        (*garb_coll)->first = ptr;
        (*garb_coll)->last = ptr;
    } else { // jinak pridej na konec
        (*garb_coll)->last->next = ptr;
        (*garb_coll)->last = ptr;
    }

}

t_garb_coll_rec* myFind(t_garb_coll** garb_coll, void *ptr) {
    t_garb_coll_rec* rec = (*garb_coll)->first;

    while (rec != NULL) {//projdeme cely seznam
        if(ptr != rec->ptr){
            rec = rec->next;
        }else return rec;
    }
    return NULL;
}


/**** FUNCTIONS END ****/