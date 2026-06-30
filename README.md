# Sistema de Verificação de Cadastro com Tabela Hash e Filtro de Bloom

Repositório do trabalho da **3ª Avaliação** da disciplina **Laboratório de Algoritmos e Estruturas de Dados II**.

## Descrição

Este projeto implementa um sistema para armazenamento e consulta de usuários utilizando duas estruturas de dados complementares:

- **Tabela Hash**: responsável pelo armazenamento e recuperação exata dos usuários cadastrados.
- **Filtro de Bloom**: estrutura probabilística utilizada como pré-filtro, evitando consultas desnecessárias à Tabela Hash.

O sistema também realiza experimentos para avaliar o desempenho, o consumo de memória e a taxa de falsos positivos do Filtro de Bloom em cenários com 1.000, 10.000 e 100.000 registros.

---

## Funcionalidades

- **RF01** — Inserção manual de usuário com validação de formato;
- **RF02** — Consulta com fluxo obrigatório: Bloom → Hash;
- **RF03** — Exibição de estatísticas: total de consultas, consultas evitadas pelo Bloom, falsos positivos, taxa percentual e tempo médio de consulta;
- **RF04** — Inserção em lote a partir de arquivo texto;
- **Experimentos** — Testes automatizados com 1.000, 10.000 e 100.000 registros, medindo tempo com e sem Bloom e taxa de falsos positivos.

---

## Estrutura do Projeto

```tex
Projeto/
│
├── data/
│   └── usuarios.txt
│
├── src/
│   ├── main.c
│   ├── hash.c
│   ├── hash.h
│   ├── bloom.c
│   ├── bloom.h
│   └── sistema.exe
│
├── testes/
|__ Relatorio/
│
└── README.md
```

---

## Requisitos

- Compilador GCC
- Biblioteca matemática (`math.h`)

---

## Compilação

Acesse a pasta `src`:

```bash
cd src
```

Linux:

```bash
gcc main.c hash.c bloom.c -o sistema -lm
```

Windows:

```bash
gcc main.c hash.c bloom.c -o sistema.exe -lm
```

---

## Execução

Ainda na pasta `src`:

Linux:

```bash
./sistema
```

Windows:

```bash
sistema.exe
```

Ao iniciar, o sistema exibe:

```
========================================
        INICIALIZANDO SISTEMA
========================================
  Tabela Hash inicializada (tamanho: 150001)
  Filtro de Bloom criado
    - Tamanho : 958506 bits (117.49 KB)
    - k (hashs): 7

  Sistema pronto!

========================================
   SISTEMA DE CONSULTA DE USUARIOS
========================================
  1. INSERIR usuario (manual)
  2. CONSULTAR usuario
  3. EXIBIR estatisticas
  4. INSERIR em lote (arquivo)
  5. EXECUTAR experimentos
  0. SAIR
========================================
```

---

## Exemplos de Uso

### Inserção manual

```
Escolha uma opcao: 1

=== INSERIR USUARIO ===
Digite o ID (formato: 8letras + 3numeros): joaosilv123
SUCESSO: Usuario 'joaosilv123' cadastrado!
```

### Consulta

```
Escolha uma opcao: 2

=== CONSULTAR USUARIO ===
Digite o ID do usuario: joaosilv123
RESULTADO: Usuario 'joaosilv123' ENCONTRADO
Tempo: 0.000001 s
```

```
Digite o ID do usuario: xyzxyzxy999
RESULTADO: Usuario 'xyzxyzxy999' INEXISTENTE (Bloom barrou)
Tempo: 0.000001 s
```

### Estatísticas

```
Escolha uma opcao: 3

=== ESTATISTICAS DO SISTEMA ===

--- TABELA HASH ---
Elementos armazenados : 15
Tamanho da tabela     : 150001
Colisoes ocorridas    : 0
Fator de carga (alpha): 0.000100

--- FILTRO DE BLOOM ---
Tamanho do vetor de bits: 958506 bits (117.49 KB)
Numero de funcoes hash  : 7
Memoria utilizada       : 119814 bytes

--- CONSULTAS NA SESSAO ---
Total de consultas      : 2
Evitadas pelo Bloom     : 1
Falsos positivos        : 0
Taxa de falsos positivos: 0.00%
Tempo medio de consulta : 0.000001 s
```

---

## Inserção em Lote

A opção **4 – Inserir em lote** carrega usuários a partir de um arquivo texto.

Como o programa é executado dentro da pasta `src`, informe o caminho relativo até a pasta `data`:

Windows:

```text
..\data\usuarios.txt
```

Linux:

```text
../data/usuarios.txt
```

Cada linha deve conter um único usuário no formato `8 letras minúsculas + 3 dígitos`:

```text
abcdefgh123
ijklmnop456
qrstuvwx789
```

Linhas fora desse formato são ignoradas e contabilizadas separadamente.

---

## Implementação

### Tabela Hash

- Função hash **DJB2** (`hash = hash * 33 + c`);
- Tratamento de colisões por **encadeamento externo** (listas ligadas);
- Tamanho da tabela: **150.001** posições (número primo), garantindo fator de carga ≈ 0,67 para até 100.000 elementos;
- Operações: inserção, busca e liberação de memória.

### Filtro de Bloom

- Vetor de bits alocado dinamicamente;
- **Double Hashing** com as funções DJB2 e SDBM;
- Tamanho do vetor (`m`) e número de funções hash (`k`) calculados automaticamente pelas fórmulas:

```
m = -(n × ln(p)) / (ln(2))²
k = (m / n) × ln(2)
```

Para n = 100.000 elementos e p = 1% de falsos positivos, resulta em m ≈ 958.506 bits e k = 7 funções hash.

---

## Tecnologias Utilizadas

- Linguagem C
- Tabela Hash com encadeamento externo
- Lista Encadeada
- Filtro de Bloom com Double Hashing

---

## Integrantes

- Eryk de Freitas Alves
- José Rafael Dias Pessoa
- Raquel Cavalcante da Silva
