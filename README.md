# MiniFS

Модель операционной системы, работающая в одном файле.

## Требования

1. Linux
2. Наличие программы `/usr/bin/realpath`, обладающей флагом -m.

## Сборка
```bash
make build
```

## Характеристики
1. Размер блока: 4096 байт.
2. Количество блоков: 2^16 = 65536.
3. Размер: примерно 250 МБ.
4. Максимальная длина пути: как и в Linux.
5. Максимальное имя файла: 40 символов (char).
6. Поддерживается косвенная адресация.


## Управление

1. Форматирование.
```bash
minifs init
```

2. Создание файла/каталога

```bash
minifs create file "/documents/new_file.txt"
minifs create dir "/documents/new_dir"
```

3. Чтение файла/каталога, результат попадает на стандартный поток вывода
```bash
minifs read file "/documents/new_file.txt"
minifs read dir "/documents/new_dir"
```

4. Удаление файла/каталога, для удаления каталог должен быть пустым
```bash
minifs delete file "/documents/new_file.txt"
minifs delete dir "/documents/new_dir"
```

5. Запись файла, осуществляется со стаднартного потока ввода, перед выполнением операции предыдущее содержимое файла стирается
```bash
minifs write "/documents/new_file.txt"
```

Соответственно, если вы хотите сохранить файл из MiniFS себе на диск, то нужно запустить команду
```bash
minifs read "/documents/new_file.txt" > my_file.txt
```
