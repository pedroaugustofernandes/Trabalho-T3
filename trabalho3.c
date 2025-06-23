#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> 
#include <string.h>
#include "video.h"
#include "teclado.h"

// includes do allegro
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

// Definições do tabuleiro
#define LINHAS 6
#define COLUNAS 5


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

typedef struct {
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

void imprimeCor(Tabuleiro *tab, int i, int j) {
    switch (tab->dados[i][j]) {
        case 0: vid_cor_fundo(fundo); vid_imps("   "); break;
        case 1: vid_cor_fundo(red); vid_imps("   "); break;
        case 2: vid_cor_fundo(green); vid_imps("   "); break;
        case 3: vid_cor_fundo(blue); vid_imps("   "); break;
        case 4: vid_cor_fundo(yellow); vid_imps("   "); break;
        case 5: vid_cor_fundo(purple); vid_imps("   "); break;
        case 6: vid_cor_fundo(orange); vid_imps("   "); break;
        case 7: vid_cor_fundo(pink); vid_imps("   "); break;
        case 8: vid_cor_fundo(brown); vid_imps("   "); break;
        default: vid_cor_fundo(white); vid_imps("   ");
    }
    vid_cor_fundo(fundo);
}

void imprimirTabela(Tabuleiro *tab) {
    vid_limpa();
    vid_cor_fundo(fundo);
    
    for (int i = 0; i < tab->linhas; i++) {
        for (int j = 0; j < tab->colunas; j++) {
            imprimeCor(tab, i, j);
        }
        vid_imps("\n\r");
    }
    vid_imps("\n\r");
    vid_atualiza();
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
bool move_lin_para_direita(Tabuleiro *tab, int lin) {
    if (lin < 0 || lin >= tab->linhas) return false;

    unsigned char ultimoElemento = tab->dados[lin][tab->colunas-1];
    for (int j = tab->colunas-1; j > 0; j--) {
        tab->dados[lin][j] = tab->dados[lin][j-1];
    }
    tab->dados[lin][0] = ultimoElemento;

    for (int i = 0; i < tab->colunas; i++) {
        aplicar_gravidade_coluna(tab, i);
    }

    imprimirTabela(tab);
    return true;
}

// Move linha para esquerda (com toroidal)
bool move_lin_para_esquerda(Tabuleiro *tab, int lin) {
    if (lin < 0 || lin >= tab->linhas) return false;

    unsigned char primeiroElemento = tab->dados[lin][0];
    for (int j = 0; j < tab->colunas-1; j++) {
        tab->dados[lin][j] = tab->dados[lin][j+1];
    }
    tab->dados[lin][tab->colunas-1] = primeiroElemento;

    for (int i = 0; i < tab->colunas; i++) {
        aplicar_gravidade_coluna(tab, i);
    }

    imprimirTabela(tab);
    
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
        if(verificaCol(&tab, col, player->pontuacao)) {
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

// Jogar
void jogar() {
    jogador j;
    jogo jogo;
    Tabuleiro tab;

    inicializa_jogo(&jogo, &j, &tab);
    
    vid_inicia();

    vid_cor_fundo(fundo);
    vid_atualiza();
    vid_limpa();

    imprimirTabela(&tab);
    sleep(2);

    while (!jogo.terminou) {
        processa_tempo(&jogo);
        //adiciona pontos ao jogador quando completa uma coluna
        adicionar_pontos(&j, &tab);

        j.pontuacao += 10;

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

        // Movimentos das linhas
        move_lin_para_direita(&tab, 0);
        move_lin_para_esquerda(&tab, 1);
        move_lin_para_direita(&tab, 2);
        move_lin_para_esquerda(&tab, 3);
        move_lin_para_direita(&tab, 4);
        move_lin_para_esquerda(&tab, 5);
        // Movimentos das colunas
        move_col_para_direita(&tab, 0);
        move_col_para_esquerda(&tab, 1);        
        move_col_para_direita(&tab, 2);
        move_col_para_esquerda(&tab, 3);
        move_col_para_direita(&tab, 4);

    }

    vid_fim();
}

int main() {
    // Inicializa gerador de números aleatórios
    srand(time(NULL));
    // Inicializa Allegro
    if (!al_init()) {
        fprintf(stderr, "Falha ao inicializar a Allegro.\n");
        return -1;
    }
    
    jogar();
    return 0;
}