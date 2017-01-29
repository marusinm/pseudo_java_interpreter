/* ****************************** main.c ************************************ */
/* Soubor:              main.c                                                */
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
#include <errno.h>
#include <memory.h>

#include "common.h"

#include "scanner.h"
#include "ilist.h"
#include "precedence.h"
#include "parser.h"
#include "interpret.h"
#include "ial.h"


//#define _DEBUG

t_hashtable *symbol_table;
tToken *token = NULL;
t_garb_coll* garb_coll;

void freeAndExit(int exit_code){
   // garbage collector free memory
   freeGC();
   // close file
   fclose(file);
   // free memory of all instruction lists
   free(staticInstrList);
   free(classifiedIdentifier);
   free(ListOfSymbolsTables);
   // free token after second phase
   free(globalToken->data);
   free(globalToken);

   exit(exit_code);
}


int main(int argc, char** argv) {

   file = NULL;

   if (argc == 0)
   {
//        printf("Neni zadan vstupni soubor\n");
      exit(FILE_ERROR);
   }
   if ((file = fopen(argv[1], "r")) == NULL)
   {
//        printf("Soubor se nepodarilo otevrit\n");
      exit(FILE_ERROR);
   }

#ifdef _DEBUG
   printf("\n");
#endif
   /*
    * Instruction list init
    */
   staticInstrList = malloc(sizeof(tListOfInstr));
   listInit(staticInstrList);

   /*
    * Garbage collector init
    */
      int a = initGC();
      if(a != -1){
   //         printf("Garbage Collector. Memory Allocated\n");
   }


   ListOfSymbolsTables = malloc(sizeof(tListOfSymbolsTables));
   classifiedIdentifier = malloc(sizeof(tClassifiedIdentifier));


   globalToken = NULL;
   symbol_table = htInit(30);
   local_symbol_table = htInit(30);

   // zaciatok parseru pre prvu fazu prechodu
   parser_phase = first_phase;
   int result_code = parseProgram();
   if (result_code != OK) {
      exit(result_code);
   }
   //vycistime pamat po prvej faze prechodu - mozno nebude za potreby
   free(globalToken->data);
   free(globalToken);
   fclose(file); //zavreme subor aby sme ho mohli do druhej fazy otvorit a citat od zaciatku

   globalToken = NULL;
   file = NULL;
   file = fopen(argv[1], "r");

   // spustenie parseru pre druhu fazu prechodu
   parser_phase = second_phase;
   result_code = parseProgram();
   if (result_code != OK) {
      exit(result_code);
   }


   /*
    * helper tables
    */
   // zacnime funkciou run v maine
   entry_s* classMainItem = htSearch(symbol_table, "Main");
   t_hashtable* classSymbolTable = classMainItem->data->value.class.class_table;
   // table_print(classSymbolTable);

   entry_s* funcRunItem = htSearch(classSymbolTable, "run");

   tListOfInstr *runInstructionList = funcRunItem->data->value.funkce.instrList;

   justInterpretIt(staticInstrList, 0, symbol_table); // inicializovanie statickych premennych

   justInterpretIt(runInstructionList, 1, funcRunItem->data->value.funkce.local_t); // interpretovanie ostatnych veci

   createFrame();

   // garbage collector free memory
   freeGC();
   // close file
   fclose(file);
   // free memory of all instruction lists
   free(staticInstrList);
   free(classifiedIdentifier);
   free(ListOfSymbolsTables);
   // free token after second phase
   free(globalToken->data);
   free(globalToken);


   return 0;
}
