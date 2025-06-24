#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> 
#include <string.h>
#include "janela.h"

// includes do allegro
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

// Definições do tabuleiro
#define LINHAS 6
#define COLUNAS 5

// Definições gerais
#define TAMANHO_TABULEIRO 6
#define TAMANHO_CASA 60
#define MARGEM 100

typedef enum { NENHUM, LINHA, COLUNA } tipo_selecao_t;

typedef struct {
    tipo_selecao_t tipo;
    int indice; // linha ou coluna selecionada
} selecao_t;

//Definição do tabuleiro
typedef struct {
    unsigned char dados[LINHAS][COLUNAS];
    int linhas;
    int colunas;
} Tabuleiro;

// Variáveis globais
int NUMCOR = 4;
int MAX_CORES = 8;
int ponto_por_peca = 1;


//cores globais
cor_t fundo = {0, 0, 0};
cor_t red = {255, 0, 0};
cor_t green = {0, 255, 0};
cor_t blue = {0, 0, 255};
cor_t yellow = {255, 255, 0};
cor_t white = {255, 255, 255};
cor_t purple = {128, 0, 128};
cor_t orange = {255, 165, 0};
cor_t pink = {255, 105, 180};
cor_t brown = {165, 42, 42};

double j_relogio(void) {
    return al_get_time();
}

static int aleatorio_entre(int minimo, int maximo) {
    return minimo + rand() % (maximo - minimo); 
}

//estado do jogo
typedef struct {
    // constantes, devem ser inicializadas antes da janela
    tamanho_t tamanho_janela;
    retangulo_t contorno_janela;
    // necessários no início de cada partida
    tecla_t tecla;
    double data_inicio;
    bool terminou;
    // não precisam de estado inicial
    double tempo_de_jogo;
    int etapa;
} jogo;

typedef struct {
    int pontuacao;
    char nome[50];
} jogador;

void gerarTab(Tabuleiro *tab) {
    for (int i = 0; i < tab->linhas; i++) {
        for (int j = 0; j < tab->colunas; j++) {
            tab->dados[i][j] = (unsigned char)aleatorio_entre(1, NUMCOR + 1);
        }
    }
}

// inicializa estado do jogo
void inicializa_jogo(jogo *pj, jogador *player, Tabuleiro *tab) {
    pj->tecla = 'x';
    player->pontuacao = 0;
    pj->data_inicio = j_relogio();
    pj->terminou = false;
    pj->etapa = 1;
    pj->tempo_de_jogo = 0.0;
    
    // Inicializa dimensões do tabuleiro
    tab->linhas = LINHAS;
    tab->colunas = COLUNAS;
    
    // Inicializa o tabuleiro
    gerarTab(tab);
}

// funções que processam entradas
void processa_tempo(jogo *pj) {
    pj->tempo_de_jogo = j_relogio() - pj->data_inicio;
    if (pj->tempo_de_jogo >= 10) pj->terminou = true;
}

void imprimeCor(int lin, int col, unsigned char valor, jogo *pj) {
    ponto_t origem;
    origem.x = pj->contorno_janela.inicio.x + col * 60;
    origem.y = pj->contorno_janela.inicio.y + lin * 60;
    tamanho_t tam = { 58, 58 }; // margem de 2px
    retangulo_t celula = { origem, tam };

    cor_t cor;
    switch (valor) {
        case 1: cor = red; break;
        case 2: cor = green; break;
        case 3: cor = blue; break;
        case 4: cor = yellow; break;
        case 5: cor = purple; break;
        case 6: cor = orange; break;
        case 7: cor = pink; break;
        case 8: cor = brown; break;
        default: cor = fundo; break;
    }

    j_retangulo(celula, 0, cor, cor); // preenchido
    j_retangulo(celula, 2, (cor_t){1,1,1,1}, (cor_t){0,0,0,0}); // contorno branco
}

void imprimirTabuleiro(Tabuleiro *tab, jogo *pj) {
    for (int i = 0; i < tab->linhas; i++) {
        for (int j = 0; j < tab->colunas; j++) {
            imprimeCor(i, j, tab->dados[i][j], pj);
        }
    }
}

// Faz um elemento "cair" em uma coluna até encontrar outra peça ou o fundo
void fazer_elemento_cair(Tabuleiro *tab, int col_destino, unsigned char elemento) {
    int linha_cair;
    for (linha_cair = 0; linha_cair < tab->linhas; linha_cair++) {
        if (tab->dados[linha_cair][col_destino] != 0) {
            break;
        }
    }
    if (linha_cair > 0) {
        tab->dados[linha_cair-1][col_destino] = elemento;
    }
}

// Aplica gravidade em uma coluna
void aplicar_gravidade_coluna(Tabuleiro *tab, int col) {
    for (int i = tab->linhas - 1; i > 0; i--) {
        if (tab->dados[i][col] == 0 && tab->dados[i - 1][col] != 0) {
            tab->dados[i][col] = tab->dados[i - 1][col];
            tab->dados[i - 1][col] = 0;
        }
    }
}

// Move linha para direita
bool move_lin_para_direita(Tabuleiro *tab, int lin, jogo *pj) {
    if (lin < 0 || lin >= tab->linhas) return false;

    unsigned char ultimoElemento = tab->dados[lin][tab->colunas-1];
    for (int j = tab->colunas-1; j > 0; j--) {
        tab->dados[lin][j] = tab->dados[lin][j-1];
    }
    tab->dados[lin][0] = ultimoElemento;

    for (int i = 0; i < tab->colunas; i++) {
        aplicar_gravidade_coluna(tab, i);
    }

    imprimirTabuleiro(tab, pj);
    return true;
}

// Move linha para esquerda
bool move_lin_para_esquerda(Tabuleiro *tab, int lin, jogo *pj) {
    if (lin < 0 || lin >= tab->linhas) return false;

    unsigned char primeiroElemento = tab->dados[lin][0];
    for (int j = 0; j < tab->colunas-1; j++) {
        tab->dados[lin][j] = tab->dados[lin][j+1];
    }
    tab->dados[lin][tab->colunas-1] = primeiroElemento;

    for (int i = 0; i < tab->colunas; i++) {
        aplicar_gravidade_coluna(tab, i);
    }

    imprimirTabuleiro(tab, pj);
    
    return true;
}

// Função para mover coluna (-1 para esquerda, +1 para direita)
bool mover_coluna(Tabuleiro *tab, int col, int direcao) {
    int col_destino = (col + direcao + tab->colunas) % tab->colunas;

    // Verifica se a coluna de destino tem espaço no topo
    if (tab->dados[0][col_destino] != 0) return false;

    // Pega o último elemento da coluna original
    unsigned char ultimo_elemento = tab->dados[tab->linhas - 1][col];
    if (ultimo_elemento == 0) return false;

    // Move o elemento para a coluna de destino
    fazer_elemento_cair(tab, col_destino, ultimo_elemento);

    // Remove o elemento da coluna original
    tab->dados[tab->linhas - 1][col] = 0;

    aplicar_gravidade_coluna(tab, col);

    return true;
}

// Funções específicas para esquerda/direita
bool move_col_para_esquerda(Tabuleiro *tab, int col) {
    return mover_coluna(tab, col, -1);
}

bool move_col_para_direita(Tabuleiro *tab, int col) {
    return mover_coluna(tab, col, +1);
}

// Verificar se as cores da coluna são iguais
bool verificaCol(Tabuleiro *tab, int col, int *pontuacao) {
    unsigned char primeiro = tab->dados[0][col];
    if (primeiro == 0) return false;
    
    for (int i = 1; i < tab->linhas; i++) {
        if (tab->dados[i][col] != primeiro) {
            return false;
        }
    }

    // Remove a coluna e adiciona pontos
    for (int i = 0; i < tab->linhas; i++) {
        tab->dados[i][col] = 0;
    }
    
    return true;
}

// Verifica se a linha está vazia
bool verificaLinha(Tabuleiro *tab, int lin) {
    for (int j = 0; j < tab->colunas; j++) {
        if (tab->dados[lin][j] != 0) {
            return false;
        }
    }
    return true;
}

// Limpa uma linha inteira
void limpar_linha(Tabuleiro *tab, int lin, int *pontuacao) {
    if (lin < 0 || lin >= tab->linhas) return;
    
    for (int j = 0; j < tab->colunas; j++) {
        tab->dados[lin][j] = 0;
    }
    // Adiciona penalidade de 2 pontos
    *pontuacao -= 2;
    
    // Aplica gravidade em todas as colunas
    for (int j = 0; j < tab->colunas; j++) {
        aplicar_gravidade_coluna(tab, j);
    }
}

// Função para ler o nome do jogador
void ler_nome_jogador(jogador *j) {
    printf("Digite seu nome: ");
    fgets(j->nome, 50, stdin);
    
    // Remove o \n do final
    j->nome[strcspn(j->nome, "\n")] = '\0';
    
    // Se o usuário não digitou nada usa "anonimo"
    if (strlen(j->nome) == 0) {
        strcpy(j->nome, "Anonimo");
    }
}

// Salva a pontuação do jogador
void salvarPontuacao(jogador *j) {
    FILE *file = fopen("pontuacao.txt", "a");
    if (file != NULL) {
        fprintf(file, "Jogador: %s\nPontuação: %d\n\n", j->nome, j->pontuacao);
        fclose(file);
    } else {
        printf("Erro ao abrir o arquivo de pontuação.\n");
    }
}


// Reinicia o jogo
void reiniciar_jogo(jogo *pj, jogador *player, Tabuleiro *tab) {
    pj->tecla = 'x';
    player->pontuacao = 0;
    pj->data_inicio = j_relogio();
    pj->terminou = false;
    pj->etapa = 1;
    pj->tempo_de_jogo = 0.0;
    gerarTab(tab);
}

// Verifica se o jogador passou de etapa
void verificaEtapa(jogo *pj, jogador *player, Tabuleiro *tab) {
    if (player->pontuacao >= (tab->linhas * tab->colunas) / 2) {
        pj->etapa++;
        // Bônus por passar de etapa
        player->pontuacao += 50 * (pj->etapa - 1);
        // Aumenta número de cores  
        NUMCOR += 1;
        if (NUMCOR > MAX_CORES) {
            // Limita o número máximo de cores
            NUMCOR = MAX_CORES; 
        }
        // Recria o tabuleiro com as novas cores
        gerarTab(tab);
        pj->data_inicio = j_relogio();
    }
}

// Verifica a linha que está vazia e preenche
void verificaLinhaVazia(Tabuleiro *tab) {
    for (int lin = 0; lin < tab->linhas; lin++) {
        if (verificaLinha(tab, lin)) {
            for (int col = 0; col < tab->colunas; col++) {
                tab->dados[lin][col] = (unsigned char)aleatorio_entre(1, NUMCOR + 1);
            }
        }
    }
}

//adiciona pontos ao jogador
void adicionar_pontos(jogador *player, Tabuleiro *tab) {
    for (int col = 0; col < tab->colunas; col++) {
        if(verificaCol(tab, col, player->pontuacao)) {
            // Adiciona pontos ao jogador
            player->pontuacao += ponto_por_peca * tab->linhas;
        }
    } 
}

// Verifica se o jogador quer jogar novamente
bool jogarNovamente(jogo *pj, jogador *player, Tabuleiro *tab) {
    char tecla;
    printf("Deseja jogar novamente?\n");
    printf("Pressione 's' para sim ou qualquer outra tecla para sair: ");
    scanf(" %c", &tecla);
    if (tecla == 's' || tecla == 'S') {
        // Reinicia o jogo
        reiniciar_jogo(pj, player, tab);
        return true;
    }
    return false;
}


// Desenhos na tela
void desenha_tela(Tabuleiro *tab, jogador *j, jogo *pj) {
    j_limpapix(); // limpa a tela

    // Desenha contorno do tabuleiro
    j_retangulo(pj->contorno_janela, 3, (cor_t){1,1,1,1}, (cor_t){0,0,0,0});

    desenha_tabuleiro(tab, pj);
    desenha_info(j, pj);

    j_mostra(); // atualiza a janela
}

void desenha_tela_final(jogador *j, jogo *pj) {
    char txt[100];
    ponto_t pos = { 
        pj->contorno_janela.inicio.x, pj->contorno_janela.inicio.y 
        + pj->contorno_janela.tamanho.altura + 20 
    };
    sprintf(txt, "Jogador: %s  |  Pontos: %d  |  Etapa: %d  |  Tempo: %.1f", j->nome, j->pontuacao, pj->etapa, pj->tempo_de_jogo);
    j_texto(pos, (cor_t){1, 1, 1, 1}, txt);
}

void processa_clique_mouse(selecao_t *selecao) {
    rato_t rato = j_rato();

    if (rato.clicado[0]) {
        float x = rato.posicao.x;
        float y = rato.posicao.y;

        // Verifica clique nas linhas
        for (int i = 0; i < TAMANHO_TABULEIRO; i++) {
            float bx = MARGEM - 30;
            float by = MARGEM + i * TAMANHO_CASA + TAMANHO_CASA/2 - 15;
            if (x >= bx && x <= bx + 30 && y >= by && y <= by + 30) {
                selecao->tipo = LINHA;
                selecao->indice = i;
                return;
            }
        }

        // Verifica clique nas colunas
        for (int j = 0; j < TAMANHO_TABULEIRO; j++) {
            float bx = MARGEM + j * TAMANHO_CASA + TAMANHO_CASA/2 - 15;
            float by = MARGEM + TAMANHO_TABULEIRO * TAMANHO_CASA + 10;
            if (x >= bx && x <= bx + 30 && y >= by && y <= by + 30) {
                selecao->tipo = COLUNA;
                selecao->indice = j;
                return;
            }
        }
    }
}

void desenha_seletores(selecao_t selecao) {
    cor_t cor_padrao = { 0.5, 0.5, 0.5, 1 };
    cor_t cor_selecionado = { 1, 0.5, 0, 1 };

    // Desenha seletores de linha (à esquerda)
    for (int i = 0; i < TAMANHO_TABULEIRO; i++) {
        ponto_t pos = { MARGEM - 30, MARGEM + i * TAMANHO_CASA + TAMANHO_CASA/2 - 15 };
        retangulo_t ret = { pos, {30, 30} };
        cor_t cor = (selecao.tipo == LINHA && selecao.indice == i) ? cor_selecionado : cor_padrao;
        j_retangulo(ret, 2, cor, cor);

        char txt[2];
        sprintf(txt, "%d", i + 1);
        j_texto((ponto_t){pos.x + 5, pos.y + 22}, (cor_t){1,1,1,1}, txt);
    }

    // Desenha seletores de coluna (abaixo)
    for (int j = 0; j < TAMANHO_TABULEIRO; j++) {
        ponto_t pos = { MARGEM + j * TAMANHO_CASA + TAMANHO_CASA/2 - 15, MARGEM + TAMANHO_TABULEIRO * TAMANHO_CASA + 10 };
        retangulo_t ret = { pos, {30, 30} };
        cor_t cor = (selecao.tipo == COLUNA && selecao.indice == j) ? cor_selecionado : cor_padrao;
        j_retangulo(ret, 2, cor, cor);

        char txt[2];
        sprintf(txt, "%d", j + 1);
        j_texto((ponto_t){pos.x + 5, pos.y + 22}, (cor_t){1,1,1,1}, txt);
    }
}
void desenha_tabuleiro(Tabuleiro *tab, jogo *pj) {
    // Desenha o tabuleiro
    imprimirTabuleiro(tab, pj);
    
    // Desenha os seletores de linha e coluna
    selecao_t selecao = { NENHUM, -1 };
    processa_clique_mouse(&selecao);
    desenha_seletores(selecao);
}


int main() {
    srand(time(NULL)); // Inicializa gerador de números aleatórios
    // Inicializa Allegro
    if (!al_init()) {
        fprintf(stderr, "Falha ao inicializar a Allegro.\n");
        return -1;
    }

    // cria e inicializa o descritor do jogo
    jogador j;
    jogo jogo;
    Tabuleiro tab;

    jogo.tamanho_janela = (tamanho_t){ 700, 500 };
    jogo.contorno_janela = (retangulo_t){{ 30, 30 }, { 440, 440 }};

    j_inicializa(jogo.tamanho_janela, "Jogo das Cores"); // inicializa a janela gráfica
    inicializa_jogo(&jogo, &j, &tab); // Inicializa o jogo

    //laço principal
    while(!jogo.terminou) {
        desenha_tela(&tab, &j, &jogo); // Desenha a tela
        // Processa o tempo de jogo
        processa_tempo(&jogo);
        //adiciona pontos ao jogador quando completa uma coluna
        adicionar_pontos(&j, &tab);
        // Verifica se passou de etapa
        verificaEtapa(&jogo, &j, &tab);
        // Verifica linhas vazias e preenche
        verificaLinhaVazia(&tab);
        // Verifica se o jogador quer jogar novamente
        if (jogo.terminou) {
            salvarPontuacao(&j);
            ler_nome_jogador(&j);
            if(jogarNovamente(&jogo, &j, &tab)) continue;
            else break;
        }
    }

    j_finaliza();
}