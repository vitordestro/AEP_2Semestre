/*
 * ============================================================================
 *  PSE em Ação — Sistema de Planejamento e Acompanhamento de Ações de Saúde
 *  na Escola (AEP 2º Semestre 2026 — UniCesumar)
 *
 *  Integrantes: Gabriel Francisco de Almeida Bezerra
 *               Leonardo dos Santos Silva
 *               Vitor dos Santos Destro
 *
 *  Compilar:  gcc -Wall -Wextra -std=c99 -o pse_em_acao pse_em_acao.c
 *  Executar:  ./pse_em_acao        (Linux/macOS)
 *             pse_em_acao.exe      (Windows)
 *
 *  Contexto: cada "ação" cadastrada é um CARDÁPIO SEMANAL (tema: alimentação
 *  saudável) com 5 dias úteis. Cada dia tem um prato-base, opções de proteína
 *  e opções de arroz (nome + calorias). O sistema guarda apenas CONTAGENS
 *  AGREGADAS de participantes por opção, nunca dados individuais de alunos
 *  (privacidade — ver seção 6.1 do documento).
 *
 *  Armazenamento: somente em memória (vetor de structs), conforme os
 *  requisitos técnicos mínimos da AEP.
 *
 *  Obs.: as mensagens do terminal estão sem acentos de propósito, para evitar
 *  caracteres quebrados em terminais Windows (requisito RNF04 - portabilidade).
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ----------------------------- CONSTANTES -------------------------------- */

#define MAX_ACOES      50   /* limite de armazenamento (vetor em memória)      */
#define DIAS_SEMANA     5   /* segunda a sexta                                 */
#define MAX_OPCOES      4   /* máx. de opções de proteína e de arroz por dia   */
#define TAM_TEXTO      50   /* tamanho padrão dos campos de texto              */
#define TAM_CODIGO     16
#define TAM_DATA       11   /* "dd/mm/aaaa" + '\0'                             */
#define MAX_PARTICIP 100000 /* teto razoável para validar contagens            */

/* Situação de cada dia do cardápio. Valores 1..3 coincidem com o menu. */
enum Situacao { PLANEJADA = 1, REALIZADA = 2, CANCELADA = 3 };

static const char *NOMES_DIAS[DIAS_SEMANA] = {
    "Segunda", "Terca", "Quarta", "Quinta", "Sexta"
};

/* ------------------------------ STRUCTS ---------------------------------- */

/* Uma opção do cardápio (ex.: "Frango grelhado", 165 kcal) e quantos
 * participantes a escolheram. É só um número agregado, sem identificar alunos. */
typedef struct {
    char nome[TAM_TEXTO];
    int  calorias;
    int  participantes;
} Opcao;

/* Um dia do cardápio semanal. */
typedef struct {
    char  prato_base[TAM_TEXTO];
    Opcao proteinas[MAX_OPCOES];
    int   qtd_proteinas;
    Opcao arroz[MAX_OPCOES];
    int   qtd_arroz;
    int   situacao;              /* PLANEJADA, REALIZADA ou CANCELADA          */
    int   total_participantes;   /* preenchido quando o dia é REALIZADO        */
} Dia;

/* Uma ação do PSE (aqui, um cardápio semanal completo). */
typedef struct {
    char codigo[TAM_CODIGO];
    char escola[TAM_TEXTO];
    char tema[TAM_TEXTO];
    char responsavel[TAM_TEXTO];
    char publico_alvo[TAM_TEXTO];
    char data_prevista[TAM_DATA];  /* data de início da semana (dd/mm/aaaa)   */
    int  qtd_prevista;             /* participantes previstos POR DIA         */
    Dia  dias[DIAS_SEMANA];
} Acao;

/* ------------------------ PROTÓTIPOS DAS FUNÇÕES ------------------------- */

/* Entrada de dados e validação */
static int  lerLinha(char *destino, int tamanho);
static void lerTexto(const char *mensagem, char *destino, int tamanho);
static int  lerInteiro(const char *mensagem, int minimo, int maximo);
static void lerData(const char *mensagem, char *destino);
static int  dataValida(const char *data);

/* Utilitários */
static int  iguaisIgnorandoCaso(const char *a, const char *b);
static int  contemIgnorandoCaso(const char *texto, const char *trecho);
static const char *nomeSituacao(int situacao);
static int  buscarPorCodigo(const Acao *acoes, int total, const char *codigo);
static int  possuiDiaRealizado(const Acao *acao);

/* Exibição */
static void exibirMenu(void);
static void imprimirAcao(const Acao *acao);

/* Funcionalidades (uma função por requisito funcional) */
static void cadastrarAcao(Acao *acoes, int *total);              /* RF01 */
static void listarAcoes(const Acao *acoes, int total);           /* RF02 */
static void pesquisarAcoes(const Acao *acoes, int total);        /* RF03 */
static void atualizarSituacao(Acao *acoes, int total);           /* RF04 */
static void gerarResumo(const Acao *acoes, int total);           /* RF05 */
static void alterarAcao(Acao *acoes, int total);                 /* RF06 */

/* Auxiliares das funcionalidades */
static void lerOpcoes(const char *titulo, Opcao *lista, int *quantidade);
static void lerCardapioDia(Dia *dia, const char *nomeDia);
static void registrarParticipacao(Dia *dia);
static int  selecionarAcao(const Acao *acoes, int total);

/* ========================================================================== */
/*                                    MAIN                                    */
/* ========================================================================== */

int main(void) {
    /* 'static' evita colocar ~100 KB na pilha; zera o vetor automaticamente. */
    static Acao acoes[MAX_ACOES];
    int total = 0;     /* quantidade de ações já cadastradas */
    int opcao;

    printf("=== PSE em Acao ===\n");
    printf("Planejamento e acompanhamento de acoes de saude na escola\n");

    /* Menu principal controlado por laço de repetição (requisito técnico):
     * repete até o usuário escolher 0 (Sair). */
    do {
        exibirMenu();
        opcao = lerInteiro("Escolha uma opcao: ", 0, 6);  /* valida opção do menu */

        switch (opcao) {
            case 1: cadastrarAcao(acoes, &total);       break;
            case 2: listarAcoes(acoes, total);          break;
            case 3: pesquisarAcoes(acoes, total);       break;
            case 4: atualizarSituacao(acoes, total);    break;
            case 5: gerarResumo(acoes, total);          break;
            case 6: alterarAcao(acoes, total);          break;
            case 0: printf("\nEncerrando o sistema. Ate logo!\n"); break;
        }
    } while (opcao != 0);

    return 0;
}

/* ========================================================================== */
/*                     ENTRADA DE DADOS E VALIDAÇÕES (RNF02)                  */
/* ========================================================================== */

/*
 * Lê uma linha inteira do teclado com fgets (mais seguro que scanf, pois não
 * deixa "lixo" no buffer e evita estouro de memória).
 * - Remove o '\n' final e espaços nas pontas.
 * - Se a linha digitada for maior que o buffer, descarta o excedente.
 * - Se a entrada acabar (EOF, ex.: Ctrl+D), encerra o programa sem travar.
 * Retorna 1 em caso de sucesso.
 */
static int lerLinha(char *destino, int tamanho) {
    if (fgets(destino, tamanho, stdin) == NULL) {
        printf("\nEntrada encerrada. Finalizando o programa.\n");
        exit(0);
    }

    size_t len = strlen(destino);
    if (len > 0 && destino[len - 1] == '\n') {
        destino[--len] = '\0';
    } else {
        /* linha maior que o buffer: descarta o que sobrou até o '\n' */
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }

    /* remove espaços do final */
    while (len > 0 && isspace((unsigned char)destino[len - 1])) {
        destino[--len] = '\0';
    }
    /* remove espaços do início */
    size_t inicio = 0;
    while (destino[inicio] != '\0' && isspace((unsigned char)destino[inicio])) {
        inicio++;
    }
    if (inicio > 0) {
        memmove(destino, destino + inicio, strlen(destino + inicio) + 1);
    }
    return 1;
}

/*
 * Lê um texto OBRIGATÓRIO: repete a pergunta enquanto o campo estiver vazio
 * (validação de "campos obrigatórios vazios").
 */
static void lerTexto(const char *mensagem, char *destino, int tamanho) {
    while (1) {
        printf("%s", mensagem);
        lerLinha(destino, tamanho);
        if (destino[0] != '\0') {
            return;
        }
        printf("  [ERRO] Este campo e obrigatorio e nao pode ficar vazio.\n");
    }
}

/*
 * Lê um número inteiro dentro de [minimo, maximo].
 * Rejeita letras, números decimais, campo vazio e valores fora do intervalo
 * (inclui valores negativos quando minimo >= 0).
 */
static int lerInteiro(const char *mensagem, int minimo, int maximo) {
    char linha[64];
    char *fim;
    long valor;

    while (1) {
        printf("%s", mensagem);
        lerLinha(linha, sizeof(linha));

        if (linha[0] == '\0') {
            printf("  [ERRO] Digite um numero.\n");
            continue;
        }
        valor = strtol(linha, &fim, 10);
        if (*fim != '\0') {   /* sobrou algo que não é número */
            printf("  [ERRO] Entrada invalida. Digite apenas numeros inteiros.\n");
            continue;
        }
        if (valor < minimo || valor > maximo) {
            printf("  [ERRO] Valor fora do permitido (de %d a %d).\n", minimo, maximo);
            continue;
        }
        return (int)valor;
    }
}

/* Verifica se a string está no formato dd/mm/aaaa e se a data existe. */
static int dataValida(const char *data) {
    int i, dia, mes, ano;
    int dias_no_mes[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (strlen(data) != 10 || data[2] != '/' || data[5] != '/') {
        return 0;
    }
    for (i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue;
        if (!isdigit((unsigned char)data[i])) return 0;
    }
    dia = (data[0] - '0') * 10 + (data[1] - '0');
    mes = (data[3] - '0') * 10 + (data[4] - '0');
    ano = (data[6] - '0') * 1000 + (data[7] - '0') * 100 +
          (data[8] - '0') * 10 + (data[9] - '0');

    if (ano < 2000 || ano > 2100 || mes < 1 || mes > 12) return 0;

    /* ano bissexto: fevereiro passa a ter 29 dias */
    if ((ano % 4 == 0 && ano % 100 != 0) || ano % 400 == 0) {
        dias_no_mes[1] = 29;
    }
    return dia >= 1 && dia <= dias_no_mes[mes - 1];
}

/* Lê uma data até que seja válida. */
static void lerData(const char *mensagem, char *destino) {
    while (1) {
        printf("%s", mensagem);
        lerLinha(destino, TAM_DATA);
        if (dataValida(destino)) {
            return;
        }
        printf("  [ERRO] Data invalida. Use o formato dd/mm/aaaa (ex.: 03/08/2026).\n");
    }
}

/* ========================================================================== */
/*                                 UTILITÁRIOS                                */
/* ========================================================================== */

/* Compara duas strings ignorando maiúsculas/minúsculas (sem usar funções
 * específicas de um sistema operacional, para manter a portabilidade). */
static int iguaisIgnorandoCaso(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

/* Verifica se 'trecho' aparece dentro de 'texto', ignorando maiúsculas. */
static int contemIgnorandoCaso(const char *texto, const char *trecho) {
    size_t i, j;
    size_t lt = strlen(texto), lr = strlen(trecho);

    if (lr == 0) return 0;
    if (lr > lt) return 0;

    for (i = 0; i + lr <= lt; i++) {
        for (j = 0; j < lr; j++) {
            if (tolower((unsigned char)texto[i + j]) !=
                tolower((unsigned char)trecho[j])) break;
        }
        if (j == lr) return 1;
    }
    return 0;
}

/* Converte o código numérico da situação em texto legível. */
static const char *nomeSituacao(int situacao) {
    switch (situacao) {
        case PLANEJADA: return "Planejada";
        case REALIZADA: return "Realizada";
        case CANCELADA: return "Cancelada";
        default:        return "Desconhecida";
    }
}

/* Procura uma ação pelo código. Retorna o índice no vetor ou -1 se não achar.
 * É usada tanto para impedir códigos duplicados quanto para localizar
 * registros (tratando "registro inexistente"). */
static int buscarPorCodigo(const Acao *acoes, int total, const char *codigo) {
    int i;
    for (i = 0; i < total; i++) {
        if (iguaisIgnorandoCaso(acoes[i].codigo, codigo)) {
            return i;
        }
    }
    return -1;
}

/* Retorna 1 se pelo menos um dia da ação já foi marcado como REALIZADO. */
static int possuiDiaRealizado(const Acao *acao) {
    int d;
    for (d = 0; d < DIAS_SEMANA; d++) {
        if (acao->dias[d].situacao == REALIZADA) return 1;
    }
    return 0;
}

/* ========================================================================== */
/*                                  EXIBIÇÃO                                  */
/* ========================================================================== */

static void exibirMenu(void) {
    printf("\n=========== MENU PRINCIPAL ===========\n");
    printf(" 1 - Cadastrar nova acao (cardapio semanal)\n");
    printf(" 2 - Listar todas as acoes\n");
    printf(" 3 - Pesquisar acao (codigo, escola ou tema)\n");
    printf(" 4 - Atualizar situacao de um dia / registrar participantes\n");
    printf(" 5 - Resumo geral\n");
    printf(" 6 - Alterar dados de uma acao\n");
    printf(" 0 - Sair\n");
    printf("======================================\n");
}

/* Mostra todos os dados de uma ação de forma organizada (RF02 / RF03). */
static void imprimirAcao(const Acao *acao) {
    int d, i;

    printf("\n--------------------------------------------------\n");
    printf("Codigo       : %s\n", acao->codigo);
    printf("Escola       : %s\n", acao->escola);
    printf("Tema         : %s\n", acao->tema);
    printf("Responsavel  : %s\n", acao->responsavel);
    printf("Publico-alvo : %s\n", acao->publico_alvo);
    printf("Inicio da semana: %s\n", acao->data_prevista);
    printf("Participantes previstos por dia: %d\n", acao->qtd_prevista);

    for (d = 0; d < DIAS_SEMANA; d++) {
        const Dia *dia = &acao->dias[d];
        printf("\n  [%s] Prato-base: %s | Situacao: %s\n",
               NOMES_DIAS[d], dia->prato_base, nomeSituacao(dia->situacao));

        printf("    Proteinas:\n");
        for (i = 0; i < dia->qtd_proteinas; i++) {
            printf("      - %-25s %4d kcal", dia->proteinas[i].nome,
                   dia->proteinas[i].calorias);
            if (dia->situacao == REALIZADA)
                printf("  -> %d participante(s)", dia->proteinas[i].participantes);
            printf("\n");
        }
        printf("    Arroz:\n");
        for (i = 0; i < dia->qtd_arroz; i++) {
            printf("      - %-25s %4d kcal", dia->arroz[i].nome,
                   dia->arroz[i].calorias);
            if (dia->situacao == REALIZADA)
                printf("  -> %d participante(s)", dia->arroz[i].participantes);
            printf("\n");
        }
        if (dia->situacao == REALIZADA)
            printf("    Total de participantes no dia: %d\n",
                   dia->total_participantes);
    }
    printf("--------------------------------------------------\n");
}

/* ========================================================================== */
/*                      RF01 — CADASTRO DE UMA NOVA AÇÃO                      */
/* ========================================================================== */

/*
 * Lê uma lista de opções (proteínas OU arroz) de um dia.
 * 'titulo' só serve para personalizar as mensagens. A quantidade de opções
 * é limitada por MAX_OPCOES para não estourar o vetor.
 */
static void lerOpcoes(const char *titulo, Opcao *lista, int *quantidade) {
    int i;
    char mensagem[80];

    sprintf(mensagem, "  Quantas opcoes de %s (1 a %d)? ", titulo, MAX_OPCOES);
    *quantidade = lerInteiro(mensagem, 1, MAX_OPCOES);

    for (i = 0; i < *quantidade; i++) {
        sprintf(mensagem, "    Nome da opcao de %s %d: ", titulo, i + 1);
        lerTexto(mensagem, lista[i].nome, TAM_TEXTO);

        sprintf(mensagem, "    Calorias (kcal) de \"%s\": ", lista[i].nome);
        lista[i].calorias = lerInteiro(mensagem, 0, 5000); /* sem negativos */

        lista[i].participantes = 0;  /* contagem começa zerada */
    }
}

/* Lê o cardápio de um único dia (prato-base + proteínas + arroz).
 * Também define a situação inicial do dia como PLANEJADA. */
static void lerCardapioDia(Dia *dia, const char *nomeDia) {
    printf("\n>> Cardapio de %s\n", nomeDia);
    lerTexto("  Prato-base: ", dia->prato_base, TAM_TEXTO);
    lerOpcoes("proteina", dia->proteinas, &dia->qtd_proteinas);
    lerOpcoes("arroz", dia->arroz, &dia->qtd_arroz);

    dia->situacao = PLANEJADA;
    dia->total_participantes = 0;
}

/*
 * Cadastra uma nova ação. Fluxo (ver fluxograma detalhado da função Cadastro):
 *   1. verifica se ainda há espaço no vetor;
 *   2. lê o código e rejeita duplicados;
 *   3. lê os dados gerais (todos obrigatórios);
 *   4. lê o cardápio dos 5 dias úteis;
 *   5. grava na posição *total e incrementa o contador.
 */
static void cadastrarAcao(Acao *acoes, int *total) {
    Acao nova;
    int d;

    printf("\n--- CADASTRO DE NOVA ACAO ---\n");

    /* Tratamento de "limite de armazenamento atingido" */
    if (*total >= MAX_ACOES) {
        printf("[ERRO] Limite de %d acoes atingido. Nao e possivel cadastrar mais.\n",
               MAX_ACOES);
        return;
    }

    memset(&nova, 0, sizeof(nova));  /* começa tudo zerado */

    /* Código: obrigatório e único */
    while (1) {
        lerTexto("Codigo da acao (ex.: ACAO01): ", nova.codigo, TAM_CODIGO);
        if (buscarPorCodigo(acoes, *total, nova.codigo) != -1) {
            printf("  [ERRO] Ja existe uma acao com o codigo \"%s\". Escolha outro.\n",
                   nova.codigo);
        } else {
            break;
        }
    }

    lerTexto("Escola: ", nova.escola, TAM_TEXTO);
    lerTexto("Tema (ex.: Alimentacao saudavel): ", nova.tema, TAM_TEXTO);
    lerTexto("Responsavel: ", nova.responsavel, TAM_TEXTO);
    lerTexto("Publico-alvo (ex.: 6o ano): ", nova.publico_alvo, TAM_TEXTO);
    lerData("Data prevista de inicio da semana (dd/mm/aaaa): ", nova.data_prevista);
    nova.qtd_prevista = lerInteiro("Quantidade prevista de participantes por dia: ",
                                   1, MAX_PARTICIP);

    /* Cardápio dos 5 dias úteis (laço de repetição + vetor) */
    for (d = 0; d < DIAS_SEMANA; d++) {
        lerCardapioDia(&nova.dias[d], NOMES_DIAS[d]);
    }

    acoes[*total] = nova;   /* copia a struct para o vetor principal */
    (*total)++;

    printf("\n[OK] Acao \"%s\" cadastrada com sucesso! (%d/%d)\n",
           nova.codigo, *total, MAX_ACOES);
}

/* ========================================================================== */
/*                         RF02 — LISTAGEM DAS AÇÕES                          */
/* ========================================================================== */

static void listarAcoes(const Acao *acoes, int total) {
    int i;

    printf("\n--- ACOES CADASTRADAS ---\n");
    if (total == 0) {
        printf("Nenhuma acao cadastrada ainda.\n");
        return;
    }
    for (i = 0; i < total; i++) {
        imprimirAcao(&acoes[i]);
    }
    printf("\nTotal: %d acao(oes).\n", total);
}

/* ========================================================================== */
/*                          RF03 — PESQUISA DE AÇÕES                          */
/* ========================================================================== */

/*
 * Pesquisa por código (igualdade exata), escola ou tema (trecho do texto,
 * sem diferenciar maiúsculas de minúsculas). Mostra todos os resultados.
 */
static void pesquisarAcoes(const Acao *acoes, int total) {
    char termo[TAM_TEXTO];
    int criterio, i, encontrados = 0;

    printf("\n--- PESQUISAR ACAO ---\n");
    if (total == 0) {
        printf("Nenhuma acao cadastrada ainda.\n");
        return;
    }

    printf(" 1 - Por codigo\n 2 - Por escola\n 3 - Por tema\n");
    criterio = lerInteiro("Pesquisar por: ", 1, 3);
    lerTexto("Digite o termo de busca: ", termo, TAM_TEXTO);

    for (i = 0; i < total; i++) {
        int achou = 0;
        if (criterio == 1)
            achou = iguaisIgnorandoCaso(acoes[i].codigo, termo);
        else if (criterio == 2)
            achou = contemIgnorandoCaso(acoes[i].escola, termo);
        else
            achou = contemIgnorandoCaso(acoes[i].tema, termo);

        if (achou) {
            imprimirAcao(&acoes[i]);
            encontrados++;
        }
    }

    if (encontrados == 0)
        printf("[AVISO] Nenhum registro encontrado para \"%s\".\n", termo);
    else
        printf("\n%d registro(s) encontrado(s).\n", encontrados);
}

/* ========================================================================== */
/*        RF04 — ATUALIZAÇÃO DA SITUAÇÃO DO DIA E REGISTRO DE PARTICIPAÇÃO     */
/* ========================================================================== */

/*
 * Pede ao usuário quantos participantes escolheram cada opção de proteína e
 * de arroz. Como cada participante escolhe UMA proteína e UM arroz, a soma
 * das proteínas deve ser igual à soma dos arroz; se não for, repete a coleta.
 * Os valores só são gravados no dia depois de passarem na validação.
 */
static void registrarParticipacao(Dia *dia) {
    int i, soma_prot, soma_arroz;
    int cont_prot[MAX_OPCOES], cont_arroz[MAX_OPCOES];
    char mensagem[100];

    while (1) {
        soma_prot = 0;
        soma_arroz = 0;

        printf("\nInforme quantos participantes escolheram cada opcao:\n");
        for (i = 0; i < dia->qtd_proteinas; i++) {
            sprintf(mensagem, "  Proteina \"%s\": ", dia->proteinas[i].nome);
            cont_prot[i] = lerInteiro(mensagem, 0, MAX_PARTICIP);
            soma_prot += cont_prot[i];
        }
        for (i = 0; i < dia->qtd_arroz; i++) {
            sprintf(mensagem, "  Arroz \"%s\": ", dia->arroz[i].nome);
            cont_arroz[i] = lerInteiro(mensagem, 0, MAX_PARTICIP);
            soma_arroz += cont_arroz[i];
        }

        if (soma_prot == soma_arroz) break;

        printf("  [ERRO] Total de proteinas (%d) diferente do total de arroz (%d).\n"
               "         Cada participante escolhe uma proteina e um arroz. Digite novamente.\n",
               soma_prot, soma_arroz);
    }

    /* Validação aprovada: grava as contagens no dia */
    for (i = 0; i < dia->qtd_proteinas; i++) dia->proteinas[i].participantes = cont_prot[i];
    for (i = 0; i < dia->qtd_arroz; i++)     dia->arroz[i].participantes = cont_arroz[i];
    dia->total_participantes = soma_prot;
}

/*
 * Pede um código e devolve o índice da ação no vetor, ou -1 se o código
 * não existir ("registro inexistente"). Mostra a mensagem de erro.
 */
static int selecionarAcao(const Acao *acoes, int total) {
    char codigo[TAM_CODIGO];
    int indice;

    if (total == 0) {
        printf("Nenhuma acao cadastrada ainda.\n");
        return -1;
    }
    lerTexto("Codigo da acao: ", codigo, TAM_CODIGO);
    indice = buscarPorCodigo(acoes, total, codigo);
    if (indice == -1) {
        printf("[ERRO] Nenhuma acao encontrada com o codigo \"%s\".\n", codigo);
    }
    return indice;
}

static void atualizarSituacao(Acao *acoes, int total) {
    int indice, d, nova;
    Dia *dia;

    printf("\n--- ATUALIZAR SITUACAO DO DIA ---\n");
    indice = selecionarAcao(acoes, total);
    if (indice == -1) return;

    printf("Acao \"%s\" - %s\n", acoes[indice].codigo, acoes[indice].escola);
    for (d = 0; d < DIAS_SEMANA; d++) {
        printf(" %d - %-8s (%s)\n", d + 1, NOMES_DIAS[d],
               nomeSituacao(acoes[indice].dias[d].situacao));
    }
    d = lerInteiro("Qual dia deseja atualizar? ", 1, DIAS_SEMANA) - 1;
    dia = &acoes[indice].dias[d];

    /* Um dia já realizado tem participação registrada: não pode ser alterado
     * (mantém a consistência dos dados e a regra do RF06). */
    if (dia->situacao == REALIZADA) {
        printf("[ERRO] Este dia ja foi marcado como realizado e nao pode mais ser alterado.\n");
        return;
    }

    printf(" 1 - Planejada\n 2 - Realizada\n 3 - Cancelada\n");
    nova = lerInteiro("Nova situacao: ", 1, 3);

    if (nova == REALIZADA) {
        registrarParticipacao(dia);   /* só marca como realizada com participação */
    } else {
        dia->total_participantes = 0;
    }
    dia->situacao = nova;

    printf("[OK] %s de \"%s\" agora esta: %s.\n", NOMES_DIAS[d],
           acoes[indice].codigo, nomeSituacao(nova));
}

/* ========================================================================== */
/*                RF06 — ALTERAÇÃO / CORREÇÃO DE UMA AÇÃO                     */
/* ========================================================================== */

/*
 * Permite corrigir dados gerais ou o cardápio de um dia, desde que a ação
 * ainda NÃO tenha nenhum dia marcado como realizado. O código não pode ser
 * alterado (é o identificador único da ação).
 */
static void alterarAcao(Acao *acoes, int total) {
    int indice, opcao, d;
    Acao *a;

    printf("\n--- ALTERAR DADOS DE UMA ACAO ---\n");
    indice = selecionarAcao(acoes, total);
    if (indice == -1) return;
    a = &acoes[indice];

    if (possuiDiaRealizado(a)) {
        printf("[ERRO] A acao \"%s\" ja possui dia realizado e nao pode mais ser alterada.\n",
               a->codigo);
        return;
    }

    printf(" 1 - Escola\n 2 - Tema\n 3 - Responsavel\n 4 - Publico-alvo\n"
           " 5 - Data prevista\n 6 - Quantidade prevista de participantes\n"
           " 7 - Cardapio de um dia\n 0 - Cancelar\n");
    opcao = lerInteiro("O que deseja alterar? ", 0, 7);

    switch (opcao) {
        case 1: lerTexto("Nova escola: ", a->escola, TAM_TEXTO); break;
        case 2: lerTexto("Novo tema: ", a->tema, TAM_TEXTO); break;
        case 3: lerTexto("Novo responsavel: ", a->responsavel, TAM_TEXTO); break;
        case 4: lerTexto("Novo publico-alvo: ", a->publico_alvo, TAM_TEXTO); break;
        case 5: lerData("Nova data prevista (dd/mm/aaaa): ", a->data_prevista); break;
        case 6:
            a->qtd_prevista = lerInteiro("Nova quantidade prevista por dia: ",
                                         1, MAX_PARTICIP);
            break;
        case 7:
            for (d = 0; d < DIAS_SEMANA; d++) {
                printf(" %d - %s\n", d + 1, NOMES_DIAS[d]);
            }
            d = lerInteiro("Qual dia? ", 1, DIAS_SEMANA) - 1;
            lerCardapioDia(&a->dias[d], NOMES_DIAS[d]);  /* refaz o dia (volta a Planejada) */
            break;
        case 0:
            printf("Alteracao cancelada.\n");
            return;
    }
    printf("[OK] Acao \"%s\" atualizada com sucesso.\n", a->codigo);
}

/* ========================================================================== */
/*                         RF05 — RESUMO GERAL                                */
/* ========================================================================== */

/*
 * Percorre todas as ações e dias, contando situações e somando participantes.
 *
 * Percentual de participação = participantes atendidos / participantes
 * previstos. Como a previsão é "por dia", o total previsto considera apenas
 * os dias REALIZADOS (previsto_por_dia * dias realizados). Assim, dias ainda
 * planejados ou cancelados não distorcem o percentual.
 */
static void gerarResumo(const Acao *acoes, int total) {
    int i, d;
    int planejados = 0, realizados = 0, cancelados = 0;
    long participantes = 0, previstos = 0;

    printf("\n--- RESUMO GERAL ---\n");
    if (total == 0) {
        printf("Nenhuma acao cadastrada ainda.\n");
        return;
    }

    for (i = 0; i < total; i++) {
        for (d = 0; d < DIAS_SEMANA; d++) {
            switch (acoes[i].dias[d].situacao) {
                case PLANEJADA: planejados++; break;
                case CANCELADA: cancelados++; break;
                case REALIZADA:
                    realizados++;
                    participantes += acoes[i].dias[d].total_participantes;
                    previstos     += acoes[i].qtd_prevista;
                    break;
            }
        }
    }

    printf("Acoes cadastradas  : %d\n", total);
    printf("Dias planejados    : %d\n", planejados);
    printf("Dias realizados    : %d\n", realizados);
    printf("Dias cancelados    : %d\n", cancelados);
    printf("Total de participantes atendidos: %ld\n", participantes);

    if (previstos > 0) {
        printf("Participacao em relacao ao previsto (dias realizados): %.1f%%\n",
               (double)participantes * 100.0 / (double)previstos);
    } else {
        printf("Participacao em relacao ao previsto: nenhum dia realizado ainda.\n");
    }
}
