#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO COORDENADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 *
 * Este programa coordena múltiplos workers para quebrar senhas MD5 em paralelo.
 * O MD5 JÁ ESTÁ IMPLEMENTADO - você deve focar na paralelização (fork/exec/wait).
 *
 * Uso: ./coordinator <hash_md5> <tamanho> <charset> <num_workers>
 *
 * Exemplo: ./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 4
 *
 * SEU TRABALHO: Implementar os TODOs marcados abaixo
 */

#define MAX_WORKERS 16
#define RESULT_FILE "password_found.txt"

/**
 * Calcula o tamanho total do espaço de busca
 *
 * @param charset_len Tamanho do conjunto de caracteres
 * @param password_len Comprimento da senha
 * @return Número total de combinações possíveis
 */
long long calculate_search_space(int charset_len, int password_len) {
    long long total = 1;
    for (int i = 0; i < password_len; i++) {
        total *= charset_len;
    }
    return total;
}

/**
 * Converte um índice numérico para uma senha
 * Usado para definir os limites de cada worker
 *
 * @param index Índice numérico da senha
 * @param charset Conjunto de caracteres
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @param output Buffer para armazenar a senha gerada
 */
void index_to_password(long long index, const char *charset, int charset_len,
                       int password_len, char *output) {
    for (int i = password_len - 1; i >= 0; i--) {
        output[i] = charset[index % charset_len];
        index /= charset_len;
    }
    output[password_len] = '\0';
}

/**
 * Função principal do coordenador
 */
int main(int argc, char *argv[]) {
    // TODO 1: Validar argumentos de entrada
    // Verificar se argc == 5 (programa + 4 argumentos)
    // Se não, imprimir mensagem de uso e sair com código 1
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <hash_md5> <tamanho> <charset> <num_workers>\n", argv[0]);
        exit(1);
    }

    // Parsing dos argumentos (após validação)
    const char *target_hash = argv[1];
    int password_len = atoi(argv[2]);
    const char *charset = argv[3];
    int num_workers = atoi(argv[4]);
    int charset_len = strlen(charset);

    // TODO: Adicionar validações dos parâmetros
    if (password_len < 1 || password_len > 10) {
        fprintf(stderr, "Erro: O tamanho da senha deve ser entre 1 e 10.\n");
        exit(1);
    }
    if (num_workers < 1 || num_workers > MAX_WORKERS) {
        fprintf(stderr, "Erro: O número de workers deve ser entre 1 e %d.\n", MAX_WORKERS);
        exit(1);
    }
    if (charset_len == 0) {
        fprintf(stderr, "Erro: O conjunto de caracteres (charset) não pode ser vazio.\n");
        exit(1);
    }

    printf("=== Mini-Projeto 1: Quebra de Senhas Paralelo ===\n");
    printf("Hash MD5 alvo: %s\n", target_hash);
    printf("Tamanho da senha: %d\n", password_len);
    printf("Charset: %s (tamanho: %d)\n", charset, charset_len);
    printf("Número de workers: %d\n", num_workers);

    // Calcular espaço de busca total
    long long total_space = calculate_search_space(charset_len, password_len);
    printf("Espaço de busca total: %lld combinações\n\n", total_space);

    // Remover arquivo de resultado anterior se existir
    unlink(RESULT_FILE);

    // Registrar tempo de início
    time_t start_time = time(NULL);

    // TODO 2: Dividir o espaço de busca entre os workers
    long long passwords_per_worker = total_space / num_workers;
    long long remaining = total_space % num_workers;

    // Arrays para armazenar os PIDs dos workers
    pid_t workers[MAX_WORKERS];

    // TODO 3: Criar os processos workers usando fork()
    printf("Iniciando workers...\n");
    // IMPLEMENTE AQUI: Loop para criar workers
    for (int i = 0; i < num_workers; i++) {
        // TODO: Calcular intervalo de senhas para este worker
        long long start_index = i * passwords_per_worker;
        long long end_index;

        if (i < remaining) {
            start_index += i;
            end_index = start_index + passwords_per_worker + 1;
        } else {
            start_index += remaining;
            end_index = start_index + passwords_per_worker;
        }
        
        if (i == num_workers - 1) {
            end_index = total_space;
        }

        // TODO: Converter indices para senhas de inicio e fim
        char start_password[password_len + 1];
        char end_password[password_len + 1];
        index_to_password(start_index, charset, charset_len, password_len, start_password);
        
        if (end_index == total_space) {
            index_to_password(total_space - 1, charset, charset_len, password_len, end_password);
        } else {
            index_to_password(end_index - 1, charset, charset_len, password_len, end_password);
        }

        // TODO 4: Usar fork() para criar processo filho
        pid_t pid = fork();

        if (pid < 0) {
            // TODO 7: Tratar erros de fork() e execl()
            perror("Erro ao criar processo filho (fork)");
            exit(1);
        }

        if (pid == 0) {
            // Processo filho
            char password_len_str[16];
            char worker_id_str[16];

            // Converte os argumentos numéricos para strings
            sprintf(password_len_str, "%d", password_len);
            sprintf(worker_id_str, "%d", i);

            // TODO 6: No processo filho: usar execl() para executar worker
            execl("./worker", "worker", target_hash, start_password, end_password, charset, password_len_str, worker_id_str, (char *)NULL);

            // Se execl() retornar, houve um erro
            perror("Erro ao executar o worker (execl)");
            exit(1);
        } else {
            // TODO 5: No processo pai: armazene o PID
            workers[i] = pid;
            printf("[Coordinator] Worker %d criado (PID=%d), intervalo %s..%s\n",
                   i, pid, start_password, end_password);
        }
    }

    printf("\nTodos os workers foram iniciados. Aguardando conclusão...\n");

    // TODO 8: Aguardar todos os workers terminarem usando wait()
    // IMPORTANTE: O pai deve aguardar TODOS os filhos para evitar zumbis
    int status;
    pid_t ended_pid;

    
    for (int i = 0; i < num_workers; i++) {
        
        ended_pid = wait(&status);
        if (ended_pid == -1) {
            perror("wait");
            continue;
        }

       
        int ended_worker_id = -1;
        for (int j = 0; j < num_workers; j++) {
            if (ended_pid == workers[j]) {
                ended_worker_id = j;
                break;
            }
        }
        
        if (ended_worker_id != -1) {
            printf("[Coordinator] Worker com PID %d terminou", ended_pid);
            
            
            if (WIFEXITED(status)) {
                
                printf(" (status=%d)\n", WEXITSTATUS(status));
            } else {
                printf(" (terminou de forma anormal)\n");
            }
        }
    }

    // Registrar tempo de fim
    time_t end_time = time(NULL);
    double elapsed_time = difftime(end_time, start_time);

    printf("\n=== Resultado ===\n");

    // TODO 9: Verificar se algum worker encontrou a senha
    // Leia e verifique o arquivo de resultado
    char buffer[256];
    char found_password[11];
    char computed_hash[33];
    int worker_id = -1;
    char *colon_pos;
    ssize_t bytes_read;

    int fd = open(RESULT_FILE, O_RDONLY);

    if (fd >= 0) { 
        bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        close(fd);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            
            
            colon_pos = strchr(buffer, ':');
            if (colon_pos) {
                *colon_pos = '\0';
                worker_id = atoi(buffer);
                strcpy(found_password, colon_pos + 1);
                
                
                char *newline_pos = strchr(found_password, '\n');
                if (newline_pos) {
                    *newline_pos = '\0';
                }

                
                md5_string(found_password, computed_hash);
                
                if (strcmp(computed_hash, target_hash) == 0) {
                    
                    printf("✓ Senha encontrada: %s\n", found_password);
                    printf("Verificada com sucesso. Encontrada pelo worker %d.\n", worker_id);
                } else {
                    printf("X Senha encontrada, mas o hash não corresponde.\n");
                }
            } else {
                printf("X Erro de formato no arquivo de resultado.\n");
            }
        } else {
            printf("Senha não encontrada (arquivo de resultado vazio).\n");
        }
    } else {
        printf("Senha não encontrada.\n");
    }

    // Estatísticas finais (opcional)
    printf("Tempo total de execução: %.2f segundos\n", elapsed_time);

    return 0;
}
