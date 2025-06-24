#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "janela.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>

// Definições
#define LINHAS 9
#define COLUNAS 10
#define MAX_CORES 8
int num_cores = 4;
int tempoEtapa = 30;
int larguraJanela = 10 * COLUNAS;
int alturaJanela = 10 * LINHAS;

// Tipos
typedef enum { NENHUM, LINHA, COLUNA } tipo_selecao_t;

typedef struct {
    tipo_selecao_t tipo;
    int indice;
} selecao_t;

typedef struct {
    unsigned char dados[LINHAS][COLUNAS];
    int linhas;
    int colunas;
} Tabuleiro;

typedef struct {
    int pontuacao;
    char nome[50];
} jogador;

typedef struct {
    tamanho_t tamanho_janela;
    retangulo_t contorno_janela;
    double data_inicio;
    bool terminou;
    int etapa;
    double tempo_de_jogo;
} jogo;

typedef struct {
    char nome[50];
    int pontuacao;
    int etapa;
} recorde_t;

recorde_t *ranking = NULL;
int num_recordes = 0;

// cores
cor_t cores[MAX_CORES + 1] = {
    {0, 0, 0, 1},                        // Fundo
    {1, 0, 0, 1},                        // Vermelho
    {0, 1, 0, 1},                        // Verde
    {0, 0, 1, 1},                        // Azul
    {1, 1, 0, 1},                        // Amarelo
    {0.5, 0, 0.5, 1},                    // Roxo
    {1, 165.0/255, 0, 1},                // Laranja
    {1, 105.0/255, 180.0/255, 1},        // Rosa
    {165.0/255, 42.0/255, 42.0/255, 1}   // Marrom
};

// Funções utilitárias
double relogio() { return al_get_time(); }
int aleatorio(int min, int max) { return min + rand() % (max - min); }
void limpa_eventos() { while (j_tecla() != T_NADA); }

void processa_tempo(jogo *pj) {
    pj->tempo_de_jogo = j_relogio() - pj->data_inicio;
    if (pj->tempo_de_jogo >= tempoEtapa) pj->terminou = true;
}

// ----------- Funções de Tabuleiro --------------
void gerar_tabuleiro(Tabuleiro *tab) {
    // Zera toda a matriz
    memset(tab->dados, 0, sizeof(tab->dados));

    // Escolhe uma cor obrigatória que estará em todas as linhas
    unsigned char cor_obrigatoria = (unsigned char)aleatorio(1, num_cores + 1);

    // Insere essa cor uma vez em cada linha em posições aleatórias
    for (int i = 0; i < tab->linhas; i++) {
        int coluna_aleatoria = aleatorio(0, tab->colunas);  // corrigido para incluir coluna 0
        tab->dados[i][coluna_aleatoria] = cor_obrigatoria;
    }

    // Preenche o restante das células que estão vazias
    for (int i = 0; i < tab->linhas; i++) {
        for (int j = 0; j < tab->colunas; j++) {
            if (tab->dados[i][j] == 0) {
                tab->dados[i][j] = (unsigned char)aleatorio(1, num_cores + 1);
            }
        }
    }
}

// Desenha o quadrado que representa uma célula do tabuleiro
void desenha_celula(int lin, int col, unsigned char valor, jogo *pj) {
    ponto_t origem = {
        pj->contorno_janela.inicio.x + col * 60,
        pj->contorno_janela.inicio.y + lin * 60
    };
    tamanho_t tam = {58, 58};
    retangulo_t celula = {origem, tam};

    cor_t cor = (valor <= MAX_CORES) ? cores[valor] : cores[0];

    j_retangulo(celula, 0, cor, cor);
    j_retangulo(celula, 2, (cor_t){1,1,1,1}, (cor_t){0,0,0,0});
}

// Desenha o tabuleiro inteiro
void desenha_tabuleiro(Tabuleiro *tab, jogo *pj) {
    for (int i = 0; i < tab->linhas; i++) {
        for (int j = 0; j < tab->colunas; j++) {
            desenha_celula(i, j, tab->dados[i][j], pj);
        }
    }
}

// Aplica a gravidade nas colunas movendo os blocos para baixo
void aplicarGravidade(Tabuleiro *tab) {
   for (int col = 0; col < tab->colunas; col++) {
        int destino = tab->linhas - 1;

        for (int origem = tab->linhas - 1; origem >= 0; origem--) {
            if (tab->dados[origem][col] != 0) {
                tab->dados[destino][col] = tab->dados[origem][col];
                if (destino != origem) {
                    tab->dados[origem][col] = 0;
                }
                destino--;
            }
        }

        // Preenche o topo com zeros, se sobrou espaço
        while (destino >= 0) {
            tab->dados[destino][col] = 0;
            destino--;
        }
    }
}

bool mover_linha(Tabuleiro *tab, int lin, int direcao) {
    if (lin < 0 || lin >= tab->linhas) return false;

    // Se o numero for maior que 0 move pra direita, se for menor move pra esquerda
    if (direcao > 0) { 
        unsigned char ultimo = tab->dados[lin][tab->colunas - 1];
        memmove(&tab->dados[lin][1], &tab->dados[lin][0], (tab->colunas - 1) * sizeof(unsigned char));
        tab->dados[lin][0] = ultimo;
    } else { 
        unsigned char primeiro = tab->dados[lin][0];
        memmove(&tab->dados[lin][0], &tab->dados[lin][1], (tab->colunas - 1) * sizeof(unsigned char));
        tab->dados[lin][tab->colunas - 1] = primeiro;
    }

    for (int j = 0; j < tab->colunas; j++) {
        aplicarGravidade(tab);
    }

    return true;
}

bool mover_coluna(Tabuleiro *tab, int col, int direcao) {
    int destino = (col + direcao + tab->colunas) % tab->colunas;

    // Verifica se o topo da coluna de destino está livre
    if (tab->dados[0][destino] != 0) return false;

    // Procura de baixo para cima o primeiro bloco na coluna de origem
    int origem = -1;
    for (int i = tab->linhas - 1; i >= 0; i--) {
        if (tab->dados[i][col] != 0) {
            origem = i;
            break;
        }
    }

    if (origem == -1) return false;

    //Salva o valor antes de apagar
    unsigned char valor = tab->dados[origem][col];
    tab->dados[origem][col] = 0;

    //Insere no destino na posição mais baixa que estiver livre
    for (int i = tab->linhas - 1; i >= 0; i--) {
        if (tab->dados[i][destino] == 0) {
            tab->dados[i][destino] = valor;
            break;
        }
    }
    aplicarGravidade(tab);

    return true;
}

void verifica_linhas_vazias(Tabuleiro *tab) {
     for (int i = 0; i < tab->linhas; i++) {
        bool vazia = true;
        for (int j = 0; j < tab->colunas; j++)
            if (tab->dados[i][j] != 0) { vazia = false; break; }
        if (vazia) {
            for (int j = 0; j < tab->colunas; j++)
                tab->dados[i][j] = aleatorio(1, num_cores + 1);
            aplicarGravidade(tab);
        }
    }
}

// -------- Pontuação e Etapas --------

bool verifica_coluna_iguais(Tabuleiro *tab, int col) {
    unsigned char ref = tab->dados[0][col];
    if (ref == 0) return false;

    for (int i = 1; i < tab->linhas; i++) {
        if (tab->dados[i][col] != ref) {
            return false;
        }
    }

    for (int i = 0; i < tab->linhas; i++) {
        tab->dados[i][col] = 0;
    }

    return true;
}

void adicionarPontos(Tabuleiro *tab, jogador *jog) {
    for (int col = 0; col < tab->colunas; col++) {
        if (verifica_coluna_iguais(tab, col)) {
            jog->pontuacao += tab->linhas;
        }
    }
}

void limpaLinha(Tabuleiro *tab, int lin, jogador *jog) {
    for (int j = 0; j < tab->colunas; j++) {
        tab->dados[lin][j] = 0;
    }
    aplicarGravidade(tab);
    // Diminui a pontuação do jogador
    jog->pontuacao -= 2;
}

void verificaEtapa(jogo *pj, jogador *jog, Tabuleiro *tab) {
    int meta = (tab->linhas * tab->colunas) / 2 * pj->etapa;
    if (jog->pontuacao >= meta) {
        pj->etapa += 1;
        meta = 0;
        // Aumento gradual das cores conforme a etapa
        if (pj->etapa == 2) num_cores = 5;
        else if (pj->etapa == 3) num_cores = 6;
        else if (pj->etapa == 4) num_cores = 7;
        else if (pj->etapa >= 5) num_cores = 8;
    
        gerar_tabuleiro(tab);
        pj->data_inicio = relogio();
    }
}

// ---------- Entrada do mouse e seletores ----------

void processa_clique(selecao_t *sel, jogo *pj) {
    rato_t rato = j_rato();

    if (rato.clicado[0]) {
        float x = rato.posicao.x;
        float y = rato.posicao.y;

        // Verifica linha
        for (int i = 0; i < LINHAS; i++) {
            ponto_t pos = {pj->contorno_janela.inicio.x - 40, pj->contorno_janela.inicio.y + i * 60 + 20};
            if (x >= pos.x && x <= pos.x + 30 && y >= pos.y && y <= pos.y + 30) {
                sel->tipo = LINHA;
                sel->indice = i;
                return;
            }
        }

        // Verifica coluna
        for (int j = 0; j < COLUNAS; j++) {
            ponto_t pos = {pj->contorno_janela.inicio.x + j * 60 + 20, pj->contorno_janela.inicio.y + LINHAS * 60 + 10};
            if (x >= pos.x && x <= pos.x + 30 && y >= pos.y && y <= pos.y + 30) {
                sel->tipo = COLUNA;
                sel->indice = j;
                return;
            }
        }
    }
}

void insereNome(char *texto, int max_len, ponto_t pos, const char *mensagem) {
    char buffer[100] = "";
    int tamanho = 0;
    bool digitando = true;

    while (digitando) {
        j_retangulo((retangulo_t){pos, {500, 50}}, 0, (cor_t){0, 0, 0, 1}, (cor_t){0, 0, 0, 1});

        char exibir[200];
        snprintf(exibir, sizeof(exibir), "%s %s_", mensagem, buffer);
        j_texto(pos, (cor_t){1,1,1,1}, exibir);

        j_mostra();

        tecla_t tecla = j_tecla();

        if (tecla >= 32 && tecla <= 126) {
            if (tamanho < (int)sizeof(buffer) - 1) {
                buffer[tamanho++] = (char)tecla;
                buffer[tamanho] = '\0';
            }
        } else if (tecla == T_BACKSPACE) {
            if (tamanho > 0) {
                buffer[--tamanho] = '\0';
            }
        } else if (tecla == T_ENTER) {
            digitando = false;
        }
    }

    if (strlen(buffer) == 0) {
        strcpy(buffer, "Anonimo");
    }

    strncpy(texto, buffer, max_len - 1);
    texto[max_len - 1] = '\0';
}

// --------- Desenho da tela ---------
void desenha_seletores(selecao_t sel, jogo *pj) {
    cor_t normal = {0.5, 0.5, 0.5, 1};
    cor_t selecionado = {1, 0.5, 0, 1};

    for (int i = 0; i < LINHAS; i++) {
        ponto_t pos = {pj->contorno_janela.inicio.x - 40, pj->contorno_janela.inicio.y + i * 60 + 20};
        retangulo_t r = {pos, {30, 30}};
        cor_t cor = (sel.tipo == LINHA && sel.indice == i) ? selecionado : normal;
        j_retangulo(r, 2, cor, cor);
    }

    for (int j = 0; j < COLUNAS; j++) {
        ponto_t pos = {pj->contorno_janela.inicio.x + j * 60 + 20, pj->contorno_janela.inicio.y + LINHAS * 60 + 10};
        retangulo_t r = {pos, {30, 30}};
        cor_t cor = (sel.tipo == COLUNA && sel.indice == j) ? selecionado : normal;
        j_retangulo(r, 2, cor, cor);
    }
}

void desenha_tela(Tabuleiro *tab, jogador *jog, jogo *pj, selecao_t sel) {
    j_retangulo(pj->contorno_janela, 3, (cor_t){1,1,1,1}, (cor_t){0,0,0,0});

    desenha_tabuleiro(tab, pj);
    desenha_seletores(sel, pj);

    ponto_t texto = {pj->contorno_janela.inicio.x, pj->contorno_janela.inicio.y + pj->contorno_janela.tamanho.altura + 80};
    char info[200];
    sprintf(info, "Tempo: %2.f | Pontos: %d | Etapa: %d", pj->tempo_de_jogo, jog->pontuacao, pj->etapa);
    j_texto(texto, (cor_t){1,1,1,1}, info);

    j_mostra();
}

// -------- Funções pra inicializar o jogo --------
void inicializaJogo (jogador *jog, jogo *pj, Tabuleiro *tab){
    pj->tamanho_janela = (tamanho_t){1000, 900};
    pj->contorno_janela = (retangulo_t){{100, 100}, {60 * COLUNAS, 60 * LINHAS}};
    pj->etapa = 1;
    pj->terminou = false;

    tab->linhas = LINHAS;
    tab->colunas = COLUNAS;
    gerar_tabuleiro(tab);

    j_inicializa(pj->tamanho_janela, "Jogo das Cores");
    al_show_mouse_cursor(al_get_current_display()); // Mostra o mouse

    pj->data_inicio = relogio();
    pj->tempo_de_jogo = 0.0;
}

// ------ Reinicia partida ------
void reiniciaPartida(Tabuleiro *tab, jogador *jog, jogo *pj) {
    pj->etapa = 1;
    pj->terminou = false;
    jog->pontuacao = 0;
    pj->data_inicio = relogio();
    gerar_tabuleiro(tab);
}

// --------- Função para salvar a pontuação ---------
void lerRanking() {
    FILE *arq = fopen("ranking.bin", "rb");
    if (!arq) {
        ranking = NULL;
        num_recordes = 0;
        return;
    }
    fread(&num_recordes, sizeof(int), 1, arq);
    ranking = malloc(num_recordes * sizeof(recorde_t));
    fread(ranking, sizeof(recorde_t), num_recordes, arq);
    fclose(arq);
}

void gravarRanking() {
    FILE *arq = fopen("ranking.bin", "wb");
    if (!arq) return;
    fwrite(&num_recordes, sizeof(int), 1, arq);
    fwrite(ranking, sizeof(recorde_t), num_recordes, arq);
    fclose(arq);
}

void inserirRecorde(const char *nome, int pontos, int etapa) {
    ranking = realloc(ranking, (num_recordes + 1) * sizeof(recorde_t));
    strcpy(ranking[num_recordes].nome, nome);
    ranking[num_recordes].pontuacao = pontos;
    ranking[num_recordes].etapa = etapa;
    num_recordes++;

    // Ordena os recordes por pontuação (decrescente)
    for (int i = 0; i < num_recordes - 1; i++) {
        for (int j = i + 1; j < num_recordes; j++) {
            if (ranking[j].pontuacao > ranking[i].pontuacao) {
                recorde_t temp = ranking[i];
                ranking[i] = ranking[j];
                ranking[j] = temp;
            }
        }
    }
}

void mostrarRanking() {
    limpa_eventos();
    ponto_t pos = {200, 150};
    while (true) {
        al_clear_to_color(al_map_rgb_f(0, 0, 0));

        if (num_recordes == 0) {
            j_texto(pos, (cor_t){1,1,1,1}, "Nenhum ranking disponível.");
        } else {
            ponto_t atual = pos;
            for (int i = 0; i < num_recordes; i++) {
                char linha[200];
                snprintf(linha, sizeof(linha), "%d - %s: %d pontos (Etapa %d)", i+1, ranking[i].nome, ranking[i].pontuacao, ranking[i].etapa);
                j_texto(atual, (cor_t){1,1,1,1}, linha);
                atual.y += 30;
            }
        }

        j_texto((ponto_t){200, pos.y + 30 * (num_recordes + 2)}, (cor_t){1,1,1,1}, "Pressione ESC para voltar.");
        j_mostra();

        tecla_t tecla = j_tecla();
        if (tecla == T_ESC) break;
    }
}

// --------- Desenho Menu final ---------
void menuFinal(Tabuleiro *tab, jogador *jog, jogo *pj, selecao_t sel) {
    bool menu_ativo = true;
    limpa_eventos();

    while (menu_ativo) {
        ponto_t pos = {pj->contorno_janela.inicio.x + 20, pj->contorno_janela.inicio.y + 150};

        al_clear_to_color(al_map_rgb_f(0, 0, 0));

        j_texto(pos, (cor_t){1,1,1,1}, "1 - Ver Ranking");
        pos.y += 40;
        j_texto(pos, (cor_t){1,1,1,1}, "2 - Reiniciar Jogo");
        pos.y += 40;
        j_texto(pos, (cor_t){1,1,1,1}, "3 - Sair");
        pos.y += 60;
        j_texto(pos, (cor_t){1,1,1,1}, "Escolha uma opcao");

        j_mostra();

        tecla_t tecla = j_tecla();

        if (tecla == '1') {
            mostrarRanking();
        } else if (tecla == '2') {
            reiniciaPartida(tab, jog, pj);
            menu_ativo = false;
        } else if (tecla == '3' || tecla == T_END) {
            pj->terminou = true;
            menu_ativo = false;
        }
    }
}

//------- Desenha a tela final --------

void desenha_tela_final(Tabuleiro *tab, jogador *jog, jogo *pj, selecao_t sel) {
    j_retangulo(pj->contorno_janela, 3, (cor_t){1,1,1,1}, (cor_t){0,0,0,0});

    ponto_t texto = {pj->contorno_janela.inicio.x + 20, pj->contorno_janela.inicio.y + 20};

    j_texto(texto, (cor_t){1,1,1,1}, "Fim de Jogo!");
    j_mostra();
    al_rest(2.0);

    ponto_t pos_nome = {texto.x, texto.y + 60};
    insereNome(jog->nome, sizeof(jog->nome), pos_nome, "Digite seu nome: ");
    j_mostra();

    inserirRecorde(jog->nome, jog->pontuacao, pj->etapa);
    gravarRanking();

    menuFinal(tab, jog, pj, sel);
}


// --------- Função Principal ---------
int main() {
    srand(time(NULL));
    jogador jog;
    jogo pj;
    Tabuleiro tab;
    selecao_t selecao = {NENHUM, -1};
    inicializaJogo(&jog, &pj, &tab);
    lerRanking();

    while (!pj.terminou) {
        processa_clique(&selecao, &pj);
        processa_tempo(&pj);
        tecla_t tecla = j_tecla();
        desenha_tela(&tab, &jog, &pj, selecao);
        adicionarPontos(&tab, &jog);
        verificaEtapa(&pj, &jog, &tab);
        verifica_linhas_vazias(&tab);
        if (selecao.tipo == LINHA) {
            if (tecla == T_DIREITA) mover_linha(&tab, selecao.indice, +1);
            if (tecla == T_ESQUERDA) mover_linha(&tab, selecao.indice, -1);
        }

        if (selecao.tipo == COLUNA) {
            if (tecla == T_DIREITA) mover_coluna(&tab, selecao.indice, +1);
            if (tecla == T_ESQUERDA) mover_coluna(&tab, selecao.indice, -1);
        }

        if (tecla == 'l' || tecla == 'L') {
            limpaLinha(&tab, selecao.indice, &jog);
        }

        if (pj.terminou){
            desenha_tela_final(&tab, &jog, &pj, selecao);
        }
        

        if (tecla == T_END) pj.terminou = true;
    }

    

    free(ranking);

    j_finaliza();
    return 0;
}
