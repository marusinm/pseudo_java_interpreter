/* ****************************** str.c ************************************* */
/* Soubor:              str.c                                                 */
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


#include "str.h"
#include "garb.h"


int init_value_str(String *str, const char *value_str) {
    int size;
    if (value_str == NULL)
        return STR_ERROR;

    size = strlen(value_str);

//    str->str = (char *) malloc((size + 1) * sizeof(char));
    str->str = (char *) myMalloc((size + 1) * sizeof(char));
    if (str->str  == NULL)
        return STR_ERROR;

    strncpy(str->str, value_str, size);
    str->str[size] = '\0';
    str->length = size;
    str->alloc_size = size + 1;

    return STR_OK;
}


String initEmptyString()
{
    String strEmpty = {

            .length  = 0,
            .alloc_size = STR_LEN_INIT,
            .str    = malloc(STR_LEN_INIT * sizeof(char))};
    strEmpty.str[0] = '\0';

    return strEmpty;
}

String initStringSize(int size)
{
    String str ={.length  = 0,
            .alloc_size = size,
            .str    = malloc(size * sizeof(char))};
    str.str[0] = '\0';

    return str;
}

void deleteString(String *dst) {
    if(dst->alloc_size > 0)
    {
        free(dst->str);
        dst->str = NULL;
    }
    dst->alloc_size = 0;
    dst->length = 0;
}

String initString(char *src)
{

    int alloc_size = strlen(src);
    String str = {
            .length  = alloc_size,
            .alloc_size = alloc_size + 1,
            .str    = malloc((alloc_size + 1) * sizeof(char)) // +1 pro  '\0'
    };

    memcpy(str.str,src,alloc_size);
    str.str[alloc_size] = '\0';
    return str;
}

//funkce pro vypis promenne dat.typu String
void printStr(String *msg){
//    printf("OBSAH STRING : %s\n",msg->str);
}
/* napr. GetSubstr(&substring,0,4);*/
void GetSubstr(String *st, int start, int length){
        st->length = length + 1;
        memcpy(st->str, st->str + start, length);
        st->str[length] = '\0';
}


int StringCmpString(String *st1, String *st2){

    if (st1->length < st2->length)
        return -1; // prvni je mensi
    else if (st1->length > st2->length)
        return 1; // prvni je delsi
    else 
        return 0; // s1->length == s2->length
}


int StringLength(String *st){
    return st->length;
}