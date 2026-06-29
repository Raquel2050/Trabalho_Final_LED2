#include "bloom.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//FUNÇÕES HASH
//Recebe uma string e gera um valor hash.

static unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;

    while (*str) {
        hash = ((hash << 5) + hash) + *str;
        str++;
    }

    return hash;
}
//Também gera um valor hash para ser utilizado
//juntamente com o DJB2 no Double Hashing.

static unsigned long hash_sdbm(const char *str) {
    unsigned long hash = 0;

    while (*str) {
        hash = *str + (hash << 6) + (hash << 16) - hash;
        str++;
    }

    return hash;
}

//FUNÇÃO AUXILIAR
//Calcula a posição do vetor de bits utilizando
//a técnica de Double Hashing.
//h1: primeira função hash.
//h2: segunda função hash.
//tentativa: número da função hash (0 até k-1).
//tamanho: tamanho do vetor de bits.
static size_t calcular_hash(unsigned long h1,
unsigned long h2, size_t tentativa, size_t tamanho) {
    return (h1 + tentativa * h2) % tamanho; 
}

//CRIAÇÃO DO FILTRO


//Cria um novo Filtro de Bloom.
//Calcula automaticamente:
// m-> tamanho do vetor de bits.
//k-> quantidade de funções hash.

filtrobloom *criar(size_t n, double p) {
    filtrobloom *filtro;

    filtro = (filtrobloom *) malloc(sizeof(filtrobloom));

    if (filtro == NULL)
        return NULL;

    //Calcula o tamanho do vetor de bits 

    double m = -((double)n * log(p)) /
               (log(2) * log(2));

    filtro->tam_bits = (size_t) ceil(m);

   //Calcula a quantidade de funções hash

    double k = ((double) filtro->tam_bits /
               (double) n) * log(2);

    filtro->num_hash = (size_t) round(k);

    if (filtro->num_hash == 0)
        filtro->num_hash = 1;

    //Calcula a quantidade de bytes necessária

    size_t bytes = (filtro->tam_bits + 7) / 8;

    filtro->v_bits = (unsigned char *)
        calloc(bytes, sizeof(unsigned char));

    if (filtro->v_bits == NULL){
      free(filtro);
      return NULL;
    }

    return filtro;
}

//DESTRUIÇÃO DO FILTRO
//Libera toda a memória utilizada pelo filtro.

void destruir(filtrobloom *filtro) {
    if (filtro == NULL)
    return;

    free(filtro->v_bits);
    filtro->v_bits = NULL;

    free(filtro);
}

//INSERÇÃO DE ELEMENTOS
//Insere uma string no Filtro de Bloom.
//Para cada função hash é calculada uma posição diferente no vetor de bits. Em seguida, o bit correspondente é ligado.

void inserir(filtrobloom *filtro, const char *texto) {
    unsigned long h1 = hash_djb2(texto);
    unsigned long h2 = hash_sdbm(texto);

    for (size_t i = 0; i < filtro->num_hash; i++) {
        //Calcula a posição do bit 
        size_t posicao = calcular_hash(
        h1, h2, i, filtro->tam_bits);

      //Descobre em qual byte o bit está 
        size_t indice = posicao / 8;

        //Descobre a posição do bit dentro do byte 
        size_t deslocamento = posicao % 8;

        //Liga o bit 
        filtro->v_bits[indice] |= (1 << deslocamento);
    }
}

//CONSULTA DE ELEMENTOS
//Verifica se uma string pertence ao conjunto.
//Retorna:
//true-> provavelmente pertence.
//false-> certamente não pertence.

bool consultar(filtrobloom *filtro, const char *texto) {
    unsigned long h1 = hash_djb2(texto);
    unsigned long h2 = hash_sdbm(texto);

    for (size_t i = 0; i < filtro->num_hash; i++) {
        //Calcula a posição correspondente 
        size_t posicao = calcular_hash(
        h1, h2, i, filtro->tam_bits);
        size_t indice = posicao / 8;
        size_t deslocamento = posicao % 8;

        //Se algum bit estiver desligado, o elemento certamente não pertence. 
      
        if ((filtro->v_bits[indice] & (1 << deslocamento)) == 0)
        {return false;}
    }

    //Todos os bits estavam ligados 
  
    return true;
}

//MEDIÇÃO DE FALSOS POSITIVOS
//Mede experimentalmente a taxa de falsos positivos.
//Esta função deve receber um conjunto de elementos que não foram inseridos previamente no filtro.
//A taxa é calculada por:
//falsos positivos / quantidade de testes

double medir_falsos_positivos(
    filtrobloom *filtro, const char *elementos[], size_t quantidade) { 
  size_t falsos_positivos = 0;
  for (size_t i = 0; i < quantidade; i++)
    {
        if (consultar(filtro, elementos[i]))
        {falsos_positivos++;}
    }

    if (quantidade == 0)
    {return 0.0;}

    return (double)falsos_positivos /
           (double)quantidade;
}
