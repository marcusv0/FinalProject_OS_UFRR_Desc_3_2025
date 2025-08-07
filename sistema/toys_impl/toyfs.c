
#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define MAX_CHILDREN 64
#define MAX_NAME_LEN 128
#define MAX_FILE_SIZE 4096

// Definição dos tipos de nós (arquivo ou diretório)
typedef enum { FILE_NODE, DIR_NODE } NodeType;

// Estrutura de dados de um nó virtual em memória (sem persistência ainda)
typedef struct Node {
    char name[MAX_NAME_LEN];
    NodeType type;
    char data[MAX_FILE_SIZE]; // Apenas para arquivos
    size_t size;
    struct Node *parent;
    struct Node *children[MAX_CHILDREN];
    int child_count;
    mode_t mode;
    time_t mtime;
} Node;

Node *root;
int toyfs_fd = -1; // Arquivo .img aberto (no futuro pode armazenar dados reais)

// Busca um nó dado um caminho (ex: /dir/arquivo.txt)
Node *find_node(const char *path) {
    if (strcmp(path, "/") == 0) return root;

    char temp[512];
    strncpy(temp, path, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';

    Node *curr = root;
    char *token = strtok(temp, "/");
    while (token != NULL && curr) {
        Node *found = NULL;
        for (int i = 0; i < curr->child_count; i++) {
            if (strcmp(curr->children[i]->name, token) == 0) {
                found = curr->children[i];
                break;
            }
        }
        curr = found;
        token = strtok(NULL, "/");
    }
    return curr;
}

// Cria um novo nó filho
Node *create_node(const char *name, NodeType type, Node *parent, mode_t mode) {
    if (!parent || parent->child_count >= MAX_CHILDREN) return NULL;
    Node *node = malloc(sizeof(Node));
    if (!node) return NULL;
    strncpy(node->name, name, MAX_NAME_LEN);
    node->type = type;
    node->size = 0;
    node->parent = parent;
    node->child_count = 0;
    node->mode = mode;
    node->mtime = time(NULL);
    parent->children[parent->child_count++] = node;
    return node;
}

static int toyfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));
    Node *node = find_node(path);
    if (!node) return -ENOENT;

    if (node->type == DIR_NODE) {
        stbuf->st_mode = S_IFDIR | node->mode;
        stbuf->st_nlink = 2;
    } else {
        stbuf->st_mode = S_IFREG | node->mode;
        stbuf->st_nlink = 1;
        stbuf->st_size = node->size;
    }
    stbuf->st_mtime = node->mtime;
    return 0;
}

static int toyfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi,
                        enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;
    Node *node = find_node(path);
    if (!node || node->type != DIR_NODE) return -ENOENT;
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    for (int i = 0; i < node->child_count; i++) {
        filler(buf, node->children[i]->name, NULL, 0, 0);
    }
    return 0;
}

static int toyfs_open(const char *path, struct fuse_file_info *fi) {
    Node *node = find_node(path);
    return (!node || node->type != FILE_NODE) ? -ENOENT : 0;
}

static int toyfs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    (void) fi;
    Node *node = find_node(path);
    if (!node || node->type != FILE_NODE) return -ENOENT;

    if (offset >= node->size) return 0;
    if (offset + size > node->size) size = node->size - offset;

    memcpy(buf, node->data + offset, size);
    return size;
}

static int toyfs_write(const char *path, const char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    (void) fi;
    Node *node = find_node(path);
    if (!node || node->type != FILE_NODE) return -ENOENT;

    if (offset + size > MAX_FILE_SIZE) size = MAX_FILE_SIZE - offset;
    memcpy(node->data + offset, buf, size);
    node->size = (offset + size > node->size) ? offset + size : node->size;
    node->mtime = time(NULL);
    return size;
}

static int toyfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;
    char *slash = strrchr(path, '/');
    if (!slash) return -EINVAL;

    char parent_path[512];
    strncpy(parent_path, path, slash - path);
    parent_path[slash - path] = '\0';

    Node *parent = find_node(strlen(parent_path) ? parent_path : "/");
    if (!parent || parent->type != DIR_NODE) return -ENOENT;

    Node *node = create_node(slash + 1, FILE_NODE, parent, mode);
    return node ? 0 : -ENOMEM;
}

static int toyfs_mkdir(const char *path, mode_t mode) {
    char *slash = strrchr(path, '/');
    if (!slash) return -EINVAL;

    char parent_path[512];
    strncpy(parent_path, path, slash - path);
    parent_path[slash - path] = '\0';

    Node *parent = find_node(strlen(parent_path) ? parent_path : "/");
    if (!parent || parent->type != DIR_NODE) return -ENOENT;

    Node *node = create_node(slash + 1, DIR_NODE, parent, mode);
    return node ? 0 : -ENOMEM;
}

static int toyfs_rmdir(const char *path) {
    Node *node = find_node(path);
    if (!node || node->type != DIR_NODE || node->child_count > 0) return -ENOTEMPTY;

    Node *parent = node->parent;
    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == node) {
            for (int j = i; j < parent->child_count - 1; j++)
                parent->children[j] = parent->children[j + 1];
            parent->child_count--;
            break;
        }
    }
    free(node);
    return 0;
}

// Inicializa o FS e o arquivo .img (modo leitura/escrita)
static void *toyfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    (void) conn;
    cfg->kernel_cache = 1;

    root = malloc(sizeof(Node));
    strcpy(root->name, "/");
    root->type = DIR_NODE;
    root->parent = NULL;
    root->child_count = 0;
    root->mode = 0755;
    root->mtime = time(NULL);
    return NULL;
}

static struct fuse_operations toyfs_oper = {
    .getattr = toyfs_getattr,
    .readdir = toyfs_readdir,
    .open    = toyfs_open,
    .read    = toyfs_read,
    .write   = toyfs_write,
    .create  = toyfs_create,
    .mkdir   = toyfs_mkdir,
    .rmdir   = toyfs_rmdir,
    .init    = toyfs_init,
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ponto_de_montagem> <arquivo_img>\n", argv[0]);
        return 1;
    }

    const char *img_path = argv[2];

    toyfs_fd = open(img_path, O_RDWR);
    if (toyfs_fd < 0) {
        perror("Erro ao abrir imagem .img");
        return 1;
    }

    // Armazene o caminho para uso em outras funções se necessário
    // disk_path = img_path;

    // Executa o FUSE com apenas o ponto de montagem
    char *fuse_argv[] = { argv[0], argv[1], "-f", NULL };
    int fuse_argc = 3;

    return fuse_main(fuse_argc, fuse_argv, &toyfs_oper, NULL);
}
