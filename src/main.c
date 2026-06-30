#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "hash.h"
#include "bloom.h"

#define MAX_LINE  100
#define MAX_USERS 100000

/* ---------------------------------------------------------------
 * CONTADORES GLOBAIS DE USO (RF03)
 * Registram o historico de consultas desde o inicio da sessao.
 * --------------------------------------------------------------- */
static long   g_consultas_total  = 0;   /* total de consultas realizadas   */
static long   g_evitadas_bloom   = 0;   /* consultas que o Bloom barrou    */
static long   g_falsos_positivos = 0;   /* Bloom disse sim, Hash disse nao */
static double g_tempo_total_seg  = 0.0; /* soma dos tempos de consulta     */

/* ---------------------------------------------------------------
 * PROTOTIPOS
 * --------------------------------------------------------------- */
void limpar_buffer(void);
void gerar_usuario(char *buffer);
int  validar_formato(const char *id);
void inserir_manual(TabHash *tabela, filtrobloom *bloom);
void consultar_usuario(TabHash *tabela, filtrobloom *bloom);
void exibir_estatisticas(TabHash *tabela, filtrobloom *bloom);
void inserir_lote(TabHash *tabela, filtrobloom *bloom);
void executar_experimentos(TabHash *tabela, filtrobloom **bloom_ptr);
void mostrar_menu(void);

/* ---------------------------------------------------------------
 * UTILITARIOS
 * --------------------------------------------------------------- */

void limpar_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* Gera usuario aleatorio no formato [8 letras minusculas][3 digitos]. */
void gerar_usuario(char *buffer) {
    const char letras[] = "abcdefghijklmnopqrstuvwxyz";  // Conjunto de letras minúsculas
    int i;
    
    // Gera 8 letras minúsculas aleatórias
    for (i = 0; i < 8; i++)
        buffer[i] = letras[rand() % 26];  // Sorteia uma letra do alfabeto
    
    // Gera 3 dígitos numéricos aleatórios (0-9)
    for (i = 0; i < 3; i++)
        buffer[8 + i] = '0' + (rand() % 10);  // Converte número para caractere
    
    buffer[11] = '\0';  // Adiciona terminador nulo ao final da string
}

int validar_formato(const char *id) {
    int i;
    
    // Verifica se o ID tem exatamente 11 caracteres (8 letras + 3 números)
    if (strlen(id) != 11)
        return 0;  // Tamanho incorreto -> inválido
    
    // Verifica os primeiros 8 caracteres (devem ser letras minúsculas)
    for (i = 0; i < 8; i++)
        if (!islower((unsigned char) id[i]))
            return 0;  // Caractere não é letra minúscula -> inválido
    
    // Verifica os últimos 3 caracteres (devem ser dígitos)
    for (i = 8; i < 11; i++)
        if (!isdigit((unsigned char) id[i]))
            return 0;  // Caractere não é dígito -> inválido
    
    return 1;  // Todos os critérios foram atendidos -> válido
}

/* ---------------------------------------------------------------
 * RF01 — INSERCAO MANUAL
 * --------------------------------------------------------------- */
void inserir_manual(TabHash *tabela, filtrobloom *bloom) {
    char buffer[100];  // Buffer para armazenar o ID digitado pelo usuário

    // Exibe cabeçalho da opção
    printf("\n=== INSERIR USUARIO ===\n");
    printf("Digite o ID (formato: 8letras + 3numeros): ");
    
    // Lê a entrada do usuário
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';  // Remove o caractere de quebra de linha

    // PASSO 1: Valida o formato do ID
    // Verifica se tem 8 letras minúsculas + 3 dígitos (exatamente 11 caracteres)
    if (!validar_formato(buffer)) {
        printf("ERRO: ID invalido. Use exatamente 8 letras minusculas + 3 digitos.\n");
        return;  // Sai da função sem inserir
    }

    // PASSO 2: Verifica se o usuário já existe na Tabela Hash
    // Evita duplicidade no cadastro
    if (h_bus(tabela, buffer) == 1) {
        printf("ERRO: Usuario '%s' ja esta cadastrado!\n", buffer);
        return;  // Sai da função sem inserir
    }

    // PASSO 3: Insere o usuário na Tabela Hash
    // h_ins() retorna 1 em caso de sucesso
    if (h_ins(tabela, buffer) == 1) {
        // PASSO 4: Insere o usuário no Filtro de Bloom
        // Mantém a sincronia entre as duas estruturas
        inserir(bloom, buffer);
        
        // Confirmação para o usuário
        printf("SUCESSO: Usuario '%s' cadastrado!\n", buffer);
    } else {
        // Caso ocorra erro na inserção na Tabela Hash
        printf("ERRO: Falha ao cadastrar usuario!\n");
    }
}
/* ---------------------------------------------------------------
 * RF02 — CONSULTA
 * Fluxo obrigatorio:
 *   1. Consulta o Filtro de Bloom.
 *   2. Se Bloom diz "nao existe" -> retorna imediatamente.
 *   3. Se Bloom diz "possivelmente existe" -> consulta a Hash.
 *   4. Informa o resultado e registra nos contadores do RF03.
 * --------------------------------------------------------------- */
void consultar_usuario(TabHash *tabela, filtrobloom *bloom) {
    char buffer[100];         // Buffer para armazenar o ID a ser consultado
    clock_t inicio, fim;      // Variáveis para medição de tempo
    double tempo;             // Tempo da consulta em segundos

    // Exibe cabeçalho e solicita o ID ao usuário
    printf("\n=== CONSULTAR USUARIO ===\n");
    printf("Digite o ID do usuario: ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';  // Remove quebra de linha

    // Inicia a medição de tempo da consulta
    inicio = clock();
    g_consultas_total++;  // Incrementa contador global de consultas

    /* ============================================================
     * PASSO 1: Consultar o Filtro de Bloom (consulta probabilística)
     * ============================================================ */
    
    // consultar() retorna:
    // - false (0): elemento CERTAMENTE NÃO existe → pode retornar imediatamente
    // - true (1): elemento POSSIVELMENTE existe → precisa verificar na Hash
    if (!consultar(bloom, buffer)) {
        // Bloom disse "definitivamente não existe" → consulta encerrada!
        fim = clock();
        tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
        
        // Atualiza métricas globais
        g_evitadas_bloom++;              // Consulta foi evitada pelo Bloom
        g_tempo_total_seg += tempo;      // Acumula tempo total
        
        // Exibe resultado e tempo da consulta
        printf("RESULTADO: Usuario '%s' INEXISTENTE (Bloom barrou)\n", buffer);
        printf("Tempo: %.6f s\n", tempo);
        return;  // Sai da função imediatamente (não consulta a Hash)
    }

    /* ============================================================
     * PASSO 2: Bloom disse "possivelmente existe"
     *          → consultar a Tabela Hash (consulta exata)
     * ============================================================ */
    
    // h_bus() retorna:
    // - 1: elemento ENCONTRADO na Tabela Hash
    // - 0: elemento NÃO encontrado (falso positivo do Bloom)
    if (h_bus(tabela, buffer) == 1) {
        // Caso 1: Bloom acertou → elemento realmente existe
        fim = clock();
        tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
        g_tempo_total_seg += tempo;
        
        printf("RESULTADO: Usuario '%s' ENCONTRADO\n", buffer);
        printf("Tempo: %.6f s\n", tempo);
    } else {
        // Caso 2: Bloom errou → falso positivo!
        // O Bloom disse que existia, mas não está na Hash
        fim = clock();
        tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
        
        // Atualiza métricas globais
        g_falsos_positivos++;            // Incrementa contador de falsos positivos
        g_tempo_total_seg += tempo;
        
        printf("RESULTADO: Usuario '%s' INEXISTENTE (falso positivo do Bloom)\n", buffer);
        printf("Tempo: %.6f s\n", tempo);
    }
}

/* ---------------------------------------------------------------
 * RF03 — ESTATISTICAS
 * --------------------------------------------------------------- */
void exibir_estatisticas(TabHash *tabela, filtrobloom *bloom) {
    printf("\n=== ESTATISTICAS DO SISTEMA ===\n");

    /* ============================================================
     * SEÇÃO 1: ESTATÍSTICAS DA TABELA HASH
     * ============================================================ */
    printf("\n--- TABELA HASH ---\n");
    printf("Elementos armazenados : %d\n",   tabela->q_elem);      // Total de elementos na tabela
    printf("Tamanho da tabela     : %d\n",   TAM_TAB);             // Capacidade máxima (150.001)
    printf("Colisoes ocorridas    : %d\n",   tabela->colis);       // Número de colisões na inserção
    
    // Fator de carga (alpha) = número de elementos / tamanho da tabela
    // Quanto menor, melhor o desempenho
    printf("Fator de carga (alpha): %.6f\n", (double) tabela->q_elem / TAM_TAB);
    
    // Percentual de colisões em relação ao total de elementos
    if (tabela->q_elem > 0)
        printf("Percentual de colisoes: %.2f%%\n",
               (double) tabela->colis / tabela->q_elem * 100.0);

    /* ============================================================
     * SEÇÃO 2: ESTATÍSTICAS DO FILTRO DE BLOOM
     * ============================================================ */
    printf("\n--- FILTRO DE BLOOM ---\n");
    printf("Tamanho do vetor de bits: %zu bits (%.2f KB)\n",
           bloom->tam_bits, (double) bloom->tam_bits / 8.0 / 1024.0);  // Tamanho em bits e KB
    printf("Numero de funcoes hash  : %zu\n", bloom->num_hash);         // Quantas funções hash (k)
    printf("Memoria utilizada       : %zu bytes\n", (bloom->tam_bits + 7) / 8);  // Memória em bytes

    /* ============================================================
     * SEÇÃO 3: ESTATÍSTICAS DE CONSULTAS NA SESSÃO ATUAL
     * ============================================================ */
    printf("\n--- CONSULTAS NA SESSAO ---\n");
    printf("Total de consultas      : %ld\n", g_consultas_total);        // Total de consultas realizadas
    printf("Evitadas pelo Bloom     : %ld\n", g_evitadas_bloom);         // Consultas barradas pelo Bloom
    printf("Falsos positivos        : %ld\n", g_falsos_positivos);       // Vezes que o Bloom errou
    
    // Só calcula se houve pelo menos uma consulta
    if (g_consultas_total > 0) {
        // Taxa de falsos positivos = falsos positivos / total de consultas * 100
        printf("Taxa de falsos positivos: %.2f%%\n",
               (double) g_falsos_positivos / g_consultas_total * 100.0);
        
        // Tempo médio de consulta = tempo total / número de consultas
        printf("Tempo medio de consulta : %.6f s\n",
               g_tempo_total_seg / g_consultas_total);
    }
}


/* ---------------------------------------------------------------
 * RF04 — INSERCAO EM LOTE
 * --------------------------------------------------------------- */
void inserir_lote(TabHash *tabela, filtrobloom *bloom) {
    char nome_arquivo[100];  // Armazena o caminho do arquivo
    char linha[50];          // Armazena cada linha lida do arquivo
    int inseridos = 0, duplicados = 0, invalidos = 0;  // Contadores

    printf("\n=== INSERIR EM LOTE ===\n");
    printf("Digite o caminho do arquivo: ");
    fgets(nome_arquivo, sizeof(nome_arquivo), stdin);
    nome_arquivo[strcspn(nome_arquivo, "\n")] = '\0';  // Remove quebra de linha

    // Tenta abrir o arquivo no modo leitura
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL) {  // Se não encontrar o arquivo
        printf("ERRO: Nao foi possivel abrir '%s'\n", nome_arquivo);
        return;
    }

    printf("Lendo arquivo...\n");

    // Lê cada palavra do arquivo (limite de 49 caracteres)
    while (fscanf(arquivo, "%49s", linha) != EOF) {
        linha[strcspn(linha, "\r\n")] = '\0';  // Remove caracteres especiais

        // Verifica se o ID tem 8 letras + 3 numeros
        if (!validar_formato(linha)) {
            invalidos++;  // Conta IDs com formato errado
            continue;     // Pula para a proxima linha
        }

        // Verifica se o usuario ja existe na tabela
        if (h_bus(tabela, linha) == 0) {  // 0 = nao encontrado
            if (h_ins(tabela, linha) == 1) {  // Tenta inserir na Hash
                inserir(bloom, linha);  // Insere tambem no Bloom
                inseridos++;  // Conta inserção bem-sucedida
            }
        } else {
            duplicados++;  // Conta usuarios repetidos
        }
    }

    fclose(arquivo);  // Fecha o arquivo

    // Exibe resumo da operação
    printf("\n=== RESULTADO DA CARGA ===\n");
    printf("Usuarios inseridos        : %d\n", inseridos);
    printf("Duplicados ignorados      : %d\n", duplicados);
    printf("Linhas invalidas ignoradas: %d\n", invalidos);
    printf("Total no sistema          : %d\n", tabela->q_elem);
}

/* ---------------------------------------------------------------
 * PARTE 3 — EXPERIMENTOS
 *
 * CORRECOES aplicadas:
 *  1. Bloom e tabela sao recriados/reiniciados entre cenarios para
 *     garantir isolamento completo dos resultados.
 *  2. Ausentes sao gerados com sufixo "999", que nunca e produzido
 *     por gerar_usuario, evitando falsos positivos inflados.
 *  3. bloom e passado como double pointer (**bloom_ptr) para que
 *     a recriacao local reflita no main() corretamente.
 * --------------------------------------------------------------- */
void executar_experimentos(TabHash *tabela, filtrobloom **bloom_ptr) {
    int tamanhos[] = {1000, 10000, 100000};  // Quantidades de registros para testar
    int t, i;

    printf("\n========================================\n");
    printf("           EXPERIMENTOS\n");
    printf("========================================\n");

    // Loop pelos 3 tamanhos de teste
    for (t = 0; t < 3; t++) {
        int n = tamanhos[t];
        clock_t inicio, fim;
        double  tempo_sem_bloom, tempo_com_bloom;
        int     consultas_hash = 0;  // Consultas que chegaram na Hash
        int     falsos_exp     = 0;  // Falsos positivos encontrados

        printf("\n----------------------------------------\n");
        printf("TESTE COM %d REGISTROS\n", n);
        printf("----------------------------------------\n");

        /* Reinicia estruturas para isolamento do cenario */
        h_lib(tabela);                 // Limpa a Tabela Hash
        h_ini(tabela);                 // Reinicia a Tabela Hash
        destruir(*bloom_ptr);          // Libera o Bloom antigo
        *bloom_ptr = criar((size_t) n, 0.01);  // Cria novo Bloom com 1% de falso positivo
        if (*bloom_ptr == NULL) {
            printf("ERRO: Falha ao criar Filtro de Bloom!\n");
            return;
        }
        filtrobloom *bloom = *bloom_ptr;

        /* Gera e insere n usuarios sem duplicatas */
        char **usuarios = (char **) malloc(n * sizeof(char *));
        if (usuarios == NULL) { printf("ERRO: sem memoria\n"); return; }

        printf("[1] Inserindo %d usuarios...\n", n);
        for (i = 0; i < n; i++) {
            usuarios[i] = (char *) malloc(12 * sizeof(char));
            if (usuarios[i] == NULL) { printf("ERRO: sem memoria\n"); return; }
            do {
                gerar_usuario(usuarios[i]);  // Gera ID aleatorio
            } while (h_bus(tabela, usuarios[i]) == 1); /* garante sem duplicata */
            h_ins(tabela, usuarios[i]);      // Insere na Hash
            inserir(bloom, usuarios[i]);     // Insere no Bloom
        }
        printf("    %d usuarios inseridos.\n", tabela->q_elem);

        /* Tempo SEM Bloom: busca todos direto na Hash */
        printf("[2] Medindo tempo SEM Bloom...\n");
        inicio = clock();
        for (i = 0; i < n; i++)
            h_bus(tabela, usuarios[i]);      // Consulta direto na Hash (SEM Bloom)
        fim = clock();
        tempo_sem_bloom = (double)(fim - inicio) / CLOCKS_PER_SEC;

        /* Tempo COM Bloom: filtra pelo Bloom antes de ir a Hash */
        printf("[3] Medindo tempo COM Bloom...\n");
        inicio = clock();
        consultas_hash = 0;
        for (i = 0; i < n; i++) {
            if (consultar(bloom, usuarios[i])) {  // Bloom diz "possivelmente existe"
                h_bus(tabela, usuarios[i]);      // Só consulta a Hash se Bloom disser que existe
                consultas_hash++;                // Conta quantas consultas chegaram na Hash
            }
        }
        fim = clock();
        tempo_com_bloom = (double)(fim - inicio) / CLOCKS_PER_SEC;

        /* Falsos positivos com ausentes garantidos (sufixo "999") */
        printf("[4] Medindo falsos positivos...\n");
        falsos_exp = 0;
        for (i = 0; i < n; i++) {
            char ausente[12];
            gerar_usuario(ausente);                // Gera ID aleatorio
            ausente[8] = '9'; ausente[9] = '9'; ausente[10] = '9';  // Força numeros 999 (garante que nao existe)
            if (h_bus(tabela, ausente) == 0 && consultar(bloom, ausente))
                falsos_exp++;                      // Bloom disse que existe, mas nao esta na Hash = falso positivo
        }

        /* Exibe resultados */
        printf("\nRESULTADOS PARA %d REGISTROS:\n", n);
        printf("  Tempo SEM Bloom        : %.6f s\n", tempo_sem_bloom);
        printf("  Tempo COM Bloom        : %.6f s\n", tempo_com_bloom);
        if (tempo_com_bloom > 0.0)
            printf("  Ganho de desempenho    : %.2fx\n",
                   tempo_sem_bloom / tempo_com_bloom);
        else
            printf("  Ganho de desempenho    : (tempo muito pequeno para medir)\n");
        printf("  Consultas na Hash      : %d / %d (%.1f%%)\n",
               consultas_hash, n, (double) consultas_hash / n * 100.0);
        printf("  Falsos positivos       : %d (%.2f%%)\n",
               falsos_exp, (double) falsos_exp / n * 100.0);
        printf("  Fator de carga (alpha) : %.6f\n",
               (double) tabela->q_elem / TAM_TAB);

        // Libera memoria dos usuarios gerados
        for (i = 0; i < n; i++)
            free(usuarios[i]);
        free(usuarios);
    }

    printf("\n========================================\n");
    printf("        EXPERIMENTOS CONCLUIDOS!\n");
    printf("========================================\n");
}

/* ---------------------------------------------------------------
 * MENU PRINCIPAL
 * --------------------------------------------------------------- */
void mostrar_menu(void) {
    printf("\n========================================\n");
    printf("   SISTEMA DE CONSULTA DE USUARIOS\n");
    printf("========================================\n");
    printf("  1. INSERIR usuario (manual)\n");
    printf("  2. CONSULTAR usuario\n");
    printf("  3. EXIBIR estatisticas\n");
    printf("  4. INSERIR em lote (arquivo)\n");
    printf("  5. EXECUTAR experimentos\n");
    printf("  0. SAIR\n");
    printf("========================================\n");
    printf("Escolha uma opcao: ");
}

/* ---------------------------------------------------------------
 * FUNCAO PRINCIPAL
 * --------------------------------------------------------------- */
int main(void) {
    TabHash      tabela;        // Tabela Hash para armazenamento exato
    filtrobloom *bloom;         // Filtro de Bloom para consultas rápidas
    int          opcao;         // Opção escolhida no menu

    // Inicializa o gerador de números aleatórios com o tempo atual
    // Isso garante que os IDs gerados sejam diferentes a cada execução
    srand((unsigned int) time(NULL));

    // Cabeçalho de inicialização
    printf("\n========================================\n");
    printf("        INICIALIZANDO SISTEMA\n");
    printf("========================================\n");

    // Inicializa a Tabela Hash com todas as posições vazias
    h_ini(&tabela);
    printf("  Tabela Hash inicializada (tamanho: %d)\n", TAM_TAB);

    // Cria o Filtro de Bloom para 100.000 elementos com 1% de falsos positivos
    bloom = criar(100000, 0.01);
    if (bloom == NULL) {  // Verifica se a alocação foi bem-sucedida
        printf("  ERRO: Falha ao criar Filtro de Bloom!\n");
        return 1;  // Sai do programa com código de erro
    }
    // Exibe informações do Filtro de Bloom criado
    printf("  Filtro de Bloom criado\n");
    printf("    - Tamanho : %zu bits (%.2f KB)\n",
           bloom->tam_bits, (double) bloom->tam_bits / 8.0 / 1024.0);
    printf("    - k (hashs): %zu\n", bloom->num_hash);
    printf("\n  Sistema pronto!\n");

    // Loop principal do programa (executa até o usuário escolher 0)
    do {
        mostrar_menu();  // Exibe as opções disponíveis
        if (scanf("%d", &opcao) != 1) opcao = -1;  // Se entrada inválida, define -1
        limpar_buffer();  // Remove caracteres residuais do buffer de entrada

        // Executa a opção escolhida pelo usuário
        switch (opcao) {
            case 1: inserir_manual(&tabela, bloom);          break;  // RF01
            case 2: consultar_usuario(&tabela, bloom);       break;  // RF02
            case 3: exibir_estatisticas(&tabela, bloom);     break;  // RF03
            case 4: inserir_lote(&tabela, bloom);            break;  // RF04
            case 5: executar_experimentos(&tabela, &bloom);  break;  // Experimentos (Parte 3)
            case 0:  // Sair do programa
                printf("\n========================================\n");
                printf("          SAINDO DO SISTEMA\n");
                printf("========================================\n");
                break;
            default:  // Opção inválida
                printf("  ERRO: Opcao invalida!\n");
        }
    } while (opcao != 0);  // Continua enquanto não escolher 0

    // Libera toda a memória alocada antes de encerrar
    h_lib(&tabela);     // Libera a Tabela Hash
    destruir(bloom);    // Libera o Filtro de Bloom
    printf("\nMemoria liberada.\n");

    return 0;  // Retorna sucesso
}