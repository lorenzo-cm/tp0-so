#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* MARK NAME Lorenzo Carneiro Magalhães */
/* MARK NAME Tomas Lacerda Muniz */

/****************************************************************
 * Shell xv6 simplificado
 *
 * Este codigo foi adaptado do codigo do UNIX xv6 e do material do
 * curso de sistemas operacionais do MIT (6.828).
 ***************************************************************/

#define MAXARGS 10

/* Todos comandos tem um tipo.  Depois de olhar para o tipo do
 * comando, o código converte um *cmd para o tipo específico de
 * comando. */
struct cmd {
  int type; /* ' ' (exec)
               '|' (pipe)
               '<' or '>' (redirection) */
};

struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // argumentos do comando a ser exec'utado
};

struct redircmd {
  int type;          // < ou > 
  struct cmd *cmd;   // o comando a rodar (ex.: um execcmd)
  char *file;        // o arquivo de entrada ou saída
  int mode;          // o modo no qual o arquivo deve ser aberto
  int fd;            // o número de descritor de arquivo que deve ser usado
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // lado esquerdo do pipe
  struct cmd *right; // lado direito do pipe
};

int fork1(void);  // Fork mas fechar se ocorrer erro.
struct cmd *parsecmd(char*); // Processar o linha de comando.

/* Executar comando com cmd apropriado.  Nunca retorna. */
void
runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit(0);
  
  switch(cmd->type){

  default:
    fprintf(stderr, "tipo de comando desconhecido\n");
    exit(-1);

  case ' ':
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit(0);

    /* MARK START task2
     * TAREFA2: Implemente codigo abaixo para executar
     * comandos simples. */

    // apenas execução do execvp com adequacao aos parametros
    execvp(ecmd->argv[0], ecmd->argv);

    /* MARK END task2 */

    break;

  case '>':
  case '<':
    rcmd = (struct redircmd*)cmd;

    /* MARK START task3
     * TAREFA3: Implemente codigo abaixo para executar
     * comando com redirecionamento. */
    
    // não precisa verificar o tipo pois a propria classe ja tem tudo incluso

    // Abre o arquivo conforme especificado na estrutura redircmd
    int current_fd = open(rcmd->file, rcmd->mode, S_IRWXU);

    if (current_fd < 0) {
      // Verifica se a abertura do arquivo foi bem-sucedida
      perror("open file failed"); // perror é usada em fork1 então assumo que é o correto
      exit(1);
    }

    // Duplica o descritor para o especificado em rcmd->fd
    if (dup2(current_fd, rcmd->fd) < 0) {
      perror("dup2 failed");
      close(current_fd);
      exit(1);
    }
    
    // Fecha o descritor
    close(current_fd); 

    // rodar com os files descriptors configurados
    runcmd(rcmd->cmd);

    /* MARK END task3 */
    break;

  case '|':
    pcmd = (struct pipecmd*)cmd;

    /* MARK START task4
     * TAREFA4: Implemente codigo abaixo para executar
     * comando com pipes. */

    // Deve ser algo recursivo, pode ter multiplos pipes

    // Descricao do metodo pipe
    /* Create a one-way communication channel (pipe).
    If successful, two file descriptors are stored in PIPEDES;
    bytes written on PIPEDES[1] can be read from PIPEDES[0].
    Returns 0 if successful, -1 if not.  */

    if(pipe(p) == -1) {
      perror("Pipe failed");
      exit(1);
    }
    
    // Usarei a funcao feita por padrao que ja lida com o erro do fork
    r = fork1();
    
    if(r == 0) {
      // Processo filho vai executar parte esquerda -> direcionar output para outro comando

      // Fecha o lado de leitura
      close(p[0]);

      // Redireciona stdout para o lado de escrita (direito) do pipe
      dup2(p[1], STDOUT_FILENO);

      // Fecha o descritor de escrita do pipe (aparentemente esse passo é necessario)
      close(p[1]); 

      // Executa o comando do lado esquerdo recursivamente
      runcmd(pcmd->left); 
    }
    
    else {
      // Processo pai aguarda o filho terminar, receber a entrada do comando previo executado e executar 
      wait(NULL);

      // Fecha o lado de escrita que não é utilizado
      // Aparentemente precisa fechar mesmo após o fechamento dele pelo filho, pelo que entendi é uma cópia
      close(p[1]);

      // Redireciona stdin para o lado de leitura do pipe
      dup2(p[0], STDIN_FILENO);

      // Fecha o descritor de leitura do pipe (aparentemente esse passo é necessario)
      close(p[0]);

      // Executa o comando do lado direito recursivamente
      runcmd(pcmd->right);
    }

    /* MARK END task4 */
    break;


  }
  exit(0);
}

/**
 * @brief Lê uma linha de comando do usuário, armazenando-a em um buffer.
 * 
 * Exibe um prompt (`$ `) se a entrada padrão é um terminal. Se o fim do arquivo (EOF) é encontrado
 * antes de qualquer caractere ser lido, a função retorna -1, indicando que não há mais entrada disponível.
 * 
 * @param buf Ponteiro para o buffer onde a linha de comando será armazenada.
 * @param nbuf O tamanho do buffer fornecido. A função garante que não ultrapasse esse limite.
 * @return int Retorna 0 se uma linha foi lida com sucesso; retorna -1 se o fim do arquivo foi encontrado imediatamente.
 */
int
getcmd(char *buf, int nbuf)
{
  if (isatty(fileno(stdin)))
    fprintf(stdout, "$ ");
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int
main(void)
{
  static char buf[100];
  int r;

  // Ler e rodar comandos.
  while(getcmd(buf, sizeof(buf)) >= 0){
    
    /* MARK START task1 */
    /* TAREFA1: O que faz o if abaixo e por que ele é necessário?
     * O if abaixo trata do comando cd (change directory)
     * Caso dê erro, ele é reportado */

    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      buf[strlen(buf)-1] = 0;
      if(chdir(buf+3) < 0)
        fprintf(stderr, "comando cd falhou\n");
      continue;
    }
    
    /* MARK END task1 */

    if(fork1() == 0)
      runcmd(parsecmd(buf));
    wait(&r);
  }
  exit(0);
}

/**
 * @brief Faz um fork, retorna o pid e lida com eventual falha
 * 
 * @return int Retorna o pid do processo e -1 caso ele falhe
 */
int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    perror("fork");
  return pid;
}

/****************************************************************
 * Funcoes auxiliares para criar estruturas de comando
 ***************************************************************/

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->mode = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}


/**
 * @brief Cria uma estrutura de comando que representa um pipeline entre dois comandos.
 * 
 * Aloca e inicializa uma estrutura de comando do tipo pipeline, onde o comando à esquerda
 * do pipe é executado e sua saída é conectada à entrada do comando à direita do pipe.
 * 
 * @param left Ponteiro para a estrutura de comando que será executada à esquerda do pipe.
 * @param right Ponteiro para a estrutura de comando que será executada à direita do pipe.
 * @return struct cmd* Ponteiro para a estrutura de comando do tipo pipeline criada.
 */
struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

/****************************************************************
 * Processamento da linha de comando
 ***************************************************************/

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";


/**
 * @brief Identifica e extrai o próximo token da string de comando.
 * 
 * Avança sobre espaços, identifica tokens como comandos, argumentos ou símbolos (e.g., '|', '<', '>'),
 * e atualiza os ponteiros para delimitar o token. Usada para análise sintática de comandos.
 * 
 * @param ps Endereço do ponteiro para a posição atual, atualizado após extração.
 * @param es Ponteiro para o final da string de comando.
 * @param q (Se não NULL) Atualizado para o início do token.
 * @param eq (Se não NULL) Atualizado para o final do token.
 * @return Caractere que indica o tipo do token, ou 0 se nenhum for encontrado.
 */
int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}


/**
 * @brief Verifica se o token especificado é o próximo na string de comandos
 * 
 * Avança verificando se o próximo caractere não nulo é um token reconhecido.
 * Não consome o token, permitindo inspeções futuras.
 * 
 * @param ps Ponteiro para o ponteiro da posição atual na string de comando. Será atualizado para apontar para o próximo caractere não branco.
 * @param es Ponteiro para o final da string de comando.
 * @param toks String contendo os caracteres token a serem procurados.
 * @return int Retorna 1 (verdadeiro) se o próximo caractere não branco é um dos tokens, 0 (falso) caso contrário.
 */
int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

/* Copiar os caracteres no buffer de entrada, comeando de s ate es.
 * Colocar terminador zero no final para obter um string valido. */
char 
*mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}


/**
 * @brief Analisa uma string de comando para construir uma representação estruturada de comandos.
 * 
 * Divide a string de comando em tokens e constrói uma árvore de comandos
 * correspondente, que pode representar comandos simples, pipelines, e redirecionamentos.
 * Em caso de erro de sintaxe ou se a análise não consumir toda a entrada, termina o programa.
 * 
 * @param s String de comando a ser analisada.
 * @return struct cmd* Ponteiro para a estrutura de comando construída.
 */
struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  // aponta para o final de s, ou seja, o caracter \0
  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}


/**
 * @brief Inicia o parsing para saber o comando a ser executado
 * 
 * @param ps Ponteiro para o início da string de comando a ser analisada.
 * @param es Ponteiro para o final da string de comando.
 * @return struct cmd* Estrutura do comando representando o comando a ser executado
 */
struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  // começa parsando o pipe
  cmd = parsepipe(ps, es);
  return cmd;
}


/**
 * @brief Analisa pipelines entre comandos na string fornecida.
 * 
 * Recursivamente constrói estruturas de comandos para representar pipelines
 * Chama a função parseexec, caso for o comando de exec
 * 
 * @param ps Ponteiro atual na string de comando.
 * @param es Ponteiro para o final da string de comando.
 * @return struct cmd* Estrutura do comando representando o pipeline.
 */
struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  // caso for o comando de exec
  cmd = parseexec(ps, es);

  // caso haja o caracter de pipe, deve-se fazer a analise dos dois lados
  // por isso a recursão
  if(peek(ps, es, "|")){

    // ps -> pointer to the start -- aponta para o começo da string de comando
    // string de comando não é a string toda, ela
    // es -> end of the string -- aponta para \0

    // no caso ps e es vao apontar para o comeco e final do segundo comando
    // e assim por diante caso hajam mais pipes
    gettoken(ps, es, 0, 0);

    // retorna um pipecmd (precisa de 2 cmds, left e right)
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }

  return cmd;
}


/**
 * @brief Analisa e aplica redirecionamentos de entrada/saída a uma estrutura de comando.
 *
 * Verifica se tem os tokens <>, caso haja, verifica para ver se tem um arquivo especificado
 * isso pode ser visto pois gettoken retorna argumento padrao 'a'
 *
 * @param cmd A estrutura de comando a ser modificada com os redirecionamentos.
 * @param ps Endereço do ponteiro para a posição atual na string de comando.
 * @param es Ponteiro para o final da string de comando.
 * @return struct cmd* Retorna o redircmd com o redirecionamento especificado
 */
struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch(tok){
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}


/**
 * @brief Analisa comandos individuais e seus argumentos da string de comando.
 * 
 * Constrói uma estrutura de comando executável com os argumentos fornecidos,
 * lidando com redirecionamentos de entrada/saída conforme necessário.
 * 
 * @param ps Ponteiro atual na string de comando.
 * @param es Ponteiro para o final da string de comando.
 * @return struct cmd* Estrutura de comando executável construída a partir da análise.
 */
struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;

  // verificar encaminhamento antes
  // ex) > file.out command arg1
  ret = parseredirs(ret, ps, es);

  while(!peek(ps, es, "|")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }

    // verificar encaminhamento depois
    // ex) command arg1 > file.out
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}

// vim: expandtab:ts=2:sw=2:sts=2

