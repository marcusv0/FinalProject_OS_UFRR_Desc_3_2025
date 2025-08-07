// toyfs_format.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#define TOYFS_MAGIC "TOYFS1.0"
#define BLOCK_SIZE 512
#define NUM_BLOCKS 20480  // 10MB / 512 bytes

#define HEADER_BLOCK 0
#define BITMAP_BLOCK 1
#define ROOTDIR_BLOCK 2

void format_toyfs(const char *filename) {
    int fd = open(filename, O_WRONLY);
    if (fd < 0) {
        perror("Erro ao abrir imagem para formatar");
        exit(1);
    }

    // Cabeçalho: escreve a magic string e metadados no primeiro bloco
    char header[BLOCK_SIZE] = {0};
    strncpy(header, TOYFS_MAGIC, strlen(TOYFS_MAGIC));
    uint32_t *meta = (uint32_t *)(header + 16);
    meta[0] = NUM_BLOCKS;
    write(fd, header, BLOCK_SIZE);

    // Bitmap de blocos livres (inicializa blocos 0,1,2 como ocupados)
    uint8_t bitmap[BLOCK_SIZE] = {0};
    bitmap[0] = 0b00000111;
    write(fd, bitmap, BLOCK_SIZE);

    // Diretório raiz vazio (inicialmente todos os bytes zerados)
    char rootdir[BLOCK_SIZE] = {0};
    write(fd, rootdir, BLOCK_SIZE);

    close(fd);
    printf("Imagem formatada com sucesso!\n");
}
void formatar_toyfs(const char *caminho_img) {
    int fd = open(caminho_img, O_RDWR);
    if (fd < 0) {
        perror("Erro ao abrir imagem");
        exit(1);
    }

    uint8_t buffer[BLOCK_SIZE];
    memset(buffer, 0, BLOCK_SIZE);

    // Escreve o cabeçalho
    snprintf((char *)buffer, BLOCK_SIZE, "%s", TOYFS_MAGIC);
    write(fd, buffer, BLOCK_SIZE);

    // Zera o resto dos blocos
    memset(buffer, 0, BLOCK_SIZE);
    for (int i = 1; i < NUM_BLOCKS; i++) {
        write(fd, buffer, BLOCK_SIZE);
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <imagem.img>\n", argv[0]);
        return 1;
    }

    const char *imagem = argv[1];
    formatar_toyfs(imagem);
    printf("Imagem formatada com sucesso: %s\n", imagem);
    return 0;
}


