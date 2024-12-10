# Resumo

Sistema de arquivo FAT32 para disciplina de Sistemas Operacionais.

Atualizado por Larissa Fantin, Lucas Borba e Thiago Andrade.

# Instruções de Compilação

### Linux

Este projeto foi feito para Linux somente. Usuários de Windows devem usar WSL.

O sistema deve ter um compilador GCC 11+, com suporte à C11, e GNU Make.

Para compilar, rode:

```
$ make fat32_fs
```

Para executar, rode:

```
$ ./fat32_fs <COMANDO> [ARGUMENTOS] <DISCO>
```

### Windows

Veja [Como instalar o Linux no Windows com o WSL](https://learn.microsoft.com/pt-br/windows/wsl/install), por Microsoft.

# Implementação

Os seguintes comandos foram implementados:

1. Listar   -- ls
2. Mover    -- mv
3. Remover  -- rm
4. Copiar   -- cp
5. Imprimir -- cat

# Exemplos

Por exemplo, para listar os arquivos:

```
$ ./fat32_fs ls disk_fat32.img
```

Para mover um arquivo:

```
$ ./fat32_fs mv teste.txt other.exe disk_fat32.img
```

Para remover um arquivo:

```
$ ./fat32_fs rm texto2.txt disk_fat32.img
```

Para copiar um arquivo:

```
$ ./fat32_fs cp texto2.txt novo.txt disk_fat32.img
```

Para imprimir um arquivo:

```
$ ./fat32_fs cat teste.txt disk_fat32.img
```