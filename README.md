# 📦 Projeto: Toy File System (ToyFS)

Este projeto tem como objetivo a criação de um sistema de arquivos virtual de 8 bits, baseado em FUSE (Filesystem in Userspace), inteiramente mantido em memória. O sistema simula operações básicas de arquivos e diretórios, proporcionando uma visão prática do funcionamento interno de sistemas de arquivos.

---

## 📌 Descrição

O ToyFS é um sistema de arquivos experimental que implementa funcionalidades básicas como criação, leitura, escrita e remoção de arquivos e diretórios. Ele é montado em memória e não possui persistência em disco. Sua arquitetura simples visa fins educacionais e foi inspirada em projetos como Hellofs e Gogislenefs.

---

## 🧰 Tecnologias Utilizadas

- Linguagem C (implementação principal do sistema de arquivos)
- Biblioteca FUSE (User-space file system interface)
- Linux Ubuntu (desenvolvimento e testes)
  
---

## ⚙️ Como usar o sistema

### 1. Compile os arquivos:

```bash
make
```

### 2. Crie uma imagem do sistema de arquivos com 10 MB:

```bash
./toyfs_format -o toyfs.img -s 10240
```

### 3. Monte o sistema com FUSE:

```bash
sudo ./toyfs_loader toyfs.img /mnt/toyfs
```

### 4. Realize operações básicas:

```bash
echo "Olá Mundo" > /mnt/toyfs/arquivo.txt
cat /mnt/toyfs/arquivo.txt
mkdir /mnt/toyfs/teste_dir
ls /mnt/toyfs
```

### 5. Desmonte o sistema:

```bash
sudo fusermount -u /mnt/toyfs
```
---

## 🔬 Funcionalidades Suportadas
- Criação e remoção de arquivos
- Escrita e leitura de conteúdo
- Criação e remoção de diretórios
- Navegação em estrutura hierárquica

---

## 📑 Relatório

O relatório completo do projeto, incluindo objetivos, estrutura de dados utilizada, código comentado e testes realizados, está disponível no arquivo Relatorio.pdf.

---

##👨‍💻 Autores

- Filipe Gabriel
- Marcus Vinícius
