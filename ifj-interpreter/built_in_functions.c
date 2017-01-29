/* *************************** built_in_functions.c ************************* */
/* Soubor:              built_in_functions.c                                  */
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

#include <ctype.h>
#include "built_in_functions.h"
#include "str.h"

/*
    Pro zavolani funkci : int variable = readInt();
*/
int readInt(){
    long int detekce_cisla;
    char *end;
    String s;

    s.str = ( char* )malloc( sizeof( char ) * STR_LEN_INIT );

    if( s.str == NULL ){
        fprintf( stderr,"Input error - IOException occured; the result will be 0.\n" );
        deleteString(&s);
        exit(INPUT_ERR);
    }

    s.alloc_size = 8;
    s.length = 0;
    int readed = read_tmp(&s);

    if(readed == INPUT_ERR){
        fprintf(stderr,"Input error - null result from readLine; the result will be 0.\n");
        deleteString(&s);
        exit(INPUT_ERR);
    }
    else if(readed == REALLOC_ERROR){
        fprintf(stderr,"Input error - IOException occured; the result will be 0.\n");
        deleteString(&s);
        exit(INPUT_ERR);
    }
    else if(isalpha(readed)){
        exit(INPUT_ERR);
    }
    else{
        detekce_cisla = strtol( s.str, &end, 10 );
        if (( *end != '\0' ) || (errno == ERANGE))
        {
            fprintf(stderr,"Input error - Invalid value for an int.\n");
            deleteString(&s);
            exit(INPUT_ERR);
            //return 0;
        }
        else
        {
            deleteString(&s);
            return detekce_cisla;
        }
    }
}
/*
    Pro zavolani funkci : double variable = readDouble();
*/
double readDouble()
{
    double detekce_cisla;
    char *end;
    String s;

    s.str = ( char* )malloc( sizeof( char ) * STR_LEN_INIT );

    if( s.str == NULL ){
        fprintf( stderr,"Input error - IOException occured; the result will be 0.\n" );
        deleteString(&s);
        //return 0.0;
        exit(INPUT_ERR);
    }

    s.alloc_size = 8;
    s.length = 0;
    int readed = read_tmp(&s);

    if(readed == INPUT_ERR){
        fprintf(stderr,"Input error - null result from readLine; the result will be 0.\n");
        deleteString(&s);
        //return 0.0;
        exit(INPUT_ERR);
    }
    else if(readed == REALLOC_ERROR){
        fprintf(stderr,"Input error - IOException occured; the result will be 0.\n");
        deleteString(&s);
        //return 0.0;
        exit(INPUT_ERR);
    }
    else{
        detekce_cisla = strtod( s.str, &end);
        if (( *end != '\0' ) || (errno == ERANGE))
        {
            fprintf(stderr,"Input error - Invalid value for a double.\n");
            deleteString(&s);
            //return 0.0;
            exit(INPUT_ERR);
        }
        else
        {
            free(s.str);
            return detekce_cisla;
        }
    }
}

/*
    Pro zavolani funkci : String variable = readString();
*/
String readString(){

    int i = 0;
    char curr_char = 48;
    String s;
    s.str = ( char* )malloc( sizeof( char ) * STR_LEN_INIT );

    if( s.str == NULL ){
        fprintf( stderr,"Input error - IOException occured; the result will be empty string\n" );
        s.str = "";
        return s;
    }

    s.alloc_size = 8;
    s.length = 0;

    while ( curr_char != EOF && curr_char !='\n' ){
        curr_char = getchar();
        if (( curr_char !='\n' )  && ( curr_char != EOF ))
        {
            s.str[i] = curr_char;
            if(( (unsigned) s.str[i] < 1 ) ||( (unsigned) s.str[i] > 255 ))
            {
                fprintf(stderr,"Input error - null result from readLine; the result will be empty string.\n");
                s.str = "";
                //return s;
                exit(INPUT_ERR);
            }

            else
            {
                i++;

                if (s.length + 1 > s.alloc_size)
                {
                    if ((s.str = (char*) realloc(s.str, *s.str + s.alloc_size * sizeof(char))) == NULL){
                        fprintf(stderr,"Input error - IOException occured; the result will be the empty string.\n");
                        s.str = "";
                        return s;
                    }
                    s.alloc_size = s.length + STR_LEN_INIT;
                }
                s.length++;
            }
        }
        else
        {
            s.str[i] = 0;
            return s;
        }
    }
    s.str = "";
    return s;
}

int read_tmp(String* s){
    char curr_char = 48;
    int i = 0;

    while ( curr_char != EOF && curr_char !='\n' ){
        curr_char = getchar();
        if (( curr_char !='\n' )  && ( curr_char != EOF ))
        {
            s->str[i] = curr_char;
            if(( (unsigned) s->str[i] < 1 ) ||( (unsigned) s->str[i] > 255 ))
            {
                exit(INPUT_ERR);
                ///return INPUT_ERR;
            }
            else
            {
                i++;

                if (s->length + 1 > s->alloc_size)
                {
                    if ((s->str = (char*) realloc(s->str, *s->str + s->alloc_size * sizeof(char))) == NULL){
                        return REALLOC_ERROR;
                    }
                    s->alloc_size = s->length + STR_LEN_INIT;
                }
                s->length++;
            }
        }

        else
        {
            s->str[i] = 0;
            if(s->length == 0){
                exit(INPUT_ERR);
                //return INPUT_ERR;
            }
            else{
                return BUILT_OK;
            }
        }
    }
    return BUILT_OK;
}

/* TODO : konkatenace */
void print(const t_table_frame_item* item){
    if(item->type == 0){
        printf("%g\n",item->value.t_double);
    }
    else if(item->type == 1){
        printf("%d\n",item->value.t_int);
    }
    else if(item->type == 2){
        printf("%s\n",item->value.str);
    }
    /*else{
        if(item->value.t_bool){
            printf("true\n");
        }else{
            printf("false\n");
        }
    }*/
}