/* ***************************** interpret.h ******************************** */
/* Soubor:              interpret.h                                           */
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

#include <stdio.h>
#include <stdlib.h>
#include "ilist.h"

#include "ial.h"
#include "frame.h"
#include "built_in_functions.h"

#define DIVIDE_ZERO 9
#define UNITIALIZED 8
#define READ_INPUT_ERR 7
#define SEMANTIC_TYPE_ERR 4

//char * key;

extern t_hashtable *symbol_table;

extern tListOfInstr *instList;

//void justInterpretIt(int static_variables);
void justInterpretIt(tListOfInstr *instructionList, int type_of_instr_list, t_hashtable *local_symbol_table);

//void generateConstant(t_hashtable* hashtable, char * key, data_value_type valueType);

int isKeyInFt(char *key_addr_ts, t_table_frame *newFrameTable);

t_table_frame_item* remapAddrStToAddrFt(entry_s *interp_addr);

t_table_frame * mapToFrameTable(t_hashtable *local_symbol_table, int size_of_local_table);
void table_frame_print(t_table_frame * hashtable_s);

void remapArgList(tListOfInstr* list, t_table_frame * frame);

//tListItem remapInstrunctionToFrameTable(tListItem *oldInstructionItem, instructions newInstructionType);

//tListOfInstr *newFrameInstructionList;


t_hashtable *local_symbol_table;

/*
 *  Our one frame instruction
 */
tListItem newFrameInstructionItem;


// declare new instruction structure
tListItem newListItem;

// declare new instruction structure
tListItem newListItem_2;

// declare new instruction callFunction
tListItem newInstrCallFunction;
