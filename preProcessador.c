#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>

#define TAMANHO_MAXIMO_INCLUDE 256
#define MAX_DEFINES 100
#define MAX_ARGS 10 // Número máximo de argumentos que uma macro com parâmetros pode ter

// Estrutura para armazenar uma definição de #define
typedef struct {
    char* nome;      // Nome da macro (ex: "PI" ou "SOMA")
    char* corpo;     // Corpo/valor da macro (ex: "3.14" ou "((a)+(b))")
    int num_args;    // Número de argumentos (0 para constantes)
    char* args[MAX_ARGS]; // Nomes dos argumentos (ex: "a", "b")
} Definicao;

int salvarCodigoEmArquivo(const char* nome_arquivo, const char* conteudo) {
    FILE *arquivo_saida;

    arquivo_saida = fopen(nome_arquivo, "w");
    if (arquivo_saida == NULL) {
        perror("Erro ao criar o arquivo de saída");
        return 1;
    }

    fputs(conteudo, arquivo_saida);
    fclose(arquivo_saida);

    printf("\n-------------------------------------------------");
    printf("\nCódigo final salvo com sucesso no arquivo: %s\n", nome_arquivo);
    printf("-------------------------------------------------\n");

    return 0;
}

// função auxiliar para verificar se um caractere é um delimitador de token.

int delimitador(char c) {
    return !delimitador(c) && c != '_';
}

 // Expande as diretivas #define (apenas constantes).
char* expansaoDefine(const char* codigo) {
    Definicao defines[MAX_DEFINES];
    int num_defines = 0;

    char* codigo_sem_defines = malloc(strlen(codigo) + 1);
    if (!codigo_sem_defines) return NULL;
    codigo_sem_defines[0] = '\0';

   
    char* codigo_dup = strdup(codigo); 
    if (!codigo_dup) {
        free(codigo_sem_defines);
        return NULL;
    }
    char* linha = strtok(codigo_dup, "\n");

    while (linha != NULL) {
        // Ignora espaços em branco no início da linha
        while (isspace((unsigned char)*linha)) linha++;

        if (strncmp(linha, "#define", 7) == 0) {
            char* ptr = linha + 7;
            while (isspace((unsigned char)*ptr)) ptr++; // Pula espaços

            char* nome = ptr;
            while (!isspace((unsigned char)*ptr) && *ptr != '(' && *ptr != '\0') ptr++;
            
            char tmp = *ptr;
            *ptr = '\0';
            defines[num_defines].nome = strdup(nome);
            *ptr = tmp;

            defines[num_defines].num_args = 0;

            if (*ptr == '(') { 
                ptr++; // Pula o '('
                char* arg_start = ptr;
                while (*ptr != ')' && *ptr != '\0' && defines[num_defines].num_args < MAX_ARGS) {
                    if (*ptr == ',') {
                        *ptr = '\0';
                        defines[num_defines].args[defines[num_defines].num_args++] = strdup(arg_start);
                        *ptr = ',';
                        ptr++;
                        while(isspace((unsigned char)*ptr)) ptr++;
                        arg_start = ptr;
                    } else {
                        ptr++;
                    }
                }
                if (*ptr == ')') {
                    *ptr = '\0';
                    defines[num_defines].args[defines[num_defines].num_args++] = strdup(arg_start);
                    *ptr = ')';
                    ptr++; // Pula o ')'
                }
            }
            
            while (isspace((unsigned char)*ptr)) ptr++;
            defines[num_defines].corpo = strdup(ptr);
            num_defines++;
        } else {
            strcat(codigo_sem_defines, linha);
            strcat(codigo_sem_defines, "\n");
        }
        linha = strtok(NULL, "\n");
    }
    free(codigo_dup);

    
    char* resultado_final = strdup(codigo_sem_defines);
    free(codigo_sem_defines);
    if (!resultado_final) return NULL;

    int substituicoes_feitas;
    do {
        substituicoes_feitas = 0;
        for (int d = 0; d < num_defines; d++) {
            if (defines[d].num_args == 0 && strlen(defines[d].nome) > 0) {
                char* pos = strstr(resultado_final, defines[d].nome);
                while (pos) {
                    int len_nome = strlen(defines[d].nome);
                    if ((pos == resultado_final || delimitador(*(pos - 1))) && delimitador(*(pos + len_nome))) {
                        int len_corpo = strlen(defines[d].corpo);
                        char* temp = malloc(strlen(resultado_final) - len_nome + len_corpo + 1);
                        if (!temp) {

                         }

                        strncpy(temp, resultado_final, pos - resultado_final);
                        temp[pos - resultado_final] = '\0';
                        strcat(temp, defines[d].corpo);
                        strcat(temp, pos + len_nome);
                        
                        free(resultado_final);
                        resultado_final = temp;
                        substituicoes_feitas++;
                        pos = strstr(resultado_final, defines[d].nome);
                    } else {
                        pos = strstr(pos + 1, defines[d].nome);
                    }
                }
            }
        }
    } while (substituicoes_feitas > 0);
    
    // Libera memória das definições
    for (int i = 0; i < num_defines; i++) {
        free(defines[i].nome);
        free(defines[i].corpo);
        for(int j=0; j<defines[i].num_args; j++){
            free(defines[i].args[j]);
        }
    }

    return resultado_final;
}

//  Remove comentários de linha (//) e de bloco (/ ... * /).
char* remocaoComentario(const char* codigo) {
    long tamanho = strlen(codigo);
    char* resultado = malloc(tamanho + 1);
    if (!resultado) return NULL;

    long i = 0, j = 0;
    int linhaComentario=0;
    int blocoComentario=0;   

    int palavra = 0;
    int caracter=0;

    while (codigo[i] != '\0') {
        // Lógica para detectar entrada/saída de string ou char
        if (codigo[i] == '"' && (i == 0 || codigo[i-1] != '\\') && !caracter) {
            palavra = !palavra;
        }
        if (codigo[i] == '\'' && (i == 0 || codigo[i-1] != '\\') && !palavra) {
            caracter = !caracter;
        }

        // Lógica de comentários só é ativada se não estivermos dentro de uma string/char
        if (!palavra && !caracter) {
            if (linhaComentario) {
                if (codigo[i] == '\n') {
                    linhaComentario = 0;
                    resultado[j++] = codigo[i]; // Mantém a quebra de linha
                }
            } else if (blocoComentario) {
                if (codigo[i] == '*' && codigo[i + 1] == '/') {
                    blocoComentario = 0;
                    i++; // Pula o '/' também
                }
            } else {
                if (codigo[i] == '/' && codigo[i + 1] == '/') {
                    linhaComentario = 1;
                    i++; // Pula o segundo '/'
                } else if (codigo[i] == '/' && codigo[i + 1] == '*') {
                    blocoComentario = 1;
                    i++; // Pula o '*'
                } else {
                    resultado[j++] = codigo[i];
                }
            }
        } else {
            // Se estiver dentro de string/char, copia literalmente
            resultado[j++] = codigo[i];
        }
        i++;
    }

    resultado[j] = '\0';
    return resultado;
}

// Expande as diretivas #include "arquivo.h".
char* expansaoInclude(const char* codigo) {
    long tamanhoInicial = strlen(codigo);
    char* resultado = malloc(tamanhoInicial * 5 + 1024); 
    if (!resultado) return NULL;
    resultado[0] = '\0';
    
    char* linha_atual = strdup(codigo);
    if (!linha_atual) { free(resultado); return NULL; }

    char* linha = strtok(linha_atual, "\n");
    while(linha != NULL) {
        char* linha_trim = linha;
        while(isspace((unsigned char)*linha_trim)) linha_trim++;

        if (strncmp(linha_trim, "#include \"", 10) == 0) {
            char nome_arquivo[TAMANHO_MAXIMO_INCLUDE];
            char* inicio = linha_trim + 10;
            char* fim = strchr(inicio, '"');
            
            if (fim) {
                size_t len = fim - inicio;
                if (len < TAMANHO_MAXIMO_INCLUDE) {
                    strncpy(nome_arquivo, inicio, len);
                    nome_arquivo[len] = '\0';

                    FILE* inc = fopen(nome_arquivo, "r");
                    if (inc) {
                        fseek(inc, 0, SEEK_END);
                        long tam = ftell(inc);
                        rewind(inc);
                        
                        char* conteudo_inc = malloc(tam + 1);
                        fread(conteudo_inc, 1, tam, inc);
                        conteudo_inc[tam] = '\0';
                        
                        strcat(resultado, conteudo_inc); 
                        
                        free(conteudo_inc);
                        fclose(inc);
                    } else {
                        fprintf(stderr, "Erro: Não foi possível abrir o arquivo incluído: %s\n", nome_arquivo);
                        strcat(resultado, linha); // Mantém a linha de include se falhar
                    }
                }
            }
        } else {
            strcat(resultado, linha);
        }
        strcat(resultado, "\n");
        linha = strtok(NULL, "\n");
    }

    free(linha_atual);
    return resultado;
}


// Remove espaços, tabulações e quebras de linha desnecessários do código.

char* minificarCodigo(const char* codigo) {
    long tamanho = strlen(codigo);
    char* resultado = malloc(tamanho + 1);
    if (!resultado) return NULL;

    long j = 0;
    int palavra = 0;
    int in_char = 0;

    for (long i = 0; i < tamanho; i++) {
        char atual = codigo[i];
        
        if (atual == '"' && (i == 0 || codigo[i-1] != '\\') && !in_char) palavra = !palavra;
        if (atual == '\'' && (i == 0 || codigo[i-1] != '\\') && !palavra) in_char = !in_char;

        if (palavra || in_char) {
            resultado[j++] = atual;
        } else {
            if (isspace((unsigned char)atual)) {
                 // Adiciona um único espaço se ele estiver entre dois identificadores
                if (j > 0 && !delimitador(resultado[j-1])) {
                    long proximo_i = i;
                    while (proximo_i < tamanho && isspace((unsigned char)codigo[proximo_i])) {
                        proximo_i++;
                    }
                    if (proximo_i < tamanho && !delimitador(codigo[proximo_i])) {
                        resultado[j++] = ' ';
                    }
                }
                // Pula todos os espaços em branco sequenciais
                while (i + 1 < tamanho && isspace((unsigned char)codigo[i+1])) {
                    i++;
                }
            } else {
                resultado[j++] = atual;
            }
        }
    }
    resultado[j] = '\0';
    return resultado;
}

int main() {
    setlocale(LC_ALL, "Portuguese");
    FILE *leitura;
    char enderecoArquivo[100];

    puts("Digite o nome do arquivo com a extensão (Ex: teste.c):");
    scanf("%s", enderecoArquivo);

    leitura = fopen(enderecoArquivo, "r");
    if (leitura == NULL) {
        perror("Erro fatal: Não foi possível abrir o arquivo");
        return 1;
    }

    // 1. LER O CONTEÚDO DO ARQUIVO
    fseek(leitura, 0, SEEK_END);
    long tamanho = ftell(leitura);
    rewind(leitura);
    char* conteudoArquivo = malloc(tamanho + 1);
    if(!conteudoArquivo) { perror("Erro de alocação"); fclose(leitura); return 1; }
    fread(conteudoArquivo, 1, tamanho, leitura);
    conteudoArquivo[tamanho] = '\0';
    fclose(leitura);

    printf("--- Conteúdo Original ---\n%s\n\n", conteudoArquivo);

    // a memória anterior é liberada.
    
    // 2. EXPANDIR INCLUDES
    char* codigoComIncludes = expansaoInclude(conteudoArquivo);
    free(conteudoArquivo);

    // 3. REMOVER COMENTÁRIOS
    char* codigoLimpo = remocaoComentario(codigoComIncludes);
    free(codigoComIncludes);

    // 4. EXPANDIR DEFINES
    char* codigoDefinido = expansaoDefine(codigoLimpo);
    free(codigoLimpo);

    // 5. MINIFICAR (REMOVER ESPAÇOS E QUEBRAS DE LINHA)
    char* codigoMinificado = minificarCodigo(codigoDefinido);
    free(codigoDefinido);
    
    // 6. EXIBIR O RESULTADO FINAL NO CONSOLE
    printf("--- Código Pré-Processado e Minificado ---\n%s\n", codigoMinificado);
    
    // 7. GERAR NOME DO ARQUIVO DE SAÍDA E SALVAR O RESULTADO
    //    (ex: "teste.c" -> "teste_processado.c")
    char nome_arquivo_saida[150];
    char* ponto = strrchr(enderecoArquivo, '.'); // Encontra a última ocorrência de '.'

    if (ponto != NULL) {
        // Se encontrou uma extensão, insere "_processado" antes dela
        int posicao_ponto = ponto - enderecoArquivo;
        strncpy(nome_arquivo_saida, enderecoArquivo, posicao_ponto);
        nome_arquivo_saida[posicao_ponto] = '\0'; // Finaliza a primeira parte da string
        strcat(nome_arquivo_saida, "_processado"); // Adiciona o sufixo
        strcat(nome_arquivo_saida, ponto);         // Adiciona o ponto e a extensão de volta
    } else {
        // Se não encontrou extensão, apenas adiciona o sufixo
        snprintf(nome_arquivo_saida, sizeof(nome_arquivo_saida), "%s_processado", enderecoArquivo);
    }
    
    // Chama a nova função para salvar o conteúdo no arquivo gerado
    salvarCodigoEmArquivo(nome_arquivo_saida, codigoMinificado);
    
    // 8. LIBERAR A MEMÓRIA FINAL
    free(codigoMinificado);

    return 0;
}