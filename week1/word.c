#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#define COUNT 10

typedef struct Word
{
    char word[30];
    int line[4000];
    int count;
} Word;

typedef struct node
{
    Word _word;
    struct node *left;
    struct node *right;
} node;
typedef struct node *_node;
_node root = NULL;
FILE *f;
FILE *sw;
int line = 0;
char stopw[18][256];
int size_sw = 0;
int check = 1;
void readwords (FILE *f) {
    char x[1024];
    while (fscanf(f, " %1023s", x) == 1) {
        strcpy(stopw[size_sw++],x);
    }
}

char* tolowerString(char *str){
for(int i = 0; str[i]; i++){
  str[i] = tolower(str[i]);
}
return str;
}

void read_words (FILE *f) {
    char x[1024];
    while (fscanf(f, " %1023s", x) == 1) {
        puts(x);
    }
}

void insert(_node *root, char *token, int line)
{
    if ((*root) == NULL)
    {
        (*root) = (_node)malloc(sizeof(node));
        memset((*root)->_word.line, 0, 4000*4);
        strcpy((*root)->_word.word, token);
        (*root)->_word.count = 1;
        (*root)->_word.line[line] = line;
        (*root)->left = NULL;
        (*root)->right = NULL;
    }
    else if ((strcmp(token, (*root)->_word.word)) < 0)
        insert(&(*root)->left, token, line);
    else if ((strcmp(token, (*root)->_word.word)) > 0)
        insert(&(*root)->right, token, line);
    else if ((strcmp(token, (*root)->_word.word)) == 0)
    {
        (*root)->_word.count += 1;
        if (!(*root)->_word.line[line])
            (*root)->_word.line[line] = line;
    }
}

void print(_node root)
{
    if (root != NULL)
    {
        print(root->left);
        printf("%s %d ", root->_word.word, root->_word.count);
        for (int i = 0; i < 4000 ; i++)
        {
            if(root->_word.line[i] != 0)
                printf("%d ",root->_word.line[i]);
        }
        printf("\n");
        print(root->right);
    }
}

int main()
{

    char *str = (char *)malloc(sizeof(char) * 1000);
    if ((f = fopen("alice30.txt", "r")) == NULL)
    {
        printf("Can't not open file\n");
        return -1;
    }

    if ((sw = fopen("stopw.txt", "r")) == NULL)
    {
        printf("Can't not open file\n");
        return -1;
    }

    readwords(sw);

    while (fgets(str, 1000, f) != NULL)
    {
        if (strcmp(str, "\r\n") == 0)
        {
            line++;
            continue;
        }
        line++;
        char *token = strtok(str, " ");
        while (token != NULL)
        {
            if (strcmp(token, "\r\n") == 0 || strcmp(token, "\n") == 0 || strcmp(token, "\r") == 0 || strcmp(token, "*") == 0)
                break;
            while (!isalpha(token[0]) && !isdigit(token[0]))
            {
                for (int i = 0; i < strlen(token); i++)
                {
                    token[i] = token[i + 1];
                }
            }
            int a = strlen(token) - 1;
            while (!isalpha(token[a]) && !isdigit(token[a]))
            {
                token[a] = '\0';
                a--;
            }
            for(int i = 0 ; i < 18 ; i++)
            {
                if(strcmp(stopw[i],tolowerString(token)) == 0)
                    check = 0;
            }
            if(check)
                insert(&root, tolowerString(token), line);
            token = strtok(NULL, " ");
            check = 1;
        }
    }
    print(root);
    fclose(f);
    fclose(sw);
}