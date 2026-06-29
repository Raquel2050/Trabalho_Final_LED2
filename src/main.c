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
    const char letras[] = "abcdefghijklmnopqrstuvwxyz";
    int i;
    for (i = 0; i < 8; i++)
        buffer[i] = letras[rand() % 26];
    for (i = 0; i < 3; i++)
        buffer[8 + i] = '0' + (rand() % 10);
    buffer[11] = '\0';
}

/* Valida se o ID tem exatamente 8 letras minusculas + 3 digitos.
 * Retorna 1 se valido, 0 caso contrario.
 */
int validar_formato(const char *id) {
    int i;
    if (strlen(id) != 11)
        return 0;
    for (i = 0; i < 8; i++)
        if (!islower((unsigned char) id[i]))
            return 0;
    for (i = 8; i < 11; i++)
        if (!isdigit((unsigned char) id[i]))
            return 0;
    return 1;
}

/* ---------------------------------------------------------------
 * RF01 — INSERCAO MANUAL
 * --------------------------------------------------------------- */
void inserir_manual(TabHash *tabela, filtrobloom *bloom) {
    char buffer[100];

    printf("\n=== INSERIR USUARIO ===\n");
    printf("Digite o ID (formato: 8letras + 3numeros): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';

    if (!validar_formato(buffer)) {
        printf("ERRO: ID invalido. Use exatamente 8 letras minusculas + 3 digitos.\n");
        return;
    }

    if (h_bus(tabela, buffer) == 1) {
        printf("ERRO: Usuario '%s' ja esta cadastrado!\n", buffer);
        return;
    }

    if (h_ins(tabela, buffer) == 1) {
        inserir(bloom, buffer);
        printf("SUCESSO: Usuario '%s' cadastrado!\n", buffer);
    } else {
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
    char buffer[100];
    clock_t inicio, fim;
    double tempo;

    printf("\n=== CONSULTAR USUARIO ===\n");
    printf("Digite o ID do usuario: ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';

    inicio = clock();
    g_consultas_total++;

    /* Passo 1: Filtro de Bloom */
    if (!consultar(bloom, buffer)) {
        fim   = clock();
        tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
        g_evitadas_bloom++;
        g_tempo_total_seg += tempo;
        printf("RESULTADO: Usuario '%s' INEXISTENTE (Bloom barrou)\n", buffer);
        printf("Tempo: %.6f s\n", tempo);
        return;
    }

    /* Passo 2: Bloom disse "possivelmente existe" — consulta Hash */
    if (h_bus(tabela, buffer) == 1) {
        fim   = clock();
        tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
        g_tempo_total_seg += tempo;
        printf("RESULTADO: Usuario '%s' ENCONTRADO\n", buffer);
        printf("Tempo: %.6f s\n", tempo);
    } else {
        /* Bloom disse sim, Hash disse nao -> falso positivo */
        fim   = clock();
        tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
        g_falsos_positivos++;
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

    printf("\n--- TABELA HASH ---\n");
    printf("Elementos armazenados : %d\n",   tabela->q_elem);
    printf("Tamanho da tabela     : %d\n",   TAM_TAB);
    printf("Colisoes ocorridas    : %d\n",   tabela->colis);
    printf("Fator de carga (alpha): %.6f\n", (double) tabela->q_elem / TAM_TAB);
    if (tabela->q_elem > 0)
        printf("Percentual de colisoes: %.2f%%\n",
               (double) tabela->colis / tabela->q_elem * 100.0);

    printf("\n--- FILTRO DE BLOOM ---\n");
    printf("Tamanho do vetor de bits: %zu bits (%.2f KB)\n",
           bloom->tam_bits, (double) bloom->tam_bits / 8.0 / 1024.0);
    printf("Numero de funcoes hash  : %zu\n", bloom->num_hash);
    printf("Memoria utilizada       : %zu bytes\n", (bloom->tam_bits + 7) / 8);

    printf("\n--- CONSULTAS NA SESSAO ---\n");
    printf("Total de consultas      : %ld\n", g_consultas_total);
    printf("Evitadas pelo Bloom     : %ld\n", g_evitadas_bloom);
    printf("Falsos positivos        : %ld\n", g_falsos_positivos);
    if (g_consultas_total > 0) {
        printf("Taxa de falsos positivos: %.2f%%\n",
               (double) g_falsos_positivos / g_consultas_total * 100.0);
        printf("Tempo medio de consulta : %.6f s\n",
               g_tempo_total_seg / g_consultas_total);
    }
}

/* ---------------------------------------------------------------
 * RF04 — INSERCAO EM LOTE
 * --------------------------------------------------------------- */
void inserir_lote(TabHash *tabela, filtrobloom *bloom) {
    char nome_arquivo[100];
    char linha[50];
    int inseridos = 0, duplicados = 0, invalidos = 0;

    printf("\n=== INSERIR EM LOTE ===\n");
    printf("Digite o caminho do arquivo: ");
    fgets(nome_arquivo, sizeof(nome_arquivo), stdin);
    nome_arquivo[strcspn(nome_arquivo, "\n")] = '\0';

    FILE *arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL) {
        printf("ERRO: Nao foi possivel abrir '%s'\n", nome_arquivo);
        return;
    }

    printf("Lendo arquivo...\n");

    while (fscanf(arquivo, "%49s", linha) != EOF) {
        linha[strcspn(linha, "\r\n")] = '\0';

        /* Valida formato: 8 letras minusculas + 3 digitos */
        if (!validar_formato(linha)) {
            invalidos++;
            continue;
        }

        if (h_bus(tabela, linha) == 0) {
            if (h_ins(tabela, linha) == 1) {
                inserir(bloom, linha);
                inseridos++;
            }
        } else {
            duplicados++;
        }
    }

    fclose(arquivo);

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
    int tamanhos[] = {1000, 10000, 100000};
    int t, i;

    printf("\n========================================\n");
    printf("           EXPERIMENTOS\n");
    printf("========================================\n");

    for (t = 0; t < 3; t++) {
        int n = tamanhos[t];
        clock_t inicio, fim;
        double  tempo_sem_bloom, tempo_com_bloom;
        int     consultas_hash = 0;
        int     falsos_exp     = 0;

        printf("\n----------------------------------------\n");
        printf("TESTE COM %d REGISTROS\n", n);
        printf("----------------------------------------\n");

        /* Reinicia estruturas para isolamento do cenario */
        h_lib(tabela);
        h_ini(tabela);
        destruir(*bloom_ptr);
        *bloom_ptr = criar((size_t) n, 0.01);
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
                gerar_usuario(usuarios[i]);
            } while (h_bus(tabela, usuarios[i]) == 1); /* garante sem duplicata */
            h_ins(tabela, usuarios[i]);
            inserir(bloom, usuarios[i]);
        }
        printf("    %d usuarios inseridos.\n", tabela->q_elem);

        /* Tempo SEM Bloom: busca todos direto na Hash */
        printf("[2] Medindo tempo SEM Bloom...\n");
        inicio = clock();
        for (i = 0; i < n; i++)
            h_bus(tabela, usuarios[i]);
        fim = clock();
        tempo_sem_bloom = (double)(fim - inicio) / CLOCKS_PER_SEC;

        /* Tempo COM Bloom: filtra pelo Bloom antes de ir a Hash */
        printf("[3] Medindo tempo COM Bloom...\n");
        inicio = clock();
        consultas_hash = 0;
        for (i = 0; i < n; i++) {
            if (consultar(bloom, usuarios[i])) {
                h_bus(tabela, usuarios[i]);
                consultas_hash++;
            }
        }
        fim = clock();
        tempo_com_bloom = (double)(fim - inicio) / CLOCKS_PER_SEC;

        /* Falsos positivos com ausentes garantidos (sufixo "999") */
        printf("[4] Medindo falsos positivos...\n");
        falsos_exp = 0;
        for (i = 0; i < n; i++) {
            char ausente[12];
            gerar_usuario(ausente);
            ausente[8] = '9'; ausente[9] = '9'; ausente[10] = '9';
            if (h_bus(tabela, ausente) == 0 && consultar(bloom, ausente))
                falsos_exp++;
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
    TabHash      tabela;
    filtrobloom *bloom;
    int          opcao;

    srand((unsigned int) time(NULL));

    printf("\n========================================\n");
    printf("        INICIALIZANDO SISTEMA\n");
    printf("========================================\n");

    h_ini(&tabela);
    printf("  Tabela Hash inicializada (tamanho: %d)\n", TAM_TAB);

    bloom = criar(100000, 0.01);
    if (bloom == NULL) {
        printf("  ERRO: Falha ao criar Filtro de Bloom!\n");
        return 1;
    }
    printf("  Filtro de Bloom criado\n");
    printf("    - Tamanho : %zu bits (%.2f KB)\n",
           bloom->tam_bits, (double) bloom->tam_bits / 8.0 / 1024.0);
    printf("    - k (hashs): %zu\n", bloom->num_hash);
    printf("\n  Sistema pronto!\n");

    do {
        mostrar_menu();
        if (scanf("%d", &opcao) != 1) opcao = -1;
        limpar_buffer();

        switch (opcao) {
            case 1: inserir_manual(&tabela, bloom);          break;
            case 2: consultar_usuario(&tabela, bloom);       break;
            case 3: exibir_estatisticas(&tabela, bloom);     break;
            case 4: inserir_lote(&tabela, bloom);            break;
            case 5: executar_experimentos(&tabela, &bloom);  break;
            case 0:
                printf("\n========================================\n");
                printf("          SAINDO DO SISTEMA\n");
                printf("========================================\n");
                break;
            default:
                printf("  ERRO: Opcao invalida!\n");
        }
    } while (opcao != 0);

    h_lib(&tabela);
    destruir(bloom);
    printf("\nMemoria liberada.\n");

    return 0;
}