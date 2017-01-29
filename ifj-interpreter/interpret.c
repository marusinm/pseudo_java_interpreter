/* ***************************** interpret.c ******************************** */
/* Soubor:              interpret.c                                           */
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

#include "interpret.h"
#include "common.h"
#include "ial.h"
#include "ilist.h"
#include "str.h"

//#define _DEBUG_INTERPRET
/*
 * type_of_instr
 * 0 - static
 * 1 - main
 * 2 - other functions
 */
void justInterpretIt(tListOfInstr *helpInstrList, int type_of_instr_list, t_hashtable *local_symbol_table) {

    tListOfInstr *instructionList;
    instructionList = helpInstrList;

//    printf("prva instrukcia je -> %d \n", instructionList->first->Instruction.instType);

    tListOfInstr *tmp;
    tListOfInstr *instrListForRecursion;

    //helpers for built_in_functions
    int result_int;
    double result_double;
    String result_string;

    // just for static values
    entry_s *destination = NULL;
    entry_s *op1 = NULL;
    entry_s *op2 = NULL;
    t_table_frame_item *dest_local = NULL;
    t_table_frame_item *op1_local = NULL;
    t_table_frame_item *frameItem = NULL;
    tListItem *jump = NULL;

    int op1_int_value = 0;
    int op2_int_value = 0;
    double op1_double_value = 0;
    double op2_double_value = 0;
    int op1_type = -1;
    int op2_type = -1;
    char *op1_string_value = "";
    char *op2_string_value = "";
    int comparison_result; // 0/1
    double comparison_left_operand;
    double comparison_right_operand;
    unsigned int alloc_size;

    // for built_in func operand
    String str_1;
    String str_2;
    String str_tmp;

#ifdef _DEBUG_INTERPRET
    // jedna sa o staticku pasku
    if(type_of_instr_list == 0)
        printf("\n\nInterpret static list = %d \n\n", type_of_instr_list);
    else if(type_of_instr_list == 1) // main paska
        printf("\n\nInterpret run list = %d \n\n", type_of_instr_list);
    else // paska pre inu funkciu
        printf("\n\nInterpret other func list = %d \n\n", type_of_instr_list);
#endif

    listFirst(instructionList);
//    print_elements_of_list(*instructionList);


    //printf("wtf value = %d \n", ((entry_s*)(instructionList->first->Instruction.addr3))->data->value.constant.value.t_int );
    if (instructionList->active == NULL){
#ifdef _DEBUG_INTERPRET
//                printf("prva instrukcia neexistuje, prosim nacitaj ju\n");
#endif
        return;
    }

    t_table_frame *frame;
    if (type_of_instr_list != 0){
        frame = mapToFrameTable(local_symbol_table, local_symbol_table->size);
    }


    int i = 0;
    do {

        i++;

        /*
         * Interpret pre funkcie
         */
        if (type_of_instr_list == 1 || type_of_instr_list == 2){

//            printf("Funkcia musime ju premapovat\n");

            /*
             * Destination
             */
            if (instructionList->active->Instruction.addr1 != NULL) {

//                customPrintInstType(instructionList->active->Instruction.instType);
//                printf("\n");
                if (instructionList->active->Instruction.instType == I_JUMP ||
                    instructionList->active->Instruction.instType == I_JUMP_IF_FALSE){
//                    printf("so far so good\n");
                } else {

                    destination = (entry_s *) instructionList->active->Instruction.addr1;
                    if (isKeyInFt(destination->key, frame) == 1){
                        dest_local = frameSearch(frame, destination->key);//naslo, pouzivame frame item
                    }

                }


            }
            /*
             * Operand 1
             */
            if (instructionList->active->Instruction.addr2 != NULL) {
                op1 = (entry_s *) instructionList->active->Instruction.addr2;

                if (isKeyInFt(op1->key, frame) == 1){
                    //naslo, pouzivame frame item
                    op1_local = frameSearch(frame, op1->key);

                        /*
                         * Zober hodnoty z frame-u
                         */
                        op1_type = op1_local->type;
                        if (op1_type == valueType_int)
                            op1_int_value = op1_local->value.t_int;
                        else if (op1_type == valueType_boolean)
                            op1_int_value = op1_local->value.t_int;
                        else if (op1_type == valueType_string)
                            op1_string_value = op1_local->value.str.str;
                        else if (op1_type == valueType_double)
                            op1_double_value = op1_local->value.t_double;

                } else {
                    /*
                     * Zober hodnoty z tabulky symbolov
                     */
                    if (instructionList->active->Instruction.instType == I_SET_PARAM){

                    } else {

                        if (op1->data->type == tTypeConstant) {

                            if (op1->data->value.constant.type == valueType_int) {
                                op1_type = valueType_int;
                                op1_int_value = op1->data->value.constant.value.t_int;
                            }
                            else if (op1->data->value.constant.type == valueType_double) {
                                op1_type = valueType_double;
                                op1_double_value = op1->data->value.constant.value.t_double;
                            }
                            else if (op1->data->value.constant.type == valueType_string) {
                                op1_type = valueType_string;
                                op1_string_value = op1->data->value.constant.value.str.str;
                            }
                            else if (op1->data->value.constant.type == valueType_boolean) {
                                op1_type = valueType_boolean;
                                op1_int_value = op1->data->value.constant.value.t_int;
                            }

                        } else if (op1->data->type == tTypeVariable) {

                            if (op1->data->value.variable.type == valueType_int) {
                                op1_type = valueType_int;
                                op1_int_value = op1->data->value.variable.value.t_int;
                            }
                            else if (op1->data->value.variable.type == valueType_double) {
                                op1_type = valueType_double;
                                op1_double_value = op1->data->value.variable.value.t_double;
                            }
                            else if (op1->data->value.variable.type == valueType_string) {
                                op1_type = valueType_string;
                                op1_string_value = op1->data->value.variable.value.str.str;
                            }
                            else if (op1->data->value.variable.type == valueType_boolean) {
                                op1_type = valueType_boolean;
                                op1_int_value = op1->data->value.variable.value.t_int;
                            }

                        }

                    }
                }
            }

                /*
                 * Operand 2
                 */
                if (instructionList->active->Instruction.addr3 != NULL) {

//                printf("inst type:");
//                customPrintInstType(instructionList->active->Instruction.instType);
//                printf("\n");

                /*
                 * Instruction I_CALL_FUNC has NOT symbol item on address 3 !
                 */
                if (instructionList->active->Instruction.instType == I_CALL_FUNC){
//                    printf("madafaka\n");
//                    tListOfInstr *tmpmartin = instructionList->active->Instruction.addr3;
//                    print_elements_of_list(*tmpmartin);

                } else {

                    op2 = (entry_s *) instructionList->active->Instruction.addr3;

                    if (isKeyInFt(op2->key, frame) == 1){
                        //naslo, pouzivame frame item
                        t_table_frame_item *op2_local;
                        op2_local = frameSearch(frame, op2->key);

                        /*
                         * Zober hodnoty z frame-u
                         */
                        op2_type = op2_local->type;
                            if (op2_type == valueType_int)
                                op2_int_value = op2_local->value.t_int;
                            else if (op2_type == valueType_boolean)
                                op2_int_value = op2_local->value.t_int;
                            else if (op2_type == valueType_string)
                                op2_string_value = op2_local->value.str.str;
                            else if (op2_type == valueType_double)
                                op2_double_value = op2_local->value.t_double;

                    } else {
                        /*
                         * Zober hodnoty z tabulky symbolov
                         */
                        if (op2->data->type == tTypeConstant) {

                            if (op2->data->value.constant.type == valueType_int) {
                                op2_type = valueType_int;
                                op2_int_value = op2->data->value.constant.value.t_int;
                            }
                            else if (op2->data->value.constant.type == valueType_double) {
                                op2_type = valueType_double;
                                op2_double_value = op2->data->value.constant.value.t_double;
                            }
                            else if (op2->data->value.constant.type == valueType_string) {
                                op2_type = valueType_string;
                                op2_string_value = op2->data->value.constant.value.str.str;
                            }
                            else if (op2->data->value.constant.type == valueType_boolean) {
                                op2_type = valueType_boolean;
                                op2_int_value = op2->data->value.constant.value.t_int;
                            }
                        }
                    }
                } // if callfunc || jump
            }
        }


        /*
         * Interpret pre staticke premenne
         */
        if (type_of_instr_list == 0) {

            destination = (entry_s *) instructionList->active->Instruction.addr1;
            op1 = (entry_s *) instructionList->active->Instruction.addr2;
            op2 = (entry_s *) instructionList->active->Instruction.addr3;
//            printf("^-^ staticke premenne\n");

            if (op1 != NULL) {
                if (op1->data->type == tTypeConstant) {
                    if (op1->data->value.constant.type == valueType_int) {
                        op1_type = valueType_int;
                        op1_int_value = op1->data->value.constant.value.t_int;
                    }
                    else if (op1->data->value.constant.type == valueType_double) {
                        op1_type = valueType_double;
                        op1_double_value = op1->data->value.constant.value.t_double;
                    }
                    else if (op1->data->value.constant.type == valueType_string) {
                        op1_type = valueType_string;
                        op1_string_value = op1->data->value.constant.value.str.str;
                    }
                    else if (op1->data->value.constant.type == valueType_boolean) {
                        op1_type = valueType_boolean;
                        op1_int_value = op1->data->value.constant.value.t_int;
                    }
                }
            }

            if (op2 != NULL) {
                if (op2->data->type == tTypeConstant) {

                    if (op2->data->value.constant.type == valueType_int) {
                        op2_type = valueType_int;
                        op2_int_value = op2->data->value.constant.value.t_int;
                    }
                    else if (op2->data->value.constant.type == valueType_double) {
                        op2_type = valueType_double;
                        op2_double_value = op2->data->value.constant.value.t_double;
                    }
                    else if (op2->data->value.constant.type == valueType_string) {
                        op2_type = valueType_string;
                        op2_string_value = op2->data->value.constant.value.str.str;
                    }
                    else if (op2->data->value.constant.type == valueType_boolean) {
                        op2_type = valueType_boolean;
                        op2_int_value = op2->data->value.constant.value.t_int;
                    }
                }
            }
        }



        switch(instructionList->active->Instruction.instType) {

            case I_ADD:
#ifdef _DEBUG_INTERPRET

                printf("instrukcia ADD\n");

/*                printf(" dest = %d \n", interp_dest->data->value.constant.value.t_int);
                printf(" o1 = %d \n", interp_op1->data->value.constant.value.t_int);
                printf(" o2 = %d \n", interp_op2->data->value.constant.value.t_int);*/
#endif

#ifdef _DEBUG_INTERPRET
//                printf("scital som 2 cisla\n");
#endif

                if (dest_local != NULL){ //destination in frame

                    if (dest_local->type == valueType_int){
                        dest_local->value.t_int = op1_int_value + op2_int_value;
#ifdef _DEBUG_INTERPRET
                        printf("ADD result = %d \n", dest_local->value.t_int);
#endif
                    }
                    else if (dest_local->type == valueType_double){
                        dest_local->value.t_double = 0.0;

                        if (op1_type == valueType_int)
                            dest_local->value.t_double += op1_int_value;
                        else if (op1_type == valueType_double)
                            dest_local->value.t_double += op1_double_value;

                        if (op2_type == valueType_int)
                            dest_local->value.t_double += op2_int_value;
                        else if (op2_type == valueType_double)
                            dest_local->value.t_double += op2_double_value;


//                        if (op1_type == valueType_int && op2_type == valueType_double)
//                            dest_local->value.t_double = op1_int_value + op2_double_value;

#ifdef _DEBUG_INTERPRET
                        printf("%d \n", op1_int_value);
                        printf("%g \n", op2_double_value);
                        printf("ADD1 result = %g \n", dest_local->value.t_double);
#endif
                    }

                } else { //destination in symbol table

                    if(destination->data->type == tTypeFunction ||
                       destination->data->type == tTypeVariable ||
                       destination->data->type == tTypeClass){
#ifdef _DEBUG_INTERPRET
                        printf("destination invalid type \n");
#endif
                        freeAndExit(INTERNAL_ERROR);
                    }
                    if(op1->data->type == tTypeClass        ||
                       op1->data->type == tTypeFunction    ||
                       op2->data->type == tTypeClass       ||
                       op2->data->type == tTypeFunction    ){
#ifdef _DEBUG_INTERPRET
                        printf("op1 or op2 invalid type \n");
#endif
                        freeAndExit(INTERNAL_ERROR);
                    }

                    //destination je vzdy konstanta
                    if(destination->data->value.constant.type == valueType_int){
                        destination->data->value.constant.value.t_int = op1_int_value + op2_int_value;
#ifdef _DEBUG_INTERPRET
                        printf("ADD result = %d \n", destination->data->value.constant.value.t_int);
#endif
                    }
                        // scitavat mozeme: int + double, double + int, double + double
                    else if (destination->data->value.constant.type == valueType_double){
                        destination->data->value.constant.value.t_double = 0;
                        if (op1_type == valueType_int)
                            destination->data->value.constant.value.t_double += op1_int_value;
                        else if (op1_type == valueType_double)
                            destination->data->value.constant.value.t_double += op1_double_value;

                        if (op2_type == valueType_int)
                            destination->data->value.constant.value.t_double += op2_int_value;
                        else if (op2_type == valueType_double)
                            destination->data->value.constant.value.t_double += op2_double_value;
#ifdef _DEBUG_INTERPRET
                        printf("ADD result = %f \n", destination->data->value.constant.value.t_double);
#endif
                    }

                }

                listNext(instructionList);

                break;
            case I_CONCATENATE:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia CONCATENATE\n");
                printf("------------------------------------\n");
#endif

                /*
                 * Operand 1
                 */
                if (op1_type == valueType_int){
                    //konvertuje int na string
                    char tmpstring[100];
                    sprintf(tmpstring, "%d", op1_int_value);
                    //skopiruje tmpstring do destination
                    alloc_size = strlen(tmpstring) + 1;
                    if (dest_local != NULL){
                        dest_local->value.str.str = myMalloc(alloc_size);
                        strcpy(dest_local->value.str.str , tmpstring);
                    } else {
                        destination->data->value.constant.value.str.str = myMalloc(alloc_size);
                        strcpy(destination->data->value.constant.value.str.str, tmpstring);
                    }

                }
                else if (op1_type == valueType_double){
                    //konvertuje double na string
                    char tmpstring[100];
                    sprintf(tmpstring, "%f", op1_double_value);
                    //skopiruje tmpstring do destination
                    alloc_size = strlen(tmpstring) + 1;
                    if (dest_local != NULL){
                        dest_local->value.str.str = myMalloc(alloc_size);
                        strcpy(dest_local->value.str.str , tmpstring);
                    } else {
                        destination->data->value.constant.value.str.str = myMalloc(alloc_size);
                        strcpy(destination->data->value.constant.value.str.str, tmpstring);
                    }
                }
                else if (op1_type == valueType_string){
                    //alokovat mozeme priamo velkost stringu
                    alloc_size = strlen(op1_string_value) + 1;
                    if (dest_local != NULL){
                        dest_local->value.str.str = myMalloc(alloc_size);
                        strcpy(dest_local->value.str.str , op1_string_value);
                    } else {
                        destination->data->value.constant.value.str.str = myMalloc(alloc_size);
                        strcpy(destination->data->value.constant.value.str.str, op1_string_value);
                    }
                }else{
#ifdef _DEBUG_INTERPRET
                    printf("neco je zle s prvym operandom konkatenace\n");
#endif
                    freeAndExit(INTERNAL_ERROR); //todo overit ci nemozu nastat ine situacie
                }

                /*
                 * Operand 2
                 */
                if (op2_type == valueType_int){
                    //konvertuje int na string
                    char tmpstring[100];
                    sprintf(tmpstring, "%d", op2_int_value);
                    //skopiruje tmpstring do destination
                    alloc_size += strlen(tmpstring) + 1;
                    if (dest_local != NULL){
                        dest_local->value.str.str = (char *) myRealloc(dest_local->value.str.str, alloc_size);
                        strcat(dest_local->value.str.str , tmpstring);
                    } else {
                        destination->data->value.constant.value.str.str = (char *) myRealloc(
                                destination->data->value.constant.value.str.str, alloc_size);
                        strcat(destination->data->value.constant.value.str.str, tmpstring);
                    }
                }
                else if (op2_type == valueType_double){
                    //konvertuje double na string
                    char tmpstring[100];
                    sprintf(tmpstring, "%f", op2_double_value);
                    //skopiruje tmpstring do destination
                    alloc_size += strlen(tmpstring) + 1;
                    if (dest_local != NULL){
                        dest_local->value.str.str = (char *) myRealloc(dest_local->value.str.str, alloc_size);
                        strcat(dest_local->value.str.str , tmpstring);
                    } else {
                        destination->data->value.constant.value.str.str = (char *) myRealloc(
                                destination->data->value.constant.value.str.str, alloc_size);
                        strcat(destination->data->value.constant.value.str.str, tmpstring);
                    }
                }
                else if (op2_type == valueType_string){
                    //alokovat mozeme priamo velkost stringu
                    alloc_size += strlen(op2_string_value)+ 1;
                    if (dest_local != NULL){
                        dest_local->value.str.str = (char *) myRealloc(dest_local->value.str.str, alloc_size);
                        strcat(dest_local->value.str.str , op2_string_value);
                    } else {
                        destination->data->value.constant.value.str.str = (char *) myRealloc(
                                destination->data->value.constant.value.str.str, alloc_size);
                        strcat(destination->data->value.constant.value.str.str, op2_string_value);
                    }
                }else{
#ifdef _DEBUG_INTERPRET
                    printf("neco je zle s druhym operandom konkatenace\n");
#endif
                    freeAndExit(INTERNAL_ERROR); //todo overit ci nemozu nastat ine situacie
                }
#ifdef _DEBUG_INTERPRET
                printf("concatenate result = %s \n", destination->data->value.constant.value.str.str);
#endif
                listNext(instructionList);
                break;

            case I_SUB:
#ifdef _DEBUG_INTERPRET
                printf("\n");
                printf("------------------------------------\n");
                printf("instrukcia SUB\n");
                printf("------------------------------------\n");
#endif

                if (dest_local != NULL) { //destination in frame

                    if (dest_local->type == valueType_int) {
                        dest_local->value.t_int = op1_int_value - op2_int_value;
#ifdef _DEBUG_INTERPRET
                        printf("SUB result = %d \n", dest_local->value.t_int);
#endif
                    } else if (dest_local->type == valueType_double) {

                        dest_local->value.t_double = 0;
                        if (op1_type == valueType_int)
                            dest_local->value.t_double += op1_int_value;
                        else if (op1_type == valueType_double)
                            dest_local->value.t_double += op1_double_value;

                        if (op2_type == valueType_int)
                            dest_local->value.t_double -= op2_int_value;
                        else if (op2_type == valueType_double)
                            dest_local->value.t_double -= op2_double_value;

#ifdef _DEBUG_INTERPRET
                        printf("SUB result = %f \n", dest_local->value.t_double);
#endif
                    }
                } else {

                    if(destination->data->type == tTypeFunction ||
                       destination->data->type == tTypeVariable ||
                       destination->data->type == tTypeClass){
#ifdef _DEBUG_INTERPRET
                        printf("destination invalid type \n");
#endif
                        freeAndExit(INTERNAL_ERROR);
                    }
                    if(op1->data->type == tTypeClass       ||
                       op1->data->type == tTypeFunction    ||
                       op2->data->type == tTypeClass       ||
                       op2->data->type == tTypeFunction    ){
#ifdef _DEBUG_INTERPRET
                        printf("op1 or op2 invalid type \n");
#endif
                        freeAndExit(INTERNAL_ERROR);
                    }

                    //destination je vzdy konstanta
                    if(destination->data->value.constant.type == valueType_int){
                        // int - int
                        destination->data->value.constant.value.t_int = op1_int_value - op2_int_value;
#ifdef _DEBUG_INTERPRET
                        printf("SUB result = %d \n", destination->data->value.constant.value.t_int);
#endif
                    }
                        // odcitat mozeme: int - double, double - int, double - double
                    else if (destination->data->value.constant.type == valueType_double){
                        destination->data->value.constant.value.t_double = 0;
                        if (op1_type == valueType_int)
                            destination->data->value.constant.value.t_double += op1_int_value;
                        else if (op1_type == valueType_double)
                            destination->data->value.constant.value.t_double += op1_double_value;

                        if (op2_type == valueType_int)
                            destination->data->value.constant.value.t_double -= op2_int_value;
                        else if (op2_type == valueType_double)
                            destination->data->value.constant.value.t_double -= op2_double_value;

#ifdef _DEBUG_INTERPRET
                        printf("SUB result = %f \n", destination->data->value.constant.value.t_double);
#endif
                    }

                }

                listNext(instructionList);

                break;

            case I_MUL:

  #ifdef _DEBUG_INTERPRET
                printf("\n");
                printf("------------------------------------\n");
                printf("instrukcia MUL\n");
                printf("------------------------------------\n");
  #endif

                if (dest_local != NULL) { //destination in frame

                    if (dest_local->type == valueType_int) {
                        dest_local->value.t_int = op1_int_value * op2_int_value;
#ifdef _DEBUG_INTERPRET
                        printf("MUL result = %d \n", dest_local->value.t_int);
#endif
                    } else if (dest_local->type == valueType_double) {
                        dest_local->value.t_double = 0;
                        if (op1_type == valueType_int)
                            dest_local->value.t_double += op1_int_value;
                        else if (op1_type == valueType_double)
                            dest_local->value.t_double += op1_double_value;

                        if (op2_type == valueType_int)
                            dest_local->value.t_double *= op2_int_value;
                        else if (op2_type == valueType_double)
                            dest_local->value.t_double *= op2_double_value;

#ifdef _DEBUG_INTERPRET
                        printf("MUL result = %f \n", dest_local->value.t_double);
#endif
                    }
                }

                if(destination->data->type == tTypeFunction ||
                   destination->data->type == tTypeVariable ||
                   destination->data->type == tTypeClass){
#ifdef _DEBUG_INTERPRET
                    printf("destination invalid type \n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }
              if(op1->data->type == tTypeClass        ||
                  op1->data->type == tTypeFunction    ||
                  op2->data->type == tTypeClass       ||
                  op2->data->type == tTypeFunction    ){
#ifdef _DEBUG_INTERPRET
                  printf("op1 or op2 invalid type \n");
#endif
                  freeAndExit(INTERNAL_ERROR);
              }

              //destination je vzdy konstanta
              if(destination->data->value.constant.type == valueType_int){
                // int * int
                  destination->data->value.constant.value.t_int = op1_int_value * op2_int_value;
#ifdef _DEBUG_INTERPRET
                  printf("MUL result = %d \n", destination->data->value.constant.value.t_int);
#endif
              }
                  // double * double
              else if (destination->data->value.constant.type == valueType_double){
                  destination->data->value.constant.value.t_double = 0;
                  if (op1_type == valueType_int)
                      destination->data->value.constant.value.t_double += op1_int_value;
                  else if (op1_type == valueType_double)
                      destination->data->value.constant.value.t_double += op1_double_value;

                  if (op2_type == valueType_int)
                      destination->data->value.constant.value.t_double *= op2_int_value;
                  else if (op2_type == valueType_double)
                      destination->data->value.constant.value.t_double *= op2_double_value;
#ifdef _DEBUG_INTERPRET
                  printf("MUL result = %f \n", destination->data->value.constant.value.t_double);
#endif
              }

              listNext(instructionList);
              break;

            case I_DIV:

    #ifdef _DEBUG_INTERPRET
                printf("\n");
                printf("------------------------------------\n");
                printf("instrukcia DIV\n");
                printf("------------------------------------\n");
    #endif

                if (dest_local != NULL){ //destination in frame

                    if (dest_local->type == valueType_int){
                        dest_local->value.t_int = op1_int_value / op2_int_value;
#ifdef _DEBUG_INTERPRET
                        printf("DIV result = %d \n", dest_local->value.t_int);
#endif
                    }
                    else if (dest_local->type == valueType_double){
                        dest_local->value.t_double = 0;
                        if (op1_type == valueType_int)
                            dest_local->value.t_double += op1_int_value;
                        else if (op1_type == valueType_double)
                            dest_local->value.t_double += op1_double_value;

                        if (op2_type == valueType_int)
                            dest_local->value.t_double /= op2_int_value;
                        else if (op2_type == valueType_double)
                            dest_local->value.t_double /= op2_double_value;

#ifdef _DEBUG_INTERPRET
                        printf("DIV result = %f \n", dest_local->value.t_double);
#endif
                    }

                } else {

                    if(destination->data->type == tTypeFunction ||
                       destination->data->type == tTypeVariable ||
                       destination->data->type == tTypeClass){
#ifdef _DEBUG_INTERPRET
                        printf("destination invalid type \n");
#endif
                        freeAndExit(INTERNAL_ERROR);
                    }
                    if(op1->data->type == tTypeClass        ||
                       op1->data->type == tTypeFunction    ||
                       op2->data->type == tTypeClass       ||
                       op2->data->type == tTypeFunction    ){
#ifdef _DEBUG_INTERPRET
                        printf("op1 or op2 invalid type \n");
#endif
                        freeAndExit(INTERNAL_ERROR);
                    }

                    //destination je vzdy konstanta
                    if(destination->data->value.constant.type == valueType_int){
                        if(op2->data->value.constant.value.t_int == 0){
                            freeAndExit(DIVIDE_ZERO);
                        }
                        // int / int
                        destination->data->value.constant.value.t_int = op1_int_value / op2_int_value;
#ifdef _DEBUG_INTERPRET
                        printf("DIV result = %d \n", destination->data->value.constant.value.t_int);
#endif
                    }
                        // double / double
                    else if (destination->data->value.constant.type == valueType_double){
//                    if(op2->data->value.constant.value.t_double == 0){
//                        freeAndExit(DIVIDE_ZERO);
//                    }
                        destination->data->value.constant.value.t_double = 0;
                        if (op1_type == valueType_int)
                            destination->data->value.constant.value.t_double += op1_int_value;
                        else if (op1_type == valueType_double)
                            destination->data->value.constant.value.t_double += op1_double_value;

                        if (op2_type == valueType_int)
                            destination->data->value.constant.value.t_double /= op2_int_value;
                        else if (op2_type == valueType_double)
                            destination->data->value.constant.value.t_double /= op2_double_value;
#ifdef _DEBUG_INTERPRET
                        printf("DIV result = %f \n", destination->data->value.constant.value.t_double);
#endif
                    }
                }


                listNext(instructionList);

                break;

            case I_LESS:


                if (op1_type == valueType_int)
                    comparison_left_operand=  op1_int_value;
                else if (op1_type == valueType_double)
                    comparison_left_operand =  op1_double_value;
                else {
#ifdef _DEBUG_INTERPRET
                    printf("LESS something is wrong with op1\n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }

                if (op2_type == valueType_int)
                    comparison_right_operand =  op2_int_value;
                else if (op2_type == valueType_double)
                    comparison_right_operand =  op2_double_value;
                else{
#ifdef _DEBUG_INTERPRET
                    printf("LESS something is wrong with op2\n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }

                /*
                 * LESS
                 */
                if (comparison_left_operand < comparison_right_operand)
                    comparison_result = 1;
                else
                    comparison_result = 0;

                /*
                 * Store result
                 */
                if (dest_local != NULL) { //destination in frame
                    dest_local->value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("LESS result = ");
                    if (dest_local->value.t_int == 1)
                        printf("true \n");
                    else if (dest_local->value.t_int == 0)
                        printf("false \n");
#endif

                } else {
                    destination->data->value.constant.value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("LESS result = ");
                    if (destination->data->value.constant.value.t_int == 1)
                        printf("true \n");
                    else if (destination->data->value.constant.value.t_int == 0)
                        printf("false \n");
#endif
                }

                listNext(instructionList);
                break;

            case I_MORE:


                if (op1_type == valueType_int)
                    comparison_left_operand =  op1_int_value;
                else if (op1_type == valueType_double)
                    comparison_left_operand =  op1_double_value;
                else{
#ifdef _DEBUG_INTERPRET
                    printf("MORE something is wrong with op1\n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }


                if (op2_type == valueType_int)
                    comparison_right_operand =  op2_int_value;
                else if (op2_type == valueType_double)
                    comparison_right_operand =  op2_double_value;
                else{
#ifdef _DEBUG_INTERPRET
                    printf("MORE something is wrong with op2\n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }

                /*
                 * MORE
                 */
                if (comparison_left_operand > comparison_right_operand)
                    comparison_result = 1;
                else
                    comparison_result = 0;

                /*
                 * Store result
                 */
                if (dest_local != NULL) { //destination in frame
                    dest_local->value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("MORE result = ");
                    if (dest_local->value.t_int == 1)
                        printf("true \n");
                    else if (dest_local->value.t_int == 0)
                        printf("false \n");
#endif

                } else {

                    destination->data->value.constant.value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("MORE result = ");
                    if (destination->data->value.constant.value.t_int == 1)
                        printf("true \n");
                    else if (destination->data->value.constant.value.t_int == 0)
                        printf("false \n");
#endif

                }

                listNext(instructionList);
                break;
            case I_LESS_EQUAL:


                if (op1_type == valueType_int)
                    comparison_left_operand =  op1_int_value;
                else if (op1_type == valueType_double)
                    comparison_left_operand =  op1_double_value;
                else {
#ifdef _DEBUG_INTERPRET
                    printf("LESS_EQUAL something is wrong with op1\n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }


                if (op2_type == valueType_int)
                    comparison_right_operand =  op2_int_value;
                else if (op2_type == valueType_double)
                    comparison_right_operand =  op2_double_value;
                else{
#ifdef _DEBUG_INTERPRET
                    printf("LESS_EQUAL something is wrong with op2\n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }

                /*
                 * LESS EQUAL
                 */
                if (comparison_left_operand <= comparison_right_operand)
                    comparison_result = 1;
                else
                    comparison_result = 0;

                /*
                 * Store result
                 */
                if (dest_local != NULL) { //destination in frame
                    dest_local->value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("LESS_EQUAL result = ");
                    if (dest_local->value.t_int == 1)
                        printf("true \n");
                    else if (dest_local->value.t_int == 0)
                        printf("false \n");
#endif

                } else {
                    destination->data->value.constant.value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("LESS_EQUAL result = ");
                    if (destination->data->value.constant.value.t_int == 1)
                        printf("true \n");
                    else if (destination->data->value.constant.value.t_int == 0)
                        printf("false \n");
#endif
                }

                listNext(instructionList);
                break;
            case I_MORE_EQUAL:

                if (op1_type == valueType_int)
                    comparison_left_operand =  op1_int_value;
                else if (op1_type == valueType_double)
                    comparison_left_operand =  op1_double_value;
                else{
#ifdef _DEBUG_INTERPRET
                    printf("MORE_EQUAL something is wrong with op1\n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }


                if (op2_type == valueType_int)
                    comparison_right_operand =  op2_int_value;
                else if (op2_type == valueType_double)
                    comparison_right_operand =  op2_double_value;
                else{
#ifdef _DEBUG_INTERPRET
                    printf("MORE_EQUAL something is wrong with op2\n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }

                /*
                 * MORE EQUAL
                 */
                if (comparison_left_operand >= comparison_right_operand)
                    comparison_result = 1;
                else
                    comparison_result = 0;

                /*
                 * Store result
                 */
                if (dest_local != NULL) { //destination in frame
                    dest_local->value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("MORE_EQUAL result = ");
                    if (dest_local->value.t_int == 1)
                        printf("true \n");
                    else if (dest_local->value.t_int == 0)
                        printf("false \n");
#endif

                } else {
                    destination->data->value.constant.value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("MORE_EQUAL result = ");
                    if (destination->data->value.constant.value.t_int == 1)
                        printf("true \n");
                    else if (destination->data->value.constant.value.t_int == 0)
                        printf("false \n");
#endif
                }

                listNext(instructionList);
                break;
            case I_EQUAL:

                if (op1_type == valueType_int)
                    comparison_left_operand =  op1_int_value;
                else if (op1_type == valueType_double)
                    comparison_left_operand =  op1_double_value;
                else{
#ifdef _DEBUG_INTERPRET
                    printf("EQUAL something is wrong with op1\n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }


                if (op2_type == valueType_int)
                    comparison_right_operand =  op2_int_value;
                else if (op2_type == valueType_double)
                    comparison_right_operand =  op2_double_value;
                else{
#ifdef _DEBUG_INTERPRET
                    printf("EQUAL something is wrong with op2\n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }

                /*
                 * EQUAL
                 */
                if (comparison_left_operand == comparison_right_operand)
                    comparison_result = 1;
                else
                    comparison_result = 0;

                /*
                 * Store result
                 */
                if (dest_local != NULL) { //destination in frame
                    dest_local->value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("EQUAL result = ");
                    if (dest_local->value.t_int == 1)
                        printf("true \n");
                    else if (dest_local->value.t_int == 0)
                        printf("false \n");
#endif

                } else {
                    destination->data->value.constant.value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("EQUAL result = ");
                    if (destination->data->value.constant.value.t_int == 1)
                        printf("true \n");
                    else if (destination->data->value.constant.value.t_int == 0)
                        printf("false \n");
#endif
                }

                listNext(instructionList);
                break;
            case I_NOT_EQUAL:

                if (op1_type == valueType_int)
                    comparison_left_operand =  op1_int_value;
                else if (op1_type == valueType_double)
                    comparison_left_operand =  op1_double_value;
                else{
#ifdef _DEBUG_INTERPRET
                    printf("NOT_EQUAL something is wrong with op1\n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }


                if (op2_type == valueType_int)
                    comparison_right_operand =  op2_int_value;
                else if (op2_type == valueType_double)
                    comparison_right_operand =  op2_double_value;
                else{
#ifdef _DEBUG_INTERPRET
                    printf("NOT_EQUAL something is wrong with op2\n");
#endif
                    freeAndExit(INTERNAL_ERROR);
                }

                /*
                 * NOT EQUAL
                 */
                if (comparison_left_operand != comparison_right_operand)
                    comparison_result = 1;
                else
                    comparison_result = 0;

                /*
                 * Store result
                 */
                if (dest_local != NULL) { //destination in frame
                    dest_local->value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("NOT_EQUAL result = ");
                    if (dest_local->value.t_int == 1)
                        printf("true \n");
                    else if (dest_local->value.t_int == 0)
                        printf("false \n");
#endif

                } else {
                    destination->data->value.constant.value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("NOT_EQUAL result = ");
                    if (destination->data->value.constant.value.t_int == 1)
                        printf("true \n");
                    else if (destination->data->value.constant.value.t_int == 0)
                        printf("false \n");
#endif
                }

                listNext(instructionList);
                break;

            case I_NOT:


                /*
                 * Store result
                 */
                if (dest_local != NULL) { //destination in frame

                    /*
                     * NOT
                     */
                    if (! op1_int_value)
                        dest_local->value.t_int = 1;
                    else
                        dest_local->value.t_int = 0;

#ifdef _DEBUG_INTERPRET
                    printf("NOT result = ");
                    if (dest_local->value.t_int == 1)
                        printf("true \n");
                    else if (dest_local->value.t_int == 0)
                        printf("false \n");
#endif

                } else {
                    /*
                     * NOT
                     */
                    if (! op1_int_value)
                        destination->data->value.constant.value.t_int = 1;
                    else
                        destination->data->value.constant.value.t_int = 0;

#ifdef _DEBUG_INTERPRET
                    printf("NOT result = ");
                    if (destination->data->value.constant.value.t_int == 1)
                        printf("true \n");
                    else if (destination->data->value.constant.value.t_int == 0)
                        printf("false \n");
#endif
                }

                listNext(instructionList);
                break;

            case I_OR:

                /*
                 * OR
                 */
                if (op1_int_value || op2_int_value)
                    comparison_result = 1;
                else
                    comparison_result = 0;

                /*
                 * Store result
                 */
                if (dest_local != NULL) { //destination in frame
                    dest_local->value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("OR result = ");
                    if (dest_local->value.t_int == 1)
                        printf("true \n");
                    else if (dest_local->value.t_int == 0)
                        printf("false \n");
#endif

                } else {
                    destination->data->value.constant.value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("OR result = ");
                    if (destination->data->value.constant.value.t_int == 1)
                        printf("true \n");
                    else if (destination->data->value.constant.value.t_int == 0)
                        printf("false \n");
#endif
                }

                listNext(instructionList);
                break;

            case I_AND:

                /*
                 * AND
                 */
                if (op1_int_value && op2_int_value)
                    comparison_result = 1;
                else
                    comparison_result = 0;


                /*
                 * Store result
                 */
                if (dest_local != NULL) { //destination in frame
                    dest_local->value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("AND result = ");
                    if (dest_local->value.t_int == 1)
                        printf("true \n");
                    else if (dest_local->value.t_int == 0)
                        printf("false \n");
#endif

                } else {
                    destination->data->value.constant.value.t_int = comparison_result;
#ifdef _DEBUG_INTERPRET
                    printf("AND result = ");
                    if (destination->data->value.constant.value.t_int == 1)
                        printf("true \n");
                    else if (destination->data->value.constant.value.t_int == 0)
                        printf("false \n");
#endif
                }

                listNext(instructionList);
                break;

            case I_CALL_FUNC:

#ifdef _DEBUG_INTERPRET
                printf("instrukcia CALL FUNCTION\n");
#endif

                //instruction list for arguments
                tmp = instructionList->active->Instruction.addr3;

                remapArgList(tmp, frame);

                if (tmp != NULL) {
//                    printf("argumentos \n");
#ifdef _DEBUG_INTERPRET
//                    print_elements_of_list(*tmp);
#endif
                }
//
                // instruction list for function
                entry_s *itemFunc;
                itemFunc = instructionList->active->Instruction.addr2;
                tListOfInstr *tmp2;
                tmp2 = itemFunc->data->value.funkce.instrList;
//                print_elements_of_list(*itemFunc->data->value.funkce.instrList);


                if (tmp != NULL){
                    instrListForRecursion = tmp;

                    if (tmp2 != NULL)
                        instrListForRecursion->last->nextItem = tmp2->first;

                } else
                    instrListForRecursion = tmp2;

#ifdef _DEBUG_INTERPRET
                print_elements_of_list(*instrListForRecursion);
#endif
                justInterpretIt(instrListForRecursion, 2, itemFunc->data->value.funkce.local_t);

//                printf("destination key: %p \n", destination);
                if (dest_local != NULL){
//                    printf("dest_local key: %s \n", dest_local->key);
//                    printf("func ret %d", itemFunc->data->value.funkce.ret_val->t_int);

                    if (itemFunc->data->value.funkce.return_type == valueType_int)
                        dest_local->value.t_int = itemFunc->data->value.funkce.ret_val->t_int;
                    else if (itemFunc->data->value.funkce.return_type == valueType_double)
                        dest_local->value.t_double = itemFunc->data->value.funkce.ret_val->t_double;
                    else if (itemFunc->data->value.funkce.return_type == valueType_boolean)
                        dest_local->value.t_int = itemFunc->data->value.funkce.ret_val->t_int;
                    else if (itemFunc->data->value.funkce.return_type == valueType_string){
                        dest_local->value.str.str = myMalloc(strlen(itemFunc->data->value.funkce.ret_val->str.str) + 1);
                        strcpy(dest_local->value.str.str, itemFunc->data->value.funkce.ret_val->str.str);
                    }

                }

                listNext(instructionList);
                break;

            case I_RETURN:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia RETURN\n");
//                printf("dest key: %s \n", destination->key);
#endif

                if (op1->data->type == tTypeConstant)
                    destination->data->value.funkce.ret_val = &op1->data->value.constant.value;
                else if (op1->data->type == tTypeVariable)
                    destination->data->value.funkce.ret_val = &op1->data->value.variable.value;

                instructionList->active = NULL;
                listNext(instructionList);
                break;


            case I_SET_PARAM:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia SET PARAM \n");
#endif

                frameItem = instructionList->active->Instruction.addr2;


                if (dest_local != NULL) { //destination in frame
                    if (dest_local->type == valueType_int) {
//                        dest_local->value.t_int = op1_int_value;
                        dest_local->value.t_int = frameItem->value.t_int;
#ifdef _DEBUG_INTERPRET
                        printf("SET PARAM result = %d \n", dest_local->value.t_int);
#endif
                    }
                    else if (dest_local->type == valueType_double) {
//                        dest_local->value.t_double = op1_double_value;
                        dest_local->value.t_double = frameItem->value.t_double;
#ifdef _DEBUG_INTERPRET
                        printf("SET PARAM result = %f \n", dest_local->value.t_double);
#endif
                    }
                    else if (dest_local->type == valueType_boolean) {
                        dest_local->value.t_int = op1_int_value;
#ifdef _DEBUG_INTERPRET
                        printf("SET PARAM result = %d \n", dest_local->value.t_int);
#endif
                    }
                    else if (dest_local->type == valueType_string) {

                        dest_local->value.str.str = myMalloc( strlen(frameItem->value.str.str) + 1);
                        strcpy(dest_local->value.str.str, frameItem->value.str.str);
#ifdef _DEBUG_INTERPRET
                        printf("SET PARAM result = %s", dest_local->value.str.str);
#endif
                    }
                }

                listNext(instructionList);
                break;

            case I_JUMP:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia JUMP \n");
#endif

                jump = instructionList->active->Instruction.addr1;
                if (jump == NULL){
#ifdef _DEBUG_INTERPRET
                    printf("veliky pruser I_JUMP\n");
#endif
                } else {
                    instructionList->active = jump;
                }

                listNext(instructionList);
                break;

            case I_JUMP_IF_FALSE:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia JUMP_IF_FALSE \n");
#endif

                if (op1_int_value == 0){
#ifdef _DEBUG_INTERPRET
                    printf("false\n");
#endif
                    jump = instructionList->active->Instruction.addr1;
                    if (jump == NULL){
#ifdef _DEBUG_INTERPRET
                        printf("veliky pruser JUMP_IF_FALSE\n");
#endif
                    } else {
                        instructionList->active = jump;
                    }
                } else {
#ifdef _DEBUG_INTERPRET

                    printf("true\n");
#endif

                }


                listNext(instructionList);
                break;

            case I_LABEL:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia LABEL \n");
#endif
                listNext(instructionList);
                break;


            case I_ASSIGN:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia ASSIGN \n");
#endif

                if (dest_local != NULL) { //destination in frame
                    if (dest_local->type == valueType_int){
                        dest_local->value.t_int = op1_int_value;
#ifdef _DEBUG_INTERPRET
                        printf("assign result = %d \n", dest_local->value.t_int);
#endif
                    }
                    else if (dest_local->type == valueType_double){
                        dest_local->value.t_double = op1_double_value;
#ifdef _DEBUG_INTERPRET
                        printf("assign result = %f \n", dest_local->value.t_double);
#endif
                    }
                    else if (dest_local->type == valueType_string){

                        dest_local->value.str.str = myMalloc(strlen(op1_string_value) + 1);
                        strcpy(dest_local->value.str.str, op1_string_value);
#ifdef _DEBUG_INTERPRET
                        printf("assign result = %s \n", dest_local->value.str.str);
#endif

                    }
                    else if (dest_local->type == valueType_boolean){
                        dest_local->value.t_int = op1_int_value;
#ifdef _DEBUG_INTERPRET
                        printf("assign result = %d \n", dest_local->value.t_int);
#endif
                    }

                } else {

//                    printf("destination->data->type : %p \n", destination);

                    if(destination->data->type == tTypeFunction ||
                       destination->data->type == tTypeConstant ||
                       destination->data->type == tTypeClass){
#ifdef _DEBUG_INTERPRET
                        printf("destination invalid type \n");
#endif
                        freeAndExit(INTERNAL_ERROR);
                    }
                    if(op1->data->type == tTypeClass){
#ifdef _DEBUG_INTERPRET
                        printf("op1 invalid type \n");
#endif
                        freeAndExit(INTERNAL_ERROR);
                    }

                    if(destination->data->value.variable.type == valueType_int){
                        destination->data->value.variable.value.t_int = op1_int_value;
#ifdef _DEBUG_INTERPRET
                        printf("assign result = %d \n", destination->data->value.variable.value.t_int);
#endif
                    }
                    else if(destination->data->value.variable.type == valueType_double){

                        if(op1_type == valueType_int){
                            destination->data->value.variable.value.t_double = op1_int_value;
                        } else if (op1_type == valueType_double){
                            destination->data->value.variable.value.t_double = op1_double_value;
                        }
#ifdef _DEBUG_INTERPRET
                        printf("assign result = %f \n", destination->data->value.variable.value.t_double);
#endif
                    }
                    else if(destination->data->value.variable.type == valueType_string){
                        if(op1_type == valueType_string){
                            destination->data->value.variable.value.str.str = myMalloc(strlen(op1_string_value) + 1);
                            strcpy(destination->data->value.variable.value.str.str, op1_string_value);
#ifdef _DEBUG_INTERPRET
                            printf("assign result = %s \n", destination->data->value.variable.value.str.str);
#endif
                        }
                    } else if(destination->data->value.variable.type == valueType_boolean){
                        destination->data->value.variable.value.t_int = op1_int_value;
#ifdef _DEBUG_INTERPRET
                        printf("assign result = %d \n", destination->data->value.variable.value.t_int);
#endif
                    }

                }

                listNext(instructionList);
                break;

            case I_PRINT:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia PRINT \n");
#endif

                if(op1_type == valueType_int) {
#ifdef _DEBUG_INTERPRET
                    printf("print result = %d \n", op1_int_value);
#endif

                    printf("%d", op1_int_value);
                }
                else if(op1_type == valueType_double) {
#ifdef _DEBUG_INTERPRET
                    printf("print result = %g \n", op1_double_value);
#endif
                    printf("%g", op1_double_value);
                }
                else if(op1_type == valueType_boolean) {

                    if(op1_int_value == 1) {
                        printf("true");
                    }
                    else if (op1_int_value == 0) {
                        printf("false");

                    }

                }
                else if (op1_type == valueType_string){
                    printf("%s", op1_string_value);
                }

                listNext(instructionList);
                break;

            case I_READLN_INT:
                /*
                 *  TODO add exit code to readSomething() functions when the input format is bad
                 */
#ifdef _DEBUG_INTERPRET
                printf("instrukcia I_READLN_INT \n");
#endif
                if(dest_local != NULL){
                    if (dest_local->type == valueType_int) {
                        dest_local->value.t_int = readInt();
                    }
                }
                else {
//                int result_int;
                    result_int = readInt();
                    destination->data->value.constant.value.t_int = result_int;

                    //printf("%d", destination->data->value.constant.value.t_int);
                }


                listNext(instructionList);

                break;

            case I_READLN_DOUBLE:
                /*
                 *  TODO add exit code to readSomething() functions when the input format is bad
                 */
#ifdef _DEBUG_INTERPRET
                printf("instrukcia I_READLN_DOUBLE \n");
#endif

                if(dest_local != NULL){
                    if (dest_local->type == valueType_double) {
                        dest_local->value.t_double = readDouble();
                    }
                }
                else {
                    result_double = readDouble();
                    destination->data->value.constant.value.t_double = result_double;
                }

#ifdef _DEBUG_INTERPRET
                printf("result double = %g \n", destination->data->value.constant.value.t_double);
#endif

                listNext(instructionList);
                break;

            case I_READLN_STRING:
                /*
                 *  TODO add exit code to readSomething() functions when the input format is bad
                 */
#ifdef _DEBUG_INTERPRET
                printf("instrukcia I_READLN_STRING \n");
#endif
                if(dest_local != NULL){
                    if (dest_local->type == valueType_string) {
                        dest_local->value.str = readString();
                    }
                }
                else {
                    result_string = readString();
                    destination->data->value.constant.value.str = result_string;
                }

#ifdef _DEBUG_INTERPRET
                printf("result string = %s \n", destination->data->value.constant.value.str);
#endif

                listNext(instructionList);
                break;


            case I_LENGTH:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia I_LENGTH \n");
#endif

                init_value_str(&str_tmp, op1_string_value);

                init_value_str(&str_1, op1_string_value);
                dest_local->value.t_int = StringLength(&str_1);

#ifdef _DEBUG_INTERPRET
                printf("result string = %d \n", dest_local->value.t_int);
#endif
                listNext(instructionList);
                break;

            case I_COMPARE:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia I_COMPARE \n");
#endif
                init_value_str(&str_1, op1_string_value);
                init_value_str(&str_2, op2_string_value);

                dest_local->value.t_int = StringCmpString(&str_1, &str_2);

#ifdef _DEBUG_INTERPRET
                printf("result = %d \n", dest_local->value.t_int);
#endif
                listNext(instructionList);
                break;

            case I_FIND:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia I_FIND\n");
#endif
                init_value_str(&str_1, op1_string_value);
                init_value_str(&str_2, op2_string_value);

                dest_local->value.t_int = find(&str_1, &str_2);
#ifdef _DEBUG_INTERPRET

                printf("result = %d \n", dest_local->value.t_int);
#endif

                listNext(instructionList);
                break;

            case I_SORT:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia I_SORT\n");
#endif
                init_value_str(&str_1, op1_string_value);

                heap_sort(&str_1);

                dest_local->value.str = str_1;
#ifdef _DEBUG_INTERPRET
                printf("result = %s \n", dest_local->value.str);
#endif

                listNext(instructionList);
                break;

/*            case I_SUBSTR:
#ifdef _DEBUG_INTERPRET
                printf("instrukcia I_SORT\n");
#endif
                GetSubstr()

                listNext(instructionList);
                break;*/
        }
    } while(instructionList->active != NULL);
}




t_table_frame * mapToFrameTable(t_hashtable *local_symbol_table, int size_of_local_table) {

    t_table_frame *newFrameTable;
    newFrameTable = frameInit(size_of_local_table);

    for (int i = 0; i < size_of_local_table; i++) {

        if (local_symbol_table->htable[i] != NULL) {

            t_table_frame_item *newFrameItem = myMalloc(sizeof(t_table_frame_item));
            entry_s *item = local_symbol_table->htable[i];
//            t_table_frame_item *temp = frameSearch(newFrameTable, item->key); // check if item is already in frame

//            if (temp == NULL){

                switch (item->data->type) {
                    case tTypeVariable:
                        newFrameItem->key = myMalloc(strlen(item->key)+1);
                        strcpy(newFrameItem->key, item->key);
                        newFrameItem->type = item->data->value.variable.type;
                        newFrameItem->value = item->data->value.variable.value;
                        newFrameItem->is_init = item->data->value.variable.is_init;
//                        printf("newFrameItem->key: %s", newFrameItem->key);
                        frameInsert(newFrameTable, newFrameItem->key, newFrameItem);
                        break;

                    case tTypeConstant:
                        newFrameItem->key = myMalloc(strlen(item->key)+1);
                        strcpy(newFrameItem->key, item->key);
                        newFrameItem->type = item->data->value.constant.type;
                        newFrameItem->value = item->data->value.constant.value;
                        newFrameItem->is_init = true; //constant is always init
                        frameInsert(newFrameTable, newFrameItem->key, newFrameItem);
                        break;

                        // nemozeme dostat na premapovanie tieto veci
                    case tTypeFunction:
                    case tTypeClass:
                    case tTypeFunItem:
                        break;
                }
//            }

            // TODO fix more than one item in index
            while (item->next != NULL){  //prejdi aj polozky zo zretazeneho zoznamu
                item = item->next;
                switch (item->data->type) {
                    case tTypeVariable:
                        newFrameItem->key = myMalloc(strlen(item->key)+1);
                        strcpy(newFrameItem->key, item->key);
                        newFrameItem->type = item->data->value.variable.type;
                        newFrameItem->value = item->data->value.variable.value;
                        newFrameItem->is_init = item->data->value.variable.is_init;
                        frameInsert(newFrameTable, newFrameItem->key, newFrameItem);
                        break;

                    case tTypeConstant:
                        newFrameItem->key = myMalloc(strlen(item->key)+1);
                        strcpy(newFrameItem->key, item->key);
                        newFrameItem->type = item->data->value.constant.type;
                        newFrameItem->value = item->data->value.constant.value;
                        newFrameItem->is_init = true; //constant is always init
                        frameInsert(newFrameTable, newFrameItem->key, newFrameItem);
                        break;

                        // nemozeme dostat na premapovanie tieto veci
                    case tTypeFunction:
                    case tTypeClass:
                    case tTypeFunItem:
                        break;
                }

            }
        }
    }

//    table_print(local_symbol_table);
//    table_frame_print(newFrameTable);

    return newFrameTable;
}

void remapArgList(tListOfInstr* list, t_table_frame * frame){
    tListItem *tmpItem = list->first;
    while (tmpItem != NULL){

        if(tmpItem->Instruction.instType == I_SET_PARAM){

            entry_s *tmpOp1 =  tmpItem->Instruction.addr2;
            t_table_frame_item *frmItem = frameSearch(frame, tmpOp1->key);
            if (frmItem != NULL){
                tmpItem->Instruction.addr2 = frmItem;
            } else {
                t_table_frame_item *newFrameItem;
                newFrameItem = myMalloc(sizeof(t_table_frame_item));
                switch (tmpOp1->data->type){
                    case tTypeConstant:
                        newFrameItem->type = tmpOp1->data->value.constant.type;
                        newFrameItem->key = tmpOp1->key;
                        newFrameItem->is_init = true; //constant is always init
                        newFrameItem->next = NULL;
                        newFrameItem->value = tmpOp1->data->value.constant.value;
                        break;
                    case tTypeVariable:
                        newFrameItem->type = tmpOp1->data->value.variable.type;
                        newFrameItem->key = tmpOp1->key;
                        newFrameItem->is_init = tmpOp1->data->value.variable.is_init;
                        newFrameItem->next = NULL;
                        newFrameItem->value = tmpOp1->data->value.constant.value;
                        break;
                    default:
                        break;

                }
                tmpItem->Instruction.addr2 = newFrameItem;
            }
        }
        tmpItem = tmpItem->nextItem;
    }
}

void table_frame_print(t_table_frame * hashtable_s) {
    int hasht;
    printf("<---- frameTable: %p ---->\n",  (void *) hashtable_s);
    t_table_frame_item *list, *pom;
    for (hasht = 0; hasht < hashtable_s->size; hasht++) {
        list = hashtable_s->items[hasht];
        while (list != NULL) {
            pom = list;
            list = list->next;
            printf("<--  idx: %d, ", hasht);
            printf("key: %s, ", pom->key);
            printf("tab_data_type: ");
/*            switch (pom->type->type){
                case tTypeVariable:
                    printf("VAR, ");
                    break;
                case tTypeFunction:
                    printf("FUNC, ");
//                    printf("data_value_typ: ");
//                    customPrintDataType(pom->data->value.variable.type);
                    break;
                case tTypeClass:
                    printf("CLASS, ");
                    break;
                case tTypeConstant:
                    printf("CONST, ");
                    printf("data_value_typ: ");
                    customPrintDataType(pom->data->value.constant.type);
                    break;
                case tTypeFunItem:
                    printf("F_ITEM, ");
                    break;
            }*/
            printf("-->\n");
        }
    }
}



/*
 * table return 1 if found key in ts
 */
int isKeyInFt(char *key_addr_ts, t_table_frame *newFrameTable) {

    //todo zrefaktorovat

    t_table_frame_item *founded_item_in_ft = NULL;
    int hash_val =  frameHash(newFrameTable, key_addr_ts);

    for(founded_item_in_ft = newFrameTable->items[hash_val]; founded_item_in_ft != NULL; founded_item_in_ft = founded_item_in_ft->next) {

        if (founded_item_in_ft == NULL){
//            printf("item je NULL!!!! \n");
        }

        if (founded_item_in_ft != NULL) {

            if (founded_item_in_ft->key != NULL) {
//                printf("isKeyInFt dostali sme sa ku kluciku\n");
            }
            if (strcmp(key_addr_ts, founded_item_in_ft->key) == 0)
                return 1;
        }
    }

    return 0;
}

