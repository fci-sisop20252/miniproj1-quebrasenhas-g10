# Relatório: Mini-Projeto 1 - Quebra-Senhas Paralelo

**Aluno(s):** Gabriel Teixeira Bolonha (10426937), Vitor dos Santos Souza (10402809) 
---

## 1. Estratégia de Paralelização


**Como você dividiu o espaço de busca entre os workers?**

O espaço de busca é dividido igualmente entre os workers com base em (tamanho_charset ^ tamanho_senha) / num_workers, e o resto é distribuído nos primeiros. Cada intervalo gera uma senha inicial e final para o worker. O coordenador usa fork e execl para executar os workers e depois wait para sincronizar.

**Código relevante:** Cole aqui a parte do coordinator.c onde você calcula a divisão:
long long total = pow(strlen(charset), password_len);
long long base = total / num_workers;
long long resto = total % num_workers;

long long inicio = i * base + (i < resto ? i : resto);
long long fim = inicio + base - 1 + (i < resto ? 1 : 0);

index_to_password(inicio, charset, password_len, start);
index_to_password(fim, charset, password_len, end);

if (fork() == 0) {
    execl("./worker", "./worker", hash, start, end, charset, argv[3], id_str, NULL);
    exit(1);
}

## 2. Implementação das System Calls

**Descreva como você usou fork(), execl() e wait() no coordinator:**

No coordinator o fork é usado para criar processos filhos, cada filho recebe seu intervalo de busca convertido em senhas inicial e final e em seguida chama execl para substituir sua imagem pelo programa worker passando o hash, os limites, o charset, o tamanho e o id como argumentos enquanto o processo pai apenas continua criando os outros filhos e armazena seus PIDs, após criar todos os workers o pai usa wait em um loop para aguardar a finalização de cada processo garantindo que nenhum fique zumbi.
**Código do fork/exec:**
for (int i = 0; i < num_workers; i++) {
    if (fork() == 0) {
        execl("./worker", "./worker", hash, start, end, charset, argv[3], id_str, NULL);
        exit(1);
    }
}
for (int i = 0; i < num_workers; i++) {
    wait(NULL);
}

## 3. Comunicação Entre Processos

**Como você garantiu que apenas um worker escrevesse o resultado?**

Garantimos que apenas um worker escrevesse o resultado com uma técnica de escrita atômica. O primeiro worker a encontrar a senha usa a chamada de sistema open() com as flags O_CREAT | O_EXCL para criar o arquivo password_found.txt. A flag O_EXCL é crucial, pois ela faz com que a chamada falhe se o arquivo já existir. Isso impede que outros workers sobrescrevam o arquivo e garante que a condição de corrida seja resolvida.

**Como o coordinator consegue ler o resultado?**

O coordinator consegue ler o resultado de forma segura em duas etapas. Primeiro, ele usa a chamada wait() para esperar que todos os workers terminem a sua execução. Em seguida, ele verifica se o arquivo password_found.txt existe. Se existir, o coordinator lê o seu conteúdo, que está no formato id_worker:senha, e faz o parsing da string usando a função strchr() para separar a informação. Por fim, ele exibe a senha encontrada e o ID do worker que a encontrou.
---

## 4. Análise de Performance
Complete a tabela com tempos reais de execução:
O speedup é o tempo do teste com 1 worker dividido pelo tempo com 4 workers.

| Teste | 1 Worker | 2 Workers | 4 Workers | Speedup (4w) |
|-------|----------|-----------|-----------|--------------|
| Hash: 202cb962ac59075b964b07152d234b70<br>Charset: "0123456789"<br>Tamanho: 3<br>Senha: "123" | _0,17_s | _0,09__s | _0,005__s | _3,40__ |
| Hash: 5d41402abc4b2a76b9719d911017c592<br>Charset: "abcdefghijklmnopqrstuvwxyz"<br>Tamanho: 5<br>Senha: "hello" | _5,505__s | _6,781__s | __1,091_s | _5,04__ |

**O speedup foi linear? Por quê?**
O speedup não foi perfeitamente linear. Isso aconteceu devido o overhead de criar, gerencia e sincronizar os processos. Por exemplo, na senha "hello" o tempo com 2 workers foi maior que 1 worker, o que sugere um overhead tão alto que causou uma anomalia, demonstrando a limitação no ganho de velocidade em um sistema paralelo.

---

## 5. Desafios e Aprendizados
**Qual foi o maior desafio técnico que você enfrentou?**
O maior desafio técnico foi o problema de comunicação entre processos. A lógica de divisão de trabalho no coordinator usava índices numéricos, mas o worker esperava receber senhas de string, causando um erro de execução. A solução foi ajustar o coordinator para converter os índices em senhas de string antes de iniciar os workers, alinhando a comunicação entre os programas. 



---

## Comandos de Teste Utilizados

```bash
# Teste básico
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2

# Teste de performance
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 1
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4

# Teste com senha maior
time ./coordinator "5d41402abc4b2a76b9719d911017c592" 5 "abcdefghijklmnopqrstuvwxyz" 4
```
---

**Checklist de Entrega:**
- [X] Código compila sem erros
- [X] Todos os TODOs foram implementados
- [X] Testes passam no `./tests/simple_test.sh`
- [X] Relatório preenchido
