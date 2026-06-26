#ifndef HASH_H
#define HASH_H

//tamanho da tabela hash
#define TAM_TAB 150001

//estrutura do nó da lista encadeada caso haja colisões
typedef struct No{
    char id[12];                       //armazenar o identificador
    struct No* prox;                   //ponteiro para o próximo nó
}No;

//estrutura da tabela hash
typedef struct{
    No* tab[TAM_TAB];                  //vetor de ponteiros para os nos
    int q_elem;                        //contador de elem. inseridos
    int colis;                         //contador de colisoes
}TabHash;

void h_ini(TabHash* t);                //iniciar a tabela hash
unsigned int h_func(const char* str);  //indice gerado pela funcao
int h_ins(TabHash*t, const char* id);  //inserir um usuario
int h_bus(TabHash*t, const char* id);  //conferir se o usuario foi inserido
void h_lib(TabHash* t);                //limpar a memoria alocada para a tabela

//função para ver os status de desempenho e eficiencia da tabela
void h_status(TabHash* t);

#endif