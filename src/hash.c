#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

//Função para iniciar a tabela zerando as posições e contadores
void h_ini(TabHash* t){
    int i;
    for (i=0; i<TAM_TAB; i++){
        t->tab[i]=NULL;
    }
    t->q_elem=0;
    t->colis=0;
}

//Função Hash Daniel J. Bernstein
unsigned int h_func(const char* str){
    unsigned long hash = 5381;
    int c;

    while((c=*str++)){
        hash = ((hash<<5)+hash)+c;
    }
    return hash%TAM_TAB;
}

//Função para inserir elementos na tabela
int h_ins(TabHash* t, const char* id){
    unsigned int indx=h_func(id);       //indx - ta armazenando o indice gerado pela funcao
    No* perc=t->tab[indx];              //perc - ponteiro para percorrer a lista caso haja colisao
    
    //verificar se o elemento ja existe
    while(perc!=NULL){
        if(strcmp(perc->id, id)==0){
            return 0;                   //retorna 0 se o elemento já está na tabela
        }
        perc=perc->prox;
    }

    //Alocar um novo no que vai ser inserido
    No* p= (No*)malloc(sizeof(No));
    if(p==NULL) return -1;              //retorna -1 se tiver falha de memoria

    strcpy(p->id, id);

    //Se tiver elemento na posicao, entao adicione mais um colisao
    if(t->tab[indx]!=NULL){
        t->colis++;
    }

    //Insere no inicio da lista encadeada
    p->prox=t->tab[indx];
    t->tab[indx]=p;
    t->q_elem++;
    return 1;                           //retorna 1 se deu certo inserir
}

int h_bus(TabHash* t, const char* id){
    unsigned int indx= h_func(id);        //indice calculado para a busca
    No* proc=t->tab[indx];                //proc - ponteiro de procura para varredura da lista

    while(proc!=NULL){
        if(strcmp(proc->id, id)==0){
            return 1;                     //retorna 1 quando foi encontrado
        }
        proc=proc->prox;
    }
    return 0;                             //retorna 0 se nao for encontrado
}

void h_status(TabHash* t){
    printf("%d\n", t->q_elem);              //elementos armazenados
    printf("%d\n", TAM_TAB);                //tamanho max da tabela
    printf("%d\n", t->colis);               //colisoes ocorridas
    
    double alpha= (double)t->q_elem / TAM_TAB;
    printf("%.4f\n", alpha);                //fator de carga
}

//funcao para liberação da memoria
void h_lib(TabHash* t){
    int i;
    No *cur, *aux;               //guarda como referencia para nao perder qual é o proximo no a ser liberado
    for(i=0; i<TAM_TAB; i++){
        cur=t->tab[i];
        
        while(cur!=NULL){
            aux=cur->prox;
            free(cur);
            cur=aux;
        }
        t->tab[i]=NULL;
    }
    t->q_elem=0;
    t->colis=0;
}